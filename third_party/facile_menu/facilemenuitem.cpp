#include "facilemenuitem.h"

FacileMenuItem::FacileMenuItem(QWidget *parent) : InteractiveButtonBase(parent)
{

}

FacileMenuItem::FacileMenuItem(QString t, QWidget *parent) : InteractiveButtonBase(t, parent)
{

}

FacileMenuItem::FacileMenuItem(QIcon i, QWidget *parent) : InteractiveButtonBase(i, parent)
{

}

FacileMenuItem::FacileMenuItem(QIcon i, QString t, QWidget *parent) : InteractiveButtonBase(i, t, parent)
{

}

FacileMenuItem::FacileMenuItem(QPixmap p, QString t, QWidget *parent) : InteractiveButtonBase(p, t, parent)
{

}

FacileMenuItem *FacileMenuItem::setEnabled(bool e)
{
    InteractiveButtonBase::setEnabled(e);
    return this;
}

FacileMenuItem *FacileMenuItem::setCheckable(bool c)
{
    checkable = c;
    if (c && model != IconText && InteractiveButtonBase::icon.isNull())
        model = IconText;
    update();
    return this;
}

bool FacileMenuItem::isCheckable() const
{
    return checkable;
}

FacileMenuItem *FacileMenuItem::setChecked(bool c)
{
    _state = c;
    if (InteractiveButtonBase::icon.isNull())
        model = IconText; // 强制显示check空白部分
    setCheckable(true);
    return this;
}

bool FacileMenuItem::isChecked()
{
    return getState();
}

FacileMenuItem *FacileMenuItem::setKey(Qt::Key key)
{
    this->key = key;
    return this;
}

bool FacileMenuItem::isKey(Qt::Key key) const
{
    return key == this->key;
}

FacileMenuItem *FacileMenuItem::setSubMenu(FacileMenu *menu)
{
    sub_menu = menu;
    return this;
}

bool FacileMenuItem::isSubMenu() const
{
    return sub_menu != nullptr;
}

bool FacileMenuItem::isLinger() const
{
    return trigger_linger;
}

FacileMenuItem *FacileMenuItem::setData(QVariant data)
{
    this->data = data;
    return this;
}

QVariant FacileMenuItem::getData()
{
    return data;
}

FacileMenuItem* FacileMenuItem::setDynamicCreate(bool dynamic)
{
    if (dynamic)
        setDynamicCreateState(-1);
    else
        setDynamicCreateState(0);
    return this;
}

void FacileMenuItem::setDynamicCreateState(short state)
{
    this->dynamic_create_state = state;
}

short FacileMenuItem::getDynamicCreateState() const
{
    return dynamic_create_state;
}

FacileMenuItem *FacileMenuItem::tip(QString sc)
{
    shortcut_tip = sc;
    return this;
}

FacileMenuItem *FacileMenuItem::tip(bool exp, QString sc)
{
    if (exp)
        tip(sc);
    return this;
}

FacileMenuItem *FacileMenuItem::tooltip(QString tt)
{
    setToolTip(tt);
    return this;
}

FacileMenuItem *FacileMenuItem::tooltip(bool exp, QString tt)
{
    if (exp)
        tooltip(tt);
    return this;
}

FacileMenuItem *FacileMenuItem::triggered(FuncType func)
{
    connect(this, &InteractiveButtonBase::clicked, this, [=]{
        func();
    });
    return this;
}

FacileMenuItem *FacileMenuItem::triggered(bool exp, FuncType func)
{
    if (!exp)
        triggered(func);
    return this;
}

FacileMenuItem *FacileMenuItem::disable(bool exp)
{
    if (exp)
        setDisabled(true);
    return this;
}

FacileMenuItem *FacileMenuItem::enable(bool exp)
{
    if (exp)
        setEnabled(true);
    return this;
}

FacileMenuItem *FacileMenuItem::hide(bool exp)
{
    if (exp)
        InteractiveButtonBase::hide();
    return this;
}

/**
 * 默认就是show状态
 * 为了和show区分开
 */
