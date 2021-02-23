#include <QFileDialog>
#include "livedanmakuwindow.h"
#include "facilemenu.h"

QT_BEGIN_NAMESPACE
    extern Q_WIDGETS_EXPORT void qt_blurImage( QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

LiveDanmakuWindow::LiveDanmakuWindow(QSettings& st, QWidget *parent)
    : QWidget(nullptr), settings(st)
{
    this->setWindowTitle("实时弹幕");
    this->setMinimumSize(45,45);                        //设置最小尺寸
    if (settings.value("livedanmakuwindow/jiWindow", false).toBool())
    {
        this->setWindowFlags(Qt::FramelessWindowHint);
        this->setAttribute(Qt::WA_TranslucentBackground, false);
    }
    else
    {
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);      //设置为无边框置顶窗口
        this->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明
    }
    if (settings.value("livedanmakuwindow/onTop", true).toBool())
    {
        this->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    }

    QFontMetrics fm(this->font());
    fontHeight = fm.height();
    lineSpacing = fm.lineSpacing();
    srand(time(0));

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
    connect(listWidget, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem *item){
        auto danmaku = item ? LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
        qint64 uid = danmaku.getUid();
        if (uid)
        {
            showUserMsgHistory(uid, danmaku.getNickname());
        }
    });

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
    lineEdit->setPlaceholderText("回车发送弹幕");
    lineEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(lineEdit, &QLineEdit::returnPressed, this, [=]{
        QString text = lineEdit->text();
        if (!text.trimmed().isEmpty())
        {
            myPrevSendMsg = text.trimmed();
            emit signalSendMsg(myPrevSendMsg);
            lineEdit->clear();
            if (settings.value("livedanmakuwindow/sendOnce", false).toBool())
                returnPrevWindow();
            lineEdit->setPlaceholderText("");
        }
    });
    connect(lineEdit, &QLineEdit::customContextMenuRequested, this, &LiveDanmakuWindow::showEditMenu);
    if (!settings.value("livedanmakuwindow/sendEdit", false).toBool())
        lineEdit->hide();

#ifndef Q_OS_ANDROID
    editShortcut = new QxtGlobalShortcut(this);
    QString def_key = settings.value("livedanmakuwindow/shortcutKey", "shift+alt+D").toString();
    editShortcut->setShortcut(QKeySequence(def_key));
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
            if (lineEdit->text().isEmpty() && !myPrevSendMsg.isEmpty())
            {
                lineEdit->setText(myPrevSendMsg);
                lineEdit->selectAll();
                myPrevSendMsg = "";
            }
        }
    });
    if (!settings.value("livedanmakuwindow/sendEditShortcut", false).toBool())
        editShortcut->setEnabled(false);
#endif

    // 颜色
    nameColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/nameColor", QColor(247, 110, 158)));
    msgColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/msgColor", QColor(Qt::white)));
    bgColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/bgColor", QColor(0x88, 0x88, 0x88, 0x32)));
    hlColor = qvariant_cast<QColor>(settings.value("livedanmakuwindow/hlColor", QColor(255, 0, 0)));
    QString fontString = settings.value("livedanmakuwindow/font").toString();
    if (!fontString.isEmpty())
        danmakuFont.fromString(fontString);

    // 背景图片
    pictureFilePath = settings.value("livedanmakuwindow/pictureFilePath", "").toString();
    pictureDirPath = settings.value("livedanmakuwindow/pictureDirPath", "").toString();
    pictureAlpha = settings.value("livedanmakuwindow/pictureAlpha", 64).toInt();
    pictureBlur = settings.value("livedanmakuwindow/pictureBlur", 0).toInt();
    switchBgTimer = new QTimer(this);
    int switchInterval = settings.value("livedanmakuwindow/pictureInterval", 20).toInt();
    switchBgTimer->setInterval(switchInterval * 1000);
    connect(switchBgTimer, &QTimer::timeout, this, [=]{
        selectBgPicture();
    });
    aspectRatio = settings.value("livedanmakuwindow/aspectRatio", false).toBool();
    if (!pictureDirPath.isEmpty())
    {
        selectBgPicture();
        switchBgTimer->start();
    }
    else if (!pictureFilePath.isEmpty())
    {
        selectBgPicture();
    }
    update();

    // 忽略的颜色
    ignoreDanmakuColors = settings.value("livedanmakuwindow/ignoreColor").toString().split(";");

    // 模式
    simpleMode = settings.value("livedanmakuwindow/simpleMode", false).toBool();
    chatMode = settings.value("livedanmakuwindow/chatMode", false).toBool();

    statusLabel = new QLabel(this);
    statusLabel->hide();
    QPalette pa;
    pa.setColor(QPalette::Text, msgColor);
    statusLabel->setPalette(pa);
    statusLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(statusLabel, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showPkMenu()));
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
#ifdef Q_OS_WIN32
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
#else
    return QWidget::nativeEvent(eventType, message, result);
#endif
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
        try {
            auto item = listWidget->item(i);
            auto widget = listWidget->itemWidget(item);
            if (!widget) // 正在动画之中，在这一瞬间突然就删除掉了
                continue;
            QHBoxLayout* layout = static_cast<QHBoxLayout*>(widget->layout());
            if (!layout)
                continue;
            auto portrait = layout->itemAt(DANMAKU_WIDGET_PORTRAIT)->widget();
            auto label = layout->itemAt(DANMAKU_WIDGET_LABEL)->widget();
            if (!widget || !label)
                continue;
            widget->setFixedWidth(w);
            label->setFixedWidth(w - (portrait->isHidden() ? 0 : PORTRAIT_SIDE + widget->layout()->spacing()) - widget->layout()->margin()*2);
            label->adjustSize();
            widget->adjustSize();
            widget->resize(widget->sizeHint());
            item->setSizeHint(widget->size());
        } catch (...) {
            qDebug() << "resizeEvent错误";
        }
    }

    if (statusLabel && !statusLabel->isHidden())
        statusLabel->move(width() - statusLabel->width(), 0);

    if (!originPixmap.isNull())
    {
        bgPixmap = originPixmap.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }
}

void LiveDanmakuWindow::paintEvent(QPaintEvent *)
{
    QColor c(30, 144, 255, 192);
    int penW = boundaryShowed;
    Q_UNUSED(penW)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 绘制背景
    QPainterPath path;
    path.addRoundedRect(rect(), 5, 5);
    painter.setClipPath(path);

    painter.fillRect(rect(), bgColor);

    // 绘制背景图片
    if (prevAlpha && !prevPixmap.isNull())
    {
        painter.setOpacity(prevAlpha / 255.0);
        if (!aspectRatio)
        {
            painter.drawPixmap(rect(), prevPixmap);
        }
        else // 绘制中间的图片
        {
            drawPixmapCenter(painter, prevPixmap);
        }
    }
    if (!bgPixmap.isNull())
    {
        painter.setOpacity(bgAlpha / 255.0);
        if (!aspectRatio)
        {
            painter.drawPixmap(rect(), bgPixmap);
        }
        else // 绘制中间的图片
        {
            drawPixmapCenter(painter, bgPixmap);
        }
    }
}

/**
 * 居中绘制图片
 */
