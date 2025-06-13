#ifndef FACILEMENUBARINTERFACE_H
#define FACILEMENUBARINTERFACE_H

#include <QObject>

class FacileMenuBarInterface
{
public:
    virtual void trigger(int) = 0;
    virtual bool triggerIfNot(int, void*) = 0;
    virtual int isCursorInArea(QPoint) const = 0;
    virtual int currentIndex() const = 0;
};

#endif // FACILEMENUBARINTERFACE_H
