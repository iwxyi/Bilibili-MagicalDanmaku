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
#include <QDir>

namespace Ui {
class LiveVideoPlayer;
}

#define newFacileMenu FacileMenu *menu = new FacileMenu(this)

#define CAPTURE_PARAM_FILE "params.ini"

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
    void processFrame(QVideoFrame frame);

protected:
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void on_videoWidget_customContextMenuRequested(const QPoint&);

    void on_playButton_clicked();

    void on_saveCapture1Button_clicked();

    void on_saveCapture5sButton_clicked();

    void on_saveCapture13sButton_clicked();

    void on_saveCapture30sButton_clicked();

    void on_saveCapture60sButton_clicked();

    void calcVideoRect();
    void slotSaveCurrentCapture();

private:
    void startCapture();
    void stopCapture(bool clear = false);
    void saveCapture();
    void saveCapture(int second);
    void showCaptureButtons(bool show);

    QString timeToPath(const qint64& time);
    QString timeToFileName(const qint64& time);

private:
    Ui::LiveVideoPlayer *ui;
    QSettings& settings;
    QString roomId;
    QMediaPlayer* player;
    QVideoProbe *probe;

    qint64 captureMaxLong = 600000;
    QDir captureDir;

    bool enablePrevCapture = false;
    QRect videoRect;
    QTimer* captureTimer;
    QList<QPair<qint64, QPixmap*>> *capturePixmaps = nullptr;
};

#endif // LIVEVIDEOPLAYER_H

