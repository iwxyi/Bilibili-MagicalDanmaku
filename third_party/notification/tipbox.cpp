#include "tipbox.h"

TipBox::TipBox(QWidget *parent) : QWidget(parent), suitable_width(CARD_FIXED_WIDTH),
    bg_color(Qt::white), font_color(Qt::black), btn_color("#4169e1")
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setMinimumSize(CARD_FIXED_WIDTH, 0);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    sum_height = 0;
    hovering = false;

}

TipCard *TipBox::createTipCard(NotificationEntry* noti)
{
    TipCard * card = new TipCard(this, noti);
    addCard(card);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    return card;
}

void TipBox::setBgColor(QColor c)
{
    bg_color = c;
}

void TipBox::setFontColor(QColor c)
{
    font_color = c;
}

void TipBox::setBtnColor(QColor c)
{
    btn_color = c;
}

void TipBox::adjustPosition()
{
    suitable_width = qMax(parentWidget()->width()/5, CARD_FIXED_WIDTH);

    move(parentWidget()->width()-suitable_width-MARGIN_PARENT_RIGHT,
         parentWidget()->height()-height()-MARGIN_PARENT_BOTTOM);
    setFixedWidth(suitable_width);

    foreach (TipCard* card, cards)
    {
        card->slotClosed();
    }
}

void TipBox::addCard(TipCard* card)
{
    card->setBgColor(bg_color);
    card->setFontColor(font_color);
    card->setBtnColor(btn_color);

    cards.append(card);
    connect(card, SIGNAL(signalClosed(TipCard*)), this, SLOT(slotCardClosed(TipCard*)));
    card->show();

    // 关联信号槽
    connect(card, &TipCard::signalCardClicked, [=](NotificationEntry* n){
        emit signalCardClicked(n->click(0));
    });
    connect(card, &TipCard::signalButton1Clicked, [=](NotificationEntry* n){
        emit signalBtnClicked(n->click(1));
    });
    connect(card, &TipCard::signalButton2Clicked, [=](NotificationEntry* n){
        emit signalBtnClicked(n->click(2));
    });
    connect(card, &TipCard::signalButton3Clicked, [=](NotificationEntry* n){
        emit signalBtnClicked(n->click(3));
    });


    // 通知卡片总区域显示的高度
    QSize add_size = card->size();
    sum_height += add_size.height() + CARDS_INTERVAL;
    QRect box_aim(parentWidget()->width()-suitable_width-MARGIN_PARENT_RIGHT,
                  parentWidget()->height()-sum_height-MARGIN_PARENT_BOTTOM,
                  suitable_width,
                  sum_height);
    QPropertyAnimation* box_ani = new QPropertyAnimation(this, "geometry");
    box_ani->setStartValue(geometry());
    box_ani->setEndValue(box_aim);
    box_ani->setDuration(300);
    box_ani->start();
    connect(box_ani, SIGNAL(finished()), box_ani, SLOT(deleteLater()));

    // 最后一个从一个小点扩大到整个卡片
    QRect start_rect(geometry().width()-1, geometry().height()-1, 1, 1);
    QRect end_rect(0, sum_height-add_size.height()-CARDS_INTERVAL, add_size.width(), add_size.height());
    QPropertyAnimation* card_ani = new QPropertyAnimation(card, "geometry");
    card_ani->setStartValue(start_rect);
    card_ani->setEndValue(end_rect);
    card_ani->setDuration(300);
    card_ani->start();
    connect(card_ani, SIGNAL(finished()), card_ani, SLOT(deleteLater()));

    // 判断位置，如果鼠标不在这上面，则开启定时消失
    if (!hovering)
    {
        card->startWaitingLeave();
    }
}

void TipBox::slotCardClosed(TipCard* removed_card)
{
    int index, temp_height = 0;
    QSize removed_size = removed_card->size();

    // 整体高度变化
    sum_height -= removed_size.height() + CARDS_INTERVAL;
    QRect box_aim(parentWidget()->width()-suitable_width-MARGIN_PARENT_RIGHT,
                  parentWidget()->height()-sum_height-MARGIN_PARENT_BOTTOM,
                  suitable_width,
                  sum_height);
    QPropertyAnimation* box_ani = new QPropertyAnimation(this, "geometry");
    box_ani->setStartValue(geometry());
    box_ani->setEndValue(box_aim);
    box_ani->setDuration(300);
    box_ani->start();
    connect(box_ani, SIGNAL(finished()), box_ani, SLOT(deleteLater()));

    // 下面的每个卡片上移
    for (index = 0; index < cards.size(); index++)
    {
        if (cards.at(index) == removed_card)
            continue ;
        TipCard* card = cards.at(index);
        QRect end_rect(0, temp_height, suitable_width, card->height());

        QPropertyAnimation* card_ani = new QPropertyAnimation(card, "geometry");
        card_ani->setStartValue(card->geometry());
        card_ani->setEndValue(end_rect);
        card_ani->setDuration(300);
        card_ani->start();

        temp_height += card->height() + CARDS_INTERVAL;
    }

    // 要删除的卡片，缩小成右边的一个点
    QRect start_rect(removed_card->geometry());
    QRect end_rect(suitable_width-1, removed_card->geometry().top(), 1, 1);
    QPropertyAnimation* animation = new QPropertyAnimation(removed_card, "geometry");
    animation->setStartValue(start_rect);
    animation->setEndValue(end_rect);
    animation->setDuration(300);
    animation->start();
    connect(animation, SIGNAL(finished()), animation, SLOT(deleteLater()));
    connect(animation, &QPropertyAnimation::finished, [=]{ removed_card->noti->deleteLater(); delete removed_card; });
    cards.removeOne(removed_card);

    if (cards.size() == 0)
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void TipBox::enterEvent(QEvent *event)
{
    hovering = true;
    foreach (TipCard* card, cards) {
        card->pauseWaitingLeave();
    }
    return QWidget::enterEvent(event);
}

void TipBox::leaveEvent(QEvent *event)
{
    hovering = false;
    foreach (TipCard* card, cards) {
        card->startWaitingLeave();
    }
    return QWidget::leaveEvent(event);
}

/*void TipBox::paintEvent(QPaintEvent *event)
{
    // 背景调试
    QPainter painter(this);
    QPainterPath path_back;
    path_back.setFillRule(Qt::WindingFill);
    path_back.addRoundedRect(QRect(0, 0, width(), height()), 3, 3);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillPath(path_back, QBrush(Qt::red));
    return QWidget::paintEvent(event);
}*/