void LiveDanmakuWindow::drawPixmapCenter(QPainter &painter, const QPixmap &bgPixmap)
{
    int x = 0, y = 0;
    if (bgPixmap.width() > width())
        x = (bgPixmap.width() - width()) / 2;
    if (bgPixmap.height() > height())
        y = (bgPixmap.height() - height()) / 2;
    painter.drawPixmap(rect(), bgPixmap, QRect(x, y, width(), height()));
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
    if (chatMode) // 聊天模式：只显示弹幕和礼物等
    {
        if (!danmaku.is(MSG_DANMAKU)
                && !danmaku.is(MSG_GIFT)
                && !danmaku.is(MSG_GUARD_BUY)
                && !danmaku.is(MSG_BLOCK))
            return ;
    }

    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum()-lineEdit->height()*2;

    QString nameColor = danmaku.getUnameColor().isEmpty()
            ? QVariant(msgColor).toString()
            : danmaku.getUnameColor();
    QString nameText = "<font color='" + nameColor + "'>"
                       + danmaku.getNickname() + "</font> ";
    QString text = nameText + danmaku.getText();

    QWidget* widget = new QWidget(listWidget);
    PortraitLabel* portrait = new PortraitLabel(PORTRAIT_SIDE, widget);
    QLabel* label = new QLabel(widget);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->addWidget(portrait);
    layout->addWidget(label);
    layout->setMargin(2);
    layout->setAlignment(Qt::AlignLeft);
    widget->setLayout(layout);
    layout->activate();

    label->setWordWrap(true);
    label->setAlignment((Qt::Alignment)( (int)Qt::AlignVCenter ));
    label->setFont(danmakuFont);

    QListWidgetItem* item = new QListWidgetItem(listWidget);
    listWidget->addItem(item);
    listWidget->setItemWidget(item, widget);

    // 设置数据
    item->setData(DANMAKU_JSON_ROLE, danmaku.toJson());
    item->setData(DANMAKU_STRING_ROLE, danmaku.toString());
    if (danmaku.getMsgType() == MSG_DANMAKU && !simpleMode) // 只显示弹幕的数据
    {
        bool hasPortrait = headPortraits.contains(danmaku.getUid());
        if (hasPortrait)
            portrait->setPixmap(headPortraits.value(danmaku.getUid()));
        else
            getUserInfo(danmaku.getUid(), item);
    }
    else
    {
        portrait->setFixedSize(0, 0);
        portrait->hide();
    }
    label->setFixedWidth(listWidget->contentsRect().width() - (portrait->isHidden() ? 0 : PORTRAIT_SIDE + widget->layout()->spacing())- widget->layout()->margin()*2);
    widget->resize(listWidget->contentsRect().width(), 1);
    setItemWidgetText(item); // 会自动调整大小

    // 各种额外功能
    MessageType msgType = danmaku.getMsgType();
    QRegExp replyRe("点歌|谢|欢迎");
    if (msgType == MSG_DANMAKU)
    {
        // 自己的，成功发送了
        if (danmaku.getText() == myPrevSendMsg)
            myPrevSendMsg = "";

        if (!danmaku.isNoReply() && !danmaku.isPkLink())
        {
            QRegExp hei("[（）\\(\\)~]"); // 带有特殊字符的黑名单

            // 自动翻译
            if (autoTrans)
            {
                QString msg = danmaku.getText();
                QRegExp riyu("[\u0800-\u4e00]+");
                QRegExp hanyu("([\u1100-\u11ff\uac00-\ud7af\u3130–bai\u318F\u3200–\u32FF\uA960–\uA97F\uD7B0–\uD7FF\uFF00–\uFFEF\\s]+)");
                QRegExp eeyu("[А-Яа-яЁё]+");
                QRegExp fanti("[\u3400-\u4db5]+");
                if (msg.indexOf(hei) == -1)
                {
                    if (msg.indexOf(riyu) != -1
                            || msg.indexOf(hanyu) != -1
                            || msg.indexOf(eeyu) != -1
                            || msg.indexOf(fanti) != -1)
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
    else if (msgType == MSG_DIANGE
             || msgType == MSG_WELCOME_GUARD
             || msgType == MSG_BLOCK)
    {
        highlightItemText(item, true);
    }
    /*else if (msgType == MSG_ATTENTION)
    {
        if (danmaku.isAttention()
                && (QDateTime::currentSecsSinceEpoch() - danmaku.getTimeline().toSecsSinceEpoch() <= 20)) // 20秒内
            highlightItemText(item, true);
    }*/
    else if (msgType == MSG_GUARD_BUY)
    {
        highlightItemText(item, false);
    }
    else if (msgType == MSG_GIFT)
    {
        if (danmaku.isGoldCoin())
        {
            if (danmaku.getTotalCoin() >= 50000) // 50元以上的，长期高亮
                highlightItemText(item, false);
            else if (danmaku.getTotalCoin() >= 10000) // 10元以上，短暂高亮
                highlightItemText(item, true);
        }
    }

    if (scrollEnd)
    {
        listWidget->scrollToBottom();
    }
}

void LiveDanmakuWindow::slotOldLiveDanmakuRemoved(LiveDanmaku danmaku)
{
    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum()-lineEdit->height();

    auto currentItem = listWidget->currentItem();
    QString s = danmaku.toString();
    for (int i = 0; i < listWidget->count(); i++)
    {
        // qDebug() << listWidget->item(i)->data(DANMAKU_STRING_ROLE).toString() << s;
        if (listWidget->item(i)->data(DANMAKU_STRING_ROLE).toString() == s)
        {
            auto item = listWidget->item(i);
            auto widget = listWidget->itemWidget(item);
            item->setData(DANMAKU_STRING_ROLE, ""); // 清空，免得重复删除到头一个上面

            if (DANMAKU_ANIMATION_ENABLED)
            {
                if (!widget)
                {
                    listWidget->removeItemWidget(item);
                    break;
                }
                QPropertyAnimation* ani = new QPropertyAnimation(widget, "size");
                ani->setStartValue(widget->size());
                ani->setEndValue(QSize(widget->width(), 0));
                ani->setDuration(300);
                ani->setEasingCurve(QEasingCurve::OutQuad);
                connect(ani, &QPropertyAnimation::valueChanged, widget, [=](const QVariant &val){
                    item->setSizeHint(val.toSize());
                });
                connect(ani, &QPropertyAnimation::finished, widget, [=]{
                    bool same = (item == listWidget->currentItem());
                    listWidget->removeItemWidget(item);
                    listWidget->takeItem(listWidget->row(item)); // 行数可能变化，需要重新判断
                    widget->deleteLater();
                    if (same)
                        listWidget->clearSelection();
                    if (scrollEnd)
                        listWidget->scrollToBottom();
                });
                ani->start();
            }
            else
            {
                listWidget->removeItemWidget(item);
                listWidget->takeItem(i);
                widget->deleteLater();
                if (item == currentItem)
                    listWidget->clearSelection();
            }
            return ;
        }
    }
    qDebug() << "忽略没找到的要删除的item" << danmaku.toString();
}

void LiveDanmakuWindow::setItemWidgetText(QListWidgetItem *item)
{
    auto widget = listWidget->itemWidget(item);
    auto label = getItemWidgetLabel(item);
    if (!label)
        return ;
    auto danmaku = item ? LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
    QPalette pa(label->palette());
    QColor c = msgColor;
    if (careUsers.contains(danmaku.getUid()))
        c = hlColor;
    pa.setColor(QPalette::Text, c);
    label->setPalette(pa);
    int oldHeight = widget->height();

    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum() -lineEdit->height()*2;

    QString msg = danmaku.getText();
    QString trans = item->data(DANMAKU_TRANS_ROLE).toString();
    QString reply = item->data(DANMAKU_REPLY_ROLE).toString();

    auto isBlankColor = [=](QString c) -> bool {
        c = c.toLower();
        return c.isEmpty() || c == "#ffffff" || c == "#000000"
                || c == "#ffffffff" || c == "#00000000";
    };

    // 设置文字
    QString nameColorStr = isBlankColor(danmaku.getUnameColor())
            ? QVariant(this->nameColor).toString()
            : danmaku.getUnameColor();
    QString nameText = "<font color='" + nameColorStr + "'>"
                       + danmaku.getNickname() + "</font>";
    if (simpleMode)
        nameText = danmaku.getNickname();

    QString text;
    MessageType msgType = danmaku.getMsgType();
    if (msgType == MSG_DANMAKU)
    {
        // 新人：0级，3次以内
        if (newbieTip && !danmaku.isPkLink())
        {
            int count = danmakuCounts->value("danmaku/"+snum(danmaku.getUid())).toInt();
            if (danmaku.getLevel() == 0 && count <= 1 && danmaku.getMedalLevel() <= 1)
            {
                if (simpleMode)
                    nameText = "<font color='gray'>[新]</font>" + nameText;
                else
                    nameText = "<font color='red'>[新]</font>" + nameText;
            }
            else if (danmaku.getLevel() > 0 && count <= 1) // 10句话以内
            {
                if (simpleMode)
                    nameText = "<font color='gray'>[初]</font>" + nameText;
                else
                    nameText = "<font color='green'>[初]</font>" + nameText;
            }
        }

        // 彩色消息
        QString colorfulMsg = msg;
        if (simpleMode)
        {}
        else if (danmaku.isNoReply() || danmaku.isPkLink()) // 灰色
        {
            colorfulMsg = "<font color='gray'>"
                    + danmaku.getText() + "</font> ";
        }
        else if (!isBlankColor(danmaku.getTextColor())
                 && !ignoreDanmakuColors.contains(danmaku.getTextColor()))
        {
            colorfulMsg = "<font color='" + danmaku.getTextColor() + "'>"
                    + danmaku.getText() + "</font> ";
        }

        text = nameText + " " + colorfulMsg;

        // 翻译
        if (!trans.isEmpty() && trans != msg)
        {
            text = text + "（" + trans + "）";
        }

        // 回复
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
        QString modify;
        QString board = "上船";
        if (danmaku.getFirst() == 1) // 初次
        {
            if (simpleMode)
                modify = "<font color='gray'>[初]</font>";
            else
                modify = "<font color='green'>[初]</font>";
        }
        else if (danmaku.getFirst() == 2) // 重新上传
        {
            if (simpleMode)
                modify = "<font color='gray'>[回]</font>";
            else
                modify = "<font color='red'>[回]</font>";
        }
        else // 续船
        {
            board = "续船";
        }
        text = modify + QString("<font color='red'>[" + board + "]</font>" + " %1 开通 %2×%3")
                .arg(nameText)
                .arg(danmaku.getGiftName())
                .arg(danmaku.getNumber());
    }
    else if (msgType == MSG_WELCOME || msgType == MSG_WELCOME_GUARD)
    {
        // 粉丝牌
        QString medalColorStr = isBlankColor(danmaku.getMedalColor())
                ? QVariant(this->msgColor).toString()
                : danmaku.getMedalColor();
        if (simpleMode)
            medalColorStr = QVariant(QColor(Qt::gray)).toString();
        if (!danmaku.getAnchorRoomid().isEmpty()
                && !danmaku.getMedalName().isEmpty())
        {
            text += QString("<font style=\"color:%1;\">%2%3</font> ")
                    .arg(medalColorStr)
                    .arg(danmaku.getMedalName())
                    .arg(danmaku.getMedalLevel());
        }

        // 人名
        if (danmaku.isAdmin() || danmaku.isGuard())
        {
            text += QString("舰长 %1 进入直播间")
                    .arg(nameText);
        }
        else
        {
            text += nameText + " ";
            if (danmaku.getNumber() > 0) // 不包括这一次的
                text += QString("进入<font color='gray'> %1次</font>").arg(danmaku.getNumber());
            else if (!danmaku.getSpreadDesc().isEmpty())
                text += "<font color='gray'>进入</font>";
            else
                text += "<font color='gray'>进入直播间</font>";
        }

        // 推广
        if (!danmaku.getSpreadDesc().isEmpty())
        {
            text += " ";
            if (danmaku.getSpreadInfo().isEmpty())
                text += danmaku.getSpreadDesc();
            else
                text += "<font color='"+danmaku.getSpreadInfo()+"'>"+danmaku.getSpreadDesc()+"</font>";
        }
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
//        qint64 second = QDateTime::currentSecsSinceEpoch() - danmaku.getPrevTimestamp();
        text = QString("<font color='gray'>[关注]</font> %1 %2")
                .arg(nameText)
                .arg(danmaku.isAttention() ? "关注了主播" : "取消关注主播");
    }
    else if (msgType == MSG_BLOCK)
    {
        text = QString("<font color='gray'>[禁言]</font> %1 被房管禁言")
                .arg(danmaku.getNickname());
    }
    else if (msgType == MSG_MSG)
    {
        text = QString("<font color='gray'>%1</font>")
                .arg(danmaku.getText());
    }

    // 主播判断
    if (upUid && danmaku.getUid() == upUid)
        text = "<font color='#F08080'>[主播]</font> " + text;

    if (danmaku.isRobot())
        text = "<font color='#5E86C1'>[机]</font> " + text;

    // 串门判断
    if (danmaku.isToView())
        text = "[串门] " + text;
    else if (danmaku.isOpposite())
        text = "[对面] " + text;

    // 消息同步
    if (danmaku.isPkLink()) // 这个最置顶前面
        text = "<font color='gray'>[同步]</font> " + text;

    // 文字与大小
    label->setText(text);
    label->adjustSize();
    widget->adjustSize();
    widget->layout()->activate();

    // item大小
    widget->resize(widget->sizeHint());
    item->setSizeHint(widget->sizeHint());

    // 动画
    if (DANMAKU_ANIMATION_ENABLED)
    {
        QPropertyAnimation* ani = new QPropertyAnimation(widget, "size");
        ani->setStartValue(QSize(widget->width(), oldHeight));
        ani->setEndValue(widget->size());
        ani->setDuration(300);
        ani->setEasingCurve(QEasingCurve::OutQuad);
        widget->resize(widget->width(), oldHeight);
        item->setSizeHint(widget->size());
        connect(ani, &QPropertyAnimation::valueChanged, widget, [=](const QVariant &val){
            item->setSizeHint(val.toSize());
            if (scrollEnd)
                listWidget->scrollToBottom();
        });
        connect(ani, &QPropertyAnimation::finished, widget, [=]{
            if (scrollEnd)
                listWidget->scrollToBottom();
            item->setSizeHint(widget->size());
        });
        ani->start();
    }

    if (scrollEnd)
        listWidget->scrollToBottom();
}

void LiveDanmakuWindow::highlightItemText(QListWidgetItem *item, bool recover)
{
    auto danmaku = item ? LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
    if (danmaku.isNoReply())
        return ;
    auto label = getItemWidgetLabel(item);
    if (!label)
        return ;
    QPalette pa(label->palette());
    pa.setColor(QPalette::Text, hlColor);
    label->setPalette(pa);
    item->setData(DANMAKU_HIGHLIGHT_ROLE, true);

    // 定时恢复原样
    if (!recover)
        return ;
    QTimer::singleShot(5000, [=]{
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
        auto label = getItemWidgetLabel(listWidget->item(i));
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

void LiveDanmakuWindow::resetItemsFont()
{
    for (int i = 0; i < listWidget->count(); i++)
    {
        auto item = listWidget->item(i);
        auto widget = listWidget->itemWidget(item);
        if (!widget)
            continue;
        QHBoxLayout* layout = static_cast<QHBoxLayout*>(widget->layout());
        auto layoutItem = layout->itemAt(DANMAKU_WIDGET_LABEL);
        auto label = layoutItem->widget();
        if (!label)
            continue;
        label->setFont(danmakuFont);
        label->adjustSize();
        widget->adjustSize();
        item->setSizeHint(widget->size());
    }
}

void LiveDanmakuWindow::mergeGift(LiveDanmaku danmaku)
{
    qint64 uid = danmaku.getUid();
    QString gift = danmaku.getGiftName();
    qint64 time = danmaku.getTimeline().toSecsSinceEpoch();

    for (int i = listWidget->count()-1; i >= 0; i--)
    {
        auto item = listWidget->item(i);
        LiveDanmaku dm = LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject());
        qint64 t = dm.getTimeline().toSecsSinceEpoch();
        if (t == 0)
            continue;
        if (t + 6 < time)
            return ;
        if (dm.getMsgType() != MSG_GIFT
                || dm.getUid() != uid
                || dm.getGiftName() != gift)
            continue;

        // 是这个没错了
        dm.addGift(danmaku.getNumber(), danmaku.getTotalCoin(), danmaku.getTimeline());
        item->setData(DANMAKU_JSON_ROLE, dm.toJson());
        item->setData(DANMAKU_STRING_ROLE, dm.toString());
        setItemWidgetText(item);
        break;
    }
}

void LiveDanmakuWindow::showMenu()
{
    auto item = listWidget->currentItem();
    auto danmaku = item ? LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
    qDebug() << "菜单信息：" << danmaku.toString();
    QString msg = danmaku.getText();
    qint64 uid = danmaku.getUid();
    MessageType type = danmaku.getMsgType();

    QMenu* menu = new QMenu(this);
    QAction* actionUserInfo = new QAction(QIcon(":/danmaku/home"), "用户主页", this);
    QAction* actionMedal = new QAction(QIcon(":/danmaku/medal"), "粉丝勋章", this);
    QAction* actionValue = new QAction(QIcon(":/icons/egg"), "礼物价值", this);
    QAction* actionHistory = new QAction(QIcon(":/danmaku/message"), "消息记录", this);
    QAction* actionFollow = new QAction(QIcon(":/danmaku/fans"), "粉丝数", this);
    QAction* actionView = new QAction(QIcon(":/danmaku/views"), "浏览量", this);

    QAction* actionAddCare = new QAction(QIcon(":/icons/heart"), "添加特别关心", this);
    QAction* actionStrongNotify = new QAction(QIcon(":/danmaku/notify"), "添加强提醒", this);
    QAction* actionSetName = new QAction(QIcon(":/danmaku/nick"), "设置专属昵称", this);
    QAction* actionSetGiftName = new QAction(QIcon(":/danmaku/"), "设置礼物别名", this);

    QAction* actionAddBlockTemp = new QAction(QIcon(":/danmaku/block2"), "禁言1小时", this);
    QAction* actionAddBlock = new QAction(QIcon(":/danmaku/block2"), "禁言720小时", this);
    QAction* actionDelBlock = new QAction(QIcon(":/danmaku/block2"), "取消禁言", this);
    QAction* actionEternalBlock = new QAction(QIcon(":/danmaku/block"), "永久禁言", this);
    QAction* actionCancelEternalBlock = new QAction(QIcon(":/danmaku/block"), "取消永久禁言", this);
    QAction* actionNotWelcome = new QAction(QIcon(":/danmaku/welcome"), "不自动欢迎", this);
    QAction* actionNotReply = new QAction(QIcon(":/danmaku/reply"), "不自动回复", this);

    QMenu* operMenu = new QMenu("文字", this);
    operMenu->setIcon(QIcon(":/danmaku/word"));
    QAction* actionRepeat = new QAction("+1", this);
    QAction* actionCopy = new QAction("复制", this);
    QAction* actionFreeCopy = new QAction("自由复制", this);
    QAction* actionSearch = new QAction("百度", this);
    QAction* actionTranslate = new QAction("翻译", this);
    QAction* actionReply = new QAction("AI回复", this);
    QAction* actionIgnoreColor = new QAction("忽视颜色", this);

    QMenu* settingMenu = new QMenu("设置", this);
    settingMenu->setIcon(QIcon(":/danmaku/settings"));
    QAction* actionNameColor = new QAction("昵称颜色", this);
    QAction* actionMsgColor = new QAction("消息颜色", this);
    QAction* actionBgColor = new QAction("背景颜色", this);
    QAction* actionHlColor = new QAction("高亮颜色", this);
    QAction* actionFont = new QAction("弹幕字体", this);

    QMenu* pictureMenu = new QMenu("背景图片", settingMenu);
    QAction* actionPictureSelect = new QAction("选择图片", this);
    QAction* actionPictureFolder = new QAction("文件夹轮播", this);
    QAction* actionPictureInterval = new QAction("轮播间隔", this);
    QAction* actionPictureRatio = new QAction("保持比例", this);
    QAction* actionPictureAlpha = new QAction("图片透明度", this);
    QAction* actionPictureBlur = new QAction("模糊半径", this);
    QAction* actionCancelPicture = new QAction("取消图片", this);

    QAction* actionSendMsg = new QAction("发送框", this);
    QAction* actionDialogSend = new QAction("快速触发", this);
    QAction* actionShortCut = new QAction("快捷键", this);
    QAction* actionSendOnce = new QAction("单次发送", this);
    QAction* actionSimpleMode  = new QAction("简约模式", this);
    QAction* actionChatMode  = new QAction("聊天模式", this);
    QAction* actionWindow = new QAction("窗口模式", this);
    QAction* actionOnTop = new QAction("置顶显示", this);

    QAction* actionDelete = new QAction(QIcon(":/danmaku/delete"), "删除", this);
    QAction* actionHide = new QAction(QIcon(":/danmaku/hide"), "隐藏", this);

    actionNotWelcome->setCheckable(true);
    actionNotReply->setCheckable(true);
    actionRepeat->setDisabled(!danmaku.is(MSG_DANMAKU));
    actionSendMsg->setCheckable(true);
    actionSendMsg->setChecked(!lineEdit->isHidden());
    actionDialogSend->setToolTip("shift+alt+D 触发编辑框，输入后ESC返回原先窗口");
    actionDialogSend->setCheckable(true);
    actionDialogSend->setChecked(settings.value("livedanmakuwindow/sendEditShortcut", false).toBool());
    actionSendOnce->setToolTip("发送后，返回原窗口（如果有）");
    actionSendOnce->setCheckable(true);
    actionSendOnce->setChecked(settings.value("livedanmakuwindow/sendOnce", false).toBool());
    actionSimpleMode->setCheckable(true);
    actionSimpleMode->setChecked(simpleMode);
    actionChatMode->setCheckable(true);
    actionChatMode->setChecked(chatMode);
    actionWindow->setCheckable(true);
    actionWindow->setChecked(settings.value("livedanmakuwindow/jiWindow", false).toBool());
    actionOnTop->setCheckable(true);
    actionOnTop->setChecked(settings.value("livedanmakuwindow/onTop", true).toBool());
    actionPictureSelect->setCheckable(true);
    actionPictureSelect->setChecked(!pictureFilePath.isEmpty());
    actionPictureFolder->setCheckable(true);
    actionPictureFolder->setChecked(!pictureDirPath.isEmpty());
    actionPictureRatio->setCheckable(true);
    actionPictureRatio->setChecked(aspectRatio);
    actionPictureBlur->setCheckable(true);
    actionPictureBlur->setChecked(pictureBlur);

    if (uid != 0)
    {
        if (danmaku.isAdmin())
            actionUserInfo->setText("房管主页");
        else if (danmaku.getGuard() == 1)
            actionUserInfo->setText("总督主页");
        else if (danmaku.getGuard() == 2)
            actionUserInfo->setText("提督主页");
        else if (danmaku.getGuard() == 3)
            actionUserInfo->setText("舰长主页");
        else if (danmaku.isVip() || danmaku.isSvip())
            actionUserInfo->setText("姥爷主页");
        // 弹幕的数据多一点，包含牌子、等级等
        if (danmaku.getMsgType() == MSG_DANMAKU)
        {
            actionUserInfo->setText(actionUserInfo->text() +  "：LV" + snum(danmaku.getLevel()));
            actionHistory->setText("消息记录：" + snum(danmakuCounts->value("danmaku/"+snum(uid)).toInt()) + "条");
        }
        else if (danmaku.getMsgType() == MSG_GIFT || danmaku.getMsgType() == MSG_GUARD_BUY)
        {
            actionHistory->setText("送礼总额：" + snum(danmakuCounts->value("gold/"+snum(uid)).toInt()/1000) + "元");
            actionMedal->setText("船员数量：" + snum(currentGuards.size()));
        }

        if (!danmaku.getAnchorRoomid().isEmpty() && !danmaku.getMedalName().isEmpty())
        {
            actionMedal->setText(danmaku.getMedalName() + " " + snum(danmaku.getMedalLevel()));
        }
        else
        {
            actionMedal->setEnabled(false);
        }

        if (danmaku.is(MSG_GIFT) || danmaku.is(MSG_GUARD_BUY))
        {
            actionValue->setText(snum(danmaku.getTotalCoin()) + " " + (danmaku.isGoldCoin() ? "金瓜子" : "银瓜子"));
            if (giftNames.contains(danmaku.getGiftId()))
                actionSetGiftName->setText("礼物别名：" + giftNames.value(danmaku.getGiftId()));
        }

        if (careUsers.contains(uid))
            actionAddCare->setText("移除特别关心");
        if (strongNotifyUsers.contains(uid))
            actionStrongNotify->setText("移除强提醒");
        if (localNicknames.contains(uid))
            actionSetName->setText("专属昵称：" + localNicknames.value(uid));
        actionNotWelcome->setChecked(notWelcomeUsers.contains(uid));
        actionNotReply->setChecked(notReplyUsers.contains(uid));

        if (!danmaku.getTextColor().isEmpty())
        {
            if (ignoreDanmakuColors.contains(danmaku.getTextColor()))
                actionIgnoreColor->setText("恢复颜色");
        }

        showFollowCountInAction(uid, actionFollow);
        showViewCountInAction(uid, actionView);

        if (danmaku.getNickname().isEmpty())
            actionEternalBlock->setEnabled(false);
    }
    else // 包括 item == nullptr
    {
        actionUserInfo->setEnabled(false);
        actionHistory->setEnabled(false);
        actionAddBlock->setEnabled(false);
        actionAddBlockTemp->setEnabled(false);
        actionDelBlock->setEnabled(false);
        actionEternalBlock->setEnabled(false);
        actionCancelEternalBlock->setEnabled(false);
        actionMedal->setEnabled(false);
        actionValue->setEnabled(false);
        actionAddCare->setEnabled(false);
        actionStrongNotify->setEnabled(false);
        actionSetName->setEnabled(false);
        actionNotWelcome->setEnabled(false);
        actionNotReply->setEnabled(false);
        actionFollow->setEnabled(false);
        actionView->setEnabled(false);
    }

    if (!item)
    {
        operMenu->setEnabled(false);
        actionAddCare->setEnabled(false);
        actionStrongNotify->setEnabled(false);
        actionSetName->setEnabled(false);
        actionCopy->setEnabled(false);
        actionSearch->setEnabled(false);
        actionTranslate->setEnabled(false);
        actionReply->setEnabled(false);
        actionFreeCopy->setEnabled(false);
        actionDelete->setEnabled(false);
        actionIgnoreColor->setEnabled(false);
    }

    menu->addAction(actionUserInfo);
    menu->addAction(actionMedal);
    if (danmaku.is(MSG_GIFT))
        menu->addAction(actionValue);
    menu->addAction(actionHistory);
    menu->addAction(actionFollow);
    menu->addAction(actionView);
    if (enableBlock && uid)
    {
        menu->addSeparator();
        if (!userBlockIds.contains(uid) && danmaku.getMsgType() != MSG_BLOCK)
        {
            menu->addAction(actionAddBlockTemp);
            menu->addAction(actionAddBlock);
            if (eternalBlockUsers.contains(EternalBlockUser(uid, roomId)))
                menu->addAction(actionCancelEternalBlock);
            else
                menu->addAction(actionEternalBlock);
        }
        else
        {
            menu->addAction(actionDelBlock);
        }
    }

    menu->addSeparator();
    menu->addAction(actionAddCare);
    menu->addAction(actionStrongNotify);
    menu->addAction(actionSetName);
    menu->addAction(actionNotWelcome);
    menu->addAction(actionNotReply);
    if (danmaku.is(MSG_GIFT))
        menu->addAction(actionSetGiftName);
    menu->addSeparator();
    menu->addMenu(operMenu);
    menu->addMenu(settingMenu);

    operMenu->addAction(actionRepeat);
    operMenu->addAction(actionCopy);
    operMenu->addAction(actionFreeCopy);
    operMenu->addSeparator();
    operMenu->addAction(actionSearch);
    operMenu->addAction(actionTranslate);
    operMenu->addAction(actionReply);
    operMenu->addSeparator();
    operMenu->addAction(actionIgnoreColor);

    pictureMenu->addAction(actionPictureSelect);
    pictureMenu->addAction(actionPictureFolder);
    pictureMenu->addSeparator();
    pictureMenu->addAction(actionPictureInterval);
    pictureMenu->addAction(actionPictureAlpha);
    pictureMenu->addAction(actionPictureRatio);
    pictureMenu->addAction(actionPictureBlur);
    pictureMenu->addSeparator();
    pictureMenu->addAction(actionCancelPicture);

    settingMenu->addAction(actionNameColor);
    settingMenu->addAction(actionMsgColor);
    settingMenu->addAction(actionBgColor);
    settingMenu->addAction(actionHlColor);
    settingMenu->addAction(actionFont);
    settingMenu->addMenu(pictureMenu);
    settingMenu->addSeparator();
    settingMenu->addAction(actionSendMsg);
#ifndef Q_OS_ANDROID
    settingMenu->addAction(actionDialogSend);
    settingMenu->addAction(actionShortCut);
#endif
    settingMenu->addAction(actionSendOnce);
    settingMenu->addSeparator();
    settingMenu->addAction(actionSimpleMode);
    settingMenu->addAction(actionChatMode);
    settingMenu->addAction(actionWindow);
    settingMenu->addAction(actionOnTop);

    menu->addAction(actionDelete);
    menu->addAction(actionHide);

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
        if (statusLabel)
        {
            QPalette pa;
            pa.setColor(QPalette::Text, c);
            statusLabel->setPalette(pa);
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
    connect(actionFont, &QAction::triggered, this, [=]{
        bool ok;
        QFont font = QFontDialog::getFont(&ok, danmakuFont, this, "设置弹幕字体");
        if (!ok)
            return ;
        this->danmakuFont = font;
        this->setFont(font);
        settings.setValue("livedanmakuwindow/font", danmakuFont.toString());
        resetItemsFont();
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
    connect(actionStrongNotify, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;

        if (strongNotifyUsers.contains(danmaku.getUid())) // 已存在，移除
        {
            strongNotifyUsers.removeOne(danmaku.getUid());
            setItemWidgetText(item);
        }
        else // 添加强提醒
        {
            strongNotifyUsers.append(danmaku.getUid());
            highlightItemText(item);
        }

        // 保存特别关心
        QStringList ress;
        foreach (qint64 uid, strongNotifyUsers)
            ress << QString::number(uid);
        settings.setValue("danmaku/strongNotifyUsers", ress.join(";"));
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
        QString tip = "设置【" + danmaku.getNickname() + "】的专属昵称\n将影响机器人的欢迎/感谢弹幕";
        if (localNicknames.contains(uid))
        {
            tip += "\n清空则取消专属，还原账号昵称";
        }
        else
        {
            QString pinyin = getPinyin(danmaku.getNickname());
            if (!pinyin.isEmpty())
                tip += "\n中文拼音：" + pinyin;
        }
        name = QInputDialog::getText(this, "专属昵称", tip, QLineEdit::Normal, name, &ok);
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
    connect(actionNotWelcome, &QAction::triggered, this, [=]{
        if (notWelcomeUsers.contains(uid))
            notWelcomeUsers.removeOne(uid);
        else
            notWelcomeUsers.append(uid);

        QStringList ress;
        foreach (qint64 uid, notWelcomeUsers)
            ress << QString::number(uid);
        settings.setValue("danmaku/notWelcomeUsers", ress.join(";"));
    });
    connect(actionNotReply, &QAction::triggered, this, [=]{
        if (notReplyUsers.contains(uid))
            notReplyUsers.removeOne(uid);
        else
            notReplyUsers.append(uid);

        QStringList ress;
        foreach (qint64 uid, notReplyUsers)
            ress << QString::number(uid);
        settings.setValue("danmaku/notReplyUsers", ress.join(";"));
    });
    connect(actionSetGiftName, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;

        // 设置别名
        int giftId = danmaku.getGiftId();
        bool ok = false;
        QString name;
        if (giftNames.contains(giftId))
            name = giftNames.value(giftId);
        else
            name = danmaku.getGiftName();
        QString tip = "设置【" + danmaku.getGiftName() + "】的别名\n只影响机器人的感谢弹幕";
        if (giftNames.contains(giftId))
        {
            tip += "\n清空则取消别名，还原礼物原始名字";
        }
        name = QInputDialog::getText(this, "礼物别名", tip, QLineEdit::Normal, name, &ok);
        if (!ok)
            return ;
        if (name.isEmpty())
        {
            if (giftNames.contains(giftId))
                giftNames.remove(giftId);
        }
        else
        {
            giftNames[giftId] = name;
        }

        // 保存本地昵称
        QStringList ress;
        auto it = giftNames.begin();
        while (it != giftNames.end())
        {
            ress << QString("%1=>%2").arg(it.key()).arg(it.value());
            it++;
        }
        settings.setValue("danmaku/giftNames", ress.join(";"));
    });
    connect(actionCopy, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;
        // 复制
        QApplication::clipboard()->setText(msg);
    });
    connect(actionRepeat, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;
        // 复制
        QString text = danmaku.getText();
        if (danmaku.isOpposite())
        {
            emit signalSendMsgToPk(text);
        }
        else
        {
            emit signalSendMsg(text);
        }
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
    connect(actionIgnoreColor, &QAction::triggered, this, [=]{
        QString s = danmaku.getTextColor();
        if (s.isEmpty())
            return ;
        if (ignoreDanmakuColors.contains(s))
        {
            ignoreDanmakuColors.removeOne(s);
        }
        else
        {
            ignoreDanmakuColors.append(s);
        }

        settings.setValue("livedanmakuwindow/ignoreColor", ignoreDanmakuColors.join(";"));
        resetItemsText();
    });
    connect(actionSendMsg, &QAction::triggered, this, [=]{
        if (lineEdit->isHidden())
            lineEdit->show();
        else
            lineEdit->hide();
        settings.setValue("livedanmakuwindow/sendEdit", !lineEdit->isHidden());
    });
#ifndef Q_OS_ANDROID
    connect(actionDialogSend, &QAction::triggered, this, [=]{
        bool enable = !settings.value("livedanmakuwindow/sendEditShortcut", false).toBool();
        settings.setValue("livedanmakuwindow/sendEditShortcut", enable);
        editShortcut->setEnabled(enable);
    });
    connect(actionShortCut, &QAction::triggered, this, [=]{
        QString def_key = settings.value("livedanmakuwindow/shortcutKey", "shift+alt+D").toString();
        QString key = QInputDialog::getText(this, "发弹幕快捷键", "设置显示弹幕发送框的快捷键", QLineEdit::Normal, def_key);
        if (!key.isEmpty())
        {
            if (editShortcut->setShortcut(QKeySequence(key)))
                settings.setValue("livedanmakuwindow/shortcutKey", key);
        }
    });
#endif
    connect(actionSendOnce, &QAction::triggered, this, [=]{
        bool enable = !settings.value("livedanmakuwindow/sendOnce", false).toBool();
        settings.setValue("livedanmakuwindow/sendOnce", enable);
    });
    connect(actionWindow, &QAction::triggered, this, [=]{
        bool enable = !settings.value("livedanmakuwindow/jiWindow", false).toBool();
        settings.setValue("livedanmakuwindow/jiWindow", enable);
        if (enable)
        {
            qDebug() << "开启直播姬窗口模式";
            this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            this->setAttribute(Qt::WA_TranslucentBackground, false);
        }
        else
        {
            qDebug() << "关闭直播姬窗口模式";
            this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
            this->setAttribute(Qt::WA_TranslucentBackground, true);
        }
#ifndef Q_OS_ANDROID
        editShortcut->setShortcut(QKeySequence(""));
        editShortcut->setDisabled(true);
        delete editShortcut;
        editShortcut = nullptr;
#endif
        emit signalChangeWindowMode();
    });
    connect(actionOnTop, &QAction::triggered, this, [=]{
        bool onTop = !settings.value("livedanmakuwindow/onTop", true).toBool();
        this->setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
        settings.setValue("livedanmakuwindow/onTop", onTop);
        qDebug() << "置顶显示：" << onTop;
        this->show();
    });
    connect(actionSimpleMode, &QAction::triggered, this, [=]{
        settings.setValue("livedanmakuwindow/simpleMode", simpleMode = !simpleMode);
    });
    connect(actionChatMode, &QAction::triggered, this, [=]{
        settings.setValue("livedanmakuwindow/chatMode", chatMode = !chatMode);
    });
    connect(actionUserInfo, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/" + snum(uid)));
    });
    connect(actionMedal, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://live.bilibili.com/" + danmaku.getAnchorRoomid()));
    });
    connect(actionValue, &QAction::triggered, this, [=]{

    });
    connect(actionHistory, &QAction::triggered, this, [=]{
        showUserMsgHistory(uid, danmaku.getNickname());
    });
    connect(actionFollow, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/"+snum(uid)+"/fans/follow"));
    });
    connect(actionView, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/"+snum(uid)+"/video"));
    });
    connect(actionAddBlockTemp, &QAction::triggered, this, [=]{
        emit signalAddBlockUser(uid, 1);
    });
    connect(actionAddBlock, &QAction::triggered, this, [=]{
        emit signalAddBlockUser(uid, 720);
    });
    connect(actionEternalBlock, &QAction::triggered, this, [=]{
        emit signalEternalBlockUser(uid, danmaku.getNickname());
    });
    connect(actionCancelEternalBlock, &QAction::triggered, this, [=]{
        emit signalCancelEternalBlockUser(uid);
    });
    connect(actionDelBlock, &QAction::triggered, this, [=]{
        emit signalDelBlockUser(uid);
    });
    connect(actionDelete, &QAction::triggered, this, [=]{
//        if (item->data(DANMAKU_STRING_ROLE).toString().isEmpty())
        {
            // 强制删除
            qDebug() << "强制删除弹幕：" << item->data(DANMAKU_STRING_ROLE).toString();
            listWidget->removeItemWidget(item);
            listWidget->takeItem(listWidget->row(item));
            return ;
        }
//        slotOldLiveDanmakuRemoved(danmaku);
    });
    connect(actionHide, &QAction::triggered, this, [=]{
        this->hide();
    });

    connect(actionPictureSelect, &QAction::triggered, this, [=]{
        QString path = QFileDialog::getOpenFileName(this, tr("请选择图片文件"), pictureFilePath, tr("Images (*.png *.xpm *.jpg)"));
        if (path.isEmpty())
            return ;

        pictureFilePath = path;
        pictureDirPath = "";
        settings.setValue("livedanmakuwindow/pictureFilePath", pictureFilePath);
        settings.setValue("livedanmakuwindow/pictureDirPath", pictureDirPath);
        switchBgTimer->stop();
        selectBgPicture();
        update();
    });
    connect(actionPictureFolder, &QAction::triggered, this, [=]{
        QString dir = QFileDialog::getExistingDirectory(this, tr("请选择图片文件夹"),
                                                         pictureDirPath,
                                                         QFileDialog::ShowDirsOnly
                                                         | QFileDialog::DontResolveSymlinks);
        if (dir.isEmpty())
            return ;

        pictureFilePath = "";
        pictureDirPath = dir;
        settings.setValue("livedanmakuwindow/pictureFilePath", pictureFilePath);
        settings.setValue("livedanmakuwindow/pictureDirPath", pictureDirPath);
        bgPixmap = QPixmap();
        switchBgTimer->start();
        selectBgPicture();
        update();
    });
    connect(actionPictureInterval, &QAction::triggered, this, [=]{
        bool ok = false;
        int val = QInputDialog::getInt(this, "轮播间隔", "每张图片显示的时长，单位秒",
                                       switchBgTimer->interval()/1000, 3, 360000, 10, &ok);
        if (!ok)
            return ;
        switchBgTimer->setInterval(val * 1000);
        settings.setValue("livedanmakuwindow/pictureInterval", val);
        update();
    });
    connect(actionPictureAlpha, &QAction::triggered, this, [=]{
        bool ok = false;
        int val = QInputDialog::getInt(this, "图片透明度", "请输入背景图片透明度，0~255，越低越透明", pictureAlpha, 0, 255, 32, &ok);
        if (!ok)
            return ;
        pictureAlpha = val;
        settings.setValue("livedanmakuwindow/pictureAlpha", pictureAlpha);
        update();
    });
    connect(actionPictureRatio, &QAction::triggered, this, [=]{
        aspectRatio = !aspectRatio;
        settings.setValue("livedanmakuwindow/aspectRatio", aspectRatio);
        update();
    });
    connect(actionPictureBlur, &QAction::triggered, this, [=]{
        bool ok = false;
        int val = QInputDialog::getInt(this, "图片模糊半径", "请输入背景图片模糊半径，越大越模糊", pictureBlur, 0, 2000, 32, &ok);
        if (!ok)
            return ;
        pictureBlur = val;
        settings.setValue("livedanmakuwindow/pictureBlur", pictureBlur);
        selectBgPicture();
        update();
    });
    connect(actionCancelPicture, &QAction::triggered, this, [=]{
        pictureFilePath = "";
        pictureDirPath = "";
        settings.setValue("livedanmakuwindow/pictureFilePath", pictureFilePath);
        settings.setValue("livedanmakuwindow/pictureDirPath", pictureDirPath);
        switchBgTimer->stop();
        selectBgPicture();
        update();
    });

    menu->exec(QCursor::pos());

    menu->deleteLater();
    actionMsgColor->deleteLater();
    actionBgColor->deleteLater();
    actionHlColor->deleteLater();
    actionMedal->deleteLater();
    actionValue->deleteLater();
    actionAddCare->deleteLater();
    actionSetName->deleteLater();
    actionRepeat->deleteLater();
    actionCopy->deleteLater();
    actionSearch->deleteLater();
    actionTranslate->deleteLater();
    actionReply->deleteLater();
    actionFreeCopy->deleteLater();
    actionSendMsg->deleteLater();
    actionDelete->deleteLater();
    actionHide->deleteLater();
    actionSimpleMode->deleteLater();
    actionChatMode->deleteLater();
    actionWindow->deleteLater();
    actionOnTop->deleteLater();
    actionPictureSelect->deleteLater();
    actionPictureFolder->deleteLater();
    actionPictureAlpha->deleteLater();
    actionPictureInterval->deleteLater();
    actionPictureRatio->deleteLater();
    actionPictureBlur->deleteLater();
    actionCancelPicture->deleteLater();
}

void LiveDanmakuWindow::showEditMenu()
{
    QString text = lineEdit->text();
    QMenu* menu = new QMenu(this);
    QAction* actionSendMsgToPk = new QAction("发送至PK对面直播间", this);

    if (!pkStatus || text.isEmpty())
    {
        actionSendMsgToPk->setEnabled(false);
    }

    menu->addAction(actionSendMsgToPk);

    connect(actionSendMsgToPk, &QAction::triggered, this, [=]{
        if (text.isEmpty())
            return;
        emit signalSendMsgToPk(text);
        lineEdit->clear();
    });

    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void LiveDanmakuWindow::showPkMenu()
{
    if (!pkStatus || !pkRoomId)
        return ;

    QMenu* menu = new QMenu(this);

    QAction* actionUser = new QAction(QIcon(":/danmaku/home"), "用户主页", this);
    QAction* actionRank = new QAction(QIcon(":/danmaku/home"), "直播等级", this);
    QAction* actionGuard = new QAction(QIcon(":/danmaku/fans"), "船员数", this);
    QAction* actionAttention = new QAction(QIcon(":/danmaku/fans"), "关注数", this);
    QAction* actionFans = new QAction(QIcon(":/danmaku/fans"), "粉丝数", this);
    QAction* actionView = new QAction(QIcon(":/danmaku/views"), "浏览量", this);
    QAction* actionRead = new QAction(QIcon(":/danmaku/views"), "浏览量", this);
    QAction* actionLike = new QAction(QIcon(":/danmaku/views"), "获赞量", this);
    QAction* actionVideo = new QAction(QIcon(":/icons/video2"), "匿名进入", this);

    menu->addAction(actionUser);
    menu->addAction(actionRank);
    menu->addSeparator();
    menu->addAction(actionGuard);
    menu->addAction(actionAttention);
    menu->addAction(actionFans);
    menu->addAction(actionLike);
    menu->addAction(actionView);
    menu->addAction(actionRead);
    menu->addSeparator();
    menu->addAction(actionVideo);

    showPkLevelInAction(pkRoomId, actionUser, actionRank);
    showFollowCountInAction(pkUid, actionAttention, actionFans);
    showViewCountInAction(pkUid, actionView, actionRead, actionLike);
    showGuardInAction(pkRoomId, pkUid, actionGuard);

    connect(actionUser, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/" + snum(pkUid)));
    });
    connect(actionUser, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://live.bilibili.com/"+snum(pkRoomId)));
    });
    connect(actionGuard, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://live.bilibili.com/"+snum(pkRoomId)));
    });
    connect(actionAttention, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/"+snum(pkUid)+"/fans/follow"));
    });
    connect(actionFans, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/"+snum(pkUid)+"/fans/fans"));
    });
    connect(actionView, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/"+snum(pkUid)+"/video"));
    });
    connect(actionRead, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/"+snum(pkUid)+"/article"));
    });
    connect(actionLike, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/"+snum(pkUid)+"/dynamic"));
    });
    connect(actionVideo, &QAction::triggered, this, [=]{
        emit signalShowPkVideo();
    });

    menu->exec(QCursor::pos());

    actionUser->deleteLater();
    actionRank->deleteLater();
    actionGuard->deleteLater();
    actionAttention->deleteLater();
    actionFans->deleteLater();
    actionView->deleteLater();
    actionLike->deleteLater();
    actionRead->deleteLater();
    actionVideo->deleteLater();
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
        if (trans.isEmpty() || trans.trimmed() == msg.trimmed())
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

