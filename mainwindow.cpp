#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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

    removeTimer = new QTimer(this);
    removeTimer->setInterval(1000);
    connect(removeTimer, SIGNAL(timeout()), this, SLOT(removeTimeoutDanmaku()));

    // 自动刷新是否启用
    bool autoRefresh = settings.value("danmaku/autoRefresh", true).toBool();
    ui->refreshDanmakuCheck->setChecked(autoRefresh);

    // 移除间隔
    this->removeDanmakuInterval = settings.value("danmaku/removeInterval", 20).toLongLong();
    ui->removeDanmakuIntervalSpin->setValue(static_cast<int>(removeDanmakuInterval));
    this->removeDanmakuInterval *= 1000;

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
       qDebug() << "【点歌自动复制】" << text;
       ui->DiangeAutoCopyCheck->setText("点歌自动复制（" + text + "）");
    });

    // 自动翻译
    bool trans = settings.value("danmaku/autoTrans", true).toBool();
    ui->languageAutoTranslateCheck->setChecked(trans);

    // 自动回复
    bool reply = settings.value("danmaku/aiReply", true).toBool();
    ui->AIReplyCheck->setChecked(reply);

    // 实时弹幕
    if (settings.value("danmaku/liveWindow", false).toBool())
    {
        on_showLiveDanmakuButton_clicked();
    }

    // 发送弹幕
    browserCookie = settings.value("danmaku/browserCookie", "").toString();
    browserData = settings.value("danmaku/browserData", "").toString();

    // 状态栏
    statusLabel = new QLabel(this);
    this->statusBar()->addWidget(statusLabel);

    // 定时任务
    srand((unsigned)time(0));
    restoreTaskList();

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
        danmakuWindow->deleteLater();
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
            lds.append(LiveDanmaku::fromJson(danmakus.at(i).toObject()));
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
    qDebug() << "+++++新弹幕：" <<danmaku.toString();
    emit signalNewDanmaku(danmaku);
}

void MainWindow::oldLiveDanmakuRemoved(LiveDanmaku danmaku)
{
//    qDebug() << "-----旧弹幕：" << danmaku.toString();
    emit signalRemoveDanmaku(danmaku);
}

void MainWindow::sendMsg(QString msg)
{
    if (browserCookie.isEmpty() || browserData.isEmpty())
    {
        QMessageBox::warning(this, "无法发送弹幕", "未设置用户信息，请点击【弹幕-帮助】查看操作");
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
        QString msg = object.value("message").toString();
        statusLabel->setText("");
        if (!msg.isEmpty())
            statusLabel->setText(msg);
    });

    manager->post(*request, ba);
}

/**
 * 显示实时弹幕
 */
