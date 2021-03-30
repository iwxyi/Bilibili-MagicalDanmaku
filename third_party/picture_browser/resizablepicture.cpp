#include "resizablepicture.h"

ResizablePicture::ResizablePicture(QWidget *parent) : QWidget(parent)
{
    label = new QLabel(this);
    label->show();
    resetScale();
}

bool ResizablePicture::setGif(QString path)
{
    unbindMovie();
    movie = new QMovie(this);

    // 保存原先的位置
    if (!originPixmap.isNull())
    {
       scaleCache[sizeToLL(originPixmap.size())] = label->geometry();
    }

    // 加载图片
    movie->setFileName(path); // 会绑定gif，导致无法删除或者提取
    label->setMovie(movie);
    movie->start();

    originPixmap = movie->currentPixmap();

    // 恢复相同的位置
    if (scaleCacheEnabled)
    {
        QSize size = originPixmap.size();
        if (scaleCache.contains(sizeToLL(size)))
        {
            QRect rect = scaleCache.value(sizeToLL(size));
            currentPixmap = originPixmap.scaled(rect.size());
            if (movie)
                movie->setScaledSize(rect.size());
            label->setGeometry(rect);
            if (!movie)
                label->setPixmap(currentPixmap);
            return true;
        }
    }

    resetScale();
    return true;
}

bool ResizablePicture::setPixmap(const QPixmap &pixmap)
{
    unbindMovie();

    // 保存原先的位置
    if (!originPixmap.isNull())
    {
       scaleCache[sizeToLL(originPixmap.size())] = label->geometry();
    }

    // 加载图片
    originPixmap = pixmap;
    if (originPixmap.isNull())
        return false;

    // 恢复相同的位置
    if (scaleCacheEnabled)
    {
        QSize size = originPixmap.size();
        if (scaleCache.contains(sizeToLL(size)))
        {
            QRect rect = scaleCache.value(sizeToLL(size));
            currentPixmap = originPixmap.scaled(rect.size());
            if (movie)
                movie->setScaledSize(rect.size());
            label->setGeometry(rect);
            if (!movie)
                label->setPixmap(currentPixmap);
            return true;
        }
    }

    resetScale();

    return true;
}

void ResizablePicture::unbindMovie()
{
    if (movie)
    {
        movie->setFileName("");
        label->setMovie(nullptr);
        delete movie;
        movie = nullptr;
    }
}

void ResizablePicture::resetScale()
{
    if (originPixmap.isNull())
    {
        label->setGeometry(this->rect());
        return ;
    }

    // 缩小大图片大小
    if (originPixmap.width() > width() || originPixmap.height() >= height())
        currentPixmap = originPixmap.scaled(size(), Qt::AspectRatioMode::KeepAspectRatio);
    else
        currentPixmap = originPixmap;
    if (movie)
        movie->setScaledSize(currentPixmap.size());

    // label缩小到pixmap的大小
    label->resize(currentPixmap.size());
    label->move((width()-label->width())/2, (height()-label->height())/2);
    if (!movie)
        label->setPixmap(currentPixmap);
}

void ResizablePicture::setScaleCache(bool enable)
{
    this->scaleCacheEnabled = enable;
}

void ResizablePicture::scaleTo(double scale, QPoint pos)
{
    // 根据不同的位置进行缩放
    double xProp = double(pos.x() - label->x()) / label->width();
    double yProp = double(pos.y() - label->y()) / label->height();
    label->move(static_cast<int>(label->x() - label->width()*xProp*(scale-1)),
                static_cast<int>(label->y() - label->height()*yProp*(scale-1)));
    currentPixmap = originPixmap.scaled(static_cast<int>(label->width() * scale),
                                        static_cast<int>(label->height() * scale));
    if (movie)
        movie->setScaledSize(currentPixmap.size());
    label->resize(currentPixmap.size());
    if (!movie)
        label->setPixmap(currentPixmap);

    scaleCache[sizeToLL(originPixmap.size())] = label->geometry();
}

/**
 * 缩放到填充屏幕（短边）
 */
void ResizablePicture::scaleToFill()
{
    if (originPixmap.isNull())
        return ;

    currentPixmap = originPixmap.scaled(size(), Qt::AspectRatioMode::KeepAspectRatio);
    if (movie)
        movie->setScaledSize(currentPixmap.size());

    label->resize(currentPixmap.size());
    label->move((width()-label->width())/2, (height()-label->height())/2);
    if (!movie)
        label->setPixmap(currentPixmap);
}

/**
 * 缩放到图片原始大小
 */
void ResizablePicture::scaleToOrigin()
{
    if (originPixmap.isNull())
        return ;

    currentPixmap = originPixmap;
    label->resize(currentPixmap.size());
    label->move((width()-label->width())/2, (height()-label->height())/2);
    if (!movie)
        label->setPixmap(currentPixmap);
    else
        movie->setScaledSize(currentPixmap.size());
}

void ResizablePicture::setResizeAutoInit(bool i)
{
    this->resizeAutoInit = i;
}

const QPixmap& ResizablePicture::getOriginPixmap()
{
    return originPixmap;
}

/**
 * 获取当前显示的位置，用于裁剪
 * @param size 原图片大小
 * @param imageArea 当前图片的位置
 * @param showArea 当前显示出来的部分
 */
void ResizablePicture::getClipArea(QSize &originSize, QRect &imageArea, QRect& showArea)
{
    if (originPixmap.isNull())
        return ;
    originSize = originPixmap.size();
    imageArea = label->geometry();
    showArea = this->rect();
}

void ResizablePicture::wheelEvent(QWheelEvent *event)
{
    QPoint pos = event->pos();
    if (!label->geometry().contains(pos))
        return ;

    // 设置缩放值
    double scale = 1;
    if (event->delta() < 0)
    {
        scale = 0.8;
    }
    else if (event->delta() > 0)
    {
        scale = 1.25;
    }
    else
        return ;

    scaleTo(scale, pos);
}

void ResizablePicture::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        pressPos = event->pos();
    }
}

void ResizablePicture::mouseMoveEvent(QMouseEvent * e)
{
    if (e->buttons() & Qt::LeftButton)
    {
        label->move(label->x() + e->x() - pressPos.x(), label->y() + e->y() - pressPos.y());
        pressPos = e->pos();
    }
}

void ResizablePicture::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        scaleCache[sizeToLL(originPixmap.size())] = label->geometry();
    }
}

void ResizablePicture::resizeEvent(QResizeEvent *)
{
    if (resizeAutoInit || originPixmap.isNull())
    {
        // 清理全部缓存
        scaleCache.clear();

        resetScale();
    }
}

qint64 ResizablePicture::sizeToLL(QSize size)
{
    return size.width() * 1000000 + size.height();
}
