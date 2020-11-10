#ifndef PORTRAITLABEL_H
#define PORTRAITLABEL_H

#include <QLabel>
#include <QPainter>
#include <QPainterPath>

class PortraitLabel : public QLabel
{
public:
    PortraitLabel(QPixmap portrait, int side, QWidget* parent = nullptr)
        : QLabel(parent), side(side)
    {
        setFixedSize(side, side);
        setPixmap(portrait);
    }

    PortraitLabel(int side, QWidget* parent = nullptr)
        : QLabel(parent), side(side)
    {
        setFixedSize(side, side);
        setText("@@@@@@");
    }

    void setPixmap(QPixmap portrait)
    {
        this->pixmap = portrait;

        setFixedSize(side, side);
        portrait = portrait.scaled(side, side);
        QPixmap pixmap(side, side);

        QPainter painter(&pixmap);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        QPainterPath path;
        path.addEllipse(0, 0, side/2, side/2);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, side, side, portrait);
        QLabel::setPixmap(pixmap);
    }

private:
    QPixmap pixmap;
    int side = 16;
};

#endif // PORTRAITLABEL_H
