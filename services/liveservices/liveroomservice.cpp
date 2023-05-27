#include <QRegularExpression>
#include "liveroomservice.h"

LiveRoomService::LiveRoomService(QObject *parent) 
    : QObject(parent), NetInterface(this), LiveStatisticService(this)
{
    init();
}

void LiveRoomService::init()
{
    // 礼物连击
    comboTimer = new QTimer(this);
    comboTimer->setInterval(500);

    // 大乱斗
    pkTimer = new QTimer(this);
    pkTimer->setInterval(300);
    pkEndingTimer = new QTimer(this);

    // 定时连接
    connectServerTimer = new QTimer(this);
    connect(connectServerTimer, &QTimer::timeout, this, [=]{
        connectServerTimer->setInterval(us->timerConnectInterval * 60000); // 比如服务器主动断开，则会短期内重新定时，还原自动连接定时
        if (isLiving() && (liveSocket->state() == QAbstractSocket::ConnectedState || liveSocket->state() == QAbstractSocket::ConnectingState))
        {
            connectServerTimer->stop();
            return ;
        }
        emit signalStartConnectRoom();
    });

    initTimeTasks();
}

void LiveRoomService::initTimeTasks()
{
    // 每分钟定时
    connect(minuteTimer, &QTimer::timeout, this, [=]{
        // 直播间人气
        if (ac->currentPopul > 1 && isLiving()) // 为0的时候不计入内；为1时可能机器人在线
        {
            sumPopul += ac->currentPopul;
            countPopul++;

            dailyAvePopul = int(sumPopul / countPopul);
            if (dailySettings)
                dailySettings->setValue("average_popularity", dailyAvePopul);
        }
        if (dailyMaxPopul < ac->currentPopul)
        {
            dailyMaxPopul = ac->currentPopul;
            if (dailySettings)
                dailySettings->setValue("max_popularity", dailyMaxPopul);
        }

        // 弹幕人气
        danmuPopulValue += minuteDanmuPopul;
        danmuPopulQueue.append(minuteDanmuPopul);
        minuteDanmuPopul = 0;
        if (danmuPopulQueue.size() > 5)
            danmuPopulValue -= danmuPopulQueue.takeFirst();
        emit signalDanmuPopularChanged("5分钟弹幕人气：" + snum(danmuPopulValue) + "，平均人气：" + snum(dailyAvePopul));


        triggerCmdEvent("DANMU_POPULARITY", LiveDanmaku(), false);
    });

    // 每小时整点的的事件
    connect(hourTimer, &QTimer::timeout, this, [=]{
        QTime currentTime = QTime::currentTime();
        QTime nextTime = currentTime;
        nextTime.setHMS((currentTime.hour() + 1) % 24, 0, 1);
        hourTimer->setInterval(currentTime.hour() < 23 ? currentTime.msecsTo(nextTime) : 3600000);

        // 自动签到
        if (us->autoDoSign)
        {
            int hour = QTime::currentTime().hour();
            if (hour == 0)
            {
                doSign();
            }
        }

        triggerCmdEvent("NEW_HOUR", LiveDanmaku(), false);
        qInfo() << "NEW_HOUR:" << QDateTime::currentDateTime();

        // 判断每天最后一小时
        // 以 23:59:30为准
        QDateTime current = QDateTime::currentDateTime();
        QTime t = current.time();
        if (todayIsEnding) // 已经触发最后一小时事件了
        {
        }
        else if (current.time().hour() == 23
                || (t.hour() == 22 && t.minute() == 59 && t.second() > 30)) // 22:59:30之后的
        {
            todayIsEnding = true;
            QDateTime dt = current;
            QTime t = dt.time();
            t.setHMS(23, 59, 30); // 移动到最后半分钟
            dt.setTime(t);
            qint64 delta = dt.toMSecsSinceEpoch() - QDateTime::currentMSecsSinceEpoch();
            if (delta < 0) // 可能已经是即将最后了
                delta = 0;
            QTimer::singleShot(delta, [=]{
                triggerCmdEvent("DAY_END", LiveDanmaku(), true);

                // 判断每月最后一天
                QDate d = current.date();
                int days = d.daysInMonth();
                if (d.day() == days) // 1~31 == 28~31
                {
                    triggerCmdEvent("MONTH_END", LiveDanmaku(), true);

                    // 判断每年最后一天
                    days = d.daysInYear();
                    if (d.dayOfYear() == days)
                    {
                        triggerCmdEvent("YEAR_END", LiveDanmaku(), true);
                    }
                }

                // 判断每周最后一天
                if (d.dayOfWeek() == 7)
                {
                    triggerCmdEvent("WEEK_END", LiveDanmaku(), true);
                }
            });
        }
        emit signalNewHour();
    });
    hourTimer->start();

    // 判断新的一天
    connect(dayTimer, &QTimer::timeout, this, [=]{
        todayIsEnding = false;
        qInfo() << "--------NEW_DAY" << QDateTime::currentDateTime();
        {
            // 当前时间必定是 0:0:1，误差0.2秒内
            QDate tomorrowDate = QDate::currentDate();
            tomorrowDate = tomorrowDate.addDays(1);
            QTime zeroTime = QTime::currentTime();
            zeroTime.setHMS(0, 0, 1);
            QDateTime tomorrow(tomorrowDate, zeroTime);
            dayTimer->setInterval(int(QDateTime::currentDateTime().msecsTo(tomorrow)));
        }

        // 每天重新计算
        if (us->calculateDailyData)
            startCalculateDailyData();
        if (danmuLogFile /* && !isLiving() */)
            startSaveDanmakuToFile();
        us->userComeTimes.clear();
        sumPopul = 0;
        countPopul = 0;

        // 触发每天事件
        const QDate currDate = QDate::currentDate();
        triggerCmdEvent("NEW_DAY", LiveDanmaku(), true);
        triggerCmdEvent("NEW_DAY_FIRST", LiveDanmaku(), true);
        us->setValue("runtime/open_day", currDate.day());
        emit signalUpdatePermission();

        processNewDayData();
        qInfo() << "当前 month =" << currDate.month() << ", day =" << currDate.day() << ", week =" << currDate.dayOfWeek();

        // 判断每一月初
        if (currDate.day() == 1)
        {
            triggerCmdEvent("NEW_MONTH", LiveDanmaku(), true);
            triggerCmdEvent("NEW_MONTH_FIRST", LiveDanmaku(), true);
            us->setValue("runtime/open_month", currDate.month());

            // 判断每一年初
            if (currDate.month() == 1)
            {
                triggerCmdEvent("NEW_YEAR", LiveDanmaku(), true);
                triggerCmdEvent("NEW_YEAR_FIRST", LiveDanmaku(), true);
                triggerCmdEvent("HAPPY_NEW_YEAR", LiveDanmaku(), true);
                us->setValue("runtime/open_year", currDate.year());
            }
        }

        // 判断每周一
        if (currDate.dayOfWeek() == 1)
        {
            triggerCmdEvent("NEW_WEEK", LiveDanmaku(), true);
            triggerCmdEvent("NEW_WEEK_FIRST", LiveDanmaku(), true);
            us->setValue("runtime/open_week_number", currDate.weekNumber());
        }

        emit signalNewDay();
    });
    dayTimer->start();

    // 判断第一次打开
    QDate currDate = QDate::currentDate();
    int prevYear = us->value("runtime/open_year", -1).toInt();
    int prevMonth = us->value("runtime/open_month", -1).toInt();
    int prevDay = us->value("runtime/open_day", -1).toInt();
    int prevWeekNumber = us->value("runtime/open_week_number", -1).toInt();
    if (prevYear != currDate.year())
    {
        prevMonth = prevDay = -1; // 避免是不同年的同一月
        triggerCmdEvent("NEW_YEAR_FIRST", LiveDanmaku(), true);
        us->setValue("runtime/open_year", currDate.year());
    }
    if (prevMonth != currDate.month())
    {
        prevDay = -1; // 避免不同月的同一天
        triggerCmdEvent("NEW_MONTH_FIRST", LiveDanmaku(), true);
        us->setValue("runtime/open_month", currDate.month());
    }
    if (prevDay != currDate.day())
    {
        triggerCmdEvent("NEW_DAY_FIRST", LiveDanmaku(), true);
        us->setValue("runtime/open_day", currDate.day());
    }
    if (prevWeekNumber != currDate.weekNumber())
    {
        triggerCmdEvent("NEW_WEEK_FIRST", LiveDanmaku(), true);
        us->setValue("runtime/open_week_number", currDate.weekNumber());
    }
}

