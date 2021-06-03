#include "winmenubutton.h"

WinMenuButton::WinMenuButton(QWidget* parent)
    : InteractiveButtonBase (parent)
{
    setUnifyGeomerey(true);
}

void WinMenuButton::paintEvent(QPaintEvent *event)
{
    InteractiveButtonBase::paintEvent(event);

    if (!show_foreground) return ; // 不显示前景

    double w = _w, h = _h;
    double dx = offset_pos.x(), dy = offset_pos.y();

    QPainter painter(this);
    painter.setPen(QPen(icon_color));

    if (click_ani_appearing)
    {
        double pro = click_ani_progress / 100.0;
        if (!getState())
            pro = 1 - pro;

        double l = _l+w/3,
               r = _l+w*2/3,
               t = _t+h/3 + pro*h/3;
        painter.drawLine(QPointF(l+dx/4,t+dy/4), QPointF(r+dx/4,t+dy/4));

        l = _l+w/3+pro*w/24;
        r = _l+w*2/3-pro*w/24;
        t = _t+w/2+pro*h*5/18;
        painter.drawLine(QPointF(l+dx/2,t+dy/2), QPointF(r+dx/2,t+dy/2));

        l = _l+w/3+pro*w/12;
        r = _l+w*2/3-pro*w/12;
        t = _t+w*2/3+pro*h*2/9;
        painter.drawLine(QPointF(l+dx,t+dy), QPointF(r+dx,t+dy));
    }
    else if (getState())
    {
        painter.drawLine(QLineF(_l+w/3+dx/4, _t+h*2/3+dy/4, w*2/3+dx/4,h*2/3+dy/4));
        painter.drawLine(QLineF(_l+w/3+w/24+dx/2, _t+h*7/9+dy/2, w*2/3-w/24+dx/2, h*7/9+dy/2));
        painter.drawLine(QLineF(_l+w/3+w/12+dx, _t+h*8/9+dy, w*2/3-w/12+dx, h*8/9+dy));
    }
    else
    {
        painter.drawLine(QPointF(_l+w/3+dx/4,_t+h/3+dy/4), QPointF(_l+w*2/3+dx/4,_t+h/3+dy/4));
        painter.drawLine(QPointF(_l+w/3+dx/2,_t+h/2+dy/2), QPointF(_l+w*2/3+dx/2,_t+h/2+dy/2));
        painter.drawLine(QPointF(_l+w/3+dx,_t+h*2/3+dy), QPointF(_l+w*2/3+dx,_t+h*2/3+dy));
    }
}

void WinMenuButton::slotClicked()
{
    setState(!getState());
    return InteractiveButtonBase::slotClicked();
}
