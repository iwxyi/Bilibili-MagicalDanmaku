#include <QLabel>
#include <QRadialGradient>
#include "facilemenu.h"
#include "imageutil.h"

QColor FacileMenu::normal_bg = QColor(255, 255, 255);
QColor FacileMenu::hover_bg = QColor(128, 128, 128, 64);
QColor FacileMenu::press_bg = QColor(128, 128, 128, 128);
QColor FacileMenu::text_fg = QColor(0, 0, 0);
int FacileMenu::blur_bg_alpha = DEFAULT_MENU_BLUR_ALPHA;
QEasingCurve FacileMenu::easing_curve = QEasingCurve::OutBack; // OutCubic 也不错
bool FacileMenu::auto_dark_mode = true;
bool FacileMenu::auto_theme_by_bg = false;
bool FacileMenu::all_menu_same_color = true;

FacileMenu::FacileMenu(QWidget *parent) : QWidget(parent)
{
    setObjectName("FacileMenu");
    setAttribute(Qt::WA_DeleteOnClose, true);
    setFocusPolicy(Qt::StrongFocus); // 获取焦点，允许按键点击
    setWindowFlag(Qt::Popup, true);
    setAutoFillBackground(false);  //这个不设置的话就背景变黑
    setAttribute(Qt::WA_StyledBackground);

    main_hlayout = new QHBoxLayout(this);
    main_vlayout = new QVBoxLayout();
    setLayout(main_hlayout);
    main_hlayout->addLayout(main_vlayout);
    main_hlayout->setMargin(0);
    main_hlayout->setSpacing(0);

    m_bg_color = normal_bg;
    m_text_color = text_fg;
    setStyleSheet("#FacileMenu { background: "+QVariant(m_bg_color).toString()+"; border: none; border-radius:5px; }");

    setMouseTracking(true);

    // 获取窗口尺寸
    int screen_number = QApplication::desktop()->screenNumber(this->parentWidget() ? this->parentWidget() : this);
    if (screen_number < 0)
        screen_number = 0;
    window_rect = QGuiApplication::screens().at(screen_number)->geometry();
    window_height = window_rect.height();

    // 动画优化
    frame_timer = new QTimer(this);
    connect(frame_timer, &QTimer::timeout, this, [=]{
        if (_showing_animation)
            return ;
        update();
    });
    frame_timer->start(16); // 60fps
}

/**
 * 这是临时菜单，并不显示出来
 * 只用来进行各种暗中的逻辑运算
 */
FacileMenu::FacileMenu(bool, QWidget *parent) : FacileMenu(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::NoFocus);

    // 保存父菜单的指针，判断鼠标移动的位置
    parent_menu = static_cast<FacileMenu*>(parent);
}

FacileMenu::~FacileMenu()
{
    if (finished_func)
    {
        (*finished_func)();
        delete  finished_func;
    }
    foreach (auto action, import_actions)
        action->deleteLater();
}

FacileMenuItem *FacileMenu::addAction(QIcon icon, QString text, FuncType clicked)
{
    auto item = createMenuItem(icon, text);
    connect(item, &InteractiveButtonBase::clicked, this, [=]{
        if (_showing_animation)
            return ;
        if (clicked != nullptr)
        {
            QTimer::singleShot(0, [=]{
                clicked();
            });
        }
        if (!item->isLinger())
        {
            emit signalActionTriggered(item);
            toHide(items.indexOf(item));
        }
    });
    connect(item, &InteractiveButtonBase::signalMouseEnter, this, [=]{ itemMouseEntered(item); });
    return item;
}

FacileMenuItem *FacileMenu::addAction(QIcon icon, FuncType clicked)
{
    return addAction(icon, "", clicked);
}

FacileMenuItem *FacileMenu::addAction(QString text, FuncType clicked)
{
    return addAction(QIcon(), text, clicked);
}

FacileMenuItem *FacileMenu::addAction(QAction *action, bool deleteWithMenu)
{
    if (deleteWithMenu)
        import_actions.append(action); // 加入列表，菜单delete时一起删了
    if (action->menu() != nullptr) // 这是个菜单
    {
        auto menu = addMenu(action->icon(), action->text(), [=]{ action->trigger(); });
        menu->addActions(action->menu()->actions());
        return menu->parentAction();
    }
    else // 普通的action
    {
        auto ac = addAction(action->icon(), action->text(), [=]{ action->trigger(); });
        if (action->isChecked())
            ac->check();
        if (!action->toolTip().isEmpty())
            ac->tooltip(action->toolTip());
        return ac;
    }
}

/**
 * 回调：类内方法（学艺不精，用不来……）
 */
template<class T>
FacileMenuItem *FacileMenu::addAction(QIcon icon, QString text, T *obj, void (T::*func)())
{
    auto item = createMenuItem(icon, text);
    connect(item, &InteractiveButtonBase::clicked, this, [=]{
        if (_showing_animation)
            return ;
        (obj->*func)();
        if (item->isLinger())
            return ;
        emit signalActionTriggered(item);
        toHide(items.indexOf(item));
    });
    connect(item, &InteractiveButtonBase::signalMouseEnter, this, [=]{ itemMouseEntered(item); });
    return item;
}

/**
 * 批量添加带数字（可以不带）的action
 * 相当于只是少了个for循环……
 * @param pattern 例如 项目%1
 * @param numberEnd 注意，结数值不包括结尾！
 */
FacileMenu *FacileMenu::addNumberedActions(QString pattern, int numberStart, int numberEnd, FuncItemType config, FuncIntType clicked, int step)
{
    if (!step)
        step = numberStart <= numberEnd ? 1 : -1;
    for (int i = numberStart; i != numberEnd; i += step)
    {
        auto ac = addAction(pattern.arg(i), [=]{
            if (clicked)
                clicked(i);
        });
        if (config)
            config(ac);
    }
    return this;
}

/**
 * 同上
 * @param pattern 例如 项目%1
 * @param numberEnd 注意，结数值不包括结尾！
 * @param config (Item*, int) 其中参数2表示number遍历的位置，不是当前item的index
 */
FacileMenu *FacileMenu::addNumberedActions(QString pattern, int numberStart, int numberEnd, FuncItemIntType config, FuncIntType clicked, int step)
{
    if (!step)
        step = numberStart <= numberEnd ? 1 : -1;
    for (int i = numberStart; i != numberEnd; i += step)
    {
        auto ac = addAction(pattern.arg(i), [=]{
            if (clicked)
                clicked(i);
        });
        if (config)
            config(ac, i);
    }
    return this;
}

FacileMenu *FacileMenu::addActions(QList<QAction *> actions)
{
    foreach (auto action, actions)
    {
        addAction(action);
    }
    return this;
}

FacileMenu *FacileMenu::addRow(FuncType addActions)
{
    QHBoxLayout* layout = new QHBoxLayout;
    row_hlayouts.append(layout);
    main_vlayout->addLayout(layout);

    align_mid_if_alone = true;
    adding_horizone = true;

    addActions();

    align_mid_if_alone = false;
    adding_horizone = false;

    return this;
}

FacileMenu *FacileMenu::beginRow()
{
    QHBoxLayout* layout = new QHBoxLayout;
    row_hlayouts.append(layout);
    main_vlayout->addLayout(layout);

    align_mid_if_alone = true;
    adding_horizone = true;

    return this;
}

FacileMenu *FacileMenu::endRow()
{
    align_mid_if_alone = false;
    adding_horizone = false;

    return this;
}

QVBoxLayout *FacileMenu::createNextColumn()
{
    main_vlayout = new QVBoxLayout;
    main_hlayout->addLayout(main_vlayout);
    main_vlayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    return main_vlayout;
}

