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
    void signalConnectionStarted(); // WebSocket开始连接
    void signalConnectionStateChanged(QAbstractSocket::SocketState state);
    void signalWorkStarted(); // 直播开始/连接上就已经开始直播
    void signalLiveStarted();
    void signalLiveStopped();

    void signalRoomUidChanged(const QString &roomId); // 房间号改变，例如通过解析身份码导致的房间ID变更
    void signalUpUidChanged(const QString &uid);
    void signalUpInfoChanged();
    void signalRobotIdChanged(const QString &uid);
    void signalRoomInfoChanged();
    void signalRoomCoverChanged(const QPixmap &pixmap);
    void signalUpHeadChanged(const QPixmap &pixmap);
    void signalRobotHeadChanged(const QPixmap &pixmap);

public slots:
    /// 开始获取房间信息
    virtual void startConnectRoom(const QString &roomId) = 0;
    /// 通过身份码获取房间信息
    virtual void startConnectIdentityCode(const QString &code) { Q_UNUSED(code) }
    /// 更新当前舰长
    virtual void updateExistGuards(int page = 0) override { Q_UNUSED(page) }
    /// 获取礼物ID
    virtual void getGiftList() {}
    /// 获取表情包ID
    virtual void getEmoticonList() {}

    /// 获取机器人账号信息
    virtual void getCookieAccount() = 0;
    QVariant getCookies() const;

public:
    /// 根据Url设置对应的Cookie
    virtual void setUrlCookie(const QString& url, QNetworkRequest* request) override;
    /// 设置全局默认的Cookie变量
    virtual void autoSetCookie(const QString &s);
    

private:
    // 房间信息
    QPixmap roomCover; // 直播间封面原图
    QPixmap upFace; // 主播头像原图

    // 我的直播
    QString myLiveRtmp; // rtmp地址
    QString myLiveCode; // 直播码

    // 本次直播的礼物列表
    QList<LiveDanmaku> liveAllGifts;
    QList<LiveDanmaku> liveAllGuards;
};

#endif // LIVEROOMSERVICE_H
