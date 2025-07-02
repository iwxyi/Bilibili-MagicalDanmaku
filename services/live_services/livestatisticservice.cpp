#include "livestatisticservice.h"

LiveStatisticService::LiveStatisticService(QObject *me) : me(me)
{
    initTimers();
}

void LiveStatisticService::readConfig()
{
}

void LiveStatisticService::initTimers()
{
    minuteTimer = new QTimer();
    minuteTimer->setInterval(60000);

    hourTimer = new QTimer();
    QTime currentTime = QTime::currentTime();
    QTime nextTime = currentTime;
    nextTime.setHMS((currentTime.hour() + 1) % 24, 0, 1);
    hourTimer->setInterval(currentTime.hour() < 23 ? currentTime.msecsTo(nextTime) : 3600000);

    dayTimer = new QTimer();
    QTime zeroTime = QTime::currentTime();
    zeroTime.setHMS(0, 0, 1); // 本应当完全0点整的，避免误差
    QDate tomorrowDate = QDate::currentDate();
    tomorrowDate = tomorrowDate.addDays(1);
    QDateTime tomorrow(tomorrowDate, zeroTime);
    dayTimer->setInterval(int(QDateTime::currentDateTime().msecsTo(tomorrow)));
}

void LiveStatisticService::startCalculateDailyData()
{
    if (dailySettings)
    {
        saveCalculateDailyData();
        dailySettings->deleteLater();
    }

    QDir dir;
    dir.mkdir(rt->dataPath + "live_daily");
    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    dailySettings = new QSettings(rt->dataPath + "live_daily/" + ac->roomId + "_" + date + ".ini", QSettings::Format::IniFormat);
    qInfo() << "保存每日直播统计：" << rt->dataPath + "live_daily/" + ac->roomId + "_" + date + ".ini";

    dailyCome = dailySettings->value("come", 0).toInt();
    dailyPeopleNum = dailySettings->value("people_num", 0).toInt();
    dailyDanmaku = dailySettings->value("danmaku", 0).toInt();
    dailyNewbieMsg = dailySettings->value("newbie_msg", 0).toInt();
    dailyNewFans = dailySettings->value("new_fans", 0).toInt();
    dailyTotalFans = dailySettings->value("total_fans", 0).toInt();
    dailyGiftSilver = dailySettings->value("gift_silver", 0).toInt();
    dailyGiftGold = dailySettings->value("gift_gold", 0).toInt();
    dailyGuard = dailySettings->value("guard", 0).toInt();
    dailyMaxPopul = dailySettings->value("max_popularity", 0).toInt();
    dailyAvePopul = 0;
    if (ac->currentGuards.size())
        dailySettings->setValue("guard_count", ac->currentGuards.size());
    else
        updateExistGuards(0);
}

void LiveStatisticService::saveCalculateDailyData()
{
    if (dailySettings)
    {
        dailySettings->setValue("come", dailyCome);
        dailySettings->setValue("people_num", qMax(dailySettings->value("people_num").toInt(), us->userComeTimes.size()));
        dailySettings->setValue("danmaku", dailyDanmaku);
        dailySettings->setValue("newbie_msg", dailyNewbieMsg);
        dailySettings->setValue("new_fans", dailyNewFans);
        dailySettings->setValue("total_fans", ac->currentFans);
        dailySettings->setValue("gift_silver", dailyGiftSilver);
        dailySettings->setValue("gift_gold", dailyGiftGold);
        dailySettings->setValue("guard", dailyGuard);
        if (ac->currentGuards.size())
            dailySettings->setValue("guard_count", ac->currentGuards.size());
    }
}

