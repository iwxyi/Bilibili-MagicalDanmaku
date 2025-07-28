#include "codelineeditor.h"
#include "facilemenu.h"

CodeLineEditor::CodeLineEditor(QWidget *parent)
    : CodeLineWidgetBase{parent}
{    
    setObjectName("CodeLineEditor");
    
    conditionGroupBox = new CollapsibleGroupBox("条件", this);
    conditionGroupBox->setCloseButtonVisible(true);
    connect(conditionGroupBox, &CollapsibleGroupBox::closeClicked, this, &CodeLineWidgetBase::toClose);
    conditionGroupBox->show();
    {
        conditionVLayout = new QVBoxLayout(conditionGroupBox->getContentWidget());
        // 动态数量的条件：纵向大的“或”，横向小的“且”
        // 添加按钮
        QHBoxLayout *btnLayout = new QHBoxLayout();
        addConditionBtn = new InteractiveButtonBase(QIcon(":/icons/add_circle"), "添加条件组（满足任一组即可）", this);
        connect(addConditionBtn, &InteractiveButtonBase::clicked, this, [=]{
            addConditionOr();
            addConditionAnd(0);
        });
        btnLayout->addWidget(addConditionBtn);
        btnLayout->addStretch();
        conditionVLayout->addLayout(btnLayout);
    }
    priorityGroupBox = new CollapsibleGroupBox("优先级", this);
    {
        QHBoxLayout *layout = new QHBoxLayout(priorityGroupBox->getContentWidget());
        QLabel* priorityLabel = new QLabel("数值越大，满足条件时越先执行", this);
        prioritySpinBox = new QSpinBox(this);
        prioritySpinBox->setRange(0, 9);
        prioritySpinBox->setSingleStep(1);
        prioritySpinBox->setValue(0);
        layout->addWidget(priorityLabel);
        layout->addWidget(prioritySpinBox);
        layout->addStretch();
    }
    prefrenceGroupBox = new CollapsibleGroupBox("选项", this);
    {
        prefrenceGroupBox->setCollapsed(true);
        QVBoxLayout *prefrenceVLayout = new QVBoxLayout(prefrenceGroupBox->getContentWidget());

        // 冷却通道
        {
            QHBoxLayout *coolingLayout = new QHBoxLayout();
            QLabel *label = new QLabel("冷却：", this);
            
            cdChannelSpinBox = new QSpinBox(this);
            cdChannelSpinBox->setRange(0, 99);
            cdChannelSpinBox->setSingleStep(1);
            cdChannelSpinBox->setValue(0);
            cdChannelSpinBox->setPrefix("通道");
            cdChannelSpinBox->setSuffix("号");

            cdTimeSpinBox = new QSpinBox(this);
            cdTimeSpinBox->setRange(0, 99999);
            cdTimeSpinBox->setSingleStep(1);
            cdTimeSpinBox->setValue(0);
            cdTimeSpinBox->setPrefix("时间");
            cdTimeSpinBox->setSuffix("秒");

            coolingLayout->addWidget(label);
            coolingLayout->addWidget(cdChannelSpinBox);
            coolingLayout->addWidget(cdTimeSpinBox);
            coolingLayout->addStretch();
            prefrenceVLayout->addLayout(coolingLayout);
        }

        // 等待通道
        {
            QHBoxLayout *waitLayout = new QHBoxLayout();
            QLabel *label = new QLabel("等待：", this);
            waitChannelSpinBox = new QSpinBox(this);
            waitChannelSpinBox->setRange(0, 99);
            waitChannelSpinBox->setSingleStep(1);
            waitChannelSpinBox->setValue(0);
            waitChannelSpinBox->setPrefix("通道");
            waitChannelSpinBox->setSuffix("号");

            waitTimeSpinBox = new QSpinBox(this);
            waitTimeSpinBox->setRange(0, 99999);
            waitTimeSpinBox->setSingleStep(1);
            waitTimeSpinBox->setValue(0);
            waitTimeSpinBox->setPrefix("数量");
            waitTimeSpinBox->setSuffix("条");

            waitLayout->addWidget(label);
            waitLayout->addWidget(waitChannelSpinBox);
            waitLayout->addWidget(waitTimeSpinBox);
            waitLayout->addStretch();
            prefrenceVLayout->addLayout(waitLayout);
        }

        // 管理员权限
        {
            QHBoxLayout *adminLayout = new QHBoxLayout();
            QLabel *label = new QLabel("管理员权限：", this);
            adminCheckBox = new QCheckBox("启用", this);
            adminCheckBox->setChecked(false);
            adminLayout->addWidget(label);
            adminLayout->addWidget(adminCheckBox);
            adminLayout->addStretch();
            prefrenceVLayout->addLayout(adminLayout);
        }

        // 子账号
        {
            QHBoxLayout *subAccountLayout = new QHBoxLayout();
            QLabel *label = new QLabel("子账号：", this);
            subAccountSpinBox = new QSpinBox(this);
            subAccountSpinBox->setRange(0, 99);
            subAccountSpinBox->setSingleStep(1);
            subAccountSpinBox->setValue(0);
            subAccountSpinBox->setSuffix("号");
            subAccountLayout->addWidget(label);
            subAccountLayout->addWidget(subAccountSpinBox);
            subAccountLayout->addStretch();
            prefrenceVLayout->addLayout(subAccountLayout);
        }
    }
    danmakuGroupBox = new CollapsibleGroupBox("弹幕/操作", this);
    {
        danmakuVLayout = new QVBoxLayout(danmakuGroupBox->getContentWidget());
        danmakuVLayout->setSpacing(0);
        // 动态数量的弹幕/操作
        // 添加按钮
        QHBoxLayout *btnLayout = new QHBoxLayout();
        addDanmakuBtn = new InteractiveButtonBase(QIcon(":/icons/add_circle"), "添加弹幕/操作", this);
        connect(addDanmakuBtn, &InteractiveButtonBase::clicked, this, &CodeLineEditor::addDanmaku);
        btnLayout->addWidget(addDanmakuBtn);
        btnLayout->addStretch();
        danmakuVLayout->addLayout(btnLayout);
    }

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(conditionGroupBox);
    mainLayout->addWidget(priorityGroupBox);
    mainLayout->addWidget(prefrenceGroupBox);
    mainLayout->addWidget(danmakuGroupBox);

    addConditionBtn->setRadius(4);
    addConditionBtn->adjustMinimumSize();
    addConditionBtn->setPaddings(18,9);
    addDanmakuBtn->setRadius(4);
    addDanmakuBtn->adjustMinimumSize();
    addDanmakuBtn->setPaddings(18, 9);
}

