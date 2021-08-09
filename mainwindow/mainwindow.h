#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSettings>
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
#if defined(ENABLE_TEXTTOSPEECH)
#include <QtTextToSpeech/QTextToSpeech>
#endif
#if defined(ENABLE_HTTP_SERVER)
#include <qhttpserver.h>
#include <qhttprequest.h>
#include <qhttpresponse.h>
#endif
#include <QWebSocketServer>
#include "netutil.h"
#include "livedanmaku.h"
#include "livedanmakuwindow.h"
#include "taskwidget.h"
#include "replywidget.h"
#include "eventwidget.h"
#include "commonvalues.h"
#include "orderplayerwindow.h"
#include "textinputdialog.h"
#include "luckydrawwindow.h"
#include "livevideoplayer.h"
#include "xfytts.h"
#include "eternalblockdialog.h"
#include "picturebrowser.h"
#include "netinterface.h"
#include "waterfloatbutton.h"
#include "custompaintwidget.h"
#include "appendbutton.h"
#include "waterzoombutton.h"
#include "tipbox.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define SOCKET_DEB if (0) qDebug() // 输出调试信息
#define SOCKET_INF if (0) qDebug() // 输出数据包信息
#define CALC_DEB if (0) qDebug() // 输出计算相关的信息

#define CONNECT_SERVER_INTERVAL 1800000

#define AUTO_MSG_CD 1500
#define NOTIFY_CD 2000

#define CHANNEL_COUNT 100
#define MAGICAL_SPLIT_CHAR "-bdm-split-bdm-"

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

#define PAGE_ROOM 0
#define PAGE_DANMAKU 1
#define PAGE_THANK 2
#define PAGE_MUSIC 3
#define PAGE_EXTENSION 4
#define PAGE_PREFENCE 5

#define TAB_TIMER_TASK 0   // 定时任务
#define TAB_AUTO_REPLY 1   // 自动回复
#define TAB_EVENT_ACTION 2 // 事件动作

#define FILTER_MUSIC_ORDER "FILTER_MUSIC_ORDER"
#define FILTER_DANMAKU_MSG "FILTER_DANMAKU_MSG"
#define FILTER_DANMAKU_COME "FILTER_DANMAKU_COME"
#define FILTER_DANMAKU_GIFT "FILTER_DANMAKU_GIFT"

class MainWindow;

typedef void(MainWindow::*VoidFunc)();
typedef ListItemInterface*(MainWindow::*InsertItemFunc)(MyJson);

typedef std::function<void(LiveDanmaku)> DanmakuFunc;
typedef std::function<void(QString)> StringFunc;

