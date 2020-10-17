#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    danmakuTimer = new QTimer(this);
    int interval = settings.value("danmaku/interval", 500).toInt();
    ui->refreshDanmakuIntervalSpin->setValue(interval);
    danmakuTimer->setInterval(interval);
    connect(danmakuTimer, SIGNAL(timeout()), this, SLOT(pullLiveDanmaku()));

    ui->refreshDanmakuCheck->setChecked(true);
}

MainWindow::~MainWindow()
{
    delete ui;
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
        addNewLiveDanmaku(lds);
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

void MainWindow::addNewLiveDanmaku(QList<LiveDanmaku> danmakus)
{
    // 去掉已经存在的弹幕
    QDateTime prevLastTime = roomDanmakus.size()
            ? roomDanmakus.last().getTimeline()
            : QDateTime::fromMSecsSinceEpoch(0);
    while (danmakus.size() && danmakus.first().getTimeline() < prevLastTime)
        danmakus.removeFirst();
    while (danmakus.size() && danmakus.first().isIn(roomDanmakus))
        danmakus.removeFirst();
    if (!danmakus.size())
        return ;

    // 添加到队列
    roomDanmakus.append(danmakus);

    // 发送信号给其他插件
    for (int i = 0; i < danmakus.size(); i++)
    {
        LiveDanmaku danmaku = danmakus.at(i);
        qDebug() << "+++++新弹幕：" <<danmaku.toString();
        emit signalNewDanmaku(danmaku);
    }

    /*QStringList texts;
    for (int i = 0; i < roomDanmakus.size(); i++)
        texts.append(roomDanmakus.at(i).toString());
    qDebug() << "当前弹幕" << texts;*/
}
