#ifndef ROOMSTATUSDIALOG_H
#define ROOMSTATUSDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class RoomStatusDialog;
}

class RoomStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RoomStatusDialog(QSettings& settings, QWidget *parent = nullptr);
    ~RoomStatusDialog() override;

private slots:
    void on_addRoomButton_clicked();

    void on_refreshStatusButton_clicked();

    void on_roomsTable_customContextMenuRequested(const QPoint &pos);

private:
    void refreshRoomStatus(QString roomId);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    Ui::RoomStatusDialog *ui;

    QSettings& settings;
    QStringList roomIds;
};

#endif // ROOMSTATUSDIALOG_H
