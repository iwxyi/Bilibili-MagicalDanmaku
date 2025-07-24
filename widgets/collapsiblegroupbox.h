#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <QWidget>
#include <QLabel>
#include "interactivebuttonbase.h"

class CollapsibleGroupBox : public QWidget
{
public:
    CollapsibleGroupBox(QWidget *parent = nullptr);
    CollapsibleGroupBox(const QString &title, QWidget *parent = nullptr);

    void setTitle(const QString &title);
    QString getTitle() const;
    void setCollapsed(bool collapsed);
    bool isCollapsed() const;
    QWidget *getContentWidget() const;

private:
    QLabel *titleLabel;
    InteractiveButtonBase *collapseButton;
    QWidget* contentWidget;
    InteractiveButtonBase *stackExpandButton;
};

#endif // COLLAPSIBLEGROUPBOX_H
