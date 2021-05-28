#include "waterfloatbutton.h"

WaterFloatButton::WaterFloatButton(QWidget *parent) : InteractiveButtonBase(parent),
        in_area(false), mwidth(16), radius(8)
{
    fore_enabled = false;
    fore_paddings.left = fore_paddings.right = radius;
}

WaterFloatButton::WaterFloatButton(QString s, QWidget *parent) : InteractiveButtonBase(s, parent),
        in_area(false), mwidth(16), radius(8)
{
    fore_enabled = false;
    fore_paddings.left = fore_paddings.right = radius;
}

void WaterFloatButton::enterEvent(QEvent *event)
{

}

void WaterFloatButton::leaveEvent(QEvent *event)
{
    if (in_area && !pressing && !inArea(mapFromGlobal(QCursor::pos())))
    {
        in_area = false;
        InteractiveButtonBase::leaveEvent(event);
    }
}

void WaterFloatButton::mousePressEvent(QMouseEvent *event)
{
    if (in_area)
        return InteractiveButtonBase::mousePressEvent(event);
}

void WaterFloatButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (pressing)
    {
        InteractiveButtonBase::mouseReleaseEvent(event);

        if (!pressing && !inArea(event->pos()))
        {
            in_area = false;
            InteractiveButtonBase::leaveEvent(event);
        }
    }
}

void WaterFloatButton::mouseMoveEvent(QMouseEvent *event)
{
    bool is_in = inArea(event->pos());

    if (!in_area && is_in) // 鼠标移入
    {
        in_area = true;
        InteractiveButtonBase::enterEvent(nullptr);
    }
    else if (in_area && !is_in && !pressing) // 鼠标移出
    {
        in_area = false;
        InteractiveButtonBase::leaveEvent(nullptr);
    }

    if (in_area)
        InteractiveButtonBase::mouseMoveEvent(event);
}

void WaterFloatButton::resizeEvent(QResizeEvent *event)
{
    int w = geometry().width(), h = geometry().height();
    if (h >= w * 4) // 宽度为准
        radius = w / 4;
    else
        radius = h/2;
    mwidth = (w-radius*2);

    return InteractiveButtonBase::resizeEvent(event);
}

void WaterFloatButton::paintEvent(QPaintEvent *event)
{
    InteractiveButtonBase::paintEvent(event);

    QPainter painter(this);
//    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing,true);

    // 鼠标悬浮进度
    QColor edge_color = hover_bg;
    int pro = 0;
    if (hover_progress > 0 || press_progress || waters.size())
    {
        if (water_animation)
        {
            /** 不用判断 water 是出现还是消失状态
              * 如果一直悬浮的话，颜色不会变
              * 而如果是点一下立马移开，文字会出现一种“渐隐渐现”的效果
              */
            if (waters.size())
                pro = max(hover_progress, waters.last().progress);
            else
                pro = hover_progress;
        }
        else
        {
            max(hover_progress, press_progress);
        }
        edge_color.setAlpha(255 * (100 - pro) / 100);
    }

    // 画边框
    QPainterPath path;
    if (show_foreground)
    {
        path = getBgPainterPath(); // 整体背景

        // 出现动画
        if (show_ani_appearing && show_ani_progress != 100 && border_bg.alpha() != 0)
        {
            int pw = size().width() * show_ani_progress / 100;
            QRect rect(0, 0, pw, size().height());
            QPainterPath rect_path;
            rect_path.addRect(rect);
            path &= rect_path;

            int x = show_ani_point.x(), y = show_ani_point.y();
            int gen = quick_sqrt(x*x + y*y);
            x = - water_radius * x / gen; // 动画起始中心点横坐标 反向
            y = - water_radius * y / gen; // 动画起始中心点纵坐标 反向
        }
        if (border_bg.alpha() != 0) // 如果有背景，则不进行画背景线条
        {
            painter.setPen(border_bg);
            painter.drawPath(path);
        }
    }

    // 画文字
    if ((self_enabled || fore_enabled) && !text.isEmpty())
    {
        QRect rect = QRect(QPoint(0,0), size());
        QColor color;
        if (pro)
        {
            if (auto_text_color)
            {
                QColor aim_color = isLightColor(hover_bg) ? QColor(0, 0, 0) : QColor(255, 255, 255);
                color = QColor(
                    text_color.red() + (aim_color.red() - text_color.red()) * pro / 100,
                    text_color.green() + (aim_color.green() - text_color.green()) * pro / 100,
                    text_color.blue() + (aim_color.blue() - text_color.blue()) * pro / 100,
                    255);
            }
            painter.setPen(color);
        }
        else
        {
            color = text_color;
            color.setAlpha(255);
        }
        painter.setPen(color);
        if (font_size > 0)
        {
            QFont font = painter.font();
            font.setPointSize(font_size);
            painter.setFont(font);
        }
        painter.drawText(rect, Qt::AlignCenter, text);
    }
}

QPainterPath WaterFloatButton::getBgPainterPath()
{
    QPainterPath path1, path2, path3;
    int w = size().width(), h = size().height();

    QRect mrect(w/2-mwidth/2, h/2-radius, mwidth, radius*2);
    path1.addRect(mrect);

    QPoint o1(w/2-mwidth/2, h/2);
    QPoint o2(w/2+mwidth/2, h/2);
    path2.addEllipse(o1.x()-radius, o1.y()-radius, radius*2, radius*2);
    path3.addEllipse(o2.x()-radius, o2.y()-radius, radius*2, radius*2);

    return path1 | path2 | path3;
}

QPainterPath WaterFloatButton::getWaterPainterPath(InteractiveButtonBase::Water water)
{
    QPainterPath path = InteractiveButtonBase::getWaterPainterPath(water) & getBgPainterPath();
    return path;
}

bool WaterFloatButton::inArea(QPoint point)
{
    int w = size().width(), h = size().height();
    QPoint o1(w/2-mwidth/2, h/2);
    QPoint o2(w/2+mwidth/2, h/2);
    QRect mrect(w/2-mwidth/2, h/2-radius, mwidth, radius*2);

    if (mrect.contains(point))
        return true;
    if ((point-o1).manhattanLength() <= radius ||
            (point-o2).manhattanLength() <= radius)
        return true;
    return false;
}
