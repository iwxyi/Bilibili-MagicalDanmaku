#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include <QNetworkCookie>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCodec>
#include <stdio.h>
#include <iostream>
#include <QtWebSockets/QWebSocket>
#include <QAuthenticator>
#include <QtConcurrent/QtConcurrent>
#include <QSystemTrayIcon>
#include <QDesktopServices>
#include <QtTextToSpeech/QTextToSpeech>
#include <qhttpserver.h>
#include <qhttprequest.h>
#include <qhttpresponse.h>
#include <QWebSocketServer>
#include "netutil.h"
#include "livedanmaku.h"
#include "livedanmakuwindow.h"
#include "taskwidget.h"
#include "replywidget.h"
#include "commonvalues.h"
#include "orderplayerwindow.h"
#include "textinputdialog.h"
#include "luckydrawwindow.h"
#include "livevideoplayer.h"
#include "xfytts.h"
#include "eternalblockdialog.h"
#include "picturebrowser.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define SOCKET_DEB if (0) qDebug() // 输出调试信息
#define SOCKET_INF if (0) qDebug() // 输出数据包信息
#define SOCKET_MODE

#define CONNECT_SERVER_INTERVAL 1800000

#define AUTO_MSG_CD 1500
#define NOTIFY_CD 2000

#define CHANNEL_COUNT 100

#define NOTIFY_CD_CN 0     // 默认通知通道（强提醒、通告、远程控制等）
#define WELCOME_CD_CN 1    // 送礼冷却通道
#define GIFT_CD_CN 2       // 礼物冷却通道
#define ATTENTION_CD_CN 3  // 关注冷却通道
#define TASK_CD_CN 4       // 定时任务冷却通道
#define REPLY_CD_CN 5      // 自动回复冷却通道
#define EVENT_CD_CN 6      // 事件动作冷却通道

#define SERVER_PORT 0
#define DANMAKU_SERVER_PORT 1
#define MUSIC_SERVER_PORT 2

typedef std::function<void(LiveDanmaku)> DanmakuFunc;
typedef std::function<void(QString)> StringFunc;

class MainWindow : public QMainWindow, public CommonValues
{
    Q_OBJECT
    Q_PROPERTY(double paletteProg READ getPaletteBgProg WRITE setPaletteBgProg)
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    struct Diange
    {
        QString nickname;
        qint64 uid;
        QString name;
        QDateTime time;
    };

    struct HostInfo
    {
        QString host;
        int port;
        int wss_port;
        int ws_port;
    };

    struct HeaderStruct
    {
        int totalSize;
        short headerSize;
        short ver;
        int operation;
        int seqId;
    };

    struct FanBean
    {
        qint64 mid;
        QString uname;
        int attribute; // 0：未关注,2：已关注,6：已互粉
        qint64 mtime;
    };

    enum Operation
    {
        HANDSHAKE = 0,
        HANDSHAKE_REPLY = 1,
        HEARTBEAT = 2, // 心跳包
        HEARTBEAT_REPLY = 3, // 心跳包回复（人气值）
        SEND_MSG = 4,
        SEND_MSG_REPLY = 5, // 普通包（命令）
        DISCONNECT_REPLY = 6,
        AUTH = 7, // 认证包
        AUTH_REPLY = 8, // 认证包（回复）
        RAW = 9,
        PROTO_READY = 10,
        PROTO_FINISH = 11,
        CHANGE_ROOM = 12,
        CHANGE_ROOM_REPLY = 13,
        REGISTER = 14,
        REGISTER_REPLY = 15,
        UNREGISTER = 16,
        UNREGISTER_REPLY = 17
    };

    enum CmdResponse
    {
        NullRes,
        AbortRes,
        DelayRes,
    };

    enum VoicePlatform
    {
        VoiceLocal,
        VoiceXfy,
        VoiceCustom
    };

