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

    void get(QString url, NetStringFunc func, QVariant cookies = QVariant())
    {
        get(url, [=](QNetworkReply* reply){
            func(reply->readAll());
        }, cookies);
    }

    void get(QString url, NetJsonFunc func, QVariant cookies = QVariant())
    {
        get(url, [=](QNetworkReply* reply){
            QJsonParseError error;
            QByteArray ba = reply->readAll();
            QJsonDocument document = QJsonDocument::fromJson(ba, &error);
            if (error.error != QJsonParseError::NoError)
            {
                // 过于频繁，导致同时回复了两遍JSON
                if (error.error == QJsonParseError::GarbageAtEnd && ba.contains("}{"))
                {
                    int index = ba.indexOf("}{");
                    ba = ba.right(ba.length() - index - 1);

                    // 再获取一遍
                    document = QJsonDocument::fromJson(ba, &error);
                    if (error.error != QJsonParseError::NoError)
                    {
                        qDebug() << error.error << error.errorString() << url << QString(ba);
                        return ;
                    }
                }
                else
                {
                    qDebug() << error.error << error.errorString() << url << QString(ba);
                    return ;
                }
            }
            func(document.object());
        }, cookies);
    }

    void get(QString url, NetReplyFunc func, QVariant cookies = QVariant())
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
        //request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
        if (cookies.isValid())
            request->setHeader(QNetworkRequest::CookieHeader, cookies);

        QObject::connect(manager, &QNetworkAccessManager::finished, me, [=](QNetworkReply* reply){
            func(reply);

            manager->deleteLater();
            delete request;
            reply->deleteLater();
        });
        manager->get(*request);
    }

    void get(QString url, QStringList params, NetStringFunc func, QVariant cookies = QVariant())
    {
        QString data;
        for (int i = 0; i < params.size(); i++)
        {
            if (i & 1) // 用户数据
                data += QUrl::toPercentEncoding(params.at(i));
            else // 固定变量
                data += (i==0?"":"&") + params.at(i) + "=";
        }
        get(url + "?" +data, [=](QNetworkReply* reply){
            func(reply->readAll());
        }, cookies);
    }

    void get(QString url, QStringList params, NetJsonFunc func, QVariant cookies = QVariant())
    {
        QString data;
        for (int i = 0; i < params.size(); i++)
        {
            if (i & 1) // 用户数据
                data += QUrl::toPercentEncoding(params.at(i));
            else // 固定变量
                data += (i==0?"":"&") + params.at(i) + "=";
        }
        get(url + "?" +data, [=](QNetworkReply* reply){
            QJsonParseError error;
            QByteArray ba = reply->readAll();
            QJsonDocument document = QJsonDocument::fromJson(ba, &error);
            if (error.error != QJsonParseError::NoError)
            {
                qWarning() << error.errorString() << url << ba.left(500);
                return ;
            }
            func(document.object());
        }, cookies);
    }

    void get(QString url, QStringList params, NetReplyFunc func, QVariant cookies = QVariant())
    {
        QString data;
        for (int i = 0; i < params.size(); i++)
        {
            if (i & 1) // 用户数据
                data += QUrl::toPercentEncoding(params.at(i));
            else // 固定变量
                data += (i==0?"":"&") + params.at(i) + "=";
        }
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url + "?" +data);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
        request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
        if (cookies.isValid())
            request->setHeader(QNetworkRequest::CookieHeader, cookies);
        QObject::connect(manager, &QNetworkAccessManager::finished, me, [=](QNetworkReply* reply){
            func(reply);

            manager->deleteLater();
            delete request;
            reply->deleteLater();
        });
        manager->get(*request);
    }

    void post(QString url, QStringList params, NetJsonFunc func, QVariant cookies = QVariant())
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
        post(url, ba, func, cookies);
    }

    void post(QString url, QStringList params, NetReplyFunc func, QVariant cookies = QVariant())
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
        post(url, ba, func, cookies);
    }

    void post(QString url, QByteArray ba, NetJsonFunc func, QVariant cookies = QVariant())
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
        }, cookies);
    }

    void post(QString url, QByteArray ba, NetReplyFunc func, QVariant cookies = QVariant())
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
        request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
        if (cookies.isValid())
            request->setHeader(QNetworkRequest::CookieHeader, cookies);

        // 连接槽
        QObject::connect(manager, &QNetworkAccessManager::finished, me, [=](QNetworkReply* reply){
            func(reply);
            manager->deleteLater();
            delete request;
            reply->deleteLater();
        });

        manager->post(*request, ba);
    }

    void postJson(QString url, QByteArray ba, NetReplyFunc func, QVariant cookies = QVariant())
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");
        request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
        if (cookies.isValid())
            request->setHeader(QNetworkRequest::CookieHeader, cookies);

        // 连接槽
        QObject::connect(manager, &QNetworkAccessManager::finished, me, [=](QNetworkReply* reply){
            func(reply);
            manager->deleteLater();
            delete request;
            reply->deleteLater();
        });

        manager->post(*request, ba);
    }

    void postJson(QString url, QJsonObject json, NetReplyFunc func, QVariant cookies = QVariant())
    {
        postJson(url, QJsonDocument(json).toJson(), func, cookies);
    }

    virtual void setUrlCookie(const QString& url, QNetworkRequest* request)
    {
        Q_UNUSED(url)
        Q_UNUSED(request)
    }

    virtual QVariant getCookies(const QString& cookieString) const
    {
        QList<QNetworkCookie> cookies;

        // 设置cookie
        QString cookieText = cookieString;
        QStringList sl = cookieText.split(";");
        foreach (auto s, sl)
        {
            s = s.trimmed();
            int pos = s.indexOf("=");
            QString key = s.left(pos);
            QString val = s.right(s.length() - pos - 1);
            cookies.push_back(QNetworkCookie(key.toUtf8(), val.toUtf8()));
        }

        // 请求头里面加入cookies
        QVariant var;
        var.setValue(cookies);
        return var;
    }

protected:
    QObject* me = nullptr;
};

#endif // NETINTERFACE_H
