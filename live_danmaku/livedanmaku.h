#ifndef LIVEDANMAKU_H
#define LIVEDANMAKU_H

#include <QString>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QDebug>

enum MessageType
{
    MSG_DEF,
    MSG_DANMAKU,
    MSG_GIFT,
    MSG_WELCOME,
    MSG_DIANGE,
    MSG_GUARD_BUY,
    MSG_WELCOME_GUARD,
    MSG_FANS,
    MSG_ATTENTION,
    MSG_BLOCK,
    MSG_MSG,
    MSG_SHARE,
    MSG_PK_BEST
};

class LiveDanmaku
{
public:
    LiveDanmaku() : msgType(MSG_DEF)
    {}

    LiveDanmaku(qint64 uid) : msgType(MSG_DEF), uid(uid)
    {}

    LiveDanmaku(qint64 uid, QString text) : msgType(MSG_DEF), uid(uid), text(text)
    {}

    LiveDanmaku(qint64 uid, QString nickname, QString text) : msgType(MSG_DEF), uid(uid), nickname(nickname), text(text)
    {}

    LiveDanmaku(QString nickname, QString text, qint64 uid, int level, QDateTime time, QString unameColor, QString textColor)
        : msgType(MSG_DANMAKU), nickname(nickname), text(text), uid(uid), level(level), timeline(time),
          uname_color(unameColor), text_color(textColor)
    {

    }

