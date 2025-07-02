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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const QPixmap* pixmap = this->pixmap();
        if (!pixmap)
#else
        const QPixmap pixmap = this->pixmap();
#endif
            return QLabel::paintEvent(e);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        // painter.setRenderHint(QPainter::SmoothPixmapTransform);
        QPainterPath path;
        path.addRoundedRect(rect(), radius, radius);
        painter.setClipPath(path);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        painter.drawPixmap(rect(), *pixmap);
#else
        painter.drawPixmap(rect(), pixmap);
#endif
    }

private:
    int radius = 5;
};

#endif // ROUNDEDLABEL_H
