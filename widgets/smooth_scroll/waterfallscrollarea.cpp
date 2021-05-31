#include <QWheelEvent>
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

void WaterfallScrollArea::setItemMargin(int m)
{
    this->itemMargin = m;
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

        if (colCount > 0)
        {
            colWidth = (cr.width() - itemMargin * 2 + itemSpacingH) / colCount - itemSpacingH;
            colWidth = qMax(colWidth, 1);
        }
    }
    if (widgets.isEmpty())
        return ;

    // 计算各类数值
    int colCount = this->colCount; // 使用列数
    if (colCount <= 0)
        colCount = (cr.width() - itemMargin * 2 + itemSpacingH) / (colWidth + itemSpacingH);
    colCount = qMax(colCount, 1);

    QList<int> colBottom; // 每一列的底部（不包括 itemSpacingV）
    for (int i = 0; i < colCount; i++)
        colBottom[i] = itemMargin;

    int allLeft = itemMargin; // 所有左边，根据Alignment判断
    if (alignment == Qt::AlignLeft) // 靠左对齐
    {
        // 默认左对齐
        allLeft = cr.left() + itemMargin;
    }
    if (alignment == Qt::AlignCenter || alignment == Qt::AlignHCenter) // 居中对齐
    {
        allLeft = cr.left() + itemMargin + (cr.width() - itemMargin * 2 - colCount * (colWidth + itemSpacingH)) / 2 + itemSpacingH;
    }
    else if (alignment == Qt::AlignRight) // 靠右对齐
    {
        allLeft = cr.right() - itemMargin - colCount * (colWidth + itemSpacingH) + itemSpacingH;
    }


    // 遍历所有控件，开始调整
    foreach (auto w, widgets)
    {
        int index = 0;
        int top = colBottom.at(0);

        for (int i = 0; i < colCount; i++)
        {
            if (colBottom.at(i) < top)
            {
                index = i;
                top = colBottom.at(i);
            }
        }

        int left = allLeft + index * (colWidth + itemSpacingH);
        w->move(left, top);
    }
}

/// 更新自己的直接子控件
void WaterfallScrollArea::updateChildWidgets()
{
    auto children = this->children();
    QWidget* w = nullptr;
    if (colCount > 0)
        colWidth = 0;
    foreach (auto child, children)
    {
        if ((w = qobject_cast<QWidget*>(child)))
            widgets.append(w);
        if (colCount > 0 && colWidth < w->width())
            colWidth = w->width();
    }
}

void WaterfallScrollArea::wheelEvent(QWheelEvent *event)
{


    return QScrollArea::wheelEvent(event);
}