    const QSettings &getSettings() const;

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent (QEvent * event) override;
    void paintEvent(QPaintEvent * event) override;

signals:
    void signalRoomChanged(QString roomId);
    void signalLiveStart(QString roomId);
    void signalNewDanmaku(LiveDanmaku danmaku);
    void signalRemoveDanmaku(LiveDanmaku danmaku);
    void signalCmdEvent(QString cmd, LiveDanmaku danmaku);

public slots:
    void pullLiveDanmaku();
    void removeTimeoutDanmaku();

private slots:
    void on_DiangeAutoCopyCheck_stateChanged(int);

    void on_testDanmakuButton_clicked();

    void on_removeDanmakuIntervalSpin_valueChanged(int arg1);

    void on_roomIdEdit_editingFinished();

    void on_languageAutoTranslateCheck_stateChanged(int);

    void on_tabWidget_tabBarClicked(int index);

    void on_SendMsgButton_clicked();

    void on_AIReplyCheck_stateChanged(int);

    void on_testDanmakuEdit_returnPressed();

    void on_SendMsgEdit_returnPressed();

    void on_taskListWidget_customContextMenuRequested(const QPoint &);

    void on_replyListWidget_customContextMenuRequested(const QPoint &);

    void on_eventListWidget_customContextMenuRequested(const QPoint &pos);

    void on_addTaskButton_clicked();

    void on_addReplyButton_clicked();

    void on_addEventButton_clicked();

    void slotDiange(LiveDanmaku danmaku);

    void sendMsg(QString msg);
    void sendRoomMsg(QString roomId, QString msg);
    void sendAutoMsg(QString msgs);
    void slotSendAutoMsg();
    void sendCdMsg(QString msg, int cd, int channel, bool enableText, bool enableVoice, bool manual = false);
    void sendGiftMsg(QString msg);

    void sendAttentionMsg(QString msg);

    void sendNotifyMsg(QString msg);

    void slotSocketError(QAbstractSocket::SocketError error);

    void slotBinaryMessageReceived(const QByteArray &message);

    void slotUncompressBytes(const QByteArray &body);

    void splitUncompressedBody(const QByteArray &unc);

    void on_autoSendWelcomeCheck_stateChanged(int arg1);

    void on_autoSendGiftCheck_stateChanged(int arg1);

    void on_autoWelcomeWordsEdit_textChanged();

    void on_autoThankWordsEdit_textChanged();

    void on_startLiveWordsEdit_editingFinished();

    void on_endLiveWordsEdit_editingFinished();

    void on_startLiveSendCheck_stateChanged(int arg1);

    void on_autoSendAttentionCheck_stateChanged(int arg1);

    void on_autoAttentionWordsEdit_textChanged();

    void on_sendWelcomeCDSpin_valueChanged(int arg1);

    void on_sendGiftCDSpin_valueChanged(int arg1);

    void on_sendAttentionCDSpin_valueChanged(int arg1);

    void showDiangeHistory();

    void addBlockUser(qint64 uid, int hour);

    void addBlockUser(qint64 uid, qint64 roomId, int hour);

    void delBlockUser(qint64 uid);

    void delBlockUser(qint64 uid, qint64 roomId);

    void delRoomBlockUser(qint64 id);

    void eternalBlockUser(qint64 uid, QString uname);

    void cancelEternalBlockUser(qint64 uid);

    void cancelEternalBlockUserAndUnblock(qint64 uid);

    void saveEternalBlockUsers();

    void detectEternalBlockUsers();

    void on_enableBlockCheck_clicked();

    void on_newbieTipCheck_clicked();

    void on_autoBlockNewbieCheck_clicked();

    void on_autoBlockNewbieKeysEdit_textChanged();

    void on_autoBlockNewbieNotifyCheck_clicked();

    void on_autoBlockNewbieNotifyWordsEdit_textChanged();

    void on_saveDanmakuToFileCheck_clicked();

    void on_promptBlockNewbieCheck_clicked();

