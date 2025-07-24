#ifndef CODELINEWIDGETBASE_H
#define CODELINEWIDGETBASE_H

#include <QWidget>
#include "codeeditorinterface.h"

class CodeLineWidgetBase : public QWidget, public CodeEditorInterface
{
public:
    CodeLineWidgetBase(QWidget *parent = nullptr) : QWidget{parent}
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("CodeLineWidgetBase, CodeLineEditor, CodeLineCommentEditor, CodeLineSplitterWidget\
                     { background-color: #ffffff; border: 1px solid #e0e0e0; border-radius: 5px; }");
    }

    virtual void fromString(const QString &code) override = 0;
    virtual QString toString() const override = 0;
};

#endif // CODELINEWIDGETBASE_H
