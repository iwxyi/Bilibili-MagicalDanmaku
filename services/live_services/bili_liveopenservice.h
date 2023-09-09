#ifndef LIVEOPENSERVICE_H
#define LIVEOPENSERVICE_H

#include <QObject>
#include <QTimer>
#include <QWebSocket>
#include "netinterface.h"
#include "bili_api_util.h"
#include "bili_liveservice.h"

#define LIVE_OPEN_DEB if (0) qDebug()
#define LIVE_OPEN_SOCKET_DEB if (0) qDebug()

/**
 * 直播服务类
 * 与 BiliLiveService 的区别是，这是官方的服务，需要使用身份码才能连接
 */
class BiliLiveOpenService : public BiliLiveService
{
    Q_OBJECT
public:
    explicit BiliLiveOpenService(QObject *parent = nullptr);

    qint64 getAppId() const;
    bool isPlaying() const;

    void startConnectIdentityCode(const QString &code) override;
    void startConnect() override;
    void sendVeriPacket(QWebSocket *liveSocket, QString roomId, QString token) override;
    void sendHeartPacket(QWebSocket *socket) override;
    void getDanmuInfo() override;

signals:
    void signalStart(bool sucess);
    void signalEnd(bool success);
    void signalError(const QString& msg);

public slots:
    void apiStart();
    void apiEnd();
    void sendHeart();
    void endIfStarted();

    void connectWS(const QString& url, const QByteArray &authBody);
    void sendWSHeart();
    virtual bool handleUncompMessage(QString cmd, MyJson json);

public:
    void post(QString url, MyJson json, NetJsonFunc func);
    bool isValid();

private:
    QString gameId;
    QTimer* heartTimer = nullptr;
    QByteArray authBody;
};

#endif // LIVEOPENSERVICE_H
