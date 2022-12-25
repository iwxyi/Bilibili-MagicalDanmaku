#ifndef BILILIVESERVICE_H
#define BILILIVESERVICE_H

#include "liveroomservice.h"

class BiliLiveService : public LiveRoomService
{
    Q_OBJECT
public:
    BiliLiveService(QObject* parent = nullptr);
    
signals:
    
    
public slots:
    void startConnectRoom(const QString& roomId) override;
    void getRoomInfo();
    void updateExistGuards(int page = 0) override;
    void getCookieAccount() override;
    void getGiftList() override;
    void getEmoticonList() override;
    
private:
    
};

#endif // BILILIVESERVICE_H