void CodeLineEditor::fromString(const QString &_code)
{
    // 解析代码
    if (_code.isEmpty())
    {
        return;
    }

    // 条件
    QString code = _code.trimmed();
    QString condRe;
    if (code.contains(QRegularExpression("^\\s*\\[\\[\\[.*\\]\\]\\]")))
        condRe = "^\\s*\\[\\[\\[(.*?)\\]\\]\\]\\s*";
    else if (code.contains(QRegularExpression("^\\s*\\[\\[.*\\]\\]"))) // [[%text% ~ "[\\u4e00-\\u9fa5]+[\\w]{3}[\\u4e00-\\u9fa5]+"]]
        condRe = "^\\s*\\[\\[(.*?)\\]\\]\\s*";
    else
        condRe = "^\\s*\\[(.*?)\\]\\s*";
    QRegularExpression re(condRe);
    if (!re.isValid())
        qWarning() << "invalid condition expression:" << condRe;
    QRegularExpressionMatch match;
    if (code.indexOf(re, 0, &match) != -1) // 有检测到条件表达式
    {
        QString cond = match.capturedTexts().first();
        QString condText = match.capturedTexts().at(1);
        QStringList condOrList = condText.split(QRegularExpression("(;|\\|\\|)"));
        for (const QString &condOr : condOrList)
        {
            addConditionOr();
            auto& group = conditionsWidgetGroups.last();
            int groupIndex = conditionsWidgetGroups.indexOf(group);
            
            QStringList condAndList = condOr.split(QRegularExpression("(,|&&)"));
            for (const QString &condAnd : condAndList)
            {
                addConditionAnd(groupIndex);
                auto& group = conditionsWidgetGroups.at(groupIndex); // 重新获取，因为addConditionAnd会改变group的地址
                int itemIndex = group.conditionItems.count() - 1;
                ConditionItem *item = group.conditionItems.at(itemIndex);
                
                // 分别设置
                QRegularExpression compRe("^\\s*([^<>=!]*?)\\s*([<>=!~]{1,2})\\s*([^<>=!]*?)\\s*$");
                QRegularExpressionMatch compMatch;
                if (condAnd.indexOf(compRe, 0, &compMatch) != -1)
                {
                    item->leftEditor->setPlainText(compMatch.captured(1).trimmed());
                    item->compBtn->setText(compMatch.captured(2).trimmed());
                    item->rightEditor->setPlainText(compMatch.captured(3).trimmed());
                }
                else
                {
                    qWarning() << "invalid condition:" << condAnd;
                }
            }
        }

        code = code.right(code.length() - cond.length());
    }
    else
    {
        conditionGroupBox->setCollapsed(true);
    }

    // 优先级
    QRegularExpression priorityRe("^\\s*\\*+\\s*");
    QRegularExpressionMatch priorityMatch;
    if (code.indexOf(priorityRe, 0, &priorityMatch) != -1)
    {
        QString priority = priorityMatch.captured(0);
        int priorityValue = priority.trimmed().size();
        prioritySpinBox->setValue(priorityValue);
        code = code.right(code.length() - priority.length());
    }
    else
    {
        priorityGroupBox->setCollapsed(true);
    }

    // 选项
    QRegularExpression optionRe("^\\s*\\(([\\w\\d\\s_]+)\\)\\s*");
    QRegularExpressionMatch optionMatch;
    if (code.indexOf(optionRe, 0, &optionMatch) != -1)
    {
        QString option = optionMatch.captured(0);
        QStringList optionList = option.split(QRegularExpression("(,|;)"));
        for (const QString &option : optionList)
        {
            QRegularExpressionMatch match;
            if (option.trimmed().indexOf(QRegularExpression("^cd(\\d+):(\\d+)$"), 0, &match) != -1)
            {
                cdChannelSpinBox->setValue(match.captured(1).trimmed().toInt());
                cdTimeSpinBox->setValue(match.captured(2).trimmed().toInt());
            }
            else if (option.trimmed().indexOf(QRegularExpression("^wait(\\d+):(\\d+)$"), 0, &match) != -1)
            {
                waitChannelSpinBox->setValue(match.captured(1).trimmed().toInt());
                waitTimeSpinBox->setValue(match.captured(2).trimmed().toInt());
            }
            else if (option.trimmed().indexOf(QRegularExpression("^admin[\\s\\w:]*$"), 0, &match) != -1)
            {
                adminCheckBox->setChecked(true);
            }
            else if (option.trimmed().indexOf(QRegularExpression("^ac:(\\d+)$"), 0, &match) != -1)
            {
                subAccountSpinBox->setValue(match.captured(1).trimmed().toInt());
            }
            else
            {
                qWarning() << "invalid option:" << option;
            }
        }
        code = code.right(code.length() - option.length());
    }
    else
    {
        prefrenceGroupBox->setCollapsed(true);
    }

    // 弹幕/操作
    QStringList danmakuList = code.split("\\n");
    for (const QString &danmaku : danmakuList)
    {
        addDanmaku();
        auto& editor = danmakuEditors.last();
        editor->setPlainText(danmaku);
    }
}