class MainWindow : public QMainWindow, public CommonValues, public NetInterface
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

    struct GiftCombo
    {
        qint64 uid;
        QString uname;
        qint64 giftId;
        QString giftName;
        int count;          // 数量
        qint64 total_coins; // 金瓜子数量

        GiftCombo(qint64 uid, QString uname, qint64 giftId, QString giftName, int count, int coins)
            : uid(uid), uname(uname), giftId(giftId), giftName(giftName), count(count), total_coins(coins)
        {}

        void merge(LiveDanmaku danmaku)
        {
            if (this->giftId != danmaku.getGiftId() || this->uid != danmaku.getUid())
            {
                qWarning() << "合并礼物数据错误：" << toString() << danmaku.toString();
            }
            this->count += danmaku.getNumber();
            this->total_coins += danmaku.getTotalCoin();
        }

        QString toString() const
        {
            return QString("%1(%2):%3(%4)x%5(%6)")
                    .arg(uname).arg(uname)
                    .arg(giftName).arg(giftId)
                    .arg(count).arg(total_coins);
        }
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

    const QSettings *getSettings() const;

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

    void on_eventListWidget_customContextMenuRequested(const QPoint &);

    void addListItemOnCurrentPage();

    void slotDiange(LiveDanmaku danmaku);

    void sendMsg(QString msg);
    void sendRoomMsg(QString roomId, QString msg);
    void sendAutoMsg(QString msgs, const LiveDanmaku& danmaku);
    void sendAutoMsgInFirst(QString msgs, const LiveDanmaku& danmaku, int interval = 0);
    void slotSendAutoMsg(bool timeout);
    void sendCdMsg(QString msg, const LiveDanmaku& danmaku, int cd, int channel, bool enableText, bool enableVoice, bool manual);
    void sendGiftMsg(QString msg, const LiveDanmaku& danmaku);
    void sendAttentionMsg(QString msg, const LiveDanmaku& danmaku);
    void sendNotifyMsg(QString msg, bool manual = false);
    void slotComboSend();

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

    void on_actionVariant_Translation_triggered();

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

    void on_orderSongShuaSpin_editingFinished();

    void on_pkMelonValButton_clicked();

    void slotPkEnding();

    void slotStartWork();

    void on_autoSwitchMedalCheck_clicked();

    void on_sendAutoOnlyLiveCheck_clicked();

    void on_autoDoSignCheck_clicked();

    void on_actionRoom_Status_triggered();

    void on_autoLOTCheck_clicked();

    void on_blockNotOnlyNewbieCheck_clicked();

    void on_autoBlockTimeSpin_editingFinished();

    void triggerCmdEvent(QString cmd, LiveDanmaku danmaku, bool debug = false);

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

    void slotAIReplyed(QString reply, qint64 uid);

    void on_danmuLongestSpin_editingFinished();

    void on_startupAnimationCheck_clicked();
#if defined (ENABLE_HTTP_SERVER)
    void serverHandle(QHttpRequest *req, QHttpResponse *resp);

    void serverHandleUrl(const QString &urlPath, QHash<QString, QString> &params, QHttpRequest *req, QHttpResponse *resp);
#endif
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

    void on_playingSongToFileCheck_clicked();

    void on_playingSongToFileFormatEdit_textEdited(const QString &arg1);

    void on_actionCatch_You_Online_triggered();

    void on_pkBlankButton_clicked();

    void on_actionUpdate_New_Version_triggered();

    void on_startOnRebootCheck_clicked();

    void on_domainEdit_editingFinished();

    void on_AIReplyIdButton_clicked();

    void on_AIReplyKeyButton_clicked();

    void prepareQuit();

    void on_giftComboSendCheck_clicked();

    void on_giftComboDelaySpin_editingFinished();

    void on_retryFailedDanmuCheck_clicked();

    void on_songLyricsToFileCheck_clicked();

    void on_songLyricsToFileMaxSpin_editingFinished();

    void on_allowWebControlCheck_clicked();

    void on_saveEveryGuardCheck_clicked();

    void on_saveMonthGuardCheck_clicked();

    void on_saveEveryGiftCheck_clicked();

    void on_exportDailyButton_clicked();

    void on_closeTransMouseButton_clicked();

    void on_pkAutoMaxGoldCheck_clicked();

    void on_saveRecvCmdsCheck_clicked();

    void on_allowRemoteControlCheck_clicked();

    void on_actionJoin_Battle_triggered();

    void on_actionQRCode_Login_triggered();

    void on_allowAdminControlCheck_clicked();

    void on_actionSponsor_triggered();

    void on_actionPaste_Code_triggered();

    void on_actionGenerate_Default_Code_triggered();

    void on_actionRead_Default_Code_triggered();

    void on_giftComboTopCheck_clicked();

    void on_giftComboMergeCheck_clicked();

    void on_listenMedalUpgradeCheck_clicked();

    void on_pushRecvCmdsButton_clicked();

    void on_pushNextCmdButton_clicked();

    void on_timerPushCmdCheck_clicked();

    void on_timerPushCmdSpin_editingFinished();

    void on_pkChuanmenCheck_stateChanged(int arg1);

    void on_actionLast_Candidate_triggered();

    void on_actionLocal_Mode_triggered();

    void on_actionDebug_Mode_triggered();

    void on_actionGuard_Online_triggered();

    void on_actionOfficial_Website_triggered();

    void on_actionAnchor_Case_triggered();

    void on_robotNameButton_clicked();

    void on_thankWelcomeTabButton_clicked();

    void on_thankGiftTabButton_clicked();

    void on_thankAttentionTabButton_clicked();

    void on_sendMsgMoreButton_clicked();

    void on_showLiveDanmakuWindowButton_clicked();

    void on_liveStatusButton_clicked();

    void on_tenCardLabel1_linkActivated(const QString &link);

    void on_tenCardLabel2_linkActivated(const QString &link);

    void on_tenCardLabel3_linkActivated(const QString &link);

    void on_tenCardLabel4_linkActivated(const QString &link);

    void on_musicBlackListButton_clicked();

    void setFilter(QString filterName, QString content);

    void on_enableFilterCheck_clicked();

    void on_actionReplace_Variant_triggered();

    void on_autoClearComeIntervalSpin_editingFinished();

    void on_roomDescriptionBrowser_anchorClicked(const QUrl &arg1);

    void on_adjustDanmakuLongestCheck_clicked();

    void on_actionBuy_VIP_triggered();

    void on_droplight_clicked();

    void on_vipExtensionButton_clicked();

    void on_enableTrayCheck_clicked();

    void on_toutaGiftCheck_clicked();

    void on_toutaGiftCountsEdit_textEdited(const QString &arg1);

    void on_toutaGiftListButton_clicked();

    void on_timerConnectIntervalSpin_editingFinished();

    void on_heartTimeSpin_editingFinished();

private:
    void initView();
    void initStyle();
    void initPath();
    void initRuntime();
    void readConfig();
    void initEvent();
    void adjustPageSize(int page);
    void switchPageAnimation(int page);

    void appendNewLiveDanmakus(QList<LiveDanmaku> roomDanmakus);
    void appendNewLiveDanmaku(LiveDanmaku danmaku);
    void newLiveDanmakuAdded(LiveDanmaku danmaku);
    void oldLiveDanmakuRemoved(LiveDanmaku danmaku);
    void addNoReplyDanmakuText(QString text);
    bool isLiving() const;
    void localNotify(QString text);
    void localNotify(QString text, qint64 uid);

    TaskWidget *addTimerTask(bool enable, int second, QString text, int index = -1);
    TaskWidget *addTimerTask(MyJson json);
    void saveTaskList();
    void restoreTaskList();

    ReplyWidget *addAutoReply(bool enable, QString key, QString reply, int index= -1);
    ReplyWidget *addAutoReply(MyJson json);
    void saveReplyList();
    void restoreReplyList();

    EventWidget *addEventAction(bool enable, QString cmd, QString action, int index= -1);
    EventWidget *addEventAction(MyJson json);
    void saveEventList();
    void restoreEventList();
    bool hasEvent(QString cmd) const;

    template<class T>
    void showListMenu(QListWidget* listWidget, QString listKey, VoidFunc saveFunc);

    void autoSetCookie(QString s);
    QVariant getCookies();
    void getCookieAccount();
    void getRobotInfo();
    void getRoomUserInfo();
    void initWS();
    void startConnectRoom();
    void sendXliveHeartBeatE();
    void sendXliveHeartBeatX();
    void sendXliveHeartBeatX(QString s, qint64 timestamp);
    void getRoomInit();
    void getRoomInfo(bool reconnect);
    bool isLivingOrMayliving();
    bool isWorking() const;

    void updatePermission();
    int hasPermission();
    void processNewDay();

    void getRoomCover(QString url);
    void setRoomCover(const QPixmap &pixmap);
    void setRoomThemeByCover(double val);
    void adjustCoverSizeByRoomCover(QPixmap pixmap);
    void adjustRoomIdWidgetPos();
    void showRoomIdWidget();
    void hideRoomIdWidget();
    QPixmap getRoundedPixmap(QPixmap pixmap) const;
    QPixmap getTopRoundedPixmap(QPixmap pixmap, int radius) const;
    void getUpInfo(QString uid);
    void getUpFace(QString faceUrl);
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
    void userComeEvent(LiveDanmaku& danmaku);
    void refreshBlockList();
    bool isInFans(qint64 upUid);
    void sendGift(int giftId, int giftNum);
    void sendBagGift(int giftId, int giftNum, qint64 bagId);
    void getRoomLiveVideoUrl(StringFunc func = nullptr);
    void roomEntryAction();
    void sendExpireGift();
    void getBagList(qint64 sendExpire = 0);
    void updateExistGuards(int page);
    void newGuardUpdate(const LiveDanmaku &danmaku);
    void updateOnlineGoldRank();
    void appendLiveGift(const LiveDanmaku& danmaku);
    void appendLiveGuard(const LiveDanmaku& danmaku);
    void getPkMatchInfo();
    void getPkOnlineGuardPage(int page);

    QString getLocalNickname(qint64 name) const;
    void analyzeMsgAndCd(QString &msg, int& cd, int& channel) const;
    QString processTimeVariants(QString msg) const;
    QStringList getEditConditionStringList(QString plainText, LiveDanmaku user);
    QString processDanmakuVariants(QString msg, const LiveDanmaku &danmaku);
    QString replaceDanmakuVariants(const LiveDanmaku &danmaku, const QString& key, bool* ok) const;
    QString replaceDanmakuJson(const QJsonObject& json, const QString &key_seq, bool *ok) const;
    QString replaceDynamicVariants(const QString& funcName, const QString& args, const LiveDanmaku &danmaku);
    QString processMsgHeaderConditions(QString msg) const;
    bool processVariantConditions(QString exprs) const;
    qint64 calcIntExpression(QString exp) const;
    template<typename T>
    bool isConditionTrue(T a, T b, QString op) const;
    bool isFilterRejected(QString filterName, const LiveDanmaku& danmaku);
    bool processFilter(QString filterText, const LiveDanmaku& danmaku);
    void translateUnicode(QString& s) const;

    qint64 unameToUid(QString text);
    QString uidToName(qint64 uid);
    QString nicknameSimplify(QString nickname) const;
    QString numberSimplify(int number) const;
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
    void restoreToutaGifts(QString text);
    void startLiveRecord();
    void startRecordUrl(QString url);
    void finishLiveRecord();

    void processRemoteCmd(QString msg, bool response = true);
    bool execFunc(QString msg, LiveDanmaku &danmaku, CmdResponse& res, int& resVal);
    void simulateKeys(QString seq);
    QStringList splitLongDanmu(QString text) const;
    void sendLongText(QString text);

    void restoreCustomVariant(QString text);
    QString saveCustomVariant();
    void restoreVariantTranslation();

    void restoreReplaceVariant(QString text);
    QString saveReplaceVariant();

    void savePlayingSong();
    void saveOrderSongs(const SongList& songs);
    void saveSongLyrics();

    void pkPre(QJsonObject json);
    void pkStart(QJsonObject json);
    void pkProcess(QJsonObject json);
    void pkEnd(QJsonObject json);
    void pkSettle(QJsonObject json);
    int getPkMaxGold(int votes);
    bool execTouta();
    void getRoomCurrentAudiences(QString roomId, QSet<qint64> &audiences);
    void connectPkRoom();
    void uncompressPkBytes(const QByteArray &body);
    void handlePkMessage(QJsonObject json);
    bool shallAutoMsg() const;
    bool shallAutoMsg(const QString& sl) const;
    bool shallAutoMsg(const QString& sl, bool& manual);
    void addBannedWord(QString word, QString anchor);

    void saveMonthGuard();
    void saveEveryGuard(LiveDanmaku danmaku);
    void saveEveryGift(LiveDanmaku danmaku);
    void appendFileLine(QString dirName, QString fileName, QString format, LiveDanmaku danmaku);

    void releaseLiveData(bool prepare = false);
    QRect getScreenRect();
    QPixmap toRoundedPixmap(QPixmap pixmap, int radius = 5) const;

    void switchMedalToRoom(qint64 targetRoomId);
    void switchMedalToUp(qint64 upId, int page = 1);
    void wearMedal(qint64 medalId);
    void doSign();
    void joinLOT(qint64 id, bool follow = true);
    void joinStorm(qint64 id);
    void sendPrivateMsg(qint64 uid, QString msg);
    void AIReply(qint64 id, QString text, NetStringFunc func, int maxLen = 50, int retry = 3);
    void joinBattle(int type);
    void detectMedalUpgrade(LiveDanmaku danmaku);
    void adjustDanmakuLongest();

    void startSplash();
    void saveGameNumbers(int channel);
    void restoreGameNumbers();
    void saveGameTexts(int channel);
    void restoreGameTexts();

    virtual void setUrlCookie(const QString &url, QNetworkRequest *request) override;
    void openLink(QString link);
    void addGuiGiftList(const LiveDanmaku& danmaku);

    void initServerData();
    void openServer(int port = 0);
    void openSocketServer();
    void processSocketTextMsg(QWebSocket* clientSocket, const QString& message);
    void closeServer();
    void sendDanmakuToSockets(QString cmd, LiveDanmaku danmaku);
    void sendJsonToSockets(QString cmd, QJsonValue data, QWebSocket* socket = nullptr);
    void processServerVariant(QByteArray& doc);
    QByteArray getApiContent(QString url, QHash<QString, QString> params, QString *contentType);
    void sendTextToSockets(QString cmd, QByteArray data, QWebSocket* socket = nullptr);
    void sendMusicList(const SongList& songs, QWebSocket* socket = nullptr);
    void sendLyricList(QWebSocket* socket = nullptr);
    QString webCache(QString name) const;

    void syncMagicalRooms();

    QString GetFileVertion(QString fullName);
    void upgradeVersionToLastest(QString oldVersion);
    void upgradeOneVersionData(QString beforeVersion);

    void generateDefaultCode(QString path = "");
    void readDefaultCode(QString path = "");
    void showError(QString title, QString s) const;
    void showError(QString s) const;

private:
    Ui::MainWindow *ui;
    QSettings* settings;
    QSettings* heaps;
    QString dataPath;
    QString appVersion; // 不带v
    QString appNewVersion;
    QString appDownloadUrl;

    // 控件
    const int widgetSizeL = 48;
    const int fluentRadius = int(5 * qApp->devicePixelRatio() + 0.5);
    const int giftImgSize = 60;
    QList<WaterZoomButton*> sideButtonList;
    QLabel* roomCoverLabel;
    QWidget* roomIdBgWidget;
    InteractiveButtonBase* roomSelectorBtn;
    QList<WaterFloatButton*> thankTabButtons;
    InteractiveButtonBase* extensionButton;
    CustomPaintWidget* musicTitleDecorateWidget;
    AppendButton* appendListItemButton;
    QLabel* statusLabel;
    TipBox* tip_box;
    InteractiveButtonBase* droplight;

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

    qint64 liveTimestamp = 0;
    QTimer* xliveHeartBeatTimer = nullptr;
    int xliveHeartBeatIndex = 0;         // 发送心跳的索引（每次+1）
    qint64 xliveHeartBeatEts = 0;        // 上次心跳时间戳
    int xliveHeartBeatInterval = 60;     // 上次心时间跳间隔（实测都是60）
    QString xliveHeartBeatBenchmark;     // 上次心跳秘钥参数（实测每次都一样）
    QJsonArray xliveHeartBeatSecretRule; // 上次心跳加密间隔（实测每次都一样）
    QString encServer = "http://iwxyi.com:6001/enc";
    int todayHeartMinite = 0; // 今天已经领取的小心心数量（本程序）

    // 动画
    double paletteProg = 0;
    BFSColor prevPa;
    BFSColor currentPa;

    // 颜色
    QColor themeBg = Qt::white;
    QColor themeFg = Qt::black;
    QColor themeSbg = Qt::white;
    QColor themeSfg = Qt::blue;
    QColor themeGradient = Qt::white;

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
    bool removeLongerRandomDanmaku = true; // 随机弹幕自动移除过长的
    LiveDanmaku lastDanmaku; // 最近一个弹幕
    int robotTotalSendMsg = 0; // 机器人发送的弹幕数量
    int liveTotalDanmaku = 0; // 本场直播的弹幕数量

    // 调试
    bool localDebug = false;   // 本地调试模式
    bool debugPrint = false;   // 调试输出模式

    bool saveRecvCmds = false; // 保存收到的CMD
    QFile* saveCmdsFile = nullptr;

    QFile* pushCmdsFile = nullptr;
    QTimer* pushCmdsTimer = nullptr;

    QString lastConditionDanmu;
    QString lastCandidateDanmaku;

    // 过滤器
    bool enableFilter = true;
    // 过滤器（已废弃方案）
    QString filter_musicOrder; // 点歌过滤
    QRegularExpression filter_musicOrderRe;
    QString filter_danmakuMsg; // 弹幕姬：消息
    QString filter_danmakuCome; // 弹幕姬：用户进入过滤
    QString filter_danmakuGift; // 弹幕姬：礼物过滤

    // 礼物连击
    QHash<QString, LiveDanmaku> giftCombos;
    QTimer* comboTimer = nullptr;

    // 发送弹幕队列
    QList<QPair<QStringList, LiveDanmaku>> autoMsgQueues; // 待发送的自动弹幕，是一个二维列表！
    QTimer* autoMsgTimer;
    bool inDanmakuCd = false;

    // 点歌
    bool diangeAutoCopy = false;
    QList<Diange> diangeHistory;
    QString diangeFormatString;
    OrderPlayerWindow* musicWindow = nullptr;

    // 连接信息
    QString cookieUid; // 自己的UID
    QString cookieUname; // 自己的昵称
    QString token;
    int cookieULevel = 0; // 自己的等级
    int cookieGuardLevel = 0; // 自己的大航海

    QString shortId; // 房间短号（有些没有，也没什么用）
    QString upUid; // 主播的UID

    QList<HostInfo> hostList;
    QWebSocket* socket;
    QTimer* heartTimer;
    QTimer* connectServerTimer;
    bool remoteControl = true;

    bool gettingRoom = false;
    bool gettingUser = false;
    bool gettingUp = false;
    QString serverPath = "http://iwxyi.com:8102/server/";
    int permissionLevel = 0;
    QTimer* permissionTimer = nullptr;

    // 每日数据
    QSettings* dailySettings = nullptr;
    QTimer* dayTimer = nullptr;
    int dailyCome = 0; // 进来数量人次
    int dailyPeopleNum = 0; // 本次进来的人数（不是全程的话，不准确）
    int dailyDanmaku = 0; // 弹幕数量
    int dailyNewbieMsg = 0; // 新人发言数量（需要开启新人发言提示）
    int dailyNewFans = 0; // 关注数量
    int dailyTotalFans = 0; // 粉丝总数量（需要开启感谢关注）
    int dailyGiftSilver = 0; // 银瓜子总价值
    int dailyGiftGold = 0; // 金瓜子总价值
    int dailyGuard = 0; // 上船/续船人次
    int dailyMaxPopul = 0; // 最高人气
    int dailyAvePopul = 0; // 平均人气
    bool todayIsEnding = false;

    QString recordFileCodec = ""; // 自动保存上船、礼物记录、每月船员等编码
    QString codeFileCodec = "UTF-8"; // 代码保存的文件编码
    QString externFileCodec = "UTF-8"; // 提供给外界读取例如歌曲文件编码

    // 船员
    bool updateGuarding = false;
    QList<LiveDanmaku> guardInfos;

    // 高能榜
    QList<LiveDanmaku> onlineGoldRank;
    QList<LiveDanmaku> onlineGuards;

    // 录播
    qint64 startRecordTime = 0;
    QString recordUrl;
    QEventLoop* recordLoop = nullptr;
    QTimer* recordTimer = nullptr;

    // 大乱斗
    bool pking = false;
    qint64 pkId = 0;
    qint64 pkToLive = 0; // PK导致的下播（视频必定触发）
    int myVotes = 0;
    int matchVotes = 0;
    qint64 pkEndTime = 0;
    QTimer* pkTimer = nullptr;
    int pkJudgeEarly = 2000;
    bool pkVideo = false;
    QList<LiveDanmaku> pkGifts;

    // 大乱斗偷塔
    int goldTransPk = 100; // 金瓜子转乱斗值的比例，除以10还是100
    int pkMaxGold = 300; // 单位是金瓜子，积分要/10
    bool pkEnding = false;
    int pkVoting = 0;
    int toutaCount = 0;
    int chiguaCount = 0;
    int toutaGold = 0;
    int oppositeTouta = 0; // 对面是否偷塔（用作判断）
    QStringList toutaBlankList; // 偷塔黑名单
    QStringList magicalRooms; // 同样使用神奇弹幕的房间
    QList<int> toutaGiftCounts; // 偷塔允许的礼物数量
    QList<LiveDanmaku> toutaGifts; // 用来偷塔的礼物信息

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
    QHash<qint64, qint64> cmAudience; // 自己这边跑过去串门了: timestamp10:串门，0已经回来/提示

    // 大乱斗对面信息
    int pkOnlineGuard1, pkOnlineGuard2, pkOnlineGuard3;

    // 欢迎
    qint64 msgCds[CHANNEL_COUNT] = {}; // 冷却通道

    // 自动禁言
    QList<LiveDanmaku> blockedQueue; // 本次自动禁言的用户，用来撤销

    // 直播间人气
    QTimer* minuteTimer;
    int currentPopul = 0; // 当前人气
    qint64 sumPopul = 0;     // 自启动以来的人气
    qint64 countPopul = 0;   // 自启动以来的人气总和

    // 弹幕人气
    int minuteDanmuPopul = 0;
    QList<int> danmuPopulQueue;
    int danmuPopulValue = 0;

    // 本次直播的礼物列表
    QList<LiveDanmaku> liveAllGifts;
    QList<LiveDanmaku> liveAllGuards;

    // 抽奖机
    LuckyDrawWindow* luckyDrawWindow = nullptr;

    // 视频
    LiveVideoPlayer* videoPlayer = nullptr;

    // 机器人
    int judgeRobot = 0;
    QSettings* robotRecord;
    QList<QWebSocket*> robots_sockets;

    // 点歌
    QStringList orderSongBlackList;

    // 托盘
    QMenu *trayMenu;//托盘菜单
    QSystemTrayIcon *tray;//托盘图标添加成员

    // 文字转语音
    VoicePlatform voicePlatform = VoiceLocal;
#if defined(ENABLE_TEXTTOSPEECH)
    QTextToSpeech *tts = nullptr;
#endif
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
    QList<qint64> gameNumberLists[CHANNEL_COUNT];
    QList<QString> gameTextLists[CHANNEL_COUNT];

    // 服务端
#ifndef Q_OS_MAC
    QHttpServer *server = nullptr;
#endif
    QString serverDomain;
    qint16 serverPort = 0;
    QDir wwwDir;
    QHash<QString, QString> contentTypeMap;
    QWebSocketServer* danmakuSocketServer = nullptr;
    QList<QWebSocket*> danmakuSockets;
    QHash<QWebSocket*, QStringList> danmakuCmdsMaps;

    bool sendSongListToSockets = false;
    bool sendLyricListToSockets = false;
    bool sendCurrentSongToSockets = false;

    // 截图管理
    PictureBrowser* pictureBrowser = nullptr;

    // 彩蛋
    QString warmWish;
};
#endif // MAINWINDOW_H
