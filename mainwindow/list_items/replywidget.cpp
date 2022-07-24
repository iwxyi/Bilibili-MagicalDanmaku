#include "replywidget.h"

ReplyWidget::ReplyWidget(QWidget *parent) : ListItemInterface(parent)
{
    keyEdit = new QLineEdit(this);
    replyEdit = new ConditionEditor(this);

    vlayout->addWidget(keyEdit);
    vlayout->addWidget(replyEdit);
    vlayout->activate();

    keyEdit->setPlaceholderText("关键词正则表达式");
    keyEdit->setStyleSheet("QLineEdit{background: transparent;}");
    replyEdit->setPlaceholderText("自动回复，多行则随机发送");
    int h = btn->sizeHint().height();
    replyEdit->setMinimumHeight(h);
    replyEdit->setMaximumHeight(h*5);
    replyEdit->setFixedHeight(h);
    replyEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    replyEdit->setStyleSheet("QPlainTextEdit{background: transparent;}");

    keyEdit->setToolTip("正则表达式简单语法\n"
                        "包含关键词：关键词\n"
                        "包含多个关键词：关键词1|关键词2|关键词3\n"
                        "完全为句子：^句子$\n"
                        "完全为多个句子：^(句子1|句子2)$");

    auto sendMsgs = [=](bool manual){
        emit signalReplyMsgs(replyEdit->toPlainText(), LiveDanmaku(), manual);
        triggered();
    };

    connect(btn, &QPushButton::clicked, this, [=]{
        sendMsgs(true);
    });

    connect(keyEdit, &QLineEdit::textChanged, this, [=]{
        keyEmpty = keyEdit->text().trimmed().isEmpty();
        keyRe = QRegularExpression(keyEdit->text());
    });

    connect(replyEdit, SIGNAL(signalInsertCodeSnippets(const QJsonDocument&)), this, SIGNAL(signalInsertCodeSnippets(const QJsonDocument&)));

    connect(replyEdit, &QPlainTextEdit::textChanged, this, [=]{
        autoResizeEdit();
    });
}

void ReplyWidget::fromJson(MyJson json)
{
    check->setChecked(json.b("enabled"));
    keyEdit->setText(json.s("key"));
    replyEdit->setPlainText(json.s("reply"));
}

MyJson ReplyWidget::toJson() const
{
    MyJson json;
    json.insert("anchor_key", CODE_AUTO_REPLY_KEY);
    json.insert("enabled", check->isChecked());
    json.insert("key", keyEdit->text());
    json.insert("reply", replyEdit->toPlainText());
    return json;
}

bool ReplyWidget::isEnabled() const
{
    return check->isChecked();
}

QString ReplyWidget::title() const
{
    return keyEdit->text();
}

QString ReplyWidget::body() const
{
    return replyEdit->toPlainText();
}

void ReplyWidget::slotNewDanmaku(LiveDanmaku danmaku)
{
    if (!check->isChecked() || !danmaku.is(MSG_DANMAKU) || danmaku.isNoReply())
        return ;

    // 判断有无条件
    if (keyEmpty || !keyRe.isValid())
    {
        qWarning() << "无效的自动回复key：" << keyEdit->text();
        return ;
    }

    QRegularExpressionMatch match;
    if (danmaku.getText().indexOf(keyRe, 0, &match) == -1)
        return ;

    // 开始发送
    qInfo() << "自动回复匹配    text:" << danmaku.getText() << "    exp:" << keyEdit->text();
    danmaku.setArgs(match.capturedTexts());
    emit signalReplyMsgs(replyEdit->toPlainText(), danmaku, false);
    triggered();
}

void ReplyWidget::autoResizeEdit()
{
    replyEdit->document()->setPageSize(QSize(this->width(), replyEdit->document()->size().height()));
    replyEdit->document()->adjustSize();
    int hh = replyEdit->document()->size().height(); // 应该是高度，为啥是行数？
    QFontMetrics fm(replyEdit->font());
    int he = fm.lineSpacing() * (hh + 1);
    int top = keyEdit->sizeHint().height() + check->sizeHint().height();
    this->setFixedHeight(top + he + layout()->margin()*2 + layout()->spacing()*2);
    replyEdit->setFixedHeight(he);
    emit signalResized();
}

void ReplyWidget::triggerAction(LiveDanmaku danmaku)
{
    emit signalReplyMsgs(replyEdit->toPlainText(), danmaku, false);
    triggered();
}

/**
 * 强行运行，如果弹幕匹配
 * 无论是否开启
 */
bool ReplyWidget::triggerIfMatch(QString msg, LiveDanmaku danmaku)
{
    // 判断有无条件
    if (!check->isChecked())
        return false;
    if (keyEmpty || !keyRe.isValid())
    {
        qWarning() << "检查到无效的自动回复key：" << keyEdit->text() << replyEdit->toPlainText();
        return false;
    }

    QRegularExpressionMatch match;
    if (msg.indexOf(keyRe, 0, &match) == -1)
        return false;

    // 开始发送
    qInfo() << "自动回复匹配    text:" << danmaku.getText() << "    exp:" << keyEdit->text();
    danmaku.setArgs(match.capturedTexts());
    emit signalReplyMsgs(replyEdit->toPlainText(), danmaku, false);
    return true;
}

