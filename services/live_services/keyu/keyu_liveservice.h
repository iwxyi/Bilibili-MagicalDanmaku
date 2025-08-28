#ifndef KEYULIVESERVICE_H
#define KEYULIVESERVICE_H

#include "anywebsocketservice.h"

class KeyuLiveService : public AnyWebSocketService
{
    Q_OBJECT
public:
    KeyuLiveService(QObject* parent = nullptr);

private slots:
    void processMessage(const QString &message) override;
};

#endif // KEYULIVESERVICE_H
