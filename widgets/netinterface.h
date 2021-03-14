#ifndef NETINTERFACE_H
#define NETINTERFACE_H

#include <QNetworkCookie>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QObject>

typedef std::function<void(QString)> const NetStringFunc;
typedef std::function<void(QJsonObject)> const NetJsonFunc;
typedef std::function<void(QNetworkReply*)> const NetReplyFunc;

class NetInterface
{
public:
    NetInterface(QObject* me);
    virtual ~NetInterface(){}

    void get(QString url, NetStringFunc func);
    void get(QString url, NetJsonFunc func);
    void get(QString url, NetReplyFunc func);
    void post(QString url, QStringList params, NetJsonFunc func);
    void post(QString url, QByteArray ba, NetJsonFunc func);
    void post(QString url, QByteArray ba, NetReplyFunc func);

    virtual void setCookie(const QString& url, QNetworkRequest* request);

protected:
    QObject* me = nullptr;
};

#endif // NETINTERFACE_H
