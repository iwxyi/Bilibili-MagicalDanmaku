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
    QString nameColor = danmaku.getUnameColor().isEmpty()
            ? QVariant(fgColor).toString()
            : danmaku.getUnameColor();
    QString nameText = "<font color='" + nameColor + "'>"
                       + danmaku.getNickname() + "</font> ";
    QString text = nameText + danmaku.getText();
    QLabel* label = new QLabel(text, listWidget);
    QPalette pa(label->palette());
    pa.setColor(QPalette::Text, fgColor);
    label->setPalette(pa);
    label->setWordWrap(true);
    label->setAlignment((Qt::Alignment)( (int)Qt::AlignVCenter ));
    label->adjustSize();
    QListWidgetItem* item = new QListWidgetItem(listWidget);
    listWidget->addItem(item);
    listWidget->setItemWidget(item, label);
    item->setSizeHint(label->sizeHint());
    listWidget->scrollToBottom();

    item->setData(DANMAKU_JSON_ROLE, danmaku.toJson());
    item->setData(DANMAKU_STRING_ROLE, danmaku.toString());

    // 自动翻译
    if (autoTrans)
    {
        QRegExp re("[\u0800-\u4e00]+");
        if (danmaku.getText().indexOf(re) != -1)
        {
            qDebug() << "检测到外语，自动翻译";
            startTranslate(item);
        }
    }
}

void LiveDanmakuWindow::slotOldLiveDanmakuRemoved(LiveDanmaku danmaku)
{
    QString s = danmaku.toString();
    for (int i = 0; i < listWidget->count(); i++)
    {
        if (listWidget->item(i)->data(DANMAKU_STRING_ROLE).toString() == s)
        {
            auto item = listWidget->item(i);
            auto widget = listWidget->itemWidget(item);
            listWidget->removeItemWidget(item);
            listWidget->takeItem(i);
            widget->deleteLater();
            break;
        }
    }
}

void LiveDanmakuWindow::appendItemText(QListWidgetItem *item, QString text)
{
    auto widget = listWidget->itemWidget(item);
    if (!widget)
        return ;
    auto label = qobject_cast<QLabel*>(widget);
    if (!label)
        return ;

    auto danmaku = item ? LiveDanmaku::fromJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
    QString nameColor = danmaku.getUnameColor().isEmpty()
            ? QVariant(fgColor).toString()
            : danmaku.getUnameColor();
    QString nameText = "<font color='" + nameColor + "'>"
                       + danmaku.getNickname() + "</font> ";
    text = nameText + danmaku.getText() + "（" +text + "）";
    label->setText(text);
    label->adjustSize();
    item->setSizeHint(label->sizeHint());
}

void LiveDanmakuWindow::showMenu()
{
    auto item = listWidget->currentItem();
    auto danmaku = item ? LiveDanmaku::fromJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
    qDebug() << danmaku.toString();
    QString msg = danmaku.getText();

    QMenu* menu = new QMenu(this);
    QAction* actionFgColor = new QAction("文字颜色", this);
    QAction* actionBgColor = new QAction("背景颜色", this);
    QAction* actionCopy = new QAction("复制弹幕", this);
    QAction* actionSearch = new QAction("百度搜索", this);
    QAction* actionTranslate = new QAction("翻译", this);
    QAction* actionFreeCopy = new QAction("自由复制", this);

    menu->addAction(actionFgColor);
    menu->addAction(actionBgColor);
    menu->addSeparator();
    menu->addAction(actionCopy);
    menu->addAction(actionSearch);
    menu->addAction(actionTranslate);
    menu->addAction(actionFreeCopy);

    if (!item)
    {
        actionCopy->setEnabled(false);
        actionSearch->setEnabled(false);
        actionTranslate->setEnabled(false);
        actionFreeCopy->setEnabled(false);
    }

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
    connect(actionCopy, &QAction::triggered, this, [=]{
        QApplication::clipboard()->setText(msg);
    });
    connect(actionSearch, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://www.baidu.com/s?wd="+msg));
    });
    connect(actionTranslate, &QAction::triggered, this, [=]{
        startTranslate(item);
    });
    connect(actionFreeCopy, &QAction::triggered, this, [=]{

    });

    menu->exec(QCursor::pos());

    menu->deleteLater();
    actionFgColor->deleteLater();
    actionBgColor->deleteLater();
    actionCopy->deleteLater();
    actionSearch->deleteLater();
    actionTranslate->deleteLater();
    actionFreeCopy->deleteLater();
}

void LiveDanmakuWindow::setAutoTranslate(bool trans)
{
    this->autoTrans = trans;
}

void LiveDanmakuWindow::startTranslate(QListWidgetItem *item)
{
    auto danmaku = LiveDanmaku::fromJson(item->data(DANMAKU_JSON_ROLE).toJsonObject());
    QString msg = danmaku.getText();
    QString url = "http://translate.google.cn/translate_a/single?client=gtx&dt=t&dj=1&ie=UTF-8&sl=auto&tl=zh_cn&q="+msg;
    connect(new NetUtil(url), &NetUtil::finished, this, [=](QString result){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(result.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }

        QJsonObject json = document.object();
        auto sentences = json.value("sentences").toArray();
        if (!sentences.size())
            return ;
        auto trans = sentences.first().toObject().value("trans").toString();
        if (trans.isEmpty())
            return ;
        if (listWidget->currentItem() != item) // 已经不是当前item，或许已经删除了
        {
            // 判断item存不存在
            int size = listWidget->count();
            bool find = false;
            for (int i = 0; i < size; i++)
                if (listWidget->item(i) == item)
                {
                    find = true;
                    break;
                }
            if (!find) // 已经不存在了
                return ;
        }

        qDebug() << "翻译：" << msg << " => " << trans;
        try {
            appendItemText(item, trans);
        } catch (...) {

        }
    });
}
