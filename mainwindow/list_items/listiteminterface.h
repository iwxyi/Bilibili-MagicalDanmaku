#ifndef LISTITEMINTERFACE_H
#define LISTITEMINTERFACE_H

#include <QWidget>
#include <QApplication>
#include <QResizeEvent>
#include <QLabel>
#include <QVBoxLayout>
#include "myjson.h"

class ListItemInterface : public QWidget
{
    Q_OBJECT
public:
    explicit ListItemInterface(QWidget *parent = nullptr);

    virtual void fromJson(MyJson) = 0;
    virtual MyJson toJson() const = 0;

    virtual bool isEnabled() const
    {
        return false;
    }

    virtual QString title() const
    {
        return "";
    }

    virtual QString body() const
    {
        return "";
    }

    void setRow(int row);

    int getRow() const;

signals:
    void signalResized();

public slots:
    virtual void autoResizeEdit()
    {}

protected:
    void resizeEvent(QResizeEvent *event) override;

protected:
    QLabel* _bgLabel;
    QVBoxLayout* vlayout;

private:
    int _row;
    int _cardMargin = 9;
};

#endif // LISTITEMINTERFACE_H
