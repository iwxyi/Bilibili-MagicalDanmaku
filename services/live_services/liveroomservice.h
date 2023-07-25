#ifndef LIVEROOMSERVICE_H
#define LIVEROOMSERVICE_H

#include <QWebSocket>
#include <QRegularExpression>
#include <QObjectCleanupHandler>
#include <QDesktopServices>
#include <QAction>
#include "widgets/netinterface.h"
#include "livestatisticservice.h"
#include "entities.h"
#include "api_type.h"
#include "sqlservice.h"

#define INTERVAL_RECONNECT_WS 5000
#define INTERVAL_RECONNECT_WS_MAX 60000
#define SOCKET_DEB if (0) qDebug() // 输出调试信息
#define SOCKET_INF if (0) qDebug() // 输出数据包信息

#define TO_SIMPLE_NUMBER(x) (x > 100000 ? QString(snum(x/10000)+"万") : snum(x))

typedef std::function<void(LiveDanmaku)> DanmakuFunc;
typedef std::function<void(QString)> StringFunc;

#define WAIT_INIT "__WAIT_INIT__"

class QLabel;

class LiveRoomService : public QObject, public NetInterface, public LiveStatisticService
{
    Q_OBJECT
    friend class MainWindow;
    friend class CodeRunner;
    
public:
    explicit LiveRoomService(QObject *parent = nullptr);

    /// 初始化所有变量，new、connect等
    virtual void init();
    void initTimeTasks();

    /// 读取设置
    virtual void readConfig();

    virtual void releaseLiveData(bool prepare);

    QList<LiveDanmaku> getDanmusByUID(qint64 uid);

    void setSqlService(SqlService* service);

signals:
    void signalStartConnectRoom(); // 通知总的开始连接
    void signalConnectionStarted(); // WebSocket开始连接
    void signalConnectionStateChanged(QAbstractSocket::SocketState state);
    void signalConnectionStateTextChanged(const QString& text);
    void signalStartWork(); // 直播开始/连接上就已经开始直播
    void signalLiveStarted();
    void signalLiveStopped();
    void signalStatusChanged(const QString& text); // 状态或提示改变，修改状态栏
    void signalLiveStatusChanged(const QString& text); // 直播状态：已开播/未开播
    void signalNewDanmaku(const LiveDanmaku& danmaku); // 通知外部各种界面
    void signalReceiveDanmakuTotalCountChanged(int count); // 本场直播弹幕总数
    void signalTryBlockDanmaku(const LiveDanmaku& danmaku); // 判断是否需要屏蔽弹幕
    void signalNewGiftReceived(const LiveDanmaku& danmaku); // 有人送礼物
    void signalSendUserWelcome(const LiveDanmaku& danmaku); // 发送欢迎的代码
    void signalSendAttentionThank(const LiveDanmaku& danmaku); // 发送大些关注
    void signalNewGuardBuy(const LiveDanmaku& danmaku); // 有人上舰长

    void signalRobotAccountChanged();
    void signalRoomIdChanged(const QString &roomId); // 房间号改变，例如通过解析身份码导致的房间ID变更
    void signalUpUidChanged(const QString &uid);
    void signalUpFaceChanged(const QPixmap& pixmap);
    void signalUpInfoChanged();
    void signalUpSignatureChanged(const QString& signature);
    void signalRoomInfoChanged();
    void signalImUpChanged(bool isUp);
    void signalRoomCoverChanged(const QPixmap &pixmap);
    void signalRoomTitleChanged(const QString& title);
    void signalRoomDescriptionChanged(const QString& desc);
    void signalRoomTagsChanged(const QStringList& tags);
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
    void signalPopularTextChanged(const QString& text);
    void signalFansCountChanged(qint64 count); // 粉丝数
    void signalSignInfoChanged(const QString& text); // 自动签到
    void signalSignDescChanged(const QString& text);
    void signalLOTInfoChanged(const QString& text); // 天选等活动
    void signalLOTDescChanged(const QString& text);
    void signalDanmakuLongestChanged(int length); // 弹幕长度
    void signalRefreshPrivateMsgEnabled(bool enable); // 私信功能开关
    void signalSendAutoMsg(const QString& msg, const LiveDanmaku& danmaku);
    void signalSendAutoMsgInFirst(const QString& msg, const LiveDanmaku& danmaku, int interval);
    void signalRoomRankChanged(const QString& desc, const QString& color);
    void signalHotRankChanged(int rank, const QString& area, const QString& msg);

