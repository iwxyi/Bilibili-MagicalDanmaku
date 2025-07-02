#include "warmwishutil.h"
#include "qt_compat.h"
#include "qt_compat_random.h"

QString WarmWishUtil::getWarmWish(QString path)
{
    QString all = readTextFile(path);
    QStringList wishes = getPromptWish(all);

    if (wishes.size() == 0)
        return "";

    // 获取随机祝福
    srand(time(0));
    int r = qrand() % (wishes.size()*3); // 祝福语概率，三分之一
    if (r >= wishes.size())
        return "";
    return wishes.at(r);
}

QStringList WarmWishUtil::getPromptWish(QString content)
{
    QStringList f_list = getXmls(content, "WISH");
    if (f_list.size() == 0) // 不是标准XML，直接按行分割
        return content.split("", SKIP_EMPTY_PARTS);

    // 获取当前时间（分钟数）
    QDateTime current_time = QDateTime::currentDateTime();
    QDate date = QDate::currentDate();
    QTime time = current_time.time();
    int year = date.year();
    int month = date.month();
    int day = date.day();
    int hour = time.hour(), minute = time.minute();
    int current_minutes = hour * 60 + minute;

    // 标准获取每一条
    QStringList wishs;
    foreach(QString f_s, f_list)
    {
        QString wish = getOneWish(f_s, current_minutes, month, day);
        if (!wish.isEmpty())
            wishs << wish;
    }
    return wishs;
}

QString WarmWishUtil::getOneWish(QString content, int minutes, int cmonth, int cday)
{
    QString wish = getXml(content, "SAY");
    if (wish.isEmpty()) // 空内容，直接删首尾空并 返回
        return content.trimmed();

    int r_minutes = minutes-1440; // 这是反过来的（昨天）

    // 判断日期
    int month = getXmlInt(content, "MON");
    int day = getXmlInt(content, "DAY");
    if (month > 0 && day > 0)
    {
        if (cmonth != month || cday != day)
            return "";
    }

    // 判断时间
    int start = getXmlInt(content, "STA");
    int end = getXmlInt(content, "END");

    if (!start && !end) // 没有限制时间
        return wish;
    if (start <= minutes && end >= minutes) // 在时间范围内
        return wish;
    if (start <= r_minutes && end >= r_minutes) // 在倒过来时间范围内
        return wish;
    else // 不在时间范围内，返回空
        return "";
}
