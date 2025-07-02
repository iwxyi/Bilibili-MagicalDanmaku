#ifndef VIDEOSURFACE_H
#define VIDEOSURFACE_H

#include <QObject>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QAbstractVideoSurface>

class VideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    VideoSurface(QObject *parent = Q_NULLPTR);
    ~VideoSurface();

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;
    bool present(const QVideoFrame &frame);

signals:
    void frameAvailable(QVideoFrame &frame);

};
#else
#include <QVideoSink>
#include <QVideoFrame>

class VideoSurface : public QObject
{
    Q_OBJECT
public:
    explicit VideoSurface(QObject *parent = nullptr);

    // 获取视频接收器，用于连接到媒体播放器/摄像头
    QVideoSink* videoSink() const { return m_videoSink; }

signals:
    // 当新帧可用时发出信号
    void frameAvailable(QVideoFrame &frame);

private:
    QVideoSink *m_videoSink;

private slots:
    void handleVideoFrame(const QVideoFrame &frame);
};

#endif

#endif // VIDEOSURFACE_H