void MainWindow::on_showLiveDanmakuButton_clicked()
{
    bool hidding = (danmakuWindow == nullptr || danmakuWindow->isHidden());
    if (danmakuWindow == nullptr)
    {
        danmakuWindow = new LiveDanmakuWindow(this);
        connect(this, SIGNAL(signalNewDanmaku(LiveDanmaku)), danmakuWindow, SLOT(slotNewLiveDanmaku(LiveDanmaku)));
        connect(this, SIGNAL(signalRemoveDanmaku(LiveDanmaku)), danmakuWindow, SLOT(slotOldLiveDanmakuRemoved(LiveDanmaku)));
        connect(danmakuWindow, SIGNAL(signalSendMsg(QString)), this, SLOT(sendMsg(QString)));
        danmakuWindow->setAutoTranslate(ui->languageAutoTranslateCheck->isChecked());
        danmakuWindow->setAIReply(ui->AIReplyCheck->isChecked());
    }

    if (hidding)
    {
        danmakuWindow->show();
        settings.setValue("danmaku/liveWindow", true);
    }
    else
    {
        danmakuWindow->hide();
        settings.setValue("danmaku/liveWindow", false);
    }
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
    appendNewLiveDanmakus(QList<LiveDanmaku>{
                LiveDanmaku("测试用户", text,
                            qrand() % 89999999 + 10000000,
                             QDateTime::currentDateTime())});

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
    if (roomId == ui->roomIdEdit->text())
        return ;
    roomId = ui->roomIdEdit->text();
    QString old = settings.value("danmaku/roomId", "").toString();
    if (roomId.isEmpty()||& old == roomId)
        return ;
    settings.setValue("danmaku/roomId", roomId);
#ifndef SOCKET_MODE
    firstPullDanmaku = true;
    prevLastDanmakuTimestamp = 0;
#else
    if (socket)
    {
        if (socket->state() != QAbstractSocket::UnconnectedState)
            socket->abort();
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
    QString steps;
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
}

void MainWindow::initWS()
{
    socket = new QWebSocket();

    connect(socket, &QWebSocket::connected, this, [=]{
        qDebug() << "socket connected";

        // 5秒内发送认证包
        sendVeriPacket();

        // 定时发送心跳包
        heartTimer->start();
    });

    connect(socket, &QWebSocket::disconnected, this, [=]{
        qDebug() << "disconnected";

        heartTimer->stop();
    });

    connect(socket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
//        qDebug() << "binaryMessageReceived" << message;
        slotBinaryMessageReceived(message);
    });

    connect(socket, &QWebSocket::textFrameReceived, this, [=](const QString &frame, bool isLastFrame){
        qDebug() << "textFrameReceived";
    });

    connect(socket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
        qDebug() << "textMessageReceived";
    });

    connect(socket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        qDebug() << "stateChanged" << state;
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
//    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
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
        if (json.value("code").toInt() != 0)
        {
            qDebug() << "返回结果不为0：" << json.value("message").toString();
            return ;
        }

        QJsonObject roomInfo = json.value("data").toObject().value("room_info").toObject();
        roomId = QString::number(roomInfo.value("room_id").toInt()); // 应当一样
        ui->roomIdEdit->setText(roomId);
        shortId = QString::number(roomInfo.value("short_id").toInt());
        uid = QString::number(roomInfo.value("uid").toInt());
        qDebug() << "getRoomInfo: roomid=" << roomId
                 << "  shortid=" << shortId
                 << "  uid=" << uid;

        getDanmuInfo();
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
            qDebug() << "返回结果不为0：" << json.value("message").toString();
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
        qDebug() << "getDanmuInfo: host数量=" << hostList.size() << "  token=" << token;

        startMsgLoop();
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
    SOCKET_DEB << "发送心跳包：" << ba;
    socket->sendBinaryMessage(ba);
}

/**
 * 解压数据
 * 这个方法必须要qDebug输出一个东西才能用
 * 否则只能拆开放到要用的地方
 */
QByteArray MainWindow::zlibUncompress(QByteArray ba)
{
    unsigned long si;
    BYTE* target = new BYTE[ba.size()*5+10000]{};
    uncompress(target, &si, (unsigned char*)ba.data(), ba.size());
    SOCKET_DEB << "解压后数据大小：" << si << ba.size(); // 这句话不能删！
    return QByteArray::fromRawData((char*)target, si);
}

