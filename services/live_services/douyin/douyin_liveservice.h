#ifndef DOUYIN_LIVESERVICE_H
#define DOUYIN_LIVESERVICE_H

#include "liveservicebase.h"

class DouyinLiveService : public LiveServiceBase
{
public:
    DouyinLiveService(QObject* parent = nullptr);

    virtual void getRoomInfo(bool reconnect, int reconnectCount = 0) override;

private:
    void initWS();

protected:
    void startConnect() override;
    virtual void setUrlCookie(const QString & url, QNetworkRequest * request) override;
    virtual void autoAddCookie(QList<QNetworkCookie> cookies) override;
};

#endif // DOUYIN_LIVESERVICE_H
