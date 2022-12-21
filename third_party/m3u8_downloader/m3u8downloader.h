#ifndef M3U8DOWNLOADER_H
#define M3U8DOWNLOADER_H

#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <QFile>

class M3u8Downloader : public QObject
{
    Q_OBJECT
public:
    explicit M3u8Downloader(QObject *parent = nullptr);
    virtual ~M3u8Downloader();

    bool isDownloading() const;

signals:
    void signalStarted();
    void signalSaved(QString file_path);
    void signalProgressChanged(qint64 size);
    void signalTimeChanged(qint64 msecond);

public slots:
    void start(QString url, QString file);
    void stop();

private:
    void startLoop();
    void getM3u8();
    void parseM3u8(const QByteArray& data);
    void downloadTs(int seq, QString url);

private slots:
    void nextSeq();

private:
    QString m3u8_url;
    QString domain_url;
    QString save_path;
    int prev_seq = 0, next_seq = 0;
    int duration = 0;
    QTimer seq_timer, second_timer;
    QEventLoop event_loop1, event_loop2;
    QFile* ts_file = nullptr;
    QList<QPair<int, QString>> ts_urls;
    qint64 start_timestamp = 0;
    qint64 total_size = 0;
};

#endif // M3U8DOWNLOADER_H