    void on_promptBlockNewbieKeysEdit_textChanged();

    void on_timerConnectServerCheck_clicked();

    void on_startLiveHourSpin_valueChanged(int arg1);

    void on_endLiveHourSpin_valueChanged(int arg1);

    void on_calculateDailyDataCheck_clicked();

    void on_pushButton_clicked();

    void on_removeDanmakuTipIntervalSpin_valueChanged(int arg1);

    void on_doveCheck_clicked();

    void on_notOnlyNewbieCheck_clicked();

    void on_pkAutoMelonCheck_clicked();

    void on_pkMaxGoldButton_clicked();

    void on_pkJudgeEarlyButton_clicked();

    void on_roomIdEdit_returnPressed();

    void on_actionData_Path_triggered();

    void on_actionShow_Live_Danmaku_triggered();

    void on_actionSet_Cookie_triggered();

    void on_actionSet_Danmaku_Data_Format_triggered();

    void on_actionCookie_Help_triggered();

    void on_actionCreate_Video_LRC_triggered();

    void on_actionShow_Order_Player_Window_triggered();

    void on_diangeReplyCheck_clicked();

    void on_actionAbout_triggered();

    void on_actionGitHub_triggered();

    void on_actionCustom_Variant_triggered();

    void on_actionSend_Long_Text_triggered();

    void on_actionShow_Lucky_Draw_triggered();

    void on_actionGet_Play_Url_triggered();

    void on_actionShow_Live_Video_triggered();

    void on_actionShow_PK_Video_triggered();

    void on_pkChuanmenCheck_clicked();

    void on_pkMsgSyncCheck_clicked();

    void slotPkBinaryMessageReceived(const QByteArray &message);

    void on_actionMany_Robots_triggered();

    void on_judgeRobotCheck_clicked();

    void showWidget(QSystemTrayIcon::ActivationReason reason);

    void on_actionAdd_Room_To_List_triggered();

    void on_recordCheck_clicked();

    void on_recordSplitSpin_valueChanged(int arg1);

    void on_sendWelcomeTextCheck_clicked();

    void on_sendWelcomeVoiceCheck_clicked();

    void on_sendGiftTextCheck_clicked();

    void on_sendGiftVoiceCheck_clicked();

    void on_sendAttentionTextCheck_clicked();

    void on_sendAttentionVoiceCheck_clicked();

    void on_enableScreenDanmakuCheck_clicked();

    void on_enableScreenMsgCheck_clicked();

    void on_screenDanmakuLeftSpin_valueChanged(int arg1);

    void on_screenDanmakuRightSpin_valueChanged(int arg1);

    void on_screenDanmakuTopSpin_valueChanged(int arg1);

    void on_screenDanmakuBottomSpin_valueChanged(int arg1);

    void on_screenDanmakuSpeedSpin_valueChanged(int arg1);

    void on_screenDanmakuFontButton_clicked();

    void on_screenDanmakuColorButton_clicked();

    void on_autoSpeekDanmakuCheck_clicked();

    void on_diangeFormatEdit_textEdited(const QString &text);

    void on_diangeNeedMedalCheck_clicked();

    void on_showOrderPlayerButton_clicked();

    void on_diangeShuaCheck_clicked();

    void on_pkMelonValButton_clicked();

    void slotPkEnding();

    void slotStartWork();

    void on_autoSwitchMedalCheck_clicked();

    void on_sendAutoOnlyLiveCheck_clicked();

    void on_autoDoSignCheck_clicked();

    void on_actionRoom_Status_triggered();

    void on_autoLOTCheck_clicked();

    void on_localDebugCheck_clicked();

    void on_blockNotOnlyNewbieCheck_clicked();

    void on_autoBlockTimeSpin_editingFinished();

    void triggerCmdEvent(QString cmd, LiveDanmaku danmaku);

    void on_voiceLocalRadio_toggled(bool checked);

