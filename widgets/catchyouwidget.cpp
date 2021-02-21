#include <QInputDialog>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QMediaContent>
#include <QSettings>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVideoWidget>
#include <QVideoProbe>
#include <QDir>
#include "catchyouwidget.h"
#include "ui_catchyouwidget.h"

CatchYouWidget::CatchYouWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CatchYouWidget)
{
    ui->setupUi(this);
}

CatchYouWidget::~CatchYouWidget()
{
    delete ui;
}

void CatchYouWidget::catchUser(QString userId)
{
    this->userId = userId;
    rooms.clear();
    ui->tableWidget->clear();

    if (userId.isEmpty())
        return ;

    currentTaskTs = QDateTime::currentMSecsSinceEpoch();

    getUserFollows(currentTaskTs, userId);
}

void CatchYouWidget::on_userButton_clicked()
{
    bool ok = false;
    QString id = QInputDialog::getText(this, "输入用户Id", "请输入要抓的用户Id", QLineEdit::Normal, "", &ok);
    if (!ok)
        return ;
    catchUser(id);
}

void CatchYouWidget::on_refreshButton_clicked()
{
    if (!userId.isEmpty())
        catchUser(userId);
}

void CatchYouWidget::getUserFollows(qint64 taskTs, QString userId, int page)
{
    if (currentTaskTs != taskTs)
        return ;
    const int pageSize = 50;
    if (page == 1)
        users.clear();

    get("http://api.bilibili.com/x/relation/followings?vmid=" + userId + "&ps=50&pn=" + QString::number(page), [=](QJsonObject json){
        QJsonObject data = json.value("data").toObject();
        int total = data.value("total").toInt();
        QJsonArray list = data.value("list").toArray();

        foreach (QJsonValue val, list)
        {
            QJsonObject obj = val.toObject();
            qint64 mid = qint64(obj.value("mid").toDouble());
            QString uname = obj.value("uname").toString();
            users.append(UserInfo(QString::number(mid), uname));
        }

        if (pageSize * page < total && page < 5) // 继续下一页
        {
            getUserFollows(taskTs, userId, page + 1);
        }
        else // 结束了
        {
            qDebug() << "总数：" << total << "可见总数：" << users.size();
            ui->progressBar->setRange(0, users.size() - 1);
            ui->progressBar->setValue(0);
            detectUserLiveStatus(taskTs, 0);
        }
    });
}

void CatchYouWidget::detectUserLiveStatus(qint64 taskTs, int index)
{
    if (currentTaskTs != taskTs || index >= users.size())
        return ;

    UserInfo user = users.at(index);

    QString url = "http://api.live.bilibili.com/room/v1/Room/getRoomInfoOld?mid=" + user.userId;
    get(url, [=](QJsonObject json) {
        if (json.value("code").toInt() != 0)
        {
            qCritical() << "返回结果不为0：" << json.value("message").toString();
            return ;
        }

        QJsonObject data = json.value("data").toObject();
        int roomStatus = data.value("roomStatus").toInt(); // 1
        int roundStatus = data.value("roundStatus").toInt(); // 1
        int liveStatus = data.value("liveStatus").toInt(); // 1
        if (roomStatus && liveStatus) // 直播中，需要检测
        {
            QString roomId = QString::number(qint64(data.value("roomid").toDouble()));

        }

        // 检测下一个
        ui->progressBar->setValue(index);
        detectUserLiveStatus(taskTs, index + 1);
    });
}

void CatchYouWidget::get(QString url, std::function<void(QJsonObject)> const func)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString() << url;
            return ;
        }
        func(document.object());

        manager->deleteLater();
        delete request;
        reply->deleteLater();
    });
    manager->get(*request);
}
