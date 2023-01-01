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
    
    
public slots:
    void startConnectRoom(const QString& roomId) override;

public:
    void getRoomInfo(bool reconnect, int reconnectCount = 0) override;
    void getRoomCover(const QString &url) override;
    void getUpInfo(const QString &uid) override;
    void updateExistGuards(int page = 0) override;
    void getCookieAccount() override;
    void getGiftList() override;
    void getEmoticonList() override;

    void getRoomBattleInfo() override;
    void updateWinningStreak(bool emitWinningStreak) override;
    
private:
};

#endif // BILILIVESERVICE_H
