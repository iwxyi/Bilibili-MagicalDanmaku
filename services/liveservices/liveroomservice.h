#ifndef LIVEROOMSERVICE_H
#define LIVEROOMSERVICE_H

#include <QWebSocket>
#include <QRegularExpression>
#include <QObjectCleanupHandler>
#include "widgets/netinterface.h"
#include "livestatisticservice.h"
#include "entities.h"

#define INTERVAL_RECONNECT_WS 5000
#define INTERVAL_RECONNECT_WS_MAX 60000
#define SOCKET_DEB if (0) qDebug() // 输出调试信息
#define SOCKET_INF if (0) qDebug() // 输出数据包信息

class LiveRoomService : public QObject, public NetInterface, public LiveStatisticService
{
    Q_OBJECT
    friend class MainWindow;

public:
    explicit LiveRoomService(QObject *parent = nullptr);

    /// 初始化所有变量，new、connect等
    virtual void init();
    void initTimeTasks();

    /// 读取设置
    virtual void readConfig();

    virtual void releaseLiveData(bool prepare);

signals:
    void signalStartConnectRoom(); // 通知总的开始连接
    void signalConnectionStarted(); // WebSocket开始连接
    void signalConnectionStateChanged(QAbstractSocket::SocketState state);
    void signalConnectionStateTextChanged(const QString& text);
    void signalStartWork(); // 直播开始/连接上就已经开始直播
    void signalLiveStarted();
    void signalLiveStopped();

    void signalRoomIdChanged(const QString &roomId); // 房间号改变，例如通过解析身份码导致的房间ID变更
    void signalUpUidChanged(const QString &uid);
    void signalUpFaceChanged(const QPixmap& pixmap);
    void signalUpInfoChanged();
    void signalUpSignatureChanged(const QString& signature);
    void signalRobotIdChanged(const QString &uid);
    void signalRoomInfoChanged();
    void signalImUpChanged(bool isUp);
    void signalRoomCoverChanged(const QPixmap &pixmap);
    void signalUpHeadChanged(const QPixmap &pixmap);
    void signalRobotHeadChanged(const QPixmap &pixmap);
    void signalGetRoomAndRobotFinished(); // 获取房间基础信息/机器人基础信息结束
    void signalFinishDove(); // 结束今天的鸽鸽
    void signalCanRecord(); // 直播中，如果连接的话则可以开始录播
    void signalGuardsChanged(); // 舰长变化
    void signalOnlineRankChanged(); // 高能榜变化
    void signalAutoAdjustDanmakuLongest(); // 调整弹幕最高字数
    void signalHeartTimeNumberChanged(int num, int minute); // 通过连接一直领取小心心
    void signalDanmuPopularChanged(const QString& text); // 5分钟弹幕人气
    void signalPopularChanged(qint64 count); // 人气值
    void signalSignInfoChanged(const QString& text); // 自动签到
    void signalSignDescChanged(const QString& text);
    void signalLOTInfoChanged(const QString& text); // 天选等活动
    void signalLOTDescChanged(const QString& text);
    void signalDanmakuLongestChanged(int length); // 弹幕长度

    void signalBattleEnabled(bool enable);
    void signalBattleRankGot();
    void signalBattleRankNameChanged(const QString& name);
    void signalBattleRankIconChanged(const QPixmap& pixmap);
    void signalBattleSeasonInfoChanged(const QString& text);
    void signalBattleNumsChanged(const QString& text);
    void signalBattleScoreChanged(const QString& text);
    void signalBattleStarted();
    void signalBattleFinished();

    void signalAppendNewLiveDanmaku(const LiveDanmaku& danmaku);
    void signalTriggerCmdEvent(const QString& cmd, const LiveDanmaku& danmaku, bool debug);
    void signalLocalNotify(const QString& text, qint64 uid);
    void signalShowError(const QString& title, const QString& info);
    void signalNewHour();
    void signalNewDay();
    void signalUpdatePermission();

public slots:
    /// 通过身份码连接房间
    virtual void startConnectIdentityCode(const QString &code) { Q_UNUSED(code) }

