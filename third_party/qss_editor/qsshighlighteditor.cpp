#include "qsshighlighteditor.h"

QSSHighlightEditor::QSSHighlightEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    new QSSHighlighter(document());
}

void QSSHighlightEditor::keyPressEvent(QKeyEvent *e)
{
    QPlainTextEdit::keyPressEvent(e);

    if (e->modifiers() == Qt::NoModifier && (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter))
    {
        // 回车跟随上一行的缩进
        QString left = toPlainText().left(textCursor().position()-1);
        textCursor().insertText(QRegularExpression("\\s*").match(left.mid(left.lastIndexOf("\n")+1)).captured());
    }
}


QSSHighlighter::QSSHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{

}

void QSSHighlighter::highlightBlock(const QString &text)
{
    static auto getTCF = [](QColor c) {
        QTextCharFormat f;
        f.setForeground(c);
        return f;
    };
    static QList<QSSRule> qss_rules = {
        // ID；
        // QSSRule{QRegularExpression("^\\s*[#\\.]\\w+"), getTCF(QColor(222, 49, 99))},
        QSSRule{QRegularExpression("^\\s*[\\w#\\.>:\\-, ]+\\{"), getTCF(QColor(222, 49, 99))},
        // QSSRule{QRegularExpression("^\\s*[\\w#\\.>, ]+\\s*[:\\w+]?\\s*\\{"), getTCF(QColor(222, 49, 99))},
        // 键
        QSSRule{QRegularExpression("[-\\w]+(?=\\s*:[^:])"), getTCF(QColor(151, 49, 197))},
        // 值
        QSSRule{QRegularExpression("(?<=:)\\s*[-#\\w\\d% \\(\\)\\., '\"]+"), getTCF(QColor(204, 85, 0))},
        // 注释
        QSSRule{QRegularExpression("/\\*.*?\\*/"), getTCF(QColor(119, 136, 153))},
        // 单位
        QSSRule{QRegularExpression("(?<=\\d)[a-zA-Z]{1,2}\\b"), getTCF(QColor(62, 106, 198))},
        // 字符串
        QSSRule{QRegularExpression("('.*?'|\".*?\")"), getTCF(QColor(80, 200, 120))},
        // 颜色（动态修改）
    };

    foreach (auto rule, qss_rules)
    {
        auto iterator = rule.pattern.globalMatch(text);
        while (iterator.hasNext()) {
            auto match = iterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // 动态修改颜色
    const QColor escapeColor = Qt::white; // 不设置的颜色，即背景色
    {
        QRegularExpression colorRe("#[0-9a-fA-F]{3,8}\\b");
        auto iterator = colorRe.globalMatch(text);
        while (iterator.hasNext()) {
            auto match = iterator.next();
            QString s = match.capturedTexts().at(0);
            QColor c(s); // 字符串转颜色
            if (c == escapeColor)
                c = Qt::black;
            QTextCharFormat f0 = this->format(match.capturedEnd());

            QTextCharFormat f;
            f.setForeground(c);
            setFormat(match.capturedStart(), match.capturedEnd(), f);
            setFormat(match.capturedEnd(), match.capturedEnd(), f0); // 恢复旧颜色
        }
    }
    {
        QRegularExpression colorRe("([argb]+)\\(([\\d, \\.]+)\\)");
        auto iterator = colorRe.globalMatch(text);
        while (iterator.hasNext()) {
            auto match = iterator.next();
            QString mode = match.captured(1);
            QString vals = match.captured(2);
            QColor c;
            {
                QStringList ms = mode.split("", QString::SkipEmptyParts);
                QStringList vs = vals.split(",", QString::SkipEmptyParts);
                int size = qMin(ms.size(), vs.size());
                for (int i = 0; i < size; i++)
                {
                    QString m = ms.at(i);
                    QString v = vs.at(i).trimmed();
                    if (m == "r")
                        c.setRed(v.toInt());
                    else if (m == "g")
                        c.setGreen(v.toInt());
                    else if (m == "b")
                        c.setBlue(v.toInt());
                    else if (m == "a") // 这个是小数呀
                        c.setAlpha(static_cast<int>(255 * v.toDouble()));
                }
            }
            if (c.red() == escapeColor.red() // 因为有alpha，只能一个个判断了
                    && c.green() == escapeColor.green()
                    && c.blue() == escapeColor.blue())
                c = Qt::black;
            if (c.alpha() <= 16)
            {
                c.setAlpha(255);
            }
            QTextCharFormat f0 = this->format(match.capturedEnd());

            QTextCharFormat f;
            f.setForeground(c);
            setFormat(match.capturedStart(), match.capturedEnd(), f);
            setFormat(match.capturedEnd(), match.capturedEnd(), f0); // 恢复旧颜色
        }
    }
}
