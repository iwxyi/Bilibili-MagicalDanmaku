#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "zlib.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), settings("settings.ini", QSettings::Format::IniFormat)
{
    ui->setupUi(this);

    // 页面
    int tabIndex = settings.value("mainwindow/tabIndex", 0).toInt();
    if (tabIndex >= 0 && tabIndex < ui->tabWidget->count())
        ui->tabWidget->setCurrentIndex(tabIndex);

    // 房间号
    roomId = settings.value("danmaku/roomId", "").toString();
    if (!roomId.isEmpty())
        ui->roomIdEdit->setText(roomId);

    // 刷新间隔
#ifndef SOCKET_MODE
    danmakuTimer = new QTimer(this);
    int interval = settings.value("danmaku/interval", 500).toInt();
    ui->refreshDanmakuIntervalSpin->setValue(interval);
    danmakuTimer->setInterval(interval);
    connect(danmakuTimer, SIGNAL(timeout()), this, SLOT(pullLiveDanmaku()));
    ui->refreshDanmakuCheck->setChecked(true);
#endif


    // 自动刷新是否启用
    bool autoRefresh = settings.value("danmaku/autoRefresh", true).toBool();
    ui->refreshDanmakuCheck->setChecked(autoRefresh);

    // 移除间隔
    removeTimer = new QTimer(this);
    removeTimer->setInterval(1000);
    connect(removeTimer, SIGNAL(timeout()), this, SLOT(removeTimeoutDanmaku()));
    removeTimer->start();

    int removeIv = settings.value("danmaku/removeInterval", 20).toLongLong();
    ui->removeDanmakuIntervalSpin->setValue(removeIv); // 自动引发改变事件
    this->removeDanmakuInterval = removeIv * 1000;

    // 点歌自动复制
    diangeAutoCopy = settings.value("danmaku/diangeAutoCopy", true).toBool();
    ui->DiangeAutoCopyCheck->setChecked(diangeAutoCopy);
    connect(this, &MainWindow::signalNewDanmaku, this, [=](LiveDanmaku danmaku){
       if (!diangeAutoCopy)
           return ;
       QString text = danmaku.getText();
       if (!text.startsWith("点歌"))
           return ;
       text = text.replace(0, 2, "");
       if (QString(" :：，。,.").contains(text.left(1)))
           text.replace(0, 1, "");
       text = text.trimmed();
       QClipboard* clip = QApplication::clipboard();
       clip->setText(text);
       qDebug() << s8("【点歌自动复制】") << text;
       ui->DiangeAutoCopyCheck->setText("点歌自动复制（" + text + "）");

       addNoReplyDanmakuText(danmaku.getText());
       QTimer::singleShot(100, [=]{
           appendNewLiveDanmaku(LiveDanmaku(danmaku.getNickname(), danmaku.getUid(), text, danmaku.getTimeline()));
       });
       diangeHistory.append(Diange{danmaku.getNickname(), danmaku.getUid(), text, danmaku.getTimeline()});
    });

    // 自动翻译
    bool trans = settings.value("danmaku/autoTrans", true).toBool();
    ui->languageAutoTranslateCheck->setChecked(trans);

    // 自动回复
    bool reply = settings.value("danmaku/aiReply", true).toBool();
    ui->AIReplyCheck->setChecked(reply);

    // 实时弹幕
    danmakuWindow = new LiveDanmakuWindow(this);
    connect(this, SIGNAL(signalNewDanmaku(LiveDanmaku)), danmakuWindow, SLOT(slotNewLiveDanmaku(LiveDanmaku)));
    connect(this, SIGNAL(signalRemoveDanmaku(LiveDanmaku)), danmakuWindow, SLOT(slotOldLiveDanmakuRemoved(LiveDanmaku)));
    connect(danmakuWindow, SIGNAL(signalSendMsg(QString)), this, SLOT(sendMsg(QString)));
    danmakuWindow->setAutoTranslate(ui->languageAutoTranslateCheck->isChecked());
    danmakuWindow->setAIReply(ui->AIReplyCheck->isChecked());

    if (settings.value("danmaku/liveWindow", false).toBool())
        danmakuWindow->show();
    else
        danmakuWindow->hide();

    // 发送弹幕
    browserCookie = settings.value("danmaku/browserCookie", "").toString();
    browserData = settings.value("danmaku/browserData", "").toString();

    // 状态栏
    statusLabel = new QLabel(this);
    this->statusBar()->addWidget(statusLabel);

    // 定时任务
    srand((unsigned)time(0));
    restoreTaskList();

    // 自动发送
    ui->autoSendWelcomeCheck->setChecked(settings.value("danmaku/sendWelcome", false).toBool());
    ui->autoSendGiftCheck->setChecked(settings.value("danmaku/sendGift", false).toBool());
    ui->autoSendAttentionCheck->setChecked(settings.value("danmaku/sendAttention", false).toBool());
    ui->sendWelcomeCDSpin->setValue(settings.value("danmaku/sendWelcomeCD", 10).toInt());
    ui->sendGiftCDSpin->setValue(settings.value("danmaku/sendGiftCD", 5).toInt());
    ui->sendAttentionCDSpin->setValue(settings.value("danmaku/sendAttentionCD", 5).toInt());
    ui->autoWelcomeWordsEdit->setPlainText(settings.value("danmaku/autoWelcomeWords", ui->autoWelcomeWordsEdit->toPlainText()).toString());
    ui->autoThankWordsEdit->setPlainText(settings.value("danmaku/autoThankWords", ui->autoThankWordsEdit->toPlainText()).toString());
    ui->autoAttentionWordsEdit->setPlainText(settings.value("danmaku/autoAttentionWords", ui->autoAttentionWordsEdit->toPlainText()).toString());

    // 开播
    ui->startLiveWordsEdit->setText(settings.value("live/startWords").toString());
    ui->endLiveWordsEdit->setText(settings.value("live/endWords").toString());
    ui->startLiveSendCheck->setChecked(settings.value("live/startSend").toBool());

#ifndef SOCKET_MODE

#else
    ui->refreshDanmakuCheck->setEnabled(false);
    ui->refreshDanmakuIntervalSpin->setEnabled(false);

    // WS连接
    initWS();
    startConnectWS();
#endif

//    for (int i = 0; i < 3; i++)
    /*{
        QFile file("receive.txt");
        file.open(QIODevice::ReadWrite);
        auto ba = file.readAll();
        ba = ba.right(ba.size()-16);

        unsigned long si;
        BYTE* target = new BYTE[ba.size()*5+100]{};
        uncompress(target, &si, (unsigned char*)ba.data(), ba.size());
        QByteArray unc = QByteArray::fromRawData((char*)target, si);
        qDebug() << "解压后的：" << unc;
        qDebug() << "直接解压：" << zlibUncompress(ba);
    }*/

    // 10秒内不进行自动化操作
    QTimer::singleShot(3000, [=]{
        justStart = false;
    });


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent *event)
{
    restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    settings.setValue("mainwindow/geometry", this->saveGeometry());

    if (danmakuWindow)
    {
        danmakuWindow->close();
        danmakuWindow->deleteLater();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // 自动调整封面大小
    if (!coverPixmap.isNull())
    {
        QPixmap pixmap = coverPixmap;
        int w = ui->roomCoverLabel->width();
        if (w > ui->tabWidget->contentsRect().width())
            w = ui->tabWidget->contentsRect().width();
        pixmap = pixmap.scaledToWidth(w, Qt::SmoothTransformation);
        ui->roomCoverLabel->setPixmap(pixmap);
        ui->roomCoverLabel->setMinimumSize(1, 1);
    }

    // 自动调整任务列表大小
    /*for (int row = 0; row < ui->taskListWidget->count(); row++)
    {
        auto item = ui->taskListWidget->item(row);
        auto widget = ui->taskListWidget->itemWidget(item);
        if (!widget)
            continue;
        widget->setMinimumWidth(ui->taskListWidget->contentsRect().width());
        widget->resize(ui->taskListWidget->contentsRect().width(), widget->height());
        widget->adjustSize();
        item->setSizeHint(widget->size());
    }*/
}

void MainWindow::pullLiveDanmaku()
{
    if (roomId.isEmpty())
        return ;
    QString url = "https://api.live.bilibili.com/ajax/msg";
    QStringList param{"roomid", roomId};
    connect(new NetUtil(url, param), &NetUtil::finished, this, [=](QString result){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(result.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << "pullLiveDanmaku.ERROR:" << error.errorString();
            qDebug() << result;
            return ;
        }
        QJsonObject json = document.object();
        QJsonArray danmakus = json.value("data").toObject().value("room").toArray();
        QList<LiveDanmaku> lds;
        for (int i = 0; i < danmakus.size(); i++)
            lds.append(LiveDanmaku::fromDanmakuJson(danmakus.at(i).toObject()));
        appendNewLiveDanmakus(lds);
    });
}

void MainWindow::removeTimeoutDanmaku()
{
    // 移除过期队列
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    if (roomDanmakus.size()) // 每次最多移除一个；用while的话则会全部移除
    {
        QDateTime dateTime = roomDanmakus.first().getTimeline();
        if (dateTime.toMSecsSinceEpoch() + removeDanmakuInterval < timestamp)
        {
            auto danmaku = roomDanmakus.takeFirst();
            oldLiveDanmakuRemoved(danmaku);
        }
    }
}


void MainWindow::on_refreshDanmakuIntervalSpin_valueChanged(int arg1)
{
    settings.setValue("danmaku/interval", arg1);
#ifndef SOCKET_MODE
    danmakuTimer->setInterval(arg1);
#endif
}

void MainWindow::on_refreshDanmakuCheck_stateChanged(int arg1)
{
#ifndef SOCKET_MODE
    if (arg1)
        danmakuTimer->start();
    else
        danmakuTimer->stop();
#endif
}

void MainWindow::appendNewLiveDanmakus(QList<LiveDanmaku> danmakus)
{
#ifndef SOCKET_MODE
    // 去掉已经存在的弹幕
    while (danmakus.size() && danmakus.first().getTimeline().toMSecsSinceEpoch() <= prevLastDanmakuTimestamp)
        danmakus.removeFirst();
    if (!danmakus.size())
        return ;
    prevLastDanmakuTimestamp = danmakus.last().getTimeline().toMSecsSinceEpoch();

    // 不是第一次加载
    if (!firstPullDanmaku || danmakus.size() == 1) // 用作测试就不需要该条件；1条很可能是测试弹幕
    {
        // 发送信号给其他插件
        for (int i = 0; i < danmakus.size(); i++)
        {
            newLiveDanmakuAdded(danmakus.at(i));
        }
    }
    else
        firstPullDanmaku = false;
#endif

    // 添加到队列
    roomDanmakus.append(danmakus);
}

void MainWindow::appendNewLiveDanmaku(LiveDanmaku danmaku)
{
    roomDanmakus.append(danmaku);
    newLiveDanmakuAdded(danmaku);
}

void MainWindow::newLiveDanmakuAdded(LiveDanmaku danmaku)
{
    SOCKET_DEB << "+++++新弹幕：" << danmaku.toString();
    emit signalNewDanmaku(danmaku);
}

void MainWindow::oldLiveDanmakuRemoved(LiveDanmaku danmaku)
{
    SOCKET_DEB << "-----旧弹幕：" << danmaku.toString();
    emit signalRemoveDanmaku(danmaku);
}

void MainWindow::addNoReplyDanmakuText(QString text)
{
    if (danmakuWindow)
        danmakuWindow->addIgnoredMsg(text);
}

void MainWindow::sendMsg(QString msg)
{
    if (browserCookie.isEmpty() || browserData.isEmpty())
    {
        statusLabel->setText("未设置Cookie信息");
        return ;
    }
    if (msg.isEmpty() || roomId.isEmpty())
        return ;

    QUrl url("https://api.live.bilibili.com/msg/send");

    // 建立对象
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");

    // 设置数据（JSON的ByteArray）
    QString s = browserData;
    int posl = s.indexOf("msg=")+4;
    int posr = s.indexOf("&", posl);
    if (posr == -1)
        posr = s.length();
    s.replace(posl, posr-posl, msg);

    posl = s.indexOf("roomid=")+7;
    posr = s.indexOf("&", posl);
    if (posr == -1)
        posr = s.length();
    s.replace(posl, posr-posl, roomId);

    QByteArray ba(s.toStdString().data());
//    QByteArray ba("color=16777215&fontsize=25&mode=1&msg=thist&rnd=1604144057&roomid=11584296&bubble=0&csrf_token=13ddba7f6f0ad582fecef801d40b3abf&csrf=13ddba7f6f0ad582fecef801d40b3abf");
//    qDebug() << ba;

    // 连接槽
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
//        qDebug() << data;
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject object = document.object();
        QString errorMsg = object.value("message").toString();
        statusLabel->setText("");
        if (!errorMsg.isEmpty())
        {
            statusLabel->setText(errorMsg);
            qDebug() << s8("warning: 发送失败：") << errorMsg << msg;
        }
    });

    manager->post(*request, ba);
}