QString CodeLineEditor::toString() const
{
    QString result;

    // 条件
    QStringList conditionsList;
    for (int i = 0; i < conditionsWidgetGroups.count(); i++) // 遍历条件组
    {
        QStringList conditionList;
        const ConditionsWidgetGroup &group = conditionsWidgetGroups.at(i);
        int count = group.conditionItems.count();
        for (int j = 0; j < count; j++) // 遍历单个条件
        {
            const ConditionItem *item = group.conditionItems.at(j);
            const ConditionLineEditor *leftEditor = item->leftEditor;
            const ConditionLineEditor *rightEditor = item->rightEditor;
            const InteractiveButtonBase *compBtn = item->compBtn;
            QString left = leftEditor->toPlainText();
            QString right = rightEditor->toPlainText();
            QString comp = compBtn->getText();
            if (right.trimmed().isEmpty()) // 不需要右值
            {
                conditionList.append(left);
            }
            else
            {
                conditionList.append(QString("%1 %2 %3").arg(left).arg(comp).arg(right));
            }
        }
        conditionsList.append(conditionList.join(", "));
    }
    if (!conditionsList.isEmpty())
    {
        result += "[" + conditionsList.join(", ") + "]";
    }

    // 优先级
    QString priorityStr; // 几级优先级就有几个星号
    for (int i = 0; i < prioritySpinBox->value(); i++)
    {
        priorityStr += "*";
    }
    result += priorityStr;

    // 选项
    QStringList optionList;
    if (cdChannelSpinBox->value() > 0 && cdTimeSpinBox->value() > 0)
    {
        optionList.append(QString("cd%1:%2").arg(cdChannelSpinBox->value()).arg(cdTimeSpinBox->value()));
    }
    if (waitChannelSpinBox->value() > 0 && waitTimeSpinBox->value() > 0)
    {
        optionList.append(QString("wait%1:%2").arg(waitChannelSpinBox->value()).arg(waitTimeSpinBox->value()));
    }
    if (adminCheckBox->isChecked())
    {
        optionList.append("admin");
    }
    if (subAccountSpinBox->value() > 0)
    {
        optionList.append(QString("ac:%1").arg(subAccountSpinBox->value()));
    }
    if (!optionList.isEmpty())
    {
        result += "(" + optionList.join(", ") + ")";
    }

    // 弹幕/操作
    QStringList danmakuList;
    for (int i = 0; i < danmakuEditors.count(); i++)
    {
        const ConditionEditor *editor = danmakuEditors.at(i);
        danmakuList.append(editor->toPlainText());
    }
    QString danmakuStr = danmakuList.join("\\n");
    if (!danmakuStr.isEmpty())
    {
        result += danmakuStr;
    }

    return result;
}

