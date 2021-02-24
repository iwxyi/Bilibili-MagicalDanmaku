#ifndef COMMANDVALUES_H
#define COMMANDVALUES_H

#include <QHash>
#include "livedanmaku.h"

class QSettings;
class EternalBlockUser;

#if true
#define s8(x) QString(x)
#else
#define s8(x)  QString(x).toStdString().c_str()
#endif

#define snum(x) QString::number(x)

class CommonValues
{
protected:
    static QHash<qint64, QString> localNicknames; // 本地昵称
    static QHash<qint64, qint64> userComeTimes;   // 用户进来的时间（客户端时间戳为准）
    static QHash<qint64, qint64> userBlockIds;    // 本次用户屏蔽的ID
    static QSettings* danmakuCounts; // 保存弹幕次数的settings
    static QList<LiveDanmaku> allDanmakus;
    static QList<qint64> careUsers; // 特别关心
    static QList<qint64> strongNotifyUsers; // 强提醒
    static QHash<QString, QString> pinyinMap; // 拼音
    static QHash<QString, QString> customVariant; // 自定义变量
    static QList<qint64> notWelcomeUsers; // 不自动欢迎的用户（某些领导、黑粉）
    static QList<qint64> notReplyUsers;   // 不自动回复的用户
    static QHash<int, QString> giftNames; // 礼物名字
    static QList<EternalBlockUser> eternalBlockUsers; // 永久禁言
    static QHash<qint64, QString> currentGuards; // 当前船员ID-Name

    // 登陆信息
    static QString browserCookie;
    static QString browserData;
    static QString csrf_token;
    static QVariant userCookies;
};

#endif // COMMANDVALUES_H
