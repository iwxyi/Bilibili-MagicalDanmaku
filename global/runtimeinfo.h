#ifndef RUNTIME_INFO_H
#define RUNTIME_INFO_H

#include <QHash>
#include <QList>
#include <QApplication>

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

    const int widgetSizeL = 48;
    const int fluentRadius = int(5 * qApp->devicePixelRatio() + 0.5);
    const int giftImgSize = 60;

    QString dataPath;
    QString appVersion; // 不带v
    QString appFileName; // 应用程序文件名（不带exe）
    QString appNewVersion;
    QString appDownloadUrl;
};

extern RuntimeInfo* rt;

#endif // RUNTIME_INFO_H