/// 添加同一组的其他条件（“且”）
void CodeLineEditor::addConditionAnd(int index)
{
    if (index < 0 || index >= conditionsWidgetGroups.count())
    {
        qWarning() << "condition index out of range, index:" << index;
        return;
    }
    
    // 添加条件
    ConditionsWidgetGroup &group = conditionsWidgetGroups[index];
    ConditionLineEditor *leftEditor = new ConditionLineEditor(this);
    leftEditor->setAutoAdjustWidth(true);
    leftEditor->setPlaceholderText("值1");
    leftEditor->setMinimumWidth(100);
    InteractiveButtonBase *compBtn = new InteractiveButtonBase("=", this);
    ConditionLineEditor *rightEditor = new ConditionLineEditor(this);
    rightEditor->setAutoAdjustWidth(true);
    rightEditor->setPlaceholderText("值2");
    rightEditor->setMinimumWidth(100);
    InteractiveButtonBase *closeBtn = new InteractiveButtonBase("×", this);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(leftEditor);
    layout->addWidget(compBtn);
    layout->addWidget(rightEditor);
    layout->addStretch();
    layout->addWidget(closeBtn);
    QVBoxLayout *itemsLayout = dynamic_cast<QVBoxLayout *>(group.groupBox->getContentWidget()->layout());
    if (itemsLayout)
    {
        int count = itemsLayout->count();
        itemsLayout->insertLayout(count - 1, layout);
    }
    else
    {
        qWarning() << "condition group box layout is not a QVBoxLayout";
    }
    group.groupBox->show();

    ConditionItem *item = new ConditionItem();
    item->leftEditor = leftEditor;
    item->compBtn = compBtn;
    item->rightEditor = rightEditor;
    item->closeBtn = closeBtn;
    group.conditionItems.append(item);

    compBtn->setRadius(4);
    compBtn->setPaddings(6, 0);
    compBtn->adjustMinimumSize();
    compBtn->setLeaveAfterClick(true);
    // compBtn->setSquareSize();
    connect(compBtn, &InteractiveButtonBase::clicked, this, [=]() {
        newFacileMenu;
        menu->addAction("= （等于）", [=]() {
            compBtn->setText("=");
        });
        menu->addAction("!= （不等于）", [=]() {
            compBtn->setText("!=");
        });
        menu->addAction("> （大于）", [=]() {
            compBtn->setText(">");
        });
        menu->addAction(">= （大于等于）", [=]() {
            compBtn->setText(">=");
        });
        menu->addAction("< （小于）", [=]() {
            compBtn->setText("<");
        });
        menu->addAction("<= （小于等于）", [=]() {
            compBtn->setText("<=");
        });
        menu->addAction("~ （文字包含）", [=]() {
            compBtn->setText("~");
        });
        menu->exec(QCursor::pos());
        compBtn->adjustMinimumSize();
    });

    closeBtn->setProperty("layout_ptr", QVariant::fromValue((void*)layout));
    closeBtn->setFixedForePos();
    closeBtn->setTextColor(Qt::darkGray);
    closeBtn->setRadius(4);
    connect(closeBtn, &InteractiveButtonBase::clicked, this, [=]() {
        int index = conditionsWidgetGroups.indexOf(group);
        if (index == -1)
        {
            qWarning() << "condition group not found";
            return;
        }
        auto& group = conditionsWidgetGroups[index];
        int itemIndex = group.conditionItems.indexOf(item);
        ConditionItem *item = group.conditionItems.takeAt(itemIndex);
        
        QHBoxLayout *layout = static_cast<QHBoxLayout*>(closeBtn->property("layout_ptr").value<void*>());
        if (layout)
        {
            layout->removeWidget(item->leftEditor);
            layout->removeWidget(item->compBtn);
            layout->removeWidget(item->rightEditor);
            layout->removeWidget(item->closeBtn);
            delete layout;
        }

        delete item->leftEditor;
        delete item->compBtn;
        delete item->rightEditor;
        delete item->closeBtn;
        delete item;
    });

    // 先显示出来再调整样式，否则因为一开始是隐藏的，所以尺寸计算会有问题
    closeBtn->show();
    QTimer::singleShot(0, this, [=]() {
        closeBtn->setSquareSize();
    });
}

