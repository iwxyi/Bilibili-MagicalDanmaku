#include "codelinecommenteditor.h"

CodeLineCommentEditor::CodeLineCommentEditor(QWidget *parent)
    : CodeLineWidgetBase{parent}
{
    layout = new QHBoxLayout(this);
    label = new QLabel("注释内容：", this);
    lineEdit = new QLineEdit(this);
    layout->addWidget(label);
    layout->addWidget(lineEdit);
    layout->setStretch(1, 1);
}

void CodeLineCommentEditor::fromString(const QString &code)
{
    QString comment = code;
    if (comment.startsWith("//"))
    {
        comment = comment.mid(2);
    }
    lineEdit->setText(comment.trimmed());
}

QString CodeLineCommentEditor::toString() const{
    QString comment = lineEdit->text();
    if (comment.isEmpty()) // 完全的空行
    {
        return "";
    }
    if (comment.startsWith("//")) // 有些可能是""///" 的情况
    {
        return comment;
    }
    return "// " + comment;
}