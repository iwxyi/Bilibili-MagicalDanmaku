#ifndef LISTITEMINTERFACE_H
#define LISTITEMINTERFACE_H

#include <QWidget>
#include <QApplication>
#include "myjson.h"

class ListItemInterface : public QWidget
{
    Q_OBJECT
public:
    explicit ListItemInterface(QWidget *parent = nullptr);

    virtual void fromJson(MyJson) = 0;
    virtual MyJson toJson() const = 0;

    void setRow(int row)
    {
        this->_row = row;
    }

    int getRow() const
    {
        return _row;
    }

signals:
    void signalResized();

public slots:
    virtual void autoResizeEdit()
    {}

private:
    int _row;
};

#endif // LISTITEMINTERFACE_H
