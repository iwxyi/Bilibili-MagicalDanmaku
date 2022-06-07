#ifndef PLATFORMINFO_H
#define PLATFORMINFO_H

#include <QHash>
#include <QPixmap>

class PlatformInfo
{
public:
    QHash<qint64, QPixmap> giftPixmaps; // 礼物图片（因为数量不多，直接用即可）
    QHash<qint64, QPixmap> userHeaders; // 用户头像
};

extern PlatformInfo* pl;

#endif // PLATFORMINFO_H