    void on_voiceXfyRadio_toggled(bool checked);

    void on_voiceCustomRadio_toggled(bool checked);

    void on_voiceNameEdit_editingFinished();

    void on_voiceNameSelectButton_clicked();

    void on_voicePitchSlider_valueChanged(int value);

    void on_voiceSpeedSlider_valueChanged(int value);

    void on_voiceVolumeSlider_valueChanged(int value);

    void on_voicePreviewButton_clicked();

    void on_voiceLocalRadio_clicked();

    void on_voiceXfyRadio_clicked();

    void on_voiceCustomRadio_clicked();

    void on_label_10_linkActivated(const QString &link);

    void on_xfyAppIdEdit_textEdited(const QString &text);

    void on_xfyApiSecretEdit_textEdited(const QString &text);

    void on_xfyApiKeyEdit_textEdited(const QString &text);

    void on_voiceCustomUrlEdit_editingFinished();

    void on_eternalBlockListButton_clicked();

    void on_AIReplyMsgCheck_clicked();

    void slotAIReplyed(QString reply);

    void on_danmuLongestSpin_editingFinished();

    void on_startupAnimationCheck_clicked();

    void serverHandle(QHttpRequest *req, QHttpResponse *resp);

    void serverHandleUrl(QString urlPath, QHttpRequest *req, QHttpResponse *resp);

    void on_serverCheck_clicked();

    void on_serverPortSpin_editingFinished();

    void on_autoPauseOuterMusicCheck_clicked();

    void on_outerMusicKeyEdit_textEdited(const QString &arg1);

    void on_acquireHeartCheck_clicked();

    void on_sendExpireGiftCheck_clicked();

    void on_actionPicture_Browser_triggered();

    void on_orderSongsToFileCheck_clicked();

    void on_orderSongsToFileFormatEdit_textEdited(const QString &arg1);

    void on_orderSongsToFileMaxSpin_editingFinished();

    void on_actionCatch_You_Online_triggered();

    void on_pkBlankButton_clicked();

    void on_actionUpdate_New_Version_triggered();

    void on_startOnRebootCheck_clicked();

private:
    void appendNewLiveDanmakus(QList<LiveDanmaku> roomDanmakus);
    void appendNewLiveDanmaku(LiveDanmaku danmaku);
    void newLiveDanmakuAdded(LiveDanmaku danmaku);
    void oldLiveDanmakuRemoved(LiveDanmaku danmaku);
    void addNoReplyDanmakuText(QString text);
    void localNotify(QString text);
    void localNotify(QString text, qint64 uid);

    void addTimerTask(bool enable, int second, QString text);
    void saveTaskList();
    void restoreTaskList();

    void addAutoReply(bool enable, QString key, QString reply);
    void saveReplyList();
    void restoreReplyList();

    void addEventAction(bool enable, QString cmd, QString action);
    void saveEventList();
    void restoreEventList();

    QVariant getCookies();
    void getUserInfo();
    void getRoomUserInfo();
    void initWS();
    void startConnectRoom();
    void sendXliveHeartBeatE();
    void sendXliveHeartBeatX();
    void sendXliveHeartBeatX(QString s, qint64 timestamp);
    void getRoomInit();
    void getRoomInfo(bool reconnect);
    bool isLivingOrMayliving();
    void getRoomCover(QString url);
    QPixmap getRoundedPixmap(QPixmap pixmap) const;
    void getUpFace(QString uid);
    void getUpPortrait(QString faceUrl);
    QPixmap getLivingPixmap(QPixmap pixmap) const;
    void getDanmuInfo();
    void getFansAndUpdate();
    void getPkInfoById(QString roomId, QString pkId);
    void startMsgLoop();
    QByteArray makePack(QByteArray body, qint32 operation);
    void sendVeriPacket(QWebSocket *socket, QString roomId, QString token);
    void sendHeartPacket();
    void handleMessage(QJsonObject json);
    bool mergeGiftCombo(LiveDanmaku danmaku);
    bool handlePK(QJsonObject json);
    bool handlePK2(QJsonObject json);
    void refreshBlockList();
    bool isInFans(qint64 upUid);
    void sendGift(int giftId, int giftNum);
    void sendBagGift(int giftId, int giftNum, qint64 bagId);
    void getRoomLiveVideoUrl(StringFunc func = nullptr);
    void roomEntryAction();
    void sendExpireGift();
    void getBagList(qint64 sendExpire = 0);
    void updateExistGuards(int page);

