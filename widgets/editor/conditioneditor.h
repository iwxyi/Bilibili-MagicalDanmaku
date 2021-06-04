#ifndef CONDITIONEDITOR_H
#define CONDITIONEDITOR_H

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

class ConditionEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    ConditionEditor(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *e) override;
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
