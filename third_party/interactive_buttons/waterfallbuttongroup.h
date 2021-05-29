#ifndef WATERFALLBUTTONGROUP_H
#define WATERFALLBUTTONGROUP_H

#include "waterfloatbutton.h"

class WaterFallButtonGroup : public QWidget
{
    Q_OBJECT
public:
    WaterFallButtonGroup(QWidget* parent = nullptr);

    void initStringList(QStringList list, QStringList selected = QStringList());
    void setSelects(QStringList list);
    void addButton(QString s, bool selected = false);
    void addButton(QString s, QColor c, bool selected = false);
    void clearButtons();

    void setSelecteable(bool en);
    void setColors(QColor normal_bg, QColor hover_bg, QColor press_bg, QColor selected_bg, QColor normal_ft, QColor selected_ft = Qt::transparent);
    void setMouseColor(QColor hover_bg, QColor press_bg);

    void updateButtonPositions();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QColor getReverseColor(QColor color);
    int getReverseChannel(int x);
    void setBtnColors(InteractiveButtonBase* btn);
    void selectBtn(InteractiveButtonBase* btn);

signals:
	void signalSelected(QString s);
    void signalUnselected(QString s);

private:
    QList<InteractiveButtonBase*>btns;
    bool selectable = true;
    QColor normal_bg, hover_bg, press_bg, selected_bg, normal_ft, selected_ft;
};

#endif // WATERFALLBUTTONGROUP_H
