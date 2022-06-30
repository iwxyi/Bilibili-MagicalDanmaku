#ifndef BILIBILIAPIUTIL_H
#define BILIBILIAPIUTIL_H

#include <QByteArray>

enum Operation
{
    OP_HANDSHAKE = 0,
    OP_HANDSHAKE_REPLY = 1,
    OP_HEARTBEAT = 2, // 心跳包
    OP_HEARTBEAT_REPLY = 3, // 心跳包回复（人气值）
    OP_SEND_MSG = 4,
    OP_SEND_MSG_REPLY = 5, // 普通包（命令）
    OP_DISCONNECT_REPLY = 6,
    OP_AUTH = 7, // 认证包
    OP_AUTH_REPLY = 8, // 认证包（回复）
    OP_RAW = 9,
    OP_PROTO_READY = 10,
    OP_PROTO_FINISH = 11,
    OP_CHANGE_ROOM = 12,
    OP_CHANGE_ROOM_REPLY = 13,
    OP_REGISTER = 14,
    OP_REGISTER_REPLY = 15,
    OP_UNREGISTER = 16,
    OP_UNREGISTER_REPLY = 17
};

class BiliApiUtil
{
public:
    /**
     * 给body加上头部信息
     * 偏移量	长度	类型	含义
     * 0	4	uint32	封包总大小（头部大小+正文大小）
     * 4	2	uint16	头部大小（一般为0x0010，16字节）
     * 6	2	uint16	协议版本:0普通包正文不使用压缩，1心跳及认证包正文不使用压缩，2普通包正文使用zlib压缩
     * 8	4	uint32	操作码（封包类型）
     * 12	4	uint32	sequence，可以取常数1
     */
    static QByteArray makePack(QByteArray body, qint32 operation)
    {
        // 因为是大端，所以要一个个复制
        qint32 totalSize = 16 + body.size();
        short headerSize = 16;
        short protover = 1;
        qint32 seqId = 1;

        auto byte4 = [=](qint32 i) -> QByteArray{
            QByteArray ba(4, 0);
            ba[3] = (uchar)(0x000000ff & i);
            ba[2] = (uchar)((0x0000ff00 & i) >> 8);
            ba[1] = (uchar)((0x00ff0000 & i) >> 16);
            ba[0] = (uchar)((0xff000000 & i) >> 24);
            return ba;
        };

        auto byte2 = [=](short i) -> QByteArray{
            QByteArray ba(2, 0);
            ba[1] = (uchar)(0x00ff & i);
            ba[0] = (uchar)((0xff00 & i) >> 8);
            return ba;
        };

        QByteArray header;
        header += byte4(totalSize);
        header += byte2(headerSize);
        header += byte2(protover);
        header += byte4(operation);
        header += byte4(seqId);

        return header + body;


        /* // 小端算法，直接上结构体
        int totalSize = 16 + body.size();
        short headerSize = 16;
        short protover = 1;
        int seqId = 1;

        HeaderStruct header{totalSize, headerSize, protover, operation, seqId};

        QByteArray ba((char*)&header, sizeof(header));
        return ba + body;*/
    }
};

#endif // BILIBILIAPIUTIL_H
