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

    void speakSSML(QString ssml);
    void speakNext();
    void playFile(QString filePath, bool deleteAfterPlay = true);

    void setAreaCode(QString area);
    void setSubscriptionKey(QString key);
    bool isPlaying() const;

signals:
    void signalError(QString err);

public slots:
    void refreshToken();
    void getWhitePiaoToken();
    void clearQueue();

private:
    bool isDownloadingOrSpeaking() const;

private:
    QString savedDir;
    QString areaCode;
    QString subscriptionKey;
    QByteArray accessToken;

    QStringList speakQueue;
    QAudioOutput *audio = nullptr;
    QAudioFormat fmt;
    // bool getting = false; // 是否正在获取语音或正在播放
    QTimer* refreshTimer = nullptr; // 刷新token时间
    QTimer* timeoutTimer = nullptr; // 连接超时
    QEventLoop connectLoop;
    bool playing = false;

};

#endif // MICROSOFTTTS_H
