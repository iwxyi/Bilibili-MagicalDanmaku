#ifndef CODELINECOMMENTEDITOR_H
#define CODELINECOMMENTEDITOR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include "codelinewidgetbase.h"

class CodeLineCommentEditor : public CodeLineWidgetBase
{
public:
    CodeLineCommentEditor(QWidget *parent = nullptr);

    void fromString(const QString &code) override;
    QString toString() const override;

private:
    QHBoxLayout *layout;
    QLabel *label;
    QLineEdit *lineEdit;
};

#endif // CODELINECOMMENTEDITOR_H