    QString getLocalNickname(qint64 name) const;
    void analyzeMsgAndCd(QString &msg, int& cd, int& channel) const;
    QString processTimeVariants(QString msg) const;
    QStringList getEditConditionStringList(QString plainText, LiveDanmaku user) const;
    QString processDanmakuVariants(QString msg, LiveDanmaku danmaku) const;
    QString processMsgHeaderConditions(QString msg) const;
    bool processVariantConditions(QString exprs) const;
    qint64 calcIntExpression(QString exp) const;
    template<typename T>
    bool isConditionTrue(T a, T b, QString op) const;
    QString nicknameSimplify(QString nickname) const;
    QString msgToShort(QString msg) const;
    double getPaletteBgProg() const;
    void setPaletteBgProg(double x);

    void sendWelcomeIfNotRobot(LiveDanmaku danmaku);
    void sendAttentionThankIfNotRobot(LiveDanmaku danmaku);
    void judgeUserRobotByFans(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs = nullptr);
    void judgeUserRobotByUpstate(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs = nullptr);
    void judgeUserRobotByUpload(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs = nullptr);
    void sendWelcome(LiveDanmaku danmaku);
    void sendAttentionThans(LiveDanmaku danmaku);
    void judgeRobotAndMark(LiveDanmaku danmaku);
    void markNotRobot(qint64 uid);
    void initTTS();
    void speekVariantText(QString text);
    void speakText(QString text);
    void downloadAndSpeak(QString text);
    void showScreenDanmaku(LiveDanmaku danmaku);

    void startSaveDanmakuToFile();
    void finishSaveDanmuToFile();
    void startCalculateDailyData();
    void saveCalculateDailyData();
    void saveTouta();
    void startLiveRecord();
    void startRecordUrl(QString url);
    void finishLiveRecord();

    void processRemoteCmd(QString msg, bool response = true);
    bool execFunc(QString msg, CmdResponse& res, int& resVal);
    void simulateKeys(QString seq);

    void restoreCustomVariant(QString text);
    QString saveCustomVariant();
    void saveOrderSongs(const SongList& songs);

    void pkPre(QJsonObject json);
    void pkStart(QJsonObject json);
    void pkProcess(QJsonObject json);
    void pkEnd(QJsonObject json);
    void getRoomCurrentAudiences(QString roomId, QSet<qint64> &audiences);
    void connectPkRoom();
    void uncompressPkBytes(const QByteArray &body);
    void handlePkMessage(QJsonObject json);
    bool shallAutoMsg() const;

    void releaseLiveData();
    QRect getScreenRect();
    QPixmap toRoundedPixmap(QPixmap pixmap, int radius = 5) const;

    void switchMedalTo(qint64 targetRoomId);
    void wearMedal(qint64 medalId);
    void doSign();
    void joinLOT(qint64 id, bool follow = true);
    void sendPrivateMsg(qint64 uid, QString msg);

    void startSplash();

    void get(QString url, NetStringFunc func);
    void get(QString url, NetJsonFunc func);
    void get(QString url, NetReplyFunc func);
    void post(QString url, QStringList params, NetJsonFunc func);
    void post(QString url, QByteArray ba, NetJsonFunc func);
    void post(QString url, QByteArray ba, NetReplyFunc func);

    void openServer(int port = 0);
    void initServerData();
    void closeServer();
    void sendSocketCmd(QString cmd, LiveDanmaku danmaku);

