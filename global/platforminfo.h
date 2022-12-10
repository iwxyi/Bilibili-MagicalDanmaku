#ifndef PLATFORMINFO_H
#define PLATFORMINFO_H

#include <QHash>
#include <QPixmap>
#include "livedanmaku.h"
#include "emoticon.h"

class PlatformInfo
{
public:
    QMap<int, LiveDanmaku> allGiftMap;
    QHash<qint64, QPixmap> giftPixmaps; // 礼物图片（因为数量不多，直接用即可）
    QHash<qint64, QPixmap> userHeaders; // 用户头像
    QMap<QString, Emoticon> emoticons; // 表情包弹幕
};

extern PlatformInfo* pl;

#endif // PLATFORMINFO_H
