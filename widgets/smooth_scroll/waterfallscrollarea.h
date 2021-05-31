#ifndef WATERFALLSCROLLAREA_H
#define WATERFALLSCROLLAREA_H

#include <QScrollArea>
#include "smoothscrollbean.h"

class WaterfallScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    WaterfallScrollArea(QWidget* parent = nullptr);

    // 瀑布流
    void setItemSpacing(int h, int v);
    void setItemMargin(int h, int v);
    void setColCount(int c);
    void setAlignment(Qt::Alignment align);

    void initFixedChildren();
    void updateChildWidgets();
    void adjustWidgetPos();

    QList<QWidget*> getWidgets();
    void adjustWidgetsBySizeHint();
    void setWidgetsEqualWidth();

    // 平滑滚动
    void setSmoothScrollEnabled(bool e);
    void setSmoothScrollSpeed(int speed);
    void setSmoothScrollDuration(int duration);

    void scrollToBottom();
    bool isToBottoming() const;

private:
    void addSmoothScrollThread(int distance, int duration);

signals:
    void signalLoadTop(); // 到顶端之后下拉
    void signalLoadBottom(); // 到底部之后上拉

public slots:
    void slotSmoothScrollDistance(SmoothScrollBean* bean, int dis);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    // 瀑布流
    int itemMarginH = 0; // 左右边距
    int itemMarginV = 0; // 上下边距
    int itemSpacingH = 9;
    int itemSpacingV = 9;
    Qt::Alignment alignment = Qt::AlignCenter;

    QList<QWidget*> widgets;
    bool fixedChildren = false;
    int colCount = 0; // 固定列数，为0的话，则自动按子项宽度来
    int colWidth = 0; // 所有子项的最宽宽度，排列按照这顺序来

    // 平滑滚动
    bool enabledSmoothScroll = true;
    int smoothScrollSpeed = 64;
    int smoothScrollDuration = 200;
    QList<SmoothScrollBean*> smooth_scrolls;
    int toBottoming = 0;
};

#endif // WATERFALLSCROLLAREA_H
