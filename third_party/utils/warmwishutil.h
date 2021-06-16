#ifndef WARMWISHUTIL_H
#define WARMWISHUTIL_H

#include <QDateTime>
#include "fileutil.h"
#include "stringutil.h"
#include "time.h"

/**
 * 获取温馨祝福的类
 * 支持设置时间区段（以每天0点开始分钟输计时）
 *
 * <PROMPT>
 *     <YEA>2020</YEA>
 *     <MON>1</MON>
 *     <DAY>1</DAY>
 *     <STA>30</STA>
 *     <END>90</END>  表示1点钟前后
 *     <SAY>现在几点？开心一点！</SAY>
 * </PROMPT>
 */

class WarmWishUtil
{
    struct Wish {
        QString content;
        int year, month, day;
        int minute_start;
        int minute_end;

        Wish(QString c, int s = 0, int e = 24)
            : content(c), minute_start(s), minute_end(e),
              year(0), month(0), day(0)
        {  }

        Wish(QString c, int y, int m, int d)
            : year(y), month(m), day(d),
              minute_start(0), minute_end(0)
        {  }
    };

public:
    static QString getWarmWish(QString path);

private:
    static QStringList getPromptWish(QString content);
    static QString getOneWish(QString content, int minutes, int cmonth, int cday);
};

#endif // WARMPROMPTUTIL_H