    /// 接收到 WS CMD 数据包
    virtual void slotBinaryMessageReceived(const QByteArray& message) { Q_UNUSED(message) }

    /// 接收到 PK 的 WS CMD 数据包
    virtual void slotPkBinaryMessageReceived(const QByteArray& message) { Q_UNUSED(message) }

    /// 设置为管理员
    virtual void appointAdmin(qint64 uid) { Q_UNUSED(uid) }
    /// 取消管理员
    virtual void dismissAdmin(qint64 uid) { Q_UNUSED(uid) }
    /// 禁言
    virtual void addBlockUser(qint64 uid, int hour, QString msg) { addBlockUser(uid, ac->roomId, hour, msg); }
    virtual void addBlockUser(qint64 uid, QString roomId, int hour, QString msg) { Q_UNUSED(uid) Q_UNUSED(hour) Q_UNUSED(roomId) Q_UNUSED(msg) }
    /// 取消禁言
    virtual void delBlockUser(qint64 uid) { delBlockUser(uid, ac->roomId); }
    virtual void delBlockUser(qint64 uid, QString roomId) { Q_UNUSED(uid) Q_UNUSED(roomId) }
    /// 禁言要通过禁言id来解除禁言，所以有两步操作
    virtual void delRoomBlockUser(qint64 id) { Q_UNUSED(id) }
    /// 刷新直播间禁言的用户
    virtual void refreshBlockList() {}
    /// 根据UL等级或者舰长自动调整字数
    virtual void adjustDanmakuLongest() {}

public:
    /// 获取直播间信息
    virtual void getRoomInfo(bool reconnect, int reconnectCount = 0) = 0;
    /// 获取直播间Host信息
    virtual void getDanmuInfo() = 0;
    /// 根据获得的Host信息，开始连接socket
    virtual void startMsgLoop() = 0;
    /// 初次连接socket，发送认证包
    virtual void sendVeriPacket(QWebSocket* socket, QString roomId, QString token) { Q_UNUSED(socket) Q_UNUSED(roomId) Q_UNUSED(token) }
    /// 连接后定时发送心跳包
    virtual void sendHeartPacket() {}

    /// 是否在直播中
    virtual bool isLiving() const;
    /// 是否在直播，或者可能要开播
    virtual bool isLivingOrMayLiving();
    virtual void getRoomCover(const QString& url) { Q_UNUSED(url) }
    virtual void getUpInfo(const QString &uid) { Q_UNUSED(uid) }
    /// 更新当前舰长
    virtual void updateExistGuards(int page = 0) override { Q_UNUSED(page) }
    /// 获取舰长数量
    virtual void getGuardCount(const LiveDanmaku &danmaku) { Q_UNUSED(danmaku) }
    /// 更新高能榜
    virtual void updateOnlineGoldRank() {}
    /// 获取礼物ID
    virtual void getGiftList() {}
    /// 获取表情包ID
    virtual void getEmoticonList() {}
    /// 每日签到
    virtual void doSign() {}
    /// 天选之人抽奖
    virtual void joinLOT(qint64 id, bool arg) { Q_UNUSED(id) Q_UNUSED(arg) }
    /// 节奏风暴
    virtual void joinStorm(qint64 id) { Q_UNUSED(id) }
    
    /// 模拟在线观看效果（带心跳的那种）
    virtual void startHeartConnection() {}
    virtual void stopHeartConnection() {}

