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
    QString roomId = settings.value("danmaku/roomId", "").toString();
    if (!roomId.isEmpty())
        ui->roomIdEdit->setText(roomId);

    // 刷新间隔
    danmakuTimer = new QTimer(this);
    int interval = settings.value("danmaku/interval", 500).toInt();
    ui->refreshDanmakuIntervalSpin->setValue(interval);
    danmakuTimer->setInterval(interval);
    connect(danmakuTimer, SIGNAL(timeout()), this, SLOT(pullLiveDanmaku()));
    ui->refreshDanmakuCheck->setChecked(true);

    // 自动刷新是否启用
    bool autoRefresh = settings.value("danmaku/autoRefresh", true).toBool();
    ui->refreshDanmakuCheck->setChecked(autoRefresh);

    // 移除间隔
    this->removeDanmakuInterval = settings.value("danmaku/removeInterval", 20000).toInt();
    ui->removeDanmakuIntervalSpin->setValue(removeDanmakuInterval/1000);

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

    // 实时弹幕
    if (settings.value("danmaku/liveWindow", false).toBool())
    {
        on_showLiveDanmakuButton_clicked();
    }

    // 发送弹幕
    browserCookie = settings.value("danmaku/browserCookie", "").toString();
    browserData = settings.value("danmaku/browserData", "").toString();
    sendMsgTimer = new QTimer(this);
    connect(sendMsgTimer, SIGNAL(timeout()), this, SLOT(on_SendMsgButton_clicked()));

    // 定时弹幕
    interval = settings.value("danmaku/sendInterval", 1800).toInt();
    ui->SendMsgIntervalSpin->setValue(interval);
    sendMsgTimer->setInterval(interval*1000);
    bool autoSend = settings.value("danmaku/autoSend", false).toBool();
    ui->SendMsgIntervalCheck->setChecked(autoSend);
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
    QString roomid = ui->roomIdEdit->text();
    if (roomid.isEmpty())
        return ;
    QString url = "https://api.live.bilibili.com/ajax/msg";
    QStringList param{"roomid", roomid};
    connect(new NetUtil(url, param), &NetUtil::finished, this, [=](QString result){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(result.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        QJsonArray danmakus = json.value("data").toObject().value("room").toArray();
        QList<LiveDanmaku> lds;
        for (int i = 0; i < danmakus.size(); i++)
            lds.append(LiveDanmaku::fromJson(danmakus.at(i).toObject()));
        appendNewLiveDanmaku(lds);
    });
}


void MainWindow::on_refreshDanmakuIntervalSpin_valueChanged(int arg1)
{
    settings.setValue("danmaku/interval", arg1);
    danmakuTimer->setInterval(arg1);
}

void MainWindow::on_refreshDanmakuCheck_stateChanged(int arg1)
{
    if (arg1)
        danmakuTimer->start();
    else
        danmakuTimer->stop();
}

void MainWindow::appendNewLiveDanmaku(QList<LiveDanmaku> danmakus)
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

    // 去掉已经存在的弹幕
    /*QDateTime prevLastTime = roomDanmakus.size()
            ? roomDanmakus.last().getTimeline()
            : QDateTime::fromMSecsSinceEpoch(0);*/
    while (danmakus.size() && danmakus.first().getTimeline().toMSecsSinceEpoch() <= prevLastDanmakuTimestamp)
        danmakus.removeFirst();
    if (!danmakus.size())
        return ;
    prevLastDanmakuTimestamp = danmakus.last().getTimeline().toMSecsSinceEpoch();

    // 不是第一次加载
    if (!firstPullDanmaku) // 用作测试就不需要该条件
    {
        // 发送信号给其他插件
        for (int i = 0; i < danmakus.size(); i++)
        {
            newLiveDanmakuAdded(danmakus.at(i));
        }
    }
    else
        firstPullDanmaku = false;

    // 添加到队列
    roomDanmakus.append(danmakus);
}

void MainWindow::newLiveDanmakuAdded(LiveDanmaku danmaku)
{
    qDebug() << "+++++新弹幕：" <<danmaku.toString();
    emit signalNewDanmaku(danmaku);
}

void MainWindow::oldLiveDanmakuRemoved(LiveDanmaku danmaku)
{
    qDebug() << "-----旧弹幕：" << danmaku.toString();
    emit signalRemoveDanmaku(danmaku);
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
        danmakuWindow->setAutoTranslate(ui->languageAutoTranslateCheck->isChecked());
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
    newLiveDanmakuAdded(
                LiveDanmaku("测试用户", text,
                            qrand() % 89999999 + 10000000,
                            QDateTime::currentDateTime()));

    ui->testDanmakuEdit->setText("");
    ui->testDanmakuEdit->setFocus();
}

void MainWindow::on_removeDanmakuIntervalSpin_valueChanged(int arg1)
{
    this->removeDanmakuInterval = arg1 * 1000;
    settings.setValue("danmaku/removeInterval", removeDanmakuInterval);
}

void MainWindow::on_roomIdEdit_editingFinished()
{
    QString room = ui->roomIdEdit->text();
    QString old = settings.value("danmaku/roomId", "").toString();
    if (room.isEmpty()||& old == room)
        return ;
    settings.setValue("danmaku/roomId", room);
    firstPullDanmaku = true;
    prevLastDanmakuTimestamp = 0;
}

void MainWindow::on_languageAutoTranslateCheck_stateChanged(int arg1)
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

}

void MainWindow::on_SendMsgIntervalSpin_valueChanged(int arg1)
{
    sendMsgTimer->setInterval(arg1 * 1000);
    settings.setValue("danmaku/sendInterval", arg1);
}

void MainWindow::on_SendMsgIntervalCheck_stateChanged(int arg1)
{
    bool enable = ui->SendMsgIntervalCheck->isChecked();
    if (enable)
        sendMsgTimer->start();
    else
        sendMsgTimer->stop();
    settings.setValue("danmaku/autoSend", enable);
}

void MainWindow::on_SendMsgButton_clicked()
{
    if (browserCookie.isEmpty() || browserData.isEmpty())
    {
        QMessageBox::warning(this, "无法发送弹幕", "未设置用户信息，请点击【弹幕-帮助】查看操作");
        ui->SendMsgIntervalCheck->setChecked(false);
        return ;
    }
    QString msg = ui->SendMsgEdit->text();
    QString roomId = ui->roomIdEdit->text();
    if (msg.isEmpty() || roomId.isEmpty())
        return ;

    QUrl url("https://api.live.bilibili.com/msg/send");

    // 建立对象
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
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
    request->setHeader(QNetworkRequest::CookieHeader, var);
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
    qDebug() << ba;

    // 连接槽
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
        qDebug() << data;

    });

    manager->post(*request, ba);
}
