#include "liveroomservice.h"
#include "usersettings.h"
#include "accountinfo.h"

LiveRoomService::LiveRoomService(QObject *parent) : QObject(parent)
{
    initObjects();
}

/**
 * 初始化所有对象
 * 只在创建的时候调用
 */
void LiveRoomService::initObjects()
{
    comboTimer = new QTimer(this);
    comboTimer->setInterval(500);
    connect(comboTimer, SIGNAL(timeout()), this, SLOT(slotComboSend()));

    privateMsgTimestamp = QDateTime::currentMSecsSinceEpoch();

    // 大乱斗
    pkTimer = new QTimer(this);
    connect(pkTimer, &QTimer::timeout, this, [=]{
        // 更新PK信息
        int second = 0;
        int minute = 0;
        if (pkEndTime)
        {
            second = static_cast<int>(pkEndTime - QDateTime::currentSecsSinceEpoch());
            if (second < 0) // 结束后会继续等待一段时间，这时候会变成负数
                second = 0;
            minute = second / 60;
            second = second % 60;
        }
        QString text = QString("%1:%2 %3/%4")
                .arg(minute)
                .arg((second < 10 ? "0" : "") + QString::number(second))
                .arg(myVotes)
                .arg(matchVotes);
        if (danmakuWindow)
            danmakuWindow->setStatusText(text);
    });
    pkTimer->setInterval(300);

    // 大乱斗自动赠送吃瓜
    pkEndingTimer = new QTimer(this);
    connect(pkEndingTimer, &QTimer::timeout, this, &MainWindow::slotPkEndingTimeout);
    pkMaxGold = us->value("pk/maxGold", 300).toInt();
    pkJudgeEarly = us->value("pk/judgeEarly", 2000).toInt();
    toutaCount = us->value("pk/toutaCount", 0).toInt();
    chiguaCount = us->value("pk/chiguaCount", 0).toInt();
    toutaGold = us->value("pk/toutaGold", 0).toInt();
    goldTransPk = us->value("pk/goldTransPk", goldTransPk).toInt();
    toutaBlankList = us->value("pk/blankList").toString().split(";");

    // PK串门提示
    pkChuanmenEnable = us->value("pk/chuanmen", false).toBool();
    ui->pkChuanmenCheck->setChecked(pkChuanmenEnable);
}

/**
 * 读取配置
 * 可能会读取多遍以刷新用户的更改
 */
void LiveRoomService::readConfig()
{
    hostUseIndex = us->value("live/hostIndex").toInt();
}
