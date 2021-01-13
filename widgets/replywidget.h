#ifndef REPLYWIDGET_H
#define REPLYWIDGET_H

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

class ReplyWidget : public QWidget
{
    Q_OBJECT
public:
    ReplyWidget(QWidget *parent = nullptr);

signals:
    void signalReplyMsgs(QString msgs, LiveDanmaku danmaku, bool manual);
    void spinChanged(int val);
    void signalResized();

public slots:
    void slotNewDanmaku(LiveDanmaku danmaku);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

public:
    QCheckBox* check;
    QPushButton* btn;
    QLineEdit* keyEdit;
    QPlainTextEdit* replyEdit;

    bool keyEmpty = true;
    QRegularExpression keyRe;
};

#endif // REPLYWIDGET_H
