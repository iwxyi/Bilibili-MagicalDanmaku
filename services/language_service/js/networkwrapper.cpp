#include "networkwrapper.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QTimer>

// 自定义超时属性
static const QNetworkRequest::Attribute TimeoutAttribute = static_cast<QNetworkRequest::Attribute>(QNetworkRequest::User + 1);

NetworkWrapper::NetworkWrapper(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
}

void NetworkWrapper::setupRequest(QNetworkRequest &request, const QVariantMap &options)
{
    qDebug() << "请求信息：" << options.keys();
    // 设置请求头
    if (options.contains("headers")) {
        QVariantMap headers = options["headers"].toMap();
        for (auto it = headers.begin(); it != headers.end(); ++it) {
            request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
        }
    }

    // 设置重定向策略
    if (options.contains("redirect")) {
        QString redirect = options["redirect"].toString().toLower();
        if (redirect == "follow") {
            request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        } else if (redirect == "manual") {
            request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false);
        }
    }

    // 设置缓存策略
    if (options.contains("cache")) {
        QString cache = options["cache"].toString().toLower();
        if (cache == "no-store") {
            request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        } else if (cache == "reload") {
            request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        }
    }
}

QString NetworkWrapper::processReply(QNetworkReply *reply)
{
    QEventLoop loop;
    QTimer timer;
    
    // 设置超时
    if (reply->request().attribute(TimeoutAttribute).isValid()) {
        int timeout = reply->request().attribute(TimeoutAttribute).toInt();
        timer.setSingleShot(true);
        timer.start(timeout);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    }
    
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (timer.isActive()) {
        timer.stop();
    }

    if (reply->error() == QNetworkReply::NoError) {
        QString response = QString::fromUtf8(reply->readAll());
        qDebug() << "JS网络请求返回：" << response.left(50) + "...";
        reply->deleteLater();
        return response;
    } else {
        QString error = QString("Network error: %1").arg(reply->errorString());
        qCritical() << "JS网络请求错误：" << error;
        reply->deleteLater();
        return error;
    }
}

QString NetworkWrapper::fetch(const QString &url, const QVariantMap &options)
{
    QNetworkRequest request(url);
    qInfo() << "JS网络请求：" << url;

    // 设置超时
    if (options.contains("timeout")) {
        request.setAttribute(TimeoutAttribute, options["timeout"].toInt());
    }

    setupRequest(request, options);

    QNetworkReply *reply;
    QString method = options.contains("method") ? options["method"].toString().toUpper() : "GET";
    
    if (method == "POST") {
        QByteArray data = options.contains("body") ? options["body"].toString().toUtf8() : QByteArray();
        reply = manager->post(request, data);
    } else if (method == "PUT") {
        QByteArray data = options.contains("body") ? options["body"].toString().toUtf8() : QByteArray();
        reply = manager->put(request, data);
    } else if (method == "DELETE") {
        reply = manager->deleteResource(request);
    } else {
        reply = manager->get(request);
    }

    return processReply(reply);
}

QString NetworkWrapper::get(const QString &url, const QVariantMap &options)
{
    qInfo() << "JS网络请求GET：" << url;
    QNetworkRequest request(url);
    
    // 设置超时
    if (options.contains("timeout")) {
        request.setAttribute(TimeoutAttribute, options["timeout"].toInt());
    }
    
    setupRequest(request, options);
    QNetworkReply *reply = manager->get(request);
    return processReply(reply);
}

QString NetworkWrapper::post(const QString &url, const QString &data, const QVariantMap &options)
{
    qInfo() << "JS网络请求POST：" << url << "，数据：" << data;
    QNetworkRequest request(url);
    
    // 设置超时
    if (options.contains("timeout")) {
        request.setAttribute(TimeoutAttribute, options["timeout"].toInt());
    }
    
    setupRequest(request, options);
    QNetworkReply *reply = manager->post(request, data.toUtf8());
    return processReply(reply);
} 
