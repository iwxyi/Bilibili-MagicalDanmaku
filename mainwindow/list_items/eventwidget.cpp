#include "eventwidget.h"
#include "fileutil.h"
#include <QScrollBar>

QCompleter* EventWidget::completer = nullptr;

EventWidget::EventWidget(QWidget *parent) : ListItemInterface(parent)
{
    eventEdit = new QLineEdit(this);
    actionEdit = new ConditionEditor(this);

    vlayout->addWidget(eventEdit);
    vlayout->addWidget(actionEdit);
    vlayout->setStretch(2, 1);
    vlayout->activate();

    eventEdit->setPlaceholderText("事件命令");
    eventEdit->setStyleSheet("QLineEdit{background: transparent;}");
    actionEdit->setPlaceholderText("响应动作，多行则随机执行一行");
    int h = btn->sizeHint().height();
    actionEdit->setFixedHeight(h);
    actionEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    actionEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    actionEdit->setStyleSheet("QPlainTextEdit,QTextEdit{background: transparent;}");

#ifdef Q_OS_MAC
    eventEdit->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    actionEdit->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

    auto sendMsgs = [=](bool manual){
        emit signalEventMsgs(actionEdit->toPlainText(), LiveDanmaku(), manual);
        triggered();
    };

    connect(check, &QCheckBox::stateChanged, this, [=]{
        if (isEnabled())
            configShortcut();
#if defined(ENABLE_SHORTCUT)
        else if (shortcut)
            deleteShortcut();
#endif
    });

    connect(btn, &QPushButton::clicked, this, [=]{
        sendMsgs(true);
    });

    // 启动时读取设置就会触发一次事件
    connect(eventEdit, &QLineEdit::textChanged, this, [=]{
        cmdKey = eventEdit->text().trimmed();
        configShortcut();
    });

    connect(actionEdit, SIGNAL(signalInsertCodeSnippets(const QJsonDocument&)), this, SIGNAL(signalInsertCodeSnippets(const QJsonDocument&)));

    /*connect(eventEdit, &QLineEdit::editingFinished, this, [=]{
        configShortcut();
    });*/

    connect(actionEdit, &QTextEdit::textChanged, this, [=]{
        autoResizeEdit();
    });

    // 设置下拉框
    if (completer == nullptr)
    {
        auto model = new QStandardItemModel(this);
        auto sl = readTextFile(":/documents/translation_events").split("\n", QString::SkipEmptyParts);
        QList<QStandardItem*> items;
        for (auto s: sl)
            model->appendRow(new QStandardItem(s));

        completer = new QCompleter(model, this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
    }

    eventEdit->setCompleter(completer);
}

/// 仅在粘贴时生效
void EventWidget::fromJson(MyJson json)
{
    check->setChecked(json.b("enabled"));
    eventEdit->setText(json.s("event"));
    actionEdit->setPlainText(json.s("action"));

    cmdKey = eventEdit->text();
    // configShortcut();
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
    qInfo() << "响应事件：" << cmd;
    if (cmd.contains("NEW_MONTH")) // 针对每个月的单独调试
        qInfo() << actionEdit->toPlainText();
    emit signalEventMsgs(actionEdit->toPlainText(), danmaku, false);
    triggered();
}

void EventWidget::triggerAction(LiveDanmaku danmaku)
{
    emit signalEventMsgs(actionEdit->toPlainText(), danmaku, false);
    triggered();
}

void EventWidget::autoResizeEdit()
{
    actionEdit->document()->setPageSize(QSize(this->width() - this->layout()->margin() * 2 - actionEdit->contentsRect().left() * 2, 0));
    //actionEdit->document()->adjustSize();
    int hh = actionEdit->document()->size().height();
    int codeDocHeight = hh + actionEdit->contentsRect().top() * 2;
    int codeEditTop = eventEdit->sizeHint().height() + check->sizeHint().height();
    this->setFixedHeight(codeEditTop + codeDocHeight + layout()->margin()*2 + layout()->spacing()*3);
    actionEdit->setFixedHeight(codeDocHeight);
    emit signalResized();
}

void EventWidget::configShortcut()
{
#if defined(ENABLE_SHORTCUT)
        // 判断快捷键
        if (isEnabled() && cmdKey.startsWith("KEY:"))
        {
            QString key = cmdKey.right(cmdKey.length() - 4).trimmed();
            if (key.isEmpty())
                return deleteShortcut();
            if (!shortcut)
            {
                shortcut = new QxtGlobalShortcut(this);
                connect(shortcut, &QxtGlobalShortcut::activated, this, [=](){
                    triggerAction(LiveDanmaku());
                });
            }
            shortcut->setShortcut(QKeySequence(key));
            shortcut->setEnabled(isEnabled());
            qInfo() << "注册快捷键：" << shortcut->shortcut();
        }
        else if (shortcut)
        {
            deleteShortcut();
        }
#endif
}

void EventWidget::deleteShortcut()
{
#if defined(ENABLE_SHORTCUT)
    qInfo() << "删除快捷键：" << shortcut->shortcut();
    shortcut->deleteLater();
    shortcut = nullptr;
#endif
}