    /// 获取大乱斗段位
    virtual void getRoomBattleInfo() {}
    /// 更新大乱斗连胜
    virtual void updateWinningStreak(bool emitWinningStreak) { Q_UNUSED(emitWinningStreak) }
    /// 获取这一把大乱斗的信息
    virtual void getPkInfoById(const QString& roomId, const QString& pkId) { Q_UNUSED(roomId) Q_UNUSED(pkId) }
    /// 单独socket连接对面直播间
    virtual void connectPkRoom() {}
    /// 根据直播间弹幕，保存当前观众信息
    virtual void getRoomCurrentAudiences(QString roomId, QSet<qint64> &audiences){ Q_UNUSED(roomId) Q_UNUSED(audiences) }
    virtual void connectPkSocket() { }

    /// 获取机器人账号信息
    virtual void getCookieAccount() = 0;
    QVariant getCookies() const;

    virtual void processNewDayData() {}
    virtual void triggerCmdEvent(const QString& cmd, const LiveDanmaku& danmaku, bool debug = false);
    virtual void localNotify(const QString& text, qint64 uid = 0);
    virtual void showError(const QString& title, const QString& desc = "");

public:
    /// 根据Url设置对应的Cookie
    virtual void setUrlCookie(const QString& url, QNetworkRequest* request) override;
    /// 设置全局默认的Cookie变量
    virtual void autoSetCookie(const QString &s);
    

protected:
    // 连接信息
    int hostUseIndex = 0;
    QList<HostInfo> hostList;
    QWebSocket* liveSocket;
    QTimer* heartTimer;
    QTimer* connectServerTimer;
    int reconnectWSDuration = INTERVAL_RECONNECT_WS; // WS重连间隔，每次上播/下播重置
    qint64 liveTimestamp = 0;

    // 状态变量
    bool gettingRoom = false;
    bool gettingUser = false;
    bool gettingUp = false;

    // 房间信息
    QList<LiveDanmaku> roomDanmakus;
    QPixmap roomCover; // 直播间封面原图
    QPixmap upFace; // 主播头像原图

    // 粉丝数量
    QList<FanBean> fansList; // 最近的关注，按时间排序

    // 船员
    bool updateGuarding = false;
    QList<LiveDanmaku> guardInfos;

    // 高能榜
    int online = 0;
    QList<LiveDanmaku> onlineGoldRank;
    QList<LiveDanmaku> onlineGuards;
    QList<qint64> onlineRankGuiIds;

    // 我的直播
    QString myLiveRtmp; // rtmp地址
    QString myLiveCode; // 直播码

    // 本次直播的礼物列表
    QList<LiveDanmaku> liveAllGifts;
    QList<LiveDanmaku> liveAllGuards;

    // 礼物连击
    QHash<QString, LiveDanmaku> giftCombos;
    QTimer* comboTimer = nullptr;

    // PK
    bool pking = false;
    int pkBattleType = 0;
    qint64 pkId = 0;
    qint64 pkToLive = 0; // PK导致的下播（视频必定触发）
    int myVotes = 0;
    int matchVotes = 0;
    qint64 pkEndTime = 0;
    QTimer* pkTimer = nullptr;
    int pkJudgeEarly = 2000;
    bool pkVideo = false;
    QList<LiveDanmaku> pkGifts;

    // PK偷塔
    QTimer* pkEndingTimer = nullptr;
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

    // PK串门
    bool pkChuanmenEnable = false;
    int pkMsgSync = 0;
    QString pkRoomId;
    QString pkUid;
    QString pkUname;
    QSet<qint64> myAudience; // 自己这边的观众
    QSet<qint64> oppositeAudience; // 对面的观众
    QWebSocket* pkLiveSocket = nullptr; // 连接对面的房间
    QString pkToken;
    QHash<qint64, qint64> cmAudience; // 自己这边跑过去串门了: timestamp10:串门，0已经回来/提示

    // 私信
    QTimer* privateMsgTimer = nullptr;
    qint64 privateMsgTimestamp = 0;

    // 活动信息
    int currentSeasonId = 0; // 大乱斗赛季，获取连胜需要赛季
    QString pkRuleUrl; // 大乱斗赛季信息

    // 内存管理
    QObjectCleanupHandler cleanupHandler;
};

#endif // LIVEROOMSERVICE_H
