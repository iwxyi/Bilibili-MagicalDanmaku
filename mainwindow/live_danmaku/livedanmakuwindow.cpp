#include <QFileDialog>
#include <QMessageBox>
#include <QMetaEnum>
#include "livedanmakuwindow.h"
#include "facilemenu.h"
#include "guardonlinedialog.h"
#include "tx_nlp.h"

QT_BEGIN_NAMESPACE
    extern Q_WIDGETS_EXPORT void qt_blurImage( QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

LiveDanmakuWindow::LiveDanmakuWindow(QWidget *parent)
    : QWidget(nullptr)
{
    this->setWindowTitle("实时弹幕");
    this->setMinimumSize(45,45);                        //设置最小尺寸
#ifdef Q_OS_ANDROID
    this->setAttribute(Qt::WA_TranslucentBackground, false);
#else
    if (us->value("livedanmakuwindow/jiWindow", false).toBool())
    {
        this->setWindowFlags(Qt::FramelessWindowHint);
        this->setAttribute(Qt::WA_TranslucentBackground, false);
    }
    else
    {
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);      //设置为无边框置顶窗口
        this->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明
    }

    bool onTop = us->value("livedanmakuwindow/onTop", true).toBool();
    if (onTop)
        this->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    if (us->value("livedanmakuwindow/transMouse", false).toBool())
    {
        this->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        this->setWindowFlag(Qt::WindowStaysOnTopHint, !onTop);
        this->setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
        QTimer::singleShot(100, [=]{
            emit signalTransMouse(true);
        });
    }
#endif

    QFontMetrics fm(this->font());
    fontHeight = fm.height();
    lineSpacing = fm.lineSpacing();
    srand(time(0));

    moveBar = new QWidget(this);
    moveBar->setStyleSheet("QWidget { background: rgba(128, 128, 128, 32); border-radius: 2; }"
                           "QWidget::hover { background: rgba(128, 128, 128, 128); }");
    moveBar->setMinimumWidth(boundaryWidth);
    moveBar->setMinimumHeight(boundaryWidth / 4);
    moveBar->setMaximumHeight(boundaryWidth / 2);
    moveBar->setCursor(Qt::SizeAllCursor);

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
        bool showEdit = us->value("livedanmakuwindow/sendEdit", false).toBool();
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
            if (us->value("livedanmakuwindow/sendOnce", false).toBool())
                returnPrevWindow();
            lineEdit->setPlaceholderText("");
        }
    });
    connect(lineEdit, &QLineEdit::customContextMenuRequested, this, &LiveDanmakuWindow::showEditMenu);
    if (!us->value("livedanmakuwindow/sendEdit", false).toBool())
        lineEdit->hide();
#ifdef Q_OS_LINUX

#endif
#if defined(ENABLE_SHORTCUT)
    editShortcut = new QxtGlobalShortcut(this);
    QString def_key = us->value("livedanmakuwindow/shortcutKey", "shift+alt+D").toString();
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
#if defined(Q_OS_WIN32)
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
    if (!us->value("livedanmakuwindow/sendEditShortcut", false).toBool())
        editShortcut->setEnabled(false);
#endif

    // 颜色
    nameColor = qvariant_cast<QColor>(us->value("livedanmakuwindow/nameColor", QColor(247, 110, 158)));
    msgColor = qvariant_cast<QColor>(us->value("livedanmakuwindow/msgColor", QColor(Qt::white)));
    bgColor = qvariant_cast<QColor>(us->value("livedanmakuwindow/bgColor", QColor(0x88, 0x88, 0x88, 0x32)));
    hlColor = qvariant_cast<QColor>(us->value("livedanmakuwindow/hlColor", QColor(255, 0, 0)));
    QString fontString = us->value("livedanmakuwindow/font").toString();
    if (!fontString.isEmpty())
        danmakuFont.fromString(fontString);
    labelStyleSheet = us->value("livedanmakuwindow/labelStyleSheet").toString();

    // 背景图片
    pictureFilePath = us->value("livedanmakuwindow/pictureFilePath", "").toString();
    pictureDirPath = us->value("livedanmakuwindow/pictureDirPath", "").toString();
    pictureAlpha = us->value("livedanmakuwindow/pictureAlpha", 64).toInt();
    pictureBlur = us->value("livedanmakuwindow/pictureBlur", 0).toInt();
    switchBgTimer = new QTimer(this);
    int switchInterval = us->value("livedanmakuwindow/pictureInterval", 20).toInt();
    switchBgTimer->setInterval(switchInterval * 1000);
    connect(switchBgTimer, &QTimer::timeout, this, [=]{
        selectBgPicture();
    });
    aspectRatio = us->value("livedanmakuwindow/aspectRatio", false).toBool();
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
    ignoreDanmakuColors = us->value("livedanmakuwindow/ignoreColor").toString().split(";");

    // 模式
    simpleMode = us->value("livedanmakuwindow/simpleMode", simpleMode).toBool();
    chatMode = us->value("livedanmakuwindow/chatMode", chatMode).toBool();
    allowH5 = us->value("livedanmakuwindow/allowH5", allowH5).toBool();
    blockComingMsg = us->value("livedanmakuwindow/blockComingMsg", blockComingMsg).toBool();
    blockSpecialGift = us->value("livedanmakuwindow/blockSpecialGift", blockSpecialGift).toBool();
    blockCommonNotice = us->value("livedanmakuwindow/blockCommonNotice", blockCommonNotice).toBool();

    headDir = rt->dataPath + "headers/";
    QDir().mkpath(headDir);

    statusLabel = new QLabel(this);
    statusLabel->hide();
    statusLabel->setStyleSheet("color:" + QVariant(msgColor).toString() + ";");
    statusLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(statusLabel, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showPkMenu()));
}

LiveDanmakuWindow::~LiveDanmakuWindow()
{
    us->setValue("livedanmakuwindow/geometry", this->saveGeometry());
}

void LiveDanmakuWindow::showEvent(QShowEvent *event)
{
#ifndef Q_OS_ANDROID
    // 如果是安卓，就显示全屏吧！
    restoreGeometry(us->value("livedanmakuwindow/geometry").toByteArray());
#endif
    enableAnimation = true;
    QWidget::showEvent(event);
}

