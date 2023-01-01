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
    void readConfig() override;
    void releaseLiveData(bool prepare) override;

    void getRoomInfo(bool reconnect, int reconnectCount = 0) override;
    void getDanmuInfo() override;
    void getRoomCover(const QString &url) override;
    void getUpInfo(const QString &uid) override;
    void updateExistGuards(int page = 0) override;
    void updateOnlineGoldRank() override;
    void getCookieAccount() override;
    void getGiftList() override;
    void getEmoticonList() override;

    void getRoomBattleInfo() override;
    void updateWinningStreak(bool emitWinningStreak) override;

    void startHeartConnection() override;
    void stopHeartConnection() override;
    void sendXliveHeartBeatE();
    void sendXliveHeartBeatX();
    void sendXliveHeartBeatX(QString s, qint64 timestamp);
    
private:
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
};

#endif // BILILIVESERVICE_H
