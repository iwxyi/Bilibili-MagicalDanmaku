#include "interactivebuttonbase.h"

/**
 * 所有内容的初始化
 * 如果要自定义，可以在这里调整所有的默认值
 */
InteractiveButtonBase::InteractiveButtonBase(QWidget *parent)
    : QPushButton(parent), icon(nullptr), text(""), paint_addin(),
      fore_paddings(4, 4, 4, 4),
      self_enabled(true), parent_enabled(false), fore_enabled(true),
      show_animation(false), show_foreground(true), show_ani_appearing(false), show_ani_disappearing(false),
      show_duration(300), show_timestamp(0), hide_timestamp(0), show_ani_progress(0), show_ani_point(0, 0),
      enter_pos(-1, -1), press_pos(-1, -1), release_pos(-1, -1), mouse_pos(-1, -1), anchor_pos(-1, -1),
      offset_pos(0, 0), effect_pos(-1, -1), release_offset(0, 0),
      hovering(false), pressing(false),
      hover_timestamp(0), leave_timestamp(0), press_timestamp(0), release_timestamp(0),
      hover_bg_duration(300), press_bg_duration(300), click_ani_duration(300),
      move_speed(5),
      icon_color(0, 0, 0), text_color(0, 0, 0),
      normal_bg(0xF2, 0xF2, 0xF2, 0), hover_bg(128, 128, 128, 32), press_bg(128, 128, 128, 64), border_bg(0, 0, 0, 0),
      focus_bg(0, 0, 0, 0), focus_border(0, 0, 0, 0),
      hover_speed(5), press_start(40), press_speed(5),
      hover_progress(0), press_progress(0), icon_padding_proper(0.2), icon_text_padding(4), icon_text_size(16),
      border_width(1), radius_x(0), radius_y(0),
      font_size(0), fixed_fore_pos(false), fixed_fore_size(false), text_dynamic_size(false), auto_text_color(true), focusing(false),
      click_ani_appearing(false), click_ani_disappearing(false), click_ani_progress(0),
      mouse_press_event(nullptr), mouse_release_event(nullptr),
      unified_geometry(false), _l(0), _t(0), _w(32), _h(32),
      jitter_animation(true), elastic_coefficient(1.2), jitter_duration(300),
      water_animation(true), water_press_duration(800), water_release_duration(400), water_finish_duration(300),
      align(Qt::AlignCenter), _state(false), leave_after_clicked(false), _block_hover(false),
      double_clicked(false), double_timer(nullptr), double_prevent(false)
{
    setMouseTracking(true); // 鼠标没有按下时也能捕获移动事件

    model = PaintModel::None;

    anchor_timer = new QTimer(this);
    anchor_timer->setInterval(10);
    connect(anchor_timer, SIGNAL(timeout()), this, SLOT(anchorTimeOut()));

    setWaterRipple();

    connect(this, SIGNAL(clicked()), this, SLOT(slotClicked()));

    setFocusPolicy(Qt::NoFocus); // 避免一个按钮还获取Tab键焦点
}

/**
 * 文字类型的按钮
 */
InteractiveButtonBase::InteractiveButtonBase(QString text, QWidget *parent)
    : InteractiveButtonBase(parent)
{
    setText(text);
}

/**
 * 图标类型的按钮
 */
InteractiveButtonBase::InteractiveButtonBase(QIcon icon, QWidget *parent)
    : InteractiveButtonBase(parent)
{
    setIcon(icon);
}

/**
 * 变色图标类型的按钮
 */
InteractiveButtonBase::InteractiveButtonBase(QPixmap pixmap, QWidget *parent)
    : InteractiveButtonBase(parent)
{
    setPixmap(pixmap);
}

InteractiveButtonBase::InteractiveButtonBase(QIcon icon, QString text, QWidget *parent)
    : InteractiveButtonBase(parent)
{
    setIcon(icon);
    setText(text);
}

InteractiveButtonBase::InteractiveButtonBase(QPixmap pixmap, QString text, QWidget *parent)
    : InteractiveButtonBase(parent)
{
    setPixmap(pixmap);
    setText(text);
}

/**
 * 设置按钮文字
 * @param text 按钮文字
 */
void InteractiveButtonBase::setText(QString text)
{
    this->text = text;
    if (model == PaintModel::None)
    {
        model = PaintModel::Text;
        setAlign(Qt::AlignCenter);
    }
    else if (model == PaintModel::PixmapMask)
    {
        if (pixmap.isNull())
            model = PaintModel::Text;
        else
            model = PaintModel::PixmapText;
        setAlign(Qt::AlignLeft | Qt::AlignVCenter);
        QFontMetrics fm(this->font());
        icon_text_size = fm.lineSpacing();
    }
    else if (model == PaintModel::Icon)
    {
        if (text.isEmpty())
        {
            model = PaintModel::Icon;
            setAlign(Qt::AlignCenter);
        }
        else
        {
            if (icon.isNull())
                model = PaintModel::Text;
            else
                model = PaintModel::IconText;
            setAlign(Qt::AlignLeft | Qt::AlignVCenter);
            QFontMetrics fm(this->font());
            icon_text_size = fm.lineSpacing();
        }
    }

    if (parent_enabled)
        QPushButton::setText(text);

    // 根据字体调整大小
    if (text_dynamic_size)
    {
        adjustMinimumSize();
    }
    update();
}

/**
 * 设置 icon 图标
 * @param path 图标路径文本
 */
void InteractiveButtonBase::setIconPath(QString path)
{
    setIcon(QIcon(path));
}

/**
 * 设置 pixmap 图标
 * @param path 图标路径文本
 */
void InteractiveButtonBase::setPixmapPath(QString path)
{
    setPixmap(QPixmap(path));
}

/**
 * 设置 icon
 * @param icon 图标
 */
void InteractiveButtonBase::setIcon(QIcon icon)
{
    if (model == PaintModel::None)
    {
        model = PaintModel::Icon;
        setAlign(Qt::AlignCenter);
    }
    else if (model == PaintModel::Text)
    {
        if (text.isEmpty())
            model = PaintModel::Icon;
        else
            model = PaintModel::IconText;
        setAlign(Qt::AlignLeft | Qt::AlignVCenter);
        QFontMetrics fm(this->font());
        icon_text_size = fm.lineSpacing();
    }
    else if (model == PaintModel::PixmapMask)
    {
        pixmap = QPixmap();
        model = PaintModel::Icon;
    }
    else if (model == PaintModel::PixmapText)
    {
        pixmap = QPixmap();
        if (text.isEmpty())
            model = PaintModel::Icon;
        else
            model = PaintModel::IconText;
        setAlign(Qt::AlignLeft | Qt::AlignVCenter);
        QFontMetrics fm(this->font());
        icon_text_size = fm.lineSpacing();
    }
    else if (model == PaintModel::IconText && text.isEmpty())
    {
        model = PaintModel::Icon;
        setAlign(Qt::AlignCenter);
    }
    this->icon = icon;
    this->svg_path = "";
    this->svg_render = nullptr;
    if (parent_enabled)
        QPushButton::setIcon(icon);
    update();
}

/**
 * 设置 Pixmap
 * @param pixmap [description]
 */
void InteractiveButtonBase::setPixmap(QPixmap pixmap)
{
    if (model == PaintModel::None)
        model = PaintModel::PixmapMask;
    else if (model == PaintModel::Text)
    {
        if (text.isEmpty())
            model = PaintModel::PixmapMask;
        else
            model = PaintModel::PixmapText;
        setAlign(Qt::AlignLeft | Qt::AlignVCenter);
        QFontMetrics fm(this->font());
        icon_text_size = fm.lineSpacing();
    }
    else if (model == PaintModel::Icon)
    {
        icon = QIcon();
        model = PaintModel::PixmapMask;
    }
    else if (model == PaintModel::IconText)
    {
        icon = QIcon();
        if (text.isEmpty())
            model = PaintModel::PixmapMask;
        else
            model = PaintModel::PixmapText;
        setAlign(Qt::AlignLeft | Qt::AlignVCenter);
        QFontMetrics fm(this->font());
        icon_text_size = fm.lineSpacing();
    }
    this->pixmap = getMaskPixmap(pixmap, isEnabled() ? icon_color : getOpacityColor(icon_color));
    this->svg_path = "";
    this->svg_render = nullptr;
    if (parent_enabled)
        QPushButton::setIcon(QIcon(pixmap));
    update();
}

void InteractiveButtonBase::setSvgPath(QString path)
{
    svg_path = path;
    svg_render = new QSvgRenderer();
    reloadSvgColor();
    if (model == PaintModel::None)
        model = PaintModel::Icon;
    else if (model == PaintModel::Text)
        model = PaintModel::IconText;
    else
        model = PaintModel::Icon;
    icon = QIcon(path);
    update();
}

