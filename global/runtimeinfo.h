#ifndef RUNTIME_INFO_H
#define RUNTIME_INFO_H

#include <QHash>
#include <QList>
#include <QApplication>
#include <QDir>

#define APP_ID 0
#define APP_VERSION "5.4.1"
#define LOCAL_MODE 0

#if true
#define s8(x) QString(x)
#else
#define s8(x)  QString(x).toStdString().c_str()
#endif

#define snum(x) QString::number(x)


class LiveDanmaku;

enum LivePlatform
{
    Bilibili = 0,       // 哔哩哔哩
    Douyin = 1,         // 抖音
    Huya,               // 虎牙
    Douyu,              // 斗鱼
    Kuaishou,           // 快手
    BilibiliOpen = 10,  // 哔哩哔哩开放平台
};

class RuntimeInfo
{
public:
    LivePlatform livePlatform = Bilibili;
    bool asPlugin = false;
    bool asFreeOnly = false;
    bool justStart = true; // 启动几秒内不进行发送，避免一些尴尬场景
    bool isReconnect = false; // 是否是重连
    bool adjustCookie = false;
    bool dontBackupSettingFile = false; // 是否不备份设置文件

    QHash<QString, QString> pinyinMap; // 拼音
    QList<LiveDanmaku> allDanmakus;
    QList<LiveDanmaku> blockedQueue; // 本次自动禁言的用户，用来撤销

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
    QWidget* mainwindow = nullptr;
    QWidget* danmakuWindow = nullptr;
};

extern RuntimeInfo* rt;

#endif // RUNTIME_INFO_H
