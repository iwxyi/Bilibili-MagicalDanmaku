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
    MSG_WELCOME,
    MSG_DIANGE,
    MSG_GUARD_BUY,
    MSG_WELCOME_GUARD,
    MSG_FANS,
    MSG_ATTENTION
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

    LiveDanmaku(QString nickname, QString text, qint64 uid, QDateTime time, QString unameColor, QString textColor)
        : msgType(MSG_DANMAKU), nickname(nickname), text(text), uid(uid), timeline(time),
          uname_color(unameColor), text_color(textColor)
    {

    }

    LiveDanmaku(QString nickname, QString gift, int num, qint64 uid, QDateTime time)
        : msgType(MSG_GIFT), nickname(nickname), giftName(gift), number(num), uid(uid), timeline(time)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QDateTime time, bool isAdmin)
        : msgType(MSG_WELCOME), nickname(nickname), uid(uid), timeline(time), isadmin(isAdmin)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QString song, QDateTime time)
        : msgType(MSG_DIANGE), nickname(nickname), uid(uid), text(song), timeline(time)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QString gift, int num)
        : msgType(MSG_GUARD_BUY), nickname(nickname), uid(uid), giftName(gift), number(num), timeline(QDateTime::currentDateTime())
    {

    }

    LiveDanmaku(int fans, int club, int delta_fans, int delta_fans_club)
        : msgType(MSG_FANS), fans(fans), fans_club(club),
          delta_fans(delta_fans), delta_fans_club(delta_fans_club),
          timeline(QDateTime::currentDateTime())
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, bool attention, QDateTime time)
        : msgType(MSG_ATTENTION), nickname(nickname), uid(uid), timeline(time), attention(attention)
    {

    }

    static LiveDanmaku fromDanmakuJson(QJsonObject object)
    {
        LiveDanmaku danmaku;
        danmaku.text = object.value("text").toString();
        danmaku.uid = object.value("uid").toInt();
        danmaku.nickname = object.value("nickname").toString();
        danmaku.uname_color = object.value("uname_color").toString();
        danmaku.text_color = object.value("text_color").toString();
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
        danmaku.fans = object.value("fans").toInt();
        danmaku.fans_club = object.value("fans_club").toInt();
        danmaku.delta_fans = object.value("delta_fans").toInt();
        danmaku.delta_fans_club = object.value("delta_fans_club").toInt();
        danmaku.attention = object.value("attention").toBool();
        danmaku.msgType = (MessageType)object.value("msgType").toInt();
        return danmaku;
    }

    QJsonObject toJson()
    {
        QJsonObject object;
        object.insert("nickname", nickname);
        object.insert("uid", uid);
        if (msgType == MSG_DANMAKU)
        {
            object.insert("text", text);
            object.insert("uname_color", uname_color);
            object.insert("text_color", text_color);
            object.insert("isadmin", isadmin);
            object.insert("vip", vip);
            object.insert("svip", svip);
        }
        else if (msgType == MSG_GIFT || msgType == MSG_GUARD_BUY)
        {
            object.insert("giftName", giftName);
            object.insert("number", number);
        }
        else if (msgType == MSG_WELCOME)
        {
            object.insert("isadmin", isadmin);
        }
        else if (msgType == MSG_DIANGE)
        {
            object.insert("text", text);
        }
        else if (msgType == MSG_FANS)
        {
            object.insert("fans", fans);
            object.insert("fans_club", fans_club);
            object.insert("delta_fans", delta_fans);
            object.insert("delta_fans_club", delta_fans_club);
        }
        else if (msgType == MSG_ATTENTION)
        {
            object.insert("attention", attention);
        }
        object.insert("timeline", timeline.toString("yyyy-MM-dd hh:mm:ss"));
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
        else if (msgType == MSG_GIFT || msgType == MSG_GUARD_BUY)
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
        else if (msgType == MSG_DIANGE)
        {
            return QString("[点歌]%1(%2) come %3")
                                .arg(nickname)
                                .arg(text)
                                .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_FANS)
        {
            return QString("[粉丝] 粉丝数：%1，粉丝团：%2 %3")
                    .arg(fans)
                    .arg(fans_club)
                    .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_ATTENTION)
        {
            return QString("[关注] %1 %2 %3")
                    .arg(nickname)
                    .arg(attention ? "关注了主播" : "取消关注主播")
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

    QString getTextColor() const
    {
        return text_color;
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

    int getFans() const
    {
        return fans;
    }

    int getFansClub() const
    {
        return fans_club;
    }

    int getDeltaFans() const
    {
        return delta_fans;
    }

    int getDeltaFansClub() const
    {
        return delta_fans_club;
    }

    bool isAttention() const
    {
        return attention;
    }

private:
    MessageType msgType = MSG_DANMAKU;

    QString text;
    qint64 uid = 0; // 用户ID
    QString nickname;
    QString uname_color; // 没有的话是空的
    QString text_color; // 没有的话是空的
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

    int fans = 0;
    int fans_club = 0;
    int delta_fans = 0;
    int delta_fans_club = 0;
    bool attention = false;
};

#endif // LIVEDANMAKU_H
