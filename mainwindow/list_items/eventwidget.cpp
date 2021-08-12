#include "eventwidget.h"
#include "conditioneditor.h"

EventWidget::EventWidget(QWidget *parent) : ListItemInterface(parent)
{
    eventEdit = new QLineEdit(this);
    actionEdit = new ConditionEditor(this);

    vlayout->addWidget(eventEdit);
    vlayout->addWidget(actionEdit);
    vlayout->activate();

    eventEdit->setPlaceholderText("事件命令");
    eventEdit->setStyleSheet("QLineEdit{background: transparent;}");
    actionEdit->setPlaceholderText("响应动作，多行则随机执行一行");
    int h = btn->sizeHint().height();
    actionEdit->setMinimumHeight(h);
    actionEdit->setMaximumHeight(h*5);
    actionEdit->setFixedHeight(h);
    actionEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    actionEdit->setStyleSheet("QPlainTextEdit{background: transparent;}");

    auto sendMsgs = [=](bool manual){
        emit signalEventMsgs(actionEdit->toPlainText(), LiveDanmaku(), manual);
    };

    connect(btn, &QPushButton::clicked, this, [=]{
        sendMsgs(true);
    });

    connect(eventEdit, &QLineEdit::textChanged, this, [=]{
        cmdKey = eventEdit->text().trimmed();
    });

    connect(actionEdit, &QPlainTextEdit::textChanged, this, [=]{
        autoResizeEdit();
    });
}

void EventWidget::fromJson(MyJson json)
{
    check->setChecked(json.b("enabled"));
    eventEdit->setText(json.s("event"));
    actionEdit->setPlainText(json.s("action"));

    cmdKey = eventEdit->text();
}

MyJson EventWidget::toJson() const
{
    MyJson json;
    json.insert("anchor_key", CODE_EVENT_ACTION_KEY);
    json.insert("enabled", check->isChecked());
    json.insert("event", eventEdit->text());
    json.insert("action", actionEdit->toPlainText());
    return json;
}

bool EventWidget::isEnabled() const
{
    return check->isChecked();
}

QString EventWidget::title() const
{
    return eventEdit->text();
}

QString EventWidget::body() const
{
    return actionEdit->toPlainText();
}

void EventWidget::triggerCmdEvent(QString cmd, LiveDanmaku danmaku)
{
    if (!check->isChecked() || cmdKey != cmd)
        return ;
    qDebug() << "响应事件：" << cmd;
    emit signalEventMsgs(actionEdit->toPlainText(), danmaku, false);
}

void EventWidget::triggerAction(LiveDanmaku danmaku)
{
    emit signalEventMsgs(actionEdit->toPlainText(), danmaku, false);
}

void EventWidget::autoResizeEdit()
{
    actionEdit->document()->setPageSize(QSize(this->width(), actionEdit->document()->size().height()));
    actionEdit->document()->adjustSize();
    int hh = actionEdit->document()->size().height(); // 应该是高度，为啥是行数？
    QFontMetrics fm(actionEdit->font());
    int he = fm.lineSpacing() * (hh + 1);
    int top = eventEdit->sizeHint().height() + check->sizeHint().height();
    this->setFixedHeight(top + he + layout()->margin()*2 + layout()->spacing()*2);
    actionEdit->setFixedHeight(he);
    emit signalResized();
}
