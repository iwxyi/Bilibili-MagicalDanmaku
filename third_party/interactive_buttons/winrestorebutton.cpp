#include "winrestorebutton.h"

WinRestoreButton::WinRestoreButton(QWidget* parent)
    : InteractiveButtonBase(parent)
{
    setUnifyGeomerey(true);
}

void WinRestoreButton::paintEvent(QPaintEvent* event)
{
    InteractiveButtonBase::paintEvent(event);

    if (!show_foreground) return ; // 不显示前景

    // 画出现一角的矩形
    int w = _w, h = _h;
    int dx = offset_pos.x(), dy = offset_pos.y();
    QRect br;
    if (click_ani_appearing || click_ani_disappearing)
    {
        double pro = click_ani_progress / 800.0;
        br = QRect(
                    _l+(w/3+dx) - (w/3+dx)*pro,
                    _t+(h/3+dy) - (h/3+dy)*pro,
                    w/3 + (w*2/3)*pro,
                    h/3 + (h*2/3)*pro
                    );
    }
    else
    {
        br = QRect(_l+w/3+dx, _t+h/3+dy, w/3, h/3);
    }

    // 画原来的矩形
    QPainter painter(this);
    painter.setPen(QPen(icon_color));
    painter.drawRect(br);

    dx /= 2; dy /= 2;
    int l = _l+w*4/9+dx, t = _t+h*2/9+dy, r = _l+w*7/9+dx, b = _t+h*5/9+dy;
    if (click_ani_appearing || click_ani_disappearing)
    {
        double pro = click_ani_progress / 800.0;
        l -= l*pro;
        t -= t*pro;
        r += (w-r)*pro;
        b += (h-b)*pro;
    }
    QPoint topLeft(l, t), topRight(r, t), bottomLeft(l, b), bottomRight(r, b);
    QList<QPoint>points;

    /* 两个矩形一样大的，所以运行的时候，需要有三大类：
     * 1、完全重合（可以视为下一点任意之一）
     * 2、有一个点落在矩形内（4种情况）
     * 3、完全不重合
     * 根据3大类共6种进行判断
     */
    if (br.topLeft() == topLeft)
    {
        points << topLeft << topRight << bottomRight << bottomLeft << topLeft;
    }
    else if (br.contains(topLeft)) // 左上角在矩形内
    {
        points << QPoint(br.right()+1, t) << topRight << bottomRight << bottomLeft << QPoint(l, br.bottom()+1);
    }
    else if (br.contains(topRight)) // 右上角在矩形内
    {
        points << QPoint(r, br.bottom()+1) << bottomRight << bottomLeft << topLeft << QPoint(br.left(), t);
    }
    else if (br.contains(bottomLeft)) // 左下角在矩形内（默认）
    {
        points << QPoint(l, br.top()) << topLeft << topRight << bottomRight << QPoint(br.right()+1, b);
    }
    else if (br.contains(bottomRight)) // 右下角在矩形内
    {
        points << QPoint(br.left(), b) << bottomLeft << topLeft << topRight << QPoint(r, br.top());
    }
    else // 没有重合
    {
        points << topLeft << topRight << bottomRight << bottomLeft << topLeft;
    }

    if (points.size() > 1)
    {
        QPainterPath path;
        path.moveTo(points.at(0));
        for (int i = 1; i < points.size(); ++i)
            path.lineTo(points.at(i));
        QColor color(icon_color);
        color.setAlpha(color.alpha()*0.8);
        painter.setPen(QPen(color));
        painter.drawPath(path);
    }
}