void MainWindow::sendWelcomeMsg(QString msg)
{
    if (!liveStatus) // 不在直播中
        return ;

    // 避免太频繁发消息
    static qint64 prevTimestamp = 0;
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    int cd = ui->sendWelcomeCDSpin->value() * 1000;
    if (timestamp - prevTimestamp < cd)
        return ;
    prevTimestamp = timestamp;

    if (msg.length() > 20)
        msg = msg.replace(" ", "");
    addNoReplyDanmakuText(msg);
    sendMsg(msg);
}

void MainWindow::sendGiftMsg(QString msg)
{
    if (!liveStatus) // 不在直播中
        return ;

    // 避免太频繁发消息
    static qint64 prevTimestamp = 0;
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    int cd = ui->sendGiftCDSpin->value() * 1000;
    if (timestamp - prevTimestamp < cd)
        return ;
    prevTimestamp = timestamp;

    if (msg.length() > 20)
        msg = msg.replace(" ", "");
    addNoReplyDanmakuText(msg);
    sendMsg(msg);
}

void MainWindow::sendAttentionMsg(QString msg)
{
    if (!liveStatus) // 不在直播中
        return ;

    // 避免太频繁发消息
    static qint64 prevTimestamp = 0;
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    int cd = ui->sendAttentionCDSpin->value() * 1000;
    if (timestamp - prevTimestamp < cd)
        return ;
    prevTimestamp = timestamp;

    if (msg.length() > 20)
        msg = msg.replace(" ", "");
    addNoReplyDanmakuText(msg);
    sendMsg(msg);
}

