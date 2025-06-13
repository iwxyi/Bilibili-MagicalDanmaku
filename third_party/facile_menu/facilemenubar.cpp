#include "facilemenubar.h"
#include "facilemenuanimation.h"

FacileMenuBar::FacileMenuBar(QWidget *parent) : QWidget(parent)
{
    hlayout = new QHBoxLayout(this);
    hlayout->setAlignment(Qt::AlignLeft);
    hlayout->setSpacing(0);
    hlayout->setMargin(0);
    setLayout(hlayout);
}

/// 鼠标是否在这个区域内
/// @param pos 绝对位置
/// @return 所在的 MenuButton 索引，-1表示不在
int FacileMenuBar::isCursorInArea(QPoint pos) const
{
    QPoint mPos = mapFromGlobal(pos);
    if (!rect().contains(mPos))
        return -1;
    for (int i = 0; i < buttons.size(); i++)
    {
        if (buttons.at(i)->geometry().contains(mPos))
            return i;
    }
    return -1;
}

int FacileMenuBar::currentIndex() const
{
    return _currentIndex;
}

/// 准备激活第index个菜单
/// 如果不是参数menu，则激活该菜单
bool FacileMenuBar::triggerIfNot(int index, void *menu)
{
    // 非菜单栏中的按钮，隐藏全部
    FacileMenu* m = static_cast<FacileMenu*>(menu);
    if (index < 0 || index >= buttons.size())
    {
        if (m)
            m->hide();
        return true;
    }

    // 切换按钮
    if (menus.at(index) == menu)
    {
        return false;
    }

    if (m && enableAnimation)
    {
        // 带动画的trigger
        switchTrigger(index, menus.indexOf(m));
    }
    else
    {
        if (m)
            m->close();
        trigger(index);
    }

    return true;
}

void FacileMenuBar::addMenu(QString name, FacileMenu *menu)
{
    auto btn = createButton(name, menu);

    buttons.append(btn);
    menus.append(menu);
    hlayout->addWidget(btn);
}

void FacileMenuBar::insertMenu(int index, QString name, FacileMenu *menu)
{
    auto btn = createButton(name, menu);

    buttons.insert(index, btn);
    menus.insert(index, menu);
    hlayout->insertWidget(index, btn);
}

void FacileMenuBar::deleteMenu(int index)
{
    if (index < 0 || index >= buttons.size())
        return ;
    buttons.takeAt(index)->deleteLater();
    menus.takeAt(index)->deleteLater();
}

int FacileMenuBar::count() const
{
    return buttons.size();
}

void FacileMenuBar::setAnimationEnabled(bool en)
{
    this->enableAnimation = en;
}

/// 显示菜单
void FacileMenuBar::trigger(int index)
{
    if (index < 0 || index >= buttons.size())
        return ;

    if (aniWidget)
    {
        aniWidget->deleteLater();
        aniWidget = nullptr;
    }

    InteractiveButtonBase* btn = buttons.at(index);
    btn->setBgColor(FacileMenu::hover_bg);
    QRect rect(btn->mapToGlobal(QPoint(0, 0)), btn->size());
    if (enableAnimation)
        menus.at(index)->setAppearAnimation(true); // 要重新设置一下，因为取消掉了
    menus.at(index)->exec(rect, true);
    _currentIndex = index;
}

void FacileMenuBar::switchTrigger(int index, int prevIndex)
{
    if (index < 0 || index >= buttons.size())
        return ;
    if (prevIndex < 0 || prevIndex >= buttons.size())
        return trigger(index);

    InteractiveButtonBase* btn = buttons.at(index);
    QRect rect(btn->mapToGlobal(QPoint(0, 0)), btn->size());

    FacileMenu* m = menus.at(index);
    m->setAppearAnimation(false); // 必须要关闭显示动画
    menus.at(prevIndex)->hide(); // 避免影响背景
    m->exec(rect, true);

    if (aniWidget)
    {
        aniWidget->deleteLater();
        aniWidget = nullptr;
    }

    auto w = new FacileSwitchWidget(menus.at(prevIndex), menus.at(index));
    aniWidget = w;
    QPoint pos = m->pos();
    m->move(-10000 - m->width(), -10000 - m->height()); // 要保持显示，才能监控鼠标位置；因为可能立刻移开了
    btn->setBgColor(FacileMenu::hover_bg); // 上面的hide会触发关闭事件，取消颜色
    connect(w, &FacileSwitchWidget::finished, w, [=]{
        m->move(pos);
        m->show();
        aniWidget = nullptr; // 自动deleteLater
    });
    _currentIndex = index;
}

InteractiveButtonBase *FacileMenuBar::createButton(QString name, FacileMenu *menu)
{
    InteractiveButtonBase* btn = new InteractiveButtonBase(name, this);
    // TODO: name 识别并注册快捷键
    connect(btn, &InteractiveButtonBase::clicked, this, [=]{
        int index = buttons.indexOf(btn);
        if (!menus.at(index)->isHidden())
        {
            // 如果是正在显示状态
            // 但是理论上来说不可能，因为这种情况是点不到按钮的
            menus.at(index)->hide();
        }
        else
        {
            // 点击触发菜单
            trigger(buttons.indexOf(btn));
        }
    });
    connect(menu, &FacileMenu::signalHidden, btn, [=]{
        btn->setBgColor(Qt::transparent);
        _currentIndex = -1;
    });

    btn->adjustMinimumSize();
    btn->setRadius(5);
    btn->setFixedForePos();

    menu->setMenuBar(this);
    menu->setAttribute(Qt::WA_DeleteOnClose, false);
//    menu->setAppearAnimation(false); // 设置显示动画
    menu->setDisappearAnimation(false); // 设置关闭动画
    menu->setSubMenuShowOnCursor(false);
    if (enableAnimation)
        menu->setBorderRadius(0);
    return btn;
}
