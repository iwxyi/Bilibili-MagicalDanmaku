#ifndef LIVEDANMAKU_H
#define LIVEDANMAKU_H

#include <QString>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

enum MessageType
{
    MSG_DANMAKU,
    MSG_GIFT,
    MSG_WELCOME
};

class LiveDanmaku
{
public:
    LiveDanmaku()
    {}

    LiveDanmaku(QString nickname, QString text, qint64 uid, QDateTime time)
        : msgType(MSG_DANMAKU), nickname(nickname), text(text), uid(uid), timeline(time)
    {

    }

    LiveDanmaku(QString nickname, QString gift, int num, qint64 uid, QDateTime time)
        : msgType(MSG_GIFT), nickname(nickname), giftName(gift), number(num), uid(uid), timeline(time)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QDateTime time)
        : msgType(MSG_WELCOME), nickname(nickname), uid(uid), timeline(time)
    {

    }

    static LiveDanmaku fromDanmakuJson(QJsonObject object)
    {
        LiveDanmaku danmaku;
        danmaku.text = object.value("text").toString();
        danmaku.uid = object.value("uid").toInt();
        danmaku.nickname = object.value("nickname").toString();
        danmaku.uname_color = object.value("uname_color").toString();
        danmaku.timeline = QDateTime::fromString(
                    object.value("timeline").toString(),
                    "yyyy-MM-dd hh:mm:ss");
        danmaku.isadmin = object.value("isadmin").toInt();
        danmaku.vip = object.value("vip").toInt();
        danmaku.svip = object.value("svip").toInt();
        QJsonArray medal = object.value("medal").toArray();
        if (medal.size() >= 3)
        {
            danmaku.medal_level = medal[0].toInt();
            danmaku.medal_up = medal[1].toString();
            danmaku.medal_name = medal[2].toString();
        }
        danmaku.giftName = object.value("giftName").toString();
        danmaku.number = object.value("number").toInt();
        danmaku.msgType = (MessageType)object.value("msgType").toInt();
        return danmaku;
    }

    QJsonObject toJson()
    {
        QJsonObject object;
        if (msgType == MSG_DANMAKU)
        {
            object.insert("text", text);
            object.insert("uid", uid);
            object.insert("nickname", nickname);
            object.insert("uname_color", uname_color);
            object.insert("timeline", timeline.toString("yyyy-MM-dd hh:mm:ss"));
            object.insert("isadmin", isadmin);
            object.insert("vip", vip);
            object.insert("svip", svip);
        }
        else if (msgType == MSG_GIFT)
        {
            object.insert("nickname", nickname);
            object.insert("uid", uid);
            object.insert("giftName", giftName);
            object.insert("number", number);
            object.insert("timeline", timeline.toString("yyyy-MM-dd hh:mm:ss"));
        }
        else if (msgType == MSG_WELCOME)
        {
            object.insert("nickname", nickname);
            object.insert("uid", uid);
            object.insert("timeline", timeline.toString("yyyy-MM-dd hh:mm:ss"));
        }
        object.insert("msgType", (int)msgType);

        return object;
    }

    QString toString() const
    {
        if (msgType == MSG_DANMAKU)
        {
            return QString("%1(%3):“%2” %4")
                    .arg(nickname)
                    .arg(text)
                    .arg(uid)
                    .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_GIFT)
        {
            return QString("%1(%3) => %2 × %4")
                    .arg(nickname)
                    .arg(giftName)
                    .arg(uid)
                    .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_WELCOME)
        {
            return QString("%1(%2) come %3")
                    .arg(nickname)
                    .arg(uid)
                    .arg(timeline.toString("hh:mm:ss"));
        }
        return "未知消息类型";
    }

    bool equal(const LiveDanmaku& another) const
    {
        return this->uid == another.uid
                && this->text == another.text;
    }

    QString getText() const
    {
        return text;
    }

    qint64 getUid() const
    {
        return uid;
    }

    QString getNickname() const
    {
        return nickname;
    }

    QString getUnameColor() const
    {
        return uname_color;
    }

    QDateTime getTimeline() const
    {
        return timeline;
    }

    bool isAdmin() const
    {
        return isadmin;
    }

    bool isIn(const QList<LiveDanmaku>& danmakus) const
    {
        int index = danmakus.size();
        while (--index >= 0)
        {
            if (this->equal(danmakus.at(index)))
                return true;
        }
        return false;
    }

    MessageType getMsgType() const
    {
        return msgType;
    }

    QString getGiftName() const
    {
        return giftName;
    }

    int getNumber() const
    {
        return number;
    }

private:
    MessageType msgType = MSG_DANMAKU;

    QString text;
    qint64 uid = 0; // 用户ID
    QString nickname;
    QString uname_color; // 没有的话是空的
    QDateTime timeline;
    int isadmin = 0; // 房管
    int vip = 0;
    int svip = 0;

    int medal_level = 0;
    QString medal_name;
    QString medal_up;

    int level = 10000;
    qint64 teamid;
    int rnd = 0;
    QString user_title;
    int guard_level = 0;
    int bubble = 0;
    QString bubble_color;
    qint64 check_info_ts;
    QString check_info_ct;
    int lpl = 0;

    QString giftName;
    int number = 0;
};

#endif // LIVEDANMAKU_H
