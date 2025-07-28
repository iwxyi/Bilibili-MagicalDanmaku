#ifndef DOUYINSIGNATUREHELPER_H
#define DOUYINSIGNATUREHELPER_H

#ifdef ENABLE_WEBENGINE
#include <QWebEngineView>
#include <QWebChannel>
#endif
#include <QTimer>
#include <QEventLoop>
#include <QMutex>
#include <QMutexLocker>

class DouyinSignatureHelper : public QObject
{
    Q_OBJECT
public:
    explicit DouyinSignatureHelper(QObject *parent = nullptr);

    // 同步阻塞函数：直接返回签名
    QString getSignature(const QString &roomId, const QString &uniqueId);
    QString getXBogus(const QString &xMsStub);
    QString getXBogusForUrl(const QString &url);


private:
    void tryGetSignature();
    QString getSTUB(const QString &roomId, const QString &uniqueId);
    void initializeIfNeeded();

private:
#ifdef ENABLE_WEBENGINE
    QWebEngineView *view;
#endif
    QTimer *timer;
    QEventLoop *eventLoop;
    QString resultSignature;
    QString pendingStub;
    QString pendingRoomId;
    QString pendingUniqueId;
    QString pendingUrl;
    QMutex mutex;
    bool isInitialized;
};

#endif // DOUYINSIGNATUREHELPER_H
