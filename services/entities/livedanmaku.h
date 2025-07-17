#ifndef LIVEDANMAKU_H
#define LIVEDANMAKU_H

#include <QString>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QDebug>

typedef QString UIDT;

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
    MSG_PK_BEST,
    MSG_SUPER_CHAT,
    MSG_EXTRA
};

class LiveDanmaku
{
public:
    explicit LiveDanmaku() : msgType(MSG_DEF)
    {}

    LiveDanmaku(qint64 uid, QString nickname, QString text)
        : LiveDanmaku(QString::number(uid), nickname, text)
    {}

    LiveDanmaku(UIDT uid, QString nickname, QString text) : msgType(MSG_MSG), uid(uid), nickname(nickname), text(text)
    {}

    LiveDanmaku(QString nickname, QString text, qint64 uid, int level, QDateTime time, QString unameColor, QString textColor)
        : LiveDanmaku(nickname, text, QString::number(uid), level, time, unameColor, textColor)
    {}

    LiveDanmaku(QString nickname, QString text, UIDT uid, int level, QDateTime time, QString unameColor, QString textColor)
        : msgType(MSG_DANMAKU), nickname(nickname), text(text), uid(uid), level(level), timeline(time),
          uname_color(unameColor), text_color(textColor)
    {

    }

    LiveDanmaku(QString nickname, int giftId, QString gift, int num, qint64 uid, QDateTime time, QString coinType, qint64 totalCoin)
        : LiveDanmaku(nickname, giftId, gift, num, QString::number(uid), time, coinType, totalCoin)
    {}

