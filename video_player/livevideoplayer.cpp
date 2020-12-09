#include "livevideoplayer.h"
#include "ui_livevideoplayer.h"

LiveVideoPlayer::LiveVideoPlayer(QSettings &settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LiveVideoPlayer),
    settings(settings)
{
    ui->setupUi(this);
    setModal(false);
    player = new QMediaPlayer(nullptr, QMediaPlayer::VideoSurface);
    playList = new QMediaPlaylist;
    player->setPlaylist(playList);
    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state) {
        qDebug() << "videoPlayer.stateChanged:" << state << QDateTime::currentDateTime();
        if (state == QMediaPlayer::StoppedState) // 播放完了
        {
        }
    });
}

LiveVideoPlayer::~LiveVideoPlayer()
{
    delete ui;
}

void LiveVideoPlayer::setRoomId(QString roomId)
{
    this->roomId = roomId;
    refreshPlayUrl();
}

void LiveVideoPlayer::addPlayUrl(QString url)
{
    this->playUrl = url;

    player->setVideoOutput(ui->videoWidget);
    QNetworkRequest req((QUrl(url)));
    QMediaContent c(req);
    playList->addMedia(c);
//    player->setMedia(c);
//    player->play();
    if (player->state() == QMediaPlayer::StoppedState)
        player->play();
}

void LiveVideoPlayer::refreshPlayUrl()
{
    if (roomId.isEmpty())
        return ;
    QString url = "http://api.live.bilibili.com/room/v1/Room/playUrl?cid=" + roomId
            + "&quality=4&qn=10000";
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 0)
        {
            qDebug() << ("返回结果不为0：") << json.value("message").toString();
            return ;
        }

        // 获取链接
        QJsonArray array = json.value("data").toObject().value("durl").toArray();
        QString url = array.first().toObject().value("url").toString(); // 第一个链接
        for (int i = 0; i < array.size(); i++)
        {
            QString url = array.at(i).toObject().value("url").toString();
            addPlayUrl(url);
        }
    });
    manager->get(*request);
}

void LiveVideoPlayer::showEvent(QShowEvent *)
{
    restoreGeometry(settings.value("videoplayer/geometry").toByteArray());

    if (!playUrl.isEmpty() && player)
    {
        player->play();
    }
}

void LiveVideoPlayer::hideEvent(QHideEvent *)
{
    settings.setValue("videoplayer/geometry", this->saveGeometry());

    if (player->state() == QMediaPlayer::PausedState)
    {
        player->play();
    }
}
