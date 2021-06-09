#include "listiteminterface.h"

ListItemInterface::ListItemInterface(QWidget *parent) : QWidget(parent)
{
    _bgLabel = new QLabel(this);
    _bgLabel->move(_cardMargin, _cardMargin);
    _bgLabel->setStyleSheet("background: white; border-radius: 5px;");

    vlayout = new QVBoxLayout(this);
    setLayout(vlayout);
    vlayout->setMargin(vlayout->margin() + _cardMargin);

    check = new QCheckBox("启用", this);
    btn = new InteractiveButtonBase("发送", this);

    hlayout = new QHBoxLayout;
    hlayout->addWidget(check);
    hlayout->addWidget(new QWidget(this));
    hlayout->addWidget(btn);
    hlayout->setStretch(1, 1);
    hlayout->setMargin(0);
    vlayout->addLayout(hlayout);

    btn->setBorderColor(Qt::black);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedForePos();
    check->setCursor(Qt::PointingHandCursor);
}

void ListItemInterface::setRow(int row)
{
    this->_row = row;
}

int ListItemInterface::getRow() const
{
    return _row;
}

void ListItemInterface::resizeEvent(QResizeEvent *event)
{
    _bgLabel->resize(event->size() - QSize(_cardMargin * 2, _cardMargin * 2));

    return QWidget::resizeEvent(event);
}
