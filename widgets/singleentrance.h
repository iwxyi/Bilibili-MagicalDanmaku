#ifndef SINGLEENTRANCE_H
#define SINGLEENTRANCE_H

#include <QWidget>

namespace Ui {
class SingleEntrance;
}

class SingleEntrance : public QWidget
{
    Q_OBJECT

public:
    explicit SingleEntrance(QWidget *parent = nullptr);
    virtual ~SingleEntrance();

    void setRoomId(QString roomId);
    void setRobotName(QString name);

signals:
    void signalRoomIdChanged(QString roomId);
    void signalLogin();

private slots:
    void on_pushButton_clicked();

    void on_lineEdit_returnPressed();

    void on_pushButton_2_clicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::SingleEntrance *ui;
};

#endif // SINGLEENTRANCE_H
