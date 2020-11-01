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
    listWidget->setSpacing(4);

    nameColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/nameColor", QColor(247, 110, 158)));
    msgColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/msgColor", QColor(Qt::white)));
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
            ? QVariant(msgColor).toString()
            : danmaku.getUnameColor();
    QString nameText = "<font color='" + nameColor + "'>"
                       + danmaku.getNickname() + "</font> ";
    QString text = nameText + danmaku.getText();
    QLabel* label = new QLabel(listWidget);
    QPalette pa(label->palette());
    pa.setColor(QPalette::Text, msgColor);
    label->setPalette(pa);
    label->setWordWrap(true);
    label->setAlignment((Qt::Alignment)( (int)Qt::AlignVCenter ));
    QListWidgetItem* item = new QListWidgetItem(listWidget);
    listWidget->addItem(item);
    listWidget->setItemWidget(item, label);
    listWidget->scrollToBottom();

    item->setData(DANMAKU_JSON_ROLE, danmaku.toJson());
    item->setData(DANMAKU_STRING_ROLE, danmaku.toString());
    setItemWidgetText(item);

    // 自动翻译
    if (autoTrans)
    {
        QString msg = danmaku.getText();
        QRegExp riyu("[\u0800-\u4e00]+");
        QRegExp hanyu("([\u1100-\u11ff\uac00-\ud7af\u3130–bai\u318F\u3200–\u32FF\uA960–\uA97F\uD7B0–\uD7FF\uFF00–\uFFEF\\s]+)");
        QRegExp eeyu("[А-Яа-яЁё]+");
        if (msg.indexOf(riyu) != -1
                || msg.indexOf(hanyu) != -1
                || msg.indexOf(eeyu) != -1)
        {
            qDebug() << "检测到外语，自动翻译";
            startTranslate(item);
        }
    }

    // AI回复
    if (aiReply)
    {
        startReply(item);
    }
}

void LiveDanmakuWindow::slotOldLiveDanmakuRemoved(LiveDanmaku danmaku)
{
    auto currentItem = listWidget->currentItem();
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
            if (item == currentItem)
                listWidget->clearSelection();
            break;
        }
    }
}

void LiveDanmakuWindow::setItemWidgetText(QListWidgetItem *item)
{
    auto widget = listWidget->itemWidget(item);
    if (!widget)
        return ;
    auto label = qobject_cast<QLabel*>(widget);
    if (!label)
        return ;

    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum();

    auto danmaku = item ? LiveDanmaku::fromJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
    QString msg = danmaku.getText();
    QString trans = item->data(DANMAKU_TRANS_ROLE).toString();
    QString reply = item->data(DANMAKU_REPLY_ROLE).toString();
    if (msg.isEmpty()) // 翻译没有变化
        return ;

    QString nameColorStr = danmaku.getUnameColor().isEmpty()
            ? QVariant(this->nameColor).toString()
            : danmaku.getUnameColor();
    QString nameText = "<font color='" + nameColorStr + "'>"
                       + danmaku.getNickname() + "</font> ";
    QString text = nameText + danmaku.getText();

    if (!trans.isEmpty() && trans != msg)
    {
        text = text + "（" + trans + "）";
    }

    if (!reply.isEmpty())
    {
        text = text + "<br/>" + "&nbsp;&nbsp;=> " + reply;
    }

    label->setText(text);
    label->adjustSize();
    item->setSizeHint(label->sizeHint());

    if (scrollEnd)
        listWidget->scrollToBottom();
}

void LiveDanmakuWindow::resetItemsTextColor()
{
    for (int i = 0; i < listWidget->count(); i++)
    {
        auto widget = listWidget->itemWidget(listWidget->item(i));
        auto label = qobject_cast<QLabel*>(widget);
        if (!label)
            return ;
        QPalette pa(label->palette());
        pa.setColor(QPalette::Text, msgColor);
        label->setPalette(pa);
    }
}

void LiveDanmakuWindow::resetItemsText()
{
    for (int i = 0; i < listWidget->count(); i++)
    {
        setItemWidgetText(listWidget->item(i));
    }
}

