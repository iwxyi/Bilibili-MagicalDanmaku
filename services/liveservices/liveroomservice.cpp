#include <QRegularExpression>
#include "liveroomservice.h"

LiveRoomService::LiveRoomService(QObject *parent) 
    : QObject(parent), NetInterface(this)
{
}

void LiveRoomService::startCalculateDailyData()
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
    if (us->currentGuards.size())
        dailySettings->setValue("guard_count", us->currentGuards.size());
    else
        updateExistGuards(0);
}

void LiveRoomService::saveCalculateDailyData()
{
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
