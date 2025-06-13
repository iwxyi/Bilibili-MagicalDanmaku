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
    void setSelect(QString text);
    void setSelect(int index);
    WaterFloatButton *addButton(QString s, bool selected = false);
    WaterFloatButton *addButton(QString s, QColor c, bool selected = false);
    QStringList getSelectedTexts() const;
    QList<int> getSelectedIndexes() const;
    void clear();

    void setColors(QColor normal_bg, QColor hover_bg, QColor press_bg, QColor selected_bg, QColor normal_ft, QColor selected_ft = Qt::transparent);
    void setSelectedColor(QColor color);
    void updateBtnColors();

    int count() const;
    QList<InteractiveButtonBase*> getButtons() const;

    void setSelectable(bool enable);
    void setSingleSelect(bool enable);
    void updateButtonPositions();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QColor getReverseColor(QColor color);
    int getReverseChannel(int x);
    void setBtnColors(InteractiveButtonBase* btn);
    void changeBtnSelect(InteractiveButtonBase* btn);
    void selectBtn(InteractiveButtonBase* btn);
    void unselectBtn(InteractiveButtonBase* btn);

private slots:
    void slotButtonClicked(InteractiveButtonBase* btn);

signals:
    void signalSelectChanged();
    void signalClicked(QString s);
	void signalSelected(QString s);
    void signalUnselected(QString s);
    void signalRightClicked(QString s);

private:
    QList<InteractiveButtonBase*> btns;

    bool selectable = false; // 是否可选中
    bool single_select = false; // 是否单选
    QColor normal_bg, hover_bg, press_bg, selected_bg, normal_ft, selected_ft;

    bool _flag_has_change = false;
};

#endif // WATERFALLBUTTONGROUP_H
