#ifndef CODELINEEDITOR_H
#define CODELINEEDITOR_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include "conditioneditor.h"
#include "codelinewidgetbase.h"

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
    QLabel *conditionLabel, *priorityLabel, *prefrenceLabel, *danmakuLabel;
    QList<QLineEdit *> conditionEdits;
    QSpinBox *prioritySpinBox;
    QList<ConditionEditor *> conditionEditors;
};

#endif // CODELINEEDITOR_H
