#ifndef COMMANDVALUES_H
#define COMMANDVALUES_H

#include <QHash>

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
    static QHash<qint64, int> userDanmuCounts;    // 弹幕次数
    static QHash<qint64, qint64> userBlockIds;    // 本次用户屏蔽的ID
};

#endif // COMMANDVALUES_H
