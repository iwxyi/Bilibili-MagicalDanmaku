#pragma once
#include <QString>

class WebLoginUtil {
public:
    // 静态方法：弹出登录对话框，返回登录后的Cookie
    static QString getCookie(const QString &platformUrl, const QString &defaultCookie = QString());
}; 