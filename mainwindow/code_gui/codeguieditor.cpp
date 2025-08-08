#include "codeguieditor.h"
#include "conditioneditor.h"
#include "codelinecommenteditor.h"
#include "codelinesplitterwidget.h"
#include "codelineeditor.h"
#include "facilemenu.h"
#include <QHeaderView>

#define CONDITION_INSERT_TEXT_ROLE Qt::UserRole
#define CONDITION_INSERT_JSON_ROLE Qt::UserRole + 1

CodeGUIEditor::CodeGUIEditor(QWidget *parent)
    : QDialog{parent}
{
    setObjectName("CodeGUIEditor");
    // setWindowModality(Qt::WindowModal);
    setMinimumSize(800, 600);
    resize(1200, 800);
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

    languageWidget = new QWidget(this);
    conditionEditor = new ConditionEditor(this);
    QVBoxLayout *languageLayout = new QVBoxLayout(languageWidget);
    languageLayout->addWidget(conditionEditor);

    codeTypeTab->addTab(itemScrollArea, "内置脚本");
    codeTypeTab->addTab(languageWidget, "编程语言");
    okBtn->setBorderColor(Qt::gray);
    okBtn->setRadius(4);

    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setSpacing(0);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setMargin(0);
    leftLayout->addWidget(triggerTab);
    triggerTab->hide(); // TODO: 隐藏触发条件
    leftLayout->addWidget(codeTypeTab);

    // ----------------------------
    refrenceTab = new QTabWidget(this);
    variableTree = new QTreeWidget(this);
    commandTree = new QTreeWidget(this);
    functionTree = new QTreeWidget(this);
    macroTree = new QTreeWidget(this);
    refrenceTab->addTab(variableTree, "变量");
    refrenceTab->addTab(functionTree, "函数");
    refrenceTab->addTab(commandTree, "命令");
    refrenceTab->addTab(macroTree, "宏");

    definitionDescEdit = new QPlainTextEdit(this);
    definitionDescEdit->setReadOnly(true);
    definitionDescEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    definitionDescSplitter = new QSplitter(this);
    definitionDescSplitter->setOrientation(Qt::Vertical);
    definitionDescSplitter->setChildrenCollapsible(false);
    definitionDescSplitter->setStretchFactor(0, 1);
    definitionDescSplitter->setStretchFactor(1, 0);
    definitionDescSplitter->setHandleWidth(1);
    definitionDescSplitter->addWidget(refrenceTab);
    definitionDescSplitter->addWidget(definitionDescEdit);
    definitionDescSplitter->setSizes({800, 10});

    mainSplitter = new QSplitter(this);
    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(definitionDescSplitter);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setSizes({800, 400});
    mainSplitter->setHandleWidth(1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainSplitter);
    mainLayout->addWidget(okBtn);
    setLayout(mainLayout);

    loadScriptDefinitionsTree();

    connect(variableTree, &QTreeWidget::currentItemChanged, this, [=](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
        if (current) {
            showDefinitionDesc(current->data(0, CONDITION_INSERT_JSON_ROLE).toByteArray());
        }
    });
    connect(variableTree, &QTreeWidget::itemActivated, this, [=]() {
        insertText(variableTree->currentItem()->data(0, CONDITION_INSERT_TEXT_ROLE).toString(), variableTree->currentItem()->data(0, CONDITION_INSERT_JSON_ROLE).toByteArray());
    });
    connect(functionTree, &QTreeWidget::currentItemChanged, this, [=](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
        if (current) {
            showDefinitionDesc(current->data(0, CONDITION_INSERT_JSON_ROLE).toByteArray());
        }
    });
    connect(functionTree, &QTreeWidget::itemActivated, this, [=]() {
        insertText(functionTree->currentItem()->data(0, CONDITION_INSERT_TEXT_ROLE).toString(), functionTree->currentItem()->data(0, CONDITION_INSERT_JSON_ROLE).toByteArray());
    });
    connect(commandTree, &QTreeWidget::currentItemChanged, this, [=](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
        if (current) {
            showDefinitionDesc(current->data(0, CONDITION_INSERT_JSON_ROLE).toByteArray());
        }
    });
    connect(commandTree, &QTreeWidget::itemActivated, this, [=]() {
        insertText(commandTree->currentItem()->data(0, CONDITION_INSERT_TEXT_ROLE).toString(), commandTree->currentItem()->data(0, CONDITION_INSERT_JSON_ROLE).toByteArray());
    });
    connect(macroTree, &QTreeWidget::currentItemChanged, this, [=](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
        if (current) {
            showDefinitionDesc(current->data(0, CONDITION_INSERT_JSON_ROLE).toByteArray());
        }
    });
    connect(macroTree, &QTreeWidget::itemActivated, this, [=]() {
        insertText(macroTree->currentItem()->data(0, CONDITION_INSERT_TEXT_ROLE).toString(), macroTree->currentItem()->data(0, CONDITION_INSERT_JSON_ROLE).toByteArray());
    });
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
        if (lang.contains(QRegularExpression("^var|js|javascript|py|python|py3|python3|lua|exec|qml$")))
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

void CodeGUIEditor::loadScriptDefinitionsTree()
{
    QFile file(":/documents/script_definitions");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    MyJson json(file.readAll());
    QJsonArray variableArray = json.a("variables");
    QJsonArray functionArray = json.a("functions");
    QJsonArray commandArray = json.a("commands");
    QJsonArray macroArray = json.a("macros");

    variableTree->setHeaderLabels({"名称", "描述"});
    functionTree->setHeaderLabels({"名称", "描述"});
    commandTree->setHeaderLabels({"名称", "描述"});
    macroTree->setHeaderLabels({"名称", "描述"});

    // 让第一列（名称）自适应内容宽度
    variableTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    functionTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    commandTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    macroTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    QStringList catalogEnList = {"user", "live", "gift", "program", "time", "game", "greet", "song", "pk", "file", "string","AI", "math", "network", "statistic", "other"};
    QStringList catalogCnList = {"用户", "直播", "礼物", "程序", "时间", "游戏", "问候", "音乐", "PK", "文件", "字符串", "AI", "数学", "网络", "统计", "其他"};

    // 变量
    // 按分类放入TreeWidget中
    QMap<QString, QList<MyJson>> variableTypeList;
    for (auto variableValue : variableArray) // 按catalog分类来遍历，这样能保持排序
    {
        MyJson variableJson = variableValue.toObject();
        if (!variableJson.b("visible", true)) // 如果不可见，则不显示
            continue;

        QString catalogEn = variableJson.s("catalog");
        if (catalogEnList.contains(catalogEn))
        {
            variableTypeList[catalogEn].append(variableJson);
        }
        else
        {
            variableTypeList["other"].append(variableJson);
        }
    }
    for (auto catalogEn : catalogEnList) // 遍历分类
    {
        if (!variableTypeList.contains(catalogEn))
            continue;
        QTreeWidgetItem *typeItem = new QTreeWidgetItem(variableTree);
        typeItem->setText(0, catalogCnList[catalogEnList.indexOf(catalogEn)]);
        typeItem->setText(1, QString::number(variableTypeList[catalogEn].count()));

        for (auto variable : variableTypeList[catalogEn])
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(typeItem);
            item->setText(0, variable.s("zh_name") + " (" + variable.s("name") + ")");
            item->setText(1, variable.s("description").replace("\n", " "));
            item->setData(0, CONDITION_INSERT_TEXT_ROLE, "%" + variable.s("zh_name") + "%");
            item->setData(0, CONDITION_INSERT_JSON_ROLE, variable.toBa());
        }
    }
    variableTree->expandAll();

    // 函数
    QMap<QString, QList<MyJson>> functionTypeList;
    for (auto functionValue : functionArray)
    {
        MyJson functionJson = functionValue.toObject();
        if (!functionJson.b("visible", true)) // 如果不可见，则不显示
            continue;

        QString catalogEn = functionJson.s("catalog");
        if (catalogEnList.contains(catalogEn))
        {
            functionTypeList[catalogEn].append(functionJson); // 按catalog分类来遍历，这样能保持排序
        }
        else
        {
            functionTypeList["other"].append(functionJson);
        }
    }
    for (auto catalogEn : catalogEnList) // 遍历分类
    {
        if (!functionTypeList.contains(catalogEn))
            continue;
        QTreeWidgetItem *typeItem = new QTreeWidgetItem(functionTree);
        typeItem->setText(0, catalogCnList[catalogEnList.indexOf(catalogEn)]);
        typeItem->setText(1, QString::number(functionTypeList[catalogEn].count()));

        for (auto function : functionTypeList[catalogEn])
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(typeItem);
            item->setText(0, function.s("zh_name") + " (" + function.s("name") + ")");
            item->setText(1, function.s("description").replace("\n", " "));
            item->setData(0, CONDITION_INSERT_TEXT_ROLE, "%>" + function.s("zh_name") + "()%");
            item->setData(0, CONDITION_INSERT_JSON_ROLE, function.toBa());
        }
    }
    functionTree->expandAll();

    // 命令
    QMap<QString, QList<MyJson>> commandTypeList;
    for (auto commandValue : commandArray)
    {
        MyJson commandJson = commandValue.toObject();
        if (!commandJson.b("visible", true)) // 如果不可见，则不显示
            continue;

        QString catalogEn = commandJson.s("catalog");
        if (catalogEnList.contains(catalogEn))
        {
            commandTypeList[catalogEn].append(commandJson);
        }
        else
        {
            commandTypeList["other"].append(commandJson);
        }
    }
    for (auto catalogEn : catalogEnList) // 遍历分类
    {
        if (!commandTypeList.contains(catalogEn))
            continue;
        QTreeWidgetItem *typeItem = new QTreeWidgetItem(commandTree);
        typeItem->setText(0, catalogCnList[catalogEnList.indexOf(catalogEn)]);
        typeItem->setText(1, QString::number(commandTypeList[catalogEn].count()));

        for (auto command : commandTypeList[catalogEn])
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(typeItem);
            item->setText(0, command.s("zh_name") + " (" + command.s("name") + ")");
            item->setText(1, command.s("description").replace("\n", " "));
            item->setData(0, CONDITION_INSERT_TEXT_ROLE, ">" + command.s("zh_name") + "()");
            item->setData(0, CONDITION_INSERT_JSON_ROLE, command.toBa());
        }
    }
    commandTree->expandAll();

    // 宏
    QMap<QString, QList<MyJson>> macroTypeList;
    for (auto macroValue : macroArray)
    {
        MyJson macroJson = macroValue.toObject();
        if (!macroJson.b("visible", true)) // 如果不可见，则不显示
            continue;

        QString catalogEn = macroJson.s("catalog");
        if (catalogEnList.contains(catalogEn))
        {
            macroTypeList[catalogEn].append(macroJson);
        }
        else
        {
            macroTypeList["other"].append(macroJson);
        }
    }
    for (auto catalogEn : catalogEnList) // 遍历分类
    {
        if (!macroTypeList.contains(catalogEn))
            continue;
        QTreeWidgetItem *typeItem = new QTreeWidgetItem(macroTree);
        typeItem->setText(0, catalogCnList[catalogEnList.indexOf(catalogEn)]);
        typeItem->setText(1, QString::number(macroTypeList[catalogEn].count()));

        for (auto macro : macroTypeList[catalogEn])
        {
            QTreeWidgetItem *item = new QTreeWidgetItem(typeItem);
            item->setText(0, macro.s("zh_name") + " (" + macro.s("name") + ")");
            item->setText(1, macro.s("description").replace("\n", " "));
            item->setData(0, CONDITION_INSERT_TEXT_ROLE, "#" + macro.s("zh_name") + "()");
            item->setData(0, CONDITION_INSERT_JSON_ROLE, macro.toBa());
        }
    }
    macroTree->expandAll();
}

