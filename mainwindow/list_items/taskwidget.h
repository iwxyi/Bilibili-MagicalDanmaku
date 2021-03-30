#ifndef TASKWIDGET_H
#define TASKWIDGET_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextDocument>
#include <QDebug>
#include "listiteminterface.h"
#include "livedanmaku.h"

#define CODE_TIMER_TASK_KEY (QApplication::applicationName() + ":TimerTask")

class TaskWidget : public ListItemInterface
{
    Q_OBJECT
public:
    TaskWidget(QWidget *parent = nullptr);

    virtual void fromJson(MyJson json) override;
    virtual MyJson toJson() const override;

signals:
    void signalSendMsgs(QString msgs, bool manual);
    void spinChanged(int val);

public slots:
    void slotSpinChanged(int val);
    void autoResizeEdit() override;
    void triggerAction(LiveDanmaku);

public:
    QTimer* timer;
    QCheckBox* check;
    QSpinBox* spin;
    QPushButton* btn;
    QPlainTextEdit* edit;
};

#endif // TASKWIDGET_H
