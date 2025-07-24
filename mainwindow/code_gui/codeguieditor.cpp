#include "codeguieditor.h"
#include "conditioneditor.h"
#include "codelinecommenteditor.h"
#include "codelinesplitterwidget.h"
#include "codelineeditor.h"
#include "facilemenu.h"

CodeGUIEditor::CodeGUIEditor(QWidget *parent)
    : QDialog{parent}
{
    // setWindowModality(Qt::WindowModal);
    setMinimumSize(800, 600);
    setWindowTitle("代码可视化编辑器");
    setWindowIcon(QIcon(":/icons/code_gui"));
    setWindowIconText("代码可视化编辑器");

    codeTypeTab = new QTabWidget(this);
    okBtn = new InteractiveButtonBase("保存", this);
    connect(okBtn, &InteractiveButtonBase::clicked, this, &CodeGUIEditor::accept);

    itemScrollArea = new QScrollArea(this);
    itemScrollWidget = new QWidget(this);
    itemLayout = new QVBoxLayout(itemScrollWidget);
    itemScrollArea->setWidget(itemScrollWidget);
    itemScrollArea->setWidgetResizable(true);

    addLineBtn = new WaterCircleButton(QIcon(":/icons/add_circle"), this);
    connect(addLineBtn, &WaterCircleButton::clicked, this, &CodeGUIEditor::showAppendNewLineMenu);
    itemLayout->addWidget(addLineBtn);
    itemLayout->addStretch();

    languageWidget = new QWidget(this);
    conditionEditor = new ConditionEditor(this);
    QVBoxLayout *languageLayout = new QVBoxLayout(languageWidget);
    languageLayout->addWidget(conditionEditor);

    codeTypeTab->addTab(itemScrollArea, "内置脚本");
    codeTypeTab->addTab(languageWidget, "编程语言");
    okBtn->setBorderColor(Qt::gray);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(codeTypeTab);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    mainLayout->addLayout(btnLayout);
    setLayout(mainLayout);
}

/// 解析代码
void CodeGUIEditor::fromString(const QString &_code)
{
    QString code = _code;
    // 1. 判断是什么类型的代码
    QString firstLine = code.trimmed().split("\n").first();
    bool isLanguage = false;
    if (firstLine.contains(":")) // 例如：`py:`
    {
        QString lang = firstLine.split(":").first().toLower();
        if (lang.contains(QRegularExpression("^js|javascript|py|python|py3|python3|lua|exec$")))
        {
            isLanguage = true;
        }
    }

    if (isLanguage)
    {
        codeTypeTab->setCurrentIndex(1);
        conditionEditor->setPlainText(code);
        return;
    }
    else
    {
        codeTypeTab->setCurrentIndex(0);
    }

    // 2. 去除转义的空行 （"\\\n"）
    code = code.replace("\\\n", "");

    // 3. 根据类型，加载不同的代码
    QStringList lines = code.split("\n");
    int layoutInsertIndex = 0;
    for (auto line : lines)
    {
        CodeLineWidgetBase *lineEditor = nullptr;
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith("//")) // 是注释
        {
            lineEditor = new CodeLineCommentEditor(this);
        }
        else if (line.contains(QRegularExpression("^\\s*-{3,}\\s*$"))) // 是分割线
        {
            lineEditor = new CodeLineSplitterWidget(this);
        }
        else // 是代码
        {
            lineEditor = new CodeLineEditor(this);
        }
        if (lineEditor)
        {
            lineEditor->fromString(line);
            itemLayout->insertWidget(layoutInsertIndex++, lineEditor);
            itemLineEditors.append(lineEditor);
        }
    }
}

/// 生成代码
QString CodeGUIEditor::toString() const
{
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
    appendCodeLine<CodeLineEditor>(new CodeLineEditor(this));
}

void CodeGUIEditor::setCode(const QString &code)
{
    // 设置代码
    if (code.trimmed().isEmpty())
    {
        loadEmptyCode();
    }
    else
    {
        fromString(code);
    }
}

/// 显示添加新行的菜单
void CodeGUIEditor::showAppendNewLineMenu()
{
    FacileMenu *menu = new FacileMenu(this);
    menu->addAction("代码行", [this]() { appendCodeLine<CodeLineEditor>(new CodeLineEditor(this)); });
    menu->addAction("注释", [this]() { appendCodeLine<CodeLineCommentEditor>(new CodeLineCommentEditor(this)); });
    menu->addAction("分割线", [this]() { appendCodeLine<CodeLineSplitterWidget>(new CodeLineSplitterWidget(this)); });
    menu->exec(QCursor::pos());
}

/// 插入新行的代码块
template<typename T>
void CodeGUIEditor::appendCodeLine(T *editor)
{
    itemLineEditors.append(editor);

    // 因为最后两个是添加按钮、stretch，所以插入到倒数第三个位置
    int lastIndex = itemLayout->count() - 1;
    itemLayout->insertWidget(lastIndex - 2, editor);
}