void LiveRoomService::readConfig()
{

}

void LiveRoomService::releaseLiveData(bool prepare)
{
    if (!prepare) // 断开连接
    {

    }
    else // 下播
    {
        cleanupHandler.clear(); // 清理泄露的内存
    }
}

QList<LiveDanmaku> LiveRoomService::getDanmusByUID(qint64 uid)
{
    QList<LiveDanmaku> dms;
    for (int i = 0; i < roomDanmakus.size(); i++)
    {
        if (roomDanmakus.at(i).getUid() == uid && roomDanmakus.at(i).is(MessageType::MSG_DANMAKU))
            dms.append(roomDanmakus.at(i));
    }
    return dms;
}

bool LiveRoomService::isLiving() const
{
    return ac->liveStatus == 1;
}

bool LiveRoomService::isLivingOrMayLiving()
{
    if (us->timerConnectServer)
    {
        if (!isLiving()) // 未开播，等待下一次的检测
        {
            // 如果是开播前一段时间，则继续保持着连接
            int start = us->startLiveHour;
            int end = us->endLiveHour;
            int hour = QDateTime::currentDateTime().time().hour();
            int minu = QDateTime::currentDateTime().time().minute();
            bool abort = false;
            qint64 currentVal = hour * 3600000 + minu * 60000;
            qint64 nextVal = (currentVal + us->timerConnectInterval * 60000) % (24 * 3600000); // 0点
            if (start < end) // 白天档
            {
                qInfo() << "白天档" << currentVal << start * 3600000 << end * 3600000;
                if (nextVal >= start * 3600000
                        && currentVal <= end * 3600000)
                {
                    if (us->liveDove) // 今天鸽了
                    {
                        abort = true;
                        qInfo() << "今天鸽了";
                    }
                    // 即将开播
                }
                else // 直播时间之外
                {
                    qInfo() << "未开播，继续等待";
                    abort = true;
                    if (currentVal > end * 3600000 && us->liveDove) // 今天结束鸽鸽
                        emit signalFinishDove();
                }
            }
            else if (start > end) // 熬夜档
            {
                qInfo() << "晚上档" << currentVal << start * 3600000 << end * 3600000;
                if (currentVal + us->timerConnectInterval * 60000 >= start * 3600000
                        || currentVal <= end * 3600000)
                {
                    if (us->liveDove) // 今晚鸽了
                        abort = true;
                    // 即将开播
                }
                else // 直播时间之外
                {
                    qInfo() << "未开播，继续等待";
                    abort = true;
                    if (currentVal > end * 3600000 && currentVal < start * 3600000
                            && us->liveDove)
                        emit signalFinishDove();
                }
            }

            if (abort)
            {
                qInfo() << "短期内不会开播，暂且断开连接";
                if (!connectServerTimer->isActive())
                    connectServerTimer->start();
                emit signalConnectionStateTextChanged("等待连接");

                // 如果正在连接或打算连接，却未开播，则断开
                if (liveSocket->state() != QAbstractSocket::UnconnectedState)
                    liveSocket->close();
                return false;
            }
        }
        else // 已开播，则停下
        {
            qInfo() << "开播，停止定时连接";
            if (connectServerTimer->isActive())
                connectServerTimer->stop();
            if (us->liveDove)
                emit signalFinishDove();
            return true;
        }
    }
    return true;
}

