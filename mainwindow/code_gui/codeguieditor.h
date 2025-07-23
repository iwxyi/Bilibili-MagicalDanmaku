#ifndef CODEGUIEDITOR_H
#define CODEGUIEDITOR_H

#include <QWidget>
#include <QTabWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include "codelineeditor.h"
#include "codeeditorinterface.h"

/**
 * 代码可视化编辑器
 */
class CodeGUIEditor : public QWidget, public CodeEditorInterface
{
    Q_OBJECT
public:
    explicit CodeGUIEditor(QWidget *parent = nullptr);

    void fromString(const QString &code) override;
    QString toString() const override;

public slots:
    void loadEmptyCode();
    void setCode(const QString &code);
    void appendCodeLine();

signals:
    void signalEditFinished(const QString &code);

private:
    QTabWidget *codeTypeTab; // 代码类型：内置脚本、编程语言
    QScrollArea *itemScrollArea;
    QWidget *itemScrollWidget;
    QVBoxLayout *itemLayout;
    QList<CodeLineWidgetBase *> itemLineEditors;
};

#endif // CODEGUIEDITOR_H
