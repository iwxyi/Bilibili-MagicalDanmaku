#include <QDebug>
#include <QAbstractItemView>
#include <QStringListModel>
#include "conditioneditor.h"

QStringList ConditionEditor::allCompletes;
int ConditionEditor::completerWidth = 200;

ConditionEditor::ConditionEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    // 设置高亮
    new ConditionHighlighter(document());

    // 设置自动补全
    completer = new QCompleter(allCompletes, this);
    completer->setWidget(this);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    connect(completer, SIGNAL(activated(QString)), this, SLOT(onCompleterActivated(QString)));

    static bool first = true;
    if (!first)
    {
        first = true;
        QFontMetrics fm(this->font());
        completerWidth = fm.horizontalAdvance(">QwertyuiopAsdfghjklZxcvbnm");
    }
}

void ConditionEditor::updateCompleterModel()
{
    completer->setModel(new QStringListModel(allCompletes));
}

void ConditionEditor::keyPressEvent(QKeyEvent *e)
{
    auto mod = e->modifiers();
    if (mod != Qt::NoModifier && mod != Qt::ShiftModifier)
    {
        return QPlainTextEdit::keyPressEvent(e);
    }
    auto key = e->key();

    // ========== 自动提示 ==========
    if (completer->popup() && completer->popup()->isVisible())
    {
        if (key == Qt::Key_Escape || key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Tab)
        {
            e->ignore();
            return ;
        }
    }

    QString fullText = toPlainText();
    int cursorPos = textCursor().position();
    QString left = fullText.left(cursorPos);
    QString leftPairs = "([\"{（【";
    QString rightPairs = ")]\"}）】";

    // Tab跳转
    if (fullText.length() > cursorPos)
    {
        if (key == Qt::Key_Tab)
        {
            QString rightChar = fullText.mid(cursorPos, 1);
            if (rightPairs.contains(rightChar))
            {
                QTextCursor cursor = textCursor();
                cursor.setPosition(cursor.position() + 1);
                setTextCursor(cursor);
                e->accept();
                return ;
            }
        }
        else if (key == Qt::Key_Backspace)
        {
            if (!left.isEmpty())
            {
                QString leftChar = left.right(1);
                QString rightChar = fullText.mid(cursorPos, 1);
                int leftIndex = leftPairs.indexOf(leftChar);
                int rightIndex = rightPairs.indexOf(rightChar);
                if (leftIndex != -1 && leftIndex == rightIndex) // 成对括号
                {
                    QTextCursor cursor = textCursor();
                    cursor.setPosition(cursorPos - 1);
                    cursor.setPosition(cursorPos + 1, QTextCursor::KeepAnchor);
                    cursor.removeSelectedText();
                    setTextCursor(cursor);
                    e->accept();
                    return ;
                }
            }
        }
    }

    QPlainTextEdit::keyPressEvent(e);

    // 修改内容后，所有都需要更新
    fullText = toPlainText();
    cursorPos = textCursor().position();
    left = fullText.left(cursorPos);

    // 回车键
    if (key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        if (left.isEmpty())
            return ;
        // 判断左边是否有表示软换行的反斜杠
        left = left.left(left.length() - 1);
        if (!left.contains(QRegularExpression("\\\\\\s*(//.*)?$")))
            return ;

        // 软换行，回车跟随上一行的缩进，或者起码有一个Tab缩进
        QString indent = QRegularExpression("\\s*").match(left.mid(left.lastIndexOf("\n")+1)).captured();
        textCursor().insertText(indent.isEmpty() ? "\t" : indent);
    }

    // 字母数字键
    else if ((key >= Qt::Key_A &&key <= Qt::Key_Z) || key == Qt::Key_Minus) // 变量或函数
    {
        // 获取当前单词
        QRegularExpressionMatch match;
        if (left.indexOf(QRegularExpression("((%>|%|>)[\\w_\u4e00-\u9fa5]+?)$"), 0, &match) == -1)
            return ;
        QString word = match.captured(1);
        // qInfo() << "当前单词：" << word << match;

        // 开始提示
        showCompleter(word);
        return ;
    }
    else if (key == Qt::Key_Percent) // 百分号%
    {
        showCompleter("%");
        return ;
    }
    else if (key == Qt::Key_Greater)
    {
        showCompleter(">");
        return ;
    }

    // 隐藏提示
    if (completer->popup() && completer->popup()->isVisible())
    {
        completer->popup()->hide();
    }

    // ========== 括号补全 ==========
    auto judgeAndInsert = [&](QString lp, QString rp) {
        if (fullText.length() > cursorPos && fullText.mid(cursorPos, 1) == rp) // 正好是右边的括号
            return ;

        auto insertRight = [&]{
            QTextCursor cursor = textCursor();
            cursor.insertText(rp);
            cursor.setPosition(cursor.position() - 1);
            setTextCursor(cursor);
        };

        QString lineLeft = left.right(left.length() - (left.lastIndexOf("\n") + 1));
        int v = 0;
        if (lp != rp) // 左右不一样的括号
        {
            for (int i = 0; i < lineLeft.length(); i++)
            {
                QString c = lineLeft.at(i);
                if (c == lp)
                    v++;
                else if (c == rp)
                    v--;
            }
            if (v > 0)
            {
                insertRight();
            }
        }
        else
        {
            for (int i = 0; i < lineLeft.length(); i++)
            {
                QString c = lineLeft.at(i);
                if (c == lp)
                    v++;
            }
            if (v % 2 == 1)
            {
                insertRight();
            }
        }

    };

    // 判断左边是否是奇数个括号
    if (key == 40) // (
    {
        judgeAndInsert("(", ")");
        return ;
    }
    else if (key == 91) // [
    {
        judgeAndInsert("[", "]");
        return ;
    }
    else if (key == 123)
    {
        judgeAndInsert("{", "}");
        return ;
    }
    else if (key == 34) // "
    {
        judgeAndInsert("\"", "\"");
        return ;
    }
}