void LiveDanmakuWindow::hideEvent(QHideEvent *event)
{
    us->setValue("livedanmakuwindow/geometry", this->saveGeometry());
    enableAnimation = false;
    QWidget::hideEvent(event);
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
    return false;         //此处返回false，留给其他事件处理器处理
#else
    return QWidget::nativeEvent(eventType, message, result);
#endif
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
    moveBar->move((this->width() - moveBar->width()) / 2, (boundaryWidth - moveBar->height()) / 2);
    moveBar->setFixedWidth(qMax(boundaryWidth * 3, this->width() / 6));

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
#ifndef Q_OS_ANDROID
    path.addRect(rect());
#else
    path.addRoundedRect(rect(), 5, 5);
#endif
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

QString LiveDanmakuWindow::filterH5(QString msg)
{
    return msg.replace("<", "&lt;")
            .replace(">", "&gt;");
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
                && !danmaku.is(MSG_BLOCK)
                && !danmaku.is(MSG_SUPER_CHAT))
            return ;
    }
    if (blockComingMsg && (danmaku.is(MSG_WELCOME) || danmaku.is(MSG_WELCOME_GUARD)))
        return ;
    if (blockSpecialGift && danmaku.is(MSG_DANMAKU)
            && blockedTexts.size() && blockedTexts.contains(danmaku.getText()))
        return ;

    bool scrollEnd = listWidget->verticalScrollBar()->sliderPosition()
            >= listWidget->verticalScrollBar()->maximum()-lineEdit->height()*2;

    QWidget* widget = new QWidget(listWidget);
    PortraitLabel* portrait = new PortraitLabel(PORTRAIT_SIDE, widget);
    QLabel* label = new QLabel(widget);
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->addWidget(portrait);
    layout->addWidget(label);

    static auto msgTypeString = [=](MessageType type) -> QString {
        switch (type) {
        case MSG_DEF:
            return "default";
        case MSG_DANMAKU:
            return "danmaku";
        case MSG_GIFT:
            return "gift";
        case MSG_WELCOME:
            return "welcome";
        case MSG_DIANGE:
            return "order-song";
        case MSG_GUARD_BUY:
            return "guard-buy";
        case MSG_WELCOME_GUARD:
            return "welcome-guard";
        case MSG_FANS:
            return "fans";
        case MSG_ATTENTION:
            return "attention";
        case MSG_BLOCK:
            return "block";
        case MSG_MSG:
            return "msg";
        case MSG_SHARE:
            return "share";
        case MSG_PK_BEST:
            return "pk-best";
        case MSG_SUPER_CHAT:
            return "super-chat";
        case MSG_EXTRA:
            return "extra";
        }
    };

    label->setObjectName(msgTypeString(danmaku.getMsgType()));
    if (!labelStyleSheet.isEmpty())
        label->setStyleSheet(labelStyleSheet);
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
    if ((danmaku.is(MSG_DANMAKU) || danmaku.is(MSG_SUPER_CHAT)) && !simpleMode) // 只显示弹幕的数据
    {
        QString path = headPath(danmaku.getUid());
        bool hasPortrait = QFileInfo(path).exists();
        if (hasPortrait)
            portrait->setPixmap(QPixmap(path));
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
                        qInfo() << "检测到外语，自动翻译";
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
             || msgType == MSG_BLOCK
             || (msgType == MSG_ATTENTION && danmaku.getSpecial()))
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

            if (enableAnimation)
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
    // qDebug() << "忽略没找到的要删除的item" << danmaku.toString();
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
    if (us->careUsers.contains(danmaku.getUid()))
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
    if (msgType == MSG_DANMAKU || msgType == MSG_SUPER_CHAT)
    {
        // 新人：0级，3次以内
        if (msgType == MSG_DANMAKU && newbieTip && !danmaku.isPkLink())
        {
            int count = us->danmakuCounts->value("danmaku/"+snum(danmaku.getUid())).toInt();
            if (danmaku.getLevel() == 0 && count <= 1 && danmaku.getMedalLevel() <= 1)
            {
                if (simpleMode)
                    nameText = "[新] " + nameText;
                else
                    nameText = "<font color='red'>[新] </font>" + nameText;
            }
            else if (danmaku.getLevel() > 0 && count <= 1) // 10句话以内
            {
                if (simpleMode)
                    nameText = "[初] " + nameText;
                else
                    nameText = "<font color='green'>[初] </font>" + nameText;
            }
        }

        // 安全过滤
        if (!allowH5)
            msg = filterH5(msg);
        // 彩色消息
        QString colorfulMsg = msg;
        if (simpleMode)
        {}
        else if (danmaku.isNoReply() || danmaku.isPkLink()) // 灰色
        {
            colorfulMsg = "<font color='gray'>"
                    + msg + "</font> ";
        }
        else if (!isBlankColor(danmaku.getTextColor())
                 && !ignoreDanmakuColors.contains(danmaku.getTextColor()))
        {
            colorfulMsg = "<font color='" + danmaku.getTextColor() + "'>"
                    + msg + "</font> ";
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

        // 醒目留言
        if (msgType == MSG_SUPER_CHAT && !simpleMode)
        {
            qint64 coin = danmaku.getTotalCoin();
            text += " <hr><center><span style='font-weight: bold;'>￥ ";
            if (coin % 1000 == 0)
                text += snum(coin / 1000);
            else
                text += QString::number(coin / 1000.0, 'f', 1);
            text += " 元</span></center>";
        }
    }
    else if (msgType == MSG_GIFT)
    {
        text = QString("<center>%1 赠送 %2")
                .arg(nameText)
                .arg(simpleMode ? danmaku.getGiftName() : "<font color='" + QVariant(this->nameColor).toString() + "'>" + danmaku.getGiftName() + "</font>");
        if (danmaku.getNumber() > 1)
            text += "×" + snum(danmaku.getNumber());
        text += "</center>";
        if (danmaku.isGoldCoin() && !simpleMode)
        {
            qint64 coin = danmaku.getTotalCoin();
            text += " <hr><center><span style='font-weight: bold;'>￥ ";
            if (coin % 1000 == 0)
                text += snum(coin / 1000);
            else
                text += QString::number(coin / 1000.0, 'f', 1);
            text += " 元</span></center>";
        }
    }
    else if (msgType == MSG_GUARD_BUY)
    {
        QString modify;
        QString board = "上船";
        if (danmaku.getFirst() == 1) // 初次
        {
            if (simpleMode)
                modify = "<font color='gray'>[初] </font>";
            else
                modify = "<font color='green'>[初] </font>";
        }
        else if (danmaku.getFirst() == 2) // 重新上传
        {
            if (simpleMode)
                modify = "<font color='gray'>[回] </font>";
            else
                modify = "<font color='red'>[回] </font>";
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
            if (danmaku.isAdmin())
                text += "[房] ";
            if (danmaku.isGuard())
                text += danmaku.getGuardName() + " ";
            text += QString("%1 进入直播间")
                    .arg(nameText);
        }
        else
        {
            text += nameText + " ";
            if (danmaku.getNumber() > 0) // 不包括这一次的
                text += simpleMode
                        ? QString("进入 %1次").arg(danmaku.getNumber())
                        : QString("进入 <font color='gray'>%1次</font>").arg(danmaku.getNumber());
            else if (!danmaku.getSpreadDesc().isEmpty())
                text += simpleMode
                        ? "进入"
                        : "<font color='gray'>进入</font>";
            else
                text += simpleMode
                        ? "进入直播间"
                        : "<font color='gray'>进入直播间</font>";
        }

        // 推广
        if (!danmaku.getSpreadDesc().isEmpty())
        {
            text += " ";
            if (danmaku.getSpreadInfo().isEmpty())
                text += danmaku.getSpreadDesc();
            else if (simpleMode)
                text += " " + danmaku.getSpreadDesc();
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
        text = QString(simpleMode
                       ? "[粉丝] 粉丝数：%1%3, 粉丝团：%2%4"
                       :  "<font color='gray'>[粉丝]</font> 粉丝数：%1%3, 粉丝团：%2%4")
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
        text = QString(simpleMode ? "[%3] %1 %2" : "<font color='gray'>[%3]</font> %1 %2")
                .arg(nameText)
                .arg(danmaku.isAttention() ? "关注了主播" : "取消关注主播")
                .arg(danmaku.getSpecial() ? "特别关注" : "关注");
    }
    else if (msgType == MSG_BLOCK)
    {
        text = QString(simpleMode ? "[禁言] %1 被房管禁言" : "<font color='gray'>[禁言]</font> %1 被房管禁言")
                .arg(danmaku.getNickname());
    }
    else if (msgType == MSG_MSG)
    {
        text = QString(simpleMode ? "%1" : "<font color='gray'>%1</font>")
                .arg(danmaku.getText());
    }

    // 主播判断
    if (upUid && (danmaku.getUid() == upUid || (pkStatus && danmaku.getUid() == pkUid && !danmaku.is(MSG_MSG))))
        text = (simpleMode ? "[主播] " : "<font color='#F08080'>[主播]</font> ") + text;

    if (danmaku.isRobot())
        text = (simpleMode ? "[机] " : "<font color='#5E86C1'>[机]</font> ") + text;

    // 串门判断
    if (danmaku.isToView())
        text = "[串门] " + text;
    else if (danmaku.isOpposite())
        text = "[对面] " + text;

    // 消息同步
    if (danmaku.isPkLink()) // 这个最置顶前面
        text = (simpleMode ? "[同步] " : "<font color='gray'>[同步]</font> ") + text;

    // 文字与大小
    label->setText(text);
    label->adjustSize();
    widget->adjustSize();
    widget->layout()->activate();

    // item大小
    widget->resize(widget->sizeHint());
    item->setSizeHint(widget->sizeHint());

    // 动画
    if (enableAnimation)
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

void LiveDanmakuWindow::resetItemsStyleSheet()
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
        label->setStyleSheet(labelStyleSheet);
        label->adjustSize();
        widget->adjustSize();
        item->setSizeHint(widget->size());
    }
}

