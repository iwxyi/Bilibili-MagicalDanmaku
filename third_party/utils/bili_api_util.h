#ifndef BILIBILIAPIUTIL_H
#define BILIBILIAPIUTIL_H

#include <QByteArray>
#include <zlib.h>
#include <QDebug>
#include "brotli/decode.h"

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

    /**
     * 博客来源：https://blog.csdn.net/doujianyoutiao/article/details/106236207
     */
    static QByteArray zlibToQtUncompr(const char *pZLIBData, uLongf dataLen/*, uLongf srcDataLen = 0x100000*/)
    {
        char *pQtData = new char[dataLen + 4];
        char *pByte = (char *)(&dataLen);/*(char *)(&srcDataLen);*/
        pQtData[3] = *pByte;
        pQtData[2] = *(pByte + 1);
        pQtData[1] = *(pByte + 2);
        pQtData[0] = *(pByte + 3);
        memcpy(pQtData + 4, pZLIBData, dataLen);
        QByteArray qByteArray(pQtData, dataLen + 4);
        delete[] pQtData;
        return qUncompress(qByteArray);
    }

    /**
     * （用不了）只解压出8字节
     */
    static QByteArray brotliDecode(const QByteArray& ba)
    {
        QByteArray output;

        // Create Brotli decoder state
        BrotliDecoderState* state = BrotliDecoderCreateInstance(NULL, NULL, NULL);
        if (!state)
        {
            qCritical() << "Failed to create Brotli decoder state";
            return output;
        }

        // Prepare input buffer
        const uint8_t* inputBuffer = reinterpret_cast<const uint8_t*>(ba.constData());
        size_t availableInput = ba.size();
        size_t totalInput = 0;

        // Prepare output buffer
        uint8_t* outputBuffer = new uint8_t[4096];
        size_t availableOutput = sizeof(outputBuffer); // 这是下一步的位置，每次遍历后会自动修改，直至0
        size_t totalOutput = 0;

        // Decompress input data
        BrotliDecoderResult result;
        int index = 0;
        do
        {
            if (availableInput == 0)
            {
                break;
            }
            result = BrotliDecoderDecompressStream(state, &availableInput, &inputBuffer, &availableOutput, &outputBuffer, &totalOutput);
            qDebug() << "Brolit解压" << index++ << ":" << availableInput << "/" << ba.size();

            if (result == BROTLI_DECODER_RESULT_ERROR)
            {
                qCritical() << "Brotli decoding error";
                break;
            }

            // Append decompressed data to output
            output.append(reinterpret_cast<const char*>(outputBuffer), totalOutput);
            qDebug() << "解压结果 len:" << totalOutput << ", output:" << output;
            totalOutput = 0;
        } while (result != BROTLI_DECODER_RESULT_SUCCESS);

        // Cleanup
        BrotliDecoderDestroyInstance(state);

        return output;
    }

    /**
     * （用不了）直接报解压失败
     */
    static QByteArray brotliDecompress(const QByteArray& compressedData)
    {
        size_t compressedSize = compressedData.size();
        const uint8_t* compressedBuffer = reinterpret_cast<const uint8_t*>(compressedData.constData());

        size_t decompressedSize = 2 * compressedSize; // 估计解压后的大小
        uint8_t* decompressedBuffer = new uint8_t[decompressedSize];

        int result = BrotliDecoderDecompress(compressedSize, compressedBuffer, &decompressedSize, decompressedBuffer);
        if (result != BROTLI_DECODER_RESULT_SUCCESS) {
            qCritical() << "Brotli解压失败" << result;
            delete[] decompressedBuffer;
            return QByteArray(); // 解压失败
        }

        QByteArray decompressedData = QByteArray(reinterpret_cast<char*>(decompressedBuffer), decompressedSize);
        return decompressedData;
    }

    /**
     * 貌似能用的解压
     */
    static QByteArray decompressData(const char* inputData, size_t inputSize) {
        const size_t outputBufferSize = 4096; // 设置输出缓冲区大小
        QByteArray outputData; // 存储解压缩后的数据

        BrotliDecoderState* brotliState = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
        if (!brotliState) {
            // 创建Brotli解压缩实例失败
            return outputData;
        }

        // 解压缩循环
        size_t availableInput = inputSize;
        const uint8_t* nextInput = reinterpret_cast<const uint8_t*>(inputData);
        int index = 0;
        do {
            // qDebug() << "Brotli解压遍历：" << index++ << brotliState << availableInput << nextInput;
            if (availableInput <= 1)
                break;

            uint8_t outputBuffer[outputBufferSize];
            size_t availableOutput = outputBufferSize;
            uint8_t* nextOutput = outputBuffer;

            BrotliDecoderResult result = BrotliDecoderDecompressStream(
                brotliState, &availableInput, &nextInput,
                &availableOutput, &nextOutput, nullptr
            );

            if (result == BROTLI_DECODER_RESULT_SUCCESS ||
                result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
                // 将解压缩的数据添加到输出数据中
                outputData.append(reinterpret_cast<char*>(outputBuffer), outputBufferSize - availableOutput);
            } else {
                // 解压缩失败
                qCritical() << "Brotli遍历解压失败" << result;
                break;
            }
        } while (availableInput > 0);

        BrotliDecoderDestroyInstance(brotliState);

        return outputData;
    }
};

#endif // BILIBILIAPIUTIL_H
