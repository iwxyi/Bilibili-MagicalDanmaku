#ifndef LIVESTATISTICSERVICE_H
#define LIVESTATISTICSERVICE_H

#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QFile>
#include "livedanmaku.h"
#include "runtimeinfo.h"
#include "usersettings.h"
#include "accountinfo.h"
#include "platforminfo.h"

class LiveStatisticService
{
public:
    LiveStatisticService(QObject* me);

    void initTimers();

    void readConfig();

    /// 初始化每日数据
    void startCalculateDailyData();
    /// 保存每日数据
    void saveCalculateDailyData();
    /// 初始化本场直播数据
    void startCalculateCurrentLiveData();
    /// 保存本场直播数据
    void saveCalculateCurrentLiveData();

    /// 保存舰长数据的时候需要在线获取
    virtual void updateExistGuards(int page = 0) { Q_UNUSED(page) }

    /// 保存弹幕记录
    void startSaveDanmakuToFile();
    void finishSaveDanmuToFile();

protected:
    QObject* me = nullptr;

    // 每日数据
    QSettings *dailySettings = nullptr;
    QTimer *dayTimer = nullptr;
    int dailyCome = 0;       // 进来数量人次
    int dailyPeopleNum = 0;  // 本次进来的人数（不是全程的话，不准确）
    int dailyDanmaku = 0;    // 弹幕数量
    int dailyNewbieMsg = 0;  // 新人发言数量（需要开启新人发言提示）
    int dailyNewFans = 0;    // 关注数量
    int dailyTotalFans = 0;  // 粉丝总数量（需要开启感谢关注）
    int dailyGiftSilver = 0; // 银瓜子总价值
    int dailyGiftGold = 0;   // 金瓜子总价值（不包括船员）
    int dailyGuard = 0;      // 上船/续船人次
    int dailyMaxPopul = 0;   // 最高人气
    int dailyAvePopul = 0;   // 平均人气
    bool todayIsEnding = false; // 最后一小时

    // 本场数据
    QSettings *currentLiveSettings = nullptr;
    qint64 currentLiveStartTimestamp = 0; // 本场开始时间
    int currentLiveCome = 0;       // 进来数量人次
    int currentLivePeopleNum = 0;  // 本次进来的人数（不是全程的话，不准确）
    int currentLiveDanmaku = 0;    // 弹幕数量
    int currentLiveNewbieMsg = 0;  // 新人发言数量（需要开启新人发言提示）
    int currentLiveNewFans = 0;    // 关注数量
    int currentLiveTotalFans = 0;  // 粉丝总数量（需要开启感谢关注）
    int currentLiveGiftSilver = 0; // 银瓜子总价值
    int currentLiveGiftGold = 0;   // 金瓜子总价值（不包括船员）
    int currentLiveGuard = 0;      // 上船/续船人次
    int currentLiveMaxPopul = 0;   // 最高人气
    int currentLiveAvePopul = 0;   // 平均人气

    // 保存的编码
    QString recordFileCodec = ""; // 自动保存上船、礼物记录、每月船员等编码
    QString codeFileCodec = "UTF-8"; // 代码保存的文件编码
    QString externFileCodec = "UTF-8"; // 提供给外界读取例如歌曲文件编码

    // 直播间人气
    QTimer* minuteTimer = nullptr;
    QTimer* hourTimer = nullptr;
    int popularVal = 2;
    qint64 sumPopul = 0;     // 自启动以来的人气
    qint64 countPopul = 0;   // 自启动以来的人气总和

    // 弹幕人气
    int minuteDanmuPopul = 0;
    QList<int> danmuPopulQueue;
    int danmuPopulValue = 0;

    // 弹幕日志
    QFile* danmuLogFile = nullptr;
    QTextStream* danmuLogStream = nullptr;
};

#endif // LIVESTATISTICSERVICE_H
