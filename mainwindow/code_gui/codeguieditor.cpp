#include "codeguieditor.h"

CodeGUIEditor::CodeGUIEditor(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::ApplicationModal);
    setWindowFlags(Qt::Window);
    setWindowTitle("代码可视化编辑器");
    setWindowIcon(QIcon(":/icons/code_gui"));
    setWindowIconText("代码可视化编辑器");


}

void CodeGUIEditor::fromString(const QString &code)
{
    // 解析代码
}

QString CodeGUIEditor::toString() const
{
    // 生成代码
    QStringList lines;
    for (auto line : itemLineEditors)
    {
        lines.append(line->toString());
    }
    return lines.join("\n");
}

void CodeGUIEditor::loadEmptyCode()
{
    // 加载空代码
}

void CodeGUIEditor::setCode(const QString &code)
{
    // 设置代码
}

void CodeGUIEditor::appendCodeLine()
{
    // 添加一行代码
}