void LiveRoomService::sendExpireGift()
{
    getBagList(24 * 3600 * 2); // 默认赠送两天内过期的
}

QVariant LiveRoomService::getCookies() const
{
    return NetInterface::getCookies(ac->browserCookie);
}

void LiveRoomService::triggerCmdEvent(const QString &cmd, const LiveDanmaku &danmaku, bool debug)
{
    emit signalTriggerCmdEvent(cmd, danmaku, debug);
}

void LiveRoomService::localNotify(const QString &text, qint64 uid)
{
    emit signalLocalNotify(text, uid);
}

void LiveRoomService::showError(const QString &title, const QString &desc)
{
    emit signalShowError(title, desc);
}

void LiveRoomService::appendNewLiveDanmakus(const QList<LiveDanmaku> &danmakus)
{
    // 添加到队列
    roomDanmakus.append(danmakus);
    rt->allDanmakus.append(danmakus);
}

void LiveRoomService::appendNewLiveDanmaku(const LiveDanmaku &danmaku)
{
    roomDanmakus.append(danmaku);
    lastDanmaku = danmaku;
    rt->allDanmakus.append(danmaku);

    // 保存到文件
    if (danmuLogStream)
    {
        if (danmaku.is(MSG_DEF) && danmaku.getText().startsWith("["))
            return ;
        (*danmuLogStream) << danmaku.toString() << "\n";
        (*danmuLogStream).flush(); // 立刻刷新到文件里
    }

    emit signalNewDanmaku(danmaku);
}

