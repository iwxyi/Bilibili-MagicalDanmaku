#include "winminbutton.h"

WinMinButton::WinMinButton(QWidget* parent)
	: InteractiveButtonBase(parent)
{
    setUnifyGeomerey(true);
}

void WinMinButton::paintEvent(QPaintEvent* event)
{
	InteractiveButtonBase::paintEvent(event);

    if (!show_foreground) return ; // 不显示前景

    int w = _w, h = _h;
    QPoint left(_l+w/3, _t+h/2), right(_l+w*2/3, _t+h/2),
           mid(_l+w/2+offset_pos.x(), _t+h/2+offset_pos.y());

    if (click_ani_appearing || click_ani_disappearing)
    {
        double pro = click_ani_progress / 800.0;
        left.setX(left.x()-left.x() * pro);
        right.setX(right.x()+(w-right.x()) * pro);
    }

    QPainter painter(this);
    QPainterPath path;
    path.moveTo(left);
    path.cubicTo(left, mid, right);
    painter.setPen(QPen(icon_color));
    if (left.y() != mid.y())
        painter.setRenderHint(QPainter::Antialiasing,true);
    painter.drawPath(path);
}
