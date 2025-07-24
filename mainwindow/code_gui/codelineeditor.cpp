#include "codelineeditor.h"

CodeLineEditor::CodeLineEditor(QWidget *parent)
    : CodeLineWidgetBase{parent}
{    
    setObjectName("CodeLineEditor");
    triggerGroupBox = new CollapsibleGroupBox("触发条件", this);
    {
        triggerTab = new QTabWidget(triggerGroupBox);
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
    }
    triggerGroupBox->hide();
    
    conditionGroupBox = new CollapsibleGroupBox("条件", this);
    conditionGroupBox->show();
    {
        conditionVLayout = new QVBoxLayout();
        // 动态数量的条件：纵向大的“或”，横向小的“且”
    }
    priorityGroupBox = new CollapsibleGroupBox("优先级", this);
    {
        QHBoxLayout *layout = new QHBoxLayout(priorityGroupBox->getContentWidget());
        QLabel* priorityLabel = new QLabel("数值越大，优先级越高", this);
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
        QVBoxLayout *prefrenceVLayout = new QVBoxLayout(prefrenceGroupBox->getContentWidget());

        // 冷却通道
        {
            QHBoxLayout *coolingLayout = new QHBoxLayout();
            QLabel *label = new QLabel("冷却：", this);
            
            channelSpinBox = new QSpinBox(this);
            channelSpinBox->setRange(0, 99);
            channelSpinBox->setSingleStep(1);
            channelSpinBox->setValue(0);
            channelSpinBox->setPrefix("通道");
            channelSpinBox->setSuffix("号");

            secondSpinBox = new QSpinBox(this);
            secondSpinBox->setRange(0, 99999);
            secondSpinBox->setSingleStep(1);
            secondSpinBox->setValue(1);
            secondSpinBox->setPrefix("时间");
            secondSpinBox->setSuffix("秒");

            coolingLayout->addWidget(label);
            coolingLayout->addWidget(channelSpinBox);
            coolingLayout->addWidget(secondSpinBox);
            coolingLayout->addStretch();
            prefrenceVLayout->addLayout(coolingLayout);
        }

        // 管理员权限
        {
            QHBoxLayout *adminLayout = new QHBoxLayout();
            QLabel *label = new QLabel("管理员权限：", this);
            QCheckBox *adminCheckBox = new QCheckBox("启用", this);
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
            subAccountSpinBox->setPrefix("子账号");
            subAccountSpinBox->setSuffix("号");
            subAccountLayout->addWidget(label);
            subAccountLayout->addWidget(subAccountSpinBox);
            subAccountLayout->addStretch();
            prefrenceVLayout->addLayout(subAccountLayout);
        }

        // prefrenceGroupBox->getContentWidget()->setLayout(prefrenceVLayout);
    }
    danmakuGroupBox = new CollapsibleGroupBox("弹幕/操作", this);
    {
        danmakuVLayout = new QVBoxLayout(danmakuGroupBox->getContentWidget());
        // 动态数量的弹幕/操作
    }

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(triggerGroupBox);
    mainLayout->addWidget(conditionGroupBox);
    mainLayout->addWidget(priorityGroupBox);
    mainLayout->addWidget(prefrenceGroupBox);
    mainLayout->addWidget(danmakuGroupBox);
    setLayout(mainLayout);
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
    // 生成代码
    return "";
}