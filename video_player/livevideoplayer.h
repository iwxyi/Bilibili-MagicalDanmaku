#ifndef LIVEVIDEOPLAYER_H
#define LIVEVIDEOPLAYER_H

#include <QDialog>
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
    void setRoomId(QString roomId);
    void setPlayUrl(QString url);
    void refreshPlayUrl();

private:
    void downloadFlv(QString url);

protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;

private:
    Ui::LiveVideoPlayer *ui;
    QSettings& settings;
    QString playUrl;
    QString roomId;
    QMediaPlayer* player;
    QMediaPlaylist* playList;
};

#endif // LIVEVIDEOPLAYER_H

