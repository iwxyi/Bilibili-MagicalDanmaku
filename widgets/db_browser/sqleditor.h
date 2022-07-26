#ifndef SQLEDITOR_H
#define SQLEDITOR_H

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

class SQLEditor : public QPlainTextEdit
{
public:
    SQLEditor(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *e) override;
};

class SQLHighlighter : public QSyntaxHighlighter
{
    struct QSSRule {
        QRegularExpression pattern;
        QTextCharFormat format;

        QSSRule(QRegularExpression re, QTextCharFormat fo)
            : pattern(re), format(fo)
        {
            pattern.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }
    };

public:
    SQLHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;
};

#endif // SQLEDITOR_H
