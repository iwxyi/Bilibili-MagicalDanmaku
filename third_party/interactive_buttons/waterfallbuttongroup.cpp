#include "waterfallbuttongroup.h"
#include <QStyleOption>

WaterFallButtonGroup::WaterFallButtonGroup(QWidget *parent)
    : QWidget(parent),
      normal_bg(128,128,128,32),
      hover_bg(100,149,237,128),
      press_bg(100,149,237),
      selected_bg(100,149,237,128),
      normal_ft(0,0,0),
      selected_ft(255,255,255)
{

}

void WaterFallButtonGroup::initStringList(QStringList list, QStringList selected)
{
    foreach (QString s, list)
    {
        addButton(s, selected.contains(s));
    }
}

/**
 * 选择指定项
 * 会清除已有的选择
 */
void WaterFallButtonGroup::setSelects(QStringList list)
{
    selectable = true;
    foreach (InteractiveButtonBase *btn, btns)
    {
        if (btn->getText().isEmpty())
            continue;
        if (list.contains(btn->getText()) && !btn->getState())
            changeBtnSelect(btn);
        else if (!list.contains(btn->getText()) && btn->getState())
            changeBtnSelect(btn);
    }
}

void WaterFallButtonGroup::setSelect(QString text)
{
    foreach (InteractiveButtonBase *btn, btns)
    {
        if (btn->getText() == text)
        {
            changeBtnSelect(btn);
            break;
        }
    }
}

void WaterFallButtonGroup::setSelect(int index)
{
    if (index < 0 || index >= btns.size())
        return;

    changeBtnSelect(btns.at(index));
}

WaterFloatButton* WaterFallButtonGroup::addButton(QString s, bool selected)
{
    return addButton(s, QColor(), selected);
}

WaterFloatButton* WaterFallButtonGroup::addButton(QString s, QColor c, bool selected)
{
    WaterFloatButton* btn = new WaterFloatButton(s, this);
    btn->setFixedForeSize();
    setBtnColors(btn);
    btn->show();
    btns.append(btn);

    if (selected)
        changeBtnSelect(btn);

    btn->setAutoTextColor(false);
    if (c.isValid())
        btn->setTextColor(c);
    connect(btn, &InteractiveButtonBase::clicked, this, [=]{
        slotButtonClicked(btn);
    });

    btn->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(btn, &InteractiveButtonBase::customContextMenuRequested, this, [=](QPoint){
        emit signalRightClicked(s);
    });
    /* connect(btn, &InteractiveButtonBase::rightClicked, this, [=]{
        emit signalRightClicked(s);
    }); */
}

void WaterFallButtonGroup::slotButtonClicked(InteractiveButtonBase* btn)
{
    QString s = btn->getText();
    if (selectable)
    {
        if (single_select) // 单选，只选择它
        {
            if (btn->getState()) // 已经选择了，则不进行处理
                return;

            // 取消所有选择
            foreach (auto btn, btns)
            {
                if (btn->getState())
                    unselectBtn(btn);
            }

            // 只选择它
            selectBtn(btn);
        }
        else // 多选，那么可以取消选择
        {
            changeBtnSelect(btn);
        }

        if (btn->getState())
            emit signalSelected(s);
        else
            emit signalUnselected(s);
        emit signalSelectChanged();
    }
    else
    {
        emit signalClicked(s);
    }
}

QStringList WaterFallButtonGroup::getSelectedTexts() const
{
    QStringList results;
    for (int i = 0; i < btns.size(); i++)
    {
        if (btns.at(i)->getState())
            results.append(btns.at(i)->getText());
    }
    return results;
}

QList<int> WaterFallButtonGroup::getSelectedIndexes() const
{
    QList<int> results;
    for (int i = 0; i < btns.size(); i++)
    {
        if (btns.at(i)->getState())
            results.append(i);
    }
    return results;
}

void WaterFallButtonGroup::clear()
{
    for (int i = 0; i < btns.size(); i++)
    {
        btns.at(i)->deleteLater();
    }
    btns.clear();
}

void WaterFallButtonGroup::setColors(QColor normal_bg, QColor hover_bg, QColor press_bg, QColor selected_bg, QColor normal_ft, QColor selected_ft)
{
    this->normal_bg = normal_bg;
    this->hover_bg = hover_bg;
    this->press_bg = press_bg;
    this->selected_bg = selected_bg;
    this->normal_ft = normal_ft;
    if (selected_ft != Qt::transparent)
        this->selected_ft = selected_ft;
    else
        selected_ft = getReverseColor(selected_bg);
}

void WaterFallButtonGroup::setSelectedColor(QColor color)
{
    this->selected_bg = color;
}

void WaterFallButtonGroup::updateBtnColors()
{
    foreach (InteractiveButtonBase *btn, btns)
    {
        setBtnColors(btn);
    }
}

int WaterFallButtonGroup::count() const
{
    return btns.size();
}

QList<InteractiveButtonBase *> WaterFallButtonGroup::getButtons() const
{
    return btns;
}

void WaterFallButtonGroup::setSelectable(bool enable)
{
    this->selectable = enable;
}

void WaterFallButtonGroup::setSingleSelect(bool enable)
{
    this->single_select = enable;
    setSelectable(true);
}

void WaterFallButtonGroup::updateButtonPositions()
{
    const int space_h = 3, space_v = 3;
    int total_w = this->width(), w = space_v;
    int total_h = 0;
    // 自动调整位置
    foreach (InteractiveButtonBase* btn, btns)
    {
        int btn_w = btn->width();
        if (w == 0 || w + btn_w <= total_w) // 开头，或者同一行
        {
            btn->move(w, total_h);
            w += btn_w + space_h;
        }
        else // 另起一行
        {
            total_h += btn->height() + space_v;
            w = space_h;
            btn->move(w, total_h);
            w += btn_w + space_h;
        }
    }
    if (btns.size())
        total_h += btns.last()->height();
    this->setFixedHeight(total_h);
}

void WaterFallButtonGroup::resizeEvent(QResizeEvent *event)
{
    updateButtonPositions();

    QWidget::resizeEvent(event);
}

void WaterFallButtonGroup::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

QColor WaterFallButtonGroup::getReverseColor(QColor color)
{
    return QColor(
            getReverseChannel(color.red()),
            getReverseChannel(color.green()),
            getReverseChannel(color.blue())
                );
}

int WaterFallButtonGroup::getReverseChannel(int x)
{
    if (x < 92 || x > 159)
        return 255 - x;
    else if (x < 128)
        return 255;
    else // if (x > 128)
        return 0;
}

void WaterFallButtonGroup::setBtnColors(InteractiveButtonBase *btn)
{
    btn->setBgColor(normal_bg);
    btn->setBgColor(hover_bg, press_bg);
}

void WaterFallButtonGroup::changeBtnSelect(InteractiveButtonBase *btn)
{
    if (btn->getState()) // 已选中，取消选中
    {
        unselectBtn(btn);
    }
    else // 未选中，进行选中
    {
        selectBtn(btn);
    }
}

void WaterFallButtonGroup::selectBtn(InteractiveButtonBase *btn)
{
    btn->setState(true);
    btn->setBgColor(selected_bg);
}

void WaterFallButtonGroup::unselectBtn(InteractiveButtonBase *btn)
{
    btn->setState(false);
    btn->setBgColor(normal_bg);
}
