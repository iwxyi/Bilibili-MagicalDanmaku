#ifndef CLICKSLIDER_H
#define CLICKSLIDER_H

#include <QSlider>
#include <QMouseEvent>

class ClickSlider : public QSlider
{
    Q_OBJECT
public:
    ClickSlider(QWidget* parent = nullptr) : QSlider(parent)
    {
    }

signals:
    void signalClickMove(int pos); // 需要手动连接 信号槽

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        // 百分比
        double p = (double)e->pos().x() / (double)width();
        int val = p*(maximum()-minimum())+minimum();
        setValue(val);
        QSlider::mousePressEvent(e);

        emit signalClickMove(val);
    }
};

#endif // CLICKSLIDER_H
