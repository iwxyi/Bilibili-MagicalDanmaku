#ifndef NETWORKWRAPPER_H
#define NETWORKWRAPPER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class NetworkWrapper : public QObject
{
    Q_OBJECT
public:
    explicit NetworkWrapper(QObject *parent = nullptr);

    Q_INVOKABLE QString fetch(const QString &url, const QVariantMap &options = QVariantMap());
    Q_INVOKABLE QString get(const QString &url, const QVariantMap &options = QVariantMap());
    Q_INVOKABLE QString post(const QString &url, const QString &data, const QVariantMap &options = QVariantMap());

private:
    QNetworkAccessManager *manager;
    QString processReply(QNetworkReply *reply);
    void setupRequest(QNetworkRequest &request, const QVariantMap &options);
};

#endif // NETWORKWRAPPER_H 