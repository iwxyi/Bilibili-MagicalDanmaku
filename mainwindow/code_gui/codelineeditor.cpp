#include "codelineeditor.h"
#include "facilemenu.h"

CodeLineEditor::CodeLineEditor(QWidget *parent)
    : CodeLineWidgetBase{parent}
{    
    setObjectName("CodeLineEditor");
    
    conditionGroupBox = new CollapsibleGroupBox("条件", this);
    conditionGroupBox->show();
    {
        conditionVLayout = new QVBoxLayout(conditionGroupBox->getContentWidget());
        // 动态数量的条件：纵向大的“或”，横向小的“且”
        // 添加按钮
        QHBoxLayout *btnLayout = new QHBoxLayout();
        addConditionBtn = new InteractiveButtonBase(QIcon(":/icons/add_circle"), "添加条件组（满足任一组即可）", this);
        connect(addConditionBtn, &InteractiveButtonBase::clicked, this, &CodeLineEditor::addConditionOr);
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

void CodeLineEditor::fromString(const QString &code)
{
    // 解析代码
    if (code.isEmpty())
    {
        return;
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
        int count = group.leftEditors.count();
        for (int j = 0; j < count; j++) // 遍历单个条件
        {
            const ConditionLineEditor *leftEditor = group.leftEditors.at(j);
            const ConditionLineEditor *rightEditor = group.rightEditors.at(j);
            const InteractiveButtonBase *compBtn = group.compBtns.at(j);
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

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(leftEditor);
    layout->addWidget(compBtn);
    layout->addWidget(rightEditor);
    layout->addStretch();
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

    group.leftEditors.append(leftEditor);
    group.compBtns.append(compBtn);
    group.rightEditors.append(rightEditor);

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
}

/// 添加大的其他条件（“或”）
void CodeLineEditor::addConditionOr()
{
    // 添加条件
    ConditionsWidgetGroup group;
    group.groupBox = new CollapsibleGroupBox("条件", conditionGroupBox);
    QVBoxLayout *itemsLayout = new QVBoxLayout(group.groupBox->getContentWidget());

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

    // 添加默认的第一个条件
    addConditionAnd(conditionsWidgetGroups.count() - 1);
}

void CodeLineEditor::addDanmaku()
{
    // 添加弹幕/操作
    ConditionLineEditor *editor = new ConditionLineEditor(this);
    editor->setAutoAdjustWidth(false);
    danmakuEditors.append(editor);
    int insertIndex = danmakuVLayout->count() - 1;
    danmakuVLayout->insertWidget(insertIndex, editor);
}
