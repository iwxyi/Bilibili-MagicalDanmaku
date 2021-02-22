#ifndef CATCHYOUWIDGET_H
#define CATCHYOUWIDGET_H

#include <QWidget>
#include <functional>
#include <QSettings>
#include "commonvalues.h"

namespace Ui {
class CatchYouWidget;
}

class QNetworkReply;

class CatchYouWidget : public QWidget, public CommonValues
{
    Q_OBJECT
public:
    explicit CatchYouWidget(QSettings& settings, QWidget *parent = nullptr);
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
        int gold100 = 0;
    };

    struct UserInfo
    {
        QString userId;
        QString userName;
        int liveStatus = 0; // -1未播，0未知，1直播

        UserInfo(QString id, QString name) : userId(id), userName(name)
        {}
    };

private slots:
    void on_userButton_clicked();

    void on_refreshButton_clicked();

    void on_tableWidget_customContextMenuRequested(const QPoint &);

    void on_cdSpin_valueChanged(int arg1);

private:
    void getUserFollows(qint64 taskTs, QString userId, int page = 1);
    void detectUserLiveStatus(qint64 taskTs, int index);
    void detectRoomDanmaku(qint64 taskTs, QString upUid, QString roomId);
    void detectRoomGift(qint64 taskTs, QString upUid, QString roomId, qint64 second, QStringList texts);
    void addInRoom(qint64 taskTs, QString upUid, QString roomId, qint64 second, QStringList texts, int gold100);
    void detectRoomGift(qint64 taskTs, QString upUid, QString roomId);

    void addToTable(RoomInfo room);
    void openRoomVideo(QString roomId);

    void get(QString url, const std::function<void(QJsonObject)> func);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    Ui::CatchYouWidget *ui;
    QSettings& settings;

    QString userId;
    QList<UserInfo> users;
    QList<RoomInfo> inRooms;

    qint64 currentTaskTs = 0; // 任务Id，允许中止
};

#endif // CATCHYOUWIDGET_H
