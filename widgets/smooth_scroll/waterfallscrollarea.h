#ifndef WATERFALLSCROLLAREA_H
#define WATERFALLSCROLLAREA_H

#include <QScrollArea>

class WaterfallScrollArea : public QScrollArea
{
public:
    WaterfallScrollArea(QWidget* parent = nullptr);

    void setItemSpacing(int h, int v);
    void setItemMargin(int h, int v);
    void setColCount(int c);
    void setAlignment(Qt::Alignment align);

    void initFixedChildren();
    void updateChildWidgets();
    void adjustWidgetPos();

    QList<QWidget*> getWidgets();
    void setChildrenFixedSize();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    int itemMarginH = 0; // 左右边距
    int itemMarginV = 0; // 上下边距
    int itemSpacingH = 9;
    int itemSpacingV = 9;
    Qt::Alignment alignment = Qt::AlignCenter;

    QList<QWidget*> widgets;
    bool fixedChildren = false;
    int colCount = 0; // 固定列数，为0的话，则自动按子项宽度来
    int colWidth = 0; // 所有子项的最宽宽度，排列按照这顺序来
};

#endif // WATERFALLSCROLLAREA_H
