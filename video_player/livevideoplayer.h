#ifndef LIVEVIDEOPLAYER_H
#define LIVEVIDEOPLAYER_H

#include <QDialog>
#include <QMediaPlayer>
#include <QMediaContent>
#include <QNetworkRequest>
namespace Ui {
class LiveVideoPlayer;
}

class LiveVideoPlayer : public QDialog
{
    Q_OBJECT
public:
    explicit LiveVideoPlayer(QWidget *parent = nullptr);
    ~LiveVideoPlayer();

public slots:
    void setPlayUrl(QString url);

private:
    Ui::LiveVideoPlayer *ui;
    QString playUrl;
    QMediaPlayer* player;
};

#endif // LIVEVIDEOPLAYER_H