/**
 * 获取当前的layout
 * 如果是正在横向布局，则返回横向布局的layout（子）
 * 否则返回竖直布局的总layout
 */
QBoxLayout *FacileMenu::currentLayout() const
{
    if (adding_horizone && row_hlayouts.size())
        return row_hlayouts.last();
    else
        return main_vlayout;
}

FacileMenu *FacileMenu::addTitle(QString text, int split)
{
    if (split < 0)
    {
        this->split();
        if (layout()->count())
            this->addSpacing(4); // 因为是与上一个菜单项分隔，不能太紧凑，要加点空白
    }
    QLabel* label = new QLabel(text, this);
    label->setStyleSheet("margin: 4px; color: gray;");
    addWidget(label);
    if (split > 0)
        this->split();
    return this;
}

/**
 * 添加一项子菜单
 * 鼠标浮在上面展开
 * 同时也可以设置点击事件
 */
FacileMenu *FacileMenu::addMenu(QIcon icon, QString text, FuncType clicked)
{
    auto item = createMenuItem(icon, text);

    // 子菜单项是否可点击
    if (clicked != nullptr)
    {
        connect(item, &InteractiveButtonBase::clicked, this, [=]{
            if (_showing_animation)
                return ;

            clicked();
            emit signalActionTriggered(item);
            toHide(items.indexOf(item));
        });
    }
    else // 点击出现子项
    {
        connect(item, &InteractiveButtonBase::clicked, this, [=]{
            if (_showing_animation)
                return ;

            // 显示子菜单
            showSubMenu(item);
        });
    }

    connect(item, &InteractiveButtonBase::signalMouseEnterLater, this, [=]{
        if (_showing_animation)
            return ;
        if (item->getDynamicCreateState() == -1)
        {
            item->setDynamicCreateState(1);
            emit signalDynamicMenuTriggered(item);
        }
        int index = items.indexOf(item);
        if (using_keyboard && current_index > -1 && current_index < items.size() && current_index != index) // 屏蔽键盘操作
            items.at(current_index)->discardHoverPress(true);
        current_index = index;

        // 显示子菜单
        // 可能是需要点击这个菜单项，但是点下去隐藏子菜单，会再次触发 mouseEnterLater 事件
        // 需要判断位置，屏蔽第二次的 enter 事件，得以点击菜单项
        // 不过还是需要双击才行，第一次是隐藏子菜单，第二次才是真正点击
        QPoint showPos = mapFromGlobal(QCursor::pos());
        if (_enter_later_pos == showPos)
            return ;

        _enter_later_pos = showPos;
        if (current_index == items.indexOf(item))
            showSubMenu(item);
    });

    // 创建菜单项
    FacileMenu* menu = new FacileMenu(true, this);
    menu->split_in_row = this->split_in_row;
    menu->enable_appear_animation = this->enable_appear_animation;
    menu->enable_disappear_animation = this->enable_disappear_animation;
    menu->sub_menu_show_on_cursor = this->sub_menu_show_on_cursor;
    menu->hide();
    item->setSubMenu(menu);
    connect(menu, &FacileMenu::signalHidden, item, [=]{
        if (!using_keyboard || current_index != items.indexOf(item))
        {
            // 子菜单隐藏，当前按钮强制取消hover状态
            item->discardHoverPress(true);
        }

        // 如果是用户主动隐藏子菜单，那么就隐藏全部菜单
        // 有一种情况是需要点击这个菜单项而不是弹出的子菜单，需要避免
        if (!menu->hidden_by_another && !linger_on_submenu_clicked
                && !rect().contains(mapFromGlobal(QCursor::pos()))) // 允许鼠标浮在菜单项上，ESC关闭子菜单
        {
            this->hide(); // 隐藏自己，在隐藏事件中继续向上传递隐藏的信号
        }
    });
    connect(menu, &FacileMenu::signalActionTriggered, this, [=](FacileMenuItem* action){
        closed_by_clicked = true;
        // 子菜单被点击了，副菜单依次隐藏
        emit signalActionTriggered(action);
        if (!linger_on_submenu_clicked)
            toHide(items.indexOf(item));
    });
    return menu;
}

FacileMenu *FacileMenu::addMenu(QString text, FuncType clicked)
{
    return addMenu(QIcon(), text, clicked);
}

/**
 * 从已有menu中导入actions
 */
FacileMenu *FacileMenu::addMenu(QMenu *menu)
{
    FacileMenu* m = addMenu(menu->icon(), menu->title());
    auto actions = menu->actions();
    for (int i = 0; i < actions.size(); i++)
    {
        m->addAction(actions.at(i));
    }
    return m;
}

/**
 * 子菜单所属的action
 */
FacileMenuItem *FacileMenu::parentAction()
{
    if (!parent_menu) // 不是子菜单
        return nullptr;
    foreach (auto item, parent_menu->items)
    {
        if (item->isSubMenu() && item->subMenu() == this)
            return item;
    }
    return nullptr;
}

/**
 * 获取最后一个action
 * 不只是最后一个，更是正在编辑的这个
 * 如果添加了子菜单，那么就需要这个返回值
 */
FacileMenuItem *FacileMenu::lastAction()
{
    if (items.size() == 0)
        return nullptr;
    return items.last();
}

/**
 * 当前正在调整的菜单项
 * 当然是最后添加的那个
 */
FacileMenuItem *FacileMenu::currentAction()
{
    return lastAction();
}

FacileMenu *FacileMenu::addLayout(QLayout *layout, int stretch)
{
    if (!layout)
        return this;
    if (adding_horizone && row_hlayouts.size()) // 如果是正在添加横向按钮
    {
        if (split_in_row && row_hlayouts.last()->count() > 0)
            addVSeparator(); // 添加竖向分割线
        row_hlayouts.last()->addLayout(layout, stretch);
    }
    else
    {
        main_vlayout->addLayout(layout, stretch);
    }
    for (int i = 0; i < layout->count(); i++)
    {
        auto it = layout->itemAt(i);
        auto widget = qobject_cast<QWidget*>(it->widget());
        if (widget) // 如果这个 LayoutItem 是 widget
            other_widgets.append(widget);
    }
    return this;
}

FacileMenu *FacileMenu::addLayoutItem(QLayoutItem* item)
{
    currentLayout()->addItem(item);
    return this;
}

FacileMenu *FacileMenu::addSpacerItem(QSpacerItem *spacerItem)
{
    currentLayout()->addSpacerItem(spacerItem);
    return this;
}

FacileMenu *FacileMenu::addSpacing(int size)
{
    currentLayout()->addSpacing(size);
    return this;
}

FacileMenu *FacileMenu::addStretch(int stretch)
{
    currentLayout()->addStretch(stretch);
    return this;
}

FacileMenu *FacileMenu::addStrut(int size)
{
    currentLayout()->addStrut(size);
    return this;
}

FacileMenu *FacileMenu::addWidget(QWidget *widget, int stretch, Qt::Alignment alignment)
{
    if (!widget)
        return this;
    QBoxLayout* layout = nullptr;
    if (adding_horizone && row_hlayouts.size()) // 如果是正在添加横向按钮
    {
        if (split_in_row && row_hlayouts.last()->count() > 0)
            addVSeparator(); // 添加竖向分割线
        layout = row_hlayouts.last();
    }
    else
    {
        if (main_vlayout->sizeHint().height() + widget->height() > window_height)
            createNextColumn();
        layout = main_vlayout;
    }
    layout->addWidget(widget, stretch, alignment);
    other_widgets.append(widget);
    return this;
}

FacileMenu *FacileMenu::setSpacing(int spacing)
{
    currentLayout()->setSpacing(spacing);
    return this;
}

