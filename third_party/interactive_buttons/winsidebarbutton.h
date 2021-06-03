#ifndef WINSIDEBARBUTTON_H
#define WINSIDEBARBUTTON_H

#include <QObject>
#include <QWidget>
#include "interactivebuttonbase.h"

class WinSidebarButton : public InteractiveButtonBase
{
public:
    WinSidebarButton(QWidget *parent = nullptr);

    void setTopLeftRadius(int r);

protected:
    void paintEvent(QPaintEvent*event);
    void slotClicked();

    QPainterPath getBgPainterPath();
    QPainterPath getWaterPainterPath(Water water);

private:
    int tl_radius;
};

#endif // WINSIDEBARBUTTON_H
