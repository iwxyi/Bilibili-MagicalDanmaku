#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDebug>
#include <QFile>
#include <QtMath>
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

void M3u8Downloader::start(QString url, QString file)
{
    qInfo() << "下载M3U8：" << url << "->" << file;
    this->m3u8_url = url;
    this->save_path = file;

    int pos = url.indexOf("://");
    pos += 3;
    pos = url.indexOf("/", pos);
    if (pos > 0)
    {
        this->domain_url = url.left(pos);
    }
    else
    {
        qWarning() << "无法获取域名：" << url;
        return;
    }

    startLoop();
}

void M3u8Downloader::stop()
{
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
    QNetworkAccessManager manager;
    QNetworkRequest* request = new QNetworkRequest(QUrl(m3u8_url));
    QNetworkReply *reply = manager.get(*request);
    connect(reply, SIGNAL(finished()), &event_loop1, SLOT(quit()));
    event_loop1.exec();

    if (!isDownloading())
        return ;

    QByteArray data = reply->readAll();
    // qDebug() << "meu8.file:" << data;

    delete reply;
    delete request;

    parseM3u8(data);
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
        s = "#EXTINF:";
        find_pos = data.indexOf(s, find_pos) + s.length();
        line_pos = data.indexOf("\n", find_pos);
        QString desc = data.mid(find_pos, line_pos - find_pos);

        find_pos = line_pos + 1;
        line_pos = data.indexOf("\n", find_pos);
        QString ts_url = data.mid(find_pos, line_pos - find_pos);
        if (!ts_url.startsWith("http"))
            ts_url = domain_url + ts_url;

        // 下一个
        find_pos = data.indexOf(s, find_pos);

        // qDebug() << "    seq:" << next_seq << ", duration:" << duration << desc << ts_url;
        if (next_seq++ <= prev_seq)
            continue ;
        downloadTs(next_seq - 1, ts_url);
    }
    prev_seq = next_seq - 1;
    seq_timer.start();
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
