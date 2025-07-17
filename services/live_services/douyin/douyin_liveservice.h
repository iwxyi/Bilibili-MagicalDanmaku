#ifndef DOUYIN_LIVESERVICE_H
#define DOUYIN_LIVESERVICE_H

#include "liveservicebase.h"

enum DouyinLiveStatus
{
    PREPARE = 1,
    LIVING = 2,
    PAUSE = 3,
    END = 4
};

struct HeaderListContext {
    QVector<QPair<QString, QString>> headers;
};

class DouyinLiveService : public LiveServiceBase
{
    Q_OBJECT
public:
    DouyinLiveService(QObject* parent = nullptr);

    bool isLiving() const override;
    QString getLiveStatusStr() const override;

private:
    void initWS();

protected:
    void startConnect() override;
    virtual void getRoomInfo(bool reconnect, int reconnectCount = 0) override;
    virtual void setUrlCookie(const QString & url, QNetworkRequest * request) override;
    virtual void autoAddCookie(QList<QNetworkCookie> cookies) override;
    virtual void getDanmuInfo() override;
    QString getSignature(QString roomId, QString uniqueId);
    QByteArray imFetch(QString roomId, QString uniqueId);
    void imPush(QString cursor, QString internalExt);

public slots:
    void onBinaryMessageReceived(const QByteArray &message);
    void processMessage(const QString &method, const QByteArray &payload);
};

#endif // DOUYIN_LIVESERVICE_H
