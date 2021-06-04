#ifndef CLICKABKELABEL_H
#define CLICKABKELABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QApplication>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    ClickableLabel(QWidget* parent = nullptr) : QLabel(parent)
    {
        setCursor(Qt::PointingHandCursor);
    }

protected:
    void mousePressEvent(QMouseEvent *ev) override
    {
        if (ev->button() == Qt::LeftButton)
        {
            pressPos = ev->pos();
        }

        return QLabel::mousePressEvent(ev);
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

        return QLabel::mouseReleaseEvent(ev);
    }

signals:
    void clicked();

private:
    QPoint pressPos;
};

#endif // CLICKABKELABEL_H