/// 腾讯AI开放平台 https://ai.qq.com/console/home
void LiveDanmakuWindow::startReply(QListWidgetItem *item)
{
    auto danmaku = LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject());
    qint64 uid = danmaku.getUid();
    if (!uid || notReplyUsers.contains(uid))
        return ;
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
            qDebug() << "AI回复：" << error.errorString();
            return ;
        }

        QJsonObject json = document.object();
        if (json.value("ret").toInt() != 0)
        {
            qDebug() << "AI回复：" << json.value("msg").toString();
            return ;
        }

        QString answer = json.value("data").toObject().value("answer").toString();
        emit signalAIReplyed(answer);

        if (!isItemExist(item))
            return ;

        qDebug() << "回复：" << msg << " => " << answer;
        item->setData(DANMAKU_REPLY_ROLE, answer);

        adjustItemTextDynamic(item);
    });
}

void LiveDanmakuWindow::setEnableBlock(bool enable)
{
    enableBlock = enable;
}

void LiveDanmakuWindow::setListWidgetItemSpacing(int x)
{
    listWidget->setSpacing(x);
}

void LiveDanmakuWindow::setNewbieTip(bool tip)
{
    this->newbieTip = tip;
}

void LiveDanmakuWindow::setIds(qint64 uid, qint64 roomId)
{
    this->upUid = uid;
    this->roomId = roomId;
}

