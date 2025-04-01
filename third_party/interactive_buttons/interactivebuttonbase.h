#ifndef INTERACTIVEBUTTONBASE_H
#define INTERACTIVEBUTTONBASE_H

#include <QObject>
#include <QApplication>
#include <QPushButton>
#include <QPoint>
#include <QTimer>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include <QDateTime>
#include <QList>
#include <QBitmap>
#include <QtMath>
#include <QSvgRenderer>

#define PI 3.1415926
#define GOLDEN_RATIO 0.618

#define DOUBLE_PRESS_INTERVAL 500 // /* 300 */松开和按下的间隔。相等为双击
#define SINGLE_PRESS_INTERVAL 200 // /* 150 */按下时间超过这个数就是单击。相等为单击

/**
 * Copyright (c) 2019 命燃芯乂 All rights reserved.
 ×
 * 邮箱：wxy@iwxyi.com
 * QQ号：482582886
 * 时间：2020.12.28
 *
 * 说明：灵性的自定义按钮，简单又有趣
 * 源码：https://github.com/MRXY001/Interactive-Windows-Buttons
 *
 * 本代码为本人编写方便自己使用，现在无私送给大家免费使用。
 * 程序版权归作者所有，只可使用不能出售，违反者本人有权追究责任。
 */

class InteractiveButtonBase : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(bool self_enabled READ getSelfEnabled WRITE setSelfEnabled)                      // 是否启用自定义的按钮（true）
    Q_PROPERTY(bool parent_enabled READ getParentEnabled WRITE setParentEnabled)                // 是否启用父类按钮（false）
    Q_PROPERTY(bool fore_enabled READ getForeEnabled WRITE setForeEnabled)                      // 是否绘制自定义按钮前景色（true）
    Q_PROPERTY(QString text READ getText WRITE setText)                                         // 前景文字
    Q_PROPERTY(QString icon_path READ getIconPath WRITE setIconPath)                            // 前景图标
    Q_PROPERTY(QString pixmap_path READ getPixmapPath WRITE setPixmapPath)                      // 前景图标
    Q_PROPERTY(QColor icon_color READ getIconColor WRITE setIconColor)                          // 前景图标帅色
    Q_PROPERTY(QColor text_color READ getTextColor WRITE setTextColor)                          // 前景文字颜色
    Q_PROPERTY(QColor background_color READ getNormalColor WRITE setNormalColor)                // 背景颜色
    Q_PROPERTY(QColor border_color READ getBorderColor WRITE setBorderColor)                    // 边界颜色
    Q_PROPERTY(QColor hover_color READ getHoverColor WRITE setHoverColor)                       // 鼠标悬浮背景颜色
    Q_PROPERTY(QColor press_color READ getPressColor WRITE setPressColor)                       // 鼠标按下背景颜色
    Q_PROPERTY(int hover_duration READ getHoverAniDuration WRITE setHoverAniDuration)           // 鼠标悬浮动画周期
    Q_PROPERTY(int press_duration READ getPressAniDuration WRITE setPressAniDuration)           // 鼠标按下动画周期
    Q_PROPERTY(int click_duration READ getClickAniDuration WRITE setClickAniDuration)           // 鼠标点击动画周期
    Q_PROPERTY(double icon_padding_proper READ getIconPaddingProper WRITE setIconPaddingProper) // 图标四边空白处大小比例
    Q_PROPERTY(int radius READ getRadius WRITE setRadius)                                       // 边框圆角半径
    Q_PROPERTY(int border_width READ getBorderWidth WRITE setBorderWidth)                       // 边框线条粗细
    Q_PROPERTY(bool fixed_fore_pos READ getFixedTextPos WRITE setFixedTextPos)                  // 是否固定前景位置（false）
    Q_PROPERTY(bool text_dynamic_size READ getTextDynamicSize WRITE setTextDynamicSize)         // 修改字体大小时调整按钮最小尺寸（false）
    Q_PROPERTY(bool leave_after_clicked READ getLeaveAfterClick WRITE setLeaveAfterClick)       // 鼠标单击松开后取消悬浮效果（针对菜单、弹窗）
    Q_PROPERTY(bool show_animation READ getShowAni WRITE setShowAni)                            // 是否启用出现动画（鼠标移开则消失）（false）
    Q_PROPERTY(bool water_animation READ getWaterRipple WRITE setWaterRipple)                   // 是否启用点击水波纹动画（否则使用渐变）（true）
    Q_PROPERTY(int font_size READ getFontSizeT WRITE setFontSizeT)                              // 动：按钮字体动画效果（自动，不应该设置）
