#include <QScrollBar>
#include <QWheelEvent>
#include "smoothlistwidget.h"

SmoothListWidget::SmoothListWidget(QWidget *parent) : QListWidget(parent)
{
    setVerticalScrollMode(QListWidget::ScrollPerPixel);
    setHorizontalScrollMode(QListWidget::ScrollPerPixel);
}

void SmoothListWidget::setSmoothScrollEnabled(bool e)
{
#ifdef Q_OS_WIN
    this->enabledSmoothScroll = e;
#endif
}

void SmoothListWidget::setSmoothScrollSpeed(int speed)
{
    this->smoothScrollSpeed = speed;
}

void SmoothListWidget::setSmoothScrollDuration(int duration)
{
    this->smoothScrollDuration = duration;
}

void SmoothListWidget::scrollToTop()
{
    scrollTo(getScrollBar()->minimum());
}

void SmoothListWidget::scrollTo(int pos)
{
    if (!enabledSmoothScroll)
        return getScrollBar()->setSliderPosition(pos);

    auto scrollBar = getScrollBar();
    int delta = pos - scrollBar->sliderPosition();
    if (qAbs(delta) <= 1)
        return getScrollBar()->setSliderPosition(pos);
    addSmoothScrollThread(delta, smoothScrollDuration);
}

void SmoothListWidget::scrollToBottom()
{
    int count = smooth_scrolls.size();
    scrollTo(getScrollBar()->maximum());
    if (!count || count >= smooth_scrolls.size()) // 理论上来说size会+1
        return ;

    toBottoming++;
    connect(smooth_scrolls.last(), &SmoothScrollBean::signalSmoothScrollFinished, this, [=]{
        toBottoming--;
        if (toBottoming < 0) // 到底部可能会提前中止
            toBottoming = 0;
    });
}

bool SmoothListWidget::isToBottoming() const
{
    return toBottoming;
}

void SmoothListWidget::addSmoothScrollThread(int distance, int duration)
{
    SmoothScrollBean* bean = new SmoothScrollBean(distance, duration);
    smooth_scrolls.append(bean);
    connect(bean, SIGNAL(signalSmoothScrollDistance(SmoothScrollBean*, int)), this, SLOT(slotSmoothScrollDistance(SmoothScrollBean*, int)));
    connect(bean, &SmoothScrollBean::signalSmoothScrollFinished, [=]{
        delete bean;
        smooth_scrolls.removeOne(bean);
    });
}

void SmoothListWidget::slotSmoothScrollDistance(SmoothScrollBean *bean, int dis)
{
    int slide = getScrollBar()->sliderPosition();
    slide += dis;
    if (slide < 0)
    {
        slide = 0;
        smooth_scrolls.removeOne(bean);
    }
    else if (slide > getScrollBar()->maximum())
    {
        slide = getScrollBar()->maximum();
        smooth_scrolls.removeOne(bean);
    }
    getScrollBar()->setSliderPosition(slide);
}

void SmoothListWidget::wheelEvent(QWheelEvent *event)
{
    if (enabledSmoothScroll)
    {
        if (event->delta() > 0) // 上滚
        {
            if (getScrollBar()->sliderPosition() == getScrollBar()->minimum() && !smooth_scrolls.size()) // 到顶部了
                emit signalLoadTop();
            addSmoothScrollThread(-smoothScrollSpeed, smoothScrollDuration);
            toBottoming = 0;
        }
        else if (event->delta() < 0) // 下滚
        {
            if (getScrollBar()->sliderPosition() == getScrollBar()->maximum() && !smooth_scrolls.size()) // 到顶部了
                emit signalLoadBottom();
            addSmoothScrollThread(smoothScrollSpeed, smoothScrollDuration);
        }
    }
    else
    {
        QListView::wheelEvent(event);
    }
}

QScrollBar *SmoothListWidget::getScrollBar()
{
    if (this->flow() == QListWidget::Flow::LeftToRight)
        return horizontalScrollBar();
    return verticalScrollBar();
}
