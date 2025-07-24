#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <QWidget>
#include <QLabel>
#include "interactivebuttonbase.h"

class CollapsibleGroupBox : public QWidget
{
    Q_OBJECT
public:
    CollapsibleGroupBox(QWidget *parent = nullptr);
    CollapsibleGroupBox(const QString &title, QWidget *parent = nullptr);

    void setTitle(const QString &title);
    QString getTitle() const;
    void setCollapsed(bool collapsed);
    bool isCollapsed() const;
    void setCloseButtonVisible(bool visible);
    bool isCloseButtonVisible() const;
    QWidget *getContentWidget() const;

signals:
    void closeClicked();

private:
    QLabel *titleLabel;
    InteractiveButtonBase *collapseButton;
    InteractiveButtonBase *closeButton;
    QWidget* contentWidget;
};

#endif // COLLAPSIBLEGROUPBOX_H
