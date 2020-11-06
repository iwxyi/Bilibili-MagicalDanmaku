#include "livedanmakuwindow.h"

LiveDanmakuWindow::LiveDanmakuWindow(QWidget *parent) : QWidget(nullptr), settings("settings.ini", QSettings::Format::IniFormat)
{
    this->setWindowTitle("实时弹幕");
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);      //设置为无边框置顶窗口
    this->setMinimumSize(45,45);                        //设置最小尺寸
    this->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明

    QFontMetrics fm(this->font());
    fontHeight = fm.height();
    lineSpacing = fm.lineSpacing();

    listWidget = new QListWidget(this);
    lineEdit = new TransparentEdit(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(listWidget);
    layout->addWidget(lineEdit);

    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showMenu()));
    listWidget->setStyleSheet("QListWidget{ background: transparent; border: none; }");
    listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidget->setWordWrap(true);
    listWidget->setSpacing(0);
    listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // 发送消息后返回原窗口
    auto returnPrevWindow = [=]{
        // 如果单次显示了输入框，则退出时隐藏
        bool showEdit = settings.value("livedanmakuwindow/sendEdit", false).toBool();
        if (!showEdit)
            lineEdit->hide();
#ifdef Q_OS_WIN32
        if (this->prevWindow)
            SwitchToThisWindow(prevWindow, true);
        prevWindow = nullptr;
#endif
    };
    lineEdit->setPlaceholderText("回车发送消息");
    connect(lineEdit, &QLineEdit::returnPressed, this, [=]{
        QString text = lineEdit->text();
        if (!text.trimmed().isEmpty())
        {
            emit signalSendMsg(text.trimmed());
            lineEdit->clear();
            if (settings.value("livedanmakuwindow/sendOnce", false).toBool())
                returnPrevWindow();
        }
    });
    if (!settings.value("livedanmakuwindow/sendEdit", false).toBool())
        lineEdit->hide();

    editShortcut = new QxtGlobalShortcut(this);
    editShortcut->setShortcut(QKeySequence("shift+alt+d"));
    connect(lineEdit, &TransparentEdit::signalESC, this, [=]{
        returnPrevWindow();
    });
    connect(editShortcut, &QxtGlobalShortcut::activated, this, [=]() {
        if (this->isActiveWindow() && lineEdit->hasFocus())
        {
            returnPrevWindow();
        }
        else // 激活并聚焦
        {
#ifdef Q_OS_WIN32
            prevWindow = GetForegroundWindow();
#endif

            this->activateWindow();
            if (lineEdit->isHidden())
                lineEdit->show();
            lineEdit->setFocus();
        }
    });
    if (!settings.value("livedanmakuwindow/sendEditShortcut", false).toBool())
        editShortcut->setEnabled(false);

    // 颜色
    nameColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/nameColor", QColor(247, 110, 158)));
    msgColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/msgColor", QColor(Qt::white)));
    bgColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/bgColor", QColor(0x88, 0x88, 0x88, 0x32)));
    hlColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/hlColor", QColor(255, 0, 0)));

    // 特别关心
    QStringList usersS = settings.value("danmaku/careUsers", "20285041").toString().split(";", QString::SkipEmptyParts);
    foreach (QString s, usersS)
    {
        careUsers.append(s.toLongLong());
    }

    // 本地昵称
    QStringList namePares = settings.value("danmaku/localNicknames").toString().split(";", QString::SkipEmptyParts);
    foreach (QString pare, namePares)
    {
        QStringList sl = pare.split("=>");
        if (sl.size() < 2)
            continue;

        localNicknames.insert(sl.at(0).toLongLong(), sl.at(1));
    }
}

QString LiveDanmakuWindow::getLocalNickname(qint64 uid)
{
    if (localNicknames.contains(uid))
        return localNicknames.value(uid);
    return "";
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
    int w = listWidget->contentsRect().width();
    for (int i = 0; i < listWidget->count(); i++)
    {
        auto item = listWidget->item(i);
        auto widget = listWidget->itemWidget(item);
        if (!widget)
            continue;
        widget->setFixedWidth(w);
        widget->adjustSize();
        widget->resize(widget->width(), widget->height() + listItemSpacing);
        item->setSizeHint(widget->size());
    }
}

void LiveDanmakuWindow::paintEvent(QPaintEvent *)
{
    QColor c(30, 144, 255, 192);
    int penW = boundaryShowed;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 绘制背景
    QPainterPath path;
    path.addRoundedRect(rect(), 5, 5);
    painter.fillPath(path, bgColor);
}

void LiveDanmakuWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        listWidget->clearSelection();
    }

    return QWidget::keyPressEvent(event);
}

