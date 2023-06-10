#include "tipcard.h"

TipCard::TipCard(QWidget *parent, NotificationEntry *noti)
    : ThreeDimenButton(parent), noti(noti)
{
    setBgColor(Qt::yellow);
    setCursor(Qt::PointingHandCursor);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // 初始化控件
    title_label = new QLabel(noti->title, this);
    content_label = new QLabel(noti->content, this);
    close_button = new InteractiveButtonBase(QIcon(":/icons/hide_right"), this);
    operator1_button = nullptr;
    operator2_button = nullptr;
    operator3_button = nullptr;

    // 初始化布局
    QHBoxLayout* margin_hlayout = new QHBoxLayout(this);
    {
        QVBoxLayout* main_vlayout = new QVBoxLayout;
        {
            main_vlayout->setSpacing(0);

            // 添加标题
            QHBoxLayout* title_hlayout = new QHBoxLayout;
            {
                title_hlayout->setSpacing(0);

                title_hlayout->addWidget(title_label);
                title_hlayout->addWidget(close_button);
            }
            // 添加内容
            main_vlayout->addSpacing(aop_h*2);
            main_vlayout->addLayout(title_hlayout);
            main_vlayout->addWidget(content_label);
            // 添加按钮
            btn_layout = new QHBoxLayout;
            main_vlayout->addLayout(btn_layout);
            main_vlayout->addSpacing(aop_h*2);
        }
        margin_hlayout->addSpacing(aop_w*2);
        margin_hlayout->addLayout(main_vlayout);
        margin_hlayout->addSpacing(aop_w*2);
    }
    setLayout(margin_hlayout);

    // 添加按钮1
    if (!noti->btn1.isEmpty())
    {
        btn_layout->addWidget(operator1_button = new InteractiveButtonBase(noti->btn1, this));
        operator1_button->setRadius(5);
        connect(operator1_button, &InteractiveButtonBase::clicked, [=]{
            emit noti->signalBtnClicked(1);
            emit signalButton1Clicked(noti);
        });
        this->setFixedSize(width(), height() + operator1_button->height());
    }

    // 添加按钮2
    if (!noti->btn2.isEmpty())
    {
        btn_layout->addWidget(operator2_button = new InteractiveButtonBase(noti->btn2, this));
        operator2_button->setRadius(5);
        connect(operator2_button, &InteractiveButtonBase::clicked, [=]{
            emit noti->signalBtnClicked(2);
            emit signalButton2Clicked(noti);
        });
    }

    // 添加按钮3
    if (!noti->btn3.isEmpty())
    {
        btn_layout->addWidget(operator3_button = new InteractiveButtonBase(noti->btn3, this));
        operator3_button->setRadius(5);
        connect(operator3_button, &InteractiveButtonBase::clicked, [=]{
            emit noti->signalBtnClicked(3);
            emit signalButton3Clicked(noti);
        });
    }

    // 事件
    connect(close_button, SIGNAL(clicked(bool)), this, SLOT(slotClosed()));
    connect(this, &ThreeDimenButton::clicked, [=]{
        emit noti->signalCardClicked();
        emit signalCardClicked(noti);
    });

    // 样式
    QFont bold_font = title_label->font();
    bold_font.setBold(true);
    bold_font.setPointSize(bold_font.pointSize() * 1.2);
    title_label->setFont(bold_font);

    // 高度
    title_label->adjustSize();
    content_label->adjustSize();
//    title_label->setFixedHeight(getWidgetHeight(title_label));
    content_label->setFixedWidth(parentWidget()->width()-aop_w*4-TIP_CARD_CONTENT_MARGIN*2);
    content_label->setFixedHeight(getWidgetHeight(content_label));
    int height = title_label->height() + content_label->height() + aop_h*4 + TIP_CARD_CONTENT_MARGIN*2 + margin_hlayout->margin() * 2;
    if (operator1_button != nullptr)
        height += operator1_button->height();
    setMinimumSize(100, max(50, height));

    // 控件大小
    close_button->setFixedSize(title_label->height(), title_label->height());
    if (parentWidget() != nullptr)
        setFixedWidth(parentWidget()->width());
    content_label->setWordWrap(true);

    // 初始化变量
    is_closing = false;
    has_leaved = false;

    // 定时器
    close_timer = new QTimer(this);
    close_timer->setSingleShot(true);
    connect(close_timer, &QTimer::timeout, this, [=]{
        if (noti->click_at == NotificationEntry::ClickAtButton::CAB_NONE)
            emit noti->signalTimeout();
        slotClosed();
    });
}

void TipCard::slotClosed()
{
    if (is_closing) return ;
    emit noti->signalClosed();
    emit signalClosed(this);
    is_closing = true;
}

void TipCard::startWaitingLeave()
{
    close_timer->start(has_leaved ? 1000 : (noti->time>0?noti->time:5000));
}

void TipCard::pauseWaitingLeave()
{
    close_timer->stop();
}

void TipCard::setBgColor(QColor c)
{
    ThreeDimenButton::setBgColor(c);
}

void TipCard::setFontColor(QColor c)
{
    QString color_qss = "color:" + colorToCss(c) + ";";
    title_label->setStyleSheet("padding-left:5px;\n" + color_qss);
    content_label->setStyleSheet(color_qss);
}

void TipCard::setBtnColor(QColor c)
{
    if (operator1_button != nullptr && (noti->default_btn == -1 || noti->default_btn == 1))
        operator1_button->setTextColor(c);
    if (operator2_button != nullptr && (noti->default_btn == -1 || noti->default_btn == 2))
        operator2_button->setTextColor(c);
    if (operator3_button != nullptr && (noti->default_btn == -1 || noti->default_btn == 3))
        operator3_button->setTextColor(c);
}

void TipCard::enterEvent(QEvent *event)
{
    ThreeDimenButton::enterEvent(event);

    if (is_closing) return ;

    close_timer->stop();
}

void TipCard::leaveEvent(QEvent *event)
{
    ThreeDimenButton::leaveEvent(event);

    has_leaved = true;
}

QString TipCard::colorToCss(QColor c)
{
    if (c.alpha() == 255)
        return QString("rgb(%1, %2, %3)").arg(c.red()).arg(c.green()).arg(c.blue());
    else
        return QString("rgb(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}

QString TipCard::getXml(QString str, QString tag)
{
    int pos = str.indexOf("<"+tag+">");
    if (pos == -1) return str;
    int pos2 = str.indexOf("</"+tag+">");
    if (pos2 == -1) return str.right(str.length()-pos-tag.length()-2);
    return str.mid(pos+tag.length()+2, pos2-tag.length()-2);
}

template<typename T>
int TipCard::getWidgetHeight(T *w)
{
    QStringList strs = w->text().split("\n"); // 分成多行
    int w_width = w->width();                 // 控件宽度（用以拆分长字符串为多行）
    int height = 0;
    QFontMetrics fm = w->fontMetrics();
    double mPixelPerCentimer = 1.0;
    int linesCount = 0;                       // 放在控件上的文字总行数（非换行数）
    foreach (QString str, strs)
    {
        double tempWidth = fm.horizontalAdvance(str) / mPixelPerCentimer; // 字数转行数
        linesCount += (tempWidth+w_width-1) / w_width;
    }
    height = fm.lineSpacing() * linesCount; // 总高度 = 单行高度 × 行数
    return height;
}
