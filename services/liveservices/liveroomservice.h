#ifndef LIVEROOMSERVICE_H
#define LIVEROOMSERVICE_H

#include "widgets/netinterface.h"
#include "livestatisticservice.h"

class LiveRoomService : public QObject, public NetInterface, public LiveStatisticService
{
    Q_OBJECT
    friend class MainWindow;

public:
    explicit LiveRoomService(QObject *parent = nullptr);

    /// 初始化所有变量，new、connect等
    virtual void init();

    /// 读取设置
    virtual void readConfig();

signals:
    /// 房间号改变，例如通过解析身份码导致的房间ID变更
    void signalRoomUidChanged(const QString &roomId);
    void signalUpUidChanged(const QString &uid);
    void signalRobotIdChanged(const QString &uid);
    void signalRoomTitleChanged(const QString &title);
    void signalRoomCoverChanged(const QPixmap &pixmap);
    void signalUpHeadChanged(const QPixmap &pixmap);
    void signalRobotHeadChanged(const QPixmap &pixmap);
    void signalConnectStateChanged(QAbstractSocket::SocketState state);

public slots:
    /// 开始获取房间信息
    virtual void startConnectRoom(const QString &roomId) = 0;
    /// 通过身份码获取房间信息
    virtual void startConnectIdentityCode(const QString &code) { Q_UNUSED(code) }
    /// 更新当前舰长
    virtual void updateExistGuards(int page = 0) override { Q_UNUSED(page) }
    /// 获取机器人账号信息
    virtual void getCookieAccount() = 0;

    QVariant getCookies() const;

public:
    /// 根据Url设置对应的Cookie
    virtual void setUrlCookie(const QString& url, QNetworkRequest* request) override;
    /// 设置全局默认的Cookie变量
    virtual void autoSetCookie(const QString &s);
    

private:
};

#endif // LIVEROOMSERVICE_H
