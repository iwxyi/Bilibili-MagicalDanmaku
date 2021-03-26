#ifndef NETINTERFACE_H
#define NETINTERFACE_H

#include <QNetworkCookie>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QObject>
#include "myjson.h"

typedef std::function<void(QString)> const NetStringFunc;
typedef std::function<void(MyJson)> const NetJsonFunc;
typedef std::function<void(QNetworkReply*)> const NetReplyFunc;

class NetInterface
{
public:
    NetInterface(QObject* me)
        : me(me)
    {
    }
    virtual ~NetInterface(){}

    void get(QString url, NetStringFunc func)
    {
        get(url, [=](QNetworkReply* reply){
            func(reply->readAll());
        });
    }

    void get(QString url, NetJsonFunc func)
    {
        get(url, [=](QNetworkReply* reply){
            QJsonParseError error;
            QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error != QJsonParseError::NoError)
            {
                qDebug() << error.errorString() << url;
                return ;
            }
            func(document.object());
        });
    }

    void get(QString url, NetReplyFunc func)
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
        request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
        QObject::connect(manager, &QNetworkAccessManager::finished, me, [=](QNetworkReply* reply){
            func(reply);

            manager->deleteLater();
            delete request;
            reply->deleteLater();
        });
        manager->get(*request);
    }

    void post(QString url, QStringList params, NetJsonFunc func)
    {
        QString data;
        for (int i = 0; i < params.size(); i++)
        {
            if (i & 1) // 用户数据
                data += QUrl::toPercentEncoding(params.at(i));
            else // 固定变量
                data += (i==0?"":"&") + params.at(i) + "=";
        }
        QByteArray ba(data.toLatin1());
        post(url, ba, func);
    }

    void post(QString url, QStringList params, NetReplyFunc func)
    {
        QString data;
        for (int i = 0; i < params.size(); i++)
        {
            if (i & 1) // 用户数据
                data += QUrl::toPercentEncoding(params.at(i));
            else // 固定变量
                data += (i==0?"":"&") + params.at(i) + "=";
        }
        QByteArray ba(data.toLatin1());
        post(url, ba, func);
    }

    void post(QString url, QByteArray ba, NetJsonFunc func)
    {
        post(url, ba, [=](QNetworkReply* reply){
            QByteArray data = reply->readAll();
            QJsonParseError error;
            QJsonDocument document = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError)
            {
                qCritical() << "解析返回文本JSON错误：" << error.errorString() << url << reply;
                return ;
            }
            QJsonObject json = document.object();
            func(json);
        });
    }

    void post(QString url, QByteArray ba, NetReplyFunc func)
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
        request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");

        // 连接槽
        QObject::connect(manager, &QNetworkAccessManager::finished, me, [=](QNetworkReply* reply){
            func(reply);
            manager->deleteLater();
            delete request;
            reply->deleteLater();
        });

        manager->post(*request, ba);
    }

    void postJson(QString url, QByteArray ba, NetReplyFunc func)
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");
        request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");

        // 连接槽
        QObject::connect(manager, &QNetworkAccessManager::finished, me, [=](QNetworkReply* reply){
            func(reply);
            manager->deleteLater();
            delete request;
            reply->deleteLater();
        });

        manager->post(*request, ba);
    }

    virtual void setUrlCookie(const QString& url, QNetworkRequest* request)
    {
        Q_UNUSED(url)
        Q_UNUSED(request)
    }

protected:
    QObject* me = nullptr;
};

#endif // NETINTERFACE_H
