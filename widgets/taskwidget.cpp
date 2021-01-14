#include "conditioneditor.h"
#include "taskwidget.h"

TaskWidget::TaskWidget(QWidget *parent) : QWidget(parent)
{
    timer = new QTimer(this);
    check = new QCheckBox("启用定时", this);
    spin = new QSpinBox(this);
    btn = new QPushButton("发送", this);
    edit = new ConditionEditor(this);

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
    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setStyleSheet("QPlainTextEdit{background: transparent;}");

    auto sendMsgs = [=](bool manual){
        emit signalSendMsgs(edit->toPlainText(), manual);
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
        sendMsgs(false);
    });

    connect(btn, &QPushButton::clicked, this, [=]{
        sendMsgs(true);
    });

    connect(edit, &QPlainTextEdit::textChanged, this, [=]{
        autoResizeEdit();
    });
}

void TaskWidget::slotSpinChanged(int val)
{
    timer->setInterval(val * 1000);
    emit spinChanged(val);
}

void TaskWidget::autoResizeEdit()
{
    edit->document()->setPageSize(QSize(this->width(), edit->document()->size().height()));
    edit->document()->adjustSize();
    int hh = edit->document()->size().height(); // 应该是高度，为啥是行数？
    QFontMetrics fm(edit->font());
    int he = fm.lineSpacing() * (hh + 1);
    int w = this->width();
    this->setFixedHeight(edit->pos().y() + he + 9);
//    this->resize(w, edit->pos().y() + he + 9);
    edit->setFixedHeight(he);
    emit signalResized();
}

void TaskWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
//    autoResizeEdit();
}
