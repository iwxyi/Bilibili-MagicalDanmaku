#ifndef LIVEROOMSERVICE_H
#define LIVEROOMSERVICE_H

#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QFile>
#include "livedanmaku.h"
#include "runtimeinfo.h"
#include "usersettings.h"
#include "accountinfo.h"
#include "platforminfo.h"
#include "widgets/netinterface.h"

class LiveRoomService : public QObject, public NetInterface
{
    Q_OBJECT
    friend class MainWindow;

public:
    explicit LiveRoomService(QObject *parent = nullptr);

signals:
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
    virtual void startConnectIdentityCode(const QString &code) {}
    /// 更新当前舰长
    virtual void updateExistGuards(int page = 0) = 0;
    /// 获取机器人账号信息
    virtual void getCookieAccount() = 0;

    /// 初始化每日数据
    void startCalculateDailyData();
    /// 保存每日数据
    void saveCalculateDailyData();
    QVariant getCookies() const;

public:
    /// 根据Url设置对应的Cookie
    virtual void setUrlCookie(const QString& url, QNetworkRequest* request) override;
    /// 设置全局默认的Cookie变量
    virtual void autoSetCookie(const QString &s);
    

private:
    // 每日数据
    QSettings *dailySettings = nullptr;
    QTimer *dayTimer = nullptr;
    int dailyCome = 0;       // 进来数量人次
    int dailyPeopleNum = 0;  // 本次进来的人数（不是全程的话，不准确）
    int dailyDanmaku = 0;    // 弹幕数量
    int dailyNewbieMsg = 0;  // 新人发言数量（需要开启新人发言提示）
    int dailyNewFans = 0;    // 关注数量
    int dailyTotalFans = 0;  // 粉丝总数量（需要开启感谢关注）
    int dailyGiftSilver = 0; // 银瓜子总价值
    int dailyGiftGold = 0;   // 金瓜子总价值（不包括船员）
    int dailyGuard = 0;      // 上船/续船人次
    int dailyMaxPopul = 0;   // 最高人气
    int dailyAvePopul = 0;   // 平均人气
    bool todayIsEnding = false;
};

#endif // LIVEROOMSERVICE_H
