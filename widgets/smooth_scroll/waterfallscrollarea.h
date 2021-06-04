#ifndef WATERFALLSCROLLAREA_H
#define WATERFALLSCROLLAREA_H

#include <QScrollArea>
#include "smoothscrollbean.h"

class WaterfallScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    WaterfallScrollArea(QWidget* parent = nullptr);

    struct BottomLine {
        int left, right, y;
    };

    // 瀑布流
    void setItemSpacing(int h, int v);
    void setItemMargin(int h, int v);
    void setFixedColCount(int c);
    void setAlignment(Qt::Alignment align);
    void setAllowDifferentWidth(bool en);
    void setAnimationEnabled(bool en);

    void initFixedChildren();
    void updateChildWidgets();
    void adjustWidgetsPos();
    void addWidget(QWidget* w);
    void removeWidget(QWidget* w);

    QList<QWidget*> getWidgets();
    void resizeWidgetsToSizeHint();
    void resizeWidgetsToEqualWidth();
    int getWidgetWidth();

    // 平滑滚动
    void setSmoothScrollEnabled(bool e);
    void setSmoothScrollSpeed(int speed);
    void setSmoothScrollDuration(int duration);

    void scrollToTop();
    void scrollToBottom();
    bool isToBottoming() const;
    void scrollTo(int pos);

private:
    void adjustWaterfallPos();
    void adjustVariantWidthPos();
    void placeVariantWidthWidget(QWidget* w);

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
    // ==== 瀑布流 ====
    // 界面数值
    int itemMarginH = 9; // 左右边距
    int itemMarginV = 9; // 上下边距
    int itemSpacingH = 9;
    int itemSpacingV = 9;
    Qt::Alignment alignment = Qt::AlignCenter;
    bool useSizeHint = false; // 添加的控件是否自动使用sizeHint
    bool useEqualWidth = false; // 新添加的控件是否自动使用 colWidth
    bool useAnimation = true; // 是否显示移动控件的动画

    // 控件及初始化
    QList<QWidget*> widgets;
    bool fixedChildren = false; // 是否不添加控件，即只初始化一次，可提升性能
    int fixedColCount = 0; // 固定列数，为0的话，则自动按子项宽度来

    // 相同宽度模式
    bool equalWidthMode = true; // 相同宽度模式（默认）
    int colCount = 0; // 目前使用的列数
    int colWidth = 0; // 所有子项的最宽宽度，排列按照这顺序来
    QList<int> colBottoms;
    int allLeft = 0;

    // 不同宽度模式
    QList<BottomLine> bottomLines;

    // ==== 平滑滚动 ====
    bool enabledSmoothScroll = true;
    int smoothScrollSpeed = 64;
    int smoothScrollDuration = 200;
    QList<SmoothScrollBean*> smooth_scrolls;
    int toBottoming = 0;
};

#endif // WATERFALLSCROLLAREA_H
