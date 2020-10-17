#ifndef LIVEDANMAKU_H
#define LIVEDANMAKU_H

#include <QString>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

class LiveDanmaku
{
public:
    LiveDanmaku()
    {}

    static LiveDanmaku fromJson(QJsonObject object)
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
        return danmaku;
    }

    QString toString() const
    {
        return QString("%1(%3):“%2” %4")
                .arg(nickname)
                .arg(text)
                .arg(uid)
                .arg(timeline.toString("hh:mm:ss"));
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

    QString getUnameColoc() const
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

private:
    QString text;
    qint64 uid; // 用户ID
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
};

#endif // LIVEDANMAKU_H
