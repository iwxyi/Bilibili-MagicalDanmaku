#include <QScrollBar>
#include <QWheelEvent>
#include "smoothlistwidget.h"

SmoothListWidget::SmoothListWidget(QWidget *parent) : QListWidget(parent)
{

}

void SmoothListWidget::setSmoothScrollEnabled(bool e)
{
    this->enabledSmoothScroll = e;
}

void SmoothListWidget::setSmoothScrollSpeed(int speed)
{
    this->smoothScrollSpeed = speed;
}

void SmoothListWidget::setSmoothScrollDuration(int duration)
{
    this->smoothScrollDuration = duration;
}

void SmoothListWidget::scrollToBottom()
{
    if (!enabledSmoothScroll)
        return QListWidget::scrollToBottom();

    auto scrollBar = verticalScrollBar();
    int delta = scrollBar->maximum() + scrollBar->pageStep() - scrollBar->sliderPosition();
    if (delta <= 1)
        return QListWidget::scrollToBottom();
    addSmoothScrollThread(delta, smoothScrollDuration);

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
    int slide = verticalScrollBar()->sliderPosition();
    slide += dis;
    if (slide < 0)
    {
        slide = 0;
        smooth_scrolls.removeOne(bean);
    }
    else if (slide > verticalScrollBar()->maximum())
    {
        slide = verticalScrollBar()->maximum();
        smooth_scrolls.removeOne(bean);
    }
    verticalScrollBar()->setSliderPosition(slide);
}

void SmoothListWidget::wheelEvent(QWheelEvent *event)
{
    if (enabledSmoothScroll)
    {
        if (event->delta() > 0) // 上滚
        {
            if (verticalScrollBar()->sliderPosition() == verticalScrollBar()->minimum() && !smooth_scrolls.size()) // 到顶部了
                emit signalLoadTop();
            addSmoothScrollThread(-smoothScrollSpeed, smoothScrollDuration);
            toBottoming = 0;
        }
        else if (event->delta() < 0) // 下滚
        {
            if (verticalScrollBar()->sliderPosition() == verticalScrollBar()->maximum() && !smooth_scrolls.size()) // 到顶部了
                emit signalLoadBottom();
            addSmoothScrollThread(smoothScrollSpeed, smoothScrollDuration);
        }
    }
    else
    {
        QListView::wheelEvent(event);
    }
}
