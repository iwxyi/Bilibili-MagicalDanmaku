#ifndef CUSTOMPAINTERWIDGET_H
#define CUSTOMPAINTERWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <functional>

typedef std::function<void(QRect canvas, QPainter* painter)> FuncCustomPaintType;

/**
 * 只显示一部分的控件
 */
class CustomPaintWidget : public QWidget
{
public:
    CustomPaintWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
    }

    void setPaint(FuncCustomPaintType func)
    {
        this->paintFunc = func;
    }

    void paintEvent(QPaintEvent *) override
    {
        if (!paintFunc)
            return ;
        QPainter painter(this);
        paintFunc(geometry(), &painter);
    }

private:
    FuncCustomPaintType paintFunc = nullptr;
};

#endif // CUSTOMPAINTERWIDGET_H
