#ifndef ENTITIES_H
#define ENTITIES_H

#include "livedanmaku.h"

struct HostInfo
{
    QString host;
    int port;
    int wss_port;
    int ws_port;
};

struct HeaderStruct
{
    int totalSize;
    short headerSize;
    short ver;
    int operation;
    int seqId;
};

struct FanBean
{
    qint64 mid;
    QString uname;
    int attribute; // 0：未关注,2：已关注,6：已互粉
    qint64 mtime;
};

struct GiftCombo
{
    qint64 uid;
    QString uname;
    qint64 giftId;
    QString giftName;
    int count;          // 数量
    qint64 total_coins; // 金瓜子数量

    GiftCombo(qint64 uid, QString uname, qint64 giftId, QString giftName, int count, int coins)
        : uid(uid), uname(uname), giftId(giftId), giftName(giftName), count(count), total_coins(coins)
    {}

    void merge(LiveDanmaku danmaku)
    {
        if (this->giftId != danmaku.getGiftId() || this->uid != danmaku.getUid())
        {
            qWarning() << "合并礼物数据错误：" << toString() << danmaku.toString();
        }
        this->count += danmaku.getNumber();
        this->total_coins += danmaku.getTotalCoin();
    }

    QString toString() const
    {
        return QString("%1(%2):%3(%4)x%5(%6)")
                .arg(uname).arg(uname)
                .arg(giftName).arg(giftId)
                .arg(count).arg(total_coins);
    }
};

#endif // ENTITIES_H
