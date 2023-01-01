#ifndef LIVEROOMSERVICE_H
#define LIVEROOMSERVICE_H

#include <QWebSocket>
#include <QRegularExpression>
#include "widgets/netinterface.h"
#include "livestatisticservice.h"
#include "entities.h"

#define INTERVAL_RECONNECT_WS 5000
#define INTERVAL_RECONNECT_WS_MAX 60000

class LiveRoomService : public QObject, public NetInterface, public LiveStatisticService
{
    Q_OBJECT
    friend class MainWindow;

public:
    explicit LiveRoomService(QObject *parent = nullptr);

    /// 初始化所有变量，new、connect等
    virtual void init();

    /// 读取设置
    virtual void readConfig();

signals:
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

    void signalBattleEnabled(bool enable);
    void signalBattleRankGot();
    void signalBattleRankNameChanged(const QString& name);
    void signalBattleRankIconChanged(const QPixmap& pixmap);
    void signalBattleSeasonInfoChanged(const QString& text);
    void signalBattleNumsChanged(const QString& text);
    void signalBattleScoreChanged(const QString& text);

    void signalTriggerCmdEvent(const QString& cmd, const LiveDanmaku& danmaku);

public slots:
    /// 开始获取房间，进行一些初始化操作
    virtual void startConnectRoom(const QString &roomId);
    /// 通过身份码连接房间
    virtual void startConnectIdentityCode(const QString &code) { Q_UNUSED(code) }

public:
    /// 获取直播间信息
    virtual void getRoomInfo(bool reconnect, int reconnectCount = 0) = 0;
    virtual bool isLiving() const;
    virtual bool isLivingOrMayLiving();
    virtual void getRoomCover(const QString& url) { Q_UNUSED(url) }
    virtual void getUpInfo(const QString &uid) { Q_UNUSED(uid) }
    /// 更新当前舰长
    virtual void updateExistGuards(int page = 0) override { Q_UNUSED(page) }
    /// 获取礼物ID
    virtual void getGiftList() {}
    /// 获取表情包ID
    virtual void getEmoticonList() {}

    /// 更新大乱斗连胜
    virtual void getRoomBattleInfo() {}
    virtual void updateWinningStreak(bool emitWinningStreak) {}

    /// 获取机器人账号信息
    virtual void getCookieAccount() = 0;
    QVariant getCookies() const;

public:
    /// 根据Url设置对应的Cookie
    virtual void setUrlCookie(const QString& url, QNetworkRequest* request) override;
    /// 设置全局默认的Cookie变量
    virtual void autoSetCookie(const QString &s);
    

protected:
    // 连接信息
    int hostUseIndex = 0;
    QList<HostInfo> hostList;
    QWebSocket* socket;
    QTimer* heartTimer;
    QTimer* connectServerTimer;
    int reconnectWSDuration = INTERVAL_RECONNECT_WS; // WS重连间隔，每次上播/下播重置

    // 状态变量
    bool gettingRoom = false;
    bool gettingUser = false;
    bool gettingUp = false;

    // 房间信息
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
    QWebSocket* pkSocket = nullptr; // 连接对面的房间
    QString pkToken;
    QHash<qint64, qint64> cmAudience; // 自己这边跑过去串门了: timestamp10:串门，0已经回来/提示

    // TODO: 下面这些变量在接口转移时，需要移动到BiliLiveService中
    // 直播心跳
    qint64 liveTimestamp = 0;
    QTimer* xliveHeartBeatTimer = nullptr;
    int xliveHeartBeatIndex = 0;         // 发送心跳的索引（每次+1）
    qint64 xliveHeartBeatEts = 0;        // 上次心跳时间戳
    int xliveHeartBeatInterval = 60;     // 上次心时间跳间隔（实测都是60）
    QString xliveHeartBeatBenchmark;     // 上次心跳秘钥参数（实测每次都一样）
    QJsonArray xliveHeartBeatSecretRule; // 上次心跳加密间隔（实测每次都一样）
    QString encServer = "http://iwxyi.com:6001/enc";
    int todayHeartMinite = 0; // 今天已经领取的小心心数量（本程序）

    // 私信
    QTimer* privateMsgTimer = nullptr;
    qint64 privateMsgTimestamp = 0;

    // 活动信息
    int currentSeasonId = 0; // 大乱斗赛季，获取连胜需要赛季
    QString pkRuleUrl; // 大乱斗赛季信息
};

#endif // LIVEROOMSERVICE_H