void LiveDanmakuWindow::showMenu()
{
    auto item = listWidget->currentItem();
    auto danmaku = item ? LiveDanmaku::fromJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
    qDebug() << danmaku.toString();
    QString msg = danmaku.getText();

    QMenu* menu = new QMenu(this);
    QAction* actionNameColor = new QAction("昵称颜色", this);
    QAction* actionMsgColor = new QAction("消息颜色", this);
    QAction* actionBgColor = new QAction("背景颜色", this);
    QAction* actionCopy = new QAction("复制", this);
    QAction* actionSearch = new QAction("百度", this);
    QAction* actionTranslate = new QAction("翻译", this);
    QAction* actionReply = new QAction("AI回复", this);
    QAction* actionFreeCopy = new QAction("自由复制", this);

    menu->addAction(actionNameColor);
    menu->addAction(actionMsgColor);
    menu->addAction(actionBgColor);
    menu->addSeparator();
    menu->addAction(actionSearch);
    menu->addAction(actionTranslate);
    menu->addAction(actionReply);
    menu->addSeparator();
    menu->addAction(actionCopy);
    menu->addAction(actionFreeCopy);

    if (!item)
    {
        actionCopy->setEnabled(false);
        actionSearch->setEnabled(false);
        actionTranslate->setEnabled(false);
        actionReply->setEnabled(false);
        actionFreeCopy->setEnabled(false);
    }

    connect(actionNameColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(nameColor, this, "选择昵称颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != nameColor)
        {
            settings.setValue("livedanmakuwindow/nmaeColor", nameColor = c);
            resetItemsText();
        }
    });
    connect(actionMsgColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(msgColor, this, "选择文字颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != msgColor)
        {
            settings.setValue("livedanmakuwindow/msgColor", msgColor = c);
            resetItemsTextColor();
        }
    });
    connect(actionBgColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(bgColor, this, "选择背景颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != bgColor)
        {
            settings.setValue("livedanmakuwindow/bgColor", bgColor = c);
            update();
        }
    });
    connect(actionCopy, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;
        // 复制
        QApplication::clipboard()->setText(msg);
    });
    connect(actionSearch, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;
        // 百度搜索
        QDesktopServices::openUrl(QUrl("https://www.baidu.com/s?wd="+msg));
    });
    connect(actionTranslate, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;
        // 翻译
        startTranslate(item);
    });
    connect(actionReply, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;
        // 翻译
        startReply(item);
    });
    connect(actionFreeCopy, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;
        // 自由复制
        FreeCopyEdit* edit = new FreeCopyEdit(listWidget);
        QRect rect = listWidget->visualItemRect(item);
        edit->setGeometry(rect);
        edit->setText(msg);
        edit->show();
        edit->setFocus();
    });

    menu->exec(QCursor::pos());

    menu->deleteLater();
    actionMsgColor->deleteLater();
    actionBgColor->deleteLater();
    actionCopy->deleteLater();
    actionSearch->deleteLater();
    actionTranslate->deleteLater();
    actionReply->deleteLater();
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
    if (msg.isEmpty())
        return ;
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

        if (!isItemExist(item))
            return ;

        qDebug() << "翻译：" << msg << " => " << trans;
        item->setData(DANMAKU_TRANS_ROLE, trans);
        setItemWidgetText(item);
    });
}

void LiveDanmakuWindow::setAIReply(bool reply)
{
    this->aiReply = reply;
}

void LiveDanmakuWindow::startReply(QListWidgetItem *item)
{
    auto danmaku = LiveDanmaku::fromJson(item->data(DANMAKU_JSON_ROLE).toJsonObject());
    QString msg = danmaku.getText();
    if (msg.isEmpty())
        return ;
    // 参数信息
    QString url = "https://api.ai.qq.com/fcgi-bin/nlp/nlp_textchat";
    QString nonce_str = "fa577ce340859f9fe";
    QStringList params{"app_id", "2159207490",
                       "nonce_str", nonce_str,
                "question", msg,
                "session", QString::number(danmaku.getUid()),
                "time_stamp", QString::number(QDateTime::currentSecsSinceEpoch()),
                      };

    // 接口鉴权
    QString pinjie;
    for (int i = 0; i < params.size()-1; i+=2)
        if (!params.at(i+1).isEmpty())
            pinjie += params.at(i) + "=" + QUrl::toPercentEncoding(params.at(i+1)) + "&";
    QString appkey = "sTuC8iS3R9yLNbL9";
    pinjie += "app_key="+appkey;

    QString sign = QString(QCryptographicHash::hash(pinjie.toLocal8Bit(), QCryptographicHash::Md5).toHex().data()).toUpper();
    params << "sign" << sign;
//    qDebug() << pinjie << sign;

    // 获取信息
    connect(new NetUtil(url, params), &NetUtil::finished, this, [=](QString result){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(result.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }

        QJsonObject json = document.object();
        if (json.value("ret").toInt() != 0)
        {
            qDebug() << json.value("msg").toString();
            return ;
        }

        QString answer = json.value("data").toObject().value("answer").toString();

        if (!isItemExist(item))
            return ;

        qDebug() << "回复：" << msg << " => " << answer;
        item->setData(DANMAKU_REPLY_ROLE, answer);
        setItemWidgetText(item);
    });
}

bool LiveDanmakuWindow::isItemExist(QListWidgetItem *item)
{
    if (listWidget->currentItem() == item)
        return true;
    // 已经不是当前item，或许已经删除了
    int size = listWidget->count();
    for (int i = 0; i < size; i++)
        if (listWidget->item(i) == item)
            return true;
    return false;
}
