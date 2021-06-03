#include "winsidebarbutton.h"

WinSidebarButton::WinSidebarButton(QWidget* parent)
    : InteractiveButtonBase (parent), tl_radius(0)
{
    setUnifyGeomerey(true);
}

void WinSidebarButton::paintEvent(QPaintEvent *event)
{
    InteractiveButtonBase::paintEvent(event);

    if (!show_foreground) return ; // 不显示前景

    int dx = offset_pos.x(), dy = offset_pos.y();
    int l = _l + _w/3+dx, t = _t + _h/3+dy, w = _w/3, h = _h/3;

    QPainter painter(this);
    painter.setPen(QPen(icon_color));
    painter.setRenderHint(QPainter::Antialiasing,true);

    if (click_ani_appearing)
    {
        double pro = click_ani_progress / 100.0;
        if (getState())
            pro = 1 - pro;
        painter.drawEllipse(l+w/2-w*pro/2, t+h/2-h*pro/2, w*pro, h*pro);

        if (getState())
            pro = getSpringBackProgress(click_ani_progress, 50) / 100.0;
        else
            pro = 1 - click_ani_progress / 100.0;

        l = _l + _w/3+dx;
        t = _t + _h/3+dy;
        w = _w / 3;
        h = _h / 3;

        l += w/2.0 - w*pro/2;
        t += h/2.0 - h*pro/2;
        w *= pro;
        h *= pro;

        QPainterPath path;
        path.addEllipse(l, t, w, h);
        painter.fillPath(path, icon_color);
    }
    else if (getState())
    {
        QPainterPath path;
        path.addEllipse(l, t, w, h);
        painter.fillPath(path, icon_color);
    }
    else
    {
        painter.drawEllipse(l, t, w, h);
    }
}

void WinSidebarButton::slotClicked()
{
    if (getState())
        setState(false);
    else
        setState(true);
    return InteractiveButtonBase::slotClicked();
}

/**
 * 针对圆角的设置
 */
void WinSidebarButton::setTopLeftRadius(int r)
{
    tl_radius = r;
}

QPainterPath WinSidebarButton::getBgPainterPath()
{
    if (!tl_radius)
        return InteractiveButtonBase::getBgPainterPath();

    QPainterPath path = InteractiveButtonBase::getBgPainterPath();
    QPainterPath round_path;
    round_path.addEllipse(0, 0, tl_radius * 2, tl_radius * 2);
    QPainterPath corner_path;
    corner_path.addRect(0, 0, tl_radius, tl_radius);
    corner_path -= round_path;
    path -= corner_path;
    return path;
}

QPainterPath WinSidebarButton::getWaterPainterPath(Water water)
{
    return InteractiveButtonBase::getWaterPainterPath(water) & WinSidebarButton::getBgPainterPath();
}