void LiveDanmakuWindow::mergeGift(LiveDanmaku danmaku, int delayTime)
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
        if (t + delayTime < time)
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

void LiveDanmakuWindow::removeAll()
{
    while (listWidget->count())
    {
        auto item = listWidget->item(0);
        listWidget->removeItemWidget(item);
        listWidget->takeItem(listWidget->row(item));
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
    QAction* actionCopyUid = new QAction(QIcon(":/icons/code"), "复制UID", this);
    QAction* actionCopyGiftId = new QAction(QIcon(":/icons/code"), "复制礼物ID", this);
    QAction* actionMedal = new QAction(QIcon(":/danmaku/medal"), "粉丝勋章", this);
    QAction* actionValue = new QAction(QIcon(":/icons/egg"), "礼物价值", this);
    QAction* actionHistory = new QAction(QIcon(":/danmaku/message"), "消息记录", this);
    QAction* actionFollow = new QAction(QIcon(":/danmaku/fans"), "粉丝数", this);
    QAction* actionView = new QAction(QIcon(":/danmaku/views"), "浏览量", this);

    QAction* actionAddCare = new QAction(QIcon(":/icons/heart"), "添加特别关心", this);
    QAction* actionStrongNotify = new QAction(QIcon(":/danmaku/notify"), "添加强提醒", this);
    QAction* actionSetName = new QAction(QIcon(":/danmaku/nick"), "设置专属昵称", this);
    QAction* actionUserMark = new QAction(QIcon(":/danmaku/mark"), "设置用户备注", this);
    QAction* actionSetGiftName = new QAction(QIcon(":/danmaku/gift"), "设置礼物别名", this);

    QAction* actionAddBlockTemp = new QAction(QIcon(":/danmaku/block2"), "禁言1小时", this);
    QAction* actionAddBlock = new QAction(QIcon(":/danmaku/block2"), "禁言720小时", this);
    QAction* actionDelBlock = new QAction(QIcon(":/danmaku/block2"), "取消禁言", this);
    QAction* actionEternalBlock = new QAction(QIcon(":/danmaku/block"), "永久禁言", this);
    QAction* actionCancelEternalBlock = new QAction(QIcon(":/danmaku/block"), "取消永久禁言", this);
    QAction* actionAddCloudShield = new QAction(QIcon(":/icons/cloud_block"), "添加云端屏蔽", this);

    QAction* actionNotWelcome = new QAction(QIcon(":/danmaku/welcome"), "不自动欢迎", this);
    QAction* actionNotReply = new QAction(QIcon(":/danmaku/reply"), "不AI回复", this);

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
    QAction* actionLabelStyleSheet = new QAction("标签样式", this);
    QAction* actionAllowH5 = new QAction("允许H5标签", this);

    QMenu* pictureMenu = new QMenu("背景图片", settingMenu);
    QAction* actionPictureSelect = new QAction("选择图片", this);
    QAction* actionPictureFolder = new QAction("文件夹轮播", this);
    QAction* actionPictureInterval = new QAction("轮播间隔", this);
    QAction* actionPictureRatio = new QAction("保持比例", this);
    QAction* actionPictureAlpha = new QAction("图片透明度", this);
    QAction* actionPictureBlur = new QAction("模糊半径", this);
    QAction* actionCancelPicture = new QAction("取消图片", this);

    QMenu* blockMenu = new QMenu("消息屏蔽", settingMenu);
    QAction* actionBlockComing = new QAction("屏蔽用户进入消息", this);
    QAction* actionBlockSpecialGift = new QAction("屏蔽节奏风暴/天选弹幕", this);

    QAction* actionSendMsg = new QAction("发送框", this);
    QAction* actionDialogSend = new QAction("快速触发", this);
    QAction* actionShortCut = new QAction("快捷键", this);
    QAction* actionSendOnce = new QAction("单次发送", this);
    QAction* actionSimpleMode  = new QAction("简约模式", this);
    QAction* actionChatMode  = new QAction("聊天模式", this);
    QAction* actionWindow = new QAction("窗口模式", this);
    QAction* actionOnTop = new QAction("置顶显示", this);
    QAction* actionTransMouse = new QAction("鼠标穿透", this);

    QAction* actionDelete = new QAction(QIcon(":/danmaku/delete"), "删除", this);
    QAction* actionHide = new QAction(QIcon(":/danmaku/hide"), "隐藏", this);

    actionNotWelcome->setCheckable(true);
    actionNotReply->setCheckable(true);
    actionRepeat->setDisabled(!danmaku.is(MSG_DANMAKU));
    actionSendMsg->setCheckable(true);
    actionSendMsg->setChecked(!lineEdit->isHidden());
    actionDialogSend->setToolTip("shift+alt+D 触发编辑框，输入后ESC返回原先窗口");
    actionDialogSend->setCheckable(true);
    actionDialogSend->setChecked(us->value("livedanmakuwindow/sendEditShortcut", false).toBool());
    actionSendOnce->setToolTip("发送后，返回原窗口（如果有）");
    actionSendOnce->setCheckable(true);
    actionSendOnce->setChecked(us->value("livedanmakuwindow/sendOnce", false).toBool());
    actionSimpleMode->setCheckable(true);
    actionSimpleMode->setChecked(simpleMode);
    actionChatMode->setCheckable(true);
    actionChatMode->setChecked(chatMode);
    actionWindow->setCheckable(true);
    actionWindow->setChecked(us->value("livedanmakuwindow/jiWindow", false).toBool());
    actionOnTop->setCheckable(true);
    actionOnTop->setChecked(us->value("livedanmakuwindow/onTop", true).toBool());
    actionTransMouse->setCheckable(true);
    actionTransMouse->setChecked(us->value("livedanmakuwindow/transMouse", false).toBool());
    actionPictureSelect->setCheckable(true);
    actionPictureSelect->setChecked(!pictureFilePath.isEmpty());
    actionPictureFolder->setCheckable(true);
    actionPictureFolder->setChecked(!pictureDirPath.isEmpty());
    actionPictureRatio->setCheckable(true);
    actionPictureRatio->setChecked(aspectRatio);
    actionPictureBlur->setCheckable(true);
    actionPictureBlur->setChecked(pictureBlur);
    actionAllowH5->setCheckable(true);
    actionAllowH5->setChecked(allowH5);
    actionBlockComing->setCheckable(true);
    actionBlockComing->setChecked(blockComingMsg);
    actionBlockSpecialGift->setCheckable(true);
    actionBlockSpecialGift->setChecked(blockSpecialGift);

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
        if (danmaku.is(MSG_DANMAKU) || danmaku.is(MSG_SUPER_CHAT))
        {
            actionUserInfo->setText(actionUserInfo->text() +  "：LV" + snum(danmaku.getLevel()));
            actionHistory->setText("消息记录：" + snum(us->danmakuCounts->value("danmaku/"+snum(uid)).toInt()) + "条");
        }
        else if (danmaku.getMsgType() == MSG_GIFT || danmaku.getMsgType() == MSG_GUARD_BUY)
        {
            actionHistory->setText("送礼总额：" + snum(us->danmakuCounts->value("gold/"+snum(uid)).toLongLong()/1000) + "元");
            if (danmaku.is(MSG_GUARD_BUY))
                actionMedal->setText("船员数量：" + snum(us->currentGuards.size()));
            actionCopyGiftId->setText("礼物ID：" + snum(danmaku.getGiftId()));
        }
        else if (danmaku.is(MSG_WELCOME_GUARD))
        {
            actionMedal->setText("船员数量：" + snum(us->currentGuards.size()));
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
            if (us->giftAlias.contains(danmaku.getGiftId()))
                actionSetGiftName->setText("礼物别名：" + us->giftAlias.value(danmaku.getGiftId()));
        }

        if (us->careUsers.contains(uid))
            actionAddCare->setText("移除特别关心");
        if (us->strongNotifyUsers.contains(uid))
            actionStrongNotify->setText("移除强提醒");
        if (us->localNicknames.contains(uid))
            actionSetName->setText("专属昵称：" + us->localNicknames.value(uid));
        if (us->userMarks->contains("base/" + snum(uid)))
            actionSetName->setText("备注：" + us->userMarks->value("base/" + snum(uid)).toString());
        actionNotWelcome->setChecked(us->notWelcomeUsers.contains(uid));
        actionNotReply->setChecked(us->notReplyUsers.contains(uid));

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
        actionCopyUid->setEnabled(false);
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
        actionUserMark->setEnabled(false);
        actionNotWelcome->setEnabled(false);
        actionNotReply->setEnabled(false);
        actionFollow->setEnabled(false);
        actionView->setEnabled(false);
    }
    if (!danmaku.is(MSG_DANMAKU))
        actionAddCloudShield->setEnabled(false);

    if (!item)
    {
        operMenu->setEnabled(false);
        actionCopyUid->setEnabled(false);
        actionCopyGiftId->setEnabled(false);
        actionAddCare->setEnabled(false);
        actionStrongNotify->setEnabled(false);
        actionSetName->setEnabled(false);
        actionUserMark->setEnabled(false);
        actionCopy->setEnabled(false);
        actionSearch->setEnabled(false);
        actionTranslate->setEnabled(false);
        actionReply->setEnabled(false);
        actionFreeCopy->setEnabled(false);
        actionDelete->setEnabled(false);
        actionIgnoreColor->setEnabled(false);
        actionAddCloudShield->setEnabled(false);
    }

    menu->addAction(actionUserInfo);
    menu->addAction(actionCopyUid);
    if (danmaku.getGiftId())
        menu->addAction(actionCopyGiftId);
    menu->addAction(actionMedal);
    if (danmaku.is(MSG_GIFT))
        menu->addAction(actionValue);
    menu->addAction(actionHistory);
    menu->addAction(actionFollow);
    menu->addAction(actionView);
    if (enableBlock && uid)
    {
        menu->addSeparator();
        if (!us->userBlockIds.contains(uid) && danmaku.getMsgType() != MSG_BLOCK)
        {
            menu->addAction(actionAddBlockTemp);
            menu->addAction(actionAddBlock);
            if (us->eternalBlockUsers.contains(EternalBlockUser(uid, roomId)))
                menu->addAction(actionCancelEternalBlock);
            else
                menu->addAction(actionEternalBlock);
        }
        else
        {
            menu->addAction(actionDelBlock);
        }
        menu->addAction(actionAddCloudShield);
    }

    menu->addSeparator();
    menu->addAction(actionAddCare);
    menu->addAction(actionStrongNotify);
    menu->addAction(actionSetName);
    menu->addAction(actionUserMark);
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

    blockMenu->addAction(actionBlockComing);
    blockMenu->addAction(actionBlockSpecialGift);

    settingMenu->addMenu(blockMenu);
    settingMenu->addSeparator();
    settingMenu->addAction(actionNameColor);
    settingMenu->addAction(actionMsgColor);
    settingMenu->addAction(actionBgColor);
    settingMenu->addAction(actionHlColor);
    settingMenu->addAction(actionFont);
    settingMenu->addAction(actionLabelStyleSheet);
    settingMenu->addAction(actionAllowH5);
    settingMenu->addMenu(pictureMenu);
    settingMenu->addSeparator();
    settingMenu->addAction(actionSendMsg);
#if defined(ENABLE_SHORTCUT)
    settingMenu->addAction(actionDialogSend);
    settingMenu->addAction(actionShortCut);
#endif
    settingMenu->addAction(actionSendOnce);
    settingMenu->addSeparator();
    settingMenu->addAction(actionSimpleMode);
    settingMenu->addAction(actionChatMode);
    settingMenu->addAction(actionWindow);
    settingMenu->addAction(actionOnTop);
    settingMenu->addAction(actionTransMouse);

    menu->addAction(actionDelete);
    menu->addAction(actionHide);

    connect(actionNameColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(nameColor, this, "选择昵称颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != nameColor)
        {
            us->setValue("livedanmakuwindow/nameColor", nameColor = c);
            resetItemsText();
        }
    });
    connect(actionMsgColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(msgColor, this, "选择文字颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != msgColor)
        {
            us->setValue("livedanmakuwindow/msgColor", msgColor = c);
            resetItemsTextColor();
        }
        if (statusLabel)
        {
            statusLabel->setStyleSheet("color:" + QVariant(msgColor).toString() + ";");
        }
    });
    connect(actionBgColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(bgColor, this, "选择背景颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != bgColor)
        {
            us->setValue("livedanmakuwindow/bgColor", bgColor = c);
            update();
        }
    });
    connect(actionHlColor, &QAction::triggered, this, [=]{
        QColor c = QColorDialog::getColor(hlColor, this, "选择高亮颜色（实时提示、特别关心）", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != hlColor)
        {
            us->setValue("livedanmakuwindow/hlColor", hlColor = c);
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
        us->setValue("livedanmakuwindow/font", danmakuFont.toString());
        resetItemsFont();
    });
    connect(actionLabelStyleSheet, &QAction::triggered, this, [=]{
        bool ok;
        QString ss = QInputDialog::getText(this, "标签样式", "请输入标签样式，支持CSS，将影响所有弹幕\n可通过CSS选择器来筛选特定样式", QLineEdit::Normal, labelStyleSheet, &ok);
        if (!ok)
            return ;
        labelStyleSheet = ss;
        us->setValue("livedanmakuwindow/labelStyleSheet", labelStyleSheet);
        resetItemsStyleSheet();
    });
    connect(actionAllowH5, &QAction::triggered, this, [=]{
        allowH5 = !allowH5;
        us->setValue("livedanmakuwindow/allowH5", allowH5);
    });
    connect(actionBlockComing, &QAction::triggered, this, [=]{
        blockComingMsg = !blockComingMsg;
        us->setValue("livedanmakuwindow/blockComingMsg", blockComingMsg);
    });
    connect(actionBlockSpecialGift, &QAction::triggered, this, [=]{
        blockSpecialGift = !blockSpecialGift;
        us->setValue("livedanmakuwindow/blockSpecialGift", blockSpecialGift);
    });
    connect(actionAddCare, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;

        if (us->careUsers.contains(danmaku.getUid())) // 已存在，移除
        {
            us->careUsers.removeOne(danmaku.getUid());
            setItemWidgetText(item);
        }
        else // 添加特别关心
        {
            us->careUsers.append(danmaku.getUid());
            highlightItemText(item);
            emit signalMarkUser(danmaku.getUid());
        }

        // 保存特别关心
        QStringList ress;
        foreach (qint64 uid, us->careUsers)
            ress << QString::number(uid);
        us->setValue("danmaku/careUsers", ress.join(";"));
    });
    connect(actionStrongNotify, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;

        if (us->strongNotifyUsers.contains(danmaku.getUid())) // 已存在，移除
        {
            us->strongNotifyUsers.removeOne(danmaku.getUid());
            setItemWidgetText(item);
        }
        else // 添加强提醒
        {
            us->strongNotifyUsers.append(danmaku.getUid());
            highlightItemText(item);
            emit signalMarkUser(danmaku.getUid());
        }

        // 保存特别关心
        QStringList ress;
        foreach (qint64 uid, us->strongNotifyUsers)
            ress << QString::number(uid);
        us->setValue("danmaku/strongNotifyUsers", ress.join(";"));
    });
    connect(actionSetName, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;

        // 设置昵称
        bool ok = false;
        QString name;
        if (us->localNicknames.contains(uid))
            name = us->localNicknames.value(uid);
        else
            name = danmaku.getNickname();
        QString tip = "设置【" + danmaku.getNickname() + "】的专属昵称\n将影响机器人的欢迎/感谢弹幕";
        if (us->localNicknames.contains(uid))
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
            if (us->localNicknames.contains(uid))
                us->localNicknames.remove(uid);
        }
        else
        {
            us->localNicknames[uid] = name;
            emit signalMarkUser(danmaku.getUid());
        }

        // 保存本地昵称
        QStringList ress;
        auto it = us->localNicknames.begin();
        while (it != us->localNicknames.end())
        {
            ress << QString("%1=>%2").arg(it.key()).arg(it.value());
            it++;
        }
        us->setValue("danmaku/localNicknames", ress.join(";"));
    });
    connect(actionUserMark, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;

        // 设置昵称
        bool ok = false;
        QString mark;
        if (us->userMarks->contains("base/" + snum(uid)))
            mark = us->userMarks->value("base/" + snum(uid), "").toString();
        QString tip = "设置【" + danmaku.getNickname() + "】的备注\n可通过%umark%放入至弹幕中";
        mark = QInputDialog::getText(this, "用户备注", tip, QLineEdit::Normal, mark, &ok);
        if (!ok)
            return ;
        if (mark.isEmpty())
        {
            if (us->userMarks->contains("base/" + snum(uid)))
                us->userMarks->remove("base/" + snum(uid));
        }
        else
        {
            us->userMarks->setValue("base/" + snum(uid), mark);
            emit signalMarkUser(danmaku.getUid());
        }
    });
    connect(actionNotWelcome, &QAction::triggered, this, [=]{
        if (us->notWelcomeUsers.contains(uid))
            us->notWelcomeUsers.removeOne(uid);
        else
            us->notWelcomeUsers.append(uid);

        QStringList ress;
        foreach (qint64 uid, us->notWelcomeUsers)
            ress << QString::number(uid);
        us->setValue("danmaku/notWelcomeUsers", ress.join(";"));
    });
    connect(actionNotReply, &QAction::triggered, this, [=]{
        if (us->notReplyUsers.contains(uid))
            us->notReplyUsers.removeOne(uid);
        else
            us->notReplyUsers.append(uid);

        QStringList ress;
        foreach (qint64 uid, us->notReplyUsers)
            ress << QString::number(uid);
        us->setValue("danmaku/notReplyUsers", ress.join(";"));
    });
    connect(actionSetGiftName, &QAction::triggered, this, [=]{
        if (listWidget->currentItem() != item) // 当前项变更
            return ;

        // 设置别名
        int giftId = danmaku.getGiftId();
        bool ok = false;
        QString name;
        if (us->giftAlias.contains(giftId))
            name = us->giftAlias.value(giftId);
        else
            name = danmaku.getGiftName();
        QString tip = "设置【" + danmaku.getGiftName() + "】的别名\n只影响机器人的感谢弹幕";
        if (us->giftAlias.contains(giftId))
        {
            tip += "\n清空则取消别名，还原礼物原始名字";
        }
        name = QInputDialog::getText(this, "礼物别名", tip, QLineEdit::Normal, name, &ok);
        if (!ok)
            return ;
        if (name.isEmpty())
        {
            if (us->giftAlias.contains(giftId))
                us->giftAlias.remove(giftId);
        }
        else
        {
            us->giftAlias[giftId] = name;
        }

        // 保存本地昵称
        QStringList ress;
        auto it = us->giftAlias.begin();
        while (it != us->giftAlias.end())
        {
            ress << QString("%1=>%2").arg(it.key()).arg(it.value());
            it++;
        }
        us->setValue("danmaku/giftNames", ress.join(";"));
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

        us->setValue("livedanmakuwindow/ignoreColor", ignoreDanmakuColors.join(";"));
        resetItemsText();
    });
    connect(actionSendMsg, &QAction::triggered, this, [=]{
        if (lineEdit->isHidden())
            lineEdit->show();
        else
            lineEdit->hide();
        us->setValue("livedanmakuwindow/sendEdit", !lineEdit->isHidden());
    });
