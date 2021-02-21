#ifndef CATCHYOUWIDGET_H
#define CATCHYOUWIDGET_H

#include <QWidget>
#include <functional>

namespace Ui {
class CatchYouWidget;
}

class QNetworkReply;

class CatchYouWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CatchYouWidget(QWidget *parent = nullptr);
    ~CatchYouWidget();

    void catchUser(QString userId);

    struct RoomInfo
    {
        QString roomId;
        QString roomName;
        QString upId;
        QString upName;
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

    void get(QString url, const std::function<void(QJsonObject)> func);

private:
    Ui::CatchYouWidget *ui;

    QString userId;
    QList<UserInfo> users;
    QList<RoomInfo> rooms;

    qint64 currentTaskTs = 0; // 任务Id，允许中止
};

#endif // CATCHYOUWIDGET_H
