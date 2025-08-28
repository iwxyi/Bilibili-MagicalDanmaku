#ifndef ANYWEBSOCKETSERVICE_H
#define ANYWEBSOCKETSERVICE_H

#include "liveservicebase.h"

class AnyWebSocketService : public LiveServiceBase
{
public:
    AnyWebSocketService(QObject* parent = nullptr);

protected:
    virtual void autoSetProxy();
    virtual void initWS();
    virtual void startConnect() override;

protected slots:
    virtual void processMessage(const QString &message);
};

#endif // ANYWEBSOCKETSERVICE_H
