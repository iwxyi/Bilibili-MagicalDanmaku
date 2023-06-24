#include <QRegularExpression>
#include "listiteminterface.h"

QColor ListItemInterface::triggerColor = QColor(245, 245, 245);

ListItemInterface::ListItemInterface(QWidget *parent) : QWidget(parent)
{
    _bgLabel = new CustomPaintWidget(this);
    _bgLabel->move(_cardMargin, _cardMargin);
    // _bgLabel->setStyleSheet("background: white; border-radius: 5px;");

    vlayout = new QVBoxLayout(this);
    setLayout(vlayout);
    vlayout->setMargin(vlayout->margin() + _cardMargin);
    vlayout->setSpacing(2);

    check = new QCheckBox("启用", this);
    btn = new InteractiveButtonBase(QIcon(":/icons/run"), this);

    hlayout = new QHBoxLayout;
    hlayout->addWidget(check);
    hlayout->addWidget(new QWidget(this));
    hlayout->addWidget(btn);
    hlayout->setStretch(1, 1);
    hlayout->setMargin(0);
    vlayout->addLayout(hlayout);

    btn->setBorderColor(Qt::black);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setSquareSize();
    btn->setFixedForePos();
    btn->setToolTip("发送/执行");
    check->setToolTip("该功能的开关，如果关闭则不会自动执行\n（除非手动点击右边的发送/执行按钮）");
    check->setCursor(Qt::PointingHandCursor);

    const int radius = 5;
    _bgLabel->setPaint([=](QRect, QPainter* painter) {
        QPainterPath path;
        path.addRoundedRect(_bgLabel->rect(), radius, radius);
        /* {
            int left = check->geometry().right() + (check->x() - _bgLabel->x()) * 2;
            int right = btn->x() - (_bgLabel->geometry().right() - btn->geometry().right()) * 2;
            int bottom = check->geometry().bottom() + vlayout->spacing() / 2;
            QPainterPath cutted;
            cutted.addRoundedRect(QRect(left, 0, right - left, bottom), radius, radius);
            path -= cutted;
        } */
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillPath(path, Qt::white);
        if (_triggering)
        {
            painter->setPen(QPen(triggerColor, 4));
            painter->drawPath(path);
        }
    });

    _triggerTimer = new QTimer(this);
    _triggerTimer->setInterval(200);
    connect(_triggerTimer, &QTimer::timeout, this, [=]{
        _triggering = false;
        update();
    });

    this->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    check->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    btn->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    _bgLabel->setAttribute(Qt::WA_LayoutUsesWidgetRect);
}

void ListItemInterface::setRow(int row)
{
    this->_row = row;
}

int ListItemInterface::getRow() const
{
    return _row;
}

bool ListItemInterface::matchId(QString s) const
{
    QString body = this->body();
    return body.indexOf(QRegularExpression("^\\s*//+\\s*" + s + "\\s*($|\n)")) > -1;
}

void ListItemInterface::triggered()
{
    _triggering = true;
    update();
    _triggerTimer->start();
}

void ListItemInterface::resizeEvent(QResizeEvent *event)
{
    _bgLabel->resize(event->size() - QSize(_cardMargin * 2, _cardMargin * 2));

    return QWidget::resizeEvent(event);
}
