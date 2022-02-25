#ifndef MICROSOFTTTS_H
#define MICROSOFTTTS_H

#include <QObject>
#include <QCryptographicHash>
#include <QtWebSockets/QWebSocket>
#include <QAuthenticator>
#include <QtConcurrent/QtConcurrent>
#include <QAudioFormat>
#include <QAudioOutput>

class MicrosoftTTS : public QObject
{
    Q_OBJECT
public:
    explicit MicrosoftTTS(QString dataPath, QString areaCode, QString key, QObject *parent = nullptr);

    void speakText(QString ssml);
    void speakNext();
    void playFile(QString filePath, bool deleteAfterPlay = true);

    void setHostUrl(QString url);
    void setSubscriptionKey(QString key);


signals:

public slots:

private slots:
    void refreshToken();

private:
    QString savedDir;
    QString areaCode;
    QString subscriptionKey;
    QByteArray accessToken;

    QStringList speakQueue;
    QAudioOutput *audio = nullptr;
    QAudioFormat fmt;
    bool getting = false; // 是否正在获取语音或正在播放

};

#endif // MICROSOFTTTS_H
