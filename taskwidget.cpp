#include "taskwidget.h"

TaskWidget::TaskWidget(QWidget *parent) : QWidget(parent)
{
    timer = new QTimer(this);
    check = new QCheckBox("启用定时", this);
    spin = new QSpinBox(this);
    btn = new QPushButton("发送", this);
    edit = new QPlainTextEdit(this);

    QHBoxLayout* hlayout = new QHBoxLayout;
    hlayout->addWidget(check);
    hlayout->addWidget(spin);
    hlayout->addWidget(new QWidget(this));
    hlayout->addWidget(btn);
    hlayout->setStretch(2, 1);

    QVBoxLayout* vlayout = new QVBoxLayout(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(edit);
    setLayout(vlayout);
    vlayout->activate();

    spin->setSuffix("秒");
    spin->setRange(10, 86400);
    spin->setSingleStep(10);
    edit->setPlaceholderText("定时发送的文本，多行则随机发送一行");
    int h = btn->sizeHint().height();
    edit->setMinimumHeight(h);
    edit->setMaximumHeight(h*5);
    edit->setFixedHeight(h);

    auto sendMsg = [=]{
        QStringList sl = edit->toPlainText().split("\n", QString::SkipEmptyParts);
        if (!sl.size())
            return ;
        int r = qrand() % sl.size();
        qDebug() << "发送弹幕信号：" << sl.at(r);
        emit signalSendMsg(sl.at(r));
    };

    connect(check, &QCheckBox::stateChanged, this, [=](int){
        bool enable = check->isChecked();
        if (enable)
            timer->start();
        else
            timer->stop();
    });

    connect(spin, SIGNAL(valueChanged(int)), this, SLOT(slotSpinChanged(int)));

    connect(timer, &QTimer::timeout, this, sendMsg);

    connect(btn, &QPushButton::clicked, this, sendMsg);

    connect(edit, &QPlainTextEdit::textChanged, this, [=]{
        int hh = edit->document()->size().height(); // 应当是高度，但为什么是1？
        QFontMetrics fm(edit->font());
        edit->setFixedHeight(fm.lineSpacing() * (hh + 1));
    });
}

void TaskWidget::slotSpinChanged(int val)
{
    timer->setInterval(val * 1000);
    emit spinChanged(val);
}

void TaskWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    emit signalResized();
}

void TaskWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    emit signalResized();
}
