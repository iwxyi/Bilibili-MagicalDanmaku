#include <QWheelEvent>
#include <QDebug>
#include <QPropertyAnimation>
#include <QScrollBar>
#include "waterfallscrollarea.h"

WaterfallScrollArea::WaterfallScrollArea(QWidget *parent) : QScrollArea(parent)
{

}

/// 设置左右和上下控件的间隔
/// 不会立即调整所有子控件的位置
void WaterfallScrollArea::setItemSpacing(int h, int v)
{
    this->itemSpacingH = h;
    this->itemSpacingV = v;
}

void WaterfallScrollArea::setItemMargin(int h, int v)
{
    this->itemMarginH = h;
    this->itemMarginV = v;
}

/// 设置固定列数
/// 如果为 0，则自动以子控件最大宽度为列宽，区域宽/列宽 则是列数
/// 不会立即调整子控件位置
void WaterfallScrollArea::setColCount(int c)
{
    this->colCount = c;
}

/// 设置横向对齐方向
void WaterfallScrollArea::setAlignment(Qt::Alignment align)
{
    this->alignment = align;
}

void WaterfallScrollArea::enableRandomSizeChildren()
{
    this->useEqualWidth = false;
}

/// 固定的子项，就是以后resize的时候不需要重复获取子项了
/// 理论上来说能够提高一些速度
/// 动态添加控件时不建议使用，若要使用务必在更新后手动调用 updateChildWidgets
void WaterfallScrollArea::initFixedChildren()
{
    fixedChildren = true;
    updateChildWidgets();
}

void WaterfallScrollArea::adjustWidgetPos()
{
    if (!fixedChildren)
    {
        updateChildWidgets();

        if (colCount > 0) // 仅根据列数变化
        {
            colWidth = (this->contentsRect().width() - itemMarginH * 2 + itemSpacingH) / colCount - itemSpacingH;
            colWidth = qMax(colWidth, 1);
        }
    }
    if (widgets.isEmpty())
        return ;

    if (useEqualWidth)
        adjustWaterfallPos();
    else
        adjustRandomSizePos();
}

/// 全部子控件宽度一致的瀑布流
void WaterfallScrollArea::adjustWaterfallPos()
{
    // 计算各类数值
    QRect cr = this->contentsRect();
    cr.setWidth(qMax(1, cr.width() - this->verticalScrollBar()->width()));
    int colCount = this->colCount; // 使用列数
    if (colCount <= 0)
        colCount = (cr.width() - itemMarginH * 2 + itemSpacingH) / (colWidth + itemSpacingH);
    colCount = qMax(colCount, 1);

    QList<int> colBottoms; // 每一列的底部（不包括 itemSpacingV）
    for (int i = 0; i < colCount; i++)
        colBottoms.append(itemMarginV);

    int allLeft = itemMarginH; // 所有左边，根据Alignment判断
    if (alignment == Qt::AlignLeft) // 靠左对齐
    {
        // 默认左对齐
        allLeft = cr.left() + itemMarginH;
    }
    if (alignment == Qt::AlignCenter || alignment == Qt::AlignHCenter) // 居中对齐
    {
        allLeft = cr.left() + itemMarginH + (cr.width() - itemMarginH * 2 - colCount * (colWidth + itemSpacingH)) / 2 + itemSpacingH;
    }
    else if (alignment == Qt::AlignRight) // 靠右对齐
    {
        allLeft = cr.right() - itemMarginH - colCount * (colWidth + itemSpacingH) + itemSpacingH;
    }

    // 遍历所有控件，开始调整
    foreach (auto w, widgets)
    {
        int index = 0;
        int top = colBottoms.at(0);

        for (int i = 0; i < colCount; i++)
        {
            if (colBottoms.at(i) < top)
            {
                index = i;
                top = colBottoms.at(i);
            }
        }

        int left = allLeft + index * (colWidth + itemSpacingH);
        colBottoms[index] += w->height() + itemSpacingV;
        if (w->pos() == QPoint(left, top))
            continue;

        // 移动动画
        QPropertyAnimation* ani = new QPropertyAnimation(w, "pos");
        ani->setDuration(300);
        ani->setStartValue(w->pos());
        ani->setEndValue(QPoint(left, top));
        ani->setEasingCurve(QEasingCurve::OutCubic);
        connect(ani, &QPropertyAnimation::stateChanged, this, [=](QPropertyAnimation::State state){
            if (state == QPropertyAnimation::State::Stopped) // 因为大概率是连续触发的移动，所以不能用 finished
                ani->deleteLater();
        });
        ani->start();
    }

    // 调整最大值
    int maxHeight = 0;
    foreach (auto b, colBottoms)
        if (b > maxHeight)
            maxHeight = b;
    this->widget()->setMinimumHeight(maxHeight + itemMarginV);
}

