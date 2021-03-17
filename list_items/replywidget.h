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
#include "listiteminterface.h"

#define CODE_AUTO_REPLY_KEY (QApplication::applicationName() + ":AutoReply")

class ReplyWidget : public ListItemInterface
{
    Q_OBJECT
public:
    ReplyWidget(QWidget *parent = nullptr);

    virtual void fromJson(MyJson json) override;
    virtual MyJson toJson() const override;

signals:
    void signalReplyMsgs(QString msgs, LiveDanmaku danmaku, bool manual);

public slots:
    void slotNewDanmaku(LiveDanmaku danmaku);
    void autoResizeEdit() override;

public:
    QCheckBox* check;
    QPushButton* btn;
    QLineEdit* keyEdit;
    QPlainTextEdit* replyEdit;

    bool keyEmpty = true;
    QRegularExpression keyRe;
};

#endif // REPLYWIDGET_H