void LiveDanmakuWindow::markRobot(qint64 uid)
{
    for (int i = 0; i < listWidget->count(); i++)
    {
        auto item = listWidget->item(i);
        auto danmaku = item ? LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject()) : LiveDanmaku();
        if (danmaku.getUid() == uid)
        {
            danmaku.setRobot(true);
            item->setData(DANMAKU_JSON_ROLE, danmaku.toJson());
            setItemWidgetText(item);
        }
    }
}

void LiveDanmakuWindow::showFastBlock(qint64 uid, QString msg)
{
    auto moveAni = [=](QWidget* widget, QPoint start, QPoint end, int duration, QEasingCurve curve) {
        QPropertyAnimation* ani = new QPropertyAnimation(widget, "pos");
        ani->setStartValue(start);
        ani->setEndValue(end);
        ani->setDuration(duration);
        ani->setEasingCurve(curve);
        connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
        ani->start();
    };

    // 初始化控件
    QLabel* label = new QLabel(this);
    QPushButton* btn = new QPushButton("拉黑", this);
    QTimer* timer = new QTimer(this);
    timer->setInterval(5000);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, timer, [=]{
        moveAni(label, label->pos(),
                QPoint(label->x(), -label->height()),
                300, QEasingCurve::InOutCubic);
        moveAni(btn, btn->pos(),
                QPoint(btn->x(), -btn->height()),
                300, QEasingCurve::InOutCubic);

        QTimer::singleShot(500, timer, [=]{
            timer->deleteLater();
            label->deleteLater();
            btn->deleteLater();
        });
    });
    connect(btn, &QPushButton::clicked, btn, [=]{
        disconnect(btn, SIGNAL(clicked()));
        emit signalAddBlockUser(uid, 720);
        timer->setInterval(100);
        timer->start();
    });

    label->setStyleSheet("QLabel { background: white; color: black; padding: 10px; "
                       "border: none; border-radius: 10px; }");
    btn->setStyleSheet("QPushButton { background: black; color:white;"
                       "border: none; border-radius: 10px;}"
                       "QPushButton:hover { background: #222222; }"
                       "QPushButton:pressed { background: #868686; }");

    label->setText(msg);
    label->setFixedWidth(listWidget->width() * 0.9);
    label->adjustSize();
    label->setMinimumHeight(24);
    label->move(this->width()/2 - label->width()/2, -label->height());

    btn->setFixedWidth(label->height()*2);
    btn->setCursor(Qt::PointingHandCursor);
    btn->move(label->geometry().right() - (label->height() - btn->height())/2 - btn->width(), -label->height());

    /*QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(label);
    effect->setColor(QColor(63, 63, 63, 30));
    effect->setBlurRadius(5);
    effect->setXOffset(0);
    effect->setYOffset(2);
    label->setGraphicsEffect(effect);*/

    label->show();
    btn->show();

    int labelTop = label->height() / 10;
    moveAni(label, label->pos(), QPoint(label->x(), labelTop), 300, QEasingCurve::OutBack);
    moveAni(btn, btn->pos(), QPoint(btn->x(), labelTop + (label->height()-btn->height())/2), 600, QEasingCurve::OutBounce);
    timer->start();
}

