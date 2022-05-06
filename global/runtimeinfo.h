#ifndef RUNTIME_INFO_H
#define RUNTIME_INFO_H

#include <QHash>
#include <QList>

#if true
#define s8(x) QString(x)
#else
#define s8(x)  QString(x).toStdString().c_str()
#endif

#define snum(x) QString::number(x)

class LiveDanmaku;

class RuntimeInfo
{
public:
    QHash<QString, QString> pinyinMap; // 拼音
    QList<LiveDanmaku> allDanmakus;

};

extern RuntimeInfo* rt;

#endif // RUNTIME_INFO_H
