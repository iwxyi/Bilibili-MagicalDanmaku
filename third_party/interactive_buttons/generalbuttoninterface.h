#ifndef GENERALBUTTON_H
#define GENERALBUTTON_H

#include <QObject>
#include <QWidget>
#include <QPushButton>
//#include "defines.h"
//#include "globalvar.h"

class GeneralButtonInterface : public QPushButton
{
    Q_OBJECT
public:
    enum DirectType {
        DIRECT_NONE,
        DIRECT_TOP,
        DIRECT_LEFT,
        DIRECT_RIGHT,
        DIRECT_BOTTOM
    };

    GeneralButtonInterface();
    GeneralButtonInterface(QWidget* parent);
    GeneralButtonInterface(QString icon, QWidget* parent);
    GeneralButtonInterface(QIcon icon, QWidget* parent);
    GeneralButtonInterface(QIcon icon, QString text, QWidget* parent);
    GeneralButtonInterface(QString icon, QString text, QWidget* parent);

    void init();

    virtual void setStore(DirectType type);
    virtual void showFore(){}
    virtual void hideFore(){}
    virtual void showBack(){}
    virtual void hideBack(){}

    virtual void disableFixed(){ this->static_fixed=false; }
    virtual void setFixed(){this->static_fixed=true;}
    virtual bool isFixed(){return static_fixed;}

public slots:
    virtual void updateUI(){}


protected:
    bool static_fixed;
    DirectType store_direct;
};

#endif // GENERALBUTTON_H
