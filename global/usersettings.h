#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include "mysettings.h"
class EternalBlockUser;

class UserSettings : public MySettings
{
public:
    UserSettings(const QString& fileName, QObject* parent = nullptr) : MySettings(fileName, QSettings::IniFormat, parent)
    {
    }

    QHash<qint64, QString> localNicknames; // 本地昵称
    QHash<qint64, qint64> userComeTimes;   // 用户进来的时间（客户端时间戳为准）
    QHash<qint64, qint64> userBlockIds;    // 本次用户屏蔽的ID
    QSettings* danmakuCounts; // 保存弹幕次数的settings
    QSettings* userMarks; // 保存每位用户的设置
    QList<qint64> careUsers; // 特别关心
    QList<qint64> strongNotifyUsers; // 强提醒
    QList<QPair<QString, QString>> customVariant; // 自定义变量
    QList<QPair<QString, QString>> variantTranslation; // 变量翻译
    QList<QPair<QString, QString>> replaceVariant; // 替换变量
    QList<qint64> notWelcomeUsers; // 不自动欢迎的用户（某些领导、黑粉）
    QList<qint64> notReplyUsers;   // 不自动回复的用户
    QHash<int, QString> giftAlias; // 礼物名字
    QHash<qint64, QString> currentGuards; // 当前船员ID-Name
    QList<EternalBlockUser> eternalBlockUsers; // 永久禁言
};

extern UserSettings* us;

#endif // USERSETTINGS_H
