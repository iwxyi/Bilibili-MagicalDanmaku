#include "generalbuttoninterface.h"

GeneralButtonInterface::GeneralButtonInterface() : QPushButton ()
{
    init();
}

GeneralButtonInterface::GeneralButtonInterface(QWidget *parent) : QPushButton (parent)
{
    init();
}

GeneralButtonInterface::GeneralButtonInterface(QString icon, QWidget *parent) : QPushButton (QIcon(icon), "", parent)
{
    init();
}

GeneralButtonInterface::GeneralButtonInterface(QIcon icon, QWidget *parent) : QPushButton (icon, "", parent)
{
    init();
}

GeneralButtonInterface::GeneralButtonInterface(QIcon icon, QString text, QWidget *parent) : QPushButton (icon, text, parent)
{
    init();
}

GeneralButtonInterface::GeneralButtonInterface(QString icon, QString text, QWidget *parent) : QPushButton (QIcon(icon), text, parent)
{
    init();
}

void GeneralButtonInterface::init()
{
    static_fixed = true;
    store_direct = DIRECT_NONE;
//    connect(thm, SIGNAL(windowChanged()), this, SLOT(updateUI()));
}

void GeneralButtonInterface::setStore(DirectType type)
{
    this->store_direct = type;
}
