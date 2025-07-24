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
#include "conditionlineeditor.h"

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

    struct ConditionItem
    {
        ConditionLineEditor *leftEditor;
        InteractiveButtonBase *compBtn;
        ConditionLineEditor *rightEditor;
        InteractiveButtonBase *closeBtn;
    };

    struct ConditionsWidgetGroup
    {
        CollapsibleGroupBox *groupBox;
        QList<ConditionItem *> conditionItems;
        InteractiveButtonBase *addConditionBtn;

        bool operator==(const ConditionsWidgetGroup &other) const
        {
            return groupBox == other.groupBox;
        }
    };

signals:

public slots:
    void addConditionAnd(int index);
    void addConditionOr();
    void addDanmaku();

private:
    QVBoxLayout *mainLayout;
    CollapsibleGroupBox *conditionGroupBox, *priorityGroupBox, *prefrenceGroupBox, *danmakuGroupBox;
    QVBoxLayout *conditionVLayout;
    QList<ConditionsWidgetGroup> conditionsWidgetGroups;
    QSpinBox *prioritySpinBox;
    QSpinBox *cdChannelSpinBox, *cdTimeSpinBox;
    QSpinBox *waitChannelSpinBox, *waitTimeSpinBox;
    QCheckBox *adminCheckBox;
    QSpinBox *subAccountSpinBox;
    QVBoxLayout* danmakuVLayout;
    QList<ConditionEditor *> danmakuEditors;
    InteractiveButtonBase *addConditionBtn, *addDanmakuBtn;
};

#endif // CODELINEEDITOR_H
