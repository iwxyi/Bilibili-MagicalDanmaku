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
        connect(this, SIGNAL(sliderMoved(int)), this, SIGNAL(signalMoved(int)));
    }

    bool isPressing() const
    {
        return _pressing;
    }

signals:
    void signalMoved(int pos); // 需要手动连接 信号槽

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        // 百分比
        double p = (double)e->pos().x() / (double)width();
        int val = p*(maximum()-minimum())+minimum();
        setValue(val);
        emit signalMoved(val);
        _pressing = true;
        QSlider::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        // emit signalMoved(this->sliderPosition());
        _pressing = false;
        QSlider::mouseReleaseEvent(e);
    }

private:
    bool _pressing = false;
};

#endif // CLICKSLIDER_H
