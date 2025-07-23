#include "codelinesplitterwidget.h"

CodeLineSplitterWidget::CodeLineSplitterWidget(QWidget *parent)
    : CodeLineWidgetBase{parent}
{
    layout = new QVBoxLayout(this);
    label = new QLabel(this);
    layout->addWidget(label);
    label->setText("---");
}

void CodeLineSplitterWidget::fromString(const QString &code)
{
}

QString CodeLineSplitterWidget::toString() const
{
    return "---";
}