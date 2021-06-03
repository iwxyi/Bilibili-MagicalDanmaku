#include "appendbutton.h"

AppendButton::AppendButton(QWidget *parent) : InteractiveButtonBase(parent)
{
    setUnifyGeomerey(true);
}

void AppendButton::paintEvent(QPaintEvent *event)
{
    InteractiveButtonBase::paintEvent(event);

    if (!show_foreground) return ; // 不显示前景

    double w = _w, h = _h;
    double l = _l+w/4+offset_pos.y(),
           t = _t+h/4-offset_pos.x(),
           r = w*3/4+offset_pos.y(),
           b = h*3/4-offset_pos.x();
    double mx = _l+w/2, my = _t+h/2;

    if (click_ani_appearing || click_ani_disappearing)
    {
        double pro = click_ani_progress / 100.0;
        l -= l * pro;
        t -= t * pro;
        r += (w-r) * pro;
        b += (h-b) * pro;
    }

    QPainter painter(this);
    painter.setPen(icon_color);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.translate(mx, my);
    painter.rotate((90+add_angle) * hover_progress / 100);
    painter.translate(-mx, -my);

    painter.drawLine(QPointF(l, (t+b)/2), QPointF(r, (t+b)/2));
    painter.drawLine(QPointF((l+r)/2, t), QPointF((l+r)/2, b));
}

void AppendButton::enterEvent(QEvent *event)
{
    add_angle = 0;
    rotate_speed = 2;
    InteractiveButtonBase::enterEvent(event);
}

void AppendButton::mouseReleaseEvent(QMouseEvent *event)
{
    rotate_speed = 2;
    InteractiveButtonBase::mouseReleaseEvent(event);
}

void AppendButton::anchorTimeOut()
{
    if (press_progress)
    {
        add_angle += rotate_speed;
        if (add_angle > 360)
        {
            add_angle -= 360;
            if (rotate_speed < 30) // 动态旋转速度
            {
                rotate_speed++;
            }
        }
    }

    InteractiveButtonBase::anchorTimeOut();
}
