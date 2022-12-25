#ifndef EMOTICON_H
#define EMOTICON_H

#include <QString>

/**
 * 图片表情
 */
struct Emoticon
{
    QString name;
    QString description;
    qint64 id = 0;
    QString unique;
    int identity = 0;
    int height = 0;
    int width = 0;
    qint64 id_dynamic = 0;
    QString url;

    qint64 pkg_id = 0;
    int pkg_type = 0;
};

#endif // EMOTICON_H
