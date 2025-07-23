#ifndef CODELINEWIDGETBASE_H
#define CODELINEWIDGETBASE_H

#include <QWidget>
#include "codeeditorinterface.h"

class CodeLineWidgetBase : public QWidget, public CodeEditorInterface
{
public:
    CodeLineWidgetBase(QWidget *parent = nullptr) : QWidget{parent} {}

    virtual void fromString(const QString &code) override = 0;
    virtual QString toString() const override = 0;
};

#endif // CODELINEWIDGETBASE_H
