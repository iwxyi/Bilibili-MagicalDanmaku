#ifndef CODERUNNER_H
#define CODERUNNER_H

#include <QObject>
#include <functional>
#include "liveroomservice.h"
#include "runtimeinfo.h"
#include "usersettings.h"
#include "accountinfo.h"
#include "orderplayerwindow.h"
#include "web_server/webserver.h"
#include "voice_service/voiceservice.h"
#include "jsengine.h"
#include "luaengine.h"
#include "pythonengine.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define AUTO_MSG_CD 1500
#define NOTIFY_CD 2000

#define CHANNEL_COUNT 100
#define MAGICAL_SPLIT_CHAR "-bdm-split-bdm-"
#define WAIT_CHANNEL_MAX 99

#define NOTIFY_CD_CN 0     // 默认通知通道（强提醒、通告、远程控制等）
#define WELCOME_CD_CN 1    // 送礼冷却通道
#define GIFT_CD_CN 2       // 礼物冷却通道
#define ATTENTION_CD_CN 3  // 关注冷却通道
#define TASK_CD_CN 4       // 定时任务冷却通道
#define REPLY_CD_CN 5      // 自动回复冷却通道
#define EVENT_CD_CN 6      // 事件动作冷却通道

#define TAB_TIMER_TASK 0   // 定时任务
#define TAB_AUTO_REPLY 1   // 自动回复
#define TAB_EVENT_ACTION 2 // 事件动作

#define FILTER_MUSIC_ORDER "FILTER_MUSIC_ORDER"
#define FILTER_DANMAKU_MSG "FILTER_DANMAKU_MSG"
#define FILTER_DANMAKU_COME "FILTER_DANMAKU_COME"
#define FILTER_DANMAKU_GIFT "FILTER_DANMAKU_GIFT"

enum CmdResponse
{
    NullRes,
    AbortRes,
    DelayRes,
};

class CodeRunner : public QObject
{
    Q_OBJECT
public:
    explicit CodeRunner(QObject *parent = nullptr);

    void setLiveService(LiveRoomService* service);
    void setHeaps(MySettings *heaps);
    void setMainUI(Ui::MainWindow *ui);
    void setMusicWindow(OrderPlayerWindow* musicWindow);
    void setWebServer(WebServer* ws);
    void setVoiceService(VoiceService* vs);
    void releaseData();

    QString getCodeContentFromFile(QString name);

signals:
    void signalTriggerCmdEvent(const QString& cmd, const LiveDanmaku& danmaku, bool debug);
    void signalLocalNotify(const QString& text, qint64 uid);
    void signalShowError(const QString& title, const QString& info);
    void signalSpeakText(const QString& text);

    void signalProcessRemoteCmd(QString msg, bool response);

public slots:
    bool sendVariantMsg(QString msg, const LiveDanmaku& danmaku, int channel = NOTIFY_CD_CN, bool manual = false, bool delayMine = false);
    void sendAutoMsg(QString msgs, const LiveDanmaku& danmaku);
    void sendAutoMsgInFirst(QString msgs, const LiveDanmaku& danmaku, int interval = 0);
    void slotSendAutoMsg(bool timeout);
    void sendCdMsg(QString msg, LiveDanmaku danmaku, int cd, int channel, bool enableText, bool enableVoice, bool manual);
    void sendCdMsg(QString msgs, const LiveDanmaku& danmaku);
    void sendGiftMsg(QString msg, const LiveDanmaku& danmaku);
    void sendAttentionMsg(QString msg, const LiveDanmaku& danmaku);
    void sendNotifyMsg(QString msg, bool manual = false);
    void sendNotifyMsg(QString msg, const LiveDanmaku &danmaku, bool manual = false);
    void setFilter(QString filterName, QString content);
    void addNoReplyDanmakuText(const QString &text);

public:
    void analyzeMsgAndCd(QString &msg, int& cd, int& channel) const;
    QString processTimeVariants(QString msg) const;
    QStringList getEditConditionStringList(QString plainText, LiveDanmaku danmaku);
    QString processDanmakuVariants(QString msg, const LiveDanmaku &danmaku);
    QString replaceDanmakuVariants(const LiveDanmaku &danmaku, const QString& key, bool* ok) const;
    QString generateCodeFunctions(const QString &funcName, const QString &args, const LiveDanmaku& danmaku, bool* ok);
    QString replaceDanmakuJson(const QString &keySeq, const QJsonObject& json, bool *ok) const;
    QString traverseJsonCode(const QString& keySeq, const QString& code, const QJsonObject& json) const;
    QString replaceDynamicVariants(const QString& funcName, const QString& args, const LiveDanmaku &danmaku);
    QString processMsgHeaderConditions(QString msg) const;
    template<typename T>
    bool isConditionTrue(T a, T b, QString op) const;
    bool isFilterRejected(QString filterName, const LiveDanmaku& danmaku);
    bool processFilter(QString filterCode, const LiveDanmaku& danmaku);
    void translateUnicode(QString& s) const;