void LiveRoomService::autoSetCookie(const QString &s)
{
    us->setValue("danmaku/browserCookie", ac->browserCookie = s);
    if (ac->browserCookie.isEmpty())
        return ;

    ac->userCookies = getCookies();
    getCookieAccount();

    // 自动设置弹幕格式
    int posl = ac->browserCookie.indexOf("bili_jct=") + 9;
    if (posl == -1)
        return ;
    int posr = ac->browserCookie.indexOf(";", posl);
    if (posr == -1) posr = ac->browserCookie.length();
    ac->csrf_token = ac->browserCookie.mid(posl, posr - posl);
    qInfo() << "检测到csrf_token:" << ac->csrf_token;

    if (ac->browserData.isEmpty())
        ac->browserData = "color=4546550&fontsize=25&mode=4&msg=&rnd=1605156247&roomid=&bubble=5&csrf_token=&csrf=";
    ac->browserData.replace(QRegularExpression("csrf_token=[^&]*"), "csrf_token=" + ac->csrf_token);
    ac->browserData.replace(QRegularExpression("csrf=[^&]*"), "csrf=" + ac->csrf_token);
    us->setValue("danmaku/browserData", ac->browserData);
    qInfo() << "设置弹幕格式：" << ac->browserData;
}

QStringList LiveRoomService::splitLongDanmu(const QString& text, int maxOne) const
{
    QStringList sl;
    int len = text.length();
    int count = (len + maxOne - 1) / maxOne;
    for (int i = 0; i < count; i++)
    {
        sl << text.mid(i * maxOne, maxOne);
    }
    return sl;
}

void LiveRoomService::sendLongText(QString text)
{
    if (text.contains("%n%"))
    {
        text.replace("%n%", "\n");
        for (auto s : text.split("\n", QString::SkipEmptyParts))
        {
            sendLongText(s);
        }
        return ;
    }
    emit signalSendAutoMsg(splitLongDanmu(text, ac->danmuLongest).join("\\n"), LiveDanmaku());
}

bool LiveRoomService::isInFans(qint64 uid) const
{
    foreach (auto fan, fansList)
    {
        if (fan.mid == uid)
            return true;
    }
    return false;
}
