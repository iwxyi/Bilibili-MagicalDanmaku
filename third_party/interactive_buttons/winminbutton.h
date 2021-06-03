#ifndef WINMINBUTTON_H
#define WINMINBUTTON_H

#include "interactivebuttonbase.h"

class WinMinButton : public InteractiveButtonBase
{
	Q_OBJECT
public:
    WinMinButton(QWidget* parent = nullptr);

    void paintEvent(QPaintEvent* event) override;
};

#endif // WINMINBUTTON_H