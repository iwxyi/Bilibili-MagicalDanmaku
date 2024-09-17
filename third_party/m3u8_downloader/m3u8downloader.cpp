#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QFile>
#include <QtMath>
#include <QDateTime>
#include "m3u8downloader.h"

M3u8Downloader::M3u8Downloader(QObject *parent) : QObject(parent)
{
    seq_timer.setSingleShot(true);
    connect(&seq_timer, SIGNAL(timeout()), this, SLOT(nextSeq()));

    second_timer.setSingleShot(false);
    connect(&second_timer, &QTimer::timeout, this, [=]{
        emit signalTimeChanged(QDateTime::currentMSecsSinceEpoch() - start_timestamp);
    });
}

M3u8Downloader::~M3u8Downloader()
{
    stop();
}

bool M3u8Downloader::isDownloading() const
{
    return ts_file != nullptr;
}

void M3u8Downloader::UpdateUrl()
{
    if(this->ptr_live_room_service != nullptr)
    {
        this->ptr_live_room_service->getRoomLiveVideoUrl([=](QString new_url){
            this->m3u8_url = new_url;
        });
        QString url = this->m3u8_url;

        if(!this->parseUrl(url))
        {
            return;
        }
    }
}

void M3u8Downloader::UpdateTsExpiresTimestamps(qint64 new_ts_record_expires_timestamps)
{

    this->ts_record_expires_timestamps = new_ts_record_expires_timestamps;
}

void M3u8Downloader::UpdateTsRecordTimestampsCurrent(qint64 new_ts_record_timestamps_current)
{
    if (new_ts_record_timestamps_current > this->ts_record_timestamps_current)
    {
        this->ts_record_timestamps_current = new_ts_record_timestamps_current;
    }
}

void M3u8Downloader::start(QString url, QString file)
{
    this->ts_record_expires_timestamps = 0;
    this->ts_record_timestamps_current = 0;
    qInfo() << "下载M3U8：" << url << "->" << file;

    this->save_path = file;
    if(!this->parseUrl(url))
    {
        return;
    }
    startLoop();
}

void M3u8Downloader::stop()
{
    this->ts_record_expires_timestamps = 0;
    this->ts_record_timestamps_current = 0;
    this->ptr_live_room_service = nullptr;
    if (ts_file == nullptr)
        return ;
    ts_file->flush();
    ts_file->close();
    ts_file->deleteLater();
    ts_urls.clear();
    seq_timer.stop();
    second_timer.stop();
    event_loop1.quit();
    event_loop2.quit();
    ts_file = nullptr;
    qInfo() << "停止下载M3U8";
    emit signalSaved(save_path);
}

void M3u8Downloader::startLoop()
{
    ts_file = new QFile(save_path);
    if (!ts_file->open(QFile::WriteOnly | QFile::Append))
    {
        qCritical() << "写入文件失败：" << save_path;
        return ;
    }

    start_timestamp = QDateTime::currentMSecsSinceEpoch();
    total_size = 0;
    seq_timer.setInterval(0);
    // seq_timer.start();
    second_timer.start();

    getM3u8();
}

void M3u8Downloader::getM3u8()
{
    // 预留一定时长，提前去更新url
    if (this->ts_record_timestamps_current + this->duration >= this->ts_record_expires_timestamps)
    {
        UpdateUrl();
    }
    QNetworkAccessManager manager;
    QNetworkRequest* request = new QNetworkRequest(QUrl(m3u8_url));
    QNetworkReply *reply = manager.get(*request);
    connect(reply, SIGNAL(finished()), &event_loop1, SLOT(quit()));
    event_loop1.exec();

    if (!isDownloading())
        return ;
    int http_status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    // qDebug() << "meu8.file:" << data;

    delete reply;
    delete request;
    // 这里的话 感觉应该用http的响应码来判断获取是否成功。2xx的视为成功
    if (http_status_code >= 200 && http_status_code <300)
    {
        parseM3u8(data);
    }
    else
    {
        UpdateUrl();
    }
    /* 在parseM3u8里 启动了下一次下载数据的seq_timer
     * 但是如果这一次获取data的时候，就失败了，那data就是空的，parseM3u8里当做数据正常去解析
     * 这会导致next_seq prev_seq 数据是异常的
     * 如果可以的话，能不能把seq_timer拿到这里
     * 因为seq_timer启动的是nextSeq()方法，而nextSeq()也就是重复getM3u8
    */
    seq_timer.start();
}

