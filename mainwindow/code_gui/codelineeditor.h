#ifndef CODELINEEDITOR_H
#define CODELINEEDITOR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QCheckBox>
#include "conditioneditor.h"
#include "codelinewidgetbase.h"
#include "collapsiblegroupbox.h"

/**
 * 代码逻辑中一行的编辑器
 * 包含：
 * - 条件
 * - 优先级
 * - 弹幕选项
 * - 弹幕内容/命令
 */
class CodeLineEditor : public CodeLineWidgetBase
{
    Q_OBJECT
public:
    explicit CodeLineEditor(QWidget *parent = nullptr);

    void fromString(const QString &code) override;
    QString toString() const override;

signals:

private:
    QVBoxLayout *mainLayout;
    QTabWidget *triggerTab;
    QSpinBox *timerTriggerSpinBox;
    QLineEdit *replyTriggerLineEdit;
    QLineEdit *eventTriggerLineEdit;
    
    CollapsibleGroupBox *triggerGroupBox, *conditionGroupBox, *priorityGroupBox, *prefrenceGroupBox, *danmakuGroupBox;
    QVBoxLayout *conditionVLayout;
    QList<QLineEdit *> conditionEdits;
    QSpinBox *prioritySpinBox;
    QSpinBox *channelSpinBox, *secondSpinBox;
    QCheckBox *adminCheckBox;
    QSpinBox *subAccountSpinBox;
    QVBoxLayout* danmakuVLayout;
    QList<ConditionEditor *> danmakuEditors;
};

#endif // CODELINEEDITOR_H
