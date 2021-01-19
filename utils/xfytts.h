#ifndef XFYTTS_H
#define XFYTTS_H

#include <QObject>
#include <QCryptographicHash>
#include <QtWebSockets/QWebSocket>
#include <QAuthenticator>
#include <QtConcurrent/QtConcurrent>

#define AUTH_DEB if (1) qDebug()

class XfyTTS : public QObject
{
public:
    XfyTTS(QString APIKey, QString APISecret, QObject* parent = nullptr);

    void startConnect();

private:
    QString getAuthorization() const;
    QString getSignature() const;
    QString getDate() const;

    QString toHmacSha1Base64(QByteArray key, QByteArray baseString) const;

private:
    QString APIKey;
    QString APISecret;
    QWebSocket* socket = nullptr;

    QString hostUrl = "wss://tts-api.xfyun.cn/v2/tts";
};

#endif // XFYTTS_H
