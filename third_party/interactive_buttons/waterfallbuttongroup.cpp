#include "waterfallbuttongroup.h"

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
    clearButtons();
    foreach (QString s, list)
    {
        if (s.trimmed().isEmpty())
            continue;
        addButton(s, selected.contains(s));
    }
    updateButtonPositions();
}

void WaterFallButtonGroup::setSelects(QStringList list)
{
    foreach (InteractiveButtonBase *btn, btns)
    {
        if (btn->getText().isEmpty())
            continue;
        if (list.contains(btn->getText()) && !btn->getState())
            selectBtn(btn);
        else if (!list.contains(btn->getText()) && btn->getState())
            selectBtn(btn);
    }
}

void WaterFallButtonGroup::addButton(QString s, bool selected)
{
    WaterFloatButton* btn = new WaterFloatButton(s, this);
    btn->setFixedForeSize();
    btn->show();
    setBtnColors(btn);
    btns.append(btn);

    if (selected)
        selectBtn(btn);

    btn->setAutoTextColor(false);
    connect(btn, &InteractiveButtonBase::clicked, this, [=]{
        if (!selectable)
            return ;

        selectBtn(btn);
        if (btn->getState())
            emit signalSelected(s);
        else
            emit signalUnselected(s);
    });
}

void WaterFallButtonGroup::addButton(QString s, QColor c, bool selected)
{
    WaterFloatButton* btn = new WaterFloatButton(s, this);
    btn->setFixedForeSize();
    setBtnColors(btn);
    btns.append(btn);

    if (selected)
        selectBtn(btn);

    btn->setAutoTextColor(false);
    btn->setTextColor(c);
    connect(btn, &InteractiveButtonBase::clicked, this, [=]{
        if (!selectable)
            return ;
        selectBtn(btn);
        if (btn->getState())
            emit signalSelected(s);
        else
            emit signalUnselected(s);
    });
}

void WaterFallButtonGroup::clearButtons()
{
    foreach (InteractiveButtonBase* btn, btns)
        btn->deleteLater();
    btns.clear();
}

void WaterFallButtonGroup::setSelecteable(bool en)
{
    this->selectable = en;
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

    foreach (InteractiveButtonBase* btn, btns)
    {
        btn->setTextColor(normal_ft);
        btn->setBgColor(normal_bg);
        btn->setBgColor(hover_bg, press_bg);
    }
}

void WaterFallButtonGroup::setMouseColor(QColor hover_bg, QColor press_bg)
{
    this->hover_bg = hover_bg;
    this->press_bg = press_bg;
    foreach (InteractiveButtonBase* btn, btns)
    {
        btn->setBgColor(hover_bg, press_bg);
    }
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

void WaterFallButtonGroup::selectBtn(InteractiveButtonBase *btn)
{
    btn->setState(!btn->getState());
    if (btn->getState()) // 选中
    {
        btn->setBgColor(selected_bg);
//        btn->setTextColor(selected_ft);
    }
    else
    {
        btn->setBgColor(normal_bg);
//        btn->setTextColor(normal_ft);
    }
}
