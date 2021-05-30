#include "watercirclebutton.h"

WaterCircleButton::WaterCircleButton(QWidget* parent) : InteractiveButtonBase (parent), in_circle(false), radius(16)
{

}

WaterCircleButton::WaterCircleButton(QIcon icon, QWidget *parent) : InteractiveButtonBase (icon, parent), in_circle(false), radius(16)
{

}

WaterCircleButton::WaterCircleButton(QPixmap pixmap, QWidget *parent) : InteractiveButtonBase (pixmap, parent), in_circle(false), radius(16)
{

}

void WaterCircleButton::enterEvent(QEvent *event)
{

}

void WaterCircleButton::leaveEvent(QEvent *event)
{
    if (in_circle && !pressing && !inArea(mapFromGlobal(QCursor::pos())))
    {
        in_circle = false;
        InteractiveButtonBase::leaveEvent(event);
    }
}

void WaterCircleButton::mousePressEvent(QMouseEvent *event)
{
    if (in_circle || (!hovering && inArea(event->pos())))
        return InteractiveButtonBase::mousePressEvent(event);
}

void WaterCircleButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (pressing)
    {
        InteractiveButtonBase::mouseReleaseEvent(event);

        if (leave_after_clicked || (!inArea(event->pos()) && !pressing)) // 鼠标移出
        {
            in_circle = false;
            InteractiveButtonBase::leaveEvent(nullptr);
        }
    }
}

void WaterCircleButton::mouseMoveEvent(QMouseEvent *event)
{
    bool is_in = inArea(event->pos());

    if (is_in && !in_circle)// 鼠标移入
    {
        in_circle = true;
        InteractiveButtonBase::enterEvent(nullptr);
    }
    else if (!is_in && in_circle && !pressing) // 鼠标移出
    {
        in_circle = false;
        InteractiveButtonBase::leaveEvent(nullptr);
    }

    if (in_circle)
        InteractiveButtonBase::mouseMoveEvent(event);
}

void WaterCircleButton::resizeEvent(QResizeEvent *event)
{
    center_pos = geometry().center() - geometry().topLeft();
    radius = min(size().width(), size().height())/ 2;

    return InteractiveButtonBase::resizeEvent(event);
}

QPainterPath WaterCircleButton::getBgPainterPath()
{
    QPainterPath path;
    int w = size().width(), h = size().height();
    QRect rect(w/2-radius, h/2-radius, radius*2, radius*2);
    path.addEllipse(rect);
    return path;
}

QPainterPath WaterCircleButton::getWaterPainterPath(InteractiveButtonBase::Water water)
{
    QPainterPath path = InteractiveButtonBase::getWaterPainterPath(water) & getBgPainterPath();
    return path;
}

void WaterCircleButton::simulateStatePress(bool s)
{
    in_circle = true;
    InteractiveButtonBase::simulateStatePress(s);
    in_circle = false;
}

bool WaterCircleButton::inArea(QPoint point)
{
    return (point - center_pos).manhattanLength() <= radius;
}