void LiveDanmakuWindow::setPkStatus(int status, qint64 roomId, qint64 uid, QString uname)
{
    this->pkStatus = status;
    this->pkRoomId = roomId;
    this->pkUid = uid;
    this->pkUname = uname;
}

void LiveDanmakuWindow::showStatusText()
{
    statusLabel->show();
}

void LiveDanmakuWindow::setStatusText(QString text)
{
    statusLabel->show();
    statusLabel->setText(text);
    statusLabel->adjustSize();
    statusLabel->move(width() - statusLabel->width(), 0);
}

void LiveDanmakuWindow::setStatusTooltip(QString tooltip)
{
    statusLabel->setToolTip(tooltip);
}

void LiveDanmakuWindow::hideStatusText()
{
    statusLabel->hide();
}

void LiveDanmakuWindow::showFollowCountInAction(qint64 uid, QAction *action, QAction *action2)
{
    QString url = "http://api.bilibili.com/x/relation/stat?vmid=" + snum(uid);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, action, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();

        int code = json.value("code").toInt();
        if (code != 0)
        {
            statusLabel->setText(json.value("message").toString());
            if(statusLabel->text().isEmpty() && code == 403)
                statusLabel->setText("您没有权限");
            return ;
        }
        QJsonObject obj = json.value("data").toObject();
        int following = obj.value("following").toInt(); // 关注
        int follower = obj.value("follower").toInt(); // 粉丝
        // int whisper = obj.value("whisper").toInt(); // 悄悄关注（自己关注）
        // int black = obj.value("black").toInt(); // 黑名单（自己登录）
        if (!action2)
        {
            action->setText(QString("关注:%1,粉丝:%2").arg(following).arg(follower));
        }
        else
        {
            action->setText(QString("关注数:%1").arg(following));
            action2->setText(QString("粉丝数:%1").arg(follower));
        }
    });
    manager->get(*request);
}

