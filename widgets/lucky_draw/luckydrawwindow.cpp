#include "luckydrawwindow.h"
#include "ui_luckydrawwindow.h"

LuckyDrawWindow::LuckyDrawWindow(QSettings* st, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LuckyDrawWindow),
    settings(st),
    countdownTimer(new QTimer(this))
{
    ui->setupUi(this);

    lotteryType = (LotteryType)settings.value("lottery/type", 0).toInt();
    lotteryDanmakuMsg =  settings.value("lottery/danmakuMsg", "我要抽奖").toString();
    lotteryGiftName = settings.value("lottery/giftName", "吃瓜").toString();

    QActionGroup* typeGroup = new QActionGroup(this);
    typeGroup->addAction(ui->actionSet_Danmaku_Msg);
    typeGroup->addAction(ui->actionSet_Gift_Name);
    if (lotteryType == LotteryDanmaku)
        ui->actionSet_Danmaku_Msg->setChecked(true);
    else
        ui->actionSet_Gift_Name->setChecked(true);

    ui->titleButton->setText(settings.value("lottery/title", "一句“我要抽奖”参与抽奖").toString());
    ui->onlyLastSpin->setValue(settings.value("lottery/onlyLast", 1).toInt());

    countdownTimer->setInterval(300);
    connect(countdownTimer, SIGNAL(timeout()), this, SLOT(slotCountdown()));

    ui->removeHalfButton->setEnabled(false);
    ui->onlyLastButton->setEnabled(false);

    QTime time;
    time= QTime::currentTime();
    qsrand(time.msec()+time.second()*1000);
}

LuckyDrawWindow::~LuckyDrawWindow()
{
    delete ui;
}

void LuckyDrawWindow::slotNewDanmaku(const LiveDanmaku &danmaku)
{
    if (this->isHidden() || !isWaiting())
        return ;

    // 过滤内容
    if (lotteryType == LotteryDanmaku && danmaku.getMsgType() == MSG_DANMAKU)
    {
        if (danmaku.getText() != lotteryDanmakuMsg)
            return ;
    }
    else if (lotteryType == LotteryGift && danmaku.getMsgType() == MSG_GIFT)
    {
        if (danmaku.getGiftName() != lotteryGiftName)
            return ;
    }
    else
        return ;

    // 过滤条件
    if (ui->actionCondition_Fans->isChecked())
    {

    }
    if (ui->actionCondition_Guard->isChecked())
    {

    }

    // 避免重复
    qint64 uid = danmaku.getUid();
    foreach (LiveDanmaku danmaku, participants)
        if (danmaku.getUid() == uid)
            return ;

    // 添加到列表
    participants.append(danmaku);
    ui->listWidget->addItem(danmaku.getNickname());
}

void LuckyDrawWindow::slotCountdown()
{
    if (!deadlineTimestamp || !isTypeValid())
        return ;

    qint64 delta = deadlineTimestamp - QDateTime::currentSecsSinceEpoch();
    if (delta > 0)
    {
        int minute = delta / 60;
        int second = delta % 60;
        if (!minute)
            ui->countdownButton->setText("倒计时：" + QString::number(second) + "秒");
        else
            ui->countdownButton->setText(QString("倒计时：%1分%2秒").arg(minute).arg(second));
        return ;
    }

    // 时间截止了
    finishWaiting();
}

void LuckyDrawWindow::startWaiting()
{
    participants.clear();
    ui->listWidget->clear();
    countdownTimer->start();

    ui->removeHalfButton->setEnabled(false);
    ui->onlyLastButton->setEnabled(false);
    ui->actionSet_Danmaku_Msg->setEnabled(false);
    ui->actionSet_Gift_Name->setEnabled(false);
}

