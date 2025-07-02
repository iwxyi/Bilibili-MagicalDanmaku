#ifndef LIVEVIDEOPLAYER_H
#define LIVEVIDEOPLAYER_H

#include <QDialog>
#include <QMediaPlayer>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QMediaPlayer>
#include <QAudioOutput>  // Qt6 需要单独的音频输出
#else
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QMediaContent>
#endif
#include <QSettings>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVideoWidget>
#include <QDir>
#include "videosurface.h"
#include "picturebrowser.h"

namespace Ui {
class LiveVideoPlayer;
}

#define CAPTURE_PARAM_FILE "params.ini"

class LiveVideoPlayer : public QDialog
{
    Q_OBJECT
public:
    explicit LiveVideoPlayer(QSettings* settings, QString dataPath, QWidget *parent = nullptr);
    ~LiveVideoPlayer() override;

public slots:
    void setRoomId(QString roomId);
    void slotLiveStart(QString roomId);
    void setPlayUrl(QString url);
    void refreshPlayUrl();
    void parsePlayUrl(const QByteArray& data);

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

    void switchFullScreen();
    void switchOnTop();

    void calcVideoRect();
    void slotSaveCurrentCapture();
    void slotSaveFrameCapture(const QPixmap& pixmap);

    void on_label_customContextMenuRequested(const QPoint &pos);

signals:
    void signalRestart();

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
    QSettings* settings;
    QString dataPath;
    QString roomId;

    QMediaPlayer* player;
    bool useVideoWidget = true;
    VideoSurface *videoSurface;
    QSize videoSize;
    bool clipCapture = false;
    int clipLeft = 0, clipTop = 0, clipRight = 0, clipBottom = 0;
    int qn = 4; // 默认原画
    QMap<int, QString> qn_desc_map; // 支持的画质

    qint64 captureMaxLong = 60000;
    QDir captureDir;

    bool enablePrevCapture = false;
    QRect videoRect;
    QTimer* captureTimer;
    bool captureRunning = false;
    int captureInterval = 100; // 每秒10帧
    qint64 prevCaptureTimestamp = 0;
    QList<QPair<qint64, QPixmap*>> *capturePixmaps = nullptr;
    Qt::TransformationMode transformation = Qt::FastTransformation;

    PictureBrowser* pictureBrowser = nullptr;
};

#endif // LIVEVIDEOPLAYER_H