void InteractiveButtonBase::setPaintAddin(QIcon icon, Qt::Alignment align, QSize size)
{
    paint_addin = PaintAddin(icon, align, size);
    update();
}

/**
 * 设置额外的图标，例如角标
 * @param pixmap 图标
 * @param align  对齐方式
 * @param size   图标尺寸
 */
void InteractiveButtonBase::setPaintAddin(QPixmap pixmap, Qt::Alignment align, QSize size)
{
    QBitmap mask = pixmap.mask();
    pixmap.fill(icon_color);
    pixmap.setMask(mask);
    paint_addin = PaintAddin(pixmap, align, size);
    update();
}

void InteractiveButtonBase::setPaintAddinPadding(int horizonal, int vertival)
{
    paint_addin.padding.left = paint_addin.padding.right = horizonal;
    paint_addin.padding.top = paint_addin.padding.bottom = vertival;
}

void InteractiveButtonBase::setPaintAddinPadding(int left, int top, int right, int bottom)
{
    paint_addin.padding.left = left;
    paint_addin.padding.top = top;
    paint_addin.padding.right = right;
    paint_addin.padding.bottom = bottom;
}

/**
 * 设置子类功能是否开启
 * 如果关闭，则相当于默认的 QPushButton
 * @param e 开关
 */
void InteractiveButtonBase::setSelfEnabled(bool e)
{
    self_enabled = e;
}

/**
 * 设置父类（QPushButton）功能是否开启
 * 如果开启，则绘制父类背景、父类前景
 * @param e 开关
 */
void InteractiveButtonBase::setParentEnabled(bool e)
{
    parent_enabled = e;

    // 传递子类内容到父类去，避免子类关掉后不显示
    if (model == PaintModel::Text || model == PaintModel::IconText || model == PaintModel::PixmapText)
        QPushButton::setText(text);
    if (model == PaintModel::Icon || model == PaintModel::IconText)
        QPushButton::setIcon(icon);
    if (model == PaintModel::PixmapMask || model == PaintModel::PixmapText)
        QPushButton::setIcon(QIcon(pixmap));
}

/**
 * 设置是否绘制前景图标/文字
 * 关闭后则只绘制背景
 * @param e 开启
 */
void InteractiveButtonBase::setForeEnabled(bool e)
{
    fore_enabled = e;
}

/**
 * 设置鼠标悬浮背景渐变的动画时长
 * @param d 动画时长（毫秒）
 */
void InteractiveButtonBase::setHoverAniDuration(int d)
{
    this->hover_bg_duration = d;
    //    hover_progress = 0; // 重置hover效果
}

/**
 * 设置鼠标按下渐变效果的动画时长
 * @param d 动画时长（毫秒）
 */
void InteractiveButtonBase::setPressAniDuration(int d)
{
    this->press_bg_duration = d;
}

/**
 * 设置单击效果的动画时长
 * @param d 动画时长（毫秒）
 */
void InteractiveButtonBase::setClickAniDuration(int d)
{
    this->click_ani_duration = d;
}

/**
 * 设置水波纹动画时长
 * @param press   按住时时长（时长毫秒）
 * @param release 松开后速度（时长毫秒）
 * @param finish  渐变消失速度（时长毫秒）
 */
void InteractiveButtonBase::setWaterAniDuration(int press, int release, int finish)
{
    this->water_press_duration = press;
    this->water_release_duration = release;
    this->water_finish_duration = finish;
}

/**
 * 各种状态改变
 * 主要是监控 可用 状态，不可用时设置为半透明
 */
void InteractiveButtonBase::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    if (event->type() == QEvent::EnabledChange && model == PixmapMask) // 可用状态改变了
    {
        if (isEnabled()) // 恢复可用：透明度变回去
        {
            QColor color = icon_color;
            color.setAlpha(color.alpha() * 2);
            setIconColor(color);
        }
        else // 变成不可用：透明度减半
        {
            QColor color = icon_color;
            color.setAlpha(color.alpha() / 2);
            setIconColor(color);
        }
    }
}

/**
 * 设置水波纹动画是否开启
 * 关闭时，将使用渐变动画
 * @param enable 开关
 */
void InteractiveButtonBase::setWaterRipple(bool enable)
{
    if (water_animation == enable)
        return;
    water_animation = enable;
}

/**
 * 设置抖动效果是否开启
 * 鼠标拖拽移动的距离越长，抖动距离越长、次数越多
 * @param enable 开关
 */
void InteractiveButtonBase::setJitterAni(bool enable)
{
    jitter_animation = enable;
}

/**
 * 设置是否使用统一图标绘制区域
 * 监听图标尺寸大小变化、中心点偏移，计算新的中心坐标位置
 * @param enable 开关
 */
void InteractiveButtonBase::setUnifyGeomerey(bool enable)
{
    unified_geometry = enable;
    _l = _t = 0;
    _w = width();
    _h = height();
}

/**
 * 设置背景颜色
 * @param bg 背景颜色
 */
void InteractiveButtonBase::setBgColor(QColor bg)
{
    setNormalColor(bg);
}

/**
 * 设置事件背景颜色
 * @param hover 鼠标悬浮时的背景颜色
 * @param press 鼠标按下时的背景颜色
 */
void InteractiveButtonBase::setBgColor(QColor hover, QColor press)
{
    if (hover != Qt::black)
        setHoverColor(hover);
    if (press != Qt::black)
        setPressColor(press);
}

/**
 * 设置按钮背景颜色
 * @param color 背景颜色
 */
void InteractiveButtonBase::setNormalColor(QColor color)
{
    normal_bg = color;
    update();
}

/**
 * 设置边框线条颜色
 * @param color 边框颜色
 */
void InteractiveButtonBase::setBorderColor(QColor color)
{
    border_bg = color;
    update();
}

/**
 * 设置鼠标悬浮时的背景颜色
 * @param color 背景颜色
 */
void InteractiveButtonBase::setHoverColor(QColor color)
{
    hover_bg = color;
    update();
}

/**
 * 设置鼠标按住时的背景颜色
 * @param color 背景颜色
 */
void InteractiveButtonBase::setPressColor(QColor color)
{
    press_bg = color;
    update();
}

/**
 * 设置图标颜色（仅针对可变色的 pixmap 图标）
 * @param color 图标颜色
 */
void InteractiveButtonBase::setIconColor(QColor color)
{
    icon_color = color;

    // 绘制图标（如果有）
    if (model == PaintModel::PixmapMask || model == PaintModel::PixmapText)
    {
        pixmap = getMaskPixmap(pixmap, isEnabled() ? icon_color : getOpacityColor(icon_color));
    }

    // 修改svg的颜色
    if (svg_render)
    {
        reloadSvgColor();
    }

    // 绘制额外角标（如果有的话）
    if (paint_addin.enable)
    {
        if (!paint_addin.pixmap.isNull())
        {
            paint_addin.pixmap = getMaskPixmap(paint_addin.pixmap, isEnabled() ? icon_color : getOpacityColor(icon_color));
        }
    }

    update();
}

/**
 * 设置前景文字颜色
 * @param color 文字颜色
 */
void InteractiveButtonBase::setTextColor(QColor color)
{
    text_color = color;
    update();
}

/**
 * 设置获取焦点时的背景颜色（默认关闭）
 * @param color 背景颜色
 */
void InteractiveButtonBase::setFocusBg(QColor color)
{
    setFocusPolicy(Qt::StrongFocus);
    focus_bg = color;
}

/**
 * 设置获取焦点时的边框颜色（默认关闭）
 * @param color 边框颜色
 */
void InteractiveButtonBase::setFocusBorder(QColor color)
{
    setFocusPolicy(Qt::StrongFocus);
    focus_border = color;
}

/**
 * 设置文字大小（PointSize，覆盖 font() 字体大小）
 * @param f 文字大小
 */
void InteractiveButtonBase::setFontSize(int f)
{
    if (!font_size) // 第一次设置字体大小，直接设置
    {
        font_size = f;
        QFont font(this->font());
        font.setPointSize(f);
        setFont(font);
        update();
    }
    else // 改变字体大小，使用字体缩放动画
    {
        QPropertyAnimation *ani = new QPropertyAnimation(this, "font_size");
        ani->setStartValue(font_size);
        ani->setEndValue(f);
        ani->setDuration(click_ani_duration);
        connect(ani, &QPropertyAnimation::finished, this, [=] {
            QFontMetrics fm(this->font());
            icon_text_size = fm.lineSpacing();
            ani->deleteLater();
        });
        ani->start();
    }
    // 修改字体大小时调整按钮的最小尺寸，避免文字显示不全
    if (text_dynamic_size)
    {
        QFont font;
        font.setPointSize(f);
        QFontMetrics fms(font);
        setMinimumSize(fms.horizontalAdvance(text) + fore_paddings.left + fore_paddings.right, fms.lineSpacing() + fore_paddings.top + fore_paddings.bottom);
    }
    if (model != PaintModel::Text)
    {
        QFontMetrics fm(this->font());
        icon_text_size = fm.lineSpacing();
    }
}

