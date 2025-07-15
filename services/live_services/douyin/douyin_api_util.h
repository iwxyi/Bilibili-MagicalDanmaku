#ifndef DOUYIN_API_UTIL_H
#define DOUYIN_API_UTIL_H

#include <QByteArray>
#include <QBuffer>
#include "zlib.h"

class DouyinApiUtil
{
public:
    // Gzip解压函数 (这是一个通用的辅助函数，你可以把它放在一个工具类里)
    static QByteArray decompressGzip(const QByteArray &data)
    {
        if (data.size() <= 4) {
            return QByteArray();
        }

        QByteArray result;
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;

        if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
            return QByteArray();
        }

        strm.next_in = (Bytef *)data.data();
        strm.avail_in = data.size();

        int ret;
        char outbuffer[4096];
        do {
            strm.next_out = (Bytef *)outbuffer;
            strm.avail_out = sizeof(outbuffer);
            ret = inflate(&strm, Z_NO_FLUSH);
            if (result.size() < strm.total_out) {
                result.append(outbuffer, strm.total_out - result.size());
            }
        } while (ret == Z_OK);

        inflateEnd(&strm);

        if (ret != Z_STREAM_END) {
            // error
            return QByteArray();
        }

        return result;
    }
};

#endif // DOUYIN_API_UTIL_H