void LiveDanmakuWindow::showViewCountInAction(qint64 uid, QAction *action, QAction *action2, QAction *action3)
{
    QString url = "http://api.bilibili.com/x/space/upstat?mid=" + snum(uid);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, action, [=](QNetworkReply* reply){
        QByteArray ba = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
//        qDebug() << url;
//        qDebug() << json;

        int code = json.value("code").toInt();
        if (code != 0)
        {
            statusLabel->setText(json.value("message").toString());
            if(statusLabel->text().isEmpty() && code == 403)
                statusLabel->setText("您没有权限");
            return ;
        }
        QJsonObject data = json.value("data").toObject();
        int achive_view = data.value("archive").toObject().value("view").toInt();
        int article_view = data.value("article").toObject().value("view").toInt();
        int article_like = data.value("likes").toInt();

        if (!action2 && !action3)
        {
            QStringList sl;
            if (achive_view)
                sl << "播放:" + snum(achive_view);
            if (article_view)
                sl << "阅读:" + snum(article_view);
            if (article_like)
                sl << "点赞:" + snum(article_like);

            if (sl.size())
                action->setText(sl.join(","));
            else
                action->setText("没有投稿");
        }
        else
        {
            action->setText("播放数:" + snum(achive_view));
            if (action2)
                action2->setText("阅读数:" + snum(article_view));
            if (action3)
                action3->setText("获赞数:" + snum(article_like));
        }
    });
    manager->get(*request);
}

