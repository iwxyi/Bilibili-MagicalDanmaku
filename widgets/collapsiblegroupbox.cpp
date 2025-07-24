#include "collapsiblegroupbox.h"
#include <QVBoxLayout>

CollapsibleGroupBox::CollapsibleGroupBox(QWidget *parent)
    : QWidget{parent}
{
    setObjectName("CollapsibleGroupBox");
    setAttribute(Qt::WA_StyledBackground, true);
    
    titleLabel = new QLabel(this);
    titleLabel->setObjectName("CollapsibleGroupBoxTitle");
    titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setStyleSheet("font-size: 12px; font-weight: bold;");

    collapseButton = new InteractiveButtonBase("▲", this);
    collapseButton->setObjectName("CollapsibleGroupBoxCollapseButton");
    collapseButton->setSquareSize();
    collapseButton->setFixedForePos();
    collapseButton->setTextColor(Qt::darkGray);
    collapseButton->setRadius(4);

    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(collapseButton);

    contentWidget = new QWidget(this);
    contentWidget->setObjectName("CollapsibleGroupBoxContent");
    contentWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(titleLayout);
    mainLayout->addWidget(contentWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);

    connect(collapseButton, &InteractiveButtonBase::clicked, this, [this]() {
        setCollapsed(!isCollapsed());
        collapseButton->setText(isCollapsed() ? "▼" : "▲");
    });

    QString color = "#f8f8f8";
    // 如果自己的parent也是CollapsibleGroupBox，则背景色为#f0f0f0，用于多层级区分（只限两级）
    if (this->parentWidget() && this->parentWidget()->objectName() == this->objectName())
        color = "#f0f0f0";
    this->setStyleSheet("#CollapsibleGroupBoxContent { background-color: " + color + "; border-radius: 4px; }");
}

CollapsibleGroupBox::CollapsibleGroupBox(const QString &title, QWidget *parent)
    : CollapsibleGroupBox(parent)
{
    setTitle(title);
}

void CollapsibleGroupBox::setTitle(const QString &title)
{
    titleLabel->setText(title);
}

QString CollapsibleGroupBox::getTitle() const
{
    return titleLabel->text();
}

void CollapsibleGroupBox::setCollapsed(bool collapsed)
{
    contentWidget->setVisible(!collapsed);
}

bool CollapsibleGroupBox::isCollapsed() const
{
    return !contentWidget->isVisible();
}

QWidget *CollapsibleGroupBox::getContentWidget() const
{
    return contentWidget;
}
