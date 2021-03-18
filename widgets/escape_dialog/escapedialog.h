#ifndef ESCAPEDIALOG_H
#define ESCAPEDIALOG_H

#include <QObject>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QDateTime>
#include <random>
#include <QDebug>
#include <QPropertyAnimation>
#include <QTimer>
#include <QKeyEvent>
#include "hoverbutton.h"

class EscapeDialog : public QDialog
{
#define MARGIN 20
    Q_OBJECT
public:
    EscapeDialog(QString title, QString msg, QString esc, QString nor, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void leaveEvent(QEvent* event) override;

private:
    void resetBtnPos();
    void recoverEscBtnPos();
    void moveEscBtnAni(QPoint aim);
    qint64 getTimestamp();
    int getRandom(int min, int max);
    bool isEqual(int a, int b);

public slots:
    void slotPosEntered(QPoint point);           // 鼠标进入事件：移动按钮或者交换按钮
    void slotEscapeButton(QPoint p = QPoint());  // 移动按钮
    void slotExchangeButton();                   // 交换按钮

private:
    QLabel* msg_lab;
    HoverButton* esc_btn/*accept*/, *nor_btn/*reject*/;

    std::random_device rd;
    std::mt19937 mt;

    bool exchanged; // 两个按钮是否交换了位置
    int escape_count; // 跑动的次数（包括交换）
    int last_escape_index; // 上次交换位置的次数（免得经常性的交换）
    bool has_overlapped; // 是否和另一个按钮进行重叠
};

#endif // ESCAPEDIALOG_H
