#include "luckydrawwindow.h"
#include "ui_luckydrawwindow.h"

LuckyDrawWindow::LuckyDrawWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LuckyDrawWindow),
    settings(QApplication::applicationDirPath()+"/settings.ini", QSettings::Format::IniFormat),
    countdownTimer(new QTimer(this))
{
    ui->setupUi(this);

    lotteryType = (LotteryType)settings.value("lottery/type", 0).toInt();
    lotteryDanmakuMsg =  settings.value("lottery/danmakuMsg").toString();
    lotteryGiftName = settings.value("lottery/giftName").toString();

    QActionGroup* typeGroup = new QActionGroup(this);
    typeGroup->addAction(ui->actionSet_Danmaku_Msg);
    typeGroup->addAction(ui->actionSet_Gift_Name);
    if (lotteryType == LotteryDanmaku)
        ui->actionSet_Danmaku_Msg->setChecked(true);
    else
        ui->actionSet_Gift_Name->setChecked(true);

    ui->titleButton->setText(settings.value("lottery/title", "一句“我要抽奖”参与抽奖").toString());

    countdownTimer->setInterval(300);
    connect(countdownTimer, SIGNAL(timeout()), this, SLOT(slotCountdown()));

    ui->removeHalfButton->hide();
}

LuckyDrawWindow::~LuckyDrawWindow()
{
    delete ui;
}

void LuckyDrawWindow::slotNewDanmaku(LiveDanmaku danmaku)
{
    if (this->isHidden())
        return ;
    qDebug() << "抽奖机弹幕：" << danmaku.toString();
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
    countdownTimer->start();
    ui->removeHalfButton->hide();
}

void LuckyDrawWindow::finishWaiting()
{
    countdownTimer->stop();
    ui->countdownButton->setText("准备抽奖...");

    // 有人参与
    if (participants.size())
    {
        ui->removeHalfButton->show();
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

bool LuckyDrawWindow::isTypeValid()
{
    if (lotteryType == LotteryDanmaku)
        return !lotteryDanmakuMsg.isEmpty();
    if (lotteryType == LotteryGift)
        return !lotteryGiftName.isEmpty();
    return false;
}
