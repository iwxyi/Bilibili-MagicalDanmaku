#ifndef LIVEVIDEOPLAYER_H
#define LIVEVIDEOPLAYER_H

#include <QDialog>
#include <QMediaPlayer>
#include <QMediaContent>
#include <QNetworkRequest>
#include <QSettings>
namespace Ui {
class LiveVideoPlayer;
}

class LiveVideoPlayer : public QDialog
{
    Q_OBJECT
public:
    explicit LiveVideoPlayer(QSettings& settings, QWidget *parent = nullptr);
    ~LiveVideoPlayer() override;

public slots:
    void setPlayUrl(QString url);

protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;

private:
    Ui::LiveVideoPlayer *ui;
    QSettings& settings;
    QString playUrl;
    QMediaPlayer* player;
};

#endif // LIVEVIDEOPLAYER_H

