#ifndef NUMBERANIMATION_H
#define NUMBERANIMATION_H

#include <QLabel>
#include <QPropertyAnimation>

class NumberAnimation : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(int fontSize READ getFontSize WRITE setFontSize)
    Q_PROPERTY(int alpha READ getAlpha WRITE setAlpha)
public:
    NumberAnimation(QWidget *parent = nullptr);
    NumberAnimation(QString text, QColor color = Qt::red, QWidget *parent = nullptr);

    void setCenter(QPoint p);
    void setText(QString text);
    void setColor(QColor color);
    void setEndProp(double prop);

    void setFontSize(int f);
    int getFontSize();

    void setAlpha(int a);
    int getAlpha();

    void startAnimation();

private:
    int fontSize; // 字体大小动画变量
    int alpha;
    QColor color;
    double endProp; // 最终大小比例
};

#endif // NUMBERANIMATION_H
