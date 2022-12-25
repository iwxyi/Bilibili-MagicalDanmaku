#include <QRegularExpression>
#include "liveroomservice.h"

LiveRoomService::LiveRoomService(QObject *parent) 
    : QObject(parent), NetInterface(this)
{
}

void LiveRoomService::init()
{

}

void LiveRoomService::readConfig()
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
