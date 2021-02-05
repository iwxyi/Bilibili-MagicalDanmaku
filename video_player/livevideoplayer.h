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

protected:
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void on_videoWidget_customContextMenuRequested(const QPoint&);

    void on_playButton_clicked();

    void on_saveCapture5sButton_clicked();

    void on_saveCapture13sButton_clicked();

    void on_saveCapture30sButton_clicked();

    void on_saveCapture60sButton_clicked();

private:
    void startCapture();
    void stopCapture(bool clear = false);
    void saveCapture(int second);

private:
    Ui::LiveVideoPlayer *ui;
    QSettings& settings;
    QString roomId;
    QMediaPlayer* player;
    QVideoProbe *probe;

    QTimer* captureTimer;
    QList<QPair<qint64, QPixmap*>> capturePixmaps;
};

#endif // LIVEVIDEOPLAYER_H

