#include "livevideoplayer.h"
#include "ui_livevideoplayer.h"

LiveVideoPlayer::LiveVideoPlayer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LiveVideoPlayer)
{
    ui->setupUi(this);
    player = new QMediaPlayer(0, QMediaPlayer::VideoSurface);
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
