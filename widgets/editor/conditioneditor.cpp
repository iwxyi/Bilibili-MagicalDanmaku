#include <QDebug>
#include <QAbstractItemView>
#include <QStringListModel>
#include <QMimeData>
#include <QClipboard>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>
#include "conditioneditor.h"
#include "facilemenu.h"
#include "usersettings.h"
#include "chatgptutil.h"
#include "fileutil.h"

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

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ConditionEditor::customContextMenuRequested, this, &ConditionEditor::showContextMenu);
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
        if (key == Qt::Key_Tab && mod == Qt::NoModifier)
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
    else if ((key >= Qt::Key_A && key <= Qt::Key_Z) || key == Qt::Key_Minus || key == Qt::Key_Underscore) // 变量或函数
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
        if (fullText.length() > cursorPos && fullText.mid(cursorPos, rp.length()) == rp) // 正好是右边的括号
            return ;

        auto insertRight = [&]{
            // 判断是否需要补全%
            if (lp != rp)
            {
                if (left.length() >= 2 && left.mid(left.length() - 2, 1) == "%")
                    rp = rp + "%";
            }
            QTextCursor cursor = textCursor();
            cursor.insertText(rp);
            cursor.setPosition(cursor.position() - rp.length());
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

void ConditionEditor::insertFromMimeData(const QMimeData *source)
{
    if (this->toPlainText().trimmed().isEmpty() && source->hasText() && source->text().contains("神奇弹幕:"))
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(source->text().toUtf8(), &error);
        if (error.error == QJsonParseError::NoError)
        {
            emit signalInsertCodeSnippets(doc);
            qInfo() << "检测到JSON数据，粘贴代码片段";
            return ;
        }
    }
    QPlainTextEdit::insertFromMimeData(source);
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
    QString right = this->toPlainText().mid(cursor.position());
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
        while (suffixStr.size() && right.size())
        {
            if (right.at(0) == suffixStr.at(0))
            {
                suffixStr.remove(0, 1);
                right.remove(0, 1);
            }
            else
                break;
        }
        cursor.insertText(suffixStr);
        cursor.setPosition(cursor.position() - suffixStr.length());
        setTextCursor(cursor);
    }
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
        QSSRule{QRegularExpression("%[\\w_\\.\\?]+?%"), getTCF(QColor(204, 85, 0))},
        // 正则 %$x%
        QSSRule{QRegularExpression("%\\$\\d+%"), getTCF(QColor(204, 85, 0))},
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

ConditionHighlighter::ConditionHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
}

/**************************** 代码生成 ********************************/

void ConditionEditor::showContextMenu(const QPoint &pos)
{
    newFacileMenu;

    const QString& selectedText = this->textCursor().selectedText();
    const QString& clipboardText = QApplication::clipboard()->text();

    const QString& gptTip = "需要启用GPT后才可以使用该功能";

    menu->addAction("复制 (&C)", [=]{
        QApplication::clipboard()->setText(selectedText);
    })->disable(selectedText.isEmpty());

    menu->addAction("粘贴 (&V)", [=]{
        if ((clipboardText.startsWith("{") && clipboardText.endsWith("}"))
                || (clipboardText.startsWith("[") && clipboardText.endsWith("]"))) {
            // TODO: 发送信号到外面，让外层来统一粘贴代码片段
            QMessageBox::information(this, "粘贴提示", "疑似JSON对象，请从外层控件的菜单粘贴，或者选择主菜单中的“粘贴代码片段”");
        }
        this->paste();
    })->disable(clipboardText.isEmpty());

    menu->split();

    menu->addAction("生成代码 (&G)", [=]{
        menu->close();
        actionGenerateCode();
    })->disable(us->open_ai_key.isEmpty())
            ->tooltip(us->open_ai_key.isEmpty(), gptTip);

    menu->addAction("修改代码 (&M)", [=]{
        menu->close();
        actionModifyCode();
    })->disable(us->open_ai_key.isEmpty())
            ->tooltip(us->open_ai_key.isEmpty(), gptTip);

    menu->addAction("解释代码 (&E)", [=]{
        menu->close();
        actionExplainCode();
    })->disable(us->open_ai_key.isEmpty())
            ->tooltip(us->open_ai_key.isEmpty(), gptTip);

    menu->addAction("检查代码 (&F)", [=]{
        menu->close();
        actionCheckCode();
    })->disable(us->open_ai_key.isEmpty())
            ->tooltip(us->open_ai_key.isEmpty(), gptTip);

    menu->exec();
}