/**
 * 获取字体大小
 * 用来作为字体动画的属性参数
 * @return 临时字体大小
 */
int InteractiveButtonBase::getFontSizeT()
{
    return font_size;
}

/**
 * 设置动画中的临时字体大小
 * 用来作为字体动画的属性参数
 * @param f 临时字体大小
 */
void InteractiveButtonBase::setFontSizeT(int f)
{
    this->font_size = f;
    QFont font(this->font());
    font.setPointSize(f);
    setFont(font);
    update();
}

/**
 * 如果点击失去焦点的话，即使鼠标移到上面，也不会出现背景
 * 可以用这个方法继续保持悬浮状态
 */
void InteractiveButtonBase::setHover()
{
    if (!hovering && inArea(mapFromGlobal(QCursor::pos())))
        InteractiveButtonBase::enterEvent(new QEvent(QEvent::Type::None));
}

/**
 * 设置对齐方式
 * @param a 对齐方式
 */
void InteractiveButtonBase::setAlign(Qt::Alignment a)
{
    align = a;
    update();
}

/**
 * 设置四个角的半径
 * @param r 半径
 */
void InteractiveButtonBase::setRadius(int r)
{
    radius_x = radius_y = r;
}

/**
 * 分开设置 X、Y 的半径
 * @param rx X半径
 * @param ry Y半径
 */
void InteractiveButtonBase::setRadius(int rx, int ry)
{
    radius_x = rx;
    radius_y = ry;
}

/**
 * 设置边框线条的粗细
 * @param x 线条粗细
 */
void InteractiveButtonBase::setBorderWidth(int x)
{
    border_width = x;
}

/**
 * 设置不可用情况（默认为假）
 * 区别于 setEnabled(bool)，两个相反的，并不是覆盖方法
 * @param dis 不可用
 */
void InteractiveButtonBase::setDisabled(bool dis)
{
    if (dis == !isEnabled()) // 相同的
        return;

    setEnabled(!dis);

    if (parentWidget() != nullptr)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, dis); // 点击穿透
    }

    if (model == PixmapMask || model == PixmapText)
    {
        pixmap = getMaskPixmap(pixmap, dis ? getOpacityColor(icon_color) : icon_color);
    }

    update(); // 修改透明度
}

/**
 * 设置前景和四条边的 paddings
 * @param l 左边空白
 * @param r 右边空白
 * @param t 顶边空白
 * @param b 底边空白
 */
void InteractiveButtonBase::setPaddings(int l, int r, int t, int b)
{
    fore_paddings.left = l;
    fore_paddings.right = r;
    fore_paddings.top = t;
    fore_paddings.bottom = b;
    setFixedForeSize();
}

/**
 * 统一设置方向的 paddings
 * @param h 横向
 * @param v 纵向
 */
void InteractiveButtonBase::setPaddings(int h, int v)
{
    fore_paddings.left = fore_paddings.right = (h + 1) / 2;
    fore_paddings.top = fore_paddings.bottom = (v + 1) / 2;
    setFixedForeSize();
}

/**
 * 统一设置前景和四条边的 paddings
 * @param x 一样大小的四边留白
 */
void InteractiveButtonBase::setPaddings(int x)
{
    fore_paddings.left = x;
    fore_paddings.right = x;
    fore_paddings.top = x;
    fore_paddings.bottom = x;
    setFixedForeSize();
}

/**
 * 设置Icon模式旁边空多少
 * @param x 0~1.0，越大越空
 */
void InteractiveButtonBase::setIconPaddingProper(double x)
{
    icon_padding_proper = x;
    int short_side = min(width(), height()); // 短边
    // 非固定的情况，尺寸大小变了之后所有 padding 都要变
    int padding = int(short_side * icon_padding_proper); //static_cast<int>(short_side * (1 - GOLDEN_RATIO) / 2);
    fore_paddings.left = fore_paddings.top = fore_paddings.right = fore_paddings.bottom = padding;
    update();
}

/**
 * 设置字体大小时是否同步修改按钮的最小尺寸（避免按钮显示不全）
 * @param d 开关
 */
void InteractiveButtonBase::setTextDynamicSize(bool d)
{
    text_dynamic_size = d;
}

/**
 * 见 setFixedForePos(bool f)
 */
void InteractiveButtonBase::setFixedTextPos(bool f)
{
    fixed_fore_pos = f;
}

/**
 * 设置前景是否固定，而不移动
 * 将去除鼠标移入靠近、抖动效果，统一图标区域大小不变
 * 只包括：鼠标进入/点击，均表现为缩放效果（默认）
 * 不影响任何其他功能
 * @param f [description]
 */
void InteractiveButtonBase::setFixedForePos(bool f)
{
    fixed_fore_pos = f;
}

/**
 * 固定按钮(最小值)为适当尺寸，并且固定四周留白
 * 前景应为文字/图标对应尺寸的最小尺寸
 * @param f     是否固定前景
 * @param addin 留白的像素大小
 */
void InteractiveButtonBase::setFixedForeSize(bool f, int addin)
{
    fixed_fore_size = f;

    if (!f)
        return;
    if (model == PaintModel::Text || model == PaintModel::IconText || model == PaintModel::PixmapText)
    {
        int icon_width = (model != PaintModel::Text && icon.isNull()) ? 0 : icon_text_size;
        QFont font = this->font();
        if (font_size > 0)
            font.setPointSize(font_size);
        QFontMetrics fm(font);
        int w = fm.horizontalAdvance(text);
        w = icon_width + w + quick_sqrt(w / 2) + fore_paddings.left + fore_paddings.right;
        setMinimumSize(
            w + addin,
            fm.lineSpacing() + fore_paddings.top + fore_paddings.bottom + addin);
    }
    else if (model == PaintModel::Icon || model == PaintModel::PixmapMask)
    {
        int size = height();
        setMinimumSize(size + addin, size + addin);
    }
}

void InteractiveButtonBase::setSquareSize()
{
    setFixedWidth(height());
    setMinimumWidth(height());
    setMaximumWidth(height());
}

/**
 * 设置鼠标单击松开后是否当做移开
 * 避免菜单、弹窗出现后，由于鼠标仍然留在按钮上面，导致依旧显示 hover 背景
 * @param l 开关
 */
void InteractiveButtonBase::setLeaveAfterClick(bool l)
{
    leave_after_clicked = l;
}

/**
 * 响应双击事件
 * 注意：会先触发单击事件、再触发双击事件(其实就是懒得做)
 * 建议在 QListWidget 等地方使用！
 * @param e 开关
 */
void InteractiveButtonBase::setDoubleClicked(bool e)
{
    double_clicked = e;

    if (double_timer == nullptr)
    {
        double_timer = new QTimer(this);
        double_timer->setInterval(DOUBLE_PRESS_INTERVAL);
        connect(double_timer, &QTimer::timeout, [=] {
            double_timer->stop();
            emit clicked(); // 手动触发单击事件
        });
    }
}

/**
 * 动画时是否自动设置文字的颜色
 */
void InteractiveButtonBase::setAutoTextColor(bool a)
{
    this->auto_text_color = a;
}

/**
 * 一开始没有聚焦时，假装获取焦点
 * 通过信号槽使其他控件（例如QLineEdit）按下enter键触发此按钮事件
 * 直到触发了焦点改变事件，此控件失去焦点（需要手动改变）
 */
void InteractiveButtonBase::setPretendFocus(bool f)
{
    focusing = f;
    update();
}

/**
 * 如果按钮被做成一个组合，在显示的时候开启动画
 * 一开始鼠标下的按钮一直在hover状态，移开也不会变
 * 开启后临时屏蔽，记得在动画结束后关闭
 */
void InteractiveButtonBase::setBlockHover(bool b)
{
    _block_hover = b;
    if (b && hovering)
        leaveEvent(nullptr);
}

/**
 * 是否开启出现动画
 * 鼠标进入按钮区域，前景图标从对面方向缩放出现
 * @param enable 开关
 */
void InteractiveButtonBase::setShowAni(bool enable)
{
    show_animation = enable;

    if (!show_animation) // 关闭隐藏前景
    {
        show_foreground = true;
    }
    else if (show_animation) // 开启隐藏前景
    {
        if (!hovering && !pressing) // 应该是隐藏状态
        {
            show_ani_appearing = show_ani_disappearing = show_foreground = false;
            show_ani_progress = 0;
        }
        else // 应该是显示状态
        {
            show_foreground = true;
            show_ani_appearing = show_ani_disappearing = false;
            show_ani_progress = 100;
        }
    }
}

