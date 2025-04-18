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
    Q_INVOKABLE std::string getText() const { return danmaku.getText().toStdString(); }
    Q_INVOKABLE long long getUid() const { return danmaku.getUid(); }
    Q_INVOKABLE std::string getNickname() const { return danmaku.getNickname().toStdString(); }
    Q_INVOKABLE std::string getUname() const { return getNickname(); }
    Q_INVOKABLE std::string getUnameColor() const { return danmaku.getUnameColor().toStdString(); }
    Q_INVOKABLE std::string getTextColor() const { return danmaku.getTextColor().toStdString(); }
    Q_INVOKABLE QDateTime getTimeline() const { return danmaku.getTimeline(); }
    Q_INVOKABLE int isAdmin() const { return danmaku.isAdmin(); }
    Q_INVOKABLE int getGuard() const { return danmaku.getGuard(); }
    Q_INVOKABLE int isVip() const { return danmaku.isVip(); }
    Q_INVOKABLE int isSvip() const { return danmaku.isSvip(); }
    Q_INVOKABLE int isUidentity() const { return danmaku.isUidentity(); }
    Q_INVOKABLE int getLevel() const { return danmaku.getLevel(); }
    Q_INVOKABLE int getWealthLevel() const { return danmaku.getWealthLevel(); }
    Q_INVOKABLE int getGiftId() const { return danmaku.getGiftId(); }
    Q_INVOKABLE std::string getGiftName() const { return danmaku.getGiftName().toStdString(); }
    Q_INVOKABLE int getNumber() const { return danmaku.getNumber(); }
    Q_INVOKABLE long long getTotalCoin() const { return danmaku.getTotalCoin(); }
    Q_INVOKABLE bool isFirst() const { return danmaku.isFirst(); }
    Q_INVOKABLE long long getReplyMid() const { return danmaku.getReplyMid(); }
    Q_INVOKABLE std::string getReplyUname() const { return danmaku.getReplyUname().toStdString(); }
    Q_INVOKABLE std::string getReplyUnameColor() const { return danmaku.getReplyUnameColor().toStdString(); }
    Q_INVOKABLE bool isReplyMystery() const { return danmaku.isReplyMystery(); }
    Q_INVOKABLE int getReplyTypeEnum() const { return danmaku.getReplyTypeEnum(); }
    Q_INVOKABLE std::string getAIReply() const { return danmaku.getAIReply().toStdString(); }
    Q_INVOKABLE std::string getFaceUrl() const { return danmaku.getFaceUrl().toStdString()  ; }

    // 重载运算符，注册Lua对象要用到
    bool operator<(const DanmakuWrapper& other) const {
        return danmaku.getTimeline() < other.danmaku.getTimeline();
    }

    bool operator==(const DanmakuWrapper& other) const {
        return danmaku.getTimeline() == other.danmaku.getTimeline();
    }

    bool operator<=(const DanmakuWrapper& other) const {
        return danmaku.getTimeline() <= other.danmaku.getTimeline();
    }

private:
    LiveDanmaku danmaku;
};

#endif // DANMAKUWRAPPER_H
