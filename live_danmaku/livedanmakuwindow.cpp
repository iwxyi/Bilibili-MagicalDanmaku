#include "livedanmakuwindow.h"

LiveDanmakuWindow::LiveDanmakuWindow(QWidget *parent) : QWidget(nullptr)
{
    this->setWindowTitle("实时弹幕");
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);      //设置为无边框置顶窗口
    this->setMinimumSize(45,45);                        //设置最小尺寸
    this->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明

    QFontMetrics fm(this->font());
    fontHeight = fm.height();
    lineSpacing = fm.lineSpacing();

    listWidget = new QListWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(listWidget);

    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showMenu()));
    listWidget->setStyleSheet("QListWidget{ background: transparent; border: none; }");
    listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setWordWrap(true);

    fgColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/fgColor", QColor(Qt::white)));
    bgColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/bgColor", QColor(0x88, 0x88, 0x88, 0x32)));
}

void LiveDanmakuWindow::showEvent(QShowEvent *event)
{
    restoreGeometry(settings.value("livedanmakuwindow/geometry").toByteArray());
}

void LiveDanmakuWindow::hideEvent(QHideEvent *event)
{
    settings.setValue("livedanmakuwindow/geometry", this->saveGeometry());
}

bool LiveDanmakuWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    MSG* msg = static_cast<MSG*>(message);
    switch(msg->message)
    {
    case WM_NCHITTEST:
        const auto ratio = devicePixelRatioF(); // 解决4K下的问题
        int xPos = static_cast<int>(GET_X_LPARAM(msg->lParam) / ratio - this->frameGeometry().x());
        int yPos = static_cast<int>(GET_Y_LPARAM(msg->lParam) / ratio - this->frameGeometry().y());
        if(xPos < boundaryWidth &&yPos<boundaryWidth)                    //左上角
            *result = HTTOPLEFT;
        else if(xPos>=width()-boundaryWidth&&yPos<boundaryWidth)          //右上角
            *result = HTTOPRIGHT;
        else if(xPos<boundaryWidth&&yPos>=height()-boundaryWidth)         //左下角
            *result = HTBOTTOMLEFT;
        else if(xPos>=width()-boundaryWidth&&yPos>=height()-boundaryWidth)//右下角
            *result = HTBOTTOMRIGHT;
        else if(xPos < boundaryWidth)                                     //左边
            *result =  HTLEFT;
        else if(xPos>=width()-boundaryWidth)                              //右边
            *result = HTRIGHT;
        /*else if(yPos<boundaryWidth)                                       //上边
            *result = HTTOP;*/
        else if(yPos>=height()-boundaryWidth)                             //下边
            *result = HTBOTTOM;
        else              //其他部分不做处理，返回false，留给其他事件处理器处理
           return false;
        return true;
    }
    return false;         //此处返回false，留给其他事件处理器处理
}

void LiveDanmakuWindow::mousePressEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton)
    {
        pressPos = e->pos();
    }
}
void LiveDanmakuWindow::mouseMoveEvent(QMouseEvent *e)
{
    if(e->buttons()&Qt::LeftButton)
    {
        move(QCursor::pos() - pressPos);
    }
}

void LiveDanmakuWindow::resizeEvent(QResizeEvent *)
{

}

void LiveDanmakuWindow::paintEvent(QPaintEvent *)
{
    QColor c(30, 144, 255, 192);
    int penW = boundaryShowed;
    QPainter painter(this);

    // 绘制用来看的边框
    painter.setPen(QPen(c, penW/2, Qt::PenStyle::DashDotLine));
    painter.fillRect(rect(), bgColor);
}

void LiveDanmakuWindow::slotNewLiveDanmaku(LiveDanmaku danmaku)
{
    QString text = "<font color='"+QVariant(danmaku.getUnameColor().isEmpty()
                                            ? fgColor : danmaku.getUnameColor()).toString()+"'>"
                   + danmaku.getNickname() + "</font> " + danmaku.getText();
    QLabel* label = new QLabel(text, listWidget);
    label->setWordWrap(true);
    label->setAlignment((Qt::Alignment)( (int)Qt::AlignVCenter ));
    label->adjustSize();
    QListWidgetItem* item = new QListWidgetItem(listWidget);
    listWidget->addItem(item);
    listWidget->setItemWidget(item, label);
    item->setSizeHint(label->sizeHint());
    listWidget->scrollToBottom();
}

void LiveDanmakuWindow::showMenu()
{
    QMenu* menu = new QMenu(this);
    QAction* actionFgColor = new QAction("文字颜色", this);
    QAction* actionBgColor = new QAction("背景颜色", this);
    menu->addAction(actionFgColor);
    menu->addAction(actionBgColor);
    connect(actionFgColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(fgColor, this, "选择文字颜色", QColorDialog::ShowAlphaChannel);
        if (c != fgColor)
        {
            settings.setValue("livedanmakuwindow/fgColor", fgColor = c);
            update();
        }
    });
    connect(actionBgColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(bgColor, this, "选择背景颜色", QColorDialog::ShowAlphaChannel);
        if (c != bgColor)
        {
            settings.setValue("livedanmakuwindow/bgColor", bgColor = c);
            update();
        }
    });
    menu->exec(QCursor::pos());
}
