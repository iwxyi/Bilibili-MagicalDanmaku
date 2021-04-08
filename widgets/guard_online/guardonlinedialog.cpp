#include <QKeyEvent>
#include <QMenu>
#include <QAction>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>
#include <QDesktopServices>
#include "guardonlinedialog.h"
#include "ui_guardonlinedialog.h"

GuardOnlineDialog::GuardOnlineDialog(QSettings *settings, QString roomId, QString upUid, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GuardOnlineDialog), settings(settings), roomId(roomId), upUid(upUid)
{
    ui->setupUi(this);
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    refreshOnlineGuards(0);
}

GuardOnlineDialog::~GuardOnlineDialog()
{
    delete ui;
}

void GuardOnlineDialog::on_refreshButton_clicked()
{
    refreshOnlineGuards(0);
}

void GuardOnlineDialog::refreshOnlineGuards(int page)
{
    if (page == 0)
    {
        page = 1;
        ui->tableWidget->setRowCount(0);
        guardIds.clear();
    }

    auto addOnlineGuard = [=](QJsonObject user){
        bool alive = user.value("is_alive").toInt();
        if (!alive)
            return ;

        ui->tableWidget->setRowCount(ui->tableWidget->rowCount()+1);

        qint64 uid = qint64(user.value("uid").toDouble());
        QString uname = user.value("username").toString();
        int guard_level = user.value("guard_level").toInt();
        QJsonObject medalInfo = user.value("medal_info").toObject();
        int medal_level = medalInfo.value("medal_level").toInt();
        QString guardName = guard_level == 1 ? "总督" : guard_level == 2 ? "提督" : "舰长";

        guardIds.append(uid);
        int row = ui->tableWidget->rowCount()-1;
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(uname));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(guardName + " " + QString::number(medal_level)));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(""));
    };

    QString url = "https://api.live.bilibili.com/xlive/app-room/v2/guardTab/topList?actionKey=appkey&appkey=27eb53fc9058f8c3&roomid=" + roomId
            +"&page=" + QString::number(page) + "&ruid=" + upUid + "&page_size=30";
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray ba = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << "获取房间信息出错：" << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 0)
        {
            qDebug() << "返回结果不为0：" << json.value("message").toString();
            return ;
        }

        QJsonObject data = json.value("data").toObject();

        // top3
        if (page == 1)
        {
            QJsonArray top3 = data.value("top3").toArray();
            foreach (QJsonValue val, top3)
                addOnlineGuard(val.toObject());
        }

        // list
        QJsonArray list = data.value("list").toArray();
        foreach (QJsonValue val, list)
            addOnlineGuard(val.toObject());

        // 下一页
        QJsonObject info = data.value("info").toObject();
        int page = info.value("page").toInt();
        int now = info.value("now").toInt();
        if (now < page)
            refreshOnlineGuards(now+1);
    });
    manager->get(*request);
}

void GuardOnlineDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_F5)
    {
        on_refreshButton_clicked();
    }
    return QDialog::keyPressEvent(e);
}

void GuardOnlineDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);

    restoreGeometry(settings->value("rooms/geometry").toByteArray());
}

void GuardOnlineDialog::closeEvent(QCloseEvent *e)
{
    settings->setValue("rooms/geometry", this->saveGeometry());

    QDialog::closeEvent(e);
}
