#ifndef WATERCIRCLEBUTTON_H
#define WATERCIRCLEBUTTON_H

#include "interactivebuttonbase.h"

class WaterCircleButton : public InteractiveButtonBase
{
public:
    WaterCircleButton(QWidget* parent = nullptr);
    WaterCircleButton(QIcon icon, QWidget* parent = nullptr);
    WaterCircleButton(QPixmap pixmap, QWidget* parent = nullptr);

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    QPainterPath getBgPainterPath() override;
    QPainterPath getWaterPainterPath(Water water) override;

    void simulateStatePress(bool s = true);
    bool inArea(QPoint point) override;

protected:
    QPoint center_pos;
    bool in_circle;
    int radius;
};

#endif // WATERCIRCLEBUTTON_H