    void initMusicServer();
    void sendMusicList(const SongList& songs, QWebSocket* socket = nullptr);

    void syncMagicalRooms();

    QString GetFileVertion(QString fullName);

private:
    Ui::MainWindow *ui;
    QSettings settings;
    QString appNewVersion;
    QString appDownloadUrl;

    // 房间信息
    QString roomId;
    int liveStatus = 0; // 是否正在直播
    QString upName;
    QString roomTitle;
    QPixmap roomCover;
    QPixmap upFace;
    QString areaId; // 例：21（整型，为了方便用字符串）
    QString areaName; // 例：视频唱见
    QString parentAreaId; // 例：1（整型，为了方便用字符串）
    QString parentAreaName; // 例：娱乐
    bool justStart = true; // 启动几秒内不进行发送，避免一些尴尬场景
    QTimer* hourTimer = nullptr;

    QTimer* xliveHeartBeatTimer = nullptr;
    int xliveHeartBeatIndex = 0;         // 发送心跳的索引（每次+1）
    qint64 xliveHeartBeatEts = 0;        // 上次心跳时间戳
    int xliveHeartBeatInterval = 60;     // 上次心时间跳间隔（实测都是60）
    QString xliveHeartBeatBenchmark;     // 上次心跳秘钥参数（实测每次都一样）
    QJsonArray xliveHeartBeatSecretRule; // 上次心跳加密间隔（实测每次都一样）
    QString encServer = "https://1578907340179965.cn-shanghai.fc.aliyuncs.com/2016-08-15/proxy/bili_server/heartbeat/";

    // 动画
    double paletteProg = 0;
    BFSColor prevPa;
    BFSColor currentPa;

    // 粉丝数量
    int currentFans = 0;
    int currentFansClub = 0;
    QList<FanBean> fansList; // 最近的关注，按时间排序
    int popularVal = 2;

    // 弹幕信息
    QList<LiveDanmaku> roomDanmakus;
    LiveDanmakuWindow* danmakuWindow = nullptr;
#ifndef SOCKET_MODE
    QTimer* danmakuTimer;
    qint64 prevLastDanmakuTimestamp = 0;
    bool firstPullDanmaku = true; // 是否不加载以前的弹幕
#endif
    QTimer* removeTimer;
    qint64 removeDanmakuInterval = 60000;
    QFile* danmuLogFile = nullptr;
    QTextStream* danmuLogStream = nullptr;
    qint64 removeDanmakuTipInterval = 20000;
    QStringList noReplyMsgs;
    int danmuLongest = 20;

    // 发送弹幕队列
    QList<QStringList> autoMsgQueues; // 待发送的自动弹幕，是一个二维列表！
    QTimer* autoMsgTimer;

    // 点歌
    bool diangeAutoCopy = false;
    QList<Diange> diangeHistory;
    QString diangeFormatString;
    OrderPlayerWindow* musicWindow = nullptr;

    // 控件
    QTimer* sendMsgTimer;
    QLabel* statusLabel;
    QLabel* fansLabel;
    QLabel* rankLabel;

    // 连接信息
    QString cookieUid; // 自己的UID
    QString cookieUname; // 自己的昵称
    bool localDebug = false; // 本地调试模式

    QString shortId; // 房间短号（有些没有，也没什么用）
    QString upUid; // 主播的UID
    QList<HostInfo> hostList;
    QString token;
    QWebSocket* socket;
    QTimer* heartTimer;
    QTimer* connectServerTimer;
    bool remoteControl = true;

