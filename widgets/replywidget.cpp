#include "conditioneditor.h"
#include "replywidget.h"

ReplyWidget::ReplyWidget(QWidget *parent) : QWidget(parent)
{
    check = new QCheckBox("启用", this);
    btn = new QPushButton("发送", this);
    keyEdit = new QLineEdit(this);
    replyEdit = new ConditionEditor(this);

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(check);
    hlayout->addWidget(new QWidget(this));
    hlayout->addWidget(btn);
    hlayout->setStretch(1, 1);

    QVBoxLayout* vlayout = new QVBoxLayout(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(keyEdit);
    vlayout->addWidget(replyEdit);
    setLayout(vlayout);
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

    auto sendMsgs = [=](bool manual){
        emit signalReplyMsgs(replyEdit->toPlainText(), LiveDanmaku(), manual);
    };

    connect(btn, &QPushButton::clicked, this, [=]{
        sendMsgs(true);
    });

    connect(keyEdit, &QLineEdit::textChanged, this, [=]{
        keyEmpty = keyEdit->text().trimmed().isEmpty();
        keyRe = QRegularExpression(keyEdit->text());
    });

    connect(replyEdit, &QPlainTextEdit::textChanged, this, [=]{
        autoResizeEdit();
    });
}

void ReplyWidget::slotNewDanmaku(LiveDanmaku danmaku)
{
    if (!check->isChecked() || !danmaku.is(MSG_DANMAKU) || danmaku.isNoReply())
        return ;

    // 判断有无条件
    if (keyEmpty || !keyRe.isValid())
    {
        qDebug() << "无效的自动回复：" << keyEdit->text();
        return ;
    }

    if (danmaku.getText().indexOf(keyRe) == -1)
        return ;

    // 开始发送
    qDebug() << "自动回复匹配    text:" << danmaku.getText() << "    exp:" << keyEdit->text();
    emit signalReplyMsgs(replyEdit->toPlainText(), danmaku, false);
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

void ReplyWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
//    autoResizeEdit();
}
