#ifndef EVENTWIDGET_H
#define EVENTWIDGET_H

#include <QObject>
#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextDocument>
#include <QDebug>
#include "livedanmaku.h"

class EventWidget : public QWidget
{
    Q_OBJECT
public:
    EventWidget(QWidget *parent = nullptr);

signals:
    void signalEventMsgs(QString msgs, LiveDanmaku danmaku, bool manual);
    void spinChanged(int val);
    void signalResized();

public slots:
    void slotCmdEvent(QString cmd, LiveDanmaku danmaku);
    void autoResizeEdit();

protected:
    void showEvent(QShowEvent *event) override;

public:
    QCheckBox* check;
    QPushButton* btn;
    QLineEdit* eventEdit;
    QPlainTextEdit* actionEdit;

    QString cmdKey;
};

#endif // EVENTWIDGET_H
