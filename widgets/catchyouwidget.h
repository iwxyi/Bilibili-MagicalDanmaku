#ifndef CATCHYOUWIDGET_H
#define CATCHYOUWIDGET_H

#include <QWidget>
#include <functional>
#include "commonvalues.h"

namespace Ui {
class CatchYouWidget;
}

class QNetworkReply;

class CatchYouWidget : public QWidget, public CommonValues
{
    Q_OBJECT
public:
    explicit CatchYouWidget(QWidget *parent = nullptr);
    ~CatchYouWidget();

    void catchUser(QString userId);
    void setDefaultUser(QString userId);

    struct RoomInfo
    {
        qint64 roomId;
        QString roomName;
        qint64 upUid;
        QString upName;
        qint64 time = 0; // 秒
        QStringList danmakus;
    };

    struct UserInfo
    {
        QString userId;
        QString userName;
        int liveStatus = 0;

        UserInfo(QString id, QString name) : userId(id), userName(name)
        {}
    };

private slots:
    void on_userButton_clicked();

    void on_refreshButton_clicked();

private:
    void getUserFollows(qint64 taskTs, QString userId, int page = 1);
    void detectUserLiveStatus(qint64 taskTs, int index);
    void detectRoomDanmaku(qint64 taskTs, QString roomId);
    void addInRoom(qint64 taskTs, QString roomId, qint64 second, QStringList texts);
    void detectRoomGift(qint64 taskTs, QString roomId);

    void addToTable(RoomInfo room);

    void get(QString url, const std::function<void(QJsonObject)> func);

private:
    Ui::CatchYouWidget *ui;

    QString userId;
    QList<UserInfo> users;
    QList<RoomInfo> inRooms;

    qint64 currentTaskTs = 0; // 任务Id，允许中止
};

#endif // CATCHYOUWIDGET_H