void LiveDanmakuWindow::slotNewLiveDanmaku(LiveDanmaku danmaku)
{
    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum()-lineEdit->height()*2;

    QString nameColor = danmaku.getUnameColor().isEmpty()
            ? QVariant(msgColor).toString()
            : danmaku.getUnameColor();
    QString nameText = "<font color='" + nameColor + "'>"
                       + danmaku.getNickname() + "</font> ";
    QString text = nameText + danmaku.getText();
    QLabel* label = new QLabel(listWidget);
    label->setWordWrap(true);
    label->setAlignment((Qt::Alignment)( (int)Qt::AlignVCenter ));
    label->setFixedWidth(listWidget->contentsRect().width());
    QListWidgetItem* item = new QListWidgetItem(listWidget);
    listWidget->addItem(item);
    listWidget->setItemWidget(item, label);

    // 设置数据
    item->setData(DANMAKU_JSON_ROLE, danmaku.toJson());
    item->setData(DANMAKU_STRING_ROLE, danmaku.toString());
    setItemWidgetText(item);

    // 开启动画
    QPropertyAnimation* ani = new QPropertyAnimation(label, "size");
    ani->setStartValue(QSize(label->width(), 0));
    ani->setEndValue(label->size());
    ani->setDuration(300);
    ani->setEasingCurve(QEasingCurve::InOutQuad);
    label->resize(label->width(), 1);
    item->setSizeHint(label->size());
    connect(ani, &QPropertyAnimation::valueChanged, label, [=](const QVariant &val){
        item->setSizeHint(val.toSize());
        if (scrollEnd)
            listWidget->scrollToBottom();
    });
    connect(ani, &QPropertyAnimation::finished, label, [=]{
        if (scrollEnd)
            listWidget->scrollToBottom();
    });
    ani->start();

    // 各种额外功能
    QRegExp replyRe("点歌|谢|欢迎");
    if (danmaku.getMsgType() == MSG_DANMAKU)
    {
        if (!noReplyStrings.contains(danmaku.getText()))
        {
            QRegExp hei("[（）\\(\\)~]"); // 带有特殊字符的黑名单

            // 自动翻译
            if (autoTrans)
            {
                QString msg = danmaku.getText();
                QRegExp riyu("[\u0800-\u4e00]+");
                QRegExp hanyu("([\u1100-\u11ff\uac00-\ud7af\u3130–bai\u318F\u3200–\u32FF\uA960–\uA97F\uD7B0–\uD7FF\uFF00–\uFFEF\\s]+)");
                QRegExp eeyu("[А-Яа-яЁё]+");
                if (msg.indexOf(hei) == -1)
                {
                    if (msg.indexOf(riyu) != -1
                            || msg.indexOf(hanyu) != -1
                            || msg.indexOf(eeyu) != -1)
                    {
                        qDebug() << "检测到外语，自动翻译";
                        startTranslate(item);
                    }
                }
            }

            // AI回复
            if (aiReply)
            {
                startReply(item);
            }
        }
    }
    else if (danmaku.getMsgType() == MSG_DIANGE)
    {
        highlightItemText(item, true);
    }
    else if (danmaku.getMsgType() == MSG_ATTENTION)
    {
        if (danmaku.isAttention())
            highlightItemText(item, true);
    }
    else if (danmaku.getMsgType() == MSG_WELCOME_GUARD)
    {
        highlightItemText(item, true);
    }
    noReplyStrings.clear();

    if (scrollEnd)
    {
        listWidget->scrollToBottom();
    }
}

void LiveDanmakuWindow::slotOldLiveDanmakuRemoved(LiveDanmaku danmaku)
{
    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum()-lineEdit->height()*2;

    auto currentItem = listWidget->currentItem();
    QString s = danmaku.toString();
    for (int i = 0; i < listWidget->count(); i++)
    {
        if (listWidget->item(i)->data(DANMAKU_STRING_ROLE).toString() == s)
        {
            auto item = listWidget->item(i);
            auto widget = listWidget->itemWidget(item);

            QPropertyAnimation* ani = new QPropertyAnimation(widget, "size");
            ani->setStartValue(widget->size());
            ani->setEndValue(QSize(widget->width(), 0));
            ani->setDuration(300);
            ani->setEasingCurve(QEasingCurve::InOutQuad);
            connect(ani, &QPropertyAnimation::valueChanged, widget, [=](const QVariant &val){
                item->setSizeHint(val.toSize());
            });
            connect(ani, &QPropertyAnimation::finished, widget, [=]{
                bool same = (item == listWidget->currentItem());
                bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
                        >= listWidget->verticalScrollBar()->maximum()-lineEdit->height()*2;
                listWidget->removeItemWidget(item);
                listWidget->takeItem(i);
                widget->deleteLater();
                if (same)
                    listWidget->clearSelection();
                if (scrollEnd)
                    listWidget->scrollToBottom();
            });
            ani->start();
            /*listWidget->removeItemWidget(item);
            listWidget->takeItem(i);
            widget->deleteLater();*/
            if (item == currentItem)
                listWidget->clearSelection();
            break;
        }
    }

//    if (scrollEnd)
//        listWidget->scrollToBottom();
}

