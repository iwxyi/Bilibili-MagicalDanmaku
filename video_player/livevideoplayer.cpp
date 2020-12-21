#include "livevideoplayer.h"
#include "ui_livevideoplayer.h"

LiveVideoPlayer::LiveVideoPlayer(QSettings &settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LiveVideoPlayer),
    settings(settings)
{
    ui->setupUi(this);
    setModal(false);
    player = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
    connect(player, &QMediaPlayer::positionChanged, this, [=](qint64 position){
//        qDebug() << "position changed:" << position << player->duration();
    });

    playList = new QMediaPlaylist;
    player->setPlaylist(playList);
    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state) {
//        qDebug() << "videoPlayer.stateChanged:" << state << QDateTime::currentDateTime() << playList->currentIndex() << playList->mediaCount();
        if (state == QMediaPlayer::StoppedState) // 播放完了
        {
            if (playList->mediaCount())
            {
                playList->setCurrentIndex(0);
                player->play();
            }
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

void LiveVideoPlayer::setPlayUrl(QString url)
{
    this->playUrl = url;

    player->setVideoOutput(ui->videoWidget);
    QNetworkRequest req((QUrl(url)));
    QMediaContent c(req);
    playList->clear();
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
            + "&quality=4&qn=10000&platform=web&otype=json";
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
        manager->deleteLater();
        delete request;

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
        playUrl = url;
        playList->clear();
        setPlayUrl(url);
        /*for (int i = 0; i < array.size(); i++)
        {
            QString url = array.at(i).toObject().value("url").toString();
            addPlayUrl(url);
        }*/
    });
    manager->get(*request);
}

void LiveVideoPlayer::downloadFlv(QString url)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray baData = reply->readAll();
        manager->deleteLater();
        delete request;

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(baData, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 200)
        {
            qDebug() << ("返回结果不为200：") << json.value("message").toString();
            return ;
        }
/*
        QJsonArray array = json.value("data").toArray();
        if (!array.size())
        {
            qDebug() << "未找到歌曲：" << song.simpleString();
            downloadingSong = Song();
            downloadNext();
            return ;
        }

        json = array.first().toObject();
        QString url = JVAL_STR(url);
        int br = JVAL_INT(br); // 比率320000
        int size = JVAL_INT(size);
        QString type = JVAL_STR(type); // mp3
        QString encodeType = JVAL_STR(encodeType); // mp3
        qDebug() << "    信息：" << br << size << type << url;
        if (size == 0)
        {
            qDebug() << "无法下载，可能没有版权" << song.simpleString();
            if (playAfterDownloaded == song)
            {
                if (orderSongs.contains(song))
                {
                    orderSongs.removeOne(song);
                }
                playNext();
            }

            downloadingSong = Song();
            downloadNext();
            return ;
        }

        // 开始下载歌曲本身
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply1 = manager.get(QNetworkRequest(QUrl(url)));
        //请求结束并下载完成后，退出子事件循环
        connect(reply1, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        //开启子事件循环
        loop.exec();
        QByteArray mp3Ba = reply1->readAll();

        // 保存到文件
        QFile file(songPath(song));
        file.open(QIODevice::WriteOnly);
        file.write(mp3Ba);
        file.flush();
        file.close();

        emit signalSongDownloadFinished(song);

        if (playAfterDownloaded == song)
            playLocalSong(song);

        downloadingSong = Song();
        downloadNext();
        */
    });
    manager->get(*request);
}

void LiveVideoPlayer::showEvent(QShowEvent *)
{
    restoreGeometry(settings.value("videoplayer/geometry").toByteArray());

    if (!playUrl.isEmpty() && player)
    {
        player->setVideoOutput(ui->videoWidget);
        player->play();
    }
}

void LiveVideoPlayer::hideEvent(QHideEvent *)
{
    settings.setValue("videoplayer/geometry", this->saveGeometry());

    if (player->state() == QMediaPlayer::PlayingState)
    {
        player->pause();
    }
    this->deleteLater();
}
