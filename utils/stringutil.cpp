#include "stringutil.h"

QStringList getStrMids(const QString& text, QString l, QString r)
{
    QStringList ans;
    int posl = 0, posr = 0;
    while (posl != -1 && posr != -1)
    {
        posl = text.indexOf(l, posr);
        if (posl == -1) return ans;
        posl += l.length();
        posr = text.indexOf(r, posl);
        if (posr == -1) return ans;
        ans << text.mid(posl, posr-posl);
        posr += r.length();
    }

    return ans;
}

QString getStrMid(const QString& text, QString l, QString r)
{
    int posl = text.indexOf(l);
    if (posl == -1) return "";
    posl += l.length();
    int posr = text.indexOf(r, posl);
    if (posr == -1) return "";
    return text.mid(posl, posr-posl);
}

QString fnEncode(QString text)
{
    QString cs[] = {"\\", "/", ":", "*", "?", "\"", "<", ">", "|", "\'", "\n", "\t"};
    for (int i = 0; i < 12; i++)
        text.replace(cs[i], QString("[&c%1;]").arg(i));
    return text;
}

QString fnDecode(QString text)
{
    QString cs[] = {"\\", "/", ":", "*", "?", "\"", "<", ">", "|", "\'", "\n", "\t"};
    for (int i = 0; i < 12; i++)
        text.replace( QString("[&c%1;]").arg(i), cs[i]);
    return text;
}

QString toFileName(QString text)
{
    QString cs[] = {"\\", "/", ":", "*", "?", "\"", "<", ">", "|", "\'", "\n", "\t"};
    for (int i = 0; i < 12; i++)
        text.replace(cs[i], "-");
    return text;
}

bool canRegExp(const QString& str, const QString& pat)
{
    return QRegExp(pat).indexIn(str) > -1;
}

/**
 * 含有正则表达式元字符的字符串转换为能直接放到其它正则里面的转义字符串
 */
QString transToReg(QString s)
{
//    s = s.replace(QRegExp("[\\.\\+\\-\\*\\^\\$\\(\\)\\[\\]\\{\\}]"), "\\\\1");
    s = s.replace(".", "\\.")
            .replace("+", "\\+")
            .replace("-", "\\-")
            .replace("*", "\\*")
            .replace("(", "\\(")
            .replace(")", "\\)")
            .replace("[", "\\[")
            .replace("]", "\\]")
            .replace("{", "\\{")
            .replace("}", "\\}");
    return s;
}

QString getXml(const QString& str, const QString& pat)
{
    int pos1 = str.indexOf(QString("<%1>").arg(pat));
    if (pos1 == -1) return "";
    pos1 += pat.length()+2;
    int pos2 = str.indexOf(QString("</%1>").arg(pat));
    if (pos2 == -1) pos2 = str.length();
    return str.mid(pos1, pos2-pos1).replace(XML_SPLIT, "</"+pat+">");
}

int getXmlInt(const QString &str, const QString &pat)
{
    QString s = getXml(str, pat).trimmed();
    if (s.isEmpty()) return 0;
    bool ok;
    int res = s.toInt(&ok);
    return ok?res:0;
}

QStringList getXmls(const QString& str, const QString& pat)
{
//    return getStrMids(str, "<"+pat+">", "</"+pat+">");
    QStringList ans;
    QString l = "<"+pat+">", r = "</"+pat+">";
    int posl = 0, posr = 0;
    while (posl != -1 && posr != -1)
    {
        posl = str.indexOf(l, posr);
        if (posl == -1) return ans;
        posl += l.length();
        posr = str.indexOf(r, posl);
        if (posr == -1) return ans;
        ans << str.mid(posl, posr-posl).replace(XML_SPLIT, r);
        posr += r.length();
    }
    return ans;
}

QString makeXml(QString str, const QString& pat)
{
    str = str.replace("</"+pat+">", XML_SPLIT);
    return QString("<%1>%2</%3>").arg(pat).arg(str).arg(pat);
}

QString makeXml(int i, const QString& pat)
{
    return QString("<%1>%2</%3>").arg(pat).arg(i).arg(pat);
}

QString makeXml(qint64 i/*专门针对时间戳*/, const QString& pat)
{
    return QString("<%1>%2</%3>").arg(pat).arg(i).arg(pat);
}

QString makeXml(bool i, const QString &pat)
{
    return QString("<%1>%2</%3>").arg(pat).arg(i ? 1 : 0).arg(pat);
}

bool isBlankChar(QString c)
{
    return (c == " " || c == "\n" || c == "　" || c == "\t" || c == "\r");
}