    void signalBattleEnabled(bool enable);
    void signalBattleRankGot();
    void signalBattleRankNameChanged(const QString& name);
    void signalBattleRankIconChanged(const QPixmap& pixmap);
    void signalBattleSeasonInfoChanged(const QString& text);
    void signalBattleNumsChanged(const QString& text);
    void signalBattleScoreChanged(const QString& text);
    void signalBattlePrepared();
    void signalBattleStarted();
    void signalBattleFinished(); // PK结束，关闭一些控件，可能会重复调用
    void signalBattleStartMatch();

    void signalDanmakuStatusChanged(const QString& text);
    void signalPKStatusChanged(int pkType, qint64 roomId, qint64 upUid, const QString& upUname);
    void signalDanmakuAddBlockText(const QString& word, int second);
    void signalMergeGiftCombo(const LiveDanmaku& danmaku, int delay); // 弹幕姬的礼物合并
    void signalAutoMelonMsgChanged(const QString& msg);

    void signalActionShowPKVideoChanged(bool enable);
    void signalActionJoinBattleChanged(bool enable);

    void signalPositiveVoteStateChanged(bool like);
    void signalPositiveVoteCountChanged(const QString& text);

    void signalTriggerCmdEvent(const QString& cmd, const LiveDanmaku& danmaku, bool debug);
    void signalLocalNotify(const QString& text, qint64 uid);
    void signalShowError(const QString& title, const QString& info);
    void signalNewHour();
    void signalNewDay();
    void signalUpdatePermission();

public slots:
    /// 通过身份码连接房间
    virtual void startConnectIdentityCode(const QString &code) {}

    /// 接收到 WS CMD 数据包
    virtual void slotBinaryMessageReceived(const QByteArray& message) {}
    /// 接收到 PK 的 WS CMD 数据包
    virtual void slotPkBinaryMessageReceived(const QByteArray& message) {}

    /// 发送弹幕
    virtual void sendMsg(const QString& msg) {}
    virtual void sendRoomMsg(QString roomId, const QString& msg) {}
    /// 恢复之前的弹幕
    virtual void pullLiveDanmaku() { }
    /// 设置为管理员
    virtual void appointAdmin(qint64 uid) {}
    /// 取消管理员
    virtual void dismissAdmin(qint64 uid) {}
    /// 禁言
    virtual void addBlockUser(qint64 uid, int hour, QString msg) { addBlockUser(uid, ac->roomId, hour, msg); }
    virtual void addBlockUser(qint64 uid, QString roomId, int hour, QString msg) {}
    /// 取消禁言
    virtual void delBlockUser(qint64 uid) { delBlockUser(uid, ac->roomId); }
    virtual void delBlockUser(qint64 uid, QString roomId) {}
    /// 禁言要通过禁言id来解除禁言，所以有两步操作
    virtual void delRoomBlockUser(qint64 id) {}
    /// 刷新直播间禁言的用户
    virtual void refreshBlockList() {}
    /// 根据UL等级或者舰长自动调整字数
    virtual void adjustDanmakuLongest() {}
    /// 私信
    virtual void refreshPrivateMsg() {}
    virtual void receivedPrivateMsg(MyJson session) {}
    /// 大乱斗
    virtual void slotPkEndingTimeout() {}
    virtual void slotPkEnding() {}

public:
    /// 获取机器人账号信息
    virtual void getCookieAccount() = 0;
    QVariant getCookies() const;
    /// 获取机器人账号信息
    virtual void getRobotInfo() = 0;
    /// 获取直播间信息
    virtual void getRoomInfo(bool reconnect, int reconnectCount = 0) = 0;
    /// 获取直播间Host信息
    virtual void getDanmuInfo() = 0;
    /// 根据获得的Host信息，开始连接socket
    virtual void startMsgLoop() = 0;
    /// 初次连接socket，发送认证包
    virtual void sendVeriPacket(QWebSocket* socket, QString roomId, QString token) {}
    /// 连接后定时发送心跳包
    virtual void sendHeartPacket(QWebSocket* socket) {}
    /// 获取直播间与用户的信息
    virtual void getRoomUserInfo() {}

