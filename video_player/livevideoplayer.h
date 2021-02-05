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
#include <QVideoWidget>
#include <QVideoProbe>

namespace Ui {
class LiveVideoPlayer;
}

#define newFacileMenu FacileMenu *menu = new FacileMenu(this)

class LiveVideoPlayer : public QDialog
{
    Q_OBJECT
public:
    explicit LiveVideoPlayer(QSettings& settings, QWidget *parent = nullptr);
    ~LiveVideoPlayer() override;

public slots:
    void setRoomId(QString roomId);
    void slotLiveStart(QString roomId);
    void setPlayUrl(QString url);
    void refreshPlayUrl();
    void processFrame(const QVideoFrame &frame);

signals:
    void signalPlayUrl(QString url);

protected:
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void on_videoWidget_customContextMenuRequested(const QPoint&);

private:
    Ui::LiveVideoPlayer *ui;
    QSettings& settings;
    QString roomId;
    QMediaPlayer* player;
    QVideoProbe *probe;
};

#endif // LIVEVIDEOPLAYER_H