void ConditionEditor::actionGenerateCode()
{
    bool ok;
    QString inputText = QInputDialog::getText(this, "生成代码", "请输入需要执行的命令", QLineEdit::Normal, "", &ok);
    if (!ok)
        return;
    qInfo() << "输入内容：" << inputText;

    chat("你是一个生成代码的机器人，负责帮程序员生成代码片段。", inputText, [=](QString text) {
        this->insertPlainText(text);
    });
}

void ConditionEditor::actionModifyCode()
{
    bool ok;
    QString inputText = QInputDialog::getText(this, "修改代码", "请输入需要修改的操作", QLineEdit::Normal, "", &ok);
    if (!ok)
        return;
    qInfo() << "输入内容：" << inputText;

    chat("你是一个修改代码的机器人，负责帮程序员修改代码片段。", inputText + "\n根据上述要求，对下面的代码进行修改，仅需返回代码内容：\n```\n" + toPlainText() + "\n```", [=](QString text) {
        if (text.contains("```"))
        {
            text = text.mid(text.indexOf("```\n") + 4);
            text = text.left(text.lastIndexOf("\n```"));
        }
        this->selectAll();
        this->insertPlainText(text);
    });
}

void ConditionEditor::actionExplainCode()
{
    chat("你是一个解释代码的机器人，负责帮程序员解释代码片段。", "解释下面的代码，阐明语法和语义：\n```\n" + toPlainText() + "\n```", [=](QString text) {
        QMessageBox::information(this, "解释结果", text);
    });
}

void ConditionEditor::actionCheckCode()
{
    chat("你是一个检查代码的机器人，负责帮程序员审阅代码片段。", "审阅下面的代码，找出潜在的语法错误或者逻辑错误：\n```\n" + toPlainText() + "\n```", [=](QString text) {
        QMessageBox::information(this, "解释结果", text);
    });
}

void ConditionEditor::chat(const QString &prompt, const QString &userContent, NetStringFunc func)
{
    /// 初始化ChatGPT
    ChatGPTUtil* chatgpt = new ChatGPTUtil(this);
    chatgpt->setStream(false);

    QProgressDialog* progressDialog = new QProgressDialog(this);
    progressDialog->setLabelText("加载中...");
    progressDialog->setCancelButton(nullptr);
    progressDialog->setRange(0, 100);

    connect(chatgpt, &ChatGPTUtil::signalResponseError, this, [=](const QByteArray& ba) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error == QJsonParseError::NoError)
        {
            QJsonObject json = document.object();
            if (json.contains("error") && json.value("error").isObject())
                json = json.value("error").toObject();
            if (json.contains("message"))
            {
                int code = json.value("code").toInt();
                QString type = json.value("type").toString();
                qCritical() << (json.value("message").toString() + "\n\n错误码：" + QString::number(code) + "  " + type);
                QMessageBox::critical(this, "ChatGPT Error", (json.value("message").toString() + "\n\n错误码：" + QString::number(code) + "  " + type));
            }
            else
            {
                qCritical() << QString(ba);
                QMessageBox::critical(this, "ChatGPT Error", ba);
            }
        }
        else
        {
            qCritical() << QString(ba);
            QMessageBox::critical(this, "ChatGPT Error", ba);
        }
    });

    connect(chatgpt, &ChatGPTUtil::signalRequestStarted, this, [=]{
        progressDialog->exec();
    });
    connect(chatgpt, &ChatGPTUtil::signalResponseFinished, this, [=]{
        progressDialog->accept();
    });
    connect(chatgpt, &ChatGPTUtil::finished, this, [=]{
        chatgpt->deleteLater();
    });
    connect(chatgpt, &ChatGPTUtil::signalResponseText, this, [=](const QString& text) {
        func(text);
    });

    /// 获取上下文
    QString codeSyntax = readTextFile(":/documents/code_syntax_introduction");
    if (!prompt.isEmpty())
        codeSyntax = prompt + "\n" + codeSyntax;

    QList<ChatBean> chats;
    chats.append(ChatBean("system", codeSyntax));
    chats.append(ChatBean("user", userContent));

    qInfo() << prompt;
    chatgpt->getResponse(chats);
}