void ConditionEditor::inputMethodEvent(QInputMethodEvent *e)
{
    QPlainTextEdit::inputMethodEvent(e);
    // 获取当前单词
    QString left = toPlainText().left(textCursor().position());
    QRegularExpressionMatch match;
    if (left.indexOf(QRegularExpression("((%>|%|>)[\\w_\u4e00-\u9fa5]+)$"), 0, &match) == -1)
        return ;
    QString word = match.captured(1);
    // qInfo() << "当前判断的单词：" << word;

    // 开始提示
    showCompleter(word);
}

void ConditionEditor::showCompleter(QString prefix)
{
    completer->setCompletionPrefix(prefix);
    if (completer->currentRow() == -1) // 没有匹配的
    {
        if (completer->popup()->isVisible())
        {
            completer->popup()->hide();
        }
        return ;
    }
    QRect cr = cursorRect();
    completer->complete(QRect(cr.left(), cr.bottom(), completerWidth, completer->popup()->sizeHint().height()));
    completer->popup()->move(mapToGlobal(cr.bottomLeft()));
}

void ConditionEditor::onCompleterActivated(const QString &completion)
{
    QString completionPrefix = completer->completionPrefix(),
            shouldInertText = completion;
    QTextCursor cursor = this->textCursor();
    if (!completion.contains(completionPrefix))
    {
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, completionPrefix.size());
        cursor.clearSelection();
    }
    else // 补全相应的字符
    {
        shouldInertText = shouldInertText.replace(
                    shouldInertText.indexOf(completionPrefix),
                    completionPrefix.size(), "");
    }
    cursor.insertText(shouldInertText);

    // 补全右括号
    QString suffixStr = "";
    if (completion.endsWith("("))
        suffixStr = ")";
    if (completion.startsWith("%") && !completion.endsWith("%"))
        suffixStr += "%";

    if (!suffixStr.isEmpty())
    {
        cursor.insertText(suffixStr);
        cursor.setPosition(cursor.position() - suffixStr.length());
        setTextCursor(cursor);
    }
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
        QSSRule{QRegularExpression("^(\\[\\[.*?\\]\\])"), getTCF(QColor(192, 34, 172))},
        QSSRule{QRegularExpression("^(\\[\\[\\[.*?\\]\\]\\])"), getTCF(QColor(255, 34, 172))},
        // 执行函数 >func(args)
        QSSRule{QRegularExpression("(^|[\\]\\)\\*])\\s*>\\s*\\w+\\s*\\(.*?\\)\\s*($|\\\\n|\\\\|//.*)"), getTCF(QColor(136, 80, 80))},
        // 变量 %val%  %.key.val%
        QSSRule{QRegularExpression("%[\\w_\\.]+?%"), getTCF(QColor(204, 85, 0))},
        // 取值 %{}%
        QSSRule{QRegularExpression("%\\{\\S+?\\}%"), getTCF(QColor(153, 107, 31))},
        // 函数 %>func()%
        QSSRule{QRegularExpression("%>\\w+\\(.*\\)%"), getTCF(QColor(153, 107, 31))},
        // 计算 %[]%
        QSSRule{QRegularExpression("%\\[.+?\\]%"), getTCF(QColor(82, 165, 190))},
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
        // 冷却通道 (cd5:10,wait5:10,admin)
        QSSRule{QRegularExpression("\\([\\w:, ]*(cd\\d+:|wait\\d+:|admin)[\\w:, ]*\\)"), getTCF(QColor(0, 128, 0))},
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
