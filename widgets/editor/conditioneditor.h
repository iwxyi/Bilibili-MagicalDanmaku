#ifndef CONDITIONEDITOR_H
#define CONDITIONEDITOR_H

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QCompleter>
#include "myjson.h"
#include "netinterface.h"

class ConditionEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    ConditionEditor(QWidget* parent = nullptr);
    ~ConditionEditor();

    void updateCompleterModel();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void inputMethodEvent(QInputMethodEvent *e) override;
    virtual void insertFromMimeData(const QMimeData *source) override;
    void focusInEvent(QFocusEvent *e) override;

private slots:
    void showCompleter(QString prefix);
    void onCompleterActivated(const QString &completion);
    void showContextMenu(const QPoint &pos);

    void actionGenerateCode();
    void actionModifyCode();
    void actionExplainCode();
    void actionCheckCode();
    void chat(const QString& prompt, const QString& def, NetStringFunc func);

signals:
    void signalInsertCodeSnippets(const QJsonDocument& doc);
    void signalFocusIn();
    void signalDeleted();

public:
    static QStringList allCompletes; // 所有默认填充的
    static int completerWidth;
    static ConditionEditor *currentConditionEditor;

protected:
    QCompleter* completer;
    QString currentPrefix;
};

class ConditionHighlighter : public QSyntaxHighlighter
{
    struct QSSRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

public:
    ConditionHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;
};


#endif // CONDITIONEDITOR_H
