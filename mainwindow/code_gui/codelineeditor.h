#ifndef CODELINEEDITOR_H
#define CODELINEEDITOR_H

#include <QWidget>
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

private:
    

signals:
};

#endif // CODELINEEDITOR_H
