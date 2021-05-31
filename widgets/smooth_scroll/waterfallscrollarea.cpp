#include <QWheelEvent>
#include <QDebug>
#include <QPropertyAnimation>
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

/// 固定的子项，就是以后resize的时候不需要重复获取子项了
/// 理论上来说能够提高一些速度
/// 禁止动态添加控件时使用，或者添加假手动更新子控件
void WaterfallScrollArea::initFixedChildren()
{
    fixedChildren = true;
    updateChildWidgets();
}

void WaterfallScrollArea::adjustWidgetPos()
{
    QRect cr = this->contentsRect();
    if (!fixedChildren)
    {
        updateChildWidgets();

        if (colCount > 0) // 仅根据列数变化
        {
            colWidth = (cr.width() - itemMarginH * 2 + itemSpacingH) / colCount - itemSpacingH;
            colWidth = qMax(colWidth, 1);
        }
    }
    if (widgets.isEmpty())
        return ;

    // 计算各类数值
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

QList<QWidget *> WaterfallScrollArea::getWidgets()
{
    return widgets;
}

/// 固定所有子控件的大小
/// 需要先 updateChildWidgets
void WaterfallScrollArea::setChildrenFixedSize()
{
    foreach (auto w, widgets)
    {
        w->setFixedSize(w->sizeHint());
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

void WaterfallScrollArea::wheelEvent(QWheelEvent *event)
{


    return QScrollArea::wheelEvent(event);
}