/**
 * 显示实时弹幕
 */
void MainWindow::on_showLiveDanmakuButton_clicked()
{
    bool hidding = danmakuWindow->isHidden();

    if (hidding)
    {
        danmakuWindow->show();
    }
    else
    {
        danmakuWindow->hide();
    }
    settings.setValue("danmaku/liveWindow", hidding);
}

void MainWindow::on_DiangeAutoCopyCheck_stateChanged(int)
{
    settings.setValue("danmaku/diangeAutoCopy", diangeAutoCopy = ui->DiangeAutoCopyCheck->isChecked());
}

void MainWindow::on_testDanmakuButton_clicked()
{
    QString text = ui->testDanmakuEdit->text();
    if (text.isEmpty())
        text = "测试弹幕";
    int r = qrand() % 7 + 1;
    appendNewLiveDanmaku(LiveDanmaku("测试用户" + QString::number(r), text,
                            10000+r,
                             QDateTime::currentDateTime()));

    ui->testDanmakuEdit->setText("");
    ui->testDanmakuEdit->setFocus();
}

void MainWindow::on_removeDanmakuIntervalSpin_valueChanged(int arg1)
{
    this->removeDanmakuInterval = arg1 * 1000;
    settings.setValue("danmaku/removeInterval", arg1);
}

void MainWindow::on_roomIdEdit_editingFinished()
{
    if (roomId == ui->roomIdEdit->text() || shortId == ui->roomIdEdit->text())
        return ;
    if (socket)
    {
        if (socket->state() != QAbstractSocket::UnconnectedState)
            socket->abort();
        liveStatus = false;
    }
    roomId = ui->roomIdEdit->text();
    settings.setValue("danmaku/roomId", roomId);
#ifndef SOCKET_MODE
    firstPullDanmaku = true;
    prevLastDanmakuTimestamp = 0;
#else
    if (socket)
    {
        startConnectWS();
    }
#endif
}

void MainWindow::on_languageAutoTranslateCheck_stateChanged(int)
{
    auto trans = ui->languageAutoTranslateCheck->isChecked();
    settings.setValue("danmaku/autoTrans", trans);
    if (danmakuWindow)
        danmakuWindow->setAutoTranslate(trans);
}

void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    settings.setValue("mainwindow/tabIndex", index);
}

void MainWindow::on_refreshDanmakuCheck_clicked()
{
    settings.setValue("danmaku/autoRefresh", ui->refreshDanmakuCheck->isChecked());
}

void MainWindow::on_SetBrowserCookieButton_clicked()
{
    bool ok = false;
    QString s = QInputDialog::getText(this, "设置Cookie", "设置用户登录的cookie", QLineEdit::Normal, browserCookie, &ok);
    if (!ok)
        return ;

    settings.setValue("danmaku/browserCookie", browserCookie = s);
}

void MainWindow::on_SetBrowserDataButton_clicked()
{
    bool ok = false;
    QString s = QInputDialog::getText(this, "设置Data", "设置用户登录的data", QLineEdit::Normal, browserData, &ok);
    if (!ok)
        return ;

    settings.setValue("danmaku/browserData", browserData = s);
}

void MainWindow::on_SetBrowserHelpButton_clicked()
{
    QString steps = "发送弹幕前需按以下步骤注入登录信息：\n\n";
    steps += "步骤一：\n浏览器登录bilibili账号，并进入对应直播间\n\n";
    steps += "步骤二：\n按下F12（开发者调试工具），找到右边顶部的“Network”项\n\n";
    steps += "步骤三：\n浏览器上发送弹幕，Network中多出一条“Send”，点它，看右边“Headers”中的代码\n\n";
    steps += "步骤四：\n复制“Request Headers”下的“cookie”冒号后的一长串内容，粘贴到本程序“设置Cookie”中\n\n";
    steps += "步骤五：\n点击“Form Data”右边的“view source”，复制它下面所有内容到本程序“设置Data”中\n\n";
    steps += "设置好直播间ID、要发送的内容，即可发送弹幕！\n\n";
    steps += "注意：请勿过于频繁发送，容易被临时拉黑！";
    QMessageBox::information(this, "定时弹幕", steps);
}

void MainWindow::on_SendMsgButton_clicked()
{
    QString msg = ui->SendMsgEdit->text();
    sendMsg(msg);
}

void MainWindow::on_AIReplyCheck_stateChanged(int)
{
    bool reply = ui->AIReplyCheck->isChecked();
    settings.setValue("danmaku/aiReply", reply);
    if (danmakuWindow)
        danmakuWindow->setAIReply(reply);
}

void MainWindow::on_testDanmakuEdit_returnPressed()
{
    on_testDanmakuButton_clicked();
}

void MainWindow::on_SendMsgEdit_returnPressed()
{
    sendMsg(ui->SendMsgEdit->text());
    ui->SendMsgEdit->clear();
}

void MainWindow::addTimerTask(bool enable, int second, QString text)
{
    TaskWidget* tw = new TaskWidget(this);
    QListWidgetItem* item = new QListWidgetItem(ui->taskListWidget);

    ui->taskListWidget->addItem(item);
    ui->taskListWidget->setItemWidget(item, tw);
    ui->taskListWidget->setCurrentRow(ui->taskListWidget->count()-1);
    ui->taskListWidget->scrollToBottom();

    // 连接信号
    connect(tw->check, &QCheckBox::stateChanged, this, [=](int){
        bool enable = tw->check->isChecked();
        int row = ui->taskListWidget->row(item);
        settings.setValue("task/r"+QString::number(row)+"Enable", enable);
    });

    connect(tw, &TaskWidget::spinChanged, this, [=](int val){
        int row = ui->taskListWidget->row(item);
        settings.setValue("task/r"+QString::number(row)+"Interval", val);
    });

    connect(tw->edit, &QPlainTextEdit::textChanged, this, [=]{
        item->setSizeHint(tw->sizeHint());

        QString content = tw->edit->toPlainText();
        int row = ui->taskListWidget->row(item);
        settings.setValue("task/r"+QString::number(row)+"Msg", content);
    });

    connect(tw, &TaskWidget::signalSendMsg, this, [=](QString msg){
        if (!liveStatus) // 没有开播，不进行定时任务
            return ;
        sendMsg(msg);
    });

    connect(tw, &TaskWidget::signalResized, tw, [=]{
        item->setSizeHint(tw->size());
    });

    // 设置属性
    tw->check->setChecked(enable);
    tw->spin->setValue(second);
    tw->edit->setPlainText(text);
    tw->adjustSize();
    item->setSizeHint(tw->sizeHint());
}

