#ifndef FANSDANMAKUWAITBEAN_H
#define FANSDANMAKUWAITBEAN_H

#include <QString>

/**
 * 粉丝弹幕处理的队列bean
 * 不过现在好像不太用得到
 */
struct FansDanmakuWaitBean
{
    QString roomId; // 因为不同房间，粉丝可能给自己立了不同的人设，所以分开计算（出门在外人设是自己给的）
    QString uid; // 粉丝的uid
    QString uname; // 粉丝的昵称

    int unprocessCount = 0; // 没有处理的弹幕数量
    qint64 lastProcessTime = 0; // 上次处理时间
    qint64 lastSendTime = 0; // 最后发送的时间。多久没发送再进行更新
};

#endif // FANSDANMAKUWAITBEAN_H
