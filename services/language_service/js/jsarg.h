#ifndef JSARG_H
#define JSARG_H

#include <QObject>
#include "livedanmaku.h"

class JSArg : public QObject, public LiveDanmaku
{
    Q_OBJECT
public:
    JSArg(const LiveDanmaku &danmaku, QObject *parent = nullptr)
        : QObject(parent), LiveDanmaku(danmaku)
    {}

    // 获取danmaku的属性
    Q_INVOKABLE QJsonObject toJson()  const { return LiveDanmaku::toJson(); }
    Q_INVOKABLE int getMsgType() const { return msgType; }
    Q_INVOKABLE QString getText() const { return text; }
    Q_INVOKABLE qint64 getUid() const { return uid; }
    Q_INVOKABLE QString getNickname() const { return nickname; }
    Q_INVOKABLE QString getUname() const { return getNickname(); }
    Q_INVOKABLE QString getUnameColor() const { return uname_color; }
    Q_INVOKABLE QString getTextColor() const { return text_color; }
    Q_INVOKABLE QDateTime getTimeline() const { return timeline; }
    Q_INVOKABLE int getAdmin() const { return admin; }
    Q_INVOKABLE int getGuard() const { return guard; }
    Q_INVOKABLE int getVip() const { return vip; }
    Q_INVOKABLE int getSvip() const { return svip; }
    Q_INVOKABLE int getUidentity() const { return uidentity; }
    Q_INVOKABLE int getLevel() const { return level; }
    Q_INVOKABLE int getWealthLevel() const { return wealth_level; }
    Q_INVOKABLE int getGiftId() const { return giftId; }
    Q_INVOKABLE QString getGiftName() const { return giftName; }
    Q_INVOKABLE int getNumber() const { return number; }
    Q_INVOKABLE qint64 getTotalCoin() const { return total_coin; }
    Q_INVOKABLE bool isFirst() const { return first; }
    Q_INVOKABLE bool isReplyMystery() const { return reply_is_mystery; }
    Q_INVOKABLE int getReplyTypeEnum() const { return reply_type_enum; }
    Q_INVOKABLE QString getAIReply() const { return ai_reply; }
    Q_INVOKABLE QString getFaceUrl() const { return faceUrl; }
    Q_INVOKABLE qint64 getReplyMid() const { return reply_mid; }
    Q_INVOKABLE QString getReplyUname() const { return reply_uname; }
    Q_INVOKABLE QString getReplyUnameColor() const { return reply_uname_color; }
};

#endif // JSARG_H
