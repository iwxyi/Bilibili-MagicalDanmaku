#ifndef DOUYINSIGNATUREHELPER_H
#define DOUYINSIGNATUREHELPER_H

#include <QObject>
#include <QWebEngineView>
#include <QWebChannel>
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

private:
    void tryGetSignature();
    QString getSTUB(const QString &roomId, const QString &uniqueId);
    void initializeIfNeeded();

private:
    QWebEngineView *view;
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
