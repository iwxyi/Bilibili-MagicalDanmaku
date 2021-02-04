#ifndef ROUNDEDLABEL_H
#define ROUNDEDLABEL_H

#include <QLabel>
#include <QPainter>
#include <QPainterPath>

class RoundedPixmapLabel : public QLabel
{
public:
    RoundedPixmapLabel(QWidget* parent = nullptr) : QLabel(parent)
    {
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const QPixmap* pixmap = this->pixmap();
        if (!pixmap)
            return ;
        QPainter painter(this);
        QPainterPath path;
        path.addRoundedRect(rect(), 5, 5);
        painter.setClipPath(path);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(rect(), *pixmap);
    }
};

#endif // ROUNDEDLABEL_H
