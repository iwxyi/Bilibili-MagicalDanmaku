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

    closeButton = new InteractiveButtonBase("×", this);
    closeButton->setObjectName("CollapsibleGroupBoxCloseButton");
    closeButton->setSquareSize();
    closeButton->setFixedForePos();
    closeButton->setTextColor(Qt::darkGray);
    closeButton->setRadius(4);
    closeButton->setVisible(false);

    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(collapseButton);
    titleLayout->addWidget(closeButton);
    collapseButton->raise(); // 不然容易误触到close

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

    connect(closeButton, &InteractiveButtonBase::clicked, this, [this]() {
        emit closeClicked();
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

void CollapsibleGroupBox::setCloseButtonVisible(bool visible)
{
    closeButton->setVisible(visible);
}

bool CollapsibleGroupBox::isCloseButtonVisible() const
{
    return closeButton->isVisible();
}

QWidget *CollapsibleGroupBox::getContentWidget() const
{
    return contentWidget;
}
