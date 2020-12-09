#include "livevideoplayer.h"
#include "ui_livevideoplayer.h"

LiveVideoPlayer::LiveVideoPlayer(QSettings &settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LiveVideoPlayer),
    settings(settings)
{
    ui->setupUi(this);
    player = new QMediaPlayer(nullptr, QMediaPlayer::VideoSurface);
}

LiveVideoPlayer::~LiveVideoPlayer()
{
    delete ui;
}

void LiveVideoPlayer::setPlayUrl(QString url)
{
    this->playUrl = url;

    player->setVideoOutput(ui->videoWidget);
    QNetworkRequest req((QUrl(url)));
    QMediaContent c(req);
    player->setMedia(c);
    player->play();
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
