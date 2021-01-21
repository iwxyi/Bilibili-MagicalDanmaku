#ifndef ROUNDEDANIMATIONLABEL_H
#define ROUNDEDANIMATIONLABEL_H

#include <QLabel>
#include <QPropertyAnimation>
#include <QPainter>
#include <QPainterPath>
#include <QPainter>
#include <QSequentialAnimationGroup>
#include <QDebug>

class RoundedAnimationLabel : public QLabel
{
public:
    RoundedAnimationLabel(QWidget* parent = nullptr) : QLabel(parent)
    {

    }

    void startHide()
    {
        QPixmap pixmap(this->size());
        this->render(&pixmap);
        this->pixmap = pixmap;
        this->setScaledContents(true);
        setStyleSheet("background: transparent;");

        originSide = qMin(width(), height());
        finalRadius = originSide / 4;

        QPropertyAnimation* ani = new QPropertyAnimation(this, "geometry");
        ani->setStartValue(this->rect());
        ani->setEndValue(QRect(this->rect().center(), QSize(1,1)));
        ani->setEasingCurve(QEasingCurve::InCubic);
        ani->setDuration(500);
        connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
        connect(ani, SIGNAL(finished()), this, SLOT(deleteLater()));
        ani->start();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        if (pixmap.isNull())
            return QLabel::paintEvent(event);

        // 绘制圆角的动画
        QPainter painter(this);

        QPainterPath path;

        QPixmap pixmap = this->pixmap.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
        int minSide = qMin(height(), width());
        int dis = originSide - minSide;
        if (dis <= originSide / 2) // 圆角矩形
        {
            double prop = double(dis) / (originSide / 2);
            int rad = finalRadius * prop;
            path.addRoundedRect(0, 0, width(), height(), rad, rad);
        }
        else // 圆形
        {
            int rad = qMin(height(), width()) / 2;
            path.addRoundedRect(0, 0, width(), height(), rad, rad);
        }
        painter.setClipPath(path);
        painter.drawPixmap(rect(), pixmap);
    }

private:
    QPixmap pixmap;
    int originSide = 0;
    int finalRadius = 0;
};

#endif // ROUNDEDANIMATIONLABEL_H