    LiveDanmaku(QString nickname, int giftId, QString gift, int num, qint64 uid, QDateTime time, QString coinType, int totalCoin)
        : msgType(MSG_GIFT), nickname(nickname), giftId(giftId), giftName(gift), number(num), uid(uid), timeline(time),
          coin_type(coinType), total_coin(totalCoin)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QDateTime time, bool admin, QString unameColor, QString spreadDesc, QString spreadInfo)
        : msgType(MSG_WELCOME), nickname(nickname), uid(uid), timeline(time), admin(admin),
          uname_color(unameColor), spread_desc(spreadDesc), spread_info(spreadInfo)
    {

    }
    LiveDanmaku(int guard, QString nickname, qint64 uid, QDateTime time)
        : msgType(MSG_WELCOME_GUARD), guard(guard), nickname(nickname), uid(uid), timeline(time)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QString song, QDateTime time)
        : msgType(MSG_DIANGE), nickname(nickname), uid(uid), text(song), timeline(time)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QString gift, int num, int guard, int gift_id, int price, int first)
        : msgType(MSG_GUARD_BUY), nickname(nickname), uid(uid), giftName(gift), number(num), timeline(QDateTime::currentDateTime()),
          giftId(gift_id), guard(guard), coin_type("gold"), total_coin(price), first(first)
    {

    }

    /// 粉丝团？忘了是啥了
    LiveDanmaku(int fans, int club, int delta_fans, int delta_fans_club)
        : msgType(MSG_FANS), fans(fans), fans_club(club),
          delta_fans(delta_fans), delta_fans_club(delta_fans_club),
          timeline(QDateTime::currentDateTime())
    {

    }

    /// 关注（粉丝）
    LiveDanmaku(QString nickname, qint64 uid, bool attention, QDateTime time)
        : msgType(MSG_ATTENTION), nickname(nickname), uid(uid), prev_timestamp(time.toSecsSinceEpoch()),
        timeline(QDateTime::currentDateTime()), attention(attention)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid)
        : msgType(MSG_BLOCK), nickname(nickname), uid(uid), timeline(QDateTime::currentDateTime())
    {

    }

    LiveDanmaku(QString msg)
        : msgType(MSG_MSG), text(msg), timeline(QDateTime::currentDateTime())
    {

    }

    LiveDanmaku(QString uname, int win, int votes)
        : msgType(MSG_PK_BEST), nickname(uname), level(win), total_coin(votes)
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
        danmaku.admin = object.value("admin").toInt();
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
        danmaku.giftId = object.value("gift_id").toInt();
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
        danmaku.medal_color = object.value("medal_color").toString();
        danmaku.medal_up = object.value("medal_up").toString();
        danmaku.no_reply = object.value("no_reply").toBool();
        danmaku.opposite = object.value("opposite").toBool();
        danmaku.to_view = object.value("to_view").toBool();
        danmaku.view_return = object.value("view_return").toBool();
        danmaku.pk_link = object.value("pk_link").toBool();
        danmaku.robot = object.value("robot").toBool();
        danmaku.uidentity = object.value("uidentity").toInt();
        danmaku.iphone = object.value("iphone").toInt();
        danmaku.guard = object.value("guard").toInt();
        danmaku.prev_timestamp = static_cast<qint64>(object.value("prev_timestamp").toDouble());
        danmaku.first = object.value("first").toInt();
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
            object.insert("admin", admin);
            object.insert("vip", vip);
            object.insert("svip", svip);
            object.insert("level", level);
            object.insert("uidentity", uidentity);
            object.insert("iphone", iphone);
            object.insert("guard", guard);
            object.insert("no_reply", no_reply);
        }
        else if (msgType == MSG_GIFT || msgType == MSG_GUARD_BUY)
        {
            object.insert("gift_id", giftId);
            object.insert("gift_name", giftName);
            object.insert("number", number);
            object.insert("coin_type", coin_type);
            object.insert("total_coin", total_coin);

            if (msgType == MSG_GUARD_BUY)
            {
                object.insert("guard", guard);
                object.insert("first", first);
            }
        }
        else if (msgType == MSG_WELCOME)
        {
            object.insert("admin", admin);
            object.insert("uname_color", uname_color);
            object.insert("spread_desc", spread_desc);
            object.insert("spread_info", spread_info);
            object.insert("number", number);
        }
        else if (msgType == MSG_WELCOME_GUARD)
        {
            object.insert("admin", admin);
            object.insert("guard", guard);
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
        else if (msgType == MSG_MSG)
        {
            object.insert("text", text);
        }
        else if (msgType == MSG_PK_BEST)
        {
            object.insert("level", level);
            object.insert("total_coin", total_coin);
        }

        object.insert("timeline", timeline.toString("yyyy-MM-dd hh:mm:ss"));
        object.insert("msgType", (int)msgType);
        if (!anchor_roomid.isEmpty())
        {
            object.insert("anchor_roomid", anchor_roomid);
            object.insert("medal_name", medal_name);
            object.insert("medal_level", medal_level);
            object.insert("medal_color", medal_color);
            object.insert("medal_up", medal_up);
        }
        if (opposite)
            object.insert("opposite", opposite);
        if (to_view)
            object.insert("to_view", to_view);
        if (view_return)
            object.insert("view_return", view_return);
        if (pk_link)
            object.insert("pk_link", pk_link);
        if (robot)
            object.insert("robot", robot);
        if (prev_timestamp)
            object.insert("prev_timestamp", prev_timestamp);

        return object;
    }

    QString toString() const
    {
        if (msgType == MSG_DANMAKU)
        {
            return QString("%1    %2\t%3")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname)
                    .arg(text);
        }
        else if (msgType == MSG_GIFT || msgType == MSG_GUARD_BUY)
        {
            return QString("%1    %2 => %3 × %4")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname)
                    .arg(giftName)
                    .arg(number);
        }
        else if (msgType == MSG_WELCOME)
        {
            if (isAdmin())
                return QString("%1    [光临] 房管 %2")
                        .arg(timeline.toString("hh:mm:ss"))
                        .arg(nickname);
            return QString("%1    [欢迎] %2 进入直播间%3")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname).arg(spread_desc.isEmpty() ? "" : (" "+spread_desc));
        }
        else if (msgType == MSG_WELCOME_GUARD)
        {
            return QString("%1    [光临] 舰长 %2")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname);
        }
        else if (msgType == MSG_WELCOME)
        {
            return QString("%1    [光临] 舰长 %2")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname);
        }
        else if (msgType == MSG_DIANGE)
        {
            return QString("%3    [点歌] %1 (%2)")
                                .arg(text).arg(nickname)
                                .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_FANS)
        {
            return QString("%3    [粉丝] 粉丝数：%1，粉丝团：%2")
                    .arg(fans)
                    .arg(fans_club)
                    .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_ATTENTION)
        {
            return QString("%3    [关注] %1 %2")
                    .arg(nickname)
                    .arg(attention ? "关注了主播" : "取消关注主播")
                    .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_BLOCK)
        {
            return QString("%2    [禁言] %1 被房管禁言")
                    .arg(nickname)
                    .arg(timeline.toString("hh:mm:ss"));
        }
        else if (msgType == MSG_MSG)
        {
            return QString("%1    %2")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(text);
        }
        return "未知消息类型";
    }

    bool is(MessageType type) const
    {
        return this->msgType == type;
    }

    void transToAttention(qint64 attentionTime)
    {
        this->msgType = MSG_ATTENTION;
        prev_timestamp = attentionTime;
        attention = true;
    }

    void transToShare()
    {
        this->msgType = MSG_SHARE;
    }

    void setMedal(QString roomId, QString name, int level, QString color, QString up = "")
    {
        this->anchor_roomid = roomId;
        this->medal_name = name;
        this->medal_level = level;
        this->medal_color = color;
        this->medal_up = up;
    }

    void setUserInfo(int admin, int vip, int svip, int uidentity, int iphone, int guard)
    {
        this->admin = admin;
        this->vip = vip;
        this->svip = svip;
        this->uidentity = uidentity;
        this->iphone = iphone;
        this->guard = guard;
    }

    void setUid(qint64 uid)
    {
        this->uid = uid;
    }

    void setNumber(int num)
    {
        this->number = num;
    }

    void setNoReply()
    {
        this->no_reply = true;
    }

    void setOpposite(bool op)
    {
        this->opposite = op;
    }

    void setToView(bool to)
    {
        this->to_view = to;
    }

    void setViewReturn(bool re)
    {
        this->view_return = re;
    }

    void setPkLink(bool link)
    {
        this->pk_link = link;
    }

    void addGift(int count, int total, QDateTime time)
    {
        this->number += count;
        this->total_coin += total;
        this->timeline = time;
    }

    void setRobot(bool r)
    {
        this->robot = r;
    }

    void setPrevTimestamp(qint64 timestamp)
    {
        this->prev_timestamp = timestamp;
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
        return admin;
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

    bool isVip() const
    {
        return vip;
    }

    bool isSvip() const
    {
        return svip;
    }

    bool isUidentity() const
    {
        return uidentity;
    }

    bool isIphone() const
    {
        return iphone;
    }

    int getGuard() const
    {
        return guard;
    }

    int getGiftId() const
    {
        return giftId;
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

    QString getMedalColor() const
    {
        return medal_color;
    }

    bool isNoReply() const
    {
        return no_reply;
    }

    bool isOpposite() const
    {
        return opposite;
    }

    bool isToView() const
    {
        return to_view;
    }

    bool isViewReturn() const
    {
        return view_return;
    }

    bool isPkLink() const
    {
        return pk_link;
    }

    bool isRobot() const
    {
        return robot;
    }

    qint64 getPrevTimestamp() const
    {
        return prev_timestamp;
    }

    bool isGuard() const
    {
        return guard;
    }

    int getFirst() const
    {
        return first;
    }

private:
    MessageType msgType = MSG_DANMAKU;

    QString text;
    qint64 uid = 0; // 用户ID
    QString nickname;
    QString uname_color; // 没有的话是空的
    QString text_color; // 没有的话是空的
    QDateTime timeline;
    int admin = 0; // 房管
    int guard = 0; // 舰长
    int vip = 0;
    int svip = 0;
    int uidentity = 0; // 正式会员
    int iphone = 0; // 手机实名
    bool no_reply = false;

    QString anchor_roomid;
    int medal_level = 0;
    QString medal_name;
    QString medal_up;
    QString medal_color;

    int level = 0;
    qint64 teamid = 0;
    int rnd = 0;
    QString user_title;
    int bubble = 0;
    QString bubble_color;
    qint64 check_info_ts;
    QString check_info_ct;
    int lpl = 0;

    int giftId = 0;
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
    bool attention = false; // 关注还是取关

    bool opposite = false; // 是否是大乱斗对面的
    bool to_view = false; // 是否是自己这边过去串门的
    bool view_return = false; // 自己这边过去串门回来的
    bool pk_link = false; // 是否是PK连接的

    bool robot = false;
    qint64 prev_timestamp = 0;
    int first = 0; // 初次：1；新的：2
};

#endif // LIVEDANMAKU_H