void MainWindow::slotBinaryMessageReceived(const QByteArray &message)
{
    int operation = (message[8] << 24)
            + (message[9] << 16)
            + (message[10] << 8)
            + (message[11]);
    QByteArray body = message.right(message.length() - 16);
    SOCKET_DEB << "操作码=" << operation << "  正文=" << body;

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(body, &error);
    QJsonObject json;
    if (error.error == QJsonParseError::NoError)
        json = document.object();

    if (operation == AUTH_REPLY) // 认证包回复
    {
        if (json.value("code").toInt() != 0)
        {
            qDebug() << "认证出错";
        }
    }
    else if (operation == HEARTBEAT_REPLY) // 心跳包回复（人气值）
    {
        qint32 popularity = (body[0] << 24)
                + (body[1] << 16)
                + (body[2] << 8)
                + body[3];
        SOCKET_DEB << "人气值=" << popularity;
    }
    else if (operation == SEND_MSG_REPLY) // 普通包
    {
        QString cmd;
        if (!json.isEmpty())
            cmd = json.value("cmd").toString();
        if (cmd == "NOTICE_MSG") // 全站广播（不用管）
        {

        }
        else
        {
            short protover = (message[6]<<8) + message[7];
            qDebug() << "协议版本=" << protover;
            if (protover == 2) // 默认协议版本，zlib解压
            {
                QFile writeFile("receive.txt");
                writeFile.open(QIODevice::WriteOnly);
                writeFile.write(body, body.size());
                writeFile.close();

                QFile readFile("receive.txt");
                readFile.open(QIODevice::ReadWrite);
                auto ba = readFile.readAll();
                QByteArray unc = zlibUncompress(ba);
                SOCKET_DEB << "解压后的数据：" << unc;

                // 循环遍历
                int offset = 0;
                short headerSize = 16;
                while (offset < unc.size() - headerSize)
                {
                    int packSize = ((uchar)unc[offset+0] << 24)
                            + ((uchar)unc[offset+1] << 16)
                            + ((uchar)unc[offset+2] << 8)
                            + (uchar)unc[offset+3];
                    QByteArray jsonBa = unc.mid(offset + headerSize, packSize - headerSize);
                    SOCKET_DEB << "单个JSON消息：" << offset << packSize << jsonBa;
                    QJsonDocument document = QJsonDocument::fromJson(jsonBa, &error);
                    if (error.error != QJsonParseError::NoError)
                    {
                        qDebug() << "解析解压后的JSON出错：" << error.errorString();
                        qDebug() << "包数值：" << offset << packSize << (unsigned int)unc[0] << (unsigned int)unc[1] << (unsigned int)unc[2] << (unsigned int)unc[3];
                        qDebug() << ">>当前JSON" << jsonBa;
                        qDebug() << ">>解压正文" << unc;
                        return ;
                    }
                    QJsonObject json = document.object();
                    handleMessage(json);
                    offset += packSize;
                }

                delete[] ba.data();
                delete[] unc.data();
            }
            else
            {
                qDebug() << "未知协议：" << protover << "，若有必要请处理";
                qDebug() << "未知正文：" << body;
            }

            /*QFile file("receive.txt");
            file.open(QIODevice::WriteOnly);
            file.write(message, message.size());
            file.close();*/
        }
    }
    else
    {

    }
    SOCKET_DEB << "消息处理结束";
}

/**
 * 数据包解析：https://segmentfault.com/a/1190000017328813?utm_source=tag-newest#tagDataPackage
 */
void MainWindow::handleMessage(QJsonObject json)
{
    QString cmd = json.value("cmd").toString();
    qDebug() << ">消息命令：" << cmd;
    if (cmd == "DANMU_MSG") // 受到弹幕
    {
        QJsonArray info = json.value("info").toArray();
        QJsonArray array = info[0].toArray();
        qint64 color = array[3].toInt();
        qint64 timestamp = static_cast<qint64>(array[4].toDouble());
        QString msg = info[1].toString();
        QJsonArray user = info[2].toArray();
        qint64 uid = static_cast<qint64>(user[0].toDouble());
        QString username = user[1].toString();

        qDebug() << "接收到弹幕：" << username << msg << QDateTime::fromSecsSinceEpoch(timestamp);
        appendNewLiveDanmaku(LiveDanmaku(username, msg, uid, QDateTime::fromSecsSinceEpoch(timestamp)));
    }
    else if (cmd == "SEND_GIFT") // 有人送礼
    {
        QJsonObject data = json.value("data").toObject();
        QString giftName = data.value("giftName").toString();
        QString username = data.value("uname").toString();
        int num = data.value("num").toInt();
        qint64 timestamp = static_cast<qint64>(data.value("timestamp").toDouble());
        qDebug() << "接收到送礼：" << username << giftName << num;
    }
    else if (cmd == "GUARD_BUY") // 有人上舰
    {

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
        int uid = data.value("uid").toInt();
        QString username = data.value("username").toString();
        qDebug() << "舰长进入：" << username;
    }
    else  if (cmd == "ENTRY_EFFECT") // 舰长进入的同时会出现
    {

    }
    else if (cmd == "WELCOME") // 进入（不知道有没有包括舰长）
    {
        QJsonObject data = json.value("data").toObject();
        int uid = data.value("uid").toInt();
        QString username = data.value("uname").toString();
        bool isAdmin = data.value("isAdmin").toBool();
        qDebug() << "观众进入：" << username << isAdmin;
    }
    else if (cmd == "INTERACT_WORD")
    {
        QJsonObject data = json.value("data").toObject();
        int uid = data.value("uid").toInt();
        QString username = data.value("uname").toString();
        qDebug() << "观众进入：" << username;
    }
}
