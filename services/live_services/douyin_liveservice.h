#ifndef DOUYIN_LIVESERVICE_H
#define DOUYIN_LIVESERVICE_H

#include "liveservicebase.h"

class DouyinLiveService : public LiveServiceBase
{
public:
    DouyinLiveService(QObject* parent = nullptr);

    virtual void getRoomInfo(bool reconnect, int reconnectCount = 0) override;
};

#endif // DOUYIN_LIVESERVICE_H