FacileMenu *FacileMenu::setStretchFactor(QWidget *widget, int stretch)
{
    main_vlayout->setStretchFactor(widget, stretch);
    return this;
}

FacileMenu *FacileMenu::setStretchFactor(QLayout *layout, int stretch)
{
    main_vlayout->setStretchFactor(layout, stretch);
    return this;
}

/**
 * 添加水平分割线
 * 不一定需要
 */
FacileMenuItem *FacileMenu::addSeparator()
{
    if (adding_horizone)
    {
        if (!row_hlayouts.last()->count())
            return nullptr;
        return addVSeparator();
    }

    if (!main_vlayout->count())
        return nullptr;

    FacileMenuItem* item = new FacileMenuItem(this);
    item->setNormalColor(QColor(64, 64, 64, 64));
    item->setFixedHeight(1);
    item->setPaddings(32, 32, 0, 0);
    item->setDisabled(true);
    item->setAttribute(Qt::WA_LayoutUsesWidgetRect);

    main_vlayout->addWidget(item);
    h_separators.append(item);
    last_added_item = item;

    return item;
}

FacileMenu *FacileMenu::split()
{
    addSeparator();
    return this;
}

/**
 * 返回最后添加的菜单项（包括分割线）
 */
FacileMenuItem *FacileMenu::lastAddedItem()
{
    return last_added_item;
}

bool FacileMenu::hasFocus() const
{
    if (QWidget::hasFocus())
        return true;
    if (current_sub_menu && current_sub_menu->hasFocus())
        return true;
    return false;
}

/**
 * 返回菜单项的索引
 * 不包括非菜单项的自定义控件、布局、分割线等
 * 未找到返回-1
 */
int FacileMenu::indexOf(FacileMenuItem *item)
{
    return items.indexOf(item);
}

/**
 * 返回索引对应的菜单项
 * 不包括非菜单项的自定义控件、布局、分割线等
 * 未找到返回nullptr
 */
FacileMenuItem *FacileMenu::at(int index)
{
    if (index < 0 || index >= items.size())
        return nullptr;
    return items.at(index);
}

/**
 * 设置菜单栏
 * 鼠标移动时会增加判断是否移动到菜单栏按钮上
 */
void FacileMenu::setMenuBar(FacileMenuBarInterface *mb)
{
    this->menu_bar = mb;
}

/**
 * 一行有多个按钮时的竖向分割线
 * 只有添加chip前有效
 */
FacileMenuItem *FacileMenu::addVSeparator()
{
    FacileMenuItem* item = new FacileMenuItem(this);
    item->setNormalColor(QColor(64, 64, 64, 64));
    item->setFixedWidth(1);
    item->setPaddings(0, 0, 0, 0);
    item->setDisabled(true);
    item->setAttribute(Qt::WA_LayoutUsesWidgetRect);

    row_hlayouts.last()->addWidget(item);
    v_separators.append(item);
    last_added_item = item;

    return item;
}

/**
 * 在鼠标或指定点展开
 * 自动避开屏幕边缘
 */
void FacileMenu::exec(QPoint pos)
{
    if (pos == QPoint(-1,-1))
        pos = QCursor::pos();
    QPoint originPos = pos; // 不包含像素偏移的原始点
    main_vlayout->setEnabled(true);
    main_vlayout->activate(); // 先调整所有控件大小
    this->adjustSize();

    // setAttribute(Qt::WA_DontShowOnScreen); // 会触发 setMouseGrabEnabled 错误
    // show();
    // hide(); // 直接显示吧
    // setAttribute(Qt::WA_DontShowOnScreen, false);

    int x = pos.x() + 1;
    int y = pos.y() + 1;
    int w = width() + 1;
    int h = height() + 1;
    QRect avai = window_rect; // 屏幕大小

    // 如果超过范围，则调整位置
    if (x + w > avai.right())
        x = avai.right() - w;
    if (y + h > avai.bottom())
        y = avai.bottom() - h;
    if (x >= w && pos.x() + w > avai.right())
        x = originPos.x() - w;
    if (y >= h && pos.y() + h > avai.bottom())
        y = originPos.y() - h;

    // 移动窗口
    move(QPoint(x, y));

    execute();
}

/**
 * 确保在指定矩形框之外展开
 * 注意：即使菜单项超过了窗口范围，也不会覆盖这个矩形
 * @param expt     这个是相对整个屏幕的坐标，用 mapToGlobal(geometry())
 * @param vertical 优先左边对齐（出现在下边），还是顶部对齐（出现在右边）
 * @param pos      默认位置。如果在expt外且非边缘，则不受expt影响
 */
void FacileMenu::exec(QRect expt, bool vertical, QPoint pos)
{
    if (pos == QPoint(-1,-1))
        pos = QCursor::pos();
    main_vlayout->setEnabled(true);
    main_hlayout->invalidate();
    main_vlayout->activate(); // 先调整所有控件大小

    // setAttribute(Qt::WA_DontShowOnScreen); // 会触发 setMouseGrabEnabled 错误
    // show(); // 但直接显示会有一瞬间闪烁情况
    // hide();
    // setAttribute(Qt::WA_DontShowOnScreen, false);

    // 根据 rect 和 avai 自动调整范围
    QRect avai = window_rect;
    QRect rect = geometry();
    rect.moveTo(pos);
    if (!vertical) // 优先横向对齐（顶上）
    {
        if (rect.left() <= expt.right() && rect.right() > expt.right())
            rect.moveLeft(expt.right());
        rect.moveTop(expt.top());

        // 避开屏幕位置
        if (expt.left() > rect.width() && rect.right() >= avai.right())
            rect.moveLeft(expt.left() - rect.width());
        if (rect.bottom() >= avai.bottom())
        {
            if (expt.top() > rect.height())
                rect.moveTop(expt.bottom() - rect.height());
            else
                rect.moveTop(avai.bottom() - rect.height());
        }
    }
    else // 优先纵向对齐（左下）
    {
        if (rect.top() <= expt.bottom() && rect.bottom() > expt.bottom())
            rect.moveTop(expt.bottom());
        rect.moveLeft(expt.left());

        // 避开屏幕位置
        if (rect.right() >= avai.right())
        {
            if (expt.left() > rect.width())
                rect.moveLeft(expt.right() - rect.width());
            else
                rect.moveLeft(avai.right() - rect.width());
        }
        if (expt.top() > rect.height() && rect.bottom() >= avai.bottom())
            rect.moveTop(expt.top() - rect.height());
    }

    // 移动窗口
    move(rect.topLeft());

    execute();
}

void FacileMenu::execute()
{
    current_index = -1;

    getBackgroupPixmap();

    // 有些重复显示的，需要再初始化一遍
    hidden_by_another = false;
    using_keyboard = false;
    closed_by_clicked = false;

    // 显示动画
    QWidget::show();
    setFocus();
    startAnimationOnShowed();
}

/**
 * 逐级隐藏菜单，并带有选中项动画
 */
void FacileMenu::toHide(int focusIndex)
{
    // 递归清理菜单（包括父菜单）焦点
    // 关闭的时候，会一层层的向上传递焦点
    // 虽然一般情况下问题不大，但在重命名（焦点不能丢失）时会出现严重问题
    FacileMenu* menu = this;
    do {
        menu->setFocusPolicy(Qt::NoFocus);
        menu = menu->parent_menu;
    } while (menu);
    this->clearFocus();

    closed_by_clicked = true;
    startAnimationOnHidden(focusIndex);
}

void FacileMenu::toClose()
{
    if (parent_menu)
        parent_menu->toClose();
    else
        this->close();
}

/**
 * 是因为点击了菜单项结束菜单
 * 还是因为其他原因，比如ESC关闭、鼠标点击其他位置呢
 */
