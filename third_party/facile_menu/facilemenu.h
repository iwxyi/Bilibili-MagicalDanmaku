#ifndef FACILEMENU_H
#define FACILEMENU_H

#include <QObject>
#include <QDialog>
#include <functional>
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QApplication>
#include <QScreen>
#include <QMenu>
#include <QAction>
#include "facilemenuitem.h"

#define DEFAULT_MENU_BLUR_ALPHA 33

#define newFacileMenu FacileMenu *menu = (new FacileMenu(this)) // 加上括号是为了可以直接设置属性

typedef std::function<void(int index, bool state)> const FuncCheckType;

class FacileMenu : public QWidget
{
    Q_OBJECT
public:
    FacileMenu(QWidget *parent = nullptr);
    ~FacileMenu() override;

    FacileMenuItem* addAction(QIcon icon, QString text, FuncType clicked = nullptr);
    FacileMenuItem* addAction(QIcon icon, FuncType clicked = nullptr);
    FacileMenuItem* addAction(QString text, FuncType clicked = nullptr);
    FacileMenuItem* addAction(QAction* action, bool deleteWithMenu = false);
    template <class T>
    FacileMenuItem* addAction(QIcon icon, QString text, T *obj, void (T::*func)());
    FacileMenu* addNumberedActions(QString pattern, int numberStart, int numberEnd, FuncItemType config = nullptr, FuncIntType clicked = nullptr, int step = 0);
    FacileMenu* addNumberedActions(QString pattern, int numberStart, int numberEnd, FuncItemIntType config, FuncIntType clicked = nullptr, int step = 0);
    FacileMenu* addActions(QList<QAction*> actions);

    FacileMenu* addRow(FuncType addActions = []{});
    FacileMenu* beginRow();
    FacileMenu* endRow();
    QVBoxLayout* createNextColumn();
    QBoxLayout* currentLayout() const;
    FacileMenu* addTitle(QString text);

    FacileMenu* addMenu(QIcon icon, QString text, FuncType clicked = nullptr);
    FacileMenu* addMenu(QString text, FuncType clicked = nullptr);
    FacileMenu* addMenu(QMenu* menu);
    FacileMenuItem* parentAction();
    FacileMenuItem* lastAction();
    FacileMenuItem* currentAction();

    FacileMenu* addLayout(QLayout *layout, int stretch = 0);
    FacileMenu* addLayoutItem(QLayoutItem *item);
    FacileMenu* addSpacerItem(QSpacerItem *spacerItem);
    FacileMenu* addSpacing(int size);
    FacileMenu* addStretch(int stretch = 0);
    FacileMenu* addStrut(int size);
    FacileMenu* addWidget(QWidget *widget, int stretch = 0, Qt::Alignment alignment = Qt::Alignment());
    FacileMenu* setSpacing(int spacing);
    FacileMenu* setStretchFactor(QWidget *widget, int stretch);
    FacileMenu* setStretchFactor(QLayout *layout, int stretch);

    FacileMenuItem* addSeparator();
    FacileMenu* split();
    FacileMenuItem* lastAddedItem();
    bool hasFocus() const;

    int indexOf(FacileMenuItem* item);
    FacileMenuItem* at(int index);

    void exec(QPoint pos = QPoint(-1, -1));
    void exec(QRect expt, bool vertical = false, QPoint pos = QPoint(-1, -1));
    void execute();
    void toHide(int focusIndex = -1);
    void toClose();
    bool isClosedByClick() const;
    FacileMenu* finished(FuncType func);

    FacileMenu* addOptions(QList<QString>texts, QList<bool>states, FuncIntType clicked);
    FacileMenu* addOptions(QList<QString>texts, int select, FuncIntType clicked);
    FacileMenu* singleCheck(FacileMenuItem* item);
    FacileMenu* uncheckAll(FacileMenuItem* except = nullptr, int begin = -1, int end = -1);
    QList<FacileMenuItem*> checkedItems();
    QList<int> checkedIndexes();
    QStringList checkedItemTexts();
    QList<QVariant> checkedItemDatas();
    FacileMenu* setSingleCheck(FuncCheckType clicked = nullptr);
    FacileMenu* setMultiCheck(FuncCheckType clicked = nullptr);