/**
 * 按钮前景出现动画
 * 从中心点出现的缩放动画
 */
void InteractiveButtonBase::showForeground()
{
    if (!show_animation)
        return;
    waters.clear();
    if (!anchor_timer->isActive())
        anchor_timer->start();
    if (show_ani_disappearing)
        show_ani_disappearing = false;
    show_ani_appearing = true;
    show_timestamp = getTimestamp();
    show_foreground = true;
    show_ani_point = QPoint(0, 0);
}

/**
 * 按钮前景出现动画2
 * 指定方向（笛卡尔坐标），从反方向至中心点
 * @param point 最开始出现的方向（大小不影响，只按 x、y 比例来）
 */
void InteractiveButtonBase::showForeground2(QPoint point)
{
    showForeground();
    if (point == QPoint(0, 0))
        point = mapFromGlobal(QCursor::pos()) - QPoint(width() / 2, height() / 2); // 相对于按钮中心
    show_ani_point = point;

    if (unified_geometry) // 统一出现动画
        updateUnifiedGeometry();
}

/**
 * 隐藏前景
 * 为下一次的出现动画做准备
 */
void InteractiveButtonBase::hideForeground()
{
    if (!show_animation)
        return;
    if (!anchor_timer->isActive())
        anchor_timer->start();
    if (show_ani_appearing)
        show_ani_appearing = false;
    show_ani_disappearing = true;
    hide_timestamp = getTimestamp();
}

/**
 * 延迟出现前景
 * 适用于多个按钮连续出现的一套效果
 * @param time  延迟时长（毫秒）
 * @param point 出现方向
 */
void InteractiveButtonBase::delayShowed(int time, QPoint point)
{
    setShowAni(true);
    QTimer::singleShot(time, [=] {
        showForeground2(point);
        connect(this, &InteractiveButtonBase::showAniFinished, [=] {
            setShowAni(false);
            disconnect(this, SIGNAL(showAniFinished()), nullptr, nullptr);
        });
    });
}

/**
 * 获取文字
 */
QString InteractiveButtonBase::getText()
{
    return text;
}

/**
 * 设置菜单
 * 并解决菜单无法监听到 release 的问题
 * @param menu 菜单对象
 */
void InteractiveButtonBase::setMenu(QMenu *menu)
{
    // 默认设置了不获取焦点事件，所以如果设置了菜单的话，就不会有Release事件，水波纹动画会一直飘荡
    // 在 focusOut 事件中，模拟了 release 事件，
    this->setFocusPolicy(Qt::FocusPolicy::ClickFocus);

    QPushButton::setMenu(menu);
}

/**
 * 根据内容，调整最小尺寸（不影响最大值）
 */
void InteractiveButtonBase::adjustMinimumSize()
{
    int icon_width = (model == PaintModel::Text || model == PaintModel::None) || icon.isNull() ? 0 : icon_text_size;
    int w = 0, h = 0;
    if (font_size <= 0)
    {
        QFontMetrics fm(font());
        w = fm.horizontalAdvance(text);
        w = icon_width + w + quick_sqrt(w / 2) + fore_paddings.left + fore_paddings.right;
        h = fm.lineSpacing() + fore_paddings.top + fore_paddings.bottom;
    }
    else
    {
        QFont font;
        font.setPointSize(font_size);
        QFontMetrics fm(font);
        w = fm.horizontalAdvance(text);
        w = icon_width + w + quick_sqrt(w / 2) + fore_paddings.left + fore_paddings.right;
        h = fm.lineSpacing() + fore_paddings.top + fore_paddings.bottom;
    }
    setMinimumSize(w, h);
}

/**
 * 设置状态
 * 一个用来作为开关效果的属性
 * @param s 状态
 */
void InteractiveButtonBase::setState(bool s)
{
    _state = s;
    update();
}

/**
 * 获取状态
 * @return 状态
 */
bool InteractiveButtonBase::getState()
{
    return _state;
}

/**
 * 模拟按下开关的效果，并改变状态
 * 如果不使用状态，则出现点击动画
 * @param s 目标状态（默认为false）
 * @param a 鼠标在区域内则点击无效（恐怕再次点击）
 */
void InteractiveButtonBase::simulateStatePress(bool s, bool a)
{
    if (getState() == s)
        return;

    // 鼠标悬浮在上方，有两种情况：
    // 1、点击按钮后触发，重复了
    // 2、需要假装触发，例如 Popup 类型，尽管悬浮在上面，但是无法点击到
    if (a && inArea(mapFromGlobal(QCursor::pos()))) // 点击当前按钮，不需要再模拟了
        return;

    mousePressEvent(new QMouseEvent(QMouseEvent::Type::None, QPoint(width() / 2, height() / 2), Qt::LeftButton, Qt::NoButton, Qt::NoModifier));

    mouseReleaseEvent(new QMouseEvent(QMouseEvent::Type::None, QPoint(width() / 2, height() / 2), Qt::LeftButton, Qt::NoButton, Qt::NoModifier));

    // if (!inArea(mapFromGlobal(QCursor::pos()))) // 针对模拟release 后面 // 必定成立
    hovering = false;
}

/**
 * 模拟鼠标悬浮的效果
 * 适用于键盘操作时，模拟鼠标hover状态
 * 用 discardHoverPress 取消状态
 */
void InteractiveButtonBase::simulateHover()
{
    if (!hovering)
    {
        if (_block_hover)
            setBlockHover(false); // 可能已经临时屏蔽掉鼠标 enter 事件，强制hover
        enterEvent(nullptr);
    }
}

/**
 * 强制丢弃hover、press状态
 * 适用于悬浮/点击后，弹出模态浮窗
 * 浮窗关闭后调用此方法
 * @param force 如果鼠标仍在此按钮内，是否强制取消hover/press状态
 */
void InteractiveButtonBase::discardHoverPress(bool force)
{
    if (!force && inArea(mapFromGlobal(QCursor::pos()))) // 鼠标还在这范围内
        return;

    if (hovering)
    {
        leaveEvent(nullptr);
    }

    if (pressing)
    {
        mouseReleaseEvent(new QMouseEvent(QMouseEvent::Type::None, QPoint(width() / 2, height() / 2), Qt::LeftButton, Qt::NoButton, Qt::NoModifier));
    }
}

/**
 * 鼠标移入事件，触发 hover 时间戳
 */
void InteractiveButtonBase::enterEvent(QEvent *event)
{
    if (_block_hover) // 临时屏蔽hover事件
    {
        if (event)
            event->accept();
        return;
    }

    if (!anchor_timer->isActive())
    {
        anchor_timer->start();
    }
    hovering = true;
    hover_timestamp = getTimestamp();
    leave_timestamp = 0;
    if (mouse_pos == QPoint(-1, -1))
        mouse_pos = mapFromGlobal(QCursor::pos());
    emit signalMouseEnter();

    return QPushButton::enterEvent(event);
}

/**
 * 鼠标移开事件，触发 leave 时间戳
 */
void InteractiveButtonBase::leaveEvent(QEvent *event)
{
    hovering = false;
    if (!pressing)
        mouse_pos = QPoint(width() / 2, height() / 2);
    emit signalMouseLeave();

    return QPushButton::leaveEvent(event);
}

/**
 * 鼠标按下事件，触发 press 时间戳
 * 添加水波纹动画 waters 队列
 */
void InteractiveButtonBase::mousePressEvent(QMouseEvent *event)
{
    mouse_pos = event->pos();

    if (event->button() == Qt::LeftButton)
    {
        if (!hovering)
            InteractiveButtonBase::enterEvent(new QEvent(QEvent::Type::None));

        pressing = true;
        press_pos = mouse_pos;
        // 判断双击事件
        if (double_clicked)
        {
            qint64 last_press_timestamp = press_timestamp;
            press_timestamp = getTimestamp();
            if (release_timestamp + DOUBLE_PRESS_INTERVAL >= press_timestamp && last_press_timestamp + SINGLE_PRESS_INTERVAL > release_timestamp && release_pos == press_pos) // 是双击(判断两次单击的间隔)
            {
                double_prevent = true; // 阻止本次的release识别为单击
                press_timestamp = 0;   // 避免很可能出现的三击、四击...
                double_timer->stop();  // 取消延迟一小会儿的单击信号
                emit doubleClicked();
                return;
            }
            else
            {
                double_prevent = false; // 避免有额外的 bug
            }
        }
        else
        {
            press_timestamp = getTimestamp();
        }

        if (water_animation)
        {
            if (waters.size() && waters.last().release_timestamp == 0) // 避免两个按键同时按下
                waters.last().release_timestamp = getTimestamp();
            waters << Water(press_pos, press_timestamp);
        }
        else // 透明渐变
        {
            if (press_progress < press_start)
                press_progress = press_start; // 直接设置为按下效果初始值（避免按下反应慢）
        }
    }
    mouse_press_event = event;
    emit signalMousePress(event);

    return QPushButton::mousePressEvent(event);
}

