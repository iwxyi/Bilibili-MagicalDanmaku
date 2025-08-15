#ifndef DANMAKUWRAPPER_H
#define DANMAKUWRAPPER_H

#include <QObject>
#include "livedanmaku.h"

class DanmakuWrapper : public QObject
{
    Q_OBJECT
public:
    DanmakuWrapper(const LiveDanmaku &danmaku, QObject *parent = nullptr)
        : QObject(parent), danmaku(danmaku)
    {}

    DanmakuWrapper(const DanmakuWrapper& other)
        : QObject(other.parent()), danmaku(other.danmaku)
    {}

    // 获取danmaku的属性
    Q_INVOKABLE QJsonObject toJson()  const { return danmaku.toJson(); }
    Q_INVOKABLE int getMsgType() const { return danmaku.getMsgType(); }
    Q_INVOKABLE QString getText() const { return danmaku.getText(); }
    Q_INVOKABLE UIDT getUid() const { return danmaku.getUid(); }
    Q_INVOKABLE QString getNickname() const { return danmaku.getNickname(); }
    Q_INVOKABLE QString getUname() const { return getNickname(); }
    Q_INVOKABLE QString getUnameColor() const { return danmaku.getUnameColor(); }
    Q_INVOKABLE QString getTextColor() const { return danmaku.getTextColor(); }
    Q_INVOKABLE QDateTime getTimeline() const { return danmaku.getTimeline(); }
    Q_INVOKABLE int isAdmin() const { return danmaku.isAdmin(); }
    Q_INVOKABLE int getGuard() const { return danmaku.getGuard(); }
    Q_INVOKABLE int isVip() const { return danmaku.isVip(); }
    Q_INVOKABLE int isSvip() const { return danmaku.isSvip(); }
    Q_INVOKABLE int isUidentity() const { return danmaku.isUidentity(); }
    Q_INVOKABLE int getLevel() const { return danmaku.getLevel(); }
    Q_INVOKABLE int getWealthLevel() const { return danmaku.getWealthLevel(); }
    Q_INVOKABLE int getGiftId() const { return danmaku.getGiftId(); }
    Q_INVOKABLE QString getGiftName() const { return danmaku.getGiftName(); }
    Q_INVOKABLE int getNumber() const { return danmaku.getNumber(); }
    Q_INVOKABLE long long getTotalCoin() const { return danmaku.getTotalCoin(); }
    Q_INVOKABLE bool isFirst() const { return danmaku.isFirst(); }
    Q_INVOKABLE long long getReplyMid() const { return danmaku.getReplyMid(); }
    Q_INVOKABLE QString getReplyUname() const { return danmaku.getReplyUname(); }
    Q_INVOKABLE QString getReplyUnameColor() const { return danmaku.getReplyUnameColor(); }
    Q_INVOKABLE bool isReplyMystery() const { return danmaku.isReplyMystery(); }
    Q_INVOKABLE int getReplyTypeEnum() const { return danmaku.getReplyTypeEnum(); }
    Q_INVOKABLE QString getAIReply() const { return danmaku.getAIReply(); }
    Q_INVOKABLE QString getFaceUrl() const { return danmaku.getAvatar()  ; }

private:
    LiveDanmaku danmaku;
};

#endif // DANMAKUWRAPPER_H