bool isBlankChar2(QString c)
{
    return (c == " " || c == "　" || c == "\t");
}

bool isAllBlank(QString s)
{
    int len = s.length();
    for (int i = 0; i < len; i++)
        if (!isBlankChar(s.mid(i, 1)))
            return false;
    return true;
}

QString repeatString(QString s, int i)
{
    QString ans = "";
    while (i--)
        ans += s;
    return ans;
}

QString removeBlank(QString s, bool start, bool end)
{
    int len = s.length();
    if (start)
    {
        int pos = 0;
        while (pos < len && isBlankChar(s.mid(pos, 1)))
            pos++;
        if (pos)
        {
            s = s.right(len - pos);
            len -= pos;
        }
    }
    if (end)
    {
        int pos = len;
        while (pos > 0 && isBlankChar(s.mid(pos - 1, 1)))
            pos--;
        if (pos < len)
        {
            s = s.left(pos);
        }
    }
    return s;
}

/**
 * 删首尾空，并且保留章节开头
 * @param  text 文本
 * @return      删首尾空后的文本
 */
QString simplifyChapter(QString text)
{
    int pos = 0, len = text.length();

    // 去除开头所有空白，保留第一段空格
    while (pos < len && isBlankChar(text.mid(pos, 1)))
        pos++;
    while (pos > 0 && text.mid(pos-1, 1) != "\n")
        pos--;
    int start_pos = pos;

    // 去除末尾所有空白
    pos = len;
    while (pos > 0 && isBlankChar(text.mid(pos-1, 1)))
        pos--;
    while (pos < len && text.mid(pos, 1) != "\n")
        pos++;
    int end_pos = pos;

    return text.mid(start_pos, end_pos-start_pos);
}

QString urlEncode(QString s)
{
    return QUrl::toPercentEncoding(s);
}

QString urlDecode(QString s)
{
    return QUrl::fromPercentEncoding(s.toUtf8());
}

bool canBeNickname(QString s)
{
    QRegExp re("^([\\w@_]|[^\\x00-\\xff]){2,20}$");
    return re.exactMatch(s);
}

QString ArabToCN(int num)
{
    static const QString letter[] = {"零","一","二","三","四","五","六","七","八","九"};
    static const QString unit[] = {"","十","百","千","万","十","百","千","亿","十"};
    QString src, des;
    char tmp[12];
    sprintf(tmp, "%d", num);
    src.append(tmp);

    if ( num < 0 )
    {
        des.append("负");
        src.remove(0, 1);
    }

    int len = src.length();
    bool bPreZero = false;
    for ( int i = 0; i < len; i++)
    {
        int digit = src.at(i).unicode() - '0';
        int unit_index = len - i - 1;
        if (i == 0 && digit == 1 && (unit_index == 1 || unit_index == 5 || unit_index == 9))
        {
            des.append(unit[unit_index]);
        }
        else if ( digit == 0 )
        {
            bPreZero = true;
            if (unit_index ==  4 ||
                    unit_index ==  8)
            {
                des.append(unit[unit_index]);
            }
        }
        else
        {
            if ( bPreZero )
            {
                des.append(letter[0]);
            }
            des.append(letter[digit]);
            des.append(unit[unit_index]);
            bPreZero = false;
        }
    }
    return des;
}

int CNToArab(QString text)
{
    int len = text.length();
    text.replace("两", "二");
    qint64 val = 0;
    int find_pos;
    QString seq("一二三四五六七八九");
    for (int i = 0; i < len; i++)
    {
        QString ch = text.mid(i, 1);
        if (ch == "零")
        { }
        else if ((find_pos = seq.indexOf(ch)) > -1)
        {
            if (i >= len-1)
            {
                val += find_pos+1;
                break;
            }
            QString ch1 = text.mid(i+1, 1);
            int mul = 1;
            if (ch1 == "十")
                mul = 10;
            else if (ch1 == "百")
                mul = 100;
            else if (ch1 == "千")
                mul = 1000;
            val += (find_pos+1) * mul;
            if (mul>1) ++i;
        }
        else if (ch == "十") // 第一位就是 十 开头，或者前面是零
            val += 10;
        else if (ch == "万")
            val *= 10000;
        else if (ch == "亿")
            val *= 100000000;
        else if (ch == "兆")
            val *= 10000000000;
    }
    return (int)val;
}

/**
 * 判断文本是不是HTML格式
 */
bool isHtmlString(const QString &str)
{
    return str.startsWith("<!DOCTYPE HTML");
}
