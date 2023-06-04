#ifndef USERSETTINGS_H
#define USERSETTINGS_H

#include "mysettings.h"
#include "externalblockuser.h"

class UserSettings : public MySettings
{
public:
    UserSettings(const QString &fileName, QObject *parent = nullptr) : MySettings(fileName, QSettings::IniFormat, parent)
    {
    }

    QHash<qint64, QString> localNicknames;             // 本地昵称
    QHash<qint64, qint64> userComeTimes;               // 用户进来的时间（客户端时间戳为准）
    QHash<qint64, qint64> userBlockIds;                // 本次用户屏蔽的ID
    QSettings *danmakuCounts;                          // 保存弹幕次数的settings
    QSettings *userMarks;                              // 保存每位用户的设置
    QList<qint64> careUsers;                           // 特别关心
    QList<qint64> strongNotifyUsers;                   // 强提醒
    QList<QPair<QString, QString>> customVariant;      // 自定义变量
    QList<QPair<QString, QString>> variantTranslation; // 变量翻译
    QList<QPair<QString, QString>> replaceVariant;     // 替换变量
    QList<qint64> notWelcomeUsers;                     // 不自动欢迎的用户（某些领导、黑粉）
    QList<qint64> notReplyUsers;                       // 不自动回复的用户
    QHash<int, QString> giftAlias;                     // 礼物名字
    QList<EternalBlockUser> eternalBlockUsers;         // 永久禁言

    bool useStringSimilar = false;   // 使用字符串编辑距离相似度算法
    int stringSimilarThreshold = 80; // 相似度达到阈值及以上则算是重复弹幕
    int danmuSimilarJudgeCount = 10; // 重复弹幕的判断数量
    int imageSimilarPrecision = 8;   // 图片相似度精度，越大越好
    bool closeGui = false;           // 关闭GUI效果

    qint64 removeDanmakuInterval = 60000;
    qint64 removeDanmakuTipInterval = 20000;
    bool adjustDanmakuLongest = true;// 自动调整弹幕最长字数
    bool removeLongerRandomDanmaku = true; // 随机弹幕自动移除过长的（无视优先级），否则长弹幕会自动分割成多条短的
    bool timerConnectServer = false; // 定时连接
    int startLiveHour = -1;          // 最早上班的时间
    int endLiveHour = -1;            // 最晚下播的时间
    int timerConnectInterval;        // 定时检测的时间（分支）
    bool liveDove = false;           // 鸽一天，不自动连接
    int getHeartTimeCount = 0;       // 获取小心心的总数
    bool saveDanmakuToFile = false;  // 保存弹幕日志文件
    bool autoDoSign = false;         // 每天自动签到
    bool autoJoinLOT = false;        // 自动参加天选
    bool calculateDailyData = false; // 统计每天数据
    bool retryFailedDanmaku = true;  // 发送失败的弹幕自动重试
    bool remoteControl = true;       // 是否允许弹幕命令控制
    bool saveToSqlite = false;       // 保存弹幕数据库
    bool saveCmdToSqlite = false;    // 保存所有CMD命令到数据库
    int judgeRobot = 0;              // 判断机器人：0关，1仅关注，2所有
    int giftComboDelay = 3;          // 礼物连击延迟（秒）
    bool AIReplyMsgLocal = false;    // AI回复本地显示
    int AIReplyMsgSend = 0;          // AI回复发送弹幕，0关闭，1仅少量，2全部

    QString open_ai_key;
    QString chatgpt_model_name = "gpt-3.5-turbo";
    bool chatgpt_history_input = false;
    int chatgpt_max_token_count = 2048;
    int chatgpt_max_context_count = 16;
    QString chatgpt_prompt;
    bool chatgpt_analysis = false;
    QString chatgpt_analysis_prompt;
    QString chatgpt_analysis_format;

    bool localMode = false;   // 本地调试模式
    bool debugPrint = false;  // 调试输出模式
    bool complexCalc = false; // 启动复杂计算（禁用，有大bug）
    QJsonObject dynamicConfigs;  // 一些默认配置

    QString getLocalNickname(qint64 uid) const
    {
        if (localNicknames.contains(uid))
            return localNicknames.value(uid);
        return "";
    }
};

extern UserSettings *us;

#endif // USERSETTINGS_H
