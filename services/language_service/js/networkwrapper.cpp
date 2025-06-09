#include "networkwrapper.h"
#include <QNetworkRequest>
#include <QJsonDocument>

NetworkWrapper::NetworkWrapper(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
}

QString NetworkWrapper::fetch(const QString &url, const QVariantMap &options)
{
    QNetworkRequest request(url);
    
    // 设置请求头
    if (options.contains("headers")) {
        QVariantMap headers = options["headers"].toMap();
        for (auto it = headers.begin(); it != headers.end(); ++it) {
            request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
        }
    }

    QNetworkReply *reply;
    if (options.contains("method") && options["method"].toString().toUpper() == "POST") {
        QByteArray data = options.contains("body") ? options["body"].toString().toUtf8() : QByteArray();
        reply = manager->post(request, data);
    } else {
        reply = manager->get(request);
    }

    return processReply(reply);
}

QString NetworkWrapper::get(const QString &url)
{
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    return processReply(reply);
}

QString NetworkWrapper::post(const QString &url, const QString &data)
{
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->post(request, data.toUtf8());
    return processReply(reply);
}

QString NetworkWrapper::processReply(QNetworkReply *reply)
{
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

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