void MainWindow::saveTaskList()
{
    settings.setValue("task/count", ui->taskListWidget->count());
    for (int row = 0; row < ui->taskListWidget->count(); row++)
    {
        auto widget = ui->taskListWidget->itemWidget(ui->taskListWidget->item(row));
        auto tw = static_cast<TaskWidget*>(widget);
        settings.setValue("task/r"+QString::number(row)+"Enable", tw->check->isChecked());
        settings.setValue("task/r"+QString::number(row)+"Interval", tw->spin->value());
        settings.setValue("task/r"+QString::number(row)+"Msg", tw->edit->toPlainText());
    }
}

void MainWindow::restoreTaskList()
{
    int count = settings.value("task/count", 0).toInt();
    for (int row = 0; row < count; row++)
    {
        bool enable = settings.value("task/r"+QString::number(row)+"Enable", true).toBool();
        int interval = settings.value("task/r"+QString::number(row)+"Interval", 1800).toInt();
        QString msg = settings.value("task/r"+QString::number(row)+"Msg", "").toString();
        addTimerTask(enable, interval, msg);
    }
}

QVariant MainWindow::getCookies()
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

void MainWindow::on_taskListWidget_customContextMenuRequested(const QPoint &)
{
    QListWidgetItem* item = ui->taskListWidget->currentItem();

    QMenu* menu = new QMenu(this);
    QAction* actionDelete = new QAction("删除", this);

    if (!item)
    {
        actionDelete->setEnabled(false);
    }

    menu->addAction(actionDelete);

    connect(actionDelete, &QAction::triggered, this, [=]{
        auto widget = ui->taskListWidget->itemWidget(item);
        auto tw = static_cast<TaskWidget*>(widget);

        ui->taskListWidget->removeItemWidget(item);
        ui->taskListWidget->takeItem(ui->taskListWidget->currentRow());

        saveTaskList();

        tw->deleteLater();
    });

    menu->exec(QCursor::pos());

    actionDelete->deleteLater();
}

void MainWindow::on_addTaskButton_clicked()
{
    addTimerTask(true, 1800, "");
    saveTaskList();
    auto widget = ui->taskListWidget->itemWidget(ui->taskListWidget->item(ui->taskListWidget->count()-1));
    auto tw = static_cast<TaskWidget*>(widget);
    tw->edit->setFocus();
}

void MainWindow::slotSocketError(QAbstractSocket::SocketError error)
{
    qDebug() << "error" << socket->errorString();
    statusLabel->setText(socket->errorString());
}

void MainWindow::initWS()
{
    socket = new QWebSocket();

    connect(socket, &QWebSocket::connected, this, [=]{
        qDebug() << "socket connected";
        ui->connectStateLabel->setText("状态：已连接");

        // 5秒内发送认证包
        sendVeriPacket();

        // 定时发送心跳包
        heartTimer->start();
    });

    connect(socket, &QWebSocket::disconnected, this, [=]{
        qDebug() << "disconnected";
        ui->connectStateLabel->setText("状态：未连接");
        ui->popularityLabel->setText("人气值：0");

        heartTimer->stop();
    });

    connect(socket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
//        qDebug() << "binaryMessageReceived" << message;
//        for (int i = 0; i < 100; i++) // 测试内存泄漏
        try {
            slotBinaryMessageReceived(message);
        } catch (...) {
            qDebug() << "!!!!!!!error:slotBinaryMessageReceived";
        }

    });

    connect(socket, &QWebSocket::textFrameReceived, this, [=](const QString &frame, bool isLastFrame){
        qDebug() << "textFrameReceived";
    });

    connect(socket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
        qDebug() << "textMessageReceived";
    });

    connect(socket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        qDebug() << "stateChanged" << state;
        QString str = "未知";
        if (state == QAbstractSocket::UnconnectedState)
            str = "未连接";
        else if (state == QAbstractSocket::ConnectingState)
            str = "连接中";
        else if (state == QAbstractSocket::ConnectedState)
            str = "已连接";
        else if (state == QAbstractSocket::BoundState)
            str = "已绑定";
        else if (state == QAbstractSocket::ClosingState)
            str = "断开中";
        else if (state == QAbstractSocket::ListeningState)
            str = "监听中";
        ui->connectStateLabel->setText(str);
    });

    heartTimer = new QTimer(this);
    heartTimer->setInterval(30000);
    connect(heartTimer, &QTimer::timeout, this, [=]{
        sendHeartPacket();
    });
}

void MainWindow::startConnectWS()
{
    if (roomId.isEmpty())
        return ;

    // 初始化主播数据
    currentFans = 0;
    currentFansClub = 0;

    getRoomInfo();
}

void MainWindow::getRoomInit()
{
    QString roomInitUrl = "https://api.live.bilibili.com/room/v1/Room/room_init?id=" + roomId;
    connect(new NetUtil(roomInitUrl), &NetUtil::finished, this, [=](QString result){
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

        int realRoom = json.value("data").toObject().value("room_id").toInt();
        qDebug() << "真实房间号：" << realRoom;
    });
}

void MainWindow::getRoomInfo()
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByRoom?room_id=" + roomId;
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
        SOCKET_INF << QString(data);
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 0)
        {
            qDebug() << s8("返回结果不为0：") << json.value("message").toString();
            return ;
        }

        // 获取房间信息
        QJsonObject dataObj = json.value("data").toObject();
        QJsonObject roomInfo = dataObj.value("room_info").toObject();
        roomId = QString::number(roomInfo.value("room_id").toInt()); // 应当一样
        ui->roomIdEdit->setText(roomId);
        shortId = QString::number(roomInfo.value("short_id").toInt());
        uid = QString::number(roomInfo.value("uid").toInt());
        liveStatus = roomInfo.value("live_status").toInt();
        qDebug() << "getRoomInfo: roomid=" << roomId
                 << "  shortid=" << shortId
                 << "  uid=" << uid;

        roomName = roomInfo.value("title").toString();
        setWindowTitle(QApplication::applicationName() + " - " + roomName);
        ui->roomNameLabel->setText(roomName);
        if (liveStatus != 1)
            ui->popularityLabel->setText("未开播");
        else
            ui->popularityLabel->setText("已开播");

        // 获取主播信息
        QJsonObject anchorInfo = dataObj.value("anchor_info").toObject();
        currentFans = anchorInfo.value("relation_info").toObject().value("attention").toInt();
        currentFansClub = anchorInfo.value("medal_info").toObject().value("fansclub").toInt();
        qDebug() << s8("被关注：") << currentFans << s8("    粉丝团：") << currentFansClub;
        getFansAndUpdate();

        // 获取弹幕信息
        getDanmuInfo();

        // 异步获取房间封面
        QString coverUrl = roomInfo.value("cover").toString();
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply1 = manager.get(QNetworkRequest(QUrl(coverUrl)));
        //请求结束并下载完成后，退出子事件循环
        connect(reply1, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        //开启子事件循环
        loop.exec();
        QByteArray jpegData = reply1->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(jpegData);
        coverPixmap = pixmap;
        int w = ui->roomCoverLabel->width();
        if (w > ui->tabWidget->contentsRect().width())
            w = ui->tabWidget->contentsRect().width();
        pixmap = pixmap.scaledToWidth(w, Qt::SmoothTransformation);
        ui->roomCoverLabel->setPixmap(pixmap);
        ui->roomCoverLabel->setMinimumSize(1, 1);
    });
    manager->get(*request);
}