void LiveStatisticService::startCalculateCurrentLiveData()
{
    if (currentLiveSettings)
    {
        saveCalculateCurrentLiveData();
        currentLiveSettings->deleteLater();
    }

    QDir dir;
    dir.mkdir(rt->dataPath + "live_current");
    currentLiveStartTimestamp = ac->liveStartTime;
    if (currentLiveStartTimestamp == 0) // 未开播，就不进行保存了
    {
        qInfo() << "未开播，忽略本场直播数据";
        return ;
    }
    QString time = QDateTime::fromSecsSinceEpoch(currentLiveStartTimestamp).toString("yyyy-MM-dd HH-mm-ss");
    currentLiveSettings = new QSettings(rt->dataPath + "live_current/" + ac->roomId + "_" + time + ".ini", QSettings::Format::IniFormat);
    qInfo() << "保存本场直播统计：" << rt->dataPath + "live_current/" + ac->roomId + "_" + time + ".ini";

    currentLiveCome = currentLiveSettings->value("come", 0).toInt();
    currentLivePeopleNum = currentLiveSettings->value("people_num", 0).toInt();
    currentLiveDanmaku = currentLiveSettings->value("danmaku", 0).toInt();
    currentLiveNewbieMsg = currentLiveSettings->value("newbie_msg", 0).toInt();
    currentLiveNewFans = currentLiveSettings->value("new_fans", 0).toInt();
    currentLiveTotalFans = currentLiveSettings->value("total_fans", 0).toInt();
    currentLiveGiftSilver = currentLiveSettings->value("gift_silver", 0).toInt();
    currentLiveGiftGold = currentLiveSettings->value("gift_gold", 0).toInt();
    currentLiveGuard = currentLiveSettings->value("guard", 0).toInt();
    currentLiveMaxPopul = currentLiveSettings->value("max_popularity", 0).toInt();
    currentLiveAvePopul = 0;
    if (ac->currentGuards.size())
        currentLiveSettings->setValue("guard_count", ac->currentGuards.size());
    else
        updateExistGuards(0);
}

void LiveStatisticService::saveCalculateCurrentLiveData()
{
    if (currentLiveSettings)
    {
        currentLiveSettings->setValue("come", currentLiveCome);
        currentLiveSettings->setValue("people_num", qMax(currentLiveSettings->value("people_num").toInt(), us->userComeTimes.size()));
        currentLiveSettings->setValue("danmaku", currentLiveDanmaku);
        currentLiveSettings->setValue("newbie_msg", currentLiveNewbieMsg);
        currentLiveSettings->setValue("new_fans", currentLiveNewFans);
        currentLiveSettings->setValue("total_fans", ac->currentFans);
        currentLiveSettings->setValue("gift_silver", currentLiveGiftSilver);
        currentLiveSettings->setValue("gift_gold", currentLiveGiftGold);
        currentLiveSettings->setValue("guard", currentLiveGuard);
        if (ac->currentGuards.size())
            currentLiveSettings->setValue("guard_count", ac->currentGuards.size());
    }
}

void LiveStatisticService::startSaveDanmakuToFile()
{
    if (danmuLogFile)
        finishSaveDanmuToFile();

    QDir dir;
    dir.mkdir(rt->dataPath+"danmaku_histories");
    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    qInfo() << "开启弹幕记录：" << rt->dataPath+"danmaku_histories/" + ac->roomId + "_" + date + ".log";
    danmuLogFile = new QFile(rt->dataPath+"danmaku_histories/" + ac->roomId + "_" + date + ".log");
    danmuLogFile->open(QIODevice::WriteOnly | QIODevice::Append);
    danmuLogStream = new QTextStream(danmuLogFile);
    danmuLogStream->setGenerateByteOrderMark(true);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    danmuLogStream->setCodec(externFileCodec.toUtf8().constData());
#else
    auto encoding = QStringConverter::encodingForName(externFileCodec.toUtf8());
    if (encoding)
        danmuLogStream->setEncoding(*encoding);
#endif
}

void LiveStatisticService::finishSaveDanmuToFile()
{
    if (!danmuLogFile)
        return ;

    delete danmuLogStream;
    danmuLogFile->close();
    danmuLogFile->deleteLater();
    danmuLogFile = nullptr;
    danmuLogStream = nullptr;
}