void LiveDanmakuWindow::showGuardInAction(qint64 roomId, qint64 uid, QAction *action)
{
    QString url = "https://api.live.bilibili.com/xlive/app-room/v2/guardTab/topList?roomid="
            +snum(roomId)+"&page=1&ruid="+snum(uid);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, action, [=](QNetworkReply* reply){
        QByteArray ba = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();

        int code = json.value("code").toInt();
        if (code != 0)
        {
            statusLabel->setText(json.value("message").toString());
            if(statusLabel->text().isEmpty() && code == 403)
                statusLabel->setText("您没有权限");
            return ;
        }
        QJsonObject data = json.value("data").toObject();
        QJsonObject info = data.value("info").toObject();
        int num = info.value("num").toInt();
        action->setText("船员数:" + snum(num));
    });
    manager->get(*request);
}

void LiveDanmakuWindow::showPkLevelInAction(qint64 roomId, QAction *actionUser, QAction *actionRank)
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByRoom?room_id="+snum(roomId);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, actionUser, [=](QNetworkReply* reply){
        QByteArray ba = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();

        int code = json.value("code").toInt();
        if (code != 0)
        {
            statusLabel->setText(json.value("message").toString());
            if(statusLabel->text().isEmpty() && code == 403)
                statusLabel->setText("您没有权限");
            return ;
        }
        QJsonObject data = json.value("data").toObject();
        QJsonObject anchor = data.value("anchor_info").toObject();
        QString uname = anchor.value("base_info").toObject().value("uname").toString();
        int level = anchor.value("live_info").toObject().value("level").toInt();
        actionUser->setText(uname + " " + snum(level));

        QJsonObject battle = data.value("battle_rank_entry_info").toObject();
        QString rankName = battle.value("rank_name").toString();
        actionRank->setText(rankName);
    });
    manager->get(*request);
}