bool FacileMenu::isClosedByClick() const
{
    return closed_by_clicked;
}

/**
 * 菜单结束的时候调用
 * 例如多选，确认多选后可调用此项
 */
FacileMenu* FacileMenu::finished(FuncType func)
{
    this->finished_func = new FuncType(func);
    return this;
}

/**
 * 添加可选菜单，快速添加多个单选项
 * @param texts  文字
 * @param states 选中状态
 * @param func   回调
 * @return
 */
FacileMenu *FacileMenu::addOptions(QList<QString> texts, QList<bool> states, FuncIntType clicked)
{
    int si = qMin(texts.size(), states.size());
    for (int i = 0; i < si; i++)
    {
        addAction(texts.at(i), [=]{
            clicked(i);
        })->check(states.at(i));
    }

    return this;
}

FacileMenu *FacileMenu::addOptions(QList<QString> texts, int select, FuncIntType clicked)
{
    QList<bool>states;
    for (int i = 0; i < texts.size(); i++)
        states << false;

    if (select >= 0 && select < states.size())
        states[select] = true;

    return addOptions(texts, states, clicked);
}

/**
 * 用于单选
 * 取消除了某项之外全部选择
 */
FacileMenu *FacileMenu::singleCheck(FacileMenuItem *item)
{
    uncheckAll(item);
    return this;
}

/**
 * 取消所有 checkable 的项的check
 * @param except 如果不为空，则设置这一项为true（相当于单选）
 * @param begin 开始取消选择的项，默认-1，从头开始
 * @param end   结束取消选择的项（不包括此项），默认-1，直到末尾
 */
FacileMenu *FacileMenu::uncheckAll(FacileMenuItem *except, int begin, int end)
{
    if (begin < 0 || begin >= items.size())
        begin = 0;
    if (end < 0 || end > items.size())
        end = items.size();
    for (int i = begin; i < end; i++)
    {
        auto item = items.at(i);
        if (!item->isCheckable())
            continue;
        item->setChecked(false);
    }
    if (except)
        except->setChecked(true);
    return this;
}

/**
 * 返回选中的菜单
 */
QList<FacileMenuItem *> FacileMenu::checkedItems()
{
    QList<FacileMenuItem*> checkeds;
    foreach (auto item, items)
    {
        if (item->isCheckable() && item->isChecked())
            checkeds.append(item);
    }
    return checkeds;
}

/**
 * 返回选中项的索引
 * 该索引不包括自定义控件、布局等
 */
QList<int> FacileMenu::checkedIndexes()
{
    QList<int> checkeds;
    for (int i = 0; i < items.size(); i++)
    {
        auto item = items.at(i);
        if (item->isCheckable() && item->isChecked())
            checkeds.append(i);
    }
    return checkeds;
}

/**
 * 返回选中项中的text列表
 */
QStringList FacileMenu::checkedItemTexts()
{
    QStringList texts;
    foreach (auto item, items)
    {
        if (item->isCheckable() && item->isChecked())
            texts.append(item->getText());
    }
    return texts;
}

/**
 * 返回选中项的自定义data列表
 * 如果没设置的话，会是无效的QVariant
 */
QList<QVariant> FacileMenu::checkedItemDatas()
{
    QList<QVariant> datas;
    foreach (auto item, items)
    {
        if (item->isCheckable() && item->isChecked())
            datas.append(item->getData());
    }
    return datas;
}

/**
 * 一键设置已有菜单项为单选项
 * 注意，一定要在添加后设置，否则对后添加的项无效
 */
FacileMenu *FacileMenu::setSingleCheck(FuncCheckType clicked)
{
    for (int i = 0; i < items.size(); i++)
    {
        auto item = items.at(i);
        if (!item->isCheckable())
            item->setCheckable(true);

        item->triggered([=]{
            item->toggle();
            if (clicked)
                clicked(i, item->isChecked());
        });
    }
    return this;
}

/**
 * 一键设置已经菜单项为多选项
 * 注意，一定要在添加后设置，否则对后添加的项无效
 */
FacileMenu *FacileMenu::setMultiCheck(FuncCheckType clicked)
{
    for (int i = 0; i < items.size(); i++)
    {
        auto item = items.at(i);
        item->setCheckable(true)->linger(); // 多选，点了一下不能消失

        item->triggered([=]{
            item->toggle();
            if (clicked)
                clicked(i, item->isChecked());
        });
    }
    return this;
}

/**
 * 设置右边提示的区域内容
 * 一般是快捷键
 * 尽量在添加菜单项前设置
 */
FacileMenu *FacileMenu::setTipArea(int x)
{
    addin_tip_area = x;
    return this;
}

/**
 * 设置右边提示的区域内容
 * 一般是用来放快捷键
 * 尽量在添加菜单项前设置
 * @param tip 内容是什么不重要，只要等同于需要容纳的最长字符串即可（例如"ctrl+shit+alt+s"）
 */
FacileMenu *FacileMenu::setTipArea(QString longestTip)
{
    QFontMetrics fm(this->font());
    addin_tip_area = fm.horizontalAdvance(longestTip + "Ctrl");
    // 修改现有的
    foreach (auto item, items)
        item->setPaddings(item_padding, addin_tip_area > 0 ? tip_area_spacing + addin_tip_area : item_padding, item_padding, item_padding);
    return this;
}

/**
 * 设置是否默认分割同一行的菜单项
 * 如果关闭，可手动使用 split() 或 addVSeparator() 分割
 */
FacileMenu *FacileMenu::setSplitInRow(bool split)
{
    split_in_row = split;
    return this;
}

FacileMenu *FacileMenu::setBorderRadius(int r)
{
    border_radius = r;
    foreach (auto item, items)
        item->subMenu() && item->subMenu()->setBorderRadius(r);
    return this;
}

FacileMenu *FacileMenu::setItemsHoverColor(QColor c)
{
    if (!c.isValid())
        return this;

    foreach (auto item, items)
        item->setHoverColor(c);
    return this;
}

FacileMenu *FacileMenu::setItemsPressColor(QColor c)
{
    if (!c.isValid())
        return this;

    foreach (auto item, items)
        item->setPressColor(c);
    return this;
}

FacileMenu *FacileMenu::setItemsTextColor(QColor c)
{
    if (!c.isValid())
        return this;

    foreach (auto item, items)
        item->setTextColor(c);
    return this;
}

FacileMenu *FacileMenu::setAppearAnimation(bool en)
{
    this->enable_appear_animation = en;
    foreach (auto item, items)
        item->subMenu() && item->subMenu()->setAppearAnimation(en);
    return this;
}

FacileMenu *FacileMenu::setDisappearAnimation(bool en)
{
    this->enable_disappear_animation = en;
    foreach (auto item, items)
        item->subMenu() && item->subMenu()->setDisappearAnimation(en);
    return this;
}

FacileMenu *FacileMenu::setSubMenuShowOnCursor(bool en)
{
    this->sub_menu_show_on_cursor = en;
    foreach (auto item, items)
        item->subMenu() && item->subMenu()->setSubMenuShowOnCursor(en);
    return this;
}

void FacileMenu::itemMouseEntered(FacileMenuItem *item)
{
    if (_showing_animation)
        return ;
    int index = items.indexOf(item);
    if (using_keyboard && current_index > -1 && current_index < items.size() && current_index != index) // 屏蔽键盘操作
        items.at(current_index)->discardHoverPress(true);
    current_index = index;

    if (current_sub_menu) // 进入这个action，其他action展开的子菜单隐藏起来
    {
        current_sub_menu->hidden_by_another = true;
        current_sub_menu->hide();
        current_sub_menu = nullptr;
    }
}

