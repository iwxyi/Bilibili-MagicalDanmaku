#ifndef M3U8DOWNLOADER_H
#define M3U8DOWNLOADER_H

#include <QObject>

class M3u8Downloader : public QObject
{
    Q_OBJECT
public:
    explicit M3u8Downloader(QObject *parent = nullptr);

signals:

public slots:
};

#endif // M3U8DOWNLOADER_H
