#ifndef LIVEROOMSERVICE_H
#define LIVEROOMSERVICE_H

#include <QObject>
#include <QPixmap>
#include <QWebSocket>
#include <QTimer>
#include "livedanmaku.h"
#include "entities.h"

#define LOCAL_MODE 0
#define INTERVAL_RECONNECT_WS 5000
#define INTERVAL_RECONNECT_WS_MAX 60000

class LiveRoomService : public QObject
{
    Q_OBJECT
    friend class MainWindow;
public:
    explicit LiveRoomService(QObject *parent = nullptr);

    virtual void initObjects();
    virtual void readConfig();

signals:

public slots:


protected:
    // 房间信息
    QPixmap roomCover; // 直播间封面原图
    QPixmap upFace; // 主播头像原图

    // 我的直播
    QString myLiveRtmp; // rtmp地址
    QString myLiveCode; // 直播码

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

    // 粉丝数量
    QList<FanBean> fansList; // 最近的关注，按时间排序

    // 礼物连击
    QHash<QString, LiveDanmaku> giftCombos;
    QTimer* comboTimer = nullptr;

    // 连接信息
    int hostUseIndex = 0;
    QList<HostInfo> hostList;
    QWebSocket* socket;
    QTimer* heartTimer;
    QTimer* connectServerTimer;
    bool remoteControl = true;
    int reconnectWSDuration = INTERVAL_RECONNECT_WS; // WS重连间隔，每次上播/下播重置

    bool gettingRoom = false;
    bool gettingUser = false;
    bool gettingUp = false;
    QString SERVER_DOMAIN = LOCAL_MODE ? "http://localhost:8102" : "http://iwxyi.com:8102";
    QString serverPath = SERVER_DOMAIN + "/server/";

    // 船员
    bool updateGuarding = false;
    QList<LiveDanmaku> guardInfos;

    // 高能榜
    QList<LiveDanmaku> onlineGoldRank;
    QList<LiveDanmaku> onlineGuards;
    QList<qint64> onlineRankGuiIds;

    // 直播间人气
    QTimer* minuteTimer;
    int popularVal = 2;
    qint64 sumPopul = 0;     // 自启动以来的人气
    qint64 countPopul = 0;   // 自启动以来的人气总和

    // 弹幕人气
    int minuteDanmuPopul = 0;
    QList<int> danmuPopulQueue;
    int danmuPopulValue = 0;

    // 本次直播的礼物列表
    QList<LiveDanmaku> liveAllGifts;
    QList<LiveDanmaku> liveAllGuards;

    // 大乱斗
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

    // 大乱斗偷塔
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
};

#endif // LIVEROOMSERVICE_H
