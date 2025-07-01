#ifndef M3U8DOWNLOADER_H
#define M3U8DOWNLOADER_H

#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include "liveservicebase.h"

class M3u8Downloader : public QObject
{
    Q_OBJECT
public:
    explicit M3u8Downloader(QObject *parent = nullptr);
    virtual ~M3u8Downloader();

    bool isDownloading() const;

    void SetLiveRoomService(LiveServiceBase* live_room_service);

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

    void UpdateUrl();
    void UpdateTsExpiresTimestamps(qint64 new_ts_record_expires_timestamps);
    void UpdateTsRecordTimestampsCurrent(qint64 new_ts_record_timestamps_current);

    bool parseUrl(QString url);

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

    // 记录当前ts录制时间戳，初始为0
    qint64 ts_record_timestamps_current = 0;
    // 录制参数里的expires
    qint64 ts_record_expires_timestamps = 0;

    LiveServiceBase* ptr_live_room_service = nullptr;
};

#endif // M3U8DOWNLOADER_H