    /// 是否在直播中
    virtual bool isLiving() const;
    /// 是否在直播，或者可能要开播
    virtual bool isLivingOrMayLiving();
    /// 获取并更新直播间封面
    virtual void getRoomCover(const QString& url) {}
    /// 获取主播信息
    virtual void getUpInfo(const QString &uid) {}
    /// 更新当前舰长
    virtual void updateExistGuards(int page = 0) override {}
    /// 获取舰长数量
    virtual void getGuardCount(const LiveDanmaku &danmaku) {}
    /// 更新高能榜
    virtual void updateOnlineGoldRank() {}
    virtual void getPkOnlineGuardPage(int page) {}
    /// 获取礼物ID
    virtual void getGiftList() {}
    /// 获取表情包ID
    virtual void getEmoticonList() {}
    /// 发送礼物
    virtual void sendGift(int giftId, int giftNum) {}
    /// 发送背包中的礼物
    virtual void sendBagGift(int giftId, int giftNum, qint64 bagId) {}
    /// 每日签到
    virtual void doSign() {}
    /// 天选之人抽奖
    virtual void joinLOT(qint64 id, bool arg) {}
    /// 节奏风暴
    virtual void joinStorm(qint64 id) {}
    /// 打开用户首页
    virtual void openUserSpacePage(QString uid) {}
    /// 打开直播间首页
    virtual void openLiveRoomPage(QString roomId) {}
    /// 打开分区排行
    virtual void openAreaRankPage(QString areaId, QString parentAreaId) {}
    /// 切换勋章至指定的主播ID
    virtual void switchMedalToUp(QString upId, int page = 1) {}
    virtual void wearMedal(qint64 medalId) {}
    /// 发送私信
    virtual void sendPrivateMsg(QString uid, QString msg) {}
    /// 开启大乱斗
    virtual void joinBattle(int type) {}
    /// 送礼物后检测勋章升级
    virtual void detectMedalUpgrade(LiveDanmaku danmaku) {}
    /// 触发进入直播间的动作
    virtual void roomEntryAction() {}
    /// 背包礼物
    virtual void sendExpireGift();
    virtual void getBagList(qint64 sendExpire) {}
    bool mergeGiftCombo(const LiveDanmaku &danmaku);

    /// 修改直播间信息
    virtual void myLiveSelectArea(bool update) {}
    virtual void myLiveUpdateArea(QString area) {}
    virtual void myLiveStartLive() {}
    virtual void myLiveStopLive() {}
    virtual void myLiveSetTitle(QString newTitle = "") {}
    virtual void myLiveSetNews() {}
    virtual void myLiveSetDescription() {}
    virtual void myLiveSetCover(QString path = "") {}
    virtual void myLiveSetTags() {}

    /// 获取PK相关信息
    virtual void showPkMenu() {}
    virtual void showPkAssists() {}
    virtual void showPkHistories() {}
    
    /// 模拟在线观看效果（带心跳的那种）
    virtual void startHeartConnection() {}
    virtual void stopHeartConnection() {}

    /// 获取大乱斗段位
    virtual void getRoomBattleInfo() {}
    /// 更新大乱斗连胜
    virtual void updateWinningStreak(bool emitWinningStreak) {}
    /// 获取这一把大乱斗的信息
    virtual void getPkInfoById(const QString& roomId, const QString& pkId) {}
    /// 单独socket连接对面直播间
    virtual void connectPkRoom() {}
    /// 根据直播间弹幕，保存当前观众信息
    virtual void getRoomCurrentAudiences(QString roomId, QSet<qint64> &audiences){}
    virtual void connectPkSocket() { }
    /// 获取PK对面直播间的信息
    virtual void getPkMatchInfo() {}

    /// PK
    virtual void pkPre(QJsonObject json) {}
    virtual void pkStart(QJsonObject json) {}
    virtual void pkProcess(QJsonObject json) {}
    virtual void pkEnd(QJsonObject json) {}
    virtual void pkSettle(QJsonObject json) {}
    virtual int getPkMaxGold(int votes) { return 0; }
    virtual bool execTouta() {}
    virtual void saveTouta() {}

