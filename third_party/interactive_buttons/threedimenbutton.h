#ifndef THREEDIMENBUTTON_H
#define THREEDIMENBUTTON_H

#include <QGraphicsDropShadowEffect>
#include "interactivebuttonbase.h"

class ThreeDimenButton : public InteractiveButtonBase
{
	#define AOPER 10
	#define SHADE 10
    Q_OBJECT
public:
    ThreeDimenButton(QWidget* parent = nullptr);

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    void anchorTimeOut() override;

	QPainterPath getBgPainterPath() override;
	QPainterPath getWaterPainterPath(InteractiveButtonBase::Water water) override;

    void simulateStatePress(bool s = true, bool a = false) override;
    bool inArea(QPointF point) override;

private:
    double cha_cheng(QPointF a, QPointF b);
    double dian_cheng(QPointF a, QPointF b);
    QPointF limitPointXY(QPointF v, int w, int h);

protected:
	QGraphicsDropShadowEffect* shadow_effect;
    bool in_rect;
    int aop_w, aop_h;
};

#endif // THREEDIMENBUTTON_H
