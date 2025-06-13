#ifndef FACILEMENUBAR_H
#define FACILEMENUBAR_H

#include <QObject>
#include <QWidget>
#include "facilemenubarinterface.h"
#include "facilemenu.h"

class FacileSwitchWidget;

class FacileMenuBar : public QWidget, public FacileMenuBarInterface
{
    Q_OBJECT
public:
    explicit FacileMenuBar(QWidget *parent = nullptr);

    virtual int isCursorInArea(QPoint pos) const override;
    int currentIndex() const override;
    virtual bool triggerIfNot(int index, void*menu) override;

    void addMenu(QString name, FacileMenu* menu);
    void insertMenu(int index, QString name, FacileMenu* menu);
    void deleteMenu(int index);
    int count() const;
    void setAnimationEnabled(bool en); // 开启动画，目前会有些问题

signals:
    void triggered();

public slots:
    virtual void trigger(int index) override;
    virtual void switchTrigger(int index, int prevIndex);

private:
    InteractiveButtonBase* createButton(QString name, FacileMenu* menu);

private:
    QList<InteractiveButtonBase*> buttons;
    QList<FacileMenu*> menus;
    QHBoxLayout* hlayout;

    int _currentIndex = -1;
    bool enableAnimation = false;
    FacileSwitchWidget* aniWidget = nullptr;
};

#endif // FACILEMENUBAR_H
