#ifndef NETINTERFACE_H
#define NETINTERFACE_H

#include <QNetworkCookie>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QObject>
#include <QEventLoop>
#include "myjson.h"


typedef std::function<void()> const NetVoidFunc;
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

    /// 非阻塞形式，Lambda连接
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
            if (func)
                func(document.object());
        }, cookies);
    }
    
    void get(QString url, NetJsonFunc func, NetStringFunc finalfunc, QVariant cookies = QVariant())
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
                        if (finalfunc)
                            finalfunc(ba);
                        return ;
                    }
                }
                else
                {
                    qDebug() << error.error << error.errorString() << url << QString(ba);
                    if (finalfunc)
                        finalfunc(ba);
                    return ;
                }
            }
            if (func)
                func(document.object());
            if (finalfunc)
                finalfunc(ba);
        }, cookies);
    }

    void get(QString url, NetReplyFunc func, QVariant cookies = QVariant())
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
        request->setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());
        if (cookies.isValid())
        {
            // 如果是字符串
            if (cookies.type() == QVariant::String)
            {
                QString c = cookies.toString();
                if (!c.isEmpty())
                    request->setRawHeader("Cookie", c.toUtf8());
            }
            else
                request->setHeader(QNetworkRequest::CookieHeader, cookies);
        }

        QObject::connect(manager, &QNetworkAccessManager::finished, me, [=](QNetworkReply* reply){
            // 获取 Set-Cookie
            QVariant setCookieHeader = reply->header(QNetworkRequest::SetCookieHeader);
            if (setCookieHeader.isValid()) {
                QList<QNetworkCookie> cookies = setCookieHeader.value<QList<QNetworkCookie>>();
                autoAddCookie(cookies);
            }
            
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
        request->setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());
        if (cookies.isValid())
        {
            // 如果是字符串
            if (cookies.type() == QVariant::String)
            {
                QString c = cookies.toString();
                if (!c.isEmpty())
                    request->setRawHeader("Cookie", c.toUtf8());
            }
            else
                request->setHeader(QNetworkRequest::CookieHeader, cookies);
        }
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
        request->setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());
        if (cookies.isValid() && !cookies.isNull())
        {
            // 如果是字符串
            if (cookies.type() == QVariant::String)
            {
                QString c = cookies.toString();
                if (!c.isEmpty())
                    request->setRawHeader("Cookie", c.toUtf8());
            }
            else
                request->setHeader(QNetworkRequest::CookieHeader, cookies);
        }

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
        request->setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());
        if (cookies.isValid())
        {
            // 如果是字符串
            if (cookies.type() == QVariant::String)
            {
                QString c = cookies.toString();
                if (!c.isEmpty())
                    request->setRawHeader("Cookie", c.toUtf8());
            }
            else
                request->setHeader(QNetworkRequest::CookieHeader, cookies);
        }

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

    /// 阻塞形式的，返回JSON
    QByteArray getToBytes(QString url, QVariant cookies = QVariant(), QList<QPair<QString, QString>> headers = {})
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());
        foreach (auto p, headers)
        {
            request->setRawHeader(p.first.toUtf8(), p.second.toUtf8());
        }
        if (cookies.isValid())
        {
            if (cookies.type() == QVariant::String)
            {
                QString c = cookies.toString();
                if (!c.isEmpty())
                    request->setRawHeader("Cookie", c.toUtf8());
            }
            else
                request->setHeader(QNetworkRequest::CookieHeader, cookies);
        }
        QNetworkReply* reply = manager->get(*request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        QByteArray data = reply->readAll();
        return data;
    }
    
    MyJson getToJson(QString url, QVariant cookies = QVariant(), QList<QPair<QString, QString>> headers = {})
    {
        QByteArray data = getToBytes(url, cookies, headers);
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << "解析返回文本JSON错误：" << error.errorString() << url << data;
            return MyJson();
        }
        return document.object();
    }

    QByteArray postToBytes(QString url, QByteArray ba, QVariant cookies = QVariant(), QList<QPair<QString, QString>> headers = {})
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        setUrlCookie(url, request);
        request->setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());
        foreach (auto p, headers)
        {
            request->setRawHeader(p.first.toUtf8(), p.second.toUtf8());
        }
        if (cookies.isValid())
        {
            if (cookies.type() == QVariant::String)
            {
                QString c = cookies.toString();
                if (!c.isEmpty())
                    request->setRawHeader("Cookie", c.toUtf8());
            }
            else
                request->setHeader(QNetworkRequest::CookieHeader, cookies);
        }
        QNetworkReply* reply = manager->post(*request, ba);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        QByteArray data = reply->readAll();
        return data;
    }

    QByteArray postToBytes(QString url, QJsonObject json, QVariant cookies = QVariant(), QList<QPair<QString, QString>> headers = {})
    {
        return postToBytes(url, QJsonDocument(json).toJson(), cookies, headers);
    }

    MyJson postToJson(QString url, QByteArray ba, QVariant cookies = QVariant(), QList<QPair<QString, QString>> headers = {})
    {
        QByteArray data = postToBytes(url, ba, cookies, headers);
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << "解析返回文本JSON错误：" << error.errorString() << url << data;
            return MyJson();
        }
        return document.object();
    }

    MyJson postToJson(QString url, QJsonObject json, QVariant cookies = QVariant(), QList<QPair<QString, QString>> headers = {})
    {
        return postToJson(url, QJsonDocument(json).toJson(), cookies, headers);
    }
    

    /// Cookie相关
    virtual void setUrlCookie(const QString& url, QNetworkRequest* request)
    {
        Q_UNUSED(url)
        Q_UNUSED(request)
    }

    /// 联网返回的Set-Cookie头
    virtual void autoAddCookie(QList<QNetworkCookie> cookies)
    {
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

    void setUserAgent(const QString& ua)
    {
        if (!ua.isEmpty())
            qInfo() << "设置UA：" << ua;
        userAgent = ua;
    }

    QString getUserAgent() const
    {
        if (userAgent.isEmpty())
            return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36";
        return userAgent;
    }

protected:
    QObject* me = nullptr;
    QString userAgent;
};

#endif // NETINTERFACE_H