void LiveDanmakuWindow::setItemWidgetText(QListWidgetItem *item)
{
    auto widget = listWidget->itemWidget(item);
    if (!widget)
        return ;
    auto label = qobject_cast<QLabel*>(widget);
    if (!label)
        return ;
    auto danmaku = item ? LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
    QPalette pa(label->palette());
    QColor c = msgColor;
    if (careUsers.contains(danmaku.getUid()))
        c = hlColor;
    pa.setColor(QPalette::Text, c);
    label->setPalette(pa);

    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum() -lineEdit->height()*2;

    QString msg = danmaku.getText();
    QString trans = item->data(DANMAKU_TRANS_ROLE).toString();
    QString reply = item->data(DANMAKU_REPLY_ROLE).toString();

    // 设置文字
    QString nameColorStr = danmaku.getUnameColor().isEmpty()
            ? QVariant(this->nameColor).toString()
            : danmaku.getUnameColor();
    QString nameText = "<font color='" + nameColorStr + "'>"
                       + danmaku.getNickname() + "</font> ";

    QString text;
    MessageType msgType = danmaku.getMsgType();
    if (msgType == MSG_DANMAKU)
    {
        text = nameText + danmaku.getText();

        if (!trans.isEmpty() && trans != msg)
        {
            text = text + "（" + trans + "）";
        }

        if (!reply.isEmpty())
        {
            text = text + "<br/>" + "&nbsp;&nbsp;=> " + reply;
        }
    }
    else if (msgType == MSG_GIFT)
    {
        text = QString("<font color='gray'>[送礼]</font> %1 赠送 %2×%3")
                .arg(nameText)
                .arg(danmaku.getGiftName())
                .arg(danmaku.getNumber());
    }
    else if (msgType == MSG_GUARD_BUY)
    {
        text = QString("<font color='gray'>[上船]</font> %1 开通 %2×%3")
                .arg(nameText)
                .arg(danmaku.getGiftName())
                .arg(danmaku.getNumber());
    }
    else if (msgType == MSG_WELCOME)
    {
        if (danmaku.isAdmin())
            text = QString("<font color='gray'>[光临]</font> 舰长 %1")
                    .arg(nameText);
        else
            text = QString("<font color='gray'>[欢迎]</font> %1 进入直播间")
                    .arg(nameText);
    }
    else if (msgType == MSG_DIANGE)
    {
        text = QString("<font color='gray'>[点歌]</font> %1")
                .arg(danmaku.getText());
    }
    else if (msgType == MSG_FANS)
    {
        text = QString("<font color='gray'>[粉丝]</font> 粉丝数：%1%3, 粉丝团：%2%4")
                .arg(danmaku.getFans())
                .arg(danmaku.getFansClub())
                .arg(danmaku.getDeltaFans()
                     ? QString("(%1%2)")
                       .arg(danmaku.getDeltaFans() > 0 ? "+" : "")
                       .arg(danmaku.getDeltaFans())
                     : "")
                .arg(danmaku.getDeltaFansClub()
                     ? QString("(%1%2)")
                       .arg(danmaku.getDeltaFansClub() > 0 ? "+" : "")
                       .arg(danmaku.getDeltaFansClub())
                     : "");
    }
    else if (msgType == MSG_ATTENTION)
    {
        qint64 second = QDateTime::currentSecsSinceEpoch() - danmaku.getTimeline().toSecsSinceEpoch();
        text = QString("<font color='gray'>[关注]</font> %1 %2 <font color='gray'>%3s前</font>")
                .arg(nameText)
                .arg(danmaku.isAttention() ? "关注了主播" : "取消关注主播")
                .arg(second);
    }

    label->setText(text);
    label->adjustSize();
    label->resize(label->width(), label->height() + listItemSpacing);
    item->setSizeHint(label->size());

    if (scrollEnd)
        listWidget->scrollToBottom();
}

