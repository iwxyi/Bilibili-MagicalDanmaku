#ifndef PIXMAPUTIL_H
#define PIXMAPUTIL_H

#include <QPixmap>
#include <QPainter>

class PixmapUtil
{
public:
    /**
     * 变成圆角图片
     */
    static QPixmap getRoundedPixmap(const QPixmap &pixmap)
    {
        QPixmap dest(pixmap.size());
        dest.fill(Qt::transparent);
        QPainter painter(&dest);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        QRect rect = QRect(0, 0, pixmap.width(), pixmap.height());
        QPainterPath path;
        path.addRoundedRect(rect, 5, 5);
        painter.setClipPath(path);
        painter.drawPixmap(rect, pixmap);
        return dest;
    }

    /**
     * 最顶上两个角变成圆角，下面两个角是直角
     * 适用于放在一个页面最顶上的位置
     */
    static QPixmap getTopRoundedPixmap(const QPixmap &pixmap, int radius)
    {
        QPixmap dest(pixmap.size());
        dest.fill(Qt::transparent);
        QPainter painter(&dest);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        QRect rect = QRect(0, 0, pixmap.width(), pixmap.height());
        QPainterPath path;
        path.addRoundedRect(rect, radius, radius);
        QPainterPath cutPath;
        cutPath.addRect(0, radius, rect.width(), rect.height() - radius);
        path -= cutPath;
        path.addRect(0, radius, rect.width(), rect.height() - radius);
        painter.setClipPath(path);
        painter.drawPixmap(rect, pixmap);
        return dest;
    }

    /**
     * 变成圆形的图片
     */
    static QPixmap toCirclePixmap(const QPixmap &pixmap)
    {
        if (pixmap.isNull())
            return pixmap;
        int side = qMin(pixmap.width(), pixmap.height());
        QPixmap rp = QPixmap(side, side);
        rp.fill(Qt::transparent);
        QPainter painter(&rp);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        QPainterPath path;
        path.addEllipse(0, 0, side, side);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, side, side, pixmap);
        return rp;
    }

    /**
     * 给直播中的头像添加绿点
     */
    static QPixmap toLivingPixmap(QPixmap pixmap)
    {
        if (pixmap.isNull())
            return pixmap;
        QPainter painter(&pixmap);
        int wid = qMin(pixmap.width(), pixmap.height()) / 3;
        QRect rect(pixmap.width() - wid, pixmap.height() - wid, wid, wid);
        QPainterPath path;
        path.addEllipse(rect);
        painter.fillPath(path, Qt::green);
        return pixmap;
    }
};

#endif // PIXMAPUTIL_H
