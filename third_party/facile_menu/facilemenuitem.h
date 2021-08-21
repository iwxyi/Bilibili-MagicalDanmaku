#ifndef FACILEMENUITEM_H
#define FACILEMENUITEM_H

#include "interactivebuttonbase.h"

class FacileMenu;
class FacileMenuItem;

typedef std::function<void()> const FuncType;
typedef std::function<void(int)> const FuncIntType;
typedef std::function<QString(QString)> const FuncStringStringType;
typedef std::function<void(FacileMenuItem*)> const FuncItemType;
typedef std::function<void(FacileMenuItem*, int)> const FuncItemIntType;

class FacileMenuItem : public InteractiveButtonBase
{
public:
    FacileMenuItem(QWidget* parent = nullptr);
    FacileMenuItem(QString t, QWidget* parent = nullptr);
    FacileMenuItem(QIcon i, QWidget* parent = nullptr);
    FacileMenuItem(QIcon i, QString t, QWidget* parent = nullptr);
    FacileMenuItem(QPixmap p, QString t, QWidget* parent = nullptr);

    FacileMenuItem* setEnabled(bool e);
    FacileMenuItem* setCheckable(bool c);
    bool isCheckable() const;
    FacileMenuItem* setChecked(bool c);
    bool isChecked();
    FacileMenuItem* setKey(Qt::Key key);
    bool isKey(Qt::Key key) const;
    FacileMenuItem* setSubMenu(FacileMenu* menu);
    bool isSubMenu() const;
    bool isLinger() const;
    FacileMenuItem* setData(QVariant data);
    QVariant getData();

    FacileMenuItem* tip(QString sc);
    FacileMenuItem* tip(bool exp, QString sc);
    FacileMenuItem* tooltip(QString tt);
    FacileMenuItem* tooltip(bool exp, QString tt);
    FacileMenuItem* triggered(FuncType func);
    FacileMenuItem* triggered(bool exp, FuncType func);
    FacileMenuItem* disable(bool exp = true); // 满足情况下触发，不满足不变，下同
    FacileMenuItem* enable(bool exp = true);
    FacileMenuItem* hide(bool exp = true);
    FacileMenuItem* visible(bool exp = true);
    FacileMenuItem* check(bool exp = true);
    FacileMenuItem* uncheck(bool exp = true);
    FacileMenuItem* toggle(bool exp = true);
    FacileMenuItem* autoToggle();
    FacileMenuItem* text(bool exp, QString str);
    FacileMenuItem* text(bool exp, QString tru, QString fal);
    FacileMenuItem* fgColor(QColor color);
    FacileMenuItem* fgColor(bool exp, QColor color);
    FacileMenuItem* bgColor(QColor color);
    FacileMenuItem* bgColor(bool exp, QColor color);
    FacileMenuItem* prefix(bool exp, QString pfix);
    FacileMenuItem* suffix(bool exp, QString sfix, bool inLeftParenthesis = true);
    FacileMenuItem* prefix(QString pfix);
    FacileMenuItem* suffix(QString sfix, bool inLeftParenthesis = true);
    FacileMenuItem* icon(bool exp, QIcon icon);
    FacileMenuItem* borderR(int radius = 3, QColor co = Qt::transparent);
    FacileMenuItem* linger();
    FacileMenuItem* lingerText(QString textAfterClick);
    FacileMenuItem* bind(bool &val);
    FacileMenuItem* longPress(FuncType func);
    FacileMenuItem* textAfterClick(QString newText);
    FacileMenuItem* textAfterClick(FuncStringStringType func);

    FacileMenuItem* ifer(bool exp);
    FacileMenuItem* elifer(bool exp);
    FacileMenuItem* elser();
    FacileMenuItem* exiter(bool exp = true);
    FacileMenuItem* ifer(bool exp, FuncItemType func, FuncItemType elseFunc = nullptr);
    FacileMenuItem* switcher(int value);
    FacileMenuItem* caser(int value, FuncType func);
    FacileMenuItem* caser(int value);
    FacileMenuItem* breaker();
    FacileMenuItem* defaulter();

    FacileMenu* subMenu();

protected:
    void paintEvent(QPaintEvent *event) override;
    void drawIconBeforeText(QPainter &painter, QRect icon_rect) override;

    FacileMenuItem* createTempItem(bool thisIsParent = true);

private:
    Qt::Key key;
    bool checkable = false;
    bool trigger_linger = false; // 点击后是否保存菜单
    FacileMenu* sub_menu = nullptr;
    QString shortcut_tip = ""; // 快捷键提示
    FacileMenuItem* parent_menu_item_in_if = nullptr; // elser/caser专用
    int switch_value = 0; // switcher的值，用来和caser比较（不需要breaker……）
    bool switch_matched = true;
    QVariant data;
};

#endif // FACILEMENUITEM_H
