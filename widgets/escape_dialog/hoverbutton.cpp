#include "hoverbutton.h"

HoverButton::HoverButton(QWidget *parent) : QPushButton(parent), ban_enter(false)
{

}

HoverButton::HoverButton(QString text, QWidget *parent) : QPushButton(text, parent), ban_enter(false)
{

}

void HoverButton::banEnter(bool ban)
{
    ban_enter = ban;
}

void HoverButton::enterEvent(QEvent *event)
{
    emit signalEntered(mapFromGlobal(QCursor::pos()));
    return QPushButton::enterEvent(event);
}

void HoverButton::leaveEvent(QEvent *event)
{
    emit signalLeaved(mapFromGlobal(QCursor::pos()));
    return QPushButton::leaveEvent(event);
}

void HoverButton::mousePressEvent(QMouseEvent *event)
{
    emit signalMousePressed(mapFromGlobal(QCursor::pos()));
    return QPushButton::mousePressEvent(event);
}

void HoverButton::mouseReleaseEvent(QMouseEvent *event)
{
    emit signalMouseReleased(mapFromGlobal(QCursor::pos()));
    return QPushButton::mouseReleaseEvent(event);
}

void HoverButton::keyPressEvent(QKeyEvent *event)
{
    emit signalKeyPressed(event);
    if (ban_enter && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) // 是 Key_Return
        return ;
    return QPushButton::keyPressEvent(event);
}

void HoverButton::keyReleaseEvent(QKeyEvent *event)
{
    emit signalKeyReleased(event);
    if (ban_enter && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter))
        return ;
    return QPushButton::keyReleaseEvent(event);
}
