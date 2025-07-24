#ifndef CODELINECOMMENTEDITOR_H
#define CODELINECOMMENTEDITOR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include "interactivebuttonbase.h"
#include "codelinewidgetbase.h"

/**
 * 代码的单行注释编辑器
 * 以`//`开头，也可能是以`///`开头的大标题
 * 也可以是空行，当toString()为空时，返回全空字符串
 */
class CodeLineCommentEditor : public CodeLineWidgetBase
{
    Q_OBJECT
public:
    CodeLineCommentEditor(QWidget *parent = nullptr);

    void fromString(const QString &code) override;
    QString toString() const override;

private:
    QHBoxLayout *layout;
    QLabel *label;
    QLineEdit *lineEdit;
    InteractiveButtonBase *closeBtn;
};

#endif // CODELINECOMMENTEDITOR_H