void MainWindow::getDanmuInfo()
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getDanmuInfo";
    url += "?id="+roomId+"&type=0";
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray dataBa = reply->readAll();
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(dataBa, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 0)
        {
            qDebug() << s8("返回结果不为0：") << json.value("message").toString();
            return ;
        }

        QJsonObject data = json.value("data").toObject();
        token = data.value("token").toString();
        QJsonArray hostArray = data.value("host_list").toArray();
        hostList.clear();
        foreach (auto val, hostArray)
        {
            QJsonObject o = val.toObject();
            hostList.append(HostInfo{
                                o.value("host").toString(),
                                o.value("port").toInt(),
                                o.value("wss_port").toInt(),
                                o.value("ws_port").toInt(),
                            });
        }
        qDebug() << s8("getDanmuInfo: host数量=") << hostList.size() << "  token=" << token;

        startMsgLoop();
    });
    manager->get(*request);
}

void MainWindow::getFansAndUpdate()
{
    QString url = "http://api.bilibili.com/x/relation/followers?vmid=" + uid;
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");

    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        QJsonArray list = json.value("data").toObject().value("list").toArray();
        QList<FanBean> newFans;
        foreach (QJsonValue val, list)
        {
            QJsonObject fan = val.toObject();
            qint64 mid = static_cast<qint64>(fan.value("mid").toDouble());
            int attribute = fan.value("attribute").toInt(); // 三位数：1-1-0 被关注-已关注-未知
            if (attribute == 0) // 0是什么意思啊，不懂诶……应该是关注吧？
                attribute = 2;
            qint64 mtime = static_cast<qint64>(fan.value("mtime").toDouble());
            QString uname = fan.value("uname").toString();

            newFans.append(FanBean{mid, uname, attribute & 2, mtime});
//            SOCKET_DEB << "    粉丝信息：" << mid << uname << attribute << QDateTime::fromSecsSinceEpoch(mtime);
        }

//        SOCKET_DEB << "用户ID：" << uid << "    现有粉丝数(第一页)：" << newFans.size();

        // 第一次加载
        if (!fansList.size())
        {
            fansList = newFans;
            return ;
        }

        // 进行比较 新增关注 的 取消关注
        // 先找到原先最后关注并且还在的，即：旧列.index == 新列.find
        int index = -1; // 旧列索引
        int find = -1;  // 新列索引
        while (++index < fansList.size())
        {
            FanBean fan = fansList.at(index);
            for (int j = 0; j < newFans.size(); j++)
            {
                FanBean nFan = newFans.at(j);
                if (fan.mid == nFan.mid)
                {
                    find = j;
                    break;
                }
            }
            if (find >= 0)
                break;
        }
//SOCKET_DEB << ">>>>>>>>>>" << "index:" << index << "           find:" << find;

        // 取消关注（只支持最新关注的，不是专门做的，只是顺带）
        if (index >= fansList.size()) // 没有被关注过，或之前关注的全部取关了？
        {
            qDebug() << s8("没有被关注过，或之前关注的全部取关了？");
        }
        else // index==0没有人取关，index>0则有人取关
        {
            for (int i = 0; i < index; i++)
            {
                FanBean fan = fansList.at(i);
                qDebug() << s8("取消关注：") << fan.uname << QDateTime::fromSecsSinceEpoch(fan.mtime);
                appendNewLiveDanmaku(LiveDanmaku(fan.uname, fan.mid, false, QDateTime::fromSecsSinceEpoch(fan.mtime)));
            }
            while (index--)
                fansList.removeFirst();
        }

        // 新增关注
        for (int i = find-1; i >= 0; i--)
        {
            FanBean fan = newFans.at(i);
            qDebug() << s8("新增关注：") << fan.uname << QDateTime::fromSecsSinceEpoch(fan.mtime);
            appendNewLiveDanmaku(LiveDanmaku(fan.uname, fan.mid, true, QDateTime::fromSecsSinceEpoch(fan.mtime)));

            if (i == 0) // 只发送第一个（其他几位，对不起了……）
            {
                QStringList words = ui->autoAttentionWordsEdit->toPlainText().split("\n", QString::SkipEmptyParts);
                if (!words.size())
                    return ;
                int r = qrand() % words.size();
                QString msg = words.at(r);
                if (!justStart && ui->autoSendAttentionCheck->isChecked())
                {
                    QString localName = danmakuWindow->getLocalNickname(fan.mid);
                    QString nick = localName.isEmpty() ? nicknameSimplify(fan.uname) : localName;
                    if (!nick.isEmpty())
                        sendAttentionMsg(msg.arg(nick));
                }
            }

            fansList.insert(0, fan);
        }

    });
    manager->get(*request);
}

void MainWindow::startMsgLoop()
{
    int hostRetry = 0;
    HostInfo hostServer = hostList.at(hostRetry);
    QString host = QString("wss://%1:%2/sub").arg(hostServer.host).arg(hostServer.wss_port);
    qDebug() << "hostServer:" << host;

    // 设置安全套接字连接模式（不知道有啥用）
    QSslConfiguration config = socket->sslConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setProtocol(QSsl::TlsV1SslV3);
    socket->setSslConfiguration(config);

    socket->open(host);
}

/**
 * 给body加上头部信息
偏移量	长度	类型	含义
0	4	uint32	封包总大小（头部大小+正文大小）
4	2	uint16	头部大小（一般为0x0010，16字节）
6	2	uint16	协议版本:0普通包正文不使用压缩，1心跳及认证包正文不使用压缩，2普通包正文使用zlib压缩
8	4	uint32	操作码（封包类型）
12	4	uint32	sequence，可以取常数1
 */