/**
 * 所有的创建菜单项的方法
 */
FacileMenuItem *FacileMenu::createMenuItem(QIcon icon, QString text)
{
    auto key = getShortcutByText(text);
    text.replace(QRegExp("&([\\w\\d])\\b"), "\\1");
    FacileMenuItem* item;
    if (!align_mid_if_alone)
    {
        item = new FacileMenuItem(icon, text, this);
    }
    else // 如果只有一项，则居中对齐
    {
        if (icon.isNull())
            item =  new FacileMenuItem(text, this);
        else if (text.isEmpty())
            item = new FacileMenuItem(icon, this);
        else
            item =  new FacileMenuItem(icon, text, this);
    }
    item->setKey(key);
    item->setAttribute(Qt::WA_LayoutUsesWidgetRect);

    setActionButton(item, adding_horizone);

    if (adding_horizone && row_hlayouts.size()) // 如果是正在添加横向按钮
    {
        if (split_in_row && row_hlayouts.last()->count() > 0)
            addVSeparator(); // 添加竖向分割线
        row_hlayouts.last()->addWidget(item);
    }
    else
    {
        // 超过了屏幕高度，需要添加另一列
        if (main_vlayout->sizeHint().height() + item->sizeHint().height() > window_height)
            createNextColumn();
        main_vlayout->addWidget(item);
    }
    items.append(item);
    last_added_item = item;

    return item;
}

Qt::Key FacileMenu::getShortcutByText(QString text) const
{
    Qt::Key key = Qt::Key_Exit;
    QRegularExpression re("&([\\d\\w])");
    auto match = re.match(text);
    if (match.hasMatch())
    {
        const QChar ch = match.capturedTexts().at(1).at(0); // 快捷键字符串
        if (ch >= '0' && ch <= '9')
            key = (Qt::Key)(Qt::Key_0 + (ch.toLatin1() - '0'));
        else if (ch >= 'a' && ch <= 'z')
            key = (Qt::Key)(Qt::Key_A + (ch.toUpper().toLatin1() - 'A'));
        else if (ch >= 'A' && ch <= 'Z')
            key = (Qt::Key)(Qt::Key_A + (ch.toUpper().toLatin1() - 'A'));
    }
    return key;
}

/**
 * 统一设置Action按钮（尺寸、颜色等）
 * @param btn     按钮
 * @param isChip  是否是小按钮（一行多个，右边不用空白）
 */
void FacileMenu::setActionButton(InteractiveButtonBase *btn, bool isChip)
{
    // 设置尺寸
    if (isChip)
    {
        btn->setPaddings(item_padding);
    }
    else
    {
        btn->setPaddings(item_padding, addin_tip_area > 0 ? tip_area_spacing + addin_tip_area : item_padding, item_padding, item_padding);
    }

    // 设置颜色
    btn->setNormalColor(Qt::transparent);
    btn->setHoverColor(hover_bg);
    btn->setPressColor(press_bg);
    btn->setTextColor(text_fg);

    QFont font(btn->font());
    font.setWeight(QFont::Medium);
    btn->setFont(font);
}

void FacileMenu::showSubMenu(FacileMenuItem *item)
{
    if (current_sub_menu)
    {
        if (item->subMenu() == current_sub_menu && !current_sub_menu->isHidden()) // 当前显示的就是这个子菜单
            return ;
        current_sub_menu->hidden_by_another = true; // 不隐藏父控件
        current_sub_menu->hide();
    }

    if (item->subMenu()->items.count() == 0) // 没有菜单项，不显示
        return ;

    current_sub_menu = item->subMenu();
    QPoint pos(-1, -1);
    QRect avai = window_rect;
    if (using_keyboard) // 键盘模式，不是跟随鼠标位置来的
    {
        // 键盘模式，相对于点击项的右边
        QPoint tl = mapToGlobal(item->pos());
        if (tl.x() + item->width() + current_sub_menu->width() < avai.width())
            pos.setX(tl.x() + item->width());
        else
            pos.setX(tl.x() - current_sub_menu->width());
        if (tl.y() + current_sub_menu->height() < avai.height())
            pos.setY(tl.y());
        else
            pos.setY(tl.y() - current_sub_menu->height());
    }
    if (sub_menu_show_on_cursor)
        current_sub_menu->exec(pos);
    else
    {
        auto geom = item->mapToGlobal(QPoint(0, 0));
        auto rect = QRect(geom, item->size());
        current_sub_menu->exec(rect, false, rect.topRight());
    }
    current_sub_menu->setKeyBoardUsed(using_keyboard);
}

/**
 * 根据 mouseMove 事件，判断鼠标的位置
 * 如果是在父菜单另一个item，则隐藏子菜单（接着触发关闭信号）
 * 如果是在父菜单的父菜单，则关闭子菜单后，父菜单也依次关闭
 * @param pos   鼠标位置（全局）
 * @param child 子菜单
 * @return      是否在父或递归父菜单的 geometry 中
 */
bool FacileMenu::isCursorInArea(QPoint pos, FacileMenu *child)
{
    // 不在这范围内
    if (!geometry().contains(pos))
    {
        // 在自己的父菜单那里
        if (isSubMenu() && parent_menu->isCursorInArea(pos, this)) // 如果这也是子菜单（已展开），则递归遍历父菜单
        {
            hidden_by_another = true;
            QTimer::singleShot(0, this, [=]{
                close(); // 把自己也隐藏了
            });
            return true;
        }
        // 在菜单栏那里
        int rst = -1;
        if (menu_bar && (rst = menu_bar->isCursorInArea(pos)) > -1)
        {
            menu_bar->triggerIfNot(rst, this);
            return true;
        }
        return false;
    }
    // 如果是正展开的这个子项按钮
    if (current_index > -1 && child && items.at(current_index)->subMenu() == child && items.at(current_index)->geometry().contains(mapFromGlobal(pos)))
        return false;
    // 在这个菜单内的其他按钮
    return true;
}

void FacileMenu::setKeyBoardUsed(bool use)
{
    using_keyboard = use;
    if (use && current_index == -1 && items.size()) // 还没有选中
    {
        items.at(current_index = 0)->simulateHover(); // 预先选中第一项
    }
}

bool FacileMenu::isSubMenu() const
{
    return parent_menu != nullptr;
}

/**
 * 菜单出现动画
 * 从光标位置依次出现
 */