void CodeGUIEditor::showDefinitionDesc(const QByteArray &json)
{
    MyJson jsonObj = MyJson(json);
    QString full;
    full += "名称：" + jsonObj.s("name") + "\n";
    full += "中文：" + jsonObj.s("zh_name") + "\n";
    if (!jsonObj.s("description").isEmpty()) {
        full += "描述：" + jsonObj.s("description") + "\n";
    }
    QJsonArray parameters = jsonObj.a("parameters");
    if (parameters.count() > 0) {
        full += "参数：\n";
        for (auto parameter : parameters) {
            MyJson param = parameter.toObject();
            full += "    - " + param.s("name") + (param.b("optional") ? "(可选)" : "") + "：" + param.s("description") + "\n";
        }
    }
    if (jsonObj.b("return") && !jsonObj.s("return").isEmpty()) {
        full += "返回：" + jsonObj.s("return") + "\n";
    }
    definitionDescEdit->setPlainText(full);
}

/// 插入到当前的文本框中
/// 当前光标所在的文本框
void CodeGUIEditor::insertText(const QString &text, const QByteArray &jsonStr)
{
    ConditionEditor *editor = ConditionEditor::currentConditionEditor;
    if (!editor)
        return;

    editor->insertPlainText(text);
    editor->setFocus();

    // 如果带括号且括号里有参数，那么光标自动移动到括号里面
    MyJson json = MyJson(jsonStr);
    if (text.contains("(") && text.contains(")")) {
        if (json.a("parameters").count() > 0) {
            int index = text.indexOf("(");
            int rightLength = text.length() - index - 1;
            QTextCursor cursor = editor->textCursor();
            int currentPos = cursor.position();
            cursor.setPosition(currentPos - rightLength);
            editor->setTextCursor(cursor);
        }
    }
}