    LiveDanmaku(QString nickname, int giftId, QString gift, int num, UIDT uid, QDateTime time, QString coinType, qint64 totalCoin)
        : msgType(MSG_GIFT), nickname(nickname), giftId(giftId), giftName(gift), number(num), uid(uid), timeline(time),
          coin_type(coinType), total_coin(totalCoin)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QDateTime time, bool admin, QString unameColor, QString spreadDesc, QString spreadInfo)
        : LiveDanmaku(nickname, QString::number(uid), time, admin, unameColor, spreadDesc, spreadInfo)
    {}

    LiveDanmaku(QString nickname, UIDT uid, QDateTime time, bool admin, QString unameColor, QString spreadDesc, QString spreadInfo)
        : msgType(MSG_WELCOME), nickname(nickname), uid(uid), timeline(time), admin(admin),
          uname_color(unameColor), spread_desc(spreadDesc), spread_info(spreadInfo)
    {

    }

    LiveDanmaku(int guard, QString nickname, qint64 uid, QDateTime time)
        : LiveDanmaku(guard, nickname, QString::number(uid), time)
    {}

    LiveDanmaku(int guard, QString nickname, UIDT uid, QDateTime time)
        : msgType(MSG_WELCOME_GUARD), guard(guard), nickname(nickname), uid(uid), timeline(time)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QString song, QDateTime time)
        : LiveDanmaku(nickname, QString::number(uid), song, time)
    {}

    LiveDanmaku(QString nickname, UIDT uid, QString song, QDateTime time)
        : msgType(MSG_DIANGE), nickname(nickname), uid(uid), text(song), timeline(time)
    {

    }

    LiveDanmaku(QString nickname, qint64 uid, QString gift, int num, int guard, int gift_id, qint64 price, int first)
        : LiveDanmaku(nickname, QString::number(uid), gift, num, guard, gift_id, price, first)
    {}

    LiveDanmaku(QString nickname, UIDT uid, QString gift, int num, int guard, int gift_id, qint64 price, int first)
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
    LiveDanmaku(QString nickname, UIDT uid, bool attention, QDateTime time)
        : msgType(MSG_ATTENTION), nickname(nickname), uid(uid), prev_timestamp(time.toSecsSinceEpoch()),
        timeline(QDateTime::currentDateTime()), attention(attention)
    {

    }

    LiveDanmaku(QString nickname, UIDT uid)
        : msgType(MSG_BLOCK), nickname(nickname), uid(uid), timeline(QDateTime::currentDateTime())
    {

    }

    LiveDanmaku(QString msg)
        : msgType(MSG_MSG), text(msg), timeline(QDateTime::currentDateTime())
    {

    }

    LiveDanmaku(QString uname, UIDT uid, int win, qint64 votes)
        : msgType(MSG_PK_BEST), nickname(uname), uid(uid), level(win), total_coin(votes)
    {
    }

    LiveDanmaku(QString nickname, QString text, UIDT uid, int level, QDateTime time, QString unameColor, QString textColor,
                int giftId, QString gift, int num, int price)
        : msgType(MSG_SUPER_CHAT), nickname(nickname), text(text), uid(uid), level(level), timeline(time),
          uname_color(unameColor), text_color(textColor), giftId(giftId), giftName(gift), number(num),
          coin_type("gold"), total_coin(price * 1000)
    {

    }

    LiveDanmaku(QJsonObject json)
        : msgType(MSG_EXTRA), extraJson(json)
    {
    }

    static LiveDanmaku fromDanmakuJson(QJsonObject object)
    {
        LiveDanmaku danmaku;
        danmaku.text = object.value("text").toString();
        if (object.value("uid").isString())
            danmaku.uid = object.value("uid").toString();
        else
            danmaku.uid = QString::number(static_cast<qint64>(object.value("uid").toDouble()));
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
        QJsonArray medal = object.value("medal").toArray(); // 会被后面覆盖
        if (medal.size() >= 3)
        {
            danmaku.medal_level = medal[0].toInt();
            danmaku.medal_up = medal[1].toString();
            danmaku.medal_name = medal[2].toString();
            if (medal.size() >= 4)
            {
                danmaku.medal_uid = medal[3].toString();
            }
        }
        danmaku.giftId = object.value("gift_id").toInt();
        danmaku.giftName = object.value("gift_name").toString();
        danmaku.number = object.value("number").toInt();
        danmaku.coin_type = object.value("coin_type").toString();
        danmaku.total_coin = qint64(object.value("total_coin").toDouble());
        danmaku.discount_price = qint64(object.value("discount_price").toDouble());
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
        danmaku.medal_uid = object.value("medal_uid").toString();
        danmaku.no_reply = object.value("no_reply").toBool();
        danmaku.opposite = object.value("opposite").toBool();
        danmaku.to_view = object.value("to_view").toBool();
        danmaku.view_return = object.value("view_return").toBool();
        danmaku.pk_link = object.value("pk_link").toBool();
        danmaku.robot = object.value("robot").toBool();
        danmaku.uidentity = object.value("uidentity").toInt();
        danmaku.iphone = object.value("iphone").toInt();
        danmaku.guard = object.value("guard_level").toInt();
        if (danmaku.isGuard())
            danmaku.guard_expired = QDateTime::fromString("yyyy-MM-dd HH:mm:ss", object.value("guard_expired").toString());
        danmaku.prev_timestamp = static_cast<qint64>(object.value("prev_timestamp").toDouble());
        danmaku.first = object.value("first").toInt();
        danmaku.special = object.value("special").toInt();
        danmaku.extraJson = object.value("extra").toObject();
        danmaku.faceUrl = object.value("face_url").toString();
        danmaku.show_reply = object.value("show_reply").toBool();
        danmaku.reply_mid = static_cast<qint64>(object.value("reply_mid").toDouble());
        danmaku.reply_uname = object.value("reply_uname").toString();
        danmaku.reply_uname_color = object.value("reply_uname_color").toString();
        danmaku.reply_is_mystery = object.value("reply_is_mystery").toBool();
        danmaku.reply_type_enum = object.value("reply_type_enum").toInt();
        danmaku.wealth_level = object.value("honor_level").toInt();
        danmaku.fromRoomId = object.value("from_room_id").toString();
        return danmaku;
    }

    QJsonObject toJson() const
    {
        QJsonObject object;
        object.insert("nickname", nickname);
        object.insert("uid", uid);
        if (msgType == MSG_DANMAKU || msgType == MSG_SUPER_CHAT)
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
            object.insert("guard_level", guard);
            object.insert("no_reply", no_reply);
        }
        if (msgType == MSG_GIFT || msgType == MSG_GUARD_BUY || msgType == MSG_SUPER_CHAT)
        {
            object.insert("gift_id", giftId);
            object.insert("gift_name", giftName);
            object.insert("number", number);
            object.insert("coin_type", coin_type);
            object.insert("total_coin", total_coin);

            if (msgType == MSG_GUARD_BUY)
            {
                object.insert("guard_level", guard);
                object.insert("first", first);
            }
            if (discount_price != 0)
            {
                object.insert("discount_price", discount_price);
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
            object.insert("guard_level", guard);
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
            if (special)
                object.insert("special", special);
        }
        else if (msgType == MSG_MSG)
        {
            object.insert("text", text);
        }
        else if (msgType == MSG_PK_BEST)
        {
            object.insert("level", level);
            object.insert("total_coin", total_coin);
            object.insert("discount_price", discount_price);
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
            if (!medal_uid.isEmpty())
                object.insert("medal_uid", medal_uid);
        }
        if (guard > 0)
        {
            object.insert("guard_level", guard);
            if (guard_expired.isValid())
                object.insert("guard_expired", guard_expired.toString("yyyy-MM-dd HH:mm:ss"));
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
        if (!extraJson.isEmpty())
            object.insert("extra", extraJson);
        if (!faceUrl.isEmpty())
            object.insert("face_url", faceUrl);
        if (show_reply && reply_mid > 0)
        {
            object.insert("show_reply", show_reply);
            object.insert("reply_mid", reply_mid);
            object.insert("reply_uname", reply_uname);
            object.insert("reply_uname_color", reply_uname_color);
            object.insert("reply_is_mystery", reply_is_mystery);
            object.insert("reply_type_enum", reply_type_enum);
        }
        if (wealth_level > 0)
        {
            object.insert("honor_level", wealth_level);
        }
        if (!fromRoomId.isEmpty())
        {
            object.insert("from_room_id", fromRoomId);
        }
        return object;
    }

    static QJsonArray toJsonArray(const QList<LiveDanmaku> danmakus)
    {
        QJsonArray array;
        for (int i = 0; i < danmakus.size(); i++)
        {
            array.append(danmakus.at(i).toJson());
        }
        return array;
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
        else if (msgType == MSG_SUPER_CHAT)
        {
            return QString("%1    [醒目留言]%2\t%3")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname)
                    .arg(text);
        }
        else if (msgType == MSG_WELCOME)
        {
            if (isAdmin())
                return QString("%1    [光临] 房管 %2")
                        .arg(timeline.toString("hh:mm:ss"))
                        .arg(nickname);
            return QString("%1    [欢迎] %2%3")
                    .arg(timeline.toString("hh:mm:ss"))
                    .arg(nickname).arg(spread_desc.isEmpty() ? "" : (" "+spread_desc));
        }
        else if (msgType == MSG_WELCOME_GUARD)
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
            if (special)
                return QString("%2    [特别关注] %1 特别关注了主播")
                        .arg(nickname)
                        .arg(timeline.toString("hh:mm:ss"));
            else
                return QString("%3    [关注] %1 %2")
                        .arg(nickname)
                        .arg(special)
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

    void transToDanmu()
    {
        this->msgType = MSG_DANMAKU;
    }

    void transToShare()
    {
        this->msgType = MSG_SHARE;
    }

    void setMsgType(MessageType type)
    {
        this->msgType = type;
    }

    void setNickname(const QString& name)
    {
        this->nickname = name;
    }

    void setUser(QString name, UIDT uid, QString face, QString unameColor = "", QString textColor = "")
    {
        this->nickname = name;
        this->uid = uid;
        this->faceUrl = face;
        this->uname_color = unameColor;
        this->text_color = textColor;
    }

    void setMedal(QString roomId, QString name, int level, QString color, QString up = "", QString uid = "")
    {
        this->anchor_roomid = roomId;
        this->medal_name = name;
        this->medal_level = level;
        this->medal_color = color;
        this->medal_up = up;
        this->medal_uid = uid;
    }

    void setGuardLevel(int level, QString exp = "")
    {
        this->guard = level;
        if (exp.isEmpty())
            this->guard_expired = QDateTime();
        else
            this->guard_expired = QDateTime::fromString("yyyy-MM-dd HH:mm:ss", exp);
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
        this->uid = QString::number(uid);
    }

    void setUid(UIDT uid)
    {
        this->uid = uid;
    }

    void setText(QString s)
    {
        this->text = s;
    }

    void setNumber(int num)
    {
        this->number = num;
    }

    void setNoReply()
    {
        this->no_reply = true;
    }

    void setAutoSend()
    {
        this->auto_send = true;
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

    void addGift(int count, qint64 total, qint64 discountPrice, QDateTime time)
    {
        this->number += count;
        this->total_coin += total;
        this->discount_price += discountPrice;
        this->timeline = time;
    }

    void setTotalCoin(qint64 coin)
    {
        this->total_coin = coin;
    }

    void setDiscountPrice(qint64 discountPrice)
    {
        this->discount_price = discountPrice;
    }

    void setTime(QDateTime time)
    {
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

    void setFirst(int first)
    {
        this->first = first;
    }

    void setAdmin(bool admin)
    {
        this->admin = admin;
    }

    void setLevel(int level)
    {
        this->level = level;
    }

    void setArgs(QStringList args)
    {
        this->args = args;
    }

    void setMedalLevel(int level)
    {
        this->medal_level = level;
    }

    void setSpecial(int s)
    {
        this->special = s;
    }

    bool equal(const LiveDanmaku& another) const
    {
        return this->uid == another.uid
                && this->text == another.text;
    }

    void setFaceUrl(const QString& url)
    {
        this->faceUrl = url;
    }

    void setAIReply(const QString& reply)
    {
        this->ai_reply = reply;
    }

    void setReplyInfo(bool show, qint64 mid, QString uname, QString unameColor, bool mystery, int type)
    {
        this->show_reply = show;
        this->reply_mid = mid;
        this->reply_uname = uname;
        this->reply_uname_color = unameColor;
        this->reply_is_mystery = mystery;
        this->reply_type_enum = type;
    }

    void setWealthLevel(int level)
    {
        this->wealth_level = level;
    }

    void setSubAccount(QString sub)
    {
        this->sub_account = sub;
    }

    void setOriginalGiftName(QString name)
    {
        this->original_gift_name = name;
    }

    QString getText() const
    {
        return text;
    }

    UIDT getUid() const
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

    QString getGuardName() const
    {
        if (guard == 1)
            return "总督";
        else if (guard == 2)
            return "提督";
        else if (guard == 3)
            return "舰长";
        return "";
    }

    QDateTime getGuardExpiredTime() const
    {
        return guard_expired;
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

    QString getCoinType() const
    {
        return coin_type;
    }

    bool isGoldCoin() const
    {
        return coin_type == "gold";
    }

    bool isSilverCoin() const
    {
        return coin_type == "silver";
    }

    bool isGiftFree() const
    {
        return coin_type != "gold";
    }

    bool isGiftMerged() const
    {
        return !first;
    }

    bool isFirst() const
    {
        return first;
    }

    qint64 getTotalCoin() const
    {
        return total_coin;
    }

    qint64 getDiscountPrice() const
    {
        return discount_price;
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

    QString getMedalUid() const
    {
        return medal_uid;
    }

    QString getMedalColor() const
    {
        return medal_color;
    }

    bool isNoReply() const
    {
        return no_reply;
    }

    bool isAutoSend() const
    {
        return auto_send;
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
        return guard > 0;
    }

    int getFirst() const
    {
        return first;
    }

    int getSpecial() const
    {
        return special;
    }

    QString getArgs(int i) const
    {
        if (i < 0 || i >= args.size())
            return "";
        return args.at(i);
    }

    LiveDanmaku& with(QJsonObject json)
    {
        this->extraJson = json;
        return *this;
    }

    LiveDanmaku& setFromRoomId(QString roomId)
    {
        this->fromRoomId = roomId;
        return *this;
    }

    QString getRoomId() const
    {
        return fromRoomId;
    }

    LiveDanmaku& withRetry(int r = 1)
    {
        retry = r;
        return *this;
    }

    int isRetry() const
    {
        return retry > 0;
    }

    QString getFaceUrl() const
    {
        return faceUrl;
    }

    QString getAIReply() const
    {
        return ai_reply;
    }

    bool isShowReply() const
    {
        return show_reply;
    }

    qint64 getReplyMid() const
    {
        return reply_mid;
    }

    QString getReplyUname() const
    {
        return reply_uname;
    }

    QString getReplyUnameColor() const
    {
        return reply_uname_color;
    }

    bool isReplyMystery() const
    {
        return reply_is_mystery;
    }

    int getReplyTypeEnum() const
    {
        return reply_type_enum;
    }

    int getWealthLevel() const
    {
        return wealth_level;
    }

    QString getSubAccount() const
    {
        return sub_account;
    }

    QString getOriginalGiftName() const
    {
        return original_gift_name;
    }

    QString getFromRoomId() const
    {
        return fromRoomId;
    }

protected:
    MessageType msgType = MSG_DANMAKU;

    QString text;
    UIDT uid = 0; // 用户ID
    QString nickname;
    QString uname_color; // 没有的话是空的
    QString text_color; // 没有的话是空的
    QDateTime timeline;
    int admin = 0; // 房管
    int guard = 0; // 舰长
    QDateTime guard_expired;
    int vip = 0;
    int svip = 0;
    int uidentity = 0; // 正式会员
    int iphone = 0; // 手机实名
    bool no_reply = false; // 不需要处理的弹幕
    bool auto_send = false; // 自己发送的弹幕，同样也不需要处理
    QString faceUrl;

    QString anchor_roomid;
    int medal_level = 0;
    QString medal_name;
    QString medal_up;
    QString medal_uid;
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
    int wealth_level = 0;

    int giftId = 0;
    QString giftName;
    int number = 0;
    QString coin_type;
    qint64 total_coin = 0;
    qint64 discount_price = 0;
    QString original_gift_name;

    QString spread_desc; // 星光推广
    QString spread_info; // 颜色

    int fans = 0;
    int fans_club = 0;
    int delta_fans = 0;
    int delta_fans_club = 0;
    bool attention = false; // 关注还是取关

    bool opposite = false; // 是否是大乱斗对面的过来
    bool to_view = false; // 是否是自己这边过去串门的
    bool view_return = false; // 自己这边过去串门回来的
    bool pk_link = false; // 是否是PK连接的

    bool robot = false;
    qint64 prev_timestamp = 0;
    int first = 0; // 初次：1；新的：2；礼物合并：>0
    int special = 0;

    QString fromRoomId;
    int retry = 0;
    QString ai_reply;

    bool show_reply = false;
    qint64 reply_mid = 0;
    QString reply_uname;
    QString reply_uname_color;
    bool reply_is_mystery = false;
    int reply_type_enum = 0;

    QString sub_account;

public:
    QStringList args;
    QJsonObject extraJson;
};

#endif // LIVEDANMAKU_H
