#ifndef APPENDBUTTON_H
#define APPENDBUTTON_H

#include "interactivebuttonbase.h"

class AppendButton : public InteractiveButtonBase
{
public:
    AppendButton(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

public slots:
    virtual void anchorTimeOut() override;

private:
    int add_angle = 0;    // 旋转角度
    int rotate_speed = 2; // 旋转的速度
};

#endif // APPENDBUTTON_H
