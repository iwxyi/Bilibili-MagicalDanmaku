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
    inRooms.clear();
    ui->tableWidget->setRowCount(0);

    if (userId.isEmpty())
        return ;

    currentTaskTs = QDateTime::currentMSecsSinceEpoch();

    getUserFollows(currentTaskTs, userId);
}

void CatchYouWidget::setDefaultUser(QString userId)
{
    this->userId = userId;
}

void CatchYouWidget::on_userButton_clicked()
{
    bool ok = false;
    QString id = QInputDialog::getText(this, "输入用户Id", "请输入要抓的用户Id", QLineEdit::Normal, userId, &ok);
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
        if (currentTaskTs != taskTs)
            return ;

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
            qDebug() << "关注总数：" << total << "可见总数：" << users.size();
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
        if (currentTaskTs != taskTs)
            return ;

        if (json.value("code").toInt() != 0)
        {
            qCritical() << "检测直播状态失败：" << json.value("message").toString() << user.userId << user.userName;
            return ;
        }

        QJsonObject data = json.value("data").toObject();
        int roomStatus = data.value("roomStatus").toInt(); // 1
        int roundStatus = data.value("roundStatus").toInt(); // 1
        int liveStatus = data.value("liveStatus").toInt(); // 1
        if (roomStatus && liveStatus) // 直播中，需要检测
        {
            QString roomId = QString::number(qint64(data.value("roomid").toDouble()));

            // 先检测弹幕
            detectRoomDanmaku(taskTs, roomId);
        }

        // 检测下一个
        ui->progressBar->setValue(index);
        detectUserLiveStatus(taskTs, index + 1);
    });
}

void CatchYouWidget::detectRoomDanmaku(qint64 taskTs, QString roomId)
{
    get("https://api.live.bilibili.com/ajax/msg?roomid=" + roomId, [=](QJsonObject json) {
        if (currentTaskTs != taskTs)
            return ;
        if (json.value("code").toInt() != 0)
        {
            qCritical() << "检测房间弹幕失败" << json.value("message").toString() << roomId;
            return ;
        }

        QJsonObject data = json.value("data").toObject();

        qint64 uid = userId.toLongLong();
        qint64 time = 0;
        QStringList texts;
        auto each = [&](QJsonObject danmaku) {
            if (qint64(danmaku.value("uid").toDouble()) == uid)
            {
                // 是这个用户的
                qint64 t = QDateTime::fromString(danmaku.value("timeline").toString(), "yyyy-MM-dd hh:mm:ss").toSecsSinceEpoch();
                if (t > time)
                    time = t;
                QString text = danmaku.value("text").toString();
                texts.append(text);
            }
        };
        foreach (QJsonValue val, data.value("admin").toArray())
        {
            each(val.toObject());
        }
        foreach (QJsonValue val, data.value("room").toArray())
        {
            each(val.toObject());
        }

        if (time) // 有弹幕
        {
            qDebug() << "检测到弹幕：" << roomId << texts;
            addInRoom(taskTs, roomId, time, texts);
        }
        else // 没有弹幕，检测送礼榜
        {
            detectRoomGift(taskTs, roomId);
        }
    });
}

void CatchYouWidget::addInRoom(qint64 taskTs, QString roomId, qint64 second, QStringList texts)
{
    get("https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByRoom?room_id=" + roomId, [=](QJsonObject json){
        if (currentTaskTs != taskTs)
            return ;

        if (json.value("code").toInt() != 0)
        {
            qCritical() << "获取房间信息失败" << json.value("message").toString() << roomId;
            return ;
        }
        QJsonObject dataObj = json.value("data").toObject();
        QJsonObject roomInfo = dataObj.value("room_info").toObject();
        QJsonObject anchorInfo = dataObj.value("anchor_info").toObject();

        qint64 roomId = roomInfo.value("room_id").toInt();
        QString roomTitle = roomInfo.value("title").toString();
        qint64 upUid = static_cast<qint64>(roomInfo.value("uid").toDouble());
        QString upName = anchorInfo.value("base_info").toObject().value("uname").toString();

        RoomInfo room;
        room.roomId = roomId;
        room.roomName = roomTitle;
        room.upUid = upUid;
        room.upName = upName;
        room.time = second;
        room.danmakus = texts;
        inRooms.append(room);

        addToTable(room);
    });
}

void CatchYouWidget::detectRoomGift(qint64 taskTs, QString roomId)
{

}

void CatchYouWidget::addToTable(CatchYouWidget::RoomInfo room)
{
    auto table = ui->tableWidget;
    int row = inRooms.size();
    table->setRowCount(row--);
    table->setItem(row, 0, new QTableWidgetItem(QString::number(room.roomId)));
    table->setItem(row, 1, new QTableWidgetItem(room.roomName));
    table->setItem(row, 2, new QTableWidgetItem(room.upName));
    table->setItem(row, 3, new QTableWidgetItem(room.time ? QDateTime::fromSecsSinceEpoch(room.time).toString("hh:mm") : ""));

    auto item = new QTableWidgetItem(room.danmakus.size() ? room.danmakus.last() : "");
    item->setToolTip(room.danmakus.join("\n"));
    table->setItem(row, 4, item);
}

void CatchYouWidget::get(QString url, std::function<void(QJsonObject)> const func)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    if (url.contains("bilibili.com"))
        request->setHeader(QNetworkRequest::CookieHeader, userCookies);
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
