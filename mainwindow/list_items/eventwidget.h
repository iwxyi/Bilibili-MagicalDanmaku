#ifndef EVENTWIDGET_H
#define EVENTWIDGET_H

#include <QObject>
#include <QWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextDocument>
#include <QDebug>
#include <QCompleter>
#include <QStandardItemModel>
#include "livedanmaku.h"
#include "listiteminterface.h"
#include "freecopyedit.h"
#if defined(ENABLE_SHORTCUT)
#include "qxtglobalshortcut.h"
#endif
#include "conditioneditor.h"
#include "crontimer.h"

#define CODE_EVENT_ACTION_KEY (QApplication::applicationName() + ":EventAction")

class EventWidget : public ListItemInterface
{
    Q_OBJECT
public:
    EventWidget(QWidget *parent = nullptr);

    virtual void fromJson(MyJson json) override;
    virtual MyJson toJson() const override;

    virtual bool isEnabled() const override;
    virtual QString title() const override;
    virtual QString body() const override;
    virtual bool isMatch(const QString &text) const override;

signals:
    void signalEventMsgs(QString msgs, LiveDanmaku danmaku, bool manual);

public slots:
    void triggerCmdEvent(QString cmd, LiveDanmaku danmaku);
    void triggerAction(LiveDanmaku danmaku);
    void autoResizeEdit() override;
    void createSpecialFunction();
    void deleteSpecialFunction();

public:
    QLineEdit* eventEdit;
    ConditionEditor* actionEdit;
    static QCompleter* completer;

    QString cmdKey;
#if defined(ENABLE_SHORTCUT)
    QxtGlobalShortcut* shortcut = nullptr;
#endif
    CronTimer* cronTimer = nullptr;
};

#endif // EVENTWIDGET_H