#if defined(ENABLE_SHORTCUT)
    connect(actionDialogSend, &QAction::triggered, this, [=]{
        bool enable = !us->value("livedanmakuwindow/sendEditShortcut", false).toBool();
        us->setValue("livedanmakuwindow/sendEditShortcut", enable);
        editShortcut->setEnabled(enable);
    });
    connect(actionShortCut, &QAction::triggered, this, [=]{
        QString def_key = us->value("livedanmakuwindow/shortcutKey", "shift+alt+D").toString();
        QString key = QInputDialog::getText(this, "发弹幕快捷键", "设置显示弹幕发送框的快捷键", QLineEdit::Normal, def_key);
        if (!key.isEmpty())
        {
            if (editShortcut->setShortcut(QKeySequence(key)))
                us->setValue("livedanmakuwindow/shortcutKey", key);
        }
    });
#endif
    connect(actionSendOnce, &QAction::triggered, this, [=]{
        bool enable = !us->value("livedanmakuwindow/sendOnce", false).toBool();
        us->setValue("livedanmakuwindow/sendOnce", enable);
    });
    connect(actionWindow, &QAction::triggered, this, [=]{
        bool enable = !us->value("livedanmakuwindow/jiWindow", false).toBool();
        us->setValue("livedanmakuwindow/jiWindow", enable);
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
        restart();
    });
    connect(actionOnTop, &QAction::triggered, this, [=]{
        bool onTop = !us->value("livedanmakuwindow/onTop", true).toBool();
        this->setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
        us->setValue("livedanmakuwindow/onTop", onTop);
        qDebug() << "置顶显示：" << onTop;
        this->show();
    });
    connect(actionTransMouse, &QAction::triggered, this, [=]{
        bool trans = !us->value("livedanmakuwindow/transMouse", false).toBool();
        setAttribute(Qt::WA_TransparentForMouseEvents, trans);
        us->setValue("livedanmakuwindow/transMouse", trans);
        emit signalTransMouse(trans);

        // 需要切换一遍置顶才生效
        bool onTop = us->value("livedanmakuwindow/onTop", true).toBool();
        this->setWindowFlag(Qt::WindowStaysOnTopHint, !onTop);
        this->setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
        this->show();

        if (us->value("ask/transMouse", true).toBool())
        {
            QMessageBox::information(this, "鼠标穿透", "开启鼠标穿透后，弹幕姬中所有内容都将无法点击\n\n在程序主界面的“弹幕”选项卡中点击“关闭鼠标穿透”");
            us->setValue("ask/transMouse", false);
        }
    });
    connect(actionSimpleMode, &QAction::triggered, this, [=]{
        us->setValue("livedanmakuwindow/simpleMode", simpleMode = !simpleMode);
    });
    connect(actionChatMode, &QAction::triggered, this, [=]{
        us->setValue("livedanmakuwindow/chatMode", chatMode = !chatMode);
    });
    connect(actionUserInfo, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/" + snum(uid)));
    });
    connect(actionCopyUid, &QAction::triggered, this, [=]{
        QApplication::clipboard()->setText(snum(uid));
    });
    connect(actionCopyGiftId, &QAction::triggered, this, [=]{
        QApplication::clipboard()->setText(snum(danmaku.getGiftId()));
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
        us->setValue("danmaku/liveWindow", false);
    });

    connect(actionPictureSelect, &QAction::triggered, this, [=]{
        QString path = QFileDialog::getOpenFileName(this, tr("请选择图片文件"), pictureFilePath, tr("Images (*.png *.xpm *.jpg)"));
        if (path.isEmpty())
            return ;

        pictureFilePath = path;
        pictureDirPath = "";
        us->setValue("livedanmakuwindow/pictureFilePath", pictureFilePath);
        us->setValue("livedanmakuwindow/pictureDirPath", pictureDirPath);
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
        us->setValue("livedanmakuwindow/pictureFilePath", pictureFilePath);
        us->setValue("livedanmakuwindow/pictureDirPath", pictureDirPath);
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
        us->setValue("livedanmakuwindow/pictureInterval", val);
        update();
    });
    connect(actionPictureAlpha, &QAction::triggered, this, [=]{
        bool ok = false;
        int val = QInputDialog::getInt(this, "图片透明度", "请输入背景图片透明度，0~255，越低越透明", pictureAlpha, 0, 255, 32, &ok);
        if (!ok)
            return ;
        pictureAlpha = val;
        us->setValue("livedanmakuwindow/pictureAlpha", pictureAlpha);
        update();
    });
    connect(actionPictureRatio, &QAction::triggered, this, [=]{
        aspectRatio = !aspectRatio;
        us->setValue("livedanmakuwindow/aspectRatio", aspectRatio);
        update();
    });
    connect(actionPictureBlur, &QAction::triggered, this, [=]{
        bool ok = false;
        int val = QInputDialog::getInt(this, "图片模糊半径", "请输入背景图片模糊半径，越大越模糊", pictureBlur, 0, 2000, 32, &ok);
        if (!ok)
            return ;
        pictureBlur = val;
        us->setValue("livedanmakuwindow/pictureBlur", pictureBlur);
        selectBgPicture();
        update();
    });
    connect(actionCancelPicture, &QAction::triggered, this, [=]{
        pictureFilePath = "";
        pictureDirPath = "";
        us->setValue("livedanmakuwindow/pictureFilePath", pictureFilePath);
        us->setValue("livedanmakuwindow/pictureDirPath", pictureDirPath);
        switchBgTimer->stop();
        selectBgPicture();
        update();
    });
    connect(actionAddCloudShield, &QAction::triggered, this, [=]{
        QString text = danmaku.getText();
        bool ok;
        text = QInputDialog::getText(this, "添加云端屏蔽词", "将该弹幕中的敏感词添加到直播间屏蔽词\n所有支持云端屏蔽词的直播间都将同步该词", QLineEdit::Normal, text, &ok);
        if (!ok)
            return ;

        if (text.length() > 15)
        {
            QMessageBox::information(this, "添加云端屏蔽词", "屏蔽词长度不能超过15字，请重试");
            return ;
        }

        emit signalAddCloudShieldKeyword(text);
    });

    menu->exec(QCursor::pos());

    menu->deleteLater();
    actionCopyUid->deleteLater();
    actionCopyGiftId->deleteLater();
    actionMsgColor->deleteLater();
    actionBgColor->deleteLater();
    actionHlColor->deleteLater();
    actionMedal->deleteLater();
    actionValue->deleteLater();
    actionAddCare->deleteLater();
    actionSetName->deleteLater();
    actionUserMark->deleteLater();
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
    actionTransMouse->deleteLater();
    actionPictureSelect->deleteLater();
    actionPictureFolder->deleteLater();
    actionPictureAlpha->deleteLater();
    actionPictureInterval->deleteLater();
    actionPictureRatio->deleteLater();
    actionPictureBlur->deleteLater();
    actionCancelPicture->deleteLater();
    actionAddCloudShield->deleteLater();
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
    QAction* actionOnline = new QAction(QIcon(":/danmaku/fans"), "在线舰长", this);

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
    menu->addAction(actionOnline);

    showPkLevelInAction(pkRoomId, actionUser, actionRank);
    showFollowCountInAction(pkUid, actionAttention, actionFans);
    showViewCountInAction(pkUid, actionView, actionRead, actionLike);
    showGuardInAction(pkRoomId, pkUid, actionGuard);

    connect(actionUser, &QAction::triggered, this, [=]{
        QDesktopServices::openUrl(QUrl("https://space.bilibili.com/" + snum(pkUid)));
    });
    connect(actionRank, &QAction::triggered, this, [=]{
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
    connect(actionOnline, &QAction::triggered, this, [=]{
        GuardOnlineDialog* god = new GuardOnlineDialog(us, snum(pkRoomId), snum(pkUid), this);
        god->show();
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
    actionOnline->deleteLater();
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
    QString url = "http://translate.google.com/translate_a/single?client=gtx&dt=t&dj=1&ie=UTF-8&sl=auto&tl=zh_cn&q="+msg;
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

        qInfo() << "翻译：" << msg << " => " << trans;
        item->setData(DANMAKU_TRANS_ROLE, trans);

        adjustItemTextDynamic(item);
    });
}

void LiveDanmakuWindow::setAIReply(bool reply)
{
    this->aiReply = reply;
    // qInfo() << "智能闲聊开关：" << reply;
}

/// 腾讯AI开放平台 https://ai.qq.com/console/home
void LiveDanmakuWindow::startReply(QListWidgetItem *item)
{
    auto danmaku = LiveDanmaku::fromDanmakuJson(item->data(DANMAKU_JSON_ROLE).toJsonObject());
    qint64 uid = danmaku.getUid();
    if (!uid || us->notReplyUsers.contains(uid))
        return ;
    QString msg = danmaku.getText();
    if (msg.isEmpty())
        return ;
    // 优化消息文本
    msg.replace(QRegularExpression("\\s+"), "，");

    TxNlp::instance()->chat(msg, [=](QString answer){
        if (answer.isEmpty())
            return ;

        emit signalAIReplyed(answer, uid);

        if (!isItemExist(item))
            return ;

        qInfo() << "回复：" << msg << " => " << answer;
        item->setData(DANMAKU_REPLY_ROLE, answer);

        adjustItemTextDynamic(item);
    }, 0);
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

void LiveDanmakuWindow::releaseLiveData(bool prepare)
{
    QDir(headDir).removeRecursively();
    QDir().mkpath(headDir);

    hideStatusText();
    setIds(0, 0);

    if (!prepare)
    {
        // 清空所有的弹幕
        while (listWidget->count())
        {
            auto item = listWidget->item(0);
            listWidget->removeItemWidget(item);
//            listWidget->itemWidget(item)->deleteLater();
            delete item;
        }
    }

    blockedTexts.clear();
}

void LiveDanmakuWindow::closeTransMouse()
{
    const bool trans = false;
    setAttribute(Qt::WA_TransparentForMouseEvents, trans);
    us->setValue("livedanmakuwindow/transMouse", trans);
    emit signalTransMouse(trans);

    restart();
    /* bool onTop = settings->value("livedanmakuwindow/onTop", true).toBool();
    this->setWindowFlag(Qt::WindowStaysOnTopHint, !onTop);
    this->setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
    this->show(); */
}

void LiveDanmakuWindow::restart()
{
#if defined(ENABLE_SHORTCUT)
        editShortcut->setShortcut(QKeySequence(""));
        editShortcut->setDisabled(true);
        delete editShortcut;
        editShortcut = nullptr;
#endif
        emit signalChangeWindowMode();
}

void LiveDanmakuWindow::addBlockText(QString text)
{
    blockedTexts.append(text);
}

void LiveDanmakuWindow::removeBlockText(QString text)
{
    blockedTexts.removeOne(text);
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

    if (false && enableAnimation)
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
            qDebug() << "用户信息返回结果不为0：" << json.value("message").toString();
            return ;
        }

        QJsonObject data = json.value("data").toObject();
        QString faceUrl = data.value("face").toString();

        getUserHeadPortrait(uid, faceUrl, item);
    });
}