void FacileMenu::startAnimationOnShowed()
{
    if (!enable_appear_animation)
        return ;

    main_vlayout->setEnabled(false);
    _showing_animation = true;
    QEasingCurve curve = easing_curve;
    int duration = 300;
    if (items.size() <= 1)
    {
        duration = 200;
    }

    // 从上往下的动画
    QPoint start_pos = mapFromGlobal(QCursor::pos());
    if (start_pos.x() < 0)
        start_pos.setX(0);
    else if (start_pos.x() > width())
        start_pos.setX(width());
    if (start_pos.y() < 0)
        start_pos.setY(0);
    else if (start_pos.y() > height())
        start_pos.setY(height());
    if (items.size() >= 1)
    {
        if (start_pos.y() == 0) // 最顶上
            start_pos.setY(-items.at(0)->height());
        if (start_pos.x() == width()) // 最右边
            start_pos.setX(width()/2);
    }
    for (int i = 0; i < items.size(); i++)
    {
        InteractiveButtonBase* btn = items.at(i);
        btn->setBlockHover(true);
        QPropertyAnimation* ani = new QPropertyAnimation(btn, "pos");
        ani->setStartValue(start_pos);
        ani->setEndValue(btn->pos());
        ani->setEasingCurve(curve);
        ani->setDuration(duration);
        connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
        connect(ani, &QPropertyAnimation::finished, btn, [=]{
            btn->setBlockHover(false);
        });
        ani->start();
    }
    for (int i = 0; i < other_widgets.size(); i++)
    {
        QWidget* btn = other_widgets.at(i);
        QPropertyAnimation* ani = new QPropertyAnimation(btn, "pos");
        ani->setStartValue(start_pos);
        ani->setEndValue(btn->pos());
        ani->setEasingCurve(curve);
        ani->setDuration(duration);
        connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
        ani->start();
    }

    // 分割线动画
    foreach (auto item, h_separators + v_separators)
        item->hide();
    QTimer::singleShot(duration, this, [=]{
        for (int i = 0; i < h_separators.size(); i++)
        {
            InteractiveButtonBase* btn = h_separators.at(i);
            btn->show();
            btn->setMinimumSize(0, 0);
            QPropertyAnimation* ani = new QPropertyAnimation(btn, "geometry");
            ani->setStartValue(QRect(btn->geometry().center(), QSize(1,1)));
            ani->setEndValue(btn->geometry());
            ani->setEasingCurve(QEasingCurve::OutQuad);
            ani->setDuration(duration);
            connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
            ani->start();
        }
        for (int i = 0; i < v_separators.size(); i++)
        {
            InteractiveButtonBase* btn = v_separators.at(i);
            btn->show();
            btn->setMinimumSize(0, 0);
            QPropertyAnimation* ani = new QPropertyAnimation(btn, "geometry");
            ani->setStartValue(QRect(btn->geometry().center(), QSize(1,1)));
            ani->setEndValue(btn->geometry());
            ani->setEasingCurve(QEasingCurve::OutQuad);
            ani->setDuration(duration);
            connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
            ani->start();
        }
    });

    QTimer::singleShot(duration, this, [=]{
        main_vlayout->setEnabled(true);
        _showing_animation = false;
    });
}

/**
 * 关闭前显示隐藏动画
 * @param focusIndex 聚焦的item，如果不存在则为-1
 */
void FacileMenu::startAnimationOnHidden(int focusIndex)
{
    if (!enable_disappear_animation)
    {
        if (focusIndex > -1)
        {
            // 等待点击动画结束
            QTimer::singleShot(100, [=]{
                close();
            });
        }
        else
        {
            close();
        }
        return ;
    }

    _showing_animation = true;
    // 控件移动动画
    main_vlayout->setEnabled(false);
    int dur_min =100, dur_max = 200;
    int up_flow_count = focusIndex > -1 ? qMax(focusIndex, items.size()-focusIndex-1) : -1;
    int up_end = items.size() ? -items.at(0)->height() : 0;
    int flow_end = height();
    int focus_top = focusIndex > -1 ? items.at(focusIndex)->pos().y() : 0;
    for (int i = 0; i < items.size(); i++)
    {
        InteractiveButtonBase* btn = items.at(i);
        // QPoint pos = btn->pos();
        btn->setBlockHover(true);
        QPropertyAnimation* ani = new QPropertyAnimation(btn, "pos");
        ani->setStartValue(btn->pos());
        ani->setEasingCurve(QEasingCurve::OutCubic);
        if (focusIndex > -1)
        {
            if (i < focusIndex) // 上面的项
            {
                ani->setEndValue(QPoint(0, up_end - (focus_top - btn->pos().y()) / 8));
                ani->setDuration(dur_max - qAbs(focusIndex-i)*(dur_max-dur_min)/up_flow_count);
            }
            else if (i == focusIndex) // 中间的项
            {
                ani->setEndValue(btn->pos());
                ani->setDuration(dur_max);
            }
            else // 下面的项
            {
                ani->setEndValue(QPoint(0, flow_end + (btn->pos().y()-focus_top) / 8));
                ani->setDuration(dur_max - qAbs(i-focusIndex)*(dur_max-dur_min)/up_flow_count);
            }
        }
        else
        {
            ani->setEndValue(QPoint(0, up_end));
            ani->setDuration(dur_max);
        }
        connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
        connect(ani, &QPropertyAnimation::finished, btn, [=]{
            btn->setBlockHover(false);
            // btn->move(pos);
        });
        ani->start();
    }

    // 第三方控件动画
    for (int i = 0; i < other_widgets.size(); i++)
    {
        QWidget* btn = other_widgets.at(i);
        QPoint pos = btn->pos();
        QPropertyAnimation* ani = new QPropertyAnimation(btn, "pos");
        ani->setStartValue(btn->pos());
        ani->setEasingCurve(QEasingCurve::OutCubic);
        if (focusIndex > -1)
        {
            if (i < focusIndex) // 上面的项
            {
                ani->setEndValue(QPoint(0, up_end - (focus_top - btn->pos().y()) / 8));
                ani->setDuration(dur_max - qAbs(focusIndex-i)*(dur_max-dur_min)/up_flow_count);
            }
            else if (i == focusIndex) // 中间的项
            {
                ani->setEndValue(btn->pos());
                ani->setDuration(dur_max);
            }
            else // 下面的项
            {
                ani->setEndValue(QPoint(0, flow_end + (btn->pos().y()-focus_top) / 8));
                ani->setDuration(dur_max - qAbs(i-focusIndex)*(dur_max-dur_min)/up_flow_count);
            }
        }
        else
        {
            ani->setEndValue(QPoint(0, up_end));
            ani->setDuration(dur_max);
        }
        connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
        // connect(ani, &QPropertyAnimation::finished, btn, [=]{
        //     btn->move(pos);
        // });
        ani->start();
    }

    // 变淡动画（针对Popup，一切透明无效）
    /*QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(this);
    effect->setOpacity(1);
    setGraphicsEffect(effect);
    QPropertyAnimation* opa_ani = new QPropertyAnimation(effect, "opacity");
    opa_ani->setDuration(dur_max * 0.8);
    opa_ani->setStartValue(1);
    opa_ani->setEndValue(0);
    connect(opa_ani, &QPropertyAnimation::finished, this, [=]{
        opa_ani->deleteLater();
        effect->deleteLater();
    });
    opa_ani->start();*/

    // 真正关闭
    QTimer::singleShot(dur_max, this, [=]{
        _showing_animation = false;
        // 挨个还原之前的位置（不知道为什么main_vlayout不能恢复了）
        main_vlayout->setEnabled(true);
        main_vlayout->activate(); // 恢复原来的位置

        close();
    });
}

void FacileMenu::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    main_vlayout->setEnabled(true);
    main_vlayout->invalidate(); // 不清除缓存的话 activate 会false
    main_vlayout->activate();
}

void FacileMenu::hideEvent(QHideEvent *event)
{
    emit signalHidden();
    this->close(); // 子菜单关闭，不会导致自己关闭，需要手动close
    return QWidget::hideEvent(event);
}

void FacileMenu::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);

    QPoint pos = QCursor::pos();
    int rst = -1;
    if ((_showing_animation && !menu_bar) || isCursorInArea(pos)) // 正在出现或在自己的区域内，不管
        ;
    else if (menu_bar && (rst = menu_bar->isCursorInArea(pos)) > -1) // 在菜单栏
    {
        menu_bar->triggerIfNot(rst, this);
    }
    else if (parent_menu && parent_menu->isCursorInArea(pos, this)) // 在父类，自己隐藏
    {
        this->hide();
        parent_menu->setFocus();
    }

    if (using_keyboard)
    {
        using_keyboard = false;
        if (current_index >= 0 && current_index <= items.size())
            items.at(current_index)->discardHoverPress(false);
    }
}