FacileMenuItem *FacileMenuItem::visible(bool exp)
{
    if (exp)
        InteractiveButtonBase::setVisible(true);
    return this;
}

FacileMenuItem *FacileMenuItem::check(bool exp)
{
    setCheckable(true);
    if (exp)
        setChecked(true);
    else if (InteractiveButtonBase::icon.isNull())
        model = IconText; // 强制显示check空白部分
    return this;
}

FacileMenuItem *FacileMenuItem::uncheck(bool exp)
{
    setCheckable(true);
    if (exp)
        setChecked(false);
    return this;
}

/**
 * 切换状态
 * 如果选中了，则取消选中；反之亦然
 * （本来打算不只是选中状态，然而还没想到其他有什么能切换的）
 */
FacileMenuItem *FacileMenuItem::toggle(bool exp)
{
    if (!exp)
        return this;
    if (isCheckable())
    {
        setChecked(!isChecked());
    }
    // 以后什么功能想到再加
    return this;
}

/**
 * 点击自动切换状态
 * 小心点，因为信号槽顺序的关系，若放在triggered后面，可能会出现相反的check
 * 建议只用于和顺序无关的自动切换
 * （还不知道怎么修改信号槽的调用顺序）
 */
FacileMenuItem *FacileMenuItem::autoToggle()
{
    connect(this, &InteractiveButtonBase::clicked, this, [=]{
        toggle();
    });
    return this;
}

FacileMenuItem *FacileMenuItem::text(bool exp, QString str)
{
    if (exp)
    {
        // 去掉快捷键符号
        // 注意：这里设置文字不会改变原来的快捷键！
        setText(str.replace(QRegExp("&([\\w\\d])\\b"), "\\1"));
        // 调整大小
        setFixedForeSize();
    }
    return this;
}

/**
 * 设置字符串，成立时 tru，不成立时 fal
 * 注意：这里是直接设置完整的文字，不会去掉快捷键&符号
 */
FacileMenuItem *FacileMenuItem::text(bool exp, QString tru, QString fal)
{
    if (exp)
        setText(tru);
    else
        setText(fal);
    return this;
}

FacileMenuItem *FacileMenuItem::fgColor(QColor color)
{
    setTextColor(color);
    return this;
}

FacileMenuItem *FacileMenuItem::fgColor(bool exp, QColor color)
{
    if (exp)
        return fgColor(color);
    return this;
}

FacileMenuItem *FacileMenuItem::bgColor(QColor color)
{
    setBgColor(color);
    return this;
}

FacileMenuItem *FacileMenuItem::bgColor(bool exp, QColor color)
{
    if (exp)
        bgColor(color);
    return this;
}

/**
 * 满足条件时，text添加前缀
 */
FacileMenuItem *FacileMenuItem::prefix(bool exp, QString pfix)
{
    if (exp)
        prefix(pfix);
    return this;
}

/**
 * 满足条件时，text添加后缀
 * @param inLeftParenthesis 支持 text(xxx) 形式，会在左括号前添加后缀
 */
FacileMenuItem *FacileMenuItem::suffix(bool exp, QString sfix, bool inLeftParenthesis)
{
    if (exp)
    {
        suffix(sfix, inLeftParenthesis);
    }
    return this;
}

FacileMenuItem *FacileMenuItem::prefix(QString pfix)
{
    setText(pfix + getText());
    return this;
}

FacileMenuItem *FacileMenuItem::suffix(QString sfix, bool inLeftParenthesis)
{
    if (!inLeftParenthesis)
    {
        setText(getText() + sfix);
    }
    else
    {
        QString text = getText();
        int index = -1;
        if ((index = text.lastIndexOf("(")) > -1)
        {
            while (index > 0 && text.mid(index-1, 1) == " ")
                index--;
        }
        if (index <= 0) // 没有左括号或者以空格开头，直接加到最后面
        {
            setText(getText() + sfix);
        }
        else
        {
            setText(text.left(index) + sfix + text.right(text.length()-index));
        }
    }
    return this;
}

FacileMenuItem *FacileMenuItem::icon(bool exp, QIcon ico)
{
    if (exp)
        setIcon(ico);
    return this;
}