void LiveDanmakuWindow::getUserHeadPortrait(qint64 uid, QString url, QListWidgetItem* item)
{
    QString path = headPath(uid);
    if (!QFileInfo(path).exists())
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QNetworkRequest* request = new QNetworkRequest(url);
        request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
        request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
        connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
            QByteArray jpegData = reply->readAll();
            QPixmap pixmap;
            pixmap.loadFromData(jpegData);
            if (pixmap.isNull())
            {
                qDebug() << "获取用户头像为空：" << uid;
                return ;
            }
            if (!pixmap.save(path))
            {
                qWarning() << "保存头像失败：" << uid << url;
                QDir().mkpath(headDir);
                pixmap.save(path);
            }

            if (!isItemExist(item))
                return ;
            PortraitLabel* label = getItemWidgetPortrait(item);
            label->setPixmap(QPixmap(path));
        });
        manager->get(*request);
        return ;
    }

    PortraitLabel* label = getItemWidgetPortrait(item);
    label->setPixmap(QPixmap(path));
}

QString LiveDanmakuWindow::headPath(qint64 uid) const
{
    return headDir + snum(uid) + ".jpg";
}

void LiveDanmakuWindow::showUserMsgHistory(qint64 uid, QString title)
{
    if (!uid)
        return ;
    QStringList sums;
    qint64 c = us->danmakuCounts->value("danmaku/"+snum(uid)).toInt();
    if(c)
        sums << snum(c)+" 条弹幕";
    c = us->danmakuCounts->value("come/"+snum(uid)).toInt();
    if (c)
        sums << "进来 " + snum(c)+" 次";
    c = us->danmakuCounts->value("gold/"+snum(uid)).toLongLong();
    if (c)
        sums << "赠送 " + snum(c)+" 金瓜子";
    c = us->danmakuCounts->value("silver/"+snum(uid)).toLongLong();
    if (c)
        sums << snum(c)+" 银瓜子";

    QStringList sl;
    if (sums.size())
        sl.append("* 总计：" + sums.join("，"));
    for (int i = rt->allDanmakus.size()-1; i >= 0; i--)
    {
        const LiveDanmaku& danmaku = rt->allDanmakus.at(i);
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
        if (rt->pinyinMap.contains(ch))
            res << ch + rt->pinyinMap.value(ch);
    }
    return res.join(" ");
}

QVariant LiveDanmakuWindow::getCookies()
{
    QList<QNetworkCookie> cookies;

    // 设置cookie
    QString cookieText = ac->browserCookie;
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

