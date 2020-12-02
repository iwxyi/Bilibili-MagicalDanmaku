#ifndef LUCKYDRAWWINDOW_H
#define LUCKYDRAWWINDOW_H

#include <QMainWindow>
#include <QInputDialog>
#include <QSettings>
#include <QActionGroup>
#include <QTimer>
#include <QMessageBox>
#include <QDebug>
#include "livedanmaku.h"

namespace Ui {
class LuckyDrawWindow;
}

class LuckyDrawWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit LuckyDrawWindow(QWidget *parent = nullptr);
    ~LuckyDrawWindow();

    enum LotteryType
    {
        LotteryDanmaku,
        LotteryGift
    };

public slots:
    void slotNewDanmaku(LiveDanmaku danmaku);
    void slotCountdown();
    void startWaiting();
    void finishWaiting();

private slots:
    void on_actionSet_Danmaku_Msg_triggered();

    void on_actionSet_Gift_Name_triggered();

    void on_titleButton_clicked();

    void on_countdownButton_clicked();

private:
    bool isTypeValid();

private:
    Ui::LuckyDrawWindow *ui;
    QSettings settings;

    LotteryType lotteryType = LotteryDanmaku;
    QString lotteryDanmakuMsg;
    QString lotteryGiftName;

    int countdownSecond = 60;
    qint64 deadlineTimestamp = 0;
    QTimer* countdownTimer;

    QList<LiveDanmaku> participants;
};

#endif // LUCKYDRAWWINDOW_H
