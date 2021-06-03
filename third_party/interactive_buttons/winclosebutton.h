#ifndef WINCLOSEBUTTON_H
#define WINCLOSEBUTTON_H

#include "interactivebuttonbase.h"

#include <QObject>

class WinCloseButton : public InteractiveButtonBase
{
public:
    WinCloseButton(QWidget* parent = nullptr);

    void setTopRightRadius(int r);

protected:
    void paintEvent(QPaintEvent*event);

    QPainterPath getBgPainterPath();
    QPainterPath getWaterPainterPath(Water water);

private:
	int tr_radius;
};

#endif // WINCLOSEBUTTON_H
