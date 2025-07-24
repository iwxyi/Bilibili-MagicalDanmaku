#include "codeguieditor.h"
#include "conditioneditor.h"
#include "codelinecommenteditor.h"
#include "codelinesplitterwidget.h"
#include "codelineeditor.h"
#include "facilemenu.h"

CodeGUIEditor::CodeGUIEditor(QWidget *parent)
    : QDialog{parent}
{
    setObjectName("CodeGUIEditor");
    // setWindowModality(Qt::WindowModal);
    setMinimumSize(800, 600);
    setWindowTitle("代码可视化编辑器");
    setWindowIcon(QIcon(":/icons/code_gui"));
    setWindowIconText("代码可视化编辑器");

    // 触发条件
    triggerTab = new QTabWidget(this);
    QWidget *timerTriggerWidget = new QWidget(this);
    {
        QHBoxLayout *layout = new QHBoxLayout(timerTriggerWidget);
        QLabel *label = new QLabel("每隔", this);
        layout->addWidget(label);

        timerTriggerSpinBox = new QSpinBox(this);
        timerTriggerSpinBox->setRange(1, 99999);
        timerTriggerSpinBox->setSingleStep(1);
        timerTriggerSpinBox->setValue(60);
        timerTriggerSpinBox->setSuffix("秒");
        layout->addWidget(timerTriggerSpinBox);
        layout->addWidget(new QLabel("循环执行", this));
        layout->addStretch();
    }

    QWidget *replyTriggerWidget = new QWidget(this);
    {
        QHBoxLayout *layout = new QHBoxLayout(replyTriggerWidget);
        QLabel *label = new QLabel("弹幕包含：", this);
        layout->addWidget(label);

        replyTriggerLineEdit = new QLineEdit(this);
        replyTriggerLineEdit->setPlaceholderText("支持正则表达式，例如\"^(签到|打卡)$\"");
        layout->addWidget(replyTriggerLineEdit);
    }

    QWidget *eventTriggerWidget = new QWidget(this);
    {
        QHBoxLayout *layout = new QHBoxLayout(eventTriggerWidget);
        QLabel *label = new QLabel("事件名称：", this);
        layout->addWidget(label);

        eventTriggerLineEdit = new QLineEdit(this);
        layout->addWidget(eventTriggerLineEdit);
    }
    
    triggerTab->addTab(timerTriggerWidget, "定时任务");
    triggerTab->addTab(replyTriggerWidget, "弹幕关键字");
    triggerTab->addTab(eventTriggerWidget, "事件触发");

    // 代码种类
    codeTypeTab = new QTabWidget(this);
    okBtn = new InteractiveButtonBase("保存", this);
    connect(okBtn, &InteractiveButtonBase::clicked, this, &CodeGUIEditor::accept);

    itemScrollArea = new QScrollArea(this);
    itemScrollWidget = new QWidget(this);
    itemLayout = new QVBoxLayout(itemScrollWidget);
    itemScrollArea->setWidget(itemScrollWidget);
    itemScrollArea->setWidgetResizable(true);

    addLineBtn = new InteractiveButtonBase(QIcon(":/icons/add_circle"), "添加一行", this);
    addLineBtn->setRadius(4);
    addLineBtn->adjustMinimumSize();
    addLineBtn->setPaddings(72, 18);
    addLineBtn->setLeaveAfterClick(true);
    connect(addLineBtn, &InteractiveButtonBase::clicked, this, &CodeGUIEditor::showAppendNewLineMenu);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(addLineBtn);
    btnLayout->addStretch();
    itemLayout->addLayout(btnLayout);
    itemLayout->addStretch();
    qDebug() << "itemLayout" << itemLayout->count();

    languageWidget = new QWidget(this);
    conditionEditor = new ConditionEditor(this);
    QVBoxLayout *languageLayout = new QVBoxLayout(languageWidget);
    languageLayout->addWidget(conditionEditor);

    codeTypeTab->addTab(itemScrollArea, "内置脚本");
    codeTypeTab->addTab(languageWidget, "编程语言");
    okBtn->setBorderColor(Qt::gray);
    okBtn->setRadius(4);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(triggerTab);
    triggerTab->hide(); // TODO: 隐藏触发条件
    mainLayout->addWidget(codeTypeTab);
    mainLayout->addWidget(okBtn);
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
        if (lang.contains(QRegularExpression("^var|js|javascript|py|python|py3|python3|lua|exec$")))
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
            insertCodeLine(layoutInsertIndex++, lineEditor);
        }
    }
}

/// 生成代码
QString CodeGUIEditor::toString() const
{
    if (codeTypeTab->currentIndex() == 0)
    {
        QStringList lines;
        for (auto line : itemLineEditors)
        {
            lines.append(line->toString());
        }
        return lines.join("\n");
    }
    else
    {
        return conditionEditor->toPlainText();
    }
}

void CodeGUIEditor::loadEmptyCode()
{
    // 加载空代码
    CodeLineEditor *editor = new CodeLineEditor(this);
    editor->addConditionOr();
    editor->addConditionAnd(0);
    editor->addDanmaku();
    insertCodeLine(0, editor);
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
    menu->addAction("代码行", [this]() {
        CodeLineEditor *editor = new CodeLineEditor(this);
        insertCodeLine(itemLineEditors.count(), editor);
        editor->addConditionOr();
        editor->addConditionAnd(0);
        editor->addDanmaku();
    });
    menu->addAction("注释", [this]() { insertCodeLine(itemLineEditors.count(), new CodeLineCommentEditor(this)); });
    menu->addAction("分割线", [this]() { insertCodeLine(itemLineEditors.count(), new CodeLineSplitterWidget(this)); });
    menu->exec(QCursor::pos());
}

void CodeGUIEditor::insertCodeLine(int index, CodeLineWidgetBase *editor)
{
    itemLineEditors.insert(index, editor);
    itemLayout->insertWidget(index, editor);

    connect(editor, &CodeLineWidgetBase::toClose, this, [=]() {
        itemLineEditors.removeAll(editor);
        itemLayout->removeWidget(editor);
        delete editor;
    });
}