/// 全部子控件宽度不一定一致的任意瀑布流
/// 能支持任意大小的控件，但是中间可能会空出一部分
void WaterfallScrollArea::adjustRandomSizePos()
{

}

QList<QWidget *> WaterfallScrollArea::getWidgets()
{
    return widgets;
}

/// 固定所有子控件的大小
/// 需要先 updateChildWidgets
void WaterfallScrollArea::adjustWidgetsBySizeHint()
{
    if (widgets.isEmpty())
        updateChildWidgets();
    foreach (auto w, widgets)
    {
        w->setFixedSize(w->sizeHint());
    }
}

void WaterfallScrollArea::setWidgetsEqualWidth()
{
    if (widgets.isEmpty())
        updateChildWidgets();
    foreach (auto w, widgets)
    {
        w->setFixedWidth(colWidth);
    }
}

void WaterfallScrollArea::resizeEvent(QResizeEvent *event)
{
    adjustWidgetPos();

    return QScrollArea::resizeEvent(event);
}

/// 更新自己的直接子控件
void WaterfallScrollArea::updateChildWidgets()
{
    auto children = this->widget()->children();
    QWidget* w = nullptr;
    widgets.clear();
    colWidth = 0;
    foreach (auto child, children)
    {
        if ((w = qobject_cast<QWidget*>(child)))
        {
            if (w->isHidden())
                continue ;
            widgets.append(w);
        }
        if (colWidth < w->width())
            colWidth = w->width();
    }
}

/// ===================================== 瀑布流 =====================================

void WaterfallScrollArea::setSmoothScrollEnabled(bool e)
{
    this->enabledSmoothScroll = e;
}

void WaterfallScrollArea::setSmoothScrollSpeed(int speed)
{
    this->smoothScrollSpeed = speed;
}

void WaterfallScrollArea::setSmoothScrollDuration(int duration)
{
    this->smoothScrollDuration = duration;
}

void WaterfallScrollArea::scrollToBottom()
{
    if (!enabledSmoothScroll)
        return verticalScrollBar()->setSliderPosition(verticalScrollBar()->maximum());

    auto scrollBar = verticalScrollBar();
    int delta = scrollBar->maximum() + scrollBar->pageStep() - scrollBar->sliderPosition();
    if (delta <= 1)
        return verticalScrollBar()->setSliderPosition(verticalScrollBar()->maximum());
    addSmoothScrollThread(delta, smoothScrollDuration);

    toBottoming++;
    connect(smooth_scrolls.last(), &SmoothScrollBean::signalSmoothScrollFinished, this, [=]{
        toBottoming--;
        if (toBottoming < 0) // 到底部可能会提前中止
            toBottoming = 0;
    });
}

bool WaterfallScrollArea::isToBottoming() const
{
    return toBottoming;
}

void WaterfallScrollArea::addSmoothScrollThread(int distance, int duration)
{
    SmoothScrollBean* bean = new SmoothScrollBean(distance, duration);
    smooth_scrolls.append(bean);
    connect(bean, SIGNAL(signalSmoothScrollDistance(SmoothScrollBean*, int)), this, SLOT(slotSmoothScrollDistance(SmoothScrollBean*, int)));
    connect(bean, &SmoothScrollBean::signalSmoothScrollFinished, [=]{
        delete bean;
        smooth_scrolls.removeOne(bean);
    });
}

void WaterfallScrollArea::slotSmoothScrollDistance(SmoothScrollBean *bean, int dis)
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

void WaterfallScrollArea::wheelEvent(QWheelEvent *event)
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
        QScrollArea::wheelEvent(event);
    }
}
