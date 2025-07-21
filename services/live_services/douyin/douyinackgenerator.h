#ifndef DOUYINACKGENERATOR_H
#define DOUYINACKGENERATOR_H

#include <QWebSocket>
#include <QByteArray>
#include <QString>
#include <QDataStream>

class DouyinAckGenerator
{
private:
    // 写 varint32
    void writeVarint32(QByteArray &buf, quint32 value) {
        while (value > 127) {
            buf.append(static_cast<char>((value & 0x7F) | 0x80));
            value >>= 7;
        }
        buf.append(static_cast<char>(value));
    }

    // 写字符串（先写长度，再写内容）
    void writeString(QByteArray &buf, const QString &str) {
        QByteArray utf8 = str.toUtf8();
        writeVarint32(buf, utf8.size());
        buf.append(utf8);
    }

    // 写字节数组（先写长度，再写内容）
    void writeBytes(QByteArray &buf, const QByteArray &bytes) {
        writeVarint32(buf, bytes.size());
        buf.append(bytes);
    }

public:
    // 组装 ACK 帧
    QByteArray encodeAckPushFrame(const QString &internalExt, qint64 logId)
    {
        QByteArray buf;

        // payloadType = 7, tag = 58
        writeVarint32(buf, 58);
        writeString(buf, "ack");

        // payload = 8, tag = 66
        writeVarint32(buf, 66);
        QByteArray payload = internalExt.toUtf8();
        writeBytes(buf, payload);

        // logId = 2, tag = 16
        if (logId != 0) {
            writeVarint32(buf, 16);
            // 写 varint64
            quint64 value = logId;
            while (value > 127) {
                buf.append(static_cast<char>((value & 0x7F) | 0x80));
                value >>= 7;
            }
            buf.append(static_cast<char>(value));
        }

        return buf;
    }

    // 发送ACK
    void sendAck(QWebSocket *ws, const QString &internalExt, qint64 logId)
    {
        QByteArray frame = encodeAckPushFrame(internalExt, logId);
        // qDebug() << "返回ack：" << frame;
        ws->sendBinaryMessage(frame);
    }
};

#endif // DOUYINACKGENERATOR_H
