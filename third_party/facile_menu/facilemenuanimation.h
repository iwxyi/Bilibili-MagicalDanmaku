#ifndef FACILEMENUANIMATION_H
#define FACILEMENUANIMATION_H

#include <cmath>
#include "interactivebuttonbase.h"

class FacileBackgrounWidget : QWidget
{

};

class FacileSwitchWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double switch_ani READ getSwitchAni WRITE setSwitchAni)
public:
    FacileSwitchWidget(QWidget* w1, QWidget* w2) : QWidget(nullptr)
    {
        // 设置属性
        setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

        // 设置圆角矩形
        int borderRadius = 0;
        if (borderRadius)
        {
            QPixmap pixmap(width(), height());
            pixmap.fill(Qt::transparent);
            QPainter pix_ptr(&pixmap);
            pix_ptr.setRenderHint(QPainter::Antialiasing, true);
            QPainterPath path;
            path.addRoundedRect(0, 0, width(), height(), borderRadius, borderRadius);
            pix_ptr.fillPath(path, Qt::white);
            setMask(pixmap.mask());
        }

        // 创建状态
        pixmap1 = QPixmap(w1->size());
        w1->render(&pixmap1);
        pixmap2 = QPixmap(w2->size());
        w2->render(&pixmap2);

        // 动画设置
        int duration = 150;

        // 绘制动画
        QPropertyAnimation* ani = new QPropertyAnimation(this, "switch_ani");
        ani->setStartValue(0);
        ani->setEndValue(255);
        ani->setDuration(duration);
        ani->setEasingCurve(QEasingCurve::OutSine);
        ani->start();
        connect(ani, &QPropertyAnimation::stateChanged, this, [=](QPropertyAnimation::State state) {
            if (state == QPropertyAnimation::State::Stopped)
                ani->deleteLater();
        });

        // 移动动画
        ani = new QPropertyAnimation(this, "geometry");
        ani->setStartValue(w1->geometry());
        ani->setEndValue(w2->geometry());
        ani->setDuration(duration);
        ani->setEasingCurve(QEasingCurve::OutSine);
        connect(ani, SIGNAL(valueChanged(const QVariant &)), this, SLOT(update()));
        ani->start();
        connect(ani, &QPropertyAnimation::stateChanged, this, [=](QPropertyAnimation::State state) {
            if (state == QPropertyAnimation::State::Stopped)
            {
                emit finished();
                ani->deleteLater();
                this->deleteLater();
            }
        });

        this->show();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setOpacity(qMax(0.0, 192 - switch_ani) / 192);
        painter.drawPixmap(rect(), pixmap1);
        painter.setOpacity(qMax(0.0, switch_ani-64) / 192);
        painter.drawPixmap(rect(), pixmap2);
    }

    double getSwitchAni() const
    {
        return switch_ani;
    }

    void setSwitchAni(double x)
    {
        this->switch_ani = x;
    }

signals:
    void finished();

private:
    QPixmap pixmap1;
    QPixmap pixmap2;
    double alpha1 = 0;
    double alpha2 = 0;
    double switch_ani = 0;
};

#endif // FACILEMENUANIMATION_H
