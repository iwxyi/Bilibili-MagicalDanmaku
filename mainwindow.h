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
#include "netutil.h"
#include "livedanmaku.h"
#include "livedanmakuwindow.h"
#include "taskwidget.h"
#include "commonvalues.h"
#include "orderplayerwindow.h"
#include "textinputdialog.h"
#include "luckydrawwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define SOCKET_DEB if (0) qDebug()
#define SOCKET_INF if (0) qDebug()
#define SOCKET_MODE

#define CONNECT_SERVER_INTERVAL 1800000

class MainWindow : public QMainWindow, public CommonValues
{
    Q_OBJECT

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

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

signals:
    void signalNewDanmaku(LiveDanmaku danmaku);
    void signalRemoveDanmaku(LiveDanmaku danmaku);

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

    void on_addTaskButton_clicked();

    void sendMsg(QString msg);

    void sendAutoMsg(QString msgs);

    void sendWelcomeMsg(QString msg);

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

    void on_diangeHistoryButton_clicked();

    void addBlockUser(qint64 upUid, int hour);

    void delBlockUser(qint64 upUid);

    void delRoomBlockUser(qint64 id);

    void on_enableBlockCheck_clicked();

    void on_newbieTipCheck_clicked();

    void on_diangeFormatButton_clicked();

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

private:
    void appendNewLiveDanmakus(QList<LiveDanmaku> roomDanmakus);
    void appendNewLiveDanmaku(LiveDanmaku danmaku);
    void newLiveDanmakuAdded(LiveDanmaku danmaku);
    void oldLiveDanmakuRemoved(LiveDanmaku danmaku);
    void addNoReplyDanmakuText(QString text);
    void showLocalNotify(QString text);

    void addTimerTask(bool enable, int second, QString text);
    void saveTaskList();
    void restoreTaskList();

    QVariant getCookies();
    void getUserInfo();
    void getRoomUserInfo();
    void initWS();
    void startConnectRoom();
    void getRoomInit();
    void getRoomInfo();
    void getDanmuInfo();
    void getFansAndUpdate();
    void startMsgLoop();
    QByteArray makePack(QByteArray body, qint32 operation);
    void sendVeriPacket();
    void sendHeartPacket();
    void handleMessage(QJsonObject json);
    bool handlePK(QJsonObject json);
    bool handlePK2(QJsonObject json);
    void refreshBlockList();
    bool isInFans(qint64 upUid);
    void sendGify(int giftId, int giftNum);

    QByteArray zlibUncompress(QByteArray ba) const;
    QString getLocalNickname(qint64 name) const;
    QString processTimeVariants(QString msg) const;
    QStringList getEditConditionStringList(QString plainText, LiveDanmaku user) const;
    QString processDanmakuVariants(QString msg, LiveDanmaku danmaku) const;
    QString processVariantConditions(QString msg) const;
    qint64 calcIntExpression(QString exp) const;
    template<typename T>
    bool isConditionTrue(T a, T b, QString op) const;
    QString nicknameSimplify(QString nickname) const;
    QString msgToShort(QString msg) const;

    void startSaveDanmakuToFile();
    void finishSaveDanmuToFile();
    void startCalculateDailyData();
    void saveCalculateDailyData();
    void saveTouta();

    void processDanmakuCmd(QString msg);

    void restoreCustomVariant(QString text);
    QString saveCustomVariant();

private:
    Ui::MainWindow *ui;
    QSettings settings;

    // 房间信息
    QString roomId;
    int liveStatus = 0; // 是否正在直播
    QString upName;
    QString roomName;
    QPixmap coverPixmap;
    bool justStart = true; // 启动10秒内不进行发送，避免一些误会

    // 粉丝数量
    int currentFans = 0;
    int currentFansClub = 0;
    QList<FanBean> fansList; // 最近的关注，按时间排序

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

    // 点歌
    bool diangeAutoCopy = false;
    QList<Diange> diangeHistory;
    QString diangeFormatString;
    OrderPlayerWindow* playerWindow = nullptr;

    // 登陆信息
    QString browserCookie;
    QString browserData;
    QString csrf_token;
    QTimer* sendMsgTimer;
    QLabel* statusLabel;

    // 连接信息
    QString cookieUid; // 自己的UID
    QString cookieUname; // 自己的昵称

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

    // 大乱斗
    bool pking = false;
    int myVotes = 0;
    int matchVotes = 0;
    qint64 pkEndTime = 0;
    QTimer* pkTimer = nullptr;
    int pkJudgeEarly = 2000;
    int pkMaxGold = 300; // 单位是金瓜子，积分要/10
    bool pkEnding = false;
    int pkVoting = 0;
    int toutaCount = 0;
    int chiguaCount = 0;

    // 弹幕人气判断
    QTimer* danmuPopularTimer;
    int minuteDanmuPopular = 0;
    QList<int> danmuPopularQueue;
    int danmuPopularValue = 0;

    // 抽奖机
    LuckyDrawWindow* luckyDrawWindow = nullptr;

};
#endif // MAINWINDOW_H
