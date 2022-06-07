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

    void setRadius(int x)
    {
        this->radius = x;
        update();
    }

protected:
    void paintEvent(QPaintEvent *e) override
    {
        const QPixmap* pixmap = this->pixmap();
        if (!pixmap)
            return QLabel::paintEvent(e);
        QPainter painter(this);
        QPainterPath path;
        path.addRoundedRect(rect(), radius, radius);
        painter.setClipPath(path);
        // painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(rect(), *pixmap);
    }

private:
    int radius = 5;
};

#endif // ROUNDEDLABEL_H