void LiveDanmakuWindow::highlightItemText(QListWidgetItem *item, bool recover)
{
    auto widget = listWidget->itemWidget(item);
    auto label = qobject_cast<QLabel*>(widget);
    if (!label)
        return ;
    QPalette pa(label->palette());
    pa.setColor(QPalette::Text, hlColor);
    label->setPalette(pa);
    item->setData(DANMAKU_HIGHLIGHT_ROLE, true);

    // 定时恢复原样
    if (!recover)
        return ;
    QTimer::singleShot(3000, [=]{
        // 怕到时候没了
       if (!isItemExist(item))
       {
           qDebug() << "高亮项已经没了";
           return ;
       }

        // 重新设置内容
        item->setData(DANMAKU_HIGHLIGHT_ROLE, false);
        setItemWidgetText(item);
    });
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
    auto danmaku = item ? LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
    qDebug() << "菜单信息：" << danmaku.toString();
    QString msg = danmaku.getText();

    QMenu* menu = new QMenu(this);
    QAction* actionNameColor = new QAction("昵称颜色", this);
    QAction* actionMsgColor = new QAction("消息颜色", this);
    QAction* actionBgColor = new QAction("背景颜色", this);
    QAction* actionHlColor = new QAction("高亮颜色", this);
    QAction* actionAddCare = new QAction("添加特别关心", this);
    QAction* actionSetName = new QAction("设置专属昵称", this);
    QAction* actionSearch = new QAction("百度", this);
    QAction* actionTranslate = new QAction("翻译", this);
    QAction* actionReply = new QAction("AI回复", this);
    QAction* actionCopy = new QAction("复制", this);
    QAction* actionFreeCopy = new QAction("自由复制", this);
    QAction* actionSendMsg = new QAction("发送框", this);
    QAction* actionDialogSend = new QAction("快速触发", this);
    QAction* actionSendOnce = new QAction("单次发送", this);

    actionSendMsg->setCheckable(true);
    actionSendMsg->setChecked(!lineEdit->isHidden());
    actionDialogSend->setToolTip("shift+alt+D 触发编辑框，输入后ESC返回原先窗口");
    actionDialogSend->setCheckable(true);
    actionDialogSend->setChecked(settings.value("livedanmakuwindow/sendEditShortcut", false).toBool());
    actionSendOnce->setToolTip("发送后，返回原窗口（如果有）");
    actionSendOnce->setCheckable(true);
    actionSendOnce->setChecked(settings.value("livedanmakuwindow/sendOnce", false).toBool());

    qint64 uid = danmaku.getUid();
    if (uid != 0)
    {
        if (careUsers.contains(uid))
            actionAddCare->setText("移除特别关心");
        if (localNicknames.contains(uid))
            actionSetName->setText("专属昵称：" + localNicknames.value(uid));
    }

    menu->addAction(actionNameColor);
    menu->addAction(actionMsgColor);
    menu->addAction(actionBgColor);
    menu->addAction(actionHlColor);
    menu->addSeparator();
    menu->addAction(actionAddCare);
    menu->addAction(actionSetName);
    menu->addSeparator();
    menu->addAction(actionSearch);
    menu->addAction(actionTranslate);
    menu->addAction(actionReply);
    menu->addSeparator();
    menu->addAction(actionCopy);
    menu->addAction(actionFreeCopy);
    menu->addSeparator();
    menu->addAction(actionSendMsg);
    menu->addAction(actionDialogSend);
    menu->addAction(actionSendOnce);

    if (!item)
    {
        actionAddCare->setEnabled(false);
        actionSetName->setEnabled(false);
        actionCopy->setEnabled(false);
        actionSearch->setEnabled(false);
        actionTranslate->setEnabled(false);
        actionReply->setEnabled(false);
        actionFreeCopy->setEnabled(false);
    }
    else if (!uid)
    {
        actionAddCare->setEnabled(false);
        actionSetName->setEnabled(false);
    }

    connect(actionNameColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(nameColor, this, "选择昵称颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != nameColor)
        {
            settings.setValue("livedanmakuwindow/nameColor", nameColor = c);
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
    connect(actionHlColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(hlColor, this, "选择高亮颜色（实时提示、特别关心）", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != hlColor)
        {
            settings.setValue("livedanmakuwindow/hlColor", hlColor = c);
            resetItemsTextColor();
        }
    });
    connect(actionAddCare, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;

        if (careUsers.contains(danmaku.getUid())) // 已存在，移除
        {
            careUsers.removeOne(danmaku.getUid());
            setItemWidgetText(item);
        }
        else // 添加特别关心
        {
            careUsers.append(danmaku.getUid());
            highlightItemText(item);
        }

        // 保存特别关心
        QStringList ress;
        foreach (qint64 uid, careUsers)
            ress << QString::number(uid);
        settings.setValue("danmaku/careUsers", ress.join(";"));
    });
    connect(actionSetName, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;

        // 设置昵称
        bool ok = false;
        QString name;
        if (localNicknames.contains(uid))
            name = localNicknames.value(uid);
        else
            name = danmaku.getNickname();
        name = QInputDialog::getText(this, "专属昵称", "设置 " + danmaku.getNickname() + " 的专属昵称，影响对应弹幕\n清空则取消专属，还原账号昵称",
                                     QLineEdit::Normal, name, &ok);
        if (!ok)
            return ;
        if (name.isEmpty())
        {
            if (localNicknames.contains(uid))
                localNicknames.remove(uid);
        }
        else
        {
            localNicknames[uid] = name;
        }

        // 保存本地昵称
        QStringList ress;
        auto it = localNicknames.begin();
        while (it != localNicknames.end())
        {
            ress << QString("%1=>%2").arg(it.key()).arg(it.value());
            it++;
        }
        settings.setValue("danmaku/localNicknames", ress.join(";"));
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
    connect(actionSendMsg, &QAction::triggered, this, [=]{
        if (lineEdit->isHidden())
            lineEdit->show();
        else
            lineEdit->hide();
        settings.setValue("livedanmakuwindow/sendEdit", !lineEdit->isHidden());
    });
    connect(actionDialogSend, &QAction::triggered, this, [=]{
        bool enable = !settings.value("livedanmakuwindow/sendEditShortcut", false).toBool();
        settings.setValue("livedanmakuwindow/sendEditShortcut", enable);
        editShortcut->setEnabled(enable);
    });
    connect(actionSendOnce, &QAction::triggered, this, [=]{
        bool enable = !settings.value("livedanmakuwindow/sendOnce", false).toBool();
        settings.setValue("livedanmakuwindow/sendOnce", enable);
    });

    menu->exec(QCursor::pos());

    menu->deleteLater();
    actionMsgColor->deleteLater();
    actionBgColor->deleteLater();
    actionHlColor->deleteLater();
    actionAddCare->deleteLater();
    actionSetName->deleteLater();
    actionCopy->deleteLater();
    actionSearch->deleteLater();
    actionTranslate->deleteLater();
    actionReply->deleteLater();
    actionFreeCopy->deleteLater();
    actionSendMsg->deleteLater();
}

void LiveDanmakuWindow::setAutoTranslate(bool trans)
{
    this->autoTrans = trans;
}

void LiveDanmakuWindow::startTranslate(QListWidgetItem *item)
{
    auto danmaku = LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject());
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

        adjustItemTextDynamic(item);
    });
}

