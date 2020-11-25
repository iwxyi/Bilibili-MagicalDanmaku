#include "numberanimation.h"

NumberAnimation::NumberAnimation(QWidget *parent) : NumberAnimation("", Qt::red, parent)
{
}

NumberAnimation::NumberAnimation(QString text, QColor color, QWidget *parent) : QLabel(parent)
{
    endProp = 0.5;
    fontSize = font().pointSize();
    setAlignment(Qt::AlignCenter);

    setText(text);
    setColor(color);

    show();
}

void NumberAnimation::setCenter(QPoint p)
{
    move(p.x() - width() / 2, p.y() - height());
}

void NumberAnimation::setText(QString text)
{
    QLabel::setText(text);
}

void NumberAnimation::setColor(QColor color)
{
    this->color = color;
    setAlpha(255);
}

void NumberAnimation::setFontSize(int f)
{
    fontSize = f;

    QFont font = this->font();
    font.setPointSize(f);
    font.setBold(true);
    setFont(font);
}

void NumberAnimation::setEndProp(double prop)
{
    endProp = prop;
}

int NumberAnimation::getFontSize()
{
    return fontSize;
}

void NumberAnimation::setAlpha(int a)
{
    this->alpha = a;
    QColor c = color;
    color.setAlpha(a);

    setStyleSheet("color:" + QVariant(c).toString());
}

int NumberAnimation::getAlpha()
{
    return alpha;
}

void NumberAnimation::startAnimation()
{
    int duration = 500;

    QPropertyAnimation *ani = new QPropertyAnimation(this, "fontSize");
    ani->setStartValue(fontSize);
    ani->setEndValue(fontSize * endProp);
    ani->setDuration(duration);
    ani->setEasingCurve(QEasingCurve::InCubic);
    connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
    ani->start();

    QPropertyAnimation *ani2 = new QPropertyAnimation(this, "pos");
    ani2->setStartValue(pos());
    ani2->setEndValue(QPoint(pos().x(), pos().y() - fontSize * 2));
    ani2->setDuration(duration);
    ani2->setEasingCurve(QEasingCurve::OutCubic);
    connect(ani2, SIGNAL(finished()), ani2, SLOT(deleteLater()));
    connect(ani2, SIGNAL(finished()), this, SLOT(deleteLater()));
    ani2->start();

    QPropertyAnimation *ani3 = new QPropertyAnimation(this, "alpha");
    ani3->setStartValue(255);
    ani3->setEndValue(0x0);
    ani3->setDuration(duration);
    ani3->setEasingCurve(QEasingCurve::InCubic);
    connect(ani3, SIGNAL(finished()), ani3, SLOT(deleteLater()));
    ani3->start();
}
