#ifndef XFYTTS_H
#define XFYTTS_H

#include <QObject>
#include <QCryptographicHash>
#include <QtWebSockets/QWebSocket>
#include <QAuthenticator>
#include <QtConcurrent/QtConcurrent>
#include <QAudioFormat>
#include <QAudioOutput>

#define AUTH_DEB if (0) qDebug()

class XfyTTS : public QObject
{
    Q_OBJECT
public:
    XfyTTS(QString dataPath, QString APPID, QString APIKey, QString APISecret, QObject* parent = nullptr);

    void speakText(QString text);
    void speakNext();
    void playFile(QString filePath, bool deleteAfterPlay = true);

    void setAppId(QString s);
    void setApiKey(QString s);
    void setApiSecret(QString s);

    void setName(QString name);
    void setPitch(int pitch);
    void setSpeed(int speed);
    void setVolume(int volume);

private:
    void startConnect();
    QString getAuthorization() const;
    QString getSignature() const;
    QString getDate() const;

    void sendText(QString text);
    void generalAudio(QByteArray* ba);

signals:
    void signalTTSPrepared(QString filePath);

private:
    QString APPID;
    QString APIKey;
    QString APISecret;

    QString savedDir;
    QWebSocket* socket = nullptr;
    QString hostUrl = "wss://tts-api.xfyun.cn/v2/tts";

    QString vcn = "xiaoyan"; // 发音人
    int pitch = 50;  // 音调
    int speed = 50;  // 音速
    int volume = 50; // 音量

    QAudioFormat fmt;
    QStringList speakQueue;
};

#endif // XFYTTS_H
