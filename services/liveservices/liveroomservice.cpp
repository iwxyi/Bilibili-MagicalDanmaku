#include <QRegularExpression>
#include "liveroomservice.h"

LiveRoomService::LiveRoomService(QObject *parent) 
    : QObject(parent), NetInterface(this), LiveStatisticService(this)
{
    init();
}

void LiveRoomService::init()
{

}

void LiveRoomService::readConfig()
{

}

void LiveRoomService::releaseLiveData(bool prepare)
{
    if (!prepare)
    {

    }
    else
    {

    }
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

QVariant LiveRoomService::getCookies() const
{
    return NetInterface::getCookies(ac->browserCookie);
}

void LiveRoomService::setUrlCookie(const QString &url, QNetworkRequest *request)
{
    if (url.contains("bilibili.com") && !ac->browserCookie.isEmpty())
        request->setHeader(QNetworkRequest::CookieHeader, ac->userCookies);
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