/// 添加大的其他条件（“或”）
void CodeLineEditor::addConditionOr()
{
    // 添加条件
    ConditionsWidgetGroup group;
    group.groupBox = new CollapsibleGroupBox("条件组", conditionGroupBox);
    group.groupBox->setCloseButtonVisible(true);
    connect(group.groupBox, &CollapsibleGroupBox::closeClicked, this, [=]() {
        conditionsWidgetGroups.removeAll(group);
        conditionVLayout->removeWidget(group.groupBox);
        delete group.groupBox;
    });
    QVBoxLayout *itemsLayout = new QVBoxLayout(group.groupBox->getContentWidget());
    itemsLayout->setSpacing(0);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    group.addConditionBtn = new InteractiveButtonBase(QIcon(":/icons/add_circle"), "添加条件（满足组内所有条件）", this);
    connect(group.addConditionBtn, &InteractiveButtonBase::clicked, this, [this, group]() {
        addConditionAnd(conditionsWidgetGroups.indexOf(group));
    });
    group.addConditionBtn->setRadius(4);
    group.addConditionBtn->setPaddings(18, 9);
    group.addConditionBtn->adjustMinimumSize();
    btnLayout->addWidget(group.addConditionBtn);
    btnLayout->addStretch();
    itemsLayout->addLayout(btnLayout);

    conditionsWidgetGroups.append(group);
    conditionVLayout->insertWidget(conditionVLayout->count() - 1, group.groupBox);
}

void CodeLineEditor::addDanmaku()
{
    // 添加弹幕/操作
    ConditionLineEditor *editor = new ConditionLineEditor(this);
    editor->setAutoAdjustWidth(false);
    danmakuEditors.append(editor);

    InteractiveButtonBase *closeBtn = new InteractiveButtonBase("×", this);
    closeBtn->setObjectName("CodeLineEditorDanmakuCloseBtn");
    closeBtn->setSquareSize();
    closeBtn->setFixedForePos();
    closeBtn->setTextColor(Qt::darkGray);
    closeBtn->setRadius(4);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(editor);
    layout->addWidget(closeBtn);

    closeBtn->setProperty("layout_ptr", QVariant::fromValue((void*)layout));
    connect(closeBtn, &InteractiveButtonBase::clicked, this, [=]() {
        danmakuEditors.removeAll(editor);
        QHBoxLayout *layout = static_cast<QHBoxLayout*>(closeBtn->property("layout_ptr").value<void*>());
        delete editor;
        delete closeBtn;
        if (layout)
        {
            danmakuVLayout->removeItem(layout);
            delete layout;
        }
    });
    
    int insertIndex = danmakuVLayout->count() - 1;
    danmakuVLayout->insertLayout(insertIndex, layout);
}
