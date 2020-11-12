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
    MSG_ATTENTION,
    MSG_BLOCK
};

class LiveDanmaku
{
public:
    LiveDanmaku()
    {}

    LiveDanmaku(QString nickname, QString text, qint64 uid, int level, QDateTime time, QString unameColor, QString textColor)
        : msgType(MSG_DANMAKU), nickname(nickname), text(text), uid(uid), level(level), timeline(time),
          uname_color(unameColor), text_color(textColor)
    {

    }

    LiveDanmaku(QString nickname, QString gift, int num, qint64 uid, QDateTime time, QString coinType, int totalCoin)
        : msgType(MSG_GIFT), nickname(nickname), giftName(gift), number(num), uid(uid), timeline(time),
          coin_type(coinType), total_coin(totalCoin)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QDateTime time, bool isAdmin, QString unameColor, QString spreadDesc, QString spreadInfo)
        : msgType(MSG_WELCOME), nickname(nickname), uid(uid), timeline(time), isadmin(isAdmin),
          uname_color(unameColor), spread_desc(spreadDesc), spread_info(spreadInfo)
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

    LiveDanmaku(QString nickname, qint64 uid)
        : msgType(MSG_BLOCK), nickname(nickname), uid(uid)
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
        danmaku.level = object.value("level").toInt();
        QJsonArray medal = object.value("medal").toArray();
        if (medal.size() >= 3)
        {
            danmaku.medal_level = medal[0].toInt();
            danmaku.medal_up = medal[1].toString();
            danmaku.medal_name = medal[2].toString();
        }
        danmaku.giftName = object.value("gift_name").toString();
        danmaku.number = object.value("number").toInt();
        danmaku.coin_type = object.value("coin_type").toString();
        danmaku.total_coin = object.value("total_coin").toInt();
        danmaku.spread_desc = object.value("spread_desc").toString();
        danmaku.spread_info = object.value("spread_info").toString();
        danmaku.fans = object.value("fans").toInt();
        danmaku.fans_club = object.value("fans_club").toInt();
        danmaku.delta_fans = object.value("delta_fans").toInt();
        danmaku.delta_fans_club = object.value("delta_fans_club").toInt();
        danmaku.attention = object.value("attention").toBool();
        danmaku.msgType = (MessageType)object.value("msgType").toInt();
        danmaku.anchor_roomid = object.value("anchor_roomid").toString();
        danmaku.medal_name = object.value("medal_name").toString();
        danmaku.medal_level = object.value("medal_level").toInt();
        danmaku.medal_up = object.value("medal_up").toString();
        danmaku.no_reply = object.value("no_reply").toBool();
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
            object.insert("level", level);
            object.insert("no_reply", no_reply);
        }
        else if (msgType == MSG_GIFT || msgType == MSG_GUARD_BUY)
        {
            object.insert("gift_name", giftName);
            object.insert("number", number);
            object.insert("coin_type", coin_type);
            object.insert("total_coin", total_coin);
        }
        else if (msgType == MSG_WELCOME)
        {
            object.insert("isadmin", isadmin);
            object.insert("uname_color", uname_color);
            object.insert("spread_desc", spread_desc);
            object.insert("spread_info", spread_info);
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
        if (!anchor_roomid.isEmpty())
        {
            object.insert("anchor_roomid", anchor_roomid);
            object.insert("medal_name", medal_name);
            object.insert("medal_level", medal_level);
            object.insert("medal_up", medal_up);
        }

        return object;
    }

    QString toString() const
    {
        if (msgType == MSG_DANMAKU)
        {
            return QString("%1\t%2\t%3")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname)
                    .arg(text);
        }
        else if (msgType == MSG_GIFT || msgType == MSG_GUARD_BUY)
        {
            return QString("%1\t%2 => %3 × %4")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname)
                    .arg(giftName)
                    .arg(number);
        }
        else if (msgType == MSG_WELCOME)
        {
            if (isAdmin())
                return QString("%1\t[光临] 舰长 %2")
                        .arg(timeline.toString("hh:mm:ss"))
                        .arg(nickname);
            return QString("%1\t[欢迎] %2 进入直播间%3")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname).arg(spread_desc.isEmpty() ? "" : (" "+spread_desc));
        }
        else if (msgType == MSG_DIANGE)
        {
            return QString("%3\t[点歌] %1 (%2)")
                                .arg(text).arg(nickname)
                                .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_FANS)
        {
            return QString("%3\t[粉丝] 粉丝数：%1，粉丝团：%2")
                    .arg(fans)
                    .arg(fans_club)
                    .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_ATTENTION)
        {
            return QString("%3\t[关注] %1 %2")
                    .arg(nickname)
                    .arg(attention ? "关注了主播" : "取消关注主播")
                    .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_BLOCK)
        {
            return QString("[禁言] %1 被房管禁言")
                                .arg(nickname);
        }
        return "未知消息类型";
    }

    void setMedal(QString roomId, QString name, int level, QString up = "")
    {
        this->anchor_roomid = roomId;
        this->medal_name = name;
        this->medal_level = level;
        this->medal_up = up;
    }

    void setNoReply()
    {
        no_reply = true;
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

    int getLevel() const
    {
        return level;
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

    bool isGoldCoin() const
    {
        return coin_type != "silver";
    }

    int getTotalCoin() const
    {
        return total_coin;
    }

    QString getSpreadDesc() const
    {
        return spread_desc;
    }

    QString getSpreadInfo() const
    {
        return spread_info;
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

    QString getAnchorRoomid() const
    {
        return anchor_roomid;
    }

    QString getMedalName() const
    {
        return medal_name;
    }

    int getMedalLevel() const
    {
        return medal_level;
    }

    QString getMedalUp() const
    {
        return medal_up;
    }

    bool isNoReply() const
    {
        return no_reply;
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
    bool no_reply = false;

    QString anchor_roomid;
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
    QString coin_type;
    int total_coin = 0;

    QString spread_desc;
    QString spread_info;

    int fans = 0;
    int fans_club = 0;
    int delta_fans = 0;
    int delta_fans_club = 0;
    bool attention = false;
};

#endif // LIVEDANMAKU_H