    /// 统计
    void appendLiveGift(const LiveDanmaku& danmaku);
    void appendLiveGuard(const LiveDanmaku& danmaku);
    void userComeEvent(LiveDanmaku &danmaku);

    /// 录播
    virtual void getRoomLiveVideoUrl(StringFunc func) {}

    /// 事件处理
    virtual void processNewDayData() {}
    virtual void triggerCmdEvent(const QString& cmd, const LiveDanmaku& danmaku, bool debug = false);
    virtual void localNotify(const QString& text, qint64 uid = 0);
    virtual void showError(const QString& title, const QString& desc = "");
    
    /// 弹幕新增
    void appendNewLiveDanmakus(const QList<LiveDanmaku> &danmakus);
    void appendNewLiveDanmaku(const LiveDanmaku &danmaku);
    /// 根据Url设置对应的Cookie
    virtual void setUrlCookie(const QString& url, QNetworkRequest* request) override {}
    /// 设置全局默认的Cookie变量
    virtual void autoSetCookie(const QString &s);
    
    /// 分割长弹幕
    QStringList splitLongDanmu(const QString& text, int maxOne) const;
    void sendLongText(QString text);

    /// 一些接口的网址
    virtual QString getApiUrl(ApiType type, qint64 id) { return ""; }
    virtual QStringList getRoomShieldKeywordsAsync(bool* ok) { if (ok) *ok = false; return QStringList(); }
    virtual void addRoomShieldKeywordsAsync(const QString& word) {}
    virtual void removeRoomShieldKeywordAsync(const QString& word) {}
    
    /// 获取评价
    virtual void updatePositiveVote() {}
    /// 切换评价，可以是好评也可以是取消好评
    virtual void setPositiveVote() {}
    virtual int getPositiveVoteCount() const { return 0; }
    virtual bool isPositiveVote() const { return false; }

    /// 动作项
    virtual void showFollowCountInAction(qint64 uid, QLabel* statusLabel, QAction* action, QAction* action2 = nullptr) const {}
    virtual void showViewCountInAction(qint64 uid, QLabel* statusLabel, QAction* action, QAction* action2 = nullptr, QAction* action3 = nullptr) const {}
    virtual void showGuardInAction(qint64 roomId, qint64 uid, QLabel* statusLabel, QAction* action) const {}
    virtual void showPkLevelInAction(qint64 roomId, QLabel* statusLabel, QAction* actionUser, QAction* actionRank) const {}

    /// 机器人判断
    virtual void judgeUserRobotByFans(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs) {}
    virtual void judgeUserRobotByUpstate(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs) {}
    virtual void judgeUserRobotByUpload(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs) {}

    /// 一些条件判断
    bool isInFans(qint64 uid) const;
    void markNotRobot(qint64 uid);

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
    QMap<ApiType, QString> apiUrls;
    QString roomRankDesc;

    // 我的信息
    QPixmap robotFace; // 自己的原图

    // 粉丝数量
    QList<FanBean> fansList; // 最近的关注，按时间排序

    // 粉丝牌
    QList<qint64> medalUpgradeWaiting; // 正在计算的升级

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

    // PK结尾
    QTimer* pkEndingTimer = nullptr;
    int goldTransPk = 100; // 金瓜子转乱斗值的比例，除以10还是100
    int pkMaxGold = 300; // 单位是金瓜子，积分要/10
    bool pkEnding = false;
    int pkVoting = 0;
    bool toutaGift = false;
    bool pkAutoMelon = false;
    bool pkAutoMaxGold = false;
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

    // 数据库
    SqlService* sqlService = nullptr;

    // 内存管理
    QObjectCleanupHandler cleanupHandler;

    // flag
    bool _loadingOldDanmakus = false;
    LiveDanmaku lastDanmaku; // 最近一个弹幕
    bool _guardJudged = false; // 每次开播/启动判断舰长的信号

    // 机器人判断
    MySettings* robotRecord = nullptr;

    // 辅助机器人
    QList<QWebSocket*> robots_sockets;
};

#endif // LIVEROOMSERVICE_H
