#ifndef CODELINESPLITTERWIDGET_H
#define CODELINESPLITTERWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "codelinewidgetbase.h"

class CodeLineSplitterWidget : public CodeLineWidgetBase
{
public:
    CodeLineSplitterWidget(QWidget *parent = nullptr);

    void fromString(const QString &code) override;
    QString toString() const override;

private:
    QVBoxLayout *layout;
    QLabel *label;
};

#endif // CODELINESPLITTERWIDGET_H
