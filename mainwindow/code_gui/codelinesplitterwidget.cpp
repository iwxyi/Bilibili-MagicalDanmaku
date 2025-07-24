#include "codelinesplitterwidget.h"

CodeLineSplitterWidget::CodeLineSplitterWidget(QWidget *parent)
    : CodeLineWidgetBase{parent}
{
    layout = new QHBoxLayout(this);
    label = new QLabel(this);
    layout->addWidget(label);
    label->setText("---");

    closeBtn = new InteractiveButtonBase("×", this);
    closeBtn->setObjectName("CodeLineSplitterWidgetCloseBtn");
    closeBtn->setSquareSize();
    closeBtn->setFixedForePos();
    closeBtn->setTextColor(Qt::darkGray);
    closeBtn->setRadius(4);
    layout->setStretch(0, 1);
    layout->addWidget(closeBtn);

    connect(closeBtn, &InteractiveButtonBase::clicked, this, &CodeLineWidgetBase::toClose);
}

void CodeLineSplitterWidget::fromString(const QString &code)
{
}

QString CodeLineSplitterWidget::toString() const
{
    return "---";
}