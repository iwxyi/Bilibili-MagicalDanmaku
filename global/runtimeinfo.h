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

enum LivePlatform
{
    Bilibili,   // 哔哩哔哩
    Huya,       // 虎牙
    Douyu,      // 斗鱼
    Douyin,     // 抖音
    Kuaishou,   // 快手
};

class RuntimeInfo
{
public:
    LivePlatform livePlatform = Bilibili;
    bool asPlugin = false;
    bool asFreeOnly = false;

    QHash<QString, QString> pinyinMap; // 拼音
    QList<LiveDanmaku> allDanmakus;

    const int widgetSizeL = 48;
    const int fluentRadius = int(5 * qApp->devicePixelRatio() + 0.5);
    const int giftImgSize = 60;
    bool firstOpen = false;

    QString dataPath;   // 末尾带/
    QString appVersion; // 不带v
    QString appFileName; // 应用程序文件名（不带exe）
    QString appNewVersion;
    QString appDownloadUrl;

    QString ffmpegPath;
};

extern RuntimeInfo* rt;

#endif // RUNTIME_INFO_H