void LiveDanmakuWindow::setAIReply(bool reply)
{
    this->aiReply = reply;
}

void LiveDanmakuWindow::startReply(QListWidgetItem *item)
{
    auto danmaku = LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject());
    QString msg = danmaku.getText();
    if (msg.isEmpty())
        return ;
    // 优化消息文本
    msg.replace(QRegularExpression("\\s+"), "，");

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

        adjustItemTextDynamic(item);
    });
}

void LiveDanmakuWindow::addNoReply(QString text)
{
    noReplyStrings.append(text);
}

void LiveDanmakuWindow::setListWidgetItemSpacing(int x)
{
    listWidget->setSpacing(x);
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

/**
 * 外语翻译、AI回复等动态修改文字的，重新设置动画
 * 否则尺寸为没有修改之前的，导致显示不全
 */
void LiveDanmakuWindow::adjustItemTextDynamic(QListWidgetItem *item)
{
    auto widget = listWidget->itemWidget(item);
    if (!widget)
        return ;
    auto label = qobject_cast<QLabel*>(widget);
    if (!label)
        return ;
    int ht = label->height();
    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum() -lineEdit->height()*2;

    setItemWidgetText(item);

    QPropertyAnimation* ani = new QPropertyAnimation(label, "size");
    ani->setStartValue(QSize(label->width(), ht));
    ani->setEndValue(label->size());
    ani->setDuration(200);
    ani->setEasingCurve(QEasingCurve::InOutQuad);
    label->resize(label->width(), ht);
    item->setSizeHint(label->size());
    connect(ani, &QPropertyAnimation::valueChanged, label, [=](const QVariant &val){
        item->setSizeHint(val.toSize());
        if (scrollEnd)
            listWidget->scrollToBottom();
    });
    connect(ani, &QPropertyAnimation::finished, label, [=]{
        if (scrollEnd)
            listWidget->scrollToBottom();
    });
    ani->start();
}
