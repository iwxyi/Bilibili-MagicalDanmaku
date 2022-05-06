#ifndef PLATFORMINFO_H
#define PLATFORMINFO_H

#include <QHash>
#include <QPixmap>

class PlatformInfo
{
public:
    QHash<qint64, QPixmap> giftImages; // 礼物图片（因为数量不多，直接用即可）
};

extern PlatformInfo* pl;

#endif // PLATFORMINFO_H