FacileMenuItem *FacileMenuItem::borderR(int radius, QColor co)
{
    setRadius(radius);
    if (co != Qt::transparent)
        setBorderColor(co);
    else
        setBorderColor(press_bg);
    return this;
}

FacileMenuItem *FacileMenuItem::linger()
{
    trigger_linger = true;
    return this;
}

FacileMenuItem *FacileMenuItem::lingerText(QString textAfterClick)
{
    this->linger();
    connect(this, &FacileMenuItem::clicked, this, [=]{
        setText(textAfterClick);
    });
    return this;
}

/**
 * 绑定某一布尔类型的变量（只能全局变量）
 * 点击即切换值
 * 注意：因为是异步的，局部变量会导致崩溃！
 */
FacileMenuItem *FacileMenuItem::bind(bool &val)
{
    connect(this, &InteractiveButtonBase::clicked, this, [&]{
        val = !val;
    });
    return this;
}

/**
 * 短期长按效果
 * 该操作不会影响其它任何交互效果
 * 即不会隐藏菜单，也不会解除单击信号
 */
FacileMenuItem *FacileMenuItem::longPress(FuncType func)
{
    connect(this, &InteractiveButtonBase::signalMousePressLater, this, [=](QMouseEvent*){
        func();
    });
    return this;
}

/**
 * 点击菜单后的新文本
 * 一般用于点击后不隐藏 或者 重新显示的菜单
 * 不然没必要设置，点击后就没了
 */
FacileMenuItem *FacileMenuItem::textAfterClick(QString newText)
{
    connect(this, &FacileMenuItem::clicked, this, [=]{
        setText(newText);
    });
    return this;
}

FacileMenuItem *FacileMenuItem::textAfterClick(FuncStringStringType func)
{
    connect(this, &FacileMenuItem::clicked, this, [=]{
        setText(func(this->InteractiveButtonBase::text));
    });
    return this;
}

/**
 * 适用于连续设置
 * 当 iff 成立时继续
 * 否则取消后面所有设置
 */
FacileMenuItem *FacileMenuItem::ifer(bool exp)
{
    if (exp)
        return this;

    // 返回一个无用item，在自己delete时也delete掉
    return createTempItem();
}

/**
 * 完全等于 ifer
 * 如果已经在 ifer 里面，则先退出
 */
FacileMenuItem *FacileMenuItem::elifer(bool exp)
{
    if (parent_menu_item_in_if) // ifer 不成立后的，退出并转至新的 ifer
        return parent_menu_item_in_if->ifer(exp);
    return ifer(exp); // 直接使用，完全等同于 ifer
}

FacileMenuItem *FacileMenuItem::elser()
{
    if (parent_menu_item_in_if)
        return parent_menu_item_in_if;
    return createTempItem();
}

/**
 * 适用于连续设置action时，满足条件则退出
 * 相当于一个控制语句
 * 当ex成立时，取消后面所有设置
 */
FacileMenuItem *FacileMenuItem::exiter(bool exp)
{
    if (!exp)
        return this;

    // 返回一个无用item，在自己delete时也delete掉
    return createTempItem(false);
}

/**
 * 适用于连续设置
 * 满足某一条件则执行 func(this)
 */
FacileMenuItem *FacileMenuItem::ifer(bool exp, FuncItemType func, FuncItemType elseFunc)
{
    if (exp)
    {
        if (func)
            func(this);
    }
    else
    {
        if (elseFunc)
            elseFunc(this);
    }
    return this;
}

/**
 * 适用于连续设置
 * 类似 switch 语句，输入判断的值
 * 当后续的 caser 满足 value 时，允许执行 caser 的 func 或后面紧跟着的的设置
 */
FacileMenuItem *FacileMenuItem::switcher(int value)
{
    switch_value = value;
    switch_matched = false;
    return this;
}

/**
 * 当 value 等同于 switcher 判断的 value 时，执行 func
 * 并返回原始 item
 * 注意与重载的 caser(int) 进行区分
 */
FacileMenuItem *FacileMenuItem::caser(int value, FuncType func)
{
    if (value == switch_value)
    {
        switch_matched = true;
        if (func)
            func();
    }
    return this;
}

