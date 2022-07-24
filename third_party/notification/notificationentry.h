#ifndef NOTIFICATIONENTRY_H
#define NOTIFICATIONENTRY_H

#include <QObject>
#include <QStringList>

class NotificationEntry : public QObject
{
    Q_OBJECT
public:
    NotificationEntry() : time(5000), click_hide(false), click_at(CAB_NONE), default_btn(CAB_NONE)
    {
    }

    NotificationEntry(QString k, QString t, QString c) : NotificationEntry()
    {
        setInfo(k, t, c);
    }

    enum SuggestButton {
        SB_NULL,
        SB_FIR,
        SB_SEC,
        SB_ALL
    };

    enum ClickAtButton {
        CAB_NONE = -1,
        CAB_CARD,
        CAB_BTN1,
        CAB_BTN2,
        CAB_BTN3
    };

    void setInfo(QString key, QString title, QString content)
    {
        this->key = key;
        this->title = title;
        this->content = content;
    }

    void setBtn(int i, QString btn, QString cmd)
    {
        if (i == 1)
        {
            btn1 = btn;
            cmd1 = cmd;
        }
        if (i == 2)
        {
            btn2 = btn;
            cmd2 = cmd;
        }
        if (i == 3)
        {
            btn3 = btn;
            cmd3 = cmd;
        }
    }

    void setDisplayTime(int ms)
    {
        this->time = ms;
    }

    void setDefaultBtn(int i)
    {
        default_btn = ClickAtButton(i);
    }

    void addFilter(QString f, QString v)
    {
        filters << f;
        values << v;
    }

    NotificationEntry* click(int i)
    {
        if (i == 0)
            click_at = CAB_CARD;
        else if (i == 1)
            click_at = CAB_BTN1;
        else if (i == 2)
            click_at = CAB_BTN2;
        else if (i == 3)
            click_at = CAB_BTN3;
        return this;
    }

    QString toString()
    {
        return QString("key:%1; title:%2; content:%3; click:%4; cmd:%5")
                .arg(key).arg(title).arg(content).arg(click_at).arg(getCmd());
    }

    QString getCmd()
    {
        QString cmd;
        if (click_at == CAB_CARD)
            cmd = desc;
        else if (click_at == CAB_BTN1)
            cmd = cmd1;
        else if (click_at == CAB_BTN2)
            cmd = cmd2;
        else if (click_at == CAB_BTN3)
            cmd = cmd3;
        return cmd;
    }

signals:
    void signalCardClicked();       // 点击卡片
    void signalBtnClicked(int i);   // 点击按钮，0/1/2
    void signalTimeout();           // 超时关闭
    void signalClosed();            // 关闭，可能超时、点击卡片、点击按钮

public:
    QString key;
    QString title;
    QString content;
    QString desc;
    QString btn1;
    int time;
    bool click_hide;
    QString btn2;
    QString btn3;
    QString cmd1;
    QString cmd2;
    QString cmd3;
    QStringList args;
    QStringList filters;
    QStringList values;
    ClickAtButton click_at;
    ClickAtButton default_btn;
};

#endif // NOTIFICATIONENTRY_H
