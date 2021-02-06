#ifndef RESIZABLEPICTURE_H
#define RESIZABLEPICTURE_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QMovie>

class ResizablePicture : public QWidget
{
    Q_OBJECT
public:
    ResizablePicture(QWidget *parent = nullptr);

    bool setGif(QString path);
    bool setPixmap(const QPixmap& pixmap);
    void unbindMovie();
    void resetScale();

    void setScaleCache(bool enable);
    void scaleTo(double scale, QPoint pos);
    void scaleToFill();
    void scaleToOrigin();

    void setResizeAutoInit(bool i);

    const QPixmap &getOriginPixmap();

    void getClipArea(QSize& originSize, QRect& imageArea, QRect &showArea);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *) override;

    qint64 sizeToLL(QSize size);

signals:

public slots:

private:
    QLabel* label;
    QMovie* movie = nullptr;

    bool resizeAutoInit = true;

    QPoint pressPos;
    QPixmap originPixmap;
    QPixmap currentPixmap;
    bool scaleCacheEnabled = true;
    QHash<qint64, QRect> scaleCache; // 缓存相同大小的图片位置信息（相同大小的图片都只看同一部分区域）
};

#endif // RESIZABLEPICTURE_H