void FacileMenu::keyPressEvent(QKeyEvent *event)
{
    auto key = event->key();
    foreach (auto item, items)
    {
        if (item->isKey((Qt::Key)key))
        {
            _showing_animation = false; // 在showing的时候，点击是无效的，所以要关掉
            item->simulateStatePress(); // 确定是这个action的快捷键
            return ;
        }
    }

    // 菜单按键响应
    if (event->modifiers() != Qt::NoModifier || items.size() == 0)
        return QWidget::keyPressEvent(event);

    switch (key) {
    case Qt::Key_Up:
    {
        if (current_index < 0 || current_index >= items.size())
            current_index = items.size()-1;
        else
        {
            items.at(current_index--)->discardHoverPress(true);
            if (current_index < 0)
                current_index = items.size()-1;
        }
        auto focusToHoriFirst = [&]{
            // 如果是横向按钮，理应聚焦到第一项；如果第一项不可用，依次延后
            if (current_index > 0 && items.at(current_index-1)->pos().y() == items.at(current_index)->pos().y())
            {
                int last_index = current_index;
                // 获取当前行的第一项
                int y = items.at(current_index)->pos().y();
                while (current_index > 0 && items.at(current_index-1)->pos().y() == y)
                    current_index--;
                int first_index = current_index;
                // 移动到第一项可用的项
                while (current_index < last_index && !items.at(current_index)->isEnabled())
                    current_index++;
                // 如果这一行全部不可用，强制回到第一项
                if (current_index == last_index)
                    current_index = first_index;
            }
        };
        focusToHoriFirst();
        // 判断 item 是否被禁用
        if (!items.at(current_index)->isEnabled())
        {
            int old_index = current_index;
            do {
                current_index--;
                if (current_index < 0)
                    current_index = items.size()-1;
                else
                    focusToHoriFirst();
            } while (current_index != old_index && !items.at(current_index)->isEnabled());
            if (current_index == old_index) // 如果绕了一圈都不能用，取消
                return ;
        }
        // 找到真正的上一项
        items.at(current_index)->simulateHover();
        using_keyboard = true;
        return ;
    }
    case Qt::Key_Down:
    {
        if (current_index < 0 || current_index >= items.size())
            current_index = 0;
        else
        {
            items.at(current_index++)->discardHoverPress(true);
            if (current_index >= items.size())
                current_index = 0;
        }
        auto focusIgnoreHorizoneRest = [&]{
            // 跳过同一行后面所有（至少要先聚焦在这一行的第二项）
            if (current_index > 0 && current_index < items.size()-1 && items.at(current_index-1)->pos().y() == items.at(current_index)->pos().y())
            {
                int y = items.at(current_index)->pos().y();
                // 先判断前面是不是有可以点击的
                int temp_index = current_index;
                bool is_line_second = false;
                while (--temp_index > 0 && items.at(temp_index)->pos().y() == y)
                {
                    if (items.at(temp_index)->isEnabled())
                    {
                        is_line_second = true;
                        break;
                    }
                }
                if (!is_line_second) // 这是这一行的第一项，可以聚焦
                    return ;
                // 跳过这一行后面所有的按钮
                while (current_index < items.size()-1 && items.at(current_index+1)->pos().y() == y)
                    current_index++;
                current_index++;
                if (current_index >= items.size())
                    current_index = 0;
            }
        };
        focusIgnoreHorizoneRest();
        // 判断 item 是否被禁用
        if (!items.at(current_index)->isEnabled())
        {
            int old_index = current_index;
            do {
                current_index++;
                if (current_index >= items.size())
                    current_index = 0;
                else
                    focusIgnoreHorizoneRest();
            } while (current_index != old_index && !items.at(current_index)->isEnabled());
            if (current_index == old_index) // 如果绕了一圈都不能用，取消
                return ;
        }
        // 找到真正的上一项
        items.at(current_index)->simulateHover();
        using_keyboard = true;
        return ;
    }
    case Qt::Key_Left:
        // 移动到左边按钮
        if (current_index > 0 && items.at(current_index-1)->pos().y() == items.at(current_index)->pos().y())
        {
            items.at(current_index--)->discardHoverPress(true);
            // 找到左边第一项能点击的按钮；如果没有，宁可移动到上几行
            if (!items.at(current_index)->isEnabled())
            {
                int y = items.at(current_index)->pos().y();
                int ori_index = current_index;
                /*while (--current_index >= 0 && !items.at(current_index)->isEnabled()) ;
                if (current_index == -1) // 前面没有能选的了
                {
                    current_index = ori_index + 1; // 恢复到之前选择的那一项
                    return ;
                }*/
                while (--current_index >= 0 && !items.at(current_index)->isEnabled() && items.at(current_index)->pos().y() == y) ;
                if (current_index < 0 || items.at(current_index)->pos().y() != y) // 前面没有能选的了
                    current_index = ori_index + 1; // 恢复到之前选择的那一项
            }
            // 聚焦到这个能点的按钮
            items.at(current_index)->simulateHover();
            using_keyboard = true;
        }
        // 退出子菜单
        else if (isSubMenu())
            close();
        return ;
    case Qt::Key_Right:
        // 移动到右边按钮
        if (current_index < items.size()-1 && items.at(current_index+1)->pos().y() == items.at(current_index)->pos().y())
        {
            items.at(current_index++)->discardHoverPress(true);
            // 找到右边第一项能点击的按钮；如果没有，宁可移动到下几行
            if (!items.at(current_index)->isEnabled())
            {
                int y = items.at(current_index)->pos().y();
                int ori_index = current_index;
                /*while (++current_index < items.size() && !items.at(current_index)->isEnabled()) ;
                if (current_index == items.size()) // 后面没有能选的了
                {
                    current_index = ori_index - 1; // 恢复到之前选择的那一项
                    return ;
                }*/
                while (++current_index < items.size() && !items.at(current_index)->isEnabled() && items.at(current_index)->pos().y() == y) ;
                if (current_index == items.size() || items.at(current_index)->pos().y() != y) // 后面没有能选的了
                    current_index = ori_index - 1; // 恢复到之前选择的那一项
            }
            // 聚焦到这个能点的按钮
            items.at(current_index)->simulateHover();
            using_keyboard = true;
        }
        // 展开子菜单
        else if (current_index >= 0 && current_index < items.size() && items.at(current_index)->isSubMenu())
        {
            showSubMenu(items.at(current_index));
        }
        return ;
    case Qt::Key_Home:
        if (current_index >= 0 || current_index < items.size())
            items.at(current_index)->discardHoverPress(true);
        // 聚焦到第一项能点的按钮
        if (!items.at(current_index = 0)->isEnabled())
        {
            while (++current_index < items.size() && !items.at(current_index)->isEnabled());
            if(current_index == items.size()) // 没有能点的（不太可能）
                return ;
        }
        items.at(current_index)->simulateHover();
        using_keyboard = true;
        return ;
    case Qt::Key_End:
        if (current_index >= 0 || current_index < items.size())
            items.at(current_index)->discardHoverPress(true);
        // 聚焦到最后一项能点的按钮
        if (!items.at(current_index = items.size()-1)->isEnabled())
        {
            while (--current_index >= 0 && !items.at(current_index)->isEnabled());
            if(current_index == -1) // 没有能点的（不太可能）
                return ;
        }
        items.at(current_index)->simulateHover();
        using_keyboard = true;
        return ;
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Space:
        if (current_index >= 0 || current_index < items.size())
        {
            auto item = items.at(current_index);
            item->simulateStatePress(!item->getState(), false);
        }
        return ;
    }

    return QWidget::keyPressEvent(event);
}