QByteArray MainWindow::makePack(QByteArray body, qint32 operation)
{
    // 因为是大端，所以要一个个复制
    qint32 totalSize = 16 + body.size();
    short headerSize = 16;
    short protover = 1;
    qint32 seqId = 1;

    auto byte4 = [=](qint32 i) -> QByteArray{
        QByteArray ba(4, 0);
        ba[3] = (uchar)(0x000000ff & i);
        ba[2] = (uchar)((0x0000ff00 & i) >> 8);
        ba[1] = (uchar)((0x00ff0000 & i) >> 16);
        ba[0] = (uchar)((0xff000000 & i) >> 24);
        return ba;
    };

    auto byte2 = [=](short i) -> QByteArray{
        QByteArray ba(2, 0);
        ba[1] = (uchar)(0x00ff & i);
        ba[0] = (uchar)((0xff00 & i) >> 8);
        return ba;
    };

    QByteArray header;
    header += byte4(totalSize);
    header += byte2(headerSize);
    header += byte2(protover);
    header += byte4(operation);
    header += byte4(seqId);

    return header + body;


    /* // 小端算法，直接上结构体
    int totalSize = 16 + body.size();
    short headerSize = 16;
    short protover = 1;
    int seqId = 1;

    HeaderStruct header{totalSize, headerSize, protover, operation, seqId};

    QByteArray ba((char*)&header, sizeof(header));
    return ba + body;*/
}

void MainWindow::sendVeriPacket()
{
    QByteArray ba;
    ba.append("{\"uid\": 0, \"roomid\": "+roomId+", \"protover\": 2, \"platform\": \"web\", \"clientver\": \"1.14.3\", \"type\": 2, \"key\": \""+token+"\"}");
    ba = makePack(ba, AUTH);
    SOCKET_DEB << "发送认证包：" << ba;
    socket->sendBinaryMessage(ba);
}

/**
 * 每隔半分钟发送一次心跳包
 */
void MainWindow::sendHeartPacket()
{
    QByteArray ba;
    ba.append("[object Object]");
    ba = makePack(ba, HEARTBEAT);
//    SOCKET_DEB << "发送心跳包：" << ba;
    socket->sendBinaryMessage(ba);
}

/**
 * 解压数据
 * 这个方法必须要qDebug输出一个东西才能用
 * 否则只能拆开放到要用的地方
 */
QByteArray MainWindow::zlibUncompress(QByteArray ba) const
{
    unsigned long si = 0;
    BYTE* target = new BYTE[ba.size()*5+10000]{};
//    uncompress(target, &si, (unsigned char*)ba.data(), ba.size());
    // SOCKET_DEB << "解压后数据大小：" << si << ba.size(); // 这句话不能删！ // 这句话不能加！
    return QByteArray::fromRawData((char*)target, si);
}

/**
 * 一个智能的用户昵称转简单称呼
 */
QString MainWindow::nicknameSimplify(QString nickname) const
{
    QString simp = nickname;

    QStringList special{"~", "丶", "°", "゛", "-", "_"};
    QStringList starts{"我叫", "叫我", "一只", "是个", "是", "原来是"};
    QStringList ends{"er", "啊", "呢", "哦", "呐"};
    starts += special;
    ends += special;

    foreach (auto start, starts)
    {
        if (simp.startsWith(start))
        {
            simp.remove(0, start.length());
            break;
        }
    }

    foreach (auto end, ends)
    {
        if (simp.endsWith(end))
        {
            simp.remove(simp.length() - end.length(), end.length());
            break;
        }
    }

    // xxx的xxx
    QRegularExpression deRe("^(.+)[的之の](.{2,})$");
    QRegularExpressionMatch match;
    if (simp.indexOf(deRe, 0, &match) > -1 && match.capturedTexts().at(1).length() <= match.capturedTexts().at(2).length())
    {
        simp = match.capturedTexts().at(2);
    }

    // 一大串中文en
    QRegularExpression ceRe("^([\u4e00-\u9fa5]{2,})(\\w+)$");
    if (simp.indexOf(ceRe, 0, &match) > -1 && match.capturedTexts().at(1).length() >= match.capturedTexts().at(2).length())
    {
        QString tmp = match.capturedTexts().at(1);
        if (!QString("的之の是叫有为奶在去着").contains(tmp.right(1)))
        {
            simp = tmp;
        }
    }

    // 没有取名字的，就不需要欢迎了
    QRegularExpression defaultRe("^[bB]ili_\\d+$");
    if (simp.indexOf(defaultRe) > -1)
    {
        return "";
    }

    if (simp.isEmpty())
        return nickname;
    return simp;
}

void MainWindow::slotBinaryMessageReceived(const QByteArray &message)
{
    qDebug() << (int)message.data() << message.size() << message;
    int operation = ((uchar)message[8] << 24)
            + ((uchar)message[9] << 16)
            + ((uchar)message[10] << 8)
            + ((uchar)message[11]);
    QByteArray body = message.right(message.length() - 16);
    SOCKET_DEB << "操作码=" << operation << "  正文=" << (body.left(35)) << "...";

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(body, &error);
    QJsonObject json;
    if (error.error == QJsonParseError::NoError)
        json = document.object();

    if (operation == AUTH_REPLY) // 认证包回复
    {
        if (json.value("code").toInt() != 0)
        {
            qDebug() << s8("认证出错");
        }
    }
    else if (operation == HEARTBEAT_REPLY) // 心跳包回复（人气值）
    {
        qint32 popularity = ((uchar)body[0] << 24)
                + ((uchar)body[1] << 16)
                + ((uchar)body[2] << 8)
                + (uchar)body[3];
        SOCKET_DEB << "人气值=" << popularity;
        if (liveStatus)
            ui->popularityLabel->setText("人气值：" + QString::number(popularity));
    }
    else if (operation == SEND_MSG_REPLY) // 普通包
    {
        QString cmd;
        if (!json.isEmpty())
        {
            cmd = json.value("cmd").toString();
            SOCKET_INF << "普通CMD：" << cmd;
        }
        if (cmd == "NOTICE_MSG") // 全站广播（不用管）
        {

        }
        else
        {
            short protover = (message[6]<<8) + message[7];
            SOCKET_INF << "协议版本：" << protover;
            if (protover == 2) // 默认协议版本，zlib解压
            {
                slotUncompressBytes(body);
            }
            else if (protover == 0)
            {
                QJsonDocument document = QJsonDocument::fromJson(body, &error);
                if (error.error != QJsonParseError::NoError)
                {
                    qDebug() << s8("body转json出错：") << error.errorString();
                    return ;
                }
                QJsonObject json = document.object();
                QString cmd = json.value("cmd").toString();

                if (cmd == "ROOM_RANK")
                {
                }
                else if (cmd == "ROOM_REAL_TIME_MESSAGE_UPDATE")
                {
                    // {"cmd":"ROOM_REAL_TIME_MESSAGE_UPDATE","data":{"roomid":22532956,"fans":1022,"red_notice":-1,"fans_club":50}}
                    QJsonObject data = json.value("data").toObject();
                    int fans = data.value("fans").toInt();
                    int fans_club = data.value("fans_club").toInt();
                    int delta_fans = 0, delta_club = 0;
                    if (currentFans || currentFansClub)
                    {
                        delta_fans = fans - currentFans;
                        delta_club = fans_club - currentFansClub;
                    }
                    currentFans = fans;
                    currentFansClub = fans_club;
                    qDebug() << s8("粉丝数量：") << fans << s8("  粉丝团：") << fans_club;
                    appendNewLiveDanmaku(LiveDanmaku(fans, fans_club,
                                                     delta_fans, delta_club));

                    if (delta_fans)
                        getFansAndUpdate();
                }
                else
                {
                    SOCKET_DEB << "未处理的命令=" << cmd << "   正文=" << body;
                }
            }
            else
            {
                qDebug() << s8("未知协议：") << protover << s8("，若有必要请处理");
                qDebug() << s8("未知正文：") << body;
            }
        }
    }
    else
    {

    }
//    delete[] body.data();
//    delete[] message.data();
    SOCKET_DEB << "消息处理结束";
}

