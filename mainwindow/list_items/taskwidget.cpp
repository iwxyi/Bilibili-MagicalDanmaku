#include "taskwidget.h"

TaskWidget::TaskWidget(QWidget *parent) : ListItemInterface(parent)
{
    timer = new QTimer(this);
    spin = new QSpinBox(this);
    edit = new ConditionEditor(this);

    hlayout->insertWidget(1, spin);

    vlayout->addWidget(edit);
    vlayout->activate();

    spin->setSuffix("秒");
    spin->setRange(1, 86400);
    spin->setSingleStep(10);
    edit->setPlaceholderText("定时发送的文本，多行则随机发送一行");
    int h = btn->sizeHint().height();
    edit->setFixedHeight(h);
    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setStyleSheet("QPlainTextEdit,QTextEdit{background: transparent;}");

#ifdef Q_OS_MAC
    spin->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    edit->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

    auto sendMsgs = [=](bool manual){
        emit signalSendMsgs(edit->toPlainText(), manual);
        triggered();
    };

    connect(check, &QCheckBox::stateChanged, this, [=](int){
        bool enable = check->isChecked();
        if (enable)
            timer->start();
        else
            timer->stop();
    });

    connect(spin, SIGNAL(valueChanged(int)), this, SLOT(slotSpinChanged(int)));

    connect(timer, &QTimer::timeout, this, [=]{
        qInfo() << "触发定时任务：" << spin->value() << "秒";
        sendMsgs(false);
    });

    connect(btn, &QPushButton::clicked, this, [=]{
        sendMsgs(true);
    });

    connect(edit, SIGNAL(signalInsertCodeSnippets(const QJsonDocument&)), this, SIGNAL(signalInsertCodeSnippets(const QJsonDocument&)));

    connect(edit, &QTextEdit::textChanged, this, [=]{
        autoResizeEdit();
    });
}

void TaskWidget::fromJson(MyJson json)
{
    check->setChecked(json.b("enabled"));
    spin->setValue(json.i("interval"));
    edit->setPlainText(json.s("text"));

    timer->setInterval(spin->value() * 1000);
}

MyJson TaskWidget::toJson() const
{
    MyJson json;
    json.insert("anchor_key", CODE_TIMER_TASK_KEY);
    json.insert("enabled", check->isChecked());
    json.insert("interval", spin->value());
    json.insert("text", edit->toPlainText());
    return json;
}

bool TaskWidget::isEnabled() const
{
    return check->isChecked();
}

QString TaskWidget::body() const
{
    return edit->toPlainText();
}

void TaskWidget::slotSpinChanged(int val)
{
    timer->setInterval(val * 1000);
    emit spinChanged(val);
}

void TaskWidget::autoResizeEdit()
{
    edit->document()->setPageSize(QSize(this->width() - this->layout()->margin() * 2 - edit->contentsRect().left() * 2, 0));
    //edit->document()->adjustSize();
    int hh = edit->document()->size().height();
    int he = hh + edit->contentsRect().top() * 2;
    int top = check->sizeHint().height();
    this->setFixedHeight(top + he + layout()->margin()*2 + layout()->spacing()*2);
    edit->setFixedHeight(he);
    emit signalResized();
}

void TaskWidget::triggerAction(LiveDanmaku)
{
    emit signalSendMsgs(edit->toPlainText(), false);
    triggered();
}
