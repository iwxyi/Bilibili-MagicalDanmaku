#include "winmaxbutton.h"

WinMaxButton::WinMaxButton(QWidget *parent)
    : InteractiveButtonBase (parent)
{
    setUnifyGeomerey(true);
}

void WinMaxButton::paintEvent(QPaintEvent *event)
{
    InteractiveButtonBase::paintEvent(event);

    if (!show_foreground) return ; // 不显示前景

    int w = _w, h = _h;
    int dx = offset_pos.x(), dy = offset_pos.y();
    QRect r;
    if (click_ani_appearing || click_ani_disappearing)
    {
        double pro = click_ani_progress / 800.0;
        r = QRect(
                    _l+(w/3+dx) - (w/3+dx)*pro,
                    _t+(h/3+dy) - (h/3+dy)*pro,
                    w/3 + (w*2/3)*pro,
                    h/3 + (h*2/3)*pro
                    );
    }
    else
    {
        r = QRect(_l+w/3+dx, _t+h/3+dy, w/3, h/3);
    }


    QPainter painter(this);
    painter.setPen(QPen(icon_color));
    painter.drawRect(r);
}