void MainWindow::slotUncompressBytes(const QByteArray &body)
{
    qDebug() << ">>>>>>>>>>>>>>解压：" << body.size() << body;
    unsigned long si;
    BYTE* target = new BYTE[body.size()*5+100]{};
    unsigned char* buffer_compress = (unsigned char*)body.data();
    unsigned long len = body.size()+1;
    uncompress(target, &si, buffer_compress, len);
    SOCKET_DEB << "解压后数据大小：" << si << "    原来：" << len;
    QByteArray unc = QByteArray::fromRawData((char*)target, si);
    qDebug() << "<<<<<<<<<<<<<<结果：" << unc.size() << unc;
    if (unc.size() < len)
        QApplication::quit();
    splitUncompressedBody(unc);
}

void MainWindow::splitUncompressedBody(const QByteArray &unc)
{
    int offset = 0;
    short headerSize = 16;
    while (offset < unc.size() - headerSize)
    {
        int packSize = ((uchar)unc[offset+0] << 24)
                + ((uchar)unc[offset+1] << 16)
                + ((uchar)unc[offset+2] << 8)
                + (uchar)unc[offset+3];
        QByteArray jsonBa = unc.mid(offset + headerSize, packSize - headerSize);
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(jsonBa, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << s8("解析解压后的JSON出错：") << error.errorString();
            qDebug() << s8("包数值：") << offset << packSize << unc.size();
            qDebug() << s8(">>当前JSON") << jsonBa;
            qDebug() << s8(">>解压正文") << unc;
            return ;
        }
        QJsonObject json = document.object();
        QString cmd = json.value("cmd").toString();
        SOCKET_INF << "解压后获取到CMD：" << cmd;
        if (cmd != "ROOM_BANNER" && cmd != "ACTIVITY_BANNER_UPDATE_V2" && cmd != "PANEL"
                && cmd != "ONLINERANK")
            SOCKET_DEB << "单个JSON消息：" << offset << packSize << QString(jsonBa);
        try {
            handleMessage(json);
        } catch (...) {
            qDebug() << s8("出错啦") << jsonBa;
        }

        offset += packSize;
    }
}

/**
 * 数据包解析： https://segmentfault.com/a/1190000017328813?utm_source=tag-newest#tagDataPackage
 */
