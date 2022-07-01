#include "singleentrance.h"
#include "ui_singleentrance.h"
#include <QPainter>

SingleEntrance::SingleEntrance(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SingleEntrance)
{
    ui->setupUi(this);
    setObjectName("SingleEntrance");
}

SingleEntrance::~SingleEntrance()
{
    delete ui;
}

void SingleEntrance::setRoomId(QString roomId)
{
    ui->lineEdit->setText(roomId);
}

void SingleEntrance::setRobotName(QString name)
{
    ui->lineEdit_2->setText(name);
}

void SingleEntrance::on_pushButton_clicked()
{
    if (ui->lineEdit->text().isEmpty())
        return ;
    emit signalRoomIdChanged(ui->lineEdit->text());
}

void SingleEntrance::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
}

void SingleEntrance::on_lineEdit_returnPressed()
{
    on_pushButton_clicked();
}

void SingleEntrance::on_pushButton_2_clicked()
{
    emit signalLogin();
}
