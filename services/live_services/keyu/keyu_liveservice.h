#ifndef KEYULIVESERVICE_H
#define KEYULIVESERVICE_H

#include "liveservicebase.h"

class KeyuLiveService : public LiveServiceBase
{
    Q_OBJECT
public:
    KeyuLiveService(QObject* parent = nullptr);

protected:
    void startConnect() override;

private slots:
    void processMessage(const MyJson &json);

private:
    void initWS();
};

#endif // KEYULIVESERVICE_H
