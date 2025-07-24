#ifndef CODELINESPLITTERWIDGET_H
#define CODELINESPLITTERWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include "interactivebuttonbase.h"
#include "codelinewidgetbase.h"

class CodeLineSplitterWidget : public CodeLineWidgetBase
{
    Q_OBJECT
public:
    CodeLineSplitterWidget(QWidget *parent = nullptr);

    void fromString(const QString &code) override;
    QString toString() const override;

private:
    QHBoxLayout *layout;
    QLabel *label;
    InteractiveButtonBase *closeBtn;
};

#endif // CODELINESPLITTERWIDGET_H
