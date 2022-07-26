#include "sqleditor.h"

SQLEditor::SQLEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    new SQLHighlighter(document());
}

void SQLEditor::keyPressEvent(QKeyEvent *e)
{
    QPlainTextEdit::keyPressEvent(e);
}

SQLHighlighter::SQLHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{

}

void SQLHighlighter::highlightBlock(const QString &text)
{
    static QStringList keywords {
        "select",
        "create", "table",
        "insert", "into", "values",
        "delete", "from",
        "update", "set", "where",
        "alter", "add", "drop",
        "order", "by", "desc", "aesc",
        "group", "limit", "distinct"
        "inner", "left", "right", "full", "join"
    };
    static auto getTCF = [](QColor c) {
        QTextCharFormat f;
        f.setForeground(c);
        return f;
    };
    static QList<QSSRule> qss_rules = {
        // 关键词
        QSSRule(QRegularExpression("\\b(" + keywords.join("|") + "|\\*)\\b"), getTCF(QColor(62, 106, 198))),
        // 常数
        QSSRule(QRegularExpression("\\d+"), getTCF(QColor(204, 85, 0))),
        // 注释
        QSSRule(QRegularExpression("--.*"), getTCF(QColor(119, 136, 153))),
        // "字符串"
        QSSRule(QRegularExpression("('.*?'|\".*?\")"), getTCF(QColor(80, 200, 120))),
        // `表名`
        QSSRule(QRegularExpression("(`.*?`)"), getTCF(QColor(151, 49, 197))),
        // 警告
        QSSRule{QRegularExpression("\\b(delete|alter|update)\\b"), getTCF(QColor(222, 49, 99))},
    };

    foreach (auto rule, qss_rules)
    {
        auto iterator = rule.pattern.globalMatch(text);
        while (iterator.hasNext()) {
            auto match = iterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