    FacileMenu* setTipArea(int x = 48);
    FacileMenu* setTipArea(QString longestTip);
    FacileMenu* setSplitInRow(bool split = true);

    FacileMenu* setAppearAnimation(bool en);
    FacileMenu* setDisappearAnimation(bool en);
    FacileMenu* setSubMenuShowOnCursor(bool en);

signals:
    void signalActionTriggered(FacileMenuItem* action);
    void signalHidden(); // 只是隐藏了自己

private slots:
    void itemMouseEntered(FacileMenuItem* item);

protected:
    FacileMenu(bool sub, QWidget* parent = nullptr);
    FacileMenuItem* createMenuItem(QIcon icon, QString text);
    Qt::Key getShortcutByText(QString text) const;
    void setActionButton(InteractiveButtonBase* btn, bool isChip = false);
    void showSubMenu(FacileMenuItem* item);
    bool isCursorInArea(QPoint pos, FacileMenu* child = nullptr);
    void setKeyBoardUsed(bool use = true);
    bool isSubMenu() const;
    FacileMenuItem* addVSeparator();
    void startAnimationOnShowed();
    void startAnimationOnHidden(int focusIndex);

    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

public:
    static void setColors(QColor normal, QColor hover, QColor press, QColor text);
    static QColor normal_bg; // 普通背景（用作全局是为了方便设置）
    static QColor hover_bg;  // 悬浮背景
    static QColor press_bg;  // 按下背景
    static QColor text_fg;   // 字体/变色图标颜色
    static int blur_bg_alpha; // 背景图显示程度，0禁用，1~100为模糊透明度

private:
    QList<FacileMenuItem*> items;
    QList<FacileMenuItem*> v_separators, h_separators;
    QList<QWidget*> other_widgets; // 手动添加的widget
    QHBoxLayout* main_hlayout;
    QVBoxLayout* main_vlayout;
    QList<QHBoxLayout*> row_hlayouts;
    QList<QAction*> import_actions;
    QPixmap bg_pixmap;

    FacileMenu* current_sub_menu = nullptr; // 当前打开（不一定显示）的子菜单
    FacileMenu* parent_menu = nullptr; // 父对象的菜单
    FacileMenuItem* last_added_item = nullptr; // 最后添加的item
    FuncType* finished_func = nullptr;

    bool hidden_by_another = false; // 是否是被要显示的另一个子菜单替换了。若否，隐藏全部菜单
    const int item_padding = 8; // 每个item四周的空白
    const int tip_area_spacing = 8; // item正文和tip的间距
    bool adding_horizone = false; // 是否正在添加横向菜单
    bool align_mid_if_alone = false; // 是否居中对齐，如果只有icon或text
    bool linger_on_submenu_clicked = false; // 子菜单点击后，父菜单是否逐级隐藏(注意子菜单若有选择项需手动改变)
    bool _showing_animation = false;
    int current_index = -1; // 当前索引
    bool using_keyboard = false; // 是否正在使用键盘挑选菜单
    QRect window_rect;
    int window_height = 0; // 窗口高度，每次打开都更新一次
    QPoint _enter_later_pos = QPoint(-1, -1); // 避免连续两次触发 enterLater 事件
    bool closed_by_clicked = false; // 是否因为被单击了才隐藏，还是因为其他原因关闭

    // 可修改的配置属性
    int addin_tip_area = 48; // 右边用来显示提示文字的区域

    // 可修改的配置属性（传递给子菜单）
    bool split_in_row = false; // 同一行是否默认添加分割线
    bool enable_appear_animation = true;
    bool enable_disappear_animation = true;
    bool sub_menu_show_on_cursor = true; // 子菜单跟随鼠标出现还是在主菜单边缘
};

#endif // FACILEMENU_H
