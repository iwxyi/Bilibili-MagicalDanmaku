#include "pointmenubutton.h"

PointMenuButton::PointMenuButton(QWidget *parent) : InteractiveButtonBase(parent)
{
    setUnifyGeomerey(true);
    radius = 1;
    setClickAniDuration(600);
}

void PointMenuButton::mousePressEvent(QMouseEvent *event)
{
    InteractiveButtonBase::slotClicked();
    return InteractiveButtonBase::mousePressEvent(event);
}

void PointMenuButton::paintEvent(QPaintEvent * event)
{
    InteractiveButtonBase::paintEvent(event);

    if (!show_foreground) return ; // 不显示前景

    int w = _w, h = _h;
    int l = _l+w/3, t = _t+h/3, r = w*2/3, b = h*2/3;
    int mx = _l+w/2+offset_pos.x(), my = _t+h/2+offset_pos.y();

    // 画笔
    QPainter painter(this);
    painter.setPen(QPen(icon_color));
    painter.setRenderHint(QPainter::Antialiasing,true);

    if (click_ani_appearing)
    {
        int midx = (l+r) / 2;
        int move_radius = (b-t)*3/4/2;
        QPainterPath path;

        // 第一个点
        if (click_ani_progress <= ANI_STEP_3) // 画圈
        {
            double tp = click_ani_progress / (double)ANI_STEP_3;
            QPoint o(midx, t + move_radius);
            QPoint p(o.x() - move_radius * sin(PI * tp), o.y() - move_radius * cos(PI * tp));
            path.addEllipse(p.x()-radius, p.y()-radius, radius<<1, radius<<1);
        }
        else if (click_ani_progress <= ANI_STEP_3*2) // 静止
        {
            QPoint o(midx, t + move_radius*2);
            path.addEllipse(o.x()-radius, o.y()-radius, radius<<1, radius<<1);
        }
        else // 下移
        {
            double tp = (click_ani_progress-ANI_STEP_3*2) / (100.0 - ANI_STEP_3*2);
            QPoint p(midx, b-(1-tp)*(b-t)/6);
            path.addEllipse(p.x()-radius, p.y()-radius, radius<<1, radius<<1);
        }

        // 第二个点
        if (click_ani_progress <= (100-ANI_STEP_3*2)) // 静止
        {
            QPoint o(midx, (t+b)/2);
            path.addEllipse(o.x()-radius, o.y()-radius, radius<<1, radius<<1);
        }
        else // 不动
        {
            QPoint o(midx, (t+b)/2);
            path.addEllipse(o.x()-radius, o.y()-radius, radius<<1, radius<<1);
        }

        // 第三个点
        if (click_ani_progress <= ANI_STEP_3) // 静止
        {
            QPoint o(midx, b);
            path.addEllipse(o.x()-radius, o.y()-radius, radius<<1, radius<<1);
        }
        else if (click_ani_progress > ANI_STEP_3 && click_ani_progress <= ANI_STEP_3*2) // 画圈
        {
            double tp = (click_ani_progress-ANI_STEP_3) / (double)ANI_STEP_3;
            QPoint o(midx, b - move_radius);
            QPoint p(o.x() + move_radius * sin(PI * tp), o.y() + move_radius * cos(PI * tp));
            path.addEllipse(p.x()-radius, p.y()-radius, radius<<1, radius<<1);
        }
        else // 上移
        {
            double tp = (click_ani_progress-ANI_STEP_3*2) / (100-ANI_STEP_3*2);
            QPoint p(midx, t+(1-tp)*(b-t)/6);
            path.addEllipse(p.x()-radius, p.y()-radius, radius<<1, radius<<1);
        }

        painter.fillPath(path, QColor(icon_color));
    }
    else
    {
        QPainterPath path;
        path.addEllipse((l+r)/2-radius, t-radius, radius<<1, radius<<1);
        path.addEllipse((l+r)/2-radius, (t+b)/2-radius, radius<<1, radius<<1);
        path.addEllipse((l+r)/2-radius, b-radius, radius<<1, radius<<1);
        painter.fillPath(path, QColor(icon_color));
    }

}