void LuckyDrawWindow::finishWaiting()
{
    countdownTimer->stop();
    ui->countdownButton->setText("正在抽奖");
    ui->actionSet_Danmaku_Msg->setEnabled(true);
    ui->actionSet_Gift_Name->setEnabled(true);

    // 有人参与
    if (participants.size())
    {
        ui->removeHalfButton->setEnabled(true);
        ui->onlyLastButton->setEnabled(true);
    }
}

/**
 * 切换到弹幕抽奖
 */
void LuckyDrawWindow::on_actionSet_Danmaku_Msg_triggered()
{
    bool ok;
    QString text = QInputDialog::getText(this, "弹幕抽奖", "请输入完整的抽奖弹幕", QLineEdit::Normal, lotteryDanmakuMsg, &ok);
    if (!ok)
        return ;
    settings.setValue("lottery/danmakuMsg", lotteryDanmakuMsg = text);
    lotteryType = LotteryDanmaku;
    ui->actionSet_Danmaku_Msg->setChecked(true);
}

/**
 * 切换到礼物抽奖
 */
void LuckyDrawWindow::on_actionSet_Gift_Name_triggered()
{
    bool ok;
    QString text = QInputDialog::getText(this, "礼物抽奖", "请输入完整的礼物名字", QLineEdit::Normal, lotteryGiftName, &ok);
    if (!ok)
        return ;
    settings.setValue("lottery/giftName", lotteryGiftName = text);
    lotteryType = LotteryGift;
    ui->actionSet_Gift_Name->setChecked(true);
}

void LuckyDrawWindow::on_titleButton_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, "标题", "请输入大标题提示", QLineEdit::Normal, ui->titleButton->text(), &ok);
    if (!ok)
        return ;
    settings.setValue("lottery/title", text);
    ui->titleButton->setText(text);
}

void LuckyDrawWindow::on_countdownButton_clicked()
{
    if (isWaiting())
    {
        if (QMessageBox::question(this, "停止倒计时", "是否停止倒计时，立即抽奖？\n当前已参与抽奖的粉丝将保留") == QMessageBox::Yes)
        {
            finishWaiting();
        }
        return ;
    }

    if (!isTypeValid())
    {
        QMessageBox::warning(this, "提示", "请先设置抽奖类型，最后设置倒计时");
        return ;
    }

    bool ok;
    int second = QInputDialog::getInt(this, "倒计时", "请输入倒计时时长，单位：秒", countdownSecond, 10, 24*3600, 10, &ok);
    if (!ok)
        return ;

    settings.setValue("lottery/countdown", countdownSecond = second);
    deadlineTimestamp = QDateTime::currentSecsSinceEpoch() + second;
    startWaiting();
}

bool LuckyDrawWindow::isWaiting() const
{
    return countdownTimer->isActive();
}

bool LuckyDrawWindow::isTypeValid() const
{
    if (lotteryType == LotteryDanmaku)
        return !lotteryDanmakuMsg.isEmpty();
    if (lotteryType == LotteryGift)
        return !lotteryGiftName.isEmpty();
    return false;
}

/**
 * 只剩下n个
 */
void LuckyDrawWindow::on_onlyLastButton_clicked()
{
    int lastCount = ui->onlyLastSpin->value();
    int allCount = participants.size();
    int delta = allCount - lastCount;
    while (delta-- > 0)
    {
        int r = qrand() % allCount;
        participants.removeAt(r);
        ui->listWidget->takeItem(r);
        allCount--;
        qDebug() << "移除index：" << r;
    }
}

void LuckyDrawWindow::on_onlyLastSpin_editingFinished()
{
    settings.setValue("lottery/onlyLast", ui->onlyLastSpin->value());
}

/**
 * 减去一半
 */
void LuckyDrawWindow::on_removeHalfButton_clicked()
{
    int allCount = participants.size();
    int lastCount = (allCount+1) / 2;
    int delta = allCount - lastCount;
    while (delta-- > 0)
    {
        int r = qrand() % allCount;
        participants.removeAt(r);
        ui->listWidget->takeItem(r);
        allCount--;
        qDebug() << "移除index：" << r;
    }
}
