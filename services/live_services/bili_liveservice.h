#ifndef BILILIVESERVICE_H
#define BILILIVESERVICE_H

#include "liveroomservice.h"

class BiliLiveService : public LiveRoomService
{
    Q_OBJECT
    friend class MainWindow;
public:
    BiliLiveService(QObject* parent = nullptr);
    
signals:
    

public:
    /// 数据操作
    void readConfig() override;
    void releaseLiveData(bool prepare) override;
    void initWS();

    /// 直播间连接
    void getCookieAccount() override;
    void getNavInfo(NetVoidFunc func = nullptr);
    QString toWbiParam(QString params) const;
    void getRobotInfo() override;
    void getRoomInfo(bool reconnect, int reconnectCount = 0) override;
    void getDanmuInfo() override;
    void startMsgLoop() override;
    void sendVeriPacket(QWebSocket *liveSocket, QString roomId, QString token) override;
    void sendHeartPacket(QWebSocket *socket) override;
    void getRoomUserInfo() override;

    /// 直播间接口
    void getRoomCover(const QString &url) override;
    void getUpInfo(const QString &uid) override;
    void updateExistGuards(int page = 0) override;
    void getGuardCount(const LiveDanmaku &danmaku) override;
    void updateOnlineGoldRank() override;
    void getPkOnlineGuardPage(int page) override;
    void getGiftList() override;
    void getEmoticonList() override;
    void sendGift(int giftId, int giftNum) override;
    void sendBagGift(int giftId, int giftNum, qint64 bagId) override;
    void doSign() override;
    void joinLOT(qint64 id, bool follow) override;
    void joinStorm(qint64 id) override;
    void openUserSpacePage(QString uid) override;
    void openLiveRoomPage(QString roomId) override;
    void openAreaRankPage(QString areaId, QString parentAreaId) override;
    void switchMedalToUp(QString upId, int page = 1) override;
    void wearMedal(qint64 medalId) override;
    void sendPrivateMsg(QString  uid, QString msg) override;
    void joinBattle(int type) override;
    void detectMedalUpgrade(LiveDanmaku danmaku) override;
    void roomEntryAction() override;
    void getBagList(qint64 sendExpire) override;

    void myLiveSelectArea(bool update) override;
    void myLiveUpdateArea(QString area) override;
    void myLiveStartLive() override;
    void myLiveStopLive() override;
    void myLiveSetTitle(QString newTitle = "") override;
    void myLiveSetNews() override;
    void myLiveSetDescription() override;
    void myLiveSetCover(QString path = "") override;
    void myLiveSetTags() override;

    void showPkMenu() override;
    void showPkAssists() override;
    void showPkHistories() override;

    /// 大乱斗
    void getRoomBattleInfo() override;
    void updateWinningStreak(bool emitWinningStreak) override;
    void getPkInfoById(const QString &roomId, const QString &pkId) override;
    void connectPkRoom() override;
    void getRoomCurrentAudiences(QString roomId, QSet<qint64> &audiences) override;
    void connectPkSocket() override;
    void getPkMatchInfo() override;
    void getPkOnlineGoldPage(int page = 0) override;

    /// PK
    virtual void pkPre(QJsonObject json) override;
    virtual void pkStart(QJsonObject json) override;
    virtual void pkProcess(QJsonObject json) override;
    virtual void pkEnd(QJsonObject json) override;
    virtual void pkSettle(QJsonObject json) override;
    virtual int getPkMaxGold(int votes) override;
    virtual bool execTouta() override;
    virtual void saveTouta() override;

    /// 录播
    void getRoomLiveVideoUrl(StringFunc func) override;

    /// 长链心跳
    void startHeartConnection() override;
    void stopHeartConnection() override;
    void sendXliveHeartBeatE();
    void sendXliveHeartBeatX();
    void sendXliveHeartBeatX(QString s, qint64 timestamp);

    /// 一些事件
    void processNewDayData() override;

    /// 解包
    void uncompressPkBytes(const QByteArray &body);
    void handlePkMessage(QJsonObject json);

    /// 一些接口
    QString getApiUrl(ApiType type, qint64 id) override;
    QStringList getRoomShieldKeywordsAsync(bool *ok) override;
    void addRoomShieldKeywordsAsync(const QString& word) override;
    void removeRoomShieldKeywordAsync(const QString &word) override;

    /// 饭贩
    void updatePositiveVote() override;
    void setPositiveVote() override;
    void fanfanLogin(bool autoVote = true);
    void fanfanAddOwn();
    int getPositiveVoteCount() const override;
    bool isPositiveVote() const override;

    void showFollowCountInAction(qint64 uid, QLabel* statusLabel, QAction* action, QAction* action2 = nullptr) const override;
    void showViewCountInAction(qint64 uid, QLabel* statusLabel, QAction* action, QAction* action2 = nullptr, QAction* action3 = nullptr) const override;
    void showGuardInAction(qint64 roomId, qint64 uid, QLabel* statusLabel, QAction* action) const override;
    void showPkLevelInAction(qint64 roomId, QLabel* statusLabel, QAction* actionUser, QAction* actionRank) const override;

    void judgeUserRobotByFans(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs) override;
    void judgeUserRobotByUpstate(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs) override;
    void judgeUserRobotByUpload(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs) override;

public slots:
    void slotBinaryMessageReceived(const QByteArray &message) override;
    void slotUncompressBytes(const QByteArray &body);
    void splitUncompressedBody(const QByteArray &unc);
    bool handleUncompMessage(QString cmd, MyJson json);
    bool handlePK(QJsonObject json);
    void handleMessage(QJsonObject json);

    void slotPkBinaryMessageReceived(const QByteArray &message) override;

    /// 弹幕
    void sendMsg(const QString& msg) override;
    void sendRoomMsg(QString uid, const QString& msg) override;
    void pullLiveDanmaku() override;
    
    /// 用户管理
    void appointAdmin(qint64 uid) override;
    void dismissAdmin(qint64 uid) override;
    void addBlockUser(qint64 uid, QString roomId, int hour, QString msg) override;
    void delBlockUser(qint64 uid, QString roomId) override;
    void delRoomBlockUser(qint64 id) override;
    void refreshBlockList() override;
    void adjustDanmakuLongest() override;
    /// 私信
    void refreshPrivateMsg() override;
    void receivedPrivateMsg(MyJson session) override;
    /// 大乱斗
    void slotPkEndingTimeout() override;
    void slotPkEnding() override;

protected:
    virtual void setUrlCookie(const QString& url, QNetworkRequest* request) override;
    
private:
    // 直播心跳
    QTimer* xliveHeartBeatTimer = nullptr;
    int xliveHeartBeatIndex = 0;         // 发送心跳的索引（每次+1）
    qint64 xliveHeartBeatEts = 0;        // 上次心跳时间戳
    int xliveHeartBeatInterval = 60;     // 上次心时间跳间隔（实测都是60）
    QString xliveHeartBeatBenchmark;     // 上次心跳秘钥参数（实测每次都一样）
    QJsonArray xliveHeartBeatSecretRule; // 上次心跳加密间隔（实测每次都一样）
    QString encServer = "http://iwxyi.com:6001/enc";
    int todayHeartMinite = 0; // 今天已经领取的小心心数量（本程序）
    
    // wbi加密
    QByteArray wbiMixinKey;

    // 饭贩
    short fanfanLike = 0; // 是否好评，0未知，1好评，-1未好评
    int fanfanLikeCount = 0; // 饭贩好评数量
    bool fanfanOwn = false; // 是否已拥有
};

#endif // BILILIVESERVICE_H
