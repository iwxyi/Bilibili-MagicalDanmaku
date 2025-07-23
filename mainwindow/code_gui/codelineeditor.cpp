#include "codelineeditor.h"

CodeLineEditor::CodeLineEditor(QWidget *parent)
    : CodeLineWidgetBase{parent}
{}

void CodeLineEditor::fromString(const QString &code)
{
    // 解析代码
}

QString CodeLineEditor::toString() const
{
    // 生成代码
}