/**
 * 鼠标松开事件，触发 release 时间戳
 * 添加抖动动画 jitters 队列
 */
void InteractiveButtonBase::mouseReleaseEvent(QMouseEvent *event)
{
    if (pressing && event->button() == Qt::LeftButton)
    {
        if (!inArea(event->pos()) || leave_after_clicked)
        {
            hovering = false;
        }
        pressing = false;
        release_pos = event->pos();
        release_timestamp = getTimestamp();

        // 添加抖动效果
        if (jitter_animation)
        {
            setJitter();
        }

        if (water_animation && waters.size())
        {
            waters.last().release_timestamp = release_timestamp;
        }

        if (double_clicked)
        {
            if (double_prevent) // 双击的当次release，不参与单击计算
            {
                double_prevent = false;
                return;
            }

            // 应该不是双击的操作
            if (release_pos != press_pos || release_timestamp - press_timestamp >= SINGLE_PRESS_INTERVAL)
            {
            }
            else // 可能是双击，准备
            {
                double_timer->start();
                return; // 禁止单击事件
            }
        }
    }
    else if (leave_after_clicked && !pressing && double_clicked && double_prevent) // 双击，失去焦点了，pressing 丢失
    {
        return;
    }
    else if (event->button() == Qt::RightButton && event->buttons() == Qt::NoButton)
    {
        if ((release_pos - press_pos).manhattanLength() < QApplication::startDragDistance())
            emit rightClicked();
    }
    mouse_release_event = event;
    emit signalMouseRelease(event);

    return QPushButton::mouseReleaseEvent(event);
}

/**
 * 鼠标移动事件
 */
void InteractiveButtonBase::mouseMoveEvent(QMouseEvent *event)
{
    if (_block_hover) // 临时屏蔽hover事件
    {
        if (event)
            event->accept();
        return;
    }
    if (hovering == false) // 失去焦点又回来了
    {
        enterEvent(nullptr);
    }
    mouse_pos = mapFromGlobal(QCursor::pos());

    return QPushButton::mouseMoveEvent(event);
}

/**
 * 尺寸大小改变事件
 * 同步调整和尺寸有关的所有属性
 */
void InteractiveButtonBase::resizeEvent(QResizeEvent *event)
{
    if (!pressing && !hovering)
    {
        mouse_pos = QPoint(width() / 2, height() / 2);
        anchor_pos = mouse_pos;
    }
    water_radius = static_cast<int>(max(width(), height()) * 1.42); // 长边
    // 非固定的情况，尺寸大小变了之后所有 padding 都要变
    if (!fixed_fore_size && (model == PaintModel::Icon || model == PaintModel::PixmapMask))
    {
        int short_side = min(width(), height());        // 短边
        int padding = int(short_side * icon_padding_proper); //static_cast<int>(short_side * (1 - GOLDEN_RATIO) / 2);
        fore_paddings.left = fore_paddings.top = fore_paddings.right = fore_paddings.bottom = padding;
    }
    _l = _t = 0;
    _w = width();
    _h = height();

    return QPushButton::resizeEvent(event);
}

/**
 * 获得焦点事件
 * 已经取消按钮获取焦点，focusIn和focusOut事件都不会触发
 */
void InteractiveButtonBase::focusInEvent(QFocusEvent *event)
{
    if (!hovering && inArea(mapFromGlobal(QCursor::pos())))
        InteractiveButtonBase::enterEvent(new QEvent(QEvent::Type::None));

    focusing = true;
    emit signalFocusIn();

    return QPushButton::focusInEvent(event);
}

/**
 * 失去焦点事件
 * 兼容按住时突然失去焦点（例如弹出菜单、被其他窗口抢走了）
 */
void InteractiveButtonBase::focusOutEvent(QFocusEvent *event)
{
    if (hovering)
    {
        hovering = false;
    }
    if (pressing) // 鼠标一直按住，可能在click事件中移动了焦点
    {
        pressing = false;
        release_pos = mapFromGlobal(QCursor::pos());
        release_timestamp = getTimestamp();

        if (water_animation && waters.size())
        {
            waters.last().release_timestamp = release_timestamp;
        }
    }

    focusing = false;
    emit signalFocusOut();

    return QPushButton::focusOutEvent(event);
}

/**
 * 重绘事件
 * 绘制所有内容：背景、动画、前景、角标
 */