void LiveDanmakuWindow::releaseLiveData()
{
    headPortraits.clear();
    hideStatusText();
    setIds(0, 0);
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

PortraitLabel *LiveDanmakuWindow::getItemWidgetPortrait(QListWidgetItem *item)
{
    auto widget = listWidget->itemWidget(item);
    if (!widget)
        return nullptr;
    QHBoxLayout* layout = static_cast<QHBoxLayout*>(widget->layout());
    auto layoutItem = layout->itemAt(DANMAKU_WIDGET_PORTRAIT);
    auto label = layoutItem->widget();
    return static_cast<PortraitLabel*>(label);
}

QLabel *LiveDanmakuWindow::getItemWidgetLabel(QListWidgetItem* item)
{
    auto widget = listWidget->itemWidget(item);
    if (!widget)
        return nullptr;
    QHBoxLayout* layout = static_cast<QHBoxLayout*>(widget->layout());
    auto layoutItem = layout->itemAt(DANMAKU_WIDGET_LABEL);
    auto label = layoutItem->widget();
    return static_cast<QLabel*>(label);
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
    auto label = getItemWidgetLabel(item);
    if (!label)
        return ;
    int ht = label->height();
    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum() -lineEdit->height()*2;

    setItemWidgetText(item);

    if (false && DANMAKU_ANIMATION_ENABLED)
    {
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
}

void LiveDanmakuWindow::getUserInfo(qint64 uid, QListWidgetItem* item)
{
    QString url = "http://api.bilibili.com/x/space/acc/info?mid=" + QString::number(uid);
    connect(new NetUtil(url), &NetUtil::finished, this, [=](QString result){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(result.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 0)
        {
            qDebug() << "返回结果不为0：" << json.value("message").toString();
            return ;
        }

        QJsonObject data = json.value("data").toObject();
        QString faceUrl = data.value("face").toString();

        getUserHeadPortrait(uid, faceUrl, item);
    });
}

void LiveDanmakuWindow::getUserHeadPortrait(qint64 uid, QString url, QListWidgetItem* item)
{
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply1 = manager.get(QNetworkRequest(QUrl(url)));
    //请求结束并下载完成后，退出子事件循环
    connect(reply1, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    //开启子事件循环
    loop.exec();
    QByteArray jpegData = reply1->readAll();
    QPixmap pixmap;
    pixmap.loadFromData(jpegData);
    if (pixmap.isNull())
    {
        qDebug() << "获取用户头像为空：" << uid;
        return ;
    }
    headPortraits[uid] = pixmap;

    if (!isItemExist(item))
        return ;
    PortraitLabel* label = getItemWidgetPortrait(item);
    label->setPixmap(pixmap);
}

void LiveDanmakuWindow::showUserMsgHistory(qint64 uid, QString title)
{
    if (!uid)
        return ;
    QStringList sums;
    int c = danmakuCounts->value("danmaku/"+snum(uid)).toInt();
    if(c)
        sums << snum(c)+" 条弹幕";
    c = danmakuCounts->value("come/"+snum(uid)).toInt();
    if (c)
        sums << "进来 " + snum(c)+" 次";
    c = danmakuCounts->value("gold/"+snum(uid)).toInt();
    if (c)
        sums << "赠送 " + snum(c)+" 金瓜子";
    c = danmakuCounts->value("silver/"+snum(uid)).toInt();
    if (c)
        sums << snum(c)+" 银瓜子";

    QStringList sl;
    if (sums.size())
        sl.append("* 总计：" + sums.join("，"));
    for (int i = allDanmakus.size()-1; i >= 0; i--)
    {
        const LiveDanmaku& danmaku = allDanmakus.at(i);
        if (danmaku.getUid() == uid)
        {
            if (danmaku.getMsgType() == MSG_DANMAKU)
                sl.append(danmaku.getTimeline().toString("hh:mm:ss") + "  " + danmaku.getText());
            else
                sl.append(danmaku.toString());
        }
    }

    QListView* view = new QListView(this);
    view->setModel(new QStringListModel(sl));
    view->setAttribute(Qt::WA_ShowModal, true);
    view->setAttribute(Qt::WA_DeleteOnClose, true);
    view->setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::Dialog);
    view->setWordWrap(true);
    QRect rect = this->geometry();
    int titleHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
    rect.setTop(rect.top()+titleHeight);
    view->setWindowTitle(title);
    view->setGeometry(rect);
    view->show();
}

QString LiveDanmakuWindow::getPinyin(QString text)
{
    QStringList chs = text.split("");
    QStringList res;
    foreach (QString ch, chs)
    {
        if (pinyinMap.contains(ch))
            res << ch + pinyinMap.value(ch);
    }
    return res.join(" ");
}

QVariant LiveDanmakuWindow::getCookies()
{
    QList<QNetworkCookie> cookies;

    // 设置cookie
    QString cookieText = browserCookie;
    QStringList sl = cookieText.split(";");
    foreach (auto s, sl)
    {
        s = s.trimmed();
        int pos = s.indexOf("=");
        QString key = s.left(pos);
        QString val = s.right(s.length() - pos - 1);
        cookies.push_back(QNetworkCookie(key.toUtf8(), val.toUtf8()));
    }

    // 请求头里面加入cookies
    QVariant var;
    var.setValue(cookies);
    return var;
}

void LiveDanmakuWindow::selectBgPicture()
{
    if (!pictureFilePath.isEmpty())
    {
        originPixmap = QPixmap(pictureFilePath);
        if (pictureBlur)
            originPixmap = getBlurPixmap(originPixmap);
        bgPixmap = originPixmap.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        bgAlpha = pictureAlpha;
        return ;
    }

    if (pictureDirPath.isEmpty()) // 取消了图片
    {
        originPixmap = QPixmap();
        bgPixmap = QPixmap();
        return ;
    }

    auto files = QDir(pictureDirPath).entryInfoList(QStringList{"*.jpg", "*.png", "*.bmp"});
    if (!files.size())
    {
        qDebug() << "未找到图片文件：" << pictureDirPath;
        bgPixmap = QPixmap();
        return ;
    }
    QString path = files.at(qrand() % files.size()).absoluteFilePath();

    prevPixmap = bgPixmap;
    originPixmap = QPixmap(path);
    if (pictureBlur)
        originPixmap = getBlurPixmap(originPixmap);
    bgPixmap = originPixmap.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPropertyAnimation* ani = new QPropertyAnimation(this, "bgAlpha");
    ani->setStartValue(0);
    ani->setEndValue(pictureAlpha);
    ani->setDuration(800);
    ani->setEasingCurve(QEasingCurve::InOutSine);
    connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
    ani->start();

    ani = new QPropertyAnimation(this, "prevAlpha");
    ani->setStartValue(pictureAlpha);
    ani->setEndValue(0);
    ani->setDuration(800);
    ani->setEasingCurve(QEasingCurve::InOutSine);
    connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
    ani->start();
}

QPixmap LiveDanmakuWindow::getBlurPixmap(QPixmap &bg)
{
    const int radius = pictureBlur;
    QPixmap pixmap = bg;
    pixmap = pixmap.scaled(this->width()+radius*2, this->height() + radius*2, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QImage img = pixmap.toImage(); // img -blur-> painter(pixmap)
    QPainter painter( &pixmap );
    qt_blurImage( &painter, img, radius, true, false );

    // 裁剪掉边缘（模糊后会有黑边）
    int c = qMin(bg.width(), bg.height());
    c = qMin(c/2, radius);
    QPixmap clip = pixmap.copy(c, c, pixmap.width()-c*2, pixmap.height()-c*2);
    return clip;
}

void LiveDanmakuWindow::setBgAlpha(int x)
{
    this->bgAlpha = x;
    update();
}

int LiveDanmakuWindow::getBgAlpha() const
{
    return bgAlpha;
}

void LiveDanmakuWindow::setPrevAlpha(int x)
{
    this->prevAlpha = x;
    update();
}

int LiveDanmakuWindow::getPrevAlpha() const
{
    return prevAlpha;
}