/**
 * 当 value 等同于 switcher 的 value 时，返回原始 item
 * 即执行 caser 后面的设置，直至 breaker
 * 注意与重载的 caser(int FuncType) 进行区分
 */
FacileMenuItem *FacileMenuItem::caser(int value)
{
    // 可能已经接着一个没有 breaker 的 caser
    // 则回到上一级（这样会导致无法嵌套）
    if (this->parent_menu_item_in_if)
    {
        // 接着一个 !=的caser 后面
        if (value == parent_menu_item_in_if->switch_value)
        {
            parent_menu_item_in_if->switch_matched = true;
            return parent_menu_item_in_if; // 真正需要使用的实例
        }
        return this; // 继续使用自己（一个临时实例）
    }
    else // 自己是第一个 caser 或者 ==的caser 后面
    {
        if (value == switch_value)
        {
            switch_matched = true;
            return this;
        }
        return createTempItem();
    }
}

/**
 * caser 的 value 不等于 switcher 的 value 时
 * 此语句用来退出
 */
FacileMenuItem *FacileMenuItem::breaker()
{
    if (parent_menu_item_in_if)
        return parent_menu_item_in_if;
    return this; // 应该不会吧……
}

/**
 * 如果switcher的caser没有满足
 */
FacileMenuItem *FacileMenuItem::defaulter()
{
    if (switch_matched) // 已经有 caser 匹配了
        return createTempItem(); // 返回无效临时实例
    return this; // 能用，返回自己
}

/**
 * 返回自己的子菜单对象
 */
FacileMenu *FacileMenuItem::subMenu()
{
    return sub_menu;
}

void FacileMenuItem::paintEvent(QPaintEvent *event)
{
    InteractiveButtonBase::paintEvent(event);

    int right = width()- 8;

    QPainter painter(this);
    if (isSubMenu())
    {
        right -= icon_text_size;
        // 画右边箭头的图标
        QRect rect(right, fore_paddings.top, icon_text_size, icon_text_size);
        painter.drawPixmap(rect, QPixmap(":/icons/sub_menu_arrow"));
    }

    right -= icon_text_padding;
    if (!shortcut_tip.isEmpty())
    {
        // 画右边的文字
        QFontMetrics fm(this->font());
        int width = fm.horizontalAdvance(shortcut_tip);
        painter.save();
        auto c = painter.pen().color();
        c.setAlpha(c.alpha() / 2);
        painter.setPen(c);
        painter.drawText(QRect(right-width, fore_paddings.top, width, height()-fore_paddings.top-fore_paddings.bottom),
                         Qt::AlignRight, shortcut_tip);
        painter.restore();
    }
}

void FacileMenuItem::drawIconBeforeText(QPainter &painter, QRect icon_rect)
{
    // 选中
    if (checkable)
    {
        QPainterPath path;
        QRect expand_rect = icon_rect;
        expand_rect.adjust(-2, -2, 2, 2);
        path.addRoundedRect(expand_rect, 3, 3);
        if (InteractiveButtonBase::icon.isNull())
        {
            // 绘制√
            if (isChecked())
                painter.drawText(icon_rect, "√");
        }
        else // 有图标，使用
        {
            if (getState())
            {
                // 绘制选中样式： 圆角矩形
                painter.fillPath(path, press_bg);
            }
            else
            {
                // 绘制未选中样式：空白边框
                painter.save();
                painter.setPen(QPen(press_bg, 1));
                painter.drawPath(path);
                painter.restore();
            }
        }
    }

    InteractiveButtonBase::drawIconBeforeText(painter, icon_rect);
}

FacileMenuItem *FacileMenuItem::createTempItem(bool thisIsParent)
{
    auto useless = new FacileMenuItem(QIcon(), "", this);
    useless->parent_menu_item_in_if = thisIsParent ? this : nullptr;
    useless->hide();
    useless->setEnabled(false);
    useless->setMinimumSize(0, 0);
    useless->setFixedSize(0, 0);
    useless->move(-999, -999);
    return useless;
}
