#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QStyle>
#include "livevideoplayer.h"
#include "ui_livevideoplayer.h"
#include "facilemenu.h"

LiveVideoPlayer::LiveVideoPlayer(QSettings &settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LiveVideoPlayer),
    settings(settings)
{
    ui->setupUi(this);
    setModal(false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    int uw = qMax(style()->pixelMetric(QStyle::PM_TitleBarHeight), 32);
    ui->playButton->setFixedSize(uw, uw);
    ui->saveCapture5sButton->setFixedSize(uw, uw);
    ui->saveCapture13sButton->setFixedSize(uw, uw);
    ui->saveCapture30sButton->setFixedSize(uw, uw);
    ui->saveCapture60sButton->setFixedSize(uw, uw);

    player = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
    connect(player, &QMediaPlayer::positionChanged, this, [=](qint64 position){
        // 直播的话，一般都是 position=0, duration=0
        // qDebug() << "position changed:" << position << player->duration();
    });
    player->setVideoOutput(ui->videoWidget);

    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state) {
        if (state == QMediaPlayer::PlayingState)
        {
            ui->playButton->setIcon(QIcon(":/icons/pause"));
        }
        else if (state == QMediaPlayer::PausedState)
        {
            ui->playButton->setIcon(QIcon(":/icons/play"));
        }
        // qDebug() << "videoPlayer.stateChanged:" << state << QDateTime::currentDateTime();
    });
    player->setVolume(settings.value("videoplayer/volume", 50).toInt());
    if (settings.value("videoplayer/fullScreen", false).toBool())
        ui->videoWidget->setFullScreen(true);

    probe = new QVideoProbe(this);
    connect(probe, SIGNAL(videoFrameProbed(const QVideoFrame &)), this, SLOT(processFrame(const QVideoFrame &)));
    probe->setSource(player);
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

void LiveVideoPlayer::slotLiveStart(QString roomId)
{
    if (this->roomId != roomId)
        return ;
    QTimer::singleShot(500, [=]{
        refreshPlayUrl();
    });
}

void LiveVideoPlayer::setPlayUrl(QString url)
{
    player->setMedia(QMediaContent((QUrl(url))));
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
        reply->deleteLater();

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
        if (!array.size())
        {
            QMessageBox::warning(this, "播放视频", "未找到可用的链接\n" + data);
            return ;
        }
        QString url = array.first().toObject().value("url").toString(); // 第一个链接
        qDebug() << "playUrl:" << url;
        setPlayUrl(url);
    });
    manager->get(*request);
}

void LiveVideoPlayer::processFrame(const QVideoFrame &frame)
{
    qDebug() << "~~~~~~~~~~~~~~~~~~~~~~~~~~~";
    qDebug() << frame.size();
}

void LiveVideoPlayer::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
    restoreGeometry(settings.value("videoplayer/geometry").toByteArray());

    if (!player->media().isNull())
    {
        player->play();
    }
}

void LiveVideoPlayer::hideEvent(QHideEvent *e)
{
    QDialog::hideEvent(e);
    settings.setValue("videoplayer/geometry", this->saveGeometry());

    if (player->state() == QMediaPlayer::PlayingState)
    {
        player->pause();
    }
}

void LiveVideoPlayer::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);
}

void LiveVideoPlayer::on_videoWidget_customContextMenuRequested(const QPoint&)
{
    newFacileMenu;
    menu->addAction(QIcon(), "全屏", [=]{
        bool fullScreen = !ui->videoWidget->isFullScreen();
        ui->videoWidget->setFullScreen(fullScreen);
        settings.setValue("videoplayer/fullScreen", fullScreen);
    })->check(ui->videoWidget->isFullScreen());

    FacileMenu* volumeMenu = menu->split()->addMenu(QIcon(":/icons/dotdotdot"), "音量");
    volumeMenu->addNumberedActions("%1", 0, 110, [=](FacileMenuItem* item, int val){
        item->check(val == (player->volume()+5) / 10 * 10);
    }, [=](int vol){
        settings.setValue("videoplayer/volume", vol);
        player->setVolume(vol);
    }, 10);

    menu->addAction("静音", [=]{
        if (player->volume())
            player->setVolume(0);
        else
            player->setVolume(settings.value("videoplayer/volume", 50).toInt());
    })->check(!player->volume());

    menu->exec();
}

void LiveVideoPlayer::on_playButton_clicked()
{
    if (player->state() == QMediaPlayer::PlayingState)
        player->pause();
    else
        player->play();
}

void LiveVideoPlayer::on_saveCapture5sButton_clicked()
{

}

void LiveVideoPlayer::on_saveCapture13sButton_clicked()
{

}

void LiveVideoPlayer::on_saveCapture30sButton_clicked()
{

}

void LiveVideoPlayer::on_saveCapture60sButton_clicked()
{

}
