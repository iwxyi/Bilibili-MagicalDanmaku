#ifndef ROOMSTATUSDIALOG_H
#define ROOMSTATUSDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class RoomStatusDialog;
}

struct RoomStatus
{
    RoomStatus(QString id)
    {
        this->roomId = id;
    }

    bool operator==(const RoomStatus& another) const
    {
        return this->roomId == another.roomId;
    }

    QString roomId;
    QString roomTitle;
    QString uid;
    QString uname;
    int liveStatus = 0;
    int pkStatus = 0;
};

class RoomStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RoomStatusDialog(QSettings& settings, QWidget *parent = nullptr);
    ~RoomStatusDialog() override;

private slots:
    void on_addRoomButton_clicked();

    void on_refreshStatusButton_clicked();

    void on_roomsTable_customContextMenuRequested(const QPoint &);

    void on_roomsTable_cellDoubleClicked(int row, int);

private:
    void refreshRoomStatus(QString roomId);
    void openRoomVideo(QString roomId);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private:
    Ui::RoomStatusDialog *ui;

    QSettings& settings;
    QList<RoomStatus> roomStatus;
};

#endif // ROOMSTATUSDIALOG_H