void FacileMenu::getBackgroupPixmap()
{
    /// 设置背景为圆角矩形
    if (height() > 0 && border_radius) // 没有菜单项的时候为0
    {
        QPixmap pixmap(width(), height());
        pixmap.fill(Qt::transparent);
        QPainter pix_ptr(&pixmap);
        pix_ptr.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(0, 0, width(), height(), border_radius, border_radius);
        pix_ptr.fillPath(path, Qt::white);
        setMask(pixmap.mask());
    }

    // 是否捕获背景模糊图片
    if (blur_bg_alpha <= 0)
        return;

    /// 获取图片
    QRect rect = this->geometry();
    int radius = qMin(64, qMin(width(), height())); // 模糊半径，也是边界
    int cut = radius;
    rect.adjust(-cut, -cut, +cut, +cut);

    // 屏幕信息
    QScreen* screen = QApplication::screenAt(QCursor::pos());
    if (!screen)
    {
        qWarning() << "无法获取屏幕";
        return;
    }
    // 屏幕截图
    QPixmap bg = screen->grabWindow(QApplication::desktop()->winId(), rect.left(), rect.top(), rect.width(), rect.height());
    // 在Mac的Retina显示屏上，QScreen::grabWindow() 返回的是物理像素图像，而不是逻辑像素
    // 设备像素比（Device Pixel Ratio）：Retina Mac通常为2.0
    qreal devicePixelRatio = screen->devicePixelRatio();

    // 当截屏有问题的时候，判断是否纯黑
    static bool is_all_blank = false;
    static bool judged = false;
    if (!judged)
    {
        judged = true;
        QColor color = bg.scaled(1, 1).toImage().pixelColor(0, 0);
        is_all_blank = (color == QColor(0, 0, 0));
        if (is_all_blank)
        {
            qWarning() << "FacileMenu: Scrollshot is all black, disabled blur background";
        }
    }
    if (is_all_blank)
        return;

    /// 获取图片主题色
    if (auto_dark_mode || auto_theme_by_bg)
    {
        if (!all_menu_same_color || !parent_menu) // 如果是统一判断，那么由最外层计算
        {
            QColor img_fg, img_bg, img_sg;
            bool ok = ImageUtil::getBgFgSgColor(ImageUtil::extractImageThemeColors(bg.toImage(), 8), &img_bg, &img_fg, &img_sg);
            if (ok)
            {
                // 使用主题色
                if (auto_theme_by_bg)
                {
                    // 加载色彩列表
                    static QList<QColor> colors;
                    static QList<QVector3D> colorLabs;
                    if (colors.isEmpty())
                    {
                        QString path = ":/documents/color_list";
                        QFile file(path);
                        if (file.open(QIODevice::ReadOnly))
                        {
                            QTextStream in(&file);
                            QString line;
                            while (in.readLineInto(&line))
                            {
                                QStringList list = line.split(" ");
                                if (list.size() == 2)
                                {
                                    QString name = list[0];
                                    QString color = list[1];
                                    colors << QColor(color);
                                    colorLabs << ImageUtil::rgbToLab(QColor(color));
                                }
                            }
                            file.close();

                            if (colors.isEmpty())
                            {
                                qWarning() << "无法加载建议的色彩列表";
                            }
                        }
                    }

                    // 各个颜色使用与色彩列表中最接近的颜色（方差）
                    img_bg = ImageUtil::getVisuallyClosestColorByCIELAB(img_bg, colors, colorLabs);
                    img_fg = ImageUtil::getVisuallyClosestColorByCIELAB(img_fg, colors, colorLabs);
                    img_sg = ImageUtil::getVisuallyClosestColorByCIELAB(img_sg, colors, colorLabs);
                    
                    // 设置颜色
                    m_bg_color = img_bg;
                    setItemsTextColor(img_fg);

                    QColor hover_color = img_sg;
                    hover_color.setAlpha(64);
                    setItemsHoverColor(hover_color);
                    QColor press_color = img_sg;
                    press_color.setAlpha(128);
                    setItemsPressColor(press_color);

                    if (all_menu_same_color && !this->parent_menu)
                    {
                        FacileMenu::text_fg = img_fg;
                        FacileMenu::normal_bg = m_bg_color;
                        FacileMenu::hover_bg = hover_color;
                        FacileMenu::press_bg = press_color;
                    }
                }
                else if (auto_dark_mode) // 判断是否是夜间模式
                {
                    double light = ImageUtil::calculateLuminance(img_bg);
                    if (light < 40) // 暗色
                    {
                        m_bg_color = QColor("#121212");
                        setItemsTextColor(QColor("#E0E0E0"));

                        if (all_menu_same_color && !this->parent_menu)
                        {
                            FacileMenu::text_fg = QColor("#E0E0E0");
                            FacileMenu::normal_bg = QColor("#121212");
                        }
                    }
                    else if (light > 120) // 亮色
                    {
                        // 默认就是亮色，不做改变
                        if (all_menu_same_color && !this->parent_menu)
                        {
                            FacileMenu::text_fg = QColor(0, 0, 0);
                            FacileMenu::normal_bg = QColor(255, 255, 255);
                        }
                    }
                }

            }
        }
        else // 统一颜色的子菜单，只要使用父菜单的样式即可
        {
            m_bg_color = normal_bg;
            setItemsTextColor(text_fg);
            setItemsHoverColor(hover_bg);
            setItemsPressColor(press_bg);
        }
    }

    /// 模糊图片
    QPixmap pixmap = bg;
    QPainter painter(&pixmap);
    // 填充半透明的背景颜色，避免太透
    QColor bg_c(m_bg_color);
    bg_c.setAlpha(m_bg_color.alpha() * (100 - blur_bg_alpha) / 100);
    painter.fillRect(0, 0, pixmap.width(), pixmap.height(), bg_c);

    // 开始模糊
    QImage img = pixmap.toImage(); // img -blur-> painter(pixmap)
    QT_BEGIN_NAMESPACE
    extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage,
                                              qreal radius, bool quality,
                                              bool alphaOnly, int transposed = 0);
    QT_END_NAMESPACE

    // 使用更高质量的模糊 (多次小半径模糊)
    for (int i = 0; i < 3; i++) {
        qt_blurImage(&painter, img, radius / 3.0, true, false);
    }

    // 裁剪掉边缘（模糊后会有黑边）
    cut = cut * devicePixelRatio; // 按照图像进行缩放
    QRect copy_rect(cut, cut, pixmap.width()-cut*2, pixmap.height()-cut*2);

    // 设置为背景
    bg_pixmap = pixmap.copy(copy_rect);
}

void FacileMenu::paintEvent(QPaintEvent *event)
{
    if (!bg_pixmap.isNull())
    {
        QPainter painter(this);

        // 绘制背景模糊
        painter.drawPixmap(QRect(0,0,width(),height()), bg_pixmap);

        // 绘制鼠标高光
        {
            QPoint pos = mapFromGlobal(QCursor::pos());
            // 绘制半径为min(width(), height())/2的渐变圆，从中心到边缘逐渐透明（0.3->0）
            int r = qMin(width(), height()) / 2;
            
            // 创建径向渐变
            QRadialGradient gradient(pos, r);
            gradient.setColorAt(0, QColor(m_bg_color.red(), m_bg_color.green(), m_bg_color.blue(), 76)); // 0.3 * 255 ≈ 76
            gradient.setColorAt(1, QColor(m_bg_color.red(), m_bg_color.green(), m_bg_color.blue(), 0));
            
            // 设置画笔
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            
            // 绘制渐变圆
            painter.drawEllipse(pos, r, r);
        }
        return ;
    }
    QWidget::paintEvent(event);
}

void FacileMenu::setColors(QColor normal, QColor hover, QColor press, QColor text)
{
    normal_bg = normal;
    hover_bg = hover;
    press_bg = press;
    text_fg = text;
}
