#include "waterzoombutton.h"

WaterZoomButton::WaterZoomButton(QString text, QWidget *parent) : InteractiveButtonBase(text, parent)
{
    choking = DEFAULT_CHOKING;
    radius_zoom = -1;
    choking_prop = 0;
}

WaterZoomButton::WaterZoomButton(QWidget *parent) : InteractiveButtonBase(parent)
{
    choking = DEFAULT_CHOKING;
    radius_zoom = -1;
    choking_prop = 0;
}

void WaterZoomButton::setChoking(int c)
{
    choking = c;
}

int WaterZoomButton::getChokingSpacing()
{
    return choking * 2;
}

int WaterZoomButton::getDefaultSpacing()
{
    return DEFAULT_CHOKING * 2;
}

void WaterZoomButton::setChokingProp(double p)
{
    choking = min(width(), height()) * p;
    choking_prop = p;
}

void WaterZoomButton::setRadiusZoom(int radius)
{
    radius_zoom = radius;
}

void WaterZoomButton::setRadius(int x, int x2)
{
    // 注意：最终绘制中只计算 x 的半径，无视 y 的半径
    InteractiveButtonBase::setRadius(x);
    radius_zoom = x2;
}

QPainterPath WaterZoomButton::getBgPainterPath()
{
    QPainterPath path;
    int c;
    int r;
    if (!hover_progress)
    {
        c = choking;
        r = radius_x;
    }
    else
    {
        c = choking * (1 - getNolinearProg(hover_progress, hovering?FastSlower:SlowFaster));
        r = radius_zoom < 0 ? radius_x :
                              radius_x + (radius_zoom-radius_x) * hover_progress / 100;
    }

    if (r)
        path.addRoundedRect(QRect(c,c,size().width()-c*2,size().height()-c*2), r, r);
    else
        path.addRect(QRect(c,c,size().width()-c*2,size().height()-c*2));
    return path;
}

void WaterZoomButton::resizeEvent(QResizeEvent *event)
{
    InteractiveButtonBase::resizeEvent(event);

    if (qAbs(choking_prop)>0.0001)
    {
        choking = min(width(), height()) * choking_prop;
    }
}
