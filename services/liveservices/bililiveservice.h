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

    /// 直播间连接
    void getRoomInfo(bool reconnect, int reconnectCount = 0) override;
    void getDanmuInfo() override;
    void startMsgLoop() override;
    void sendVeriPacket(QWebSocket *liveSocket, QString roomId, QString token) override;
    void sendHeartPacket() override;

    /// 直播间接口
    void getRoomCover(const QString &url) override;
    void getUpInfo(const QString &uid) override;
    void updateExistGuards(int page = 0) override;
    void getGuardCount(const LiveDanmaku &danmaku) override;
    void updateOnlineGoldRank() override;
    void getCookieAccount() override;
    void getGiftList() override;
    void getEmoticonList() override;
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
    void myLiveSelectArea(bool update) override;
    void myLiveUpdateArea(QString area) override;
    void myLiveStartLive() override;
    void myLiveStopLive() override;
    void myLiveSetTitle(QString newTitle = "") override;
    void myLiveSetNews() override;
    void myLiveSetDescription() override;
    void myLiveSetCover(QString path = "") override;
    void myLiveSetTags() override;

    /// 大乱斗
    void getRoomBattleInfo() override;
    void updateWinningStreak(bool emitWinningStreak) override;
    void getPkInfoById(const QString &roomId, const QString &pkId) override;
    void connectPkRoom() override;
    void getRoomCurrentAudiences(QString roomId, QSet<qint64> &audiences) override;
    void connectPkSocket() override;

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

public slots:
    void slotBinaryMessageReceived(const QByteArray &message) override;

    void slotPkBinaryMessageReceived(const QByteArray &message) override;

    /// 用户管理
    void appointAdmin(qint64 uid) override;
    void dismissAdmin(qint64 uid) override;
    void addBlockUser(qint64 uid, QString roomId, int hour, QString msg) override;
    void delBlockUser(qint64 uid, QString roomId) override;
    void delRoomBlockUser(qint64 id) override;
    void refreshBlockList() override;
    void adjustDanmakuLongest() override;
    
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
};

#endif // BILILIVESERVICE_H