void MainWindow::handleMessage(QJsonObject json)
{
    QString cmd = json.value("cmd").toString();
    qDebug() << s8(">消息命令：") << cmd;
    if (cmd == "LIVE") // 开播？
    {
        QString roomId = json.value("roomid").toString();
        if (roomId.isEmpty())
            roomId = QString::number(static_cast<qint64>(json.value("roomid").toDouble()));
//        if (roomId == this->roomId || roomId == this->shortId) // 是当前房间的
        {
            QString text = ui->startLiveWordsEdit->text();
            if (ui->startLiveSendCheck->isChecked() && !text.trimmed().isEmpty())
                sendMsg(text);
            ui->popularityLabel->setText("已开播");
            liveStatus = true;
        }
    }
    else if (cmd == "PREPARING") // 下播
    {
        QString roomId = json.value("roomid").toString();
//        if (roomId == this->roomId || roomId == this->shortId) // 是当前房间的
        {
            QString text = ui->endLiveWordsEdit->text();
            if (ui->startLiveSendCheck->isChecked() &&!text.trimmed().isEmpty())
                sendMsg(text);
            ui->popularityLabel->setText("已下播");
            liveStatus = false;
        }
    }
    else if (cmd == "ROOM_CHANGE")
    {
        QJsonObject data = json.value("data").toObject();

    }
    else if (cmd == "DANMU_MSG") // 收到弹幕
    {
        QJsonArray info = json.value("info").toArray();
        if (info.size() <= 2)
            QMessageBox::information(this, "弹幕数据 info", QString(QJsonDocument(info).toJson()));
        QJsonArray array = info[0].toArray();
        if (array.size() <= 3)
            QMessageBox::information(this, "弹幕数据 array", QString(QJsonDocument(array).toJson()));
        qint64 textColor = array[3].toInt(); // 弹幕颜色
        qint64 timestamp = static_cast<qint64>(array[4].toDouble());
        QString msg = info[1].toString();
        QJsonArray user = info[2].toArray();
        if (user.size() <= 1)
            QMessageBox::information(this, "弹幕数据 user", QString(QJsonDocument(user).toJson()));
        qint64 uid = static_cast<qint64>(user[0].toDouble());
        QString username = user[1].toString();
        QString unameColor = user[7].toString();

        // !弹幕的时间戳是13位，其他的是10位！
        qDebug() << s8("接收到弹幕：") << username << msg << QDateTime::fromMSecsSinceEpoch(timestamp);
        /*QString localName = danmakuWindow->getLocalNickname(uid);
        if (!localName.isEmpty())
            username = localName;*/
        QString cs = QString::number(textColor, 16);
        while (cs.size() < 6)
            cs = "0" + cs;
        appendNewLiveDanmaku(LiveDanmaku(username, msg, uid, QDateTime::fromMSecsSinceEpoch(timestamp),
                                         unameColor, "#"+cs));
    }
    else if (cmd == "SEND_GIFT") // 有人送礼
    {
        QJsonObject data = json.value("data").toObject();
        QString giftName = data.value("giftName").toString();
        QString username = data.value("uname").toString();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        int num = data.value("num").toInt();
        qint64 timestamp = static_cast<qint64>(data.value("timestamp").toDouble());
        QString coinType = data.value("coin_type").toString();
        int totalCoin = data.value("total_coin").toInt();

        qDebug() << s8("接收到送礼：") << username << giftName << num << s8("  总价值：") << totalCoin << coinType;
        QString localName = danmakuWindow->getLocalNickname(uid);
        /*if (!localName.isEmpty())
            username = localName;*/
        appendNewLiveDanmaku(LiveDanmaku(username, giftName, num, uid, QDateTime::fromSecsSinceEpoch(timestamp)));

        QStringList words = ui->autoThankWordsEdit->toPlainText().split("\n", QString::SkipEmptyParts);
        if (!words.size())
            return ;
        int r = qrand() % words.size();
        QString msg = words.at(r);
        if (coinType == "gold")
        {
            int yuan = totalCoin / 1000;
            if (yuan >= 9)
            {
                msg = "哇，感谢 %1 赠送的%2！";
            }
            else if (yuan >= 70)
            {
                msg = "哇，感谢 %1 赠送的%2！！！";
            }
        }
        if (!justStart && ui->autoSendGiftCheck->isChecked())
        {
            QString nick = localName.isEmpty() ? nicknameSimplify(username) : localName;
            if (!nick.isEmpty())
                sendWelcomeMsg(msg.arg(nick).arg(giftName));
        }
    }
    else if (cmd == "GUARD_BUY") // 有人上舰
    {
        QJsonObject data = json.value("data").toObject();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString username = data.value("username").toString();
        QString giftName = data.value("giftName").toString();
        int num = data.value("num").toInt();
        qint64 timestamp = static_cast <qint64>(data.value("timestamp").toDouble());
        qDebug() << username << s8("购买舰长");
        appendNewLiveDanmaku(LiveDanmaku(username, uid, giftName, num));
    }
    else if (cmd == "SUPER_CHAT_MESSAGE") // 醒目留言
    {

    }
    else if (cmd == "SUPER_CHAT_MESSAGE_DELETE") // 删除醒目留言
    {

    }
    else if (cmd == "WELCOME_GUARD") // 舰长进入
    {
        QJsonObject data = json.value("data").toObject();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString username = data.value("username").toString();
        qint64 startTime = static_cast<qint64>(data.value("start_time").toDouble());
        qint64 endTime = static_cast<qint64>(data.value("end_time").toDouble());
        qint64 timestamp = static_cast<qint64>(data.value("timestamp").toDouble());
        qDebug() << s8("舰长进入：") << username;
        /*QString localName = danmakuWindow->getLocalNickname(uid);
        if (!localName.isEmpty())
            username = localName;*/
        appendNewLiveDanmaku(LiveDanmaku(username, uid, QDateTime::fromSecsSinceEpoch(timestamp), true));
    }
    else  if (cmd == "ENTRY_EFFECT") // 舰长进入的同时会出现
    {

    }
    else if (cmd == "WELCOME") // 进入（似乎已经废弃）
    {
        QJsonObject data = json.value("data").toObject();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString username = data.value("uname").toString();
        bool isAdmin = data.value("isAdmin").toBool();
        qDebug() << s8("观众进入：") << username << isAdmin;
    }
    else if (cmd == "INTERACT_WORD")
    {
        QJsonObject data = json.value("data").toObject();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString username = data.value("uname").toString();
        qint64 timestamp = static_cast<qint64>(data.value("timestamp").toDouble());
        bool isadmin = data.value("isadmin").toBool();
        qDebug() << s8("观众进入：") << username;
        QString localName = danmakuWindow->getLocalNickname(uid);
        /*if (!localName.isEmpty())
            username = localName;*/
        appendNewLiveDanmaku(LiveDanmaku(username, uid, QDateTime::fromSecsSinceEpoch(timestamp), isadmin));
        if (!justStart && ui->autoSendWelcomeCheck->isChecked())
        {
            qint64 currentTime = QDateTime::currentSecsSinceEpoch();
            int cd = ui->sendWelcomeCDSpin->value() * 1000 * 10; // 10倍冷却时间
            if (userComeTimes.contains(uid) && userComeTimes.value(uid) + cd > currentTime)
                return ; // 避免同一个人连续欢迎多次（好像B站自动不发送？）
            userComeTimes[uid] = currentTime;

            QStringList words = ui->autoWelcomeWordsEdit->toPlainText().split("\n", QString::SkipEmptyParts);
            if (!words.size())
                return ;
            int r = qrand() % words.size();
            QString msg = words.at(r);
            QString nick = localName.isEmpty() ? nicknameSimplify(username) : localName;
            if (!nick.isEmpty())
                sendWelcomeMsg(msg.arg(nick));
        }
    }
}

void MainWindow::on_autoSendWelcomeCheck_stateChanged(int arg1)
{
    settings.setValue("danmaku/sendWelcome", ui->autoSendWelcomeCheck->isChecked());
}

void MainWindow::on_autoSendGiftCheck_stateChanged(int arg1)
{
    settings.setValue("danmaku/sendGift", ui->autoSendGiftCheck->isChecked());
}

void MainWindow::on_autoWelcomeWordsEdit_textChanged()
{
    settings.setValue("danmaku/autoWelcomeWords", ui->autoWelcomeWordsEdit->toPlainText());
}

void MainWindow::on_autoThankWordsEdit_textChanged()
{
    settings.setValue("danmaku/autoThankWords", ui->autoThankWordsEdit->toPlainText());
}

void MainWindow::on_startLiveWordsEdit_editingFinished()
{
    settings.setValue("live/startWords", ui->startLiveWordsEdit->text());
}

void MainWindow::on_endLiveWordsEdit_editingFinished()
{
    settings.setValue("live/endWords", ui->endLiveWordsEdit->text());
}

void MainWindow::on_startLiveSendCheck_stateChanged(int arg1)
{
    settings.setValue("live/startSend", ui->startLiveSendCheck->isChecked());
}

void MainWindow::on_autoSendAttentionCheck_stateChanged(int arg1)
{
    settings.setValue("danmaku/sendAttention", ui->startLiveSendCheck->isChecked());
}

void MainWindow::on_autoAttentionWordsEdit_textChanged()
{
    settings.setValue("danmaku/autoAttentionWords", ui->autoAttentionWordsEdit->toPlainText());
}

void MainWindow::on_sendWelcomeCDSpin_valueChanged(int arg1)
{
    settings.setValue("danmaku/sendWelcomeCD", arg1);
}

void MainWindow::on_sendGiftCDSpin_valueChanged(int arg1)
{
    settings.setValue("danmaku/sendGiftCD", arg1);
}

void MainWindow::on_sendAttentionCDSpin_valueChanged(int arg1)
{
    settings.setValue("danmaku/sendAttentionCD", arg1);
}

void MainWindow::on_diangeHistoryButton_clicked()
{
    QStringList list;
    int first = qMax(0, diangeHistory.size() - 10);
    for (int i = diangeHistory.size()-1; i >= first; i--)
    {
        Diange dg = diangeHistory.at(i);
        list << dg.name + "  -  " + dg.nickname + "  " + dg.time.toString("hh:mm:ss");
    }
    QString text = list.size() ? list.join("\n") : "没有点歌记录";
    QMessageBox::information(this, "点歌历史", text);
}
