#ifndef VIDEOLYRICSCREATOR_H
#define VIDEOLYRICSCREATOR_H

#include <QMainWindow>
#include <QSettings>
#include <QJsonArray>

namespace Ui {
class VideoLyricsCreator;
}

class VideoLyricsCreator : public QMainWindow
{
    Q_OBJECT
public:
    explicit VideoLyricsCreator(QWidget *parent = nullptr);
    ~VideoLyricsCreator();

private slots:
    void on_sendButton_clicked();

    void on_actionSet_Lyrics_Danmaku_Sample_triggered();

    void on_actionSet_Sample_Help_triggered();

    void on_actionSet_Cookie_triggered();

    void on_urlEdit_editingFinished();

    void on_listView_clicked(const QModelIndex &index);

    void sendNextLyric();

private:
    void getVideoInfo(QString key, QString id);
    void sendLyrics(qint64 time, QString text);
    void setSendingState(bool state);
    QVariant getCookies();

private:
    Ui::VideoLyricsCreator *ui;
    QSettings settings;

    // 用户配置
    QString userCookie;
    QString lyricsSample;

    // 视频信息
    QJsonArray videoPages;

    // 发送中
    bool sending = false;
    QTimer* sendTimer = nullptr;
    QString sendSample; // 要发送的内容，只有时间和文字不一样
    QString videoAVorBV;
    QString videoId;
    QString videoCid;
    int pageIndex = 0;
    QStringList lyricList;
    QStringList failedList;
    qint64 offsetMSecond = 0;
    int totalSending = 0;
};

#endif // VIDEOLYRICSCREATOR_H