void M3u8Downloader::parseM3u8(const QByteArray &data)
{
    int find_pos = 0;
    QByteArray s = "#EXTM3U";
    find_pos = data.indexOf(s) + s.length();

    s = "#EXT-X-MEDIA-SEQUENCE:";
    find_pos = data.indexOf(s, find_pos) + s.length();
    int line_pos = data.indexOf("\n", find_pos);
    QString val_s = data.mid(find_pos, line_pos - find_pos);
    next_seq = val_s.toInt();

    s = "#EXT-X-TARGETDURATION:";
    find_pos = data.indexOf(s, find_pos) + s.length();
    line_pos = data.indexOf("\n", find_pos);
    val_s = data.mid(find_pos, line_pos - find_pos);
    duration = val_s.toInt();

    while (find_pos != -1)
    {
        // 解析#EXT-X-PROGRAM-DATE-TIME:
        s = "#EXT-X-PROGRAM-DATE-TIME:";
        find_pos = data.indexOf(s, find_pos) + s.length();
        line_pos = data.indexOf("\n", find_pos);
        QString str_data_time = data.mid(find_pos, line_pos - find_pos);
        QDateTime date_time = QDateTime::fromString(str_data_time, Qt::ISODate);

        s = "#EXTINF:";
        find_pos = data.indexOf(s, find_pos) + s.length();
        line_pos = data.indexOf("\n", find_pos);
        QString desc = data.mid(find_pos, line_pos - find_pos);
        int comma_pos = desc.indexOf(",");
        if (comma_pos != -1)
        {
            QString str_one_fragment_duration = desc.left(comma_pos);

            double double_one_fragment_duration = str_one_fragment_duration.toDouble();
            qint64 one_fragment_duration(double_one_fragment_duration);
            if (date_time.isValid()) {
                qint64 current_timestamps = date_time.toTime_t();
                qint64 video_update_timestamps = current_timestamps + one_fragment_duration;
                UpdateTsRecordTimestampsCurrent(video_update_timestamps);
            }
        }
        find_pos = line_pos + 1;
        line_pos = data.indexOf("\n", find_pos);
        QString ts_url = data.mid(find_pos, line_pos - find_pos);
        if (!ts_url.startsWith("http"))
            ts_url = domain_url + (domain_url.endsWith("/") ? "" : "/") + ts_url;

        // 下一个
        // 看了下m3u8的协议，下一个的开头应该得是#EXT-X-PROGRAM-DATE-TIME:
        s = "#EXT-X-PROGRAM-DATE-TIME:";
        find_pos = data.indexOf(s, find_pos);

        // qDebug() << "    seq:" << next_seq << ", duration:" << duration << desc << ts_url;
        if (next_seq++ <= prev_seq)
            continue ;
        downloadTs(next_seq - 1, ts_url);
    }
    prev_seq = next_seq - 1;
}

void M3u8Downloader::downloadTs(int seq, QString url)
{
    if (event_loop2.isRunning())
    {
        // qDebug() << "        que " << seq;
        ts_urls.append(qMakePair(seq, url));
        return ;
    }

    QNetworkAccessManager manager;
    QNetworkRequest* request = new QNetworkRequest(QUrl(url));
    QNetworkReply *reply = manager.get(*request);
    connect(reply, SIGNAL(finished()), &event_loop2, SLOT(quit()));
    event_loop2.exec();

    // 这里可能会结束录制，如果再保存文件会出错
    if (!isDownloading())
        return ;

    QByteArray data = reply->readAll();
    // qDebug() << "        " << seq << "ts.file: +" << data.size()/1024 << "KB";
    ts_file->write(data);

    delete reply;
    delete request;

    total_size += data.size();
    emit signalProgressChanged(total_size);

    if (ts_urls.size())
    {
        auto pair = ts_urls.takeFirst();
        downloadTs(pair.first, pair.second);
    }
}

void M3u8Downloader::nextSeq()
{
    getM3u8();
}

void M3u8Downloader::SetLiveRoomService(LiveRoomService* live_room_service)
{
    this->ptr_live_room_service = live_room_service;
}

bool M3u8Downloader::parseUrl(QString url)
{
    qInfo() << "下载M3U8：" << url;
    this->m3u8_url = url;

    int pos = url.indexOf("://");
    pos += 3;
    pos = url.lastIndexOf("/");
    if (pos > 0)
    {
        this->domain_url = url.left(pos);
    }
    else
    {
        qWarning() << "无法获取域名：" << url;
        return false;
    }

    QUrl qurl(url);

    if (qurl.isValid())
    {
        QUrlQuery qurl_query(qurl);
        if (qurl_query.hasQueryItem("expires"))
        {
            QString str_expires = qurl_query.queryItemValue("expires");
            qint64 new_ts_expires_timestamps = str_expires.toLongLong();
            UpdateTsExpiresTimestamps(new_ts_expires_timestamps);
        }
    }
    return true;
}
