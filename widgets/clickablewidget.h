#ifndef CLICKABLEWIDGET_H
#define CLICKABLEWIDGET_H

#include <QLabel>
#include <QMouseEvent>
#include <QApplication>

class ClickableWidget : public QWidget
{
    Q_OBJECT
public:
    ClickableWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setObjectName("ClickableWidget");
    }

protected:
    void mousePressEvent(QMouseEvent *ev) override
    {
        if (ev->button() == Qt::LeftButton)
        {
            pressPos = ev->pos();
        }

        return QWidget::mousePressEvent(ev);
    }

    void mouseReleaseEvent(QMouseEvent *ev) override
    {
        if (ev->button() == Qt::LeftButton)
        {
            if ((pressPos - ev->pos()).manhattanLength() < QApplication::startDragDistance())
            {
                emit clicked();
                ev->ignore();
                return ;
            }
        }

        return QWidget::mouseReleaseEvent(ev);
    }

signals:
    void clicked();

private:
    QPoint pressPos;
};

#endif // CLICKABLEWIDGET_H
