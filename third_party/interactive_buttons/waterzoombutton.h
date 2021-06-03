#ifndef WATERZOOMBUTTON_H
#define WATERZOOMBUTTON_H

#include "interactivebuttonbase.h"

const int DEFAULT_CHOKING = 5;

class WaterZoomButton : public InteractiveButtonBase
{
public:
    WaterZoomButton(QString text, QWidget* parent = nullptr);
    WaterZoomButton(QWidget* parent = nullptr);

    void setChoking(int c);
    void setChokingProp(double p);
    void setRadiusZoom(int radius);
    void setRadius(int x, int x2);

    int getChokingSpacing();
    static int getDefaultSpacing();

protected:
    QPainterPath getBgPainterPath() override;
    void resizeEvent(QResizeEvent *event) override;

protected:
    int choking;
    double choking_prop;
    int radius_zoom;
};

#endif // WATERZOOMBUTTON_H