    QString getReplyExecutionResult(QString key, const LiveDanmaku &danmaku);
    QString getEventExecutionResult(QString key, const LiveDanmaku &danmaku);
    QString getExecutionResult(QStringList &msgs, const LiveDanmaku &_danmaku);
    void simulateKeys(QString seq, bool press = true, bool release = true);
    void simulateClick();
    void simulateClickButton(qint64 keys);
    void moveMouse(unsigned long x, unsigned long dy);
    void moveMouseTo(unsigned long tx, unsigned long ty);
    void addBannedWord(QString word, QString anchor);
    void speekVariantText(QString text);

    bool hasSimilarOldDanmaku(const QString &s) const;

    void triggerCmdEvent(const QString& cmd, const LiveDanmaku& danmaku, bool debug = false);
    void localNotify(const QString& text, qint64 uid = 0);
    void showError(const QString& title, const QString& desc = "");

    QString msgToShort(QString msg) const;
    QString toFilePath(const QString& fileName) const;
    QString toSingleLine(QString text) const;
    QString toMultiLine(QString text) const;
    QString toMultiLineForJson(QString text) const;
    QString toRunableCode(QString text) const;
    qint64 unameToUid(QString text);
    QString uidToName(qint64 uid);
    QString nicknameSimplify(const LiveDanmaku& danmaku) const;
    QString numberSimplify(int number) const;
    bool shallAutoMsg() const;
    bool shallAutoMsg(const QString& sl) const;
    bool shallAutoMsg(const QString& sl, bool& manual);
    bool shallSpeakText();
    bool isWorking() const;

private:
    LiveRoomService* liveService = nullptr;
    Ui::MainWindow *ui;

public:
    // 设置
    MySettings* heaps = nullptr;
    MySettings* extSettings = nullptr;
    bool enableFilter = true; // 过滤器总开关

    // 过滤器（已废弃方案）
    QString filter_musicOrder; // 点歌过滤
    QRegularExpression filter_musicOrderRe;
    QString filter_danmakuMsg; // 弹幕姬：消息
    QString filter_danmakuCome; // 弹幕姬：用户进入过滤
    QString filter_danmakuGift; // 弹幕姬：礼物过滤

    // 发送弹幕队列
    QList<QPair<QStringList, LiveDanmaku>> autoMsgQueues; // 待发送的自动弹幕，是一个二维列表！
    QTimer* autoMsgTimer;
    bool inDanmakuCd = false;    // 避免频繁发送弹幕
    int inDanmakuDelay = 0; // 对于 >run(>delay(xxx)) 的支持
    QStringList noReplyMsgs;

    // 欢迎
    qint64 msgCds[CHANNEL_COUNT] = {}; // 冷却通道（精确到毫秒）
    int msgWaits[CHANNEL_COUNT] = {}; // 等待通道

    // 游戏列表
    QList<qint64> gameUsers[CHANNEL_COUNT];
    QList<qint64> gameNumberLists[CHANNEL_COUNT];
    QList<QString> gameTextLists[CHANNEL_COUNT];

    // 弹幕记录
    const int debugLastCount = 20;
    QStringList lastConditionDanmu;   // 替换变量后的代码
    QStringList lastCandidateDanmaku; // 要执行的代码
    int robotTotalSendMsg = 0; // 机器人发送的弹幕数量
    int liveTotalDanmaku = 0; // 本场直播的弹幕数量

    // 其他模块
    OrderPlayerWindow* musicWindow = nullptr;
    WebServer* webServer = nullptr;

    // 缓存
    QHash<QString, QImage> cacheImages;

    // 语音
    VoiceService* voiceService;

    // bool (*execFuncCallback)(QString msg, LiveDanmaku &danmaku, CmdResponse &res, int &resVal) = nullptr;
    std::function<bool(QString, LiveDanmaku&, CmdResponse&, int&)> execFuncCallback;

    // 编程引擎
    JSEngine* jsEngine;
    LuaEngine* luaEngine;
    PythonEngine* pythonEngine;
};

extern CodeRunner* cr;

#endif // CODERUNNER_H
