#ifndef LIVEOPENSERVICE_H
#define LIVEOPENSERVICE_H

#include <QObject>
#include <QTimer>
#include <QWebSocket>
#include "netinterface.h"
#include "bili_api_util.h"

#define BILI_API_DOMAIN QString("https://live-open.biliapi.com")
#define BILI_APP_ID 1658282676661

#define LIVE_OPEN_DEB if (1) qDebug()
#define LIVE_OPEN_SOCKET_DEB if (0) qDebug()

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

    void connectWS(const QString& url, const QByteArray &authBody);
    void sendWSHeart();

public:
    void post(QString url, MyJson json, NetJsonFunc func);
    bool isValid();

private:
    QString gameId;
    QTimer* heartTimer = nullptr;
    QWebSocket* websocket = nullptr;
};

#endif // LIVEOPENSERVICE_H