void InteractiveButtonBase::paintEvent(QPaintEvent *event)
{
    if (parent_enabled) // 绘制父类（以便使用父类的QSS和各项属性）
        QPushButton::paintEvent(event);
    if (!self_enabled) // 不绘制自己
        return;
    QPainter painter(this);

    // ==== 绘制背景 ====
    QPainterPath path_back = getBgPainterPath();
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (normal_bg.alpha() != 0) // 默认背景
    {
        painter.fillPath(path_back, isEnabled() ? normal_bg : getOpacityColor(normal_bg));
    }
    if (focusing && focus_bg.alpha() != 0) // 焦点背景
    {
        painter.fillPath(path_back, focus_bg);
    }

    if ((border_bg.alpha() != 0 || (focusing && focus_border.alpha() != 0)) && border_width > 0)
    {
        painter.save();
        QPen pen;
        pen.setColor((focusing && focus_border.alpha()) ? focus_border : border_bg);
        pen.setWidth(border_width);
        painter.setPen(pen);
        painter.drawPath(path_back);
        painter.restore();
    }

    if (hover_progress) // 悬浮背景
    {
        painter.fillPath(path_back, getOpacityColor(hover_bg, hover_progress / 100.0));
    }

    if (press_progress && !water_animation) // 按下渐变淡化消失
    {
        painter.fillPath(path_back, getOpacityColor(press_bg, press_progress / 100.0));
    }
    else if (water_animation && waters.size()) // 水波纹，且至少有一个水波纹
    {
        paintWaterRipple(painter);
    }

    // ==== 绘制前景 ====
    if (fore_enabled /*针对按钮设置*/ && show_foreground /*针对动画设置*/)
    {
        painter.setPen(isEnabled() ? icon_color : getOpacityColor(icon_color));

        // 绘制额外内容（可能被前景覆盖）
        if (paint_addin.enable)
        {
            int l = fore_paddings.left, t = fore_paddings.top, r = width() - fore_paddings.right, b = height() - fore_paddings.bottom;
            if (paint_addin.padding != EdgeVal(0, 0, 0, 0))
            {
                l = paint_addin.padding.left;
                t = paint_addin.padding.top;
                r = width() - paint_addin.padding.right;
                b = height() - paint_addin.padding.bottom;
            }
            int small_edge = min(height(), width());
            int pw = paint_addin.size.width() ? paint_addin.size.width() : small_edge * 4 / 5;
            int ph = paint_addin.size.height() ? paint_addin.size.height() : small_edge * 4 / 5;
            if (paint_addin.align & Qt::AlignLeft)
                r = l + pw;
            else if (paint_addin.align & Qt::AlignRight)
                l = r - pw;
            else if (paint_addin.align & Qt::AlignHCenter)
            {
                l = width() / 2 - pw / 2;
                r = l + pw;
            }
            if (paint_addin.align & Qt::AlignTop)
                b = t + ph;
            else if (paint_addin.align & Qt::AlignBottom)
                t = b - ph;
            else if (paint_addin.align & Qt::AlignVCenter)
            {
                t = height() / 2 - ph / 2;
                b = t + ph;
            }
            if (!paint_addin.icon.isNull())
            {
                painter.drawPixmap(QRect(l, t, r - l, b - t), paint_addin.icon.pixmap(paint_addin.size));
            }
            else if (!paint_addin.pixmap.isNull())
            {
                painter.drawPixmap(QRect(l, t, r - l, b - t), paint_addin.pixmap);
            }
        }

        QRectF &rect = paint_rect;
        rect = QRectF(fore_paddings.left + (fixed_fore_pos ? 0 : offset_pos.x()),
                     fore_paddings.top + (fixed_fore_pos ? 0 : offset_pos.y()), // 原来的位置，不包含点击、出现效果
                     width() - fore_paddings.left - fore_paddings.right,
                     height() - fore_paddings.top - fore_paddings.bottom);

        // 抖动出现动画
        if ((show_ani_appearing || show_ani_disappearing) && show_ani_point != QPoint(0, 0) && !fixed_fore_pos)
        {
            //int w = width(), h = height();
            int pro = getSpringBackProgress(show_ani_progress, 50);

            // show_ani_point 是鼠标进入的点，那么起始方向应该是相反的
            double x = show_ani_point.x(), y = show_ani_point.y();
            int gen = quick_sqrt(long(x * x + y * y));
            x = water_radius * x / gen; // 动画起始中心点横坐标 反向
            y = water_radius * y / gen; // 动画起始中心点纵坐标 反向

            rect = QRectF(
                rect.left() - x * (100 - pro) / 100 + rect.width() * (100 - pro) / 100,
                rect.top() - y * (100 - pro) / 100 + rect.height() * (100 - pro) / 100,
                rect.width() * pro / 100,
                rect.height() * pro / 100);
        }
        else if (align == Qt::AlignCenter && model != PaintModel::Text && !fixed_fore_size) // 默认的缩放动画
        {
            double delta_x = 0, delta_y = 0;
            if (click_ani_progress != 0) // 图标缩放
            {
                delta_x = rect.width() * click_ani_progress / 400;
                delta_y = rect.height() * click_ani_progress / 400;
            }
            else if (show_ani_appearing)
            {
                /*int pro; // 将动画进度转换为回弹动画进度
                if (show_ani_progress <= 50)
                    pro = show_ani_progress * 2;
                else if (show_ani_progress <= 75)
                    pro = (show_ani_progress-50)/2 + 100;
                else
                    pro = 100 + (100-show_ani_progress)/2;

                delta_x = rect.width() * (100-pro) / 100;
                delta_y = rect.height() * (100-pro) / 100;*/

                double pro = getNolinearProg(show_ani_progress, SpringBack50);
                delta_x = static_cast<int>(rect.width() * (1 - pro));
                delta_y = static_cast<int>(rect.height() * (1 - pro));
            }
            else if (show_ani_disappearing)
            {
                double pro = 1 - getNolinearProg(show_ani_progress, SlowFaster);
                delta_x = rect.width() * pro;  // (100-show_ani_progress) / 100;
                delta_y = rect.height() * pro; // (100-show_ani_progress) / 100;
            }
            if (int(delta_x+1e-6) || int(delta_y+1e-6))
                rect = QRectF(rect.left() + delta_x, rect.top() + delta_y,
                             rect.width() - delta_x * 2, rect.height() - delta_y * 2);
        }

        /*if (this->isEnabled())
        {
            QColor color = icon_color;
            color.setAlpha(color.alpha() / 2);
            painter.setPen(color);
        }*/

        if (model == None)
        {
            // 子类自己的绘制内容
        }
        else if (model == Text)
        {
            // 绘制文字教程： https://blog.csdn.net/temetnosce/article/details/78068464
            painter.setPen(isEnabled() ? text_color : getOpacityColor(text_color));
            /*if (show_ani_appearing || show_ani_disappearing)
            {
                int pro = getSpringBackProgress(show_ani_progress, 50);
                QFont font = painter.font();
                int ps = font.pointSize();
                ps = ps * show_ani_progress / 100;
                font.setPointSize(ps);
                painter.setFont(font);
            }*/
            if (font_size > 0)
            {
                QFont font = painter.font();
                font.setPointSize(font_size);
                painter.setFont(font);
            }
            painter.drawText(rect, static_cast<int>(align), text);
        }
        else if (model == Icon) // 绘制图标
        {
            if (svg_render)
            {
                svg_render->render(&painter, rect.toRect());
            }
            else
            {
                icon.paint(&painter, rect.toRect(), align, getIconMode());
            }
        }
        else if (model == PixmapMask)
        {
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true); // 可以让边缘看起来平滑一些
            painter.drawPixmap(rect.toRect(), pixmap);
        }
        else if (model == IconText || model == PixmapText) // 强制左对齐；左图标中文字
        {
            // 绘制图标
            int &sz = icon_text_size;
            QRectF icon_rect(rect.left(), rect.top() + rect.height() / 2 - sz / 2, sz, sz);
            icon_rect.moveTo(icon_rect.left() - quick_sqrt(long(offset_pos.x())), icon_rect.top() - quick_sqrt(long(offset_pos.y())));
            drawIconBeforeText(painter, icon_rect.toRect());
            rect.setLeft(rect.left() + sz + icon_text_padding);

            // 绘制文字
            // 扩展文字范围，确保文字可见
            painter.setPen(isEnabled() ? text_color : getOpacityColor(text_color));
            rect.setWidth(rect.width() + sz + icon_text_padding);
            if (font_size > 0)
            {
                QFont font = painter.font();
                font.setPointSize(font_size);
                painter.setFont(font);
            }
            painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
        }
    }

    // ==== 绘制鼠标位置 ====
    //    painter.drawEllipse(QRect(anchor_pos.x()-5, anchor_pos.y()-5, 10, 10)); // 移动锚点
    //    painter.drawEllipse(QRect(effect_pos.x()-2, effect_pos.y()-2, 4, 4)); // 影响位置锚点

    //    return QPushButton::paintEvent(event); // 不绘制父类背景了
}

/**
 * IconText/PixmapText模式下，绘制图标
 * 可扩展到绘制图标背景色（模仿menu选中、禁用情况）等
 */
void InteractiveButtonBase::drawIconBeforeText(QPainter &painter, QRect icon_rect)
{
    if (model == IconText)
    {
        if (svg_render)
        {
            svg_render->render(&painter, icon_rect);
        }
        else
        {
            icon.paint(&painter, icon_rect, align, getIconMode());
        }
    }
    else if (model == PixmapText)
    {
        painter.drawPixmap(icon_rect, pixmap);
    }
}

/**
 * 判断坐标是否在按钮区域内
 * 避免失去了焦点，但是依旧需要 hover 效果（非菜单和弹窗抢走焦点）
 * 为子类异形按钮区域判断提供支持
 * @param  point 当前鼠标
 * @return       是否在区域内
 */
bool InteractiveButtonBase::inArea(QPoint point)
{
    return !(point.x() < 0 || point.y() < 0 || point.x() > width() || point.y() > height());
}

bool InteractiveButtonBase::inArea(QPointF point)
{
    return !(point.x() < 0 || point.y() < 0 || point.x() > width() || point.y() > height());
}

/**
 * 获取按钮背景的绘制区域
 * 为子类异形按钮提供支持
 * @return [description]
 */
QPainterPath InteractiveButtonBase::getBgPainterPath()
{
    QPainterPath path;
    if (radius_x || radius_y)
        path.addRoundedRect(QRect(0, 0, width(), height()), radius_x, radius_y);
    else
        path.addRect(QRect(0, 0, width(), height()));
    return path;
}

/**
 * 获取水波纹绘制区域（圆形，但不规则区域）
 * 圆形水面 & 按钮区域
 * @param  water 一面水波纹动画对象
 * @return       绘制路径
 */
QPainterPath InteractiveButtonBase::getWaterPainterPath(InteractiveButtonBase::Water water)
{
    double prog = getNolinearProg(water.progress, FastSlower);
    double ra = water_radius * prog;
    QRectF circle(water.point.x() - ra,
                 water.point.y() - ra,
                 ra * 2,
                 ra * 2);
    /*QRect circle(water.point.x() - water_radius*water.progress/100,
                water.point.y() - water_radius*water.progress/100,
                water_radius*water.progress/50,
                water_radius*water.progress/50);*/
    QPainterPath path;
    path.addEllipse(circle);
    if (radius_x || radius_y)
        return path & getBgPainterPath();
    return path;
}

/**
 * 获取统一的尺寸大小（已废弃）
 * 兼容圆形按钮出现动画，半径使用水波纹（对角线）
 * 可直接使用 protected 对象
 * @return 前景绘制区域
 */
QRectF InteractiveButtonBase::getUnifiedGeometry()
{
    // 将动画进度转换为回弹动画进度
    int pro = show_ani_appearing ? getSpringBackProgress(show_ani_progress, 50) : show_ani_progress;
    double ul = 0, ut = 0, uw = width(), uh = height();

    // show_ani_point 是鼠标进入的点，那么起始方向应该是相反的
    double x = show_ani_point.x(), y = show_ani_point.y();
    int gen = quick_sqrt(long(x * x + y * y));
    x = -water_radius * x / gen; // 动画起始中心点横坐标 反向
    y = -water_radius * y / gen; // 动画起始中心点纵坐标 反向

    ul = ul + x * (100 - pro) / 100 + uw * (100 - pro) / 200;
    ut = ut + y * (100 - pro) / 100 + uh * (100 - pro) / 200;
    uw = uw * pro / 100;
    uh = uh * pro / 100;

    return QRectF(ul, ut, uw, uh);
}

