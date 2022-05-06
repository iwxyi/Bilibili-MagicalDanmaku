#ifndef ACCOUNT_INFO_H
#define ACCOUNT_INFO_H

#include <QHash>
#include "livedanmaku.h"

class AccountInfo
{
public:
    QString browserCookie;
    QString browserData;
    QString csrf_token;
    QVariant userCookies;
};

extern AccountInfo* ac;

#endif // ACCOUNT_INFO_H