    // 每日数据
    QSettings* dailySettings = nullptr;
    int dailyCome = 0; // 进来数量人次
    int dailyPeopleNum = 0; // 本次进来的人数（不是全程的话，不准确）
    int dailyDanmaku = 0; // 弹幕数量
    int dailyNewbieMsg = 0; // 新人发言数量（需要开启新人发言提示）
    int dailyNewFans = 0; // 关注数量
    int dailyTotalFans = 0; // 粉丝总数量（需要开启感谢关注）
    int dailyGiftSilver = 0; // 银瓜子总价值
    int dailyGiftGold = 0; // 金瓜子总价值
    int dailyGuard = 0; // 上船/续船人次

    // 录播
    qint64 startRecordTime = 0;
    QString recordUrl;
    QEventLoop* recordLoop = nullptr;
    QTimer* recordTimer = nullptr;

    // 大乱斗
    bool pking = false;
    qint64 pkToLive = 0; // PK导致的下播（视频必定触发）
    int myVotes = 0;
    int matchVotes = 0;
    qint64 pkEndTime = 0;
    QTimer* pkTimer = nullptr;
    int pkJudgeEarly = 2000;
    bool pkVideo = false;

    // 大乱斗偷塔
    int goldTransPk = 100; // 金瓜子转乱斗值的比例，除以10还是100
    int pkMaxGold = 300; // 单位是金瓜子，积分要/10
    bool pkEnding = false;
    int pkVoting = 0;
    int toutaCount = 0;
    int chiguaCount = 0;
    int oppositeTouta = 0; // 对面是否偷塔（用作判断）
    QStringList toutaBlankList; // 偷塔黑名单
    QStringList magicalRooms; // 同样使用神奇弹幕的房间

    // 大乱斗串门
    bool pkChuanmenEnable = false;
    int pkMsgSync = 0;
    QString pkRoomId;
    QString pkUid;
    QString pkUname;
    QSet<qint64> myAudience; // 自己这边的观众
    QSet<qint64> oppositeAudience; // 对面的观众
    QWebSocket* pkSocket = nullptr; // 连接对面的房间
    QString pkToken;
    QHash<qint64, int> cmAudience; // 自己这边跑过去串门了: 1串门，0已经回来/提示

    // 欢迎
    qint64 msgCds[CHANNEL_COUNT] = {}; // 冷却通道

    // 自动禁言
    QList<LiveDanmaku> blockedQueue; // 本次自动禁言的用户，用来撤销

    // 弹幕人气判断
    QTimer* danmuPopularTimer;
    int minuteDanmuPopular = 0;
    QList<int> danmuPopularQueue;
    int danmuPopularValue = 0;

    // 抽奖机
    LuckyDrawWindow* luckyDrawWindow = nullptr;

    // 视频
    LiveVideoPlayer* videoPlayer = nullptr;

    // 机器人
    int judgeRobot = 0;
    QSettings robotRecord;
    QList<QWebSocket*> robots_sockets;

    // 托盘
    QMenu *trayMenu;//托盘菜单
    QSystemTrayIcon *tray;//托盘图标添加成员

    // 文字转语音
    VoicePlatform voicePlatform = VoiceLocal;
    QTextToSpeech *tts = nullptr;
    XfyTTS* xfyTTS = nullptr;
    int voicePitch = 50;
    int voiceSpeed = 50;
    int voiceVolume = 50;
    QString voiceName;

    // 全屏弹幕
    QFont screenDanmakuFont;
    QColor screenDanmakuColor;
    QList<QLabel*> screenLabels;

    // 游戏列表
    QList<qint64> gameUsers[CHANNEL_COUNT];

    // 服务端
    QHttpServer *server = nullptr;
    qint16 serverPort = 0;
    QDir wwwDir;
    QHash<QString, QString> contentTypeMap;
    QWebSocketServer* danmakuSocketServer = nullptr;
    QList<QWebSocket*> danmakuSockets;
    QHash<QWebSocket*, QStringList> danmakuCmdsMaps;

    QWebSocketServer* musicSocketServer = nullptr;
    QList<QWebSocket*> musicSockets;

    // 截图管理
    PictureBrowser* pictureBrowser = nullptr;
};
#endif // MAINWINDOW_H