/**
 * 更新统一绘制区域
 * 内部的 _l, _t, _w, _h 可直接使用
 */
void InteractiveButtonBase::updateUnifiedGeometry()
{
    _l = 0;
    _t = 0;
    _w = width();
    _h = height();
    if ((show_ani_appearing || show_ani_disappearing) && show_ani_point != QPoint(0, 0))
    {
        int pro; // 将动画进度转换为回弹动画进度
        pro = show_ani_appearing ? getSpringBackProgress(show_ani_progress, 50) : show_ani_progress;

        // show_ani_point 是鼠标进入的点，那么起始方向应该是相反的
        double x = show_ani_point.x(), y = show_ani_point.y();
        int gen = quick_sqrt(long(x * x + y * y));
        x = -water_radius * x / gen; // 动画起始中心点横坐标 反向
        y = -water_radius * y / gen; // 动画起始中心点纵坐标 反向

        _l = _l + x * (100 - pro) / 100 + _w * (100 - pro) / 200;
        _t = _t + y * (100 - pro) / 100 + _h * (100 - pro) / 200;
        _w = _w * pro / 100;
        _h = _h * pro / 100;
    }
}

/**
 * 绘制一个水波纹动画
 * @param painter 绘制对象（即painter(this)对象）
 */
void InteractiveButtonBase::paintWaterRipple(QPainter &painter)
{
    QColor water_finished_color(press_bg);

    for (int i = 0; i < waters.size(); i++)
    {
        Water water = waters.at(i);
        if (water.finished) // 渐变消失
        {
            water_finished_color.setAlpha(press_bg.alpha() * water.progress / 100);
            QPainterPath path_back = getBgPainterPath();
            //                painter.setPen(water_finished_color);
            painter.fillPath(path_back, QBrush(water_finished_color));
        }
        else // 圆形出现
        {
            QPainterPath path = getWaterPainterPath(water);
            painter.fillPath(path, QBrush(press_bg));
        }
    }
}

/**
 * 鼠标松开的时候，计算所有抖动效果的路径和事件
 * 在每次重绘界面的时候，依次遍历所有的路径
 */
void InteractiveButtonBase::setJitter()
{
    jitters.clear();
    QPoint center_pos = geometry().center() - geometry().topLeft();
    double full_manh = (anchor_pos - center_pos).manhattanLength(); // 距离
    // 是否达到需要抖动的距离
    if (full_manh > (geometry().topLeft() - geometry().bottomRight()).manhattanLength()) // 距离超过外接圆半径，开启抖动
    {
        QPointF jitter_pos(effect_pos);
        full_manh = (jitter_pos - center_pos).manhattanLength();
        double manh = full_manh;
        int duration = jitter_duration;
        qint64 timestamp = release_timestamp;
        while (manh > elastic_coefficient)
        {
            jitters << Jitter(jitter_pos, timestamp);
            jitter_pos = center_pos - (jitter_pos - center_pos) / elastic_coefficient;
            duration = int(jitter_duration * manh / full_manh);
            timestamp += duration;
            manh = static_cast<int>(manh / elastic_coefficient);
        }
        jitters << Jitter(center_pos, timestamp);
        anchor_pos = mouse_pos = center_pos;
    }
    else if (!hovering) // 悬浮的时候依旧有效
    {
        // 未达到抖动距离，直接恢复
        mouse_pos = center_pos;
    }
}

/**
 * 速度极快的开方算法，效率未知，原理未知
 * @param  X 待开方的数字
 * @return   平方根
 */
int InteractiveButtonBase::quick_sqrt(long X) const
{
    bool fu = false;
    if (X < 0)
    {
        fu = true;
        X = -X;
    }
#if !defined(Q_OS_WIN)
    X = qSqrt(X);
    return fu ? -X : X;
#endif
    unsigned long M = static_cast<unsigned long>(X);
    unsigned int N, i;
    unsigned long tmp, ttp; // 结果、循环计数
    if (M == 0)             // 被开方数，开方结果也为0
        return 0;
    N = 0;
    tmp = (M >> 30); // 获取最高位：B[m-1]
    M <<= 2;
    if (tmp > 1) // 最高位为1
    {
        N++; // 结果当前位为1，否则为默认的0
        tmp -= N;
    }
    for (i = 15; i > 0; i--) // 求剩余的15位
    {
        N <<= 1; // 左移一位
        tmp <<= 2;
        tmp += (M >> 30); // 假设
        ttp = N;
        ttp = (ttp << 1) + 1;
        M <<= 2;
        if (tmp >= ttp) // 假设成立
        {
            tmp -= ttp;
            N++;
        }
    }
    return (fu ? -1 : 1) * static_cast<int>(N); // 不知道为什么计算出来的结果是反过来的
}

/**
 * 最大值
 */
int InteractiveButtonBase::max(int a, int b) const { return a > b ? a : b; }

/**
 * 最小值
 */
int InteractiveButtonBase::min(int a, int b) const { return a < b ? a : b; }

/**
 * 获取现行时间戳，13位，精确到毫秒
 * @return 时间戳
 */
qint64 InteractiveButtonBase::getTimestamp() const
{
    return QDateTime::currentDateTime().toMSecsSinceEpoch();
}

/**
 * 是否为亮色颜色
 * @param  color 颜色
 * @return       是否为亮色
 */
bool InteractiveButtonBase::isLightColor(QColor color)
{
    return color.red() * 0.299 + color.green() * 0.578 + color.blue() * 0.114 >= 192;
}

/**
 * 获取非线性动画在某一时间比例的动画进度
 * 仅适用于弹过头效果的动画
 * @param  x   实际相对完整100%的动画进度
 * @param  max 前半部分动画进度上限
 * @return     应当显示的动画进度
 */
int InteractiveButtonBase::getSpringBackProgress(int x, int max)
{
    if (x <= max)
        return x * 100 / max;
    if (x <= max + (100 - max) / 2)
        return (x - max) / 2 + 100;
    return 100 + (100 - x) / 2;
}

/**
 * 获取透明的颜色
 * @param  color 颜色
 * @param  level 比例
 * @return       透明颜色
 */
QColor InteractiveButtonBase::getOpacityColor(QColor color, double level)
{
    color.setAlpha(static_cast<int>(color.alpha() * level));
    return color;
}

/**
 * 获取对应颜色的图标 pixmap
 * 以此实现图标跟随主题变色的功能
 * @param  p 图标
 * @param  c 颜色
 * @return   对应颜色的图标
 */
QPixmap InteractiveButtonBase::getMaskPixmap(QPixmap p, QColor c)
{
    // 创建一个新的 QPixmap 对象，用于修改颜色
    QPixmap newPixmap(p.size());
    newPixmap.fill(Qt::transparent); // 设置背景为透明

    QPainter painter(&newPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawPixmap(0, 0, p); // 将原始图片绘制到新的 QPixmap 上

    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(newPixmap.rect(), c); // 以指定颜色填充整个 QPixmap

    painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    painter.drawPixmap(0, 0, p); // 将原始图片再次绘制到新的 QPixmap 上，覆盖指定颜色

    painter.end();

    return newPixmap;
}

double InteractiveButtonBase::getNolinearProg(int p, InteractiveButtonBase::NolinearType type)
{
    if (p <= 0)
        return 0.0;
    if (p >= 100)
        return 1.0;

    switch (type)
    {
    case Linear:
        return p / 100.0;
    case SlowFaster:
        return p * p / 10000.0;
    case FastSlower:
        return quick_sqrt(p * 100) / 100.0;
    case SlowFastSlower:
        if (p <= 50)
            return p * p / 50.0;
        else
            return 0.5 + quick_sqrt(50 * (p - 50)) / 100.0;
    case SpringBack20:
    case SpringBack50:
        if (p <= 50)
            return p / 50.0;
        else if (p < 75)
            return 1.0 + (p - 50) / 200.0;
        else
            return 1.0 + (100 - p) / 200.0;
    }
}

QIcon::Mode InteractiveButtonBase::getIconMode()
{
    return isEnabled() ? (getState() ? QIcon::Selected : (hovering || pressing ? QIcon::Active : QIcon::Normal)) : QIcon::Disabled;
}

void InteractiveButtonBase::reloadSvgColor()
{
    if (icon_color.isValid())
    {
        QFile file(svg_path);
        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray data = file.readAll();
            QString str(data);
            // 替换fill="xxx"为当前颜色
            str.replace(QRegExp("fill=\"#?\\w+\""), "fill=\"" + icon_color.name() + "\"");
            svg_render->load(str.toUtf8());
            file.close();
        }
        else
        {
            qWarning() << "Could not open SVG file:" << svg_path;
        }
    }
    else
    {
        svg_render->load(svg_path);
    }
}

