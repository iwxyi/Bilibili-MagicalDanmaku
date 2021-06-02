#include "conditioneditor.h"

ConditionEditor::ConditionEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    new ConditionHighlighter(document());
//    setWordWrapMode(QTextOption::NoWrap); // 不自动换行
}

ConditionHighlighter::ConditionHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{

}

void ConditionHighlighter::highlightBlock(const QString &text)
{
    static auto getTCF = [](QColor c) {
        QTextCharFormat f;
        f.setForeground(c);
        return f;
    };
    static QList<QSSRule> qss_rules = {
        // [condition]
        QSSRule{QRegularExpression("^(\\[.*?\\])"), getTCF(QColor(128, 34, 172))},
        // 执行函数 >func(args)
        QSSRule{QRegularExpression(">\\s*\\w+\\s*\\(.*?\\)($|\\\\n|//.*)"), getTCF(QColor(136, 80, 80))},
        // 变量 %val%
        QSSRule{QRegularExpression("%\\S+?%"), getTCF(QColor(204, 85, 0))},
        // 取值 %{}%
        QSSRule{QRegularExpression("%\\{\\S+?\\}%"), getTCF(QColor(153, 107, 31))},
        // 计算 %[]%
        QSSRule{QRegularExpression("%\\[\\S+?\\]%"), getTCF(QColor(82, 165, 190))},
        // 名字类变量 %xxx_name%
        QSSRule{QRegularExpression("%[^%\\s]*?name[^%\\s]*?%"), getTCF(QColor(237, 51, 0))},
        // 数字 123
        QSSRule{QRegularExpression("\\d+?"), getTCF(QColor(9, 54, 184))},
        // 字符串 'str'  "str
        QSSRule{QRegularExpression("('.*?'|\".*?\")"), getTCF(QColor(80, 200, 120))},
        // 优先级 []**
        QSSRule{QRegularExpression("(?<=\\])\\s*(\\*)+"), getTCF(QColor(82, 165, 190))},
        // 标签 <h1>
        QSSRule{QRegularExpression("(<[^\\],]+?>|\\\\n)"), getTCF(QColor(216, 167, 9))},
        // 冷却通道 (cd5:10)
        QSSRule{QRegularExpression("\\(cd\\d{1,2}:\\d+\\)"), getTCF(QColor(0, 128, 0))},
        // 注释
        QSSRule{QRegularExpression("(?<!:)//.*?(?=\\n|$|\\\\n)"), getTCF(QColor(119, 136, 153))},
        // 开头注释，标记为标题
        QSSRule{QRegularExpression("^\\s*///.*?(?=\\n|$|\\\\n)"), getTCF(QColor(43, 85, 213))},
        // 软换行符
        QSSRule{QRegularExpression("\\s*\\\\\\s*\\n\\s*"), getTCF(QColor(119, 136, 153))},
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
