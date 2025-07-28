#ifndef CODEGUIEDITOR_H
#define CODEGUIEDITOR_H

#include <QDialog>
#include <QTabWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include "codelineeditor.h"
#include "codeeditorinterface.h"
#include "conditioneditor.h"
#include "interactivebuttonbase.h"

/**
 * 代码可视化编辑器
 */
class CodeGUIEditor : public QDialog, public CodeEditorInterface
{
    Q_OBJECT
public:
    explicit CodeGUIEditor(QWidget *parent = nullptr);

    void fromString(const QString &code) override;
    QString toString() const override;

    void insertCodeLine(int index, CodeLineWidgetBase *editor);

private:
    void loadScriptDefinitionsTree();
    void connectAllConditionEditors(QWidget *widget);

public slots:
    void loadEmptyCode();
    void setCode(const QString &code);
    void showAppendNewLineMenu();
    void showDefinitionDesc(const QByteArray &json);
    void insertText(const QString &text, const QByteArray &json);

signals:
    void signalEditFinished(const QString &code);

private:
    QTabWidget *triggerTab;
    QSpinBox *timerTriggerSpinBox;
    QLineEdit *replyTriggerLineEdit;
    QLineEdit *eventTriggerLineEdit;

    QTabWidget *codeTypeTab; // 代码类型：内置脚本、编程语言
    InteractiveButtonBase *okBtn;
    
    QScrollArea *itemScrollArea;
    QWidget *itemScrollWidget;
    QVBoxLayout *itemLayout;
    QList<CodeLineWidgetBase *> itemLineEditors;
    InteractiveButtonBase *addLineBtn;

    QWidget *languageWidget;
    ConditionEditor *conditionEditor;
    
    QSplitter *mainSplitter;
    QTabWidget *refrenceTab;
    QTreeWidget *variableTree;
    QTreeWidget *functionTree;
    QTreeWidget *commandTree;
    QTreeWidget *macroTree;
    QSplitter *definitionDescSplitter;
    QPlainTextEdit *definitionDescEdit;
};

#endif // CODEGUIEDITOR_H