/**
 * 锚点变成到鼠标位置的定时时钟
 * 同步计算所有和时间或者帧数有关的动画和属性
 */
void InteractiveButtonBase::anchorTimeOut()
{
    qint64 timestamp = getTimestamp();
    // ==== 背景色 ====
    /*if (hovering) // 在框内：加深
    {
        if (hover_progress < 100) // 先判断，再计算，可节约运算资源
            hover_progress = min((timestamp - hover_timestamp) * 100 / press_bg_duration, 100);
    }
    else // 在框外：变浅
    {
        if (hover_progress > 0)
            hover_progress = max((timestamp - leave_timestamp) * 100 / press_bg_duration, 0);
    }

    if (pressing)
    {
        if (press_progress < 100)
            press_progress = min(press_start + (timestamp - press_timestamp) * 100 / press_bg_duration, 100);
    }
    else
    {
        if (press_progress > 0) // 如果按下的效果还在，变浅
            press_progress = max((timestamp - release_timestamp) * 100 / press_bg_duration, 0);
    }*/

    if (pressing) // 鼠标按下
    {
        if (press_progress < 100) // 透明渐变，且没有完成
        {
            press_progress += press_speed;
            if (press_progress >= 100)
            {
                press_progress = 100;
                if (mouse_press_event)
                {
                    emit signalMousePressLater(mouse_press_event);
                    mouse_press_event = nullptr;
                }
            }
        }
        if (hovering && hover_progress < 100)
        {
            hover_progress += hover_speed;
            if (hover_progress >= 100)
            {
                hover_progress = 100;
                emit signalMouseEnterLater();
            }
        }
    }
    else // 鼠标悬浮
    {
        if (press_progress > 0) // 如果按下的效果还在，变浅
        {
            press_progress -= press_speed;
            if (press_progress <= 0)
            {
                press_progress = 0;
                if (mouse_release_event)
                {
                    emit signalMouseReleaseLater(mouse_release_event);
                    mouse_release_event = nullptr;
                }
            }
        }

        if (hovering) // 在框内：加深
        {
            if (hover_progress < 100)
            {
                hover_progress += hover_speed;
                if (hover_progress >= 100)
                {
                    hover_progress = 100;
                    emit signalMouseEnterLater();
                }
            }
        }
        else // 在框外：变浅
        {
            if (hover_progress > 0)
            {
                hover_progress -= hover_speed;
                if (hover_progress <= 0)
                {
                    hover_progress = 0;
                    emit signalMouseLeaveLater();
                }
            }
        }
    }

    // ==== 按下背景水波纹动画 ====
    if (water_animation)
    {
        for (int i = 0; i < waters.size(); i++)
        {
            Water &water = waters[i];
            if (water.finished) // 结束状态
            {
                water.progress = static_cast<int>(100 - 100 * (timestamp - water.finish_timestamp) / water_finish_duration);
                if (water.progress <= 0)
                {
                    waters.removeAt(i--);
                    if (mouse_release_event) // 还没有发送按下延迟信号
                    {
                        emit signalMouseReleaseLater(mouse_release_event);
                        mouse_release_event = nullptr;
                    }
                }
            }
            else // 正在出现状态
            {
                if (water.progress >= 100) // 满了
                {
                    water.progress = 100;
                    if (water.release_timestamp) // 鼠标已经松开了
                    {
                        water.finished = true; // 准备结束
                        water.finish_timestamp = timestamp;
                    }
                }
                else // 动画中的
                {
                    if (water.release_timestamp) // 鼠标已经松开了
                    {
                        water.progress = static_cast<int>(100 * (water.release_timestamp - water.press_timestamp) / water_press_duration + 100 * (timestamp - water.release_timestamp) / water_release_duration);
                    }
                    else // 鼠标一直按下
                    {
                        water.progress = static_cast<int>(100 * (timestamp - water.press_timestamp) / water_press_duration);
                    }
                    if (water.progress >= 100)
                    {
                        water.progress = 100;
                        if (mouse_press_event) // 还没有发送按下延迟信号
                        {
                            emit signalMousePressLater(mouse_press_event);
                            mouse_press_event = nullptr;
                        }
                    }
                }
            }
        }
    }

    // ==== 出现动画 ====
    if (show_animation)
    {
        if (show_ani_appearing) // 出现
        {
            qint64 delta = getTimestamp() - show_timestamp;
            if (show_ani_progress >= 100) // 出现结束
            {
                show_ani_appearing = false;
                emit showAniFinished();
            }
            else
            {
                show_ani_progress = static_cast<int>(100 * delta / show_duration);
                if (show_ani_progress > 100)
                    show_ani_progress = 100;
            }
        }
        if (show_ani_disappearing) // 消失
        {
            qint64 delta = getTimestamp() - hide_timestamp;
            if (show_ani_progress <= 0) // 消失结束
            {
                show_ani_disappearing = false;
                show_foreground = false;
                show_ani_point = QPoint(0, 0);
                emit hideAniFinished();
            }
            else
            {
                show_ani_progress = static_cast<int>(100 - 100 * delta / show_duration);
                if (show_ani_progress < 0)
                    show_ani_progress = 0;
            }
        }
    }

    // ==== 按下动画 ====
    if (click_ani_disappearing) // 点击动画效果消失
    {
        qint64 delta = getTimestamp() - release_timestamp - click_ani_duration;
        if (delta <= 0)
            click_ani_progress = 100;
        else
            click_ani_progress = static_cast<int>(100 - delta * 100 / click_ani_duration);
        if (click_ani_progress < 0)
        {
            click_ani_progress = 0;
            click_ani_disappearing = false;
            emit pressAppearAniFinished();
        }
    }
    if (click_ani_appearing) // 点击动画效果
    {
        qint64 delta = getTimestamp() - release_timestamp;
        if (delta <= 0)
            click_ani_progress = 0;
        else
            click_ani_progress = static_cast<int>(delta * 100 / click_ani_duration);
        if (click_ani_progress > 100)
        {
            click_ani_progress = 100; // 保持100的状态，下次点击时回到0
            click_ani_appearing = false;
            click_ani_disappearing = true;
            emit pressDisappearAniFinished();
        }
    }

    // ==== 锚点移动 ====
    if (jitters.size() > 0) // 松开时的抖动效果
    {
        // 当前应该是处在最后一个点
        Jitter cur = jitters.first();
        Jitter aim = jitters.at(1);
        int del = static_cast<int>(getTimestamp() - cur.timestamp);
        int dur = static_cast<int>(aim.timestamp - cur.timestamp);
        effect_pos = cur.point + (aim.point - cur.point) * del / dur;
        offset_pos = effect_pos - (geometry().center() - geometry().topLeft());

        if (del >= dur)
            jitters.removeFirst();

        // 抖动结束
        if (jitters.size() == 1)
        {
            jitters.clear();
            emit jitterAniFinished();
        }
    }
    else if (anchor_pos != mouse_pos) // 移动效果
    {
        double delta_x = anchor_pos.x() - mouse_pos.x(),
               delta_y = anchor_pos.y() - mouse_pos.y();

        anchor_pos.setX(anchor_pos.x() - quick_sqrt(int(delta_x)));
        anchor_pos.setY(anchor_pos.y() - quick_sqrt(int(delta_y)));

        offset_pos.setX(quick_sqrt(long(anchor_pos.x() - (width() >> 1))));
        offset_pos.setY(quick_sqrt(long(anchor_pos.y() - (height() >> 1))));
        effect_pos.setX((width() >> 1) + offset_pos.x());
        effect_pos.setY((height() >> 1) + offset_pos.y());
    }
    else if (!pressing && !hovering && !hover_progress && !press_progress && !click_ani_appearing && !click_ani_disappearing && !jitters.size() && !waters.size() && !show_ani_appearing && !show_ani_disappearing) // 没有需要加载的项，暂停（节约资源）
    {
        anchor_timer->stop();
    }

    // ==== 统一坐标的出现动画 ====
    if (unified_geometry)
    {
        updateUnifiedGeometry();
    }

    update();
}

/**
 * 鼠标单击事件
 * 实测按下后，在按钮区域弹起，不管移动多少距离都算是 clicked
 */
void InteractiveButtonBase::slotClicked()
{
    click_ani_appearing = true;
    click_ani_disappearing = false;
    click_ani_progress = 0;
    release_offset = offset_pos;

    jitters.clear(); // 清除抖动
}

/**
 * 强行关闭状态
 * 以槽的形式，便与利用
 */
void InteractiveButtonBase::slotCloseState()
{
    setState(false);
}
