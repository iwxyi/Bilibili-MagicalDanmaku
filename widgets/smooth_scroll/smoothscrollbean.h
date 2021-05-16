#ifndef SMOOTHSCROLLBEAN_H
#define SMOOTHSCROLLBEAN_H

#include <QObject>
#include <QTimer>

#define TIMER_INTERVAL 10 // 时钟周期

class SmoothScrollBean : public QObject
{
    Q_OBJECT
public:
    SmoothScrollBean(int distance, int duration) : distance(distance), duration(duration)
    {
        per = distance * TIMER_INTERVAL / duration; // 每个时钟周期滚动的距离
        if (per != 0)
            count = distance / per; // 滚动次数也等于 duration / TIMER_INTERVAL
        else // 滚动一小段距离
        {
            per = distance > 0 ? 1 : -1;
            count = distance;
        }

        QTimer* timer = new QTimer(this);
        timer->setInterval(TIMER_INTERVAL);
        timer->start();
        connect(timer, &QTimer::timeout, [=]{
            emit signalSmoothScrollDistance(this, per);
            count--;
            if (count <= 0)
            {
                timer->stop();
                timer->deleteLater();
                emit signalSmoothScrollFinished();
            }
        });
    }

    bool isPositive()
    {
        return per > 0;
    }

signals:
    void signalSmoothScrollDistance(SmoothScrollBean* bean, int dis);
    void signalSmoothScrollFinished();

private:
    int per;
    int count;
    int distance;
    int duration;
    QTimer* timer;
};

#endif // SMOOTHSCROLLBEAN_H
