#include "winclosebutton.h"

WinCloseButton::WinCloseButton(QWidget *parent)
    : InteractiveButtonBase (parent), tr_radius(0)
{
    setUnifyGeomerey(true);
}

void WinCloseButton::paintEvent(QPaintEvent *event)
{
    InteractiveButtonBase::paintEvent(event);

    if (!show_foreground) return ; // 不显示前景

    int w = _w, h = _h;
    int l = _l+w/3, t = _t+h/3, r = w*2/3, b = h*2/3;
    int mx = _l+w/2+offset_pos.x(), my = _t+h/2+offset_pos.y();

    if (click_ani_appearing || click_ani_disappearing)
    {
        double pro = click_ani_progress / 100.0;
        l -= l * pro;
        t -= t * pro;
        r += (w-r) * pro;
        b += (h-b) * pro;
    }

    QPainter painter(this);
    painter.setPen(QPen(icon_color));
    painter.setRenderHint(QPainter::Antialiasing,true);
    if (offset_pos == QPoint(0,0))
    {
        painter.drawLine(QPoint(l,t), QPoint(r,b));
        painter.drawLine(QPoint(r,t), QPoint(l,b));
    }
    else
    {
        QPainterPath path;
        path.moveTo(QPoint(l,t));
        path.cubicTo(QPoint(l,t), QPoint(mx,my), QPoint(r,b));
        path.moveTo(QPoint(r,t));
        path.cubicTo(QPoint(r,t), QPoint(mx,my), QPoint(l,b));

        painter.drawPath(path);
    }
}

/**
 * 针对圆角的设置
 */
void WinCloseButton::setTopRightRadius(int r)
{
    tr_radius = r;
}

QPainterPath WinCloseButton::getBgPainterPath()
{
    if (!tr_radius)
        return InteractiveButtonBase::getBgPainterPath();

    QPainterPath path = InteractiveButtonBase::getBgPainterPath();
    QPainterPath round_path;
    round_path.addEllipse(width() - tr_radius - tr_radius, 0, tr_radius*2, tr_radius*2);
    QPainterPath corner_path;
    corner_path.addRect(width() - tr_radius, 0, tr_radius, tr_radius);
    corner_path -= round_path;
    path -= corner_path;
    return path;
}

QPainterPath WinCloseButton::getWaterPainterPath(Water water)
{
    return InteractiveButtonBase::getWaterPainterPath(water) & WinCloseButton::getBgPainterPath();
}
