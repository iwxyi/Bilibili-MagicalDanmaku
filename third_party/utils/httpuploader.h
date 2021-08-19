#ifndef HTTPUPLOADER_H
#define HTTPUPLOADER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QtNetwork/QHttpPart>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>

class HttpUploader : public QObject
{
    Q_OBJECT
public:
    HttpUploader(const QString& url);
    ~HttpUploader();

    void setUrl(const QString& url) { this->url = url; }
    QString getUrl() { return url; }
    void setCookies(QString c);
    void setCookies(QVariant c);
    QVariant toVariantCookies(QString c) const;

    bool addTextField(const QString& key, const QByteArray& value);
    bool addFileField(const QString& key, const QString& file_path);
    bool addFileField(const QString& key, const QString& file_name, const QString& file_path);
    bool addFileField(const QString& key, const QString& file_name, const QString& file_path, const QString& type);
    bool post();
    QNetworkAccessManager* getNetworkManager() { return net_manager; }

signals:
    void finished(QNetworkReply* reply);
    void errored(QNetworkReply::NetworkError code);
    void progress(qint64 sent, qint64 total); // 结束后sent和total都会变成0，相除要特判！

private:
    QString url;
    QByteArray post_content;
    QVariant cookies;

    QNetworkAccessManager*  net_manager = nullptr;
    QNetworkReply*          net_reply = nullptr;
    QHttpMultiPart*         multi_part = nullptr;
};

#endif // HTTPUPLOADER_H
