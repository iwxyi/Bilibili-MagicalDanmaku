#ifndef CHECKBOX1_H
#define CHECKBOX1_H

#include "anicheckbox.h"

class CheckBox1 : public AniCheckBox
{
public:
    CheckBox1(QWidget* parent = nullptr);

    void setUncheckedColor(QColor c);

protected:
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;

    virtual void checkStateChanged(int state) override;

    virtual void drawBox(QPainter &painter, QRectF rect) override;

protected:
    QColor uncheckedColor = QColor("#88888888");
};

#endif // CHECKBOX1_H
