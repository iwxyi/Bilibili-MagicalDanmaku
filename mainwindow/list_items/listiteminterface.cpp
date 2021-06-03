#include "listiteminterface.h"

ListItemInterface::ListItemInterface(QWidget *parent) : QWidget(parent)
{
    _bgLabel = new QLabel(this);
    _bgLabel->move(_cardMargin, _cardMargin);
    _bgLabel->setStyleSheet("background: white; border-radius: 5px;");

    vlayout = new QVBoxLayout(this);
    setLayout(vlayout);
    vlayout->setMargin(vlayout->margin() + _cardMargin);
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