public:
    InteractiveButtonBase(QWidget *parent = nullptr);
    InteractiveButtonBase(QString text, QWidget *parent = nullptr);
    InteractiveButtonBase(QIcon icon, QWidget *parent = nullptr);
    InteractiveButtonBase(QPixmap pixmap, QWidget *parent = nullptr);
    InteractiveButtonBase(QIcon icon, QString text, QWidget *parent = nullptr);
    InteractiveButtonBase(QPixmap pixmap, QString text, QWidget *parent = nullptr);

    /**
     * 前景实体
     */
    enum PaintModel
    {
        None,       // 无前景，仅使用背景
        Text,       // 纯文字（替代父类）
        Icon,       // 纯图标
        PixmapMask, // 可变色图标（通过pixmap+遮罩实现），锯齿化明显
        IconText,   // 图标+文字（强制左对齐）
        PixmapText  // 变色图标+文字（强制左对齐）
    };

    /**
     * 四周边界的padding
     * 调整按钮大小时：宽度+左右、高度+上下
     */
    struct EdgeVal
    {
        EdgeVal() {}
        EdgeVal(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
        int left = 0, top = 0, right = 0, bottom = 0; // 四个边界的空白距离

        bool operator==(const EdgeVal& another)
        {
            return left == another.left && top == another.top && right == another.right && bottom == another.right;
        }

        bool operator!= (const EdgeVal& another)
        {
            return !((*this) == another);
        }
    };

    /**
     * 前景额外的图标（可以多个）
     * 可能是角标（比如展开箭头）
     * 可能是前缀（图例）
     */
    struct PaintAddin
    {
        PaintAddin() : enable(false) {}
        PaintAddin(QIcon i, Qt::Alignment a, QSize s) : enable(true), icon(i), align(a), size(s) {}
        PaintAddin(QPixmap p, Qt::Alignment a, QSize s) : enable(true), pixmap(p), align(a), size(s) {}
        bool enable;         // 是否启用
        QIcon icon;          // 不可变色图标（优先）
        QPixmap pixmap;      // 可变色图标
        Qt::Alignment align; // 对齐方式
        QSize size;          // 固定大小
        EdgeVal padding;     // 四边大小
    };

    /**
     * 鼠标松开时抖动动画
     * 松开的时候计算每一次抖动距离+时间，放入队列中
     * 定时调整抖动的队列实体索引
     */
    struct Jitter
    {
        Jitter(QPointF p, qint64 t) : point(p), timestamp(t) {}
        QPointF point;     // 要运动到的目标坐标
        qint64 timestamp; // 运动到目标坐标应该的时间戳，结束后删除本次抖动路径对象
    };

    /**
     * 鼠标按下/弹起水波纹动画
     * 鼠标按下时动画速度慢（压住），松开后动画速度骤然加快
     * 同样用队列记录所有的水波纹动画实体
     */
    struct Water
    {
        Water(QPointF p, qint64 t) : point(p), progress(0), press_timestamp(t),
                                    release_timestamp(0), finish_timestamp(0), finished(false) {}
        QPointF point;
        int progress;             // 水波纹进度100%（已弃用，当前使用时间戳）
        qint64 press_timestamp;   // 鼠标按下时间戳
        qint64 release_timestamp; // 鼠标松开时间戳。与按下时间戳、现行时间戳一起成为水波纹进度计算参数
        qint64 finish_timestamp;  // 结束时间戳。与当前时间戳相减则为渐变消失经过的时间戳
        bool finished;            // 是否结束。结束后改为渐变消失
    };

    enum NolinearType
    {
        Linear,
        SlowFaster,
        FastSlower,
        SlowFastSlower,
        SpringBack20,
        SpringBack50
    };

    virtual void setText(QString text);
    virtual void setIconPath(QString path);
    virtual void setIcon(QIcon icon);
    virtual void setPixmapPath(QString path);
    virtual void setPixmap(QPixmap pixmap);
    virtual void setSvgPath(QString path);
    virtual void setPaintAddin(QIcon icon, Qt::Alignment align = Qt::AlignRight, QSize size = QSize(32, 32));
    virtual void setPaintAddin(QPixmap pixmap, Qt::Alignment align = Qt::AlignRight, QSize size = QSize(0, 0));
    virtual void setPaintAddinPadding(int horizonal, int vertival);
    virtual void setPaintAddinPadding(int left, int top, int right, int bottom);

    void setSelfEnabled(bool e = true);
    void setParentEnabled(bool e = false);
    void setForeEnabled(bool e = true);

    void setHoverAniDuration(int d);
    void setPressAniDuration(int d);
    void setClickAniDuration(int d);
    void setWaterAniDuration(int press, int release, int finish);
    void setWaterRipple(bool enable = true);
    void setJitterAni(bool enable = true);
    void setUnifyGeomerey(bool enable = true);
    void setBgColor(QColor bg);
    void setBgColor(QColor hover, QColor press);
    void setNormalColor(QColor color);
    void setBorderColor(QColor color);
    void setHoverColor(QColor color);
    void setPressColor(QColor color);
    void setIconColor(QColor color = QColor(0, 0, 0));
    void setTextColor(QColor color = QColor(0, 0, 0));
    void setFocusBg(QColor color);
    void setFocusBorder(QColor color);
    void setFontSize(int f);
    void setHover();
    void setAlign(Qt::Alignment a);
    void setRadius(int r);
    void setRadius(int rx, int ry);
    void setBorderWidth(int x);
    void setDisabled(bool dis = true);
    void setPaddings(int l, int r, int t, int b);
    void setPaddings(int h, int v);
    void setPaddings(int x);
    void setIconPaddingProper(double x);
    void setFixedForePos(bool f = true);
    void setFixedForeSize(bool f = true, int addin = 0);
    void setSquareSize();
    void setTextDynamicSize(bool d = true);
    void setLeaveAfterClick(bool l = true);
    void setDoubleClicked(bool e = true);
    void setAutoTextColor(bool a = true);
    void setPretendFocus(bool f = true);
    void setBlockHover(bool b = true);

    void setShowAni(bool enable = true);
    void showForeground();
    void showForeground2(QPoint point = QPoint(0, 0));
    void hideForeground();
    void delayShowed(int time, QPoint point = QPoint(0, 0));

    QString getText();
    void setMenu(QMenu *menu);
    void adjustMinimumSize();
    void setState(bool s = true);
    bool getState();
    virtual void simulateStatePress(bool s = true, bool a = false);
    bool isHovering() { return hovering; }
    bool isPressing() { return pressing; }
    void simulateHover();
    void discardHoverPress(bool force = false);

    bool getSelfEnabled() { return self_enabled; }
    bool getParentEnabled() { return parent_enabled; }
    bool getForeEnabled() { return fore_enabled; }
    QColor getIconColor() { return icon_color; }
    QColor getTextColor() { return text_color; }
    QColor getNormalColor() { return normal_bg; }
    QColor getBorderColor() { return border_bg; }
    QColor getHoverColor() { return hover_bg; }
    QColor getPressColor() { return press_bg; }
    QString getIconPath() { return ""; }
    QString getPixmapPath() { return ""; }
    int getHoverAniDuration() { return hover_bg_duration; }
    int getPressAniDuration() { return press_bg_duration; }
    int getClickAniDuration() { return click_ani_duration; }
    double getIconPaddingProper() { return icon_padding_proper; }
    int getRadius() { return qMax(radius_x, radius_y); }
    int getBorderWidth() { return border_width; }
    bool getFixedTextPos() { return fixed_fore_pos; }
    bool getTextDynamicSize() { return text_dynamic_size; }
    bool getLeaveAfterClick() { return leave_after_clicked; }
    bool getShowAni() { return show_animation; }
    bool getWaterRipple() { return water_animation; }

#if QT_DEPRECATED_SINCE(5, 11)
    QT_DEPRECATED_X("Use InteractiveButtonBase::setFixedForePos(bool fixed = true)")
    void setFixedTextPos(bool f = true);
#endif

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    virtual bool inArea(QPoint point);
    virtual bool inArea(QPointF point);
    virtual QPainterPath getBgPainterPath();
    virtual QPainterPath getWaterPainterPath(Water water);
    virtual void drawIconBeforeText(QPainter &painter, QRect icon_rect);

    QRectF getUnifiedGeometry();
    void updateUnifiedGeometry();
    void paintWaterRipple(QPainter &painter);
    void setJitter();

    int getFontSizeT();
    void setFontSizeT(int f);

    int max(int a, int b) const;
    int min(int a, int b) const;
    int quick_sqrt(long X) const;
    qint64 getTimestamp() const;
    bool isLightColor(QColor color);
    int getSpringBackProgress(int x, int max);
    QColor getOpacityColor(QColor color, double level = 0.5);
    QPixmap getMaskPixmap(QPixmap p, QColor c);

    double getNolinearProg(int p, NolinearType type);
    QIcon::Mode getIconMode();
    void reloadSvgColor();

signals:
    void showAniFinished();
    void hideAniFinished();
    void pressAppearAniFinished();
    void pressDisappearAniFinished();
    void jitterAniFinished();
    void doubleClicked();
    void rightClicked();
    void signalFocusIn();
    void signalFocusOut();

    void signalMouseEnter();
    void signalMouseEnterLater(); // 进入后延迟信号（以渐变动画完成为准，相当于可手动设置）
    void signalMouseLeave();
    void signalMouseLeaveLater(); // 离开后延迟的信号（直至渐变动画完成（要是划过一下子离开，这个也会变快））
    void signalMousePress(QMouseEvent *event);
    void signalMousePressLater(QMouseEvent *event);
    void signalMouseRelease(QMouseEvent *event);
    void signalMouseReleaseLater(QMouseEvent *event);

public slots:
    virtual void anchorTimeOut();
    virtual void slotClicked();
    void slotCloseState();

protected:
    PaintModel model;
    QIcon icon;
    QString svg_path;
    QString text;
    QPixmap pixmap;
    QSvgRenderer* svg_render = nullptr;
    PaintAddin paint_addin;
    EdgeVal fore_paddings;

protected:
    // 总体开关
    bool self_enabled, parent_enabled, fore_enabled; // 是否启用子类、启动父类、绘制子类前景

    // 出现前景的动画
    bool show_animation, show_foreground;
    bool show_ani_appearing, show_ani_disappearing;
    int show_duration;
    qint64 show_timestamp, hide_timestamp;
    int show_ani_progress;
    QPointF show_ani_point;
    QRectF paint_rect;

    // 鼠标开始悬浮、按下、松开、离开的坐标和时间戳
    // 鼠标锚点、目标锚点、当前锚点的坐标；当前XY的偏移量
    QPointF enter_pos, press_pos, release_pos, mouse_pos, anchor_pos /*目标锚点渐渐靠近鼠标*/;
    QPointF offset_pos /*当前偏移量*/, effect_pos, release_offset;                // 相对中心、相对左上角、弹起时的平方根偏移
    bool hovering, pressing;                                                     // 是否悬浮和按下的状态机
    qint64 hover_timestamp, leave_timestamp, press_timestamp, release_timestamp; // 各种事件的时间戳
    int hover_bg_duration, press_bg_duration, click_ani_duration;                // 各种动画时长

    // 定时刷新界面（保证动画持续）
    QTimer *anchor_timer;
    int move_speed;

    // 背景与前景
    QColor icon_color, text_color;                   // 前景颜色
    QColor normal_bg, hover_bg, press_bg, border_bg; // 各种背景颜色
    QColor focus_bg, focus_border;                   // 有焦点的颜色
    int hover_speed, press_start, press_speed;       // 颜色渐变速度
    int hover_progress, press_progress;              // 颜色渐变进度
    double icon_padding_proper;                      // 图标的大小比例
    int icon_text_padding, icon_text_size;           // 图标+文字模式共存时，两者间隔、图标大小
    int border_width;
    int radius_x, radius_y;
    int font_size;
    bool fixed_fore_pos;    // 鼠标进入时是否固定文字位置
    bool fixed_fore_size;   // 鼠标进入/点击时是否固定前景大小
    bool text_dynamic_size; // 设置字体时自动调整最小宽高
    bool auto_text_color;   // 动画时是否自动调整文字颜色
    bool focusing;          // 是否获得了焦点

    // 鼠标单击动画
    bool click_ani_appearing, click_ani_disappearing; // 是否正在按下的动画效果中
    int click_ani_progress;                           // 按下的进度（使用时间差计算）
    QMouseEvent *mouse_press_event, *mouse_release_event;

    // 统一绘制图标的区域（从整个按钮变为中心三分之二，并且根据偏移计算）
    bool unified_geometry; // 上面用不到的话，这个也用不到……
    double _l, _t, _w, _h;

    // 鼠标拖拽弹起来回抖动效果
    bool jitter_animation;      // 是否开启鼠标松开时的抖动效果
    double elastic_coefficient; // 弹性系数
    QList<Jitter> jitters;
    int jitter_duration; // 抖动一次，多次效果叠加

    // 鼠标按下水波纹动画效果
    bool water_animation; // 是否开启水波纹动画
    QList<Water> waters;
    int water_press_duration, water_release_duration, water_finish_duration;
    int water_radius;

    // 其他效果
    Qt::Alignment align;      // 文字/图标对其方向
    bool _state;              // 一个记录状态的变量，比如是否持续
    bool leave_after_clicked; // 鼠标单击松开后取消悬浮效果（针对菜单、弹窗），按钮必定失去焦点
    bool _block_hover;        // 如果有出现动画，临时屏蔽hovering效果

    // 双击
    bool double_clicked;  // 开启双击
    QTimer *double_timer; // 双击时钟
    bool double_prevent;  // 双击阻止单击release的flag
};

#endif // INTERACTIVEBUTTONBASE_H
