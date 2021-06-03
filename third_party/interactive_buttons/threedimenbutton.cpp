#include "threedimenbutton.h"

ThreeDimenButton::ThreeDimenButton(QWidget* parent) : InteractiveButtonBase (parent)
{
    setMouseTracking(true);
	aop_w = width() / AOPER;
	aop_h = height() / AOPER;

    shadow_effect = new QGraphicsDropShadowEffect(this);
	shadow_effect->setOffset(0, 0);
	shadow_effect->setColor(QColor(0x88, 0x88, 0x88, 0x64));
	shadow_effect->setBlurRadius(10);
	setGraphicsEffect(shadow_effect);

	setJitterAni(false);
}

void ThreeDimenButton::enterEvent(QEvent *event)
{

}

void ThreeDimenButton::leaveEvent(QEvent *event)
{
    if (in_rect && !pressing && !inArea(mapFromGlobal(QCursor::pos())))
    {
        in_rect = false;
        InteractiveButtonBase::leaveEvent(nullptr);
    }

    // 不return，因为区域不一样
}

void ThreeDimenButton::mousePressEvent(QMouseEvent *event)
{
    // 因为上面可能有控件，所以可能无法监听到 enter 事件
    if (!in_rect && inArea(event->pos())) // 鼠标移入
    {
        in_rect = true;
        InteractiveButtonBase::enterEvent(nullptr);
    }

    if (in_rect)
        return InteractiveButtonBase::mousePressEvent(event);
}

void ThreeDimenButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (pressing)
    {
        InteractiveButtonBase::mouseReleaseEvent(event);

        if (leave_after_clicked || (!inArea(event->pos()) && !pressing)) // 鼠标移出
        {
            in_rect = false;
            InteractiveButtonBase::leaveEvent(nullptr);
        }
    }
}

void ThreeDimenButton::mouseMoveEvent(QMouseEvent *event)
{
    bool is_in = inArea(event->pos());

    if (is_in && !in_rect)// 鼠标移入
    {
        in_rect = true;
        InteractiveButtonBase::enterEvent(nullptr);
    }
    else if (!is_in && in_rect && !pressing) // 鼠标移出
    {
        in_rect = false;
        InteractiveButtonBase::leaveEvent(nullptr);
    }

    if (in_rect)
        InteractiveButtonBase::mouseMoveEvent(event);
}

void ThreeDimenButton::resizeEvent(QResizeEvent *event)
{
	aop_w = width() / AOPER;
	aop_h = height() / AOPER;
    return InteractiveButtonBase::resizeEvent(event);
}

void ThreeDimenButton::anchorTimeOut()
{
    // 因为上面有控件挡住了，所以需要定时监控move情况
    mouse_pos = mapFromGlobal(QCursor::pos());
    if (!pressing && !inArea(mouse_pos)) // 鼠标移出
    {
        InteractiveButtonBase::leaveEvent(nullptr);
    }

    InteractiveButtonBase::anchorTimeOut();

    // 修改阴影的位置
    if (offset_pos == QPoint(0,0))
        shadow_effect->setOffset(0, 0);
    else
    {
        if (offset_pos.manhattanLength() > SHADE)
        {
            double sx = -SHADE * offset_pos.x() / offset_pos.manhattanLength();
            double sy = -SHADE * offset_pos.y() / offset_pos.manhattanLength();
            shadow_effect->setOffset(sx*hover_progress/100, sy*hover_progress/100);
        }
        else
        {
            shadow_effect->setOffset(-offset_pos.x()*hover_progress/100, -offset_pos.y()*hover_progress/100);
        }
    }
}

QPainterPath ThreeDimenButton::getBgPainterPath()
{
	QPainterPath path;
	if (hover_progress) // 鼠标悬浮效果
	{
		/**
		 * 位置比例 = 悬浮比例 × 距离比例
		 * 坐标位置 ≈ 鼠标方向偏移
		 */
		double hp = hover_progress / 100.0;
		QPoint o(width()/2, height()/2);         // 中心点
        QPointF m = limitPointXY(mapFromGlobal(QCursor::pos())-o, width()/2, height()/2); // 当前鼠标的点
        QPointF f = limitPointXY(offset_pos, aop_w, aop_h);  // 偏移点（压力中心）

        QPointF lt, lb, rb, rt;
		// 左上角
		{
            QPointF p = QPoint(aop_w, aop_h) - o;
            double prob = dian_cheng(m, p) / dian_cheng(p, p);
			lt = o + (p) * (1-prob*hp/AOPER);
		}
		// 右上角
		{
            QPointF p = QPoint(width() - aop_w, aop_h) - o;
            double prob = dian_cheng(m, p) / dian_cheng(p, p);
			rt = o + (p) * (1-prob*hp/AOPER);
		}
		// 左下角
		{
            QPointF p = QPoint(aop_w, height() - aop_h) - o;
            double prob = dian_cheng(m, p) / dian_cheng(p, p);
			lb = o + (p) * (1-prob*hp/AOPER);
		}
		// 右下角
		{
            QPointF p = QPoint(width() - aop_w, height() - aop_h) - o;
            double prob = dian_cheng(m, p) / dian_cheng(p, p);
			rb = o + (p) * (1-prob*hp/AOPER);
		}

		path.moveTo(lt);
		path.lineTo(lb);
		path.lineTo(rb);
		path.lineTo(rt);
		path.lineTo(lt);
	}
	else
	{
		// 简单的path，提升性能用
		path.addRect(aop_w, aop_h, width()-aop_w*2, height()-aop_h*2);
	}

    return path;
}

QPainterPath ThreeDimenButton::getWaterPainterPath(InteractiveButtonBase::Water water)
{
    QRect circle(water.point.x() - water_radius*water.progress/100,
                water.point.y() - water_radius*water.progress/100,
                water_radius*water.progress/50,
                water_radius*water.progress/50);
    QPainterPath path;
    path.addEllipse(circle);
    return path & getBgPainterPath();
}

void ThreeDimenButton::simulateStatePress(bool s, bool a)
{
    in_rect = true;
    InteractiveButtonBase::simulateStatePress(s, a);
    in_rect = false;
}

bool ThreeDimenButton::inArea(QPointF point)
{
	return !(point.x() < aop_w
		|| point.x() > width()-aop_w
		|| point.y() < aop_h
		|| point.y() > height()-aop_h);
}

/**
 * 计算两个向量的叉积
 * 获取压力值
 */
double ThreeDimenButton::cha_cheng(QPointF a, QPointF b)
{
	return a.x() * b.y() - b.x()* a.y();
}

double ThreeDimenButton::dian_cheng(QPointF a, QPointF b)
{
	return a.x() * b.x() + a.y() * b.y();
}

QPointF ThreeDimenButton::limitPointXY(QPointF v, int w, int h)
{
	// 注意：成立时，v.x != 0，否则除零错误
	if (v.x() < -w)
	{
		v.setY(v.y()*-w/v.x());
		v.setX(-w);
	}

	if (v.x() > w)
	{
		v.setY(v.y()*w/v.x());
		v.setX(w);
	}

	if (v.y() < -h)
	{
		v.setX(v.x()*-h/v.y());
		v.setY(-h);
	}

	if (v.y() > h)
	{
		v.setX(v.x()*h/v.y());
		v.setY(h);
	}

	return v;
}
