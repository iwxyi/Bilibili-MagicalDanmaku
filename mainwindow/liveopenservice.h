#ifndef LIVEOPENSERVICE_H
#define LIVEOPENSERVICE_H

#include <QObject>
#include <QTimer>
#include "netinterface.h"

#define LIVE_OPEN_DEB 0

class LiveOpenService : public QObject
{
    Q_OBJECT
public:
    explicit LiveOpenService(QObject *parent = nullptr);

    qint64 getAppId() const;
    bool isPlaying() const;

signals:
    void signalError(const QString& msg);

public slots:
    void start();
    void end();
    void sendHeart();

    void started(const QString& gameId);

protected:
    void post(QString url, MyJson json, NetJsonFunc func);
    bool isValid();

private:
    const QString API_DOMAIN = "https://live-open.biliapi.com";
    const qint64 APP_ID = 1658282676661;
    QString gameId;
    QTimer* heartTimer = nullptr;
};

#endif // LIVEOPENSERVICE_H
