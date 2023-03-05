#ifndef CONDITIONUTIL_H
#define CONDITIONUTIL_H

#include <QString>
#include <QDebug>
#include <QRegularExpression>

#define CALC_DEB if (0) qDebug() // 输出计算相关的信息

class ConditionUtil
{
public:
    /**
     * 判断逻辑条件是否成立
     * @param exprs exp1, exp2; exp3
     */
    static bool judgeCondition(QString exprs)
    {
        CALC_DEB << "判断表达式：" << exprs;
        QStringList orExps = exprs.split(QRegularExpression("(;|\\|\\|)"), QString::SkipEmptyParts);
        bool isTrue = false;
        QRegularExpression compRe("^\\s*([^<>=!]*?)\\s*([<>=!~]{1,2})\\s*([^<>=!]*?)\\s*$");
        QRegularExpression intRe("^[\\d\\+\\-\\*\\/% \\(\\)]+$");
        // QRegularExpression overlayIntRe("\\d{11,}");
        QRegularExpressionMatch match;
        foreach (QString orExp, orExps)
        {
            isTrue = true;
            QStringList andExps = orExp.split(QRegularExpression("(,|&&)"), QString::SkipEmptyParts);
            CALC_DEB << "表达式or内：" << andExps;
            foreach (QString exp, andExps)
            {
                CALC_DEB << "表达式and内：" << exp;
                exp = exp.trimmed();
                if (exp.indexOf(compRe, 0, &match) == -1         // 非比较
                        || (match.captured(1).isEmpty() && match.captured(2) == "!"))    // 取反类型
                {
                    bool notTrue = exp.startsWith("!"); // 与否取反
                    if (notTrue) // 取反……
                    {
                        exp = exp.right(exp.length() - 1);
                    }
                    if (exp.isEmpty() || exp == "0" || exp.toLower() == "false") // false
                    {
                        if (!notTrue)
                        {
                            isTrue = false;
                            break;
                        }
                        else // 取反
                        {
                            isTrue = true;
                            break;
                        }
                    }
                    else // true
                    {
                        if (notTrue)
                        {
                            isTrue = false;
                            break;
                        }
                    }
                    continue;
                }

                // 比较类型
                QStringList caps = match.capturedTexts();
                QString s1 = caps.at(1);
                QString op = caps.at(2);
                QString s2 = caps.at(3);
                CALC_DEB << "比较：" << s1 << op << s2;
                if (s1.contains(intRe) && s2.contains(intRe) // 都是整数
                        && QStringList{">", "<", "=", ">=", "<=", "==", "!="}.contains(op)) // 是这个运算符
                        // && !s1.contains(overlayIntRe) && !s2.contains(overlayIntRe)) // 没有溢出
                {
                    qint64 i1 = calcIntExpression(s1);
                    qint64 i2 = calcIntExpression(s2);
                    CALC_DEB << "比较整数" << i1 << op << i2;
                    if (!isConditionTrue<qint64>(i1, i2, op))
                    {
                        isTrue = false;
                        break;
                    }
                }
                else/* if (s1.startsWith("\"") || s1.endsWith("\"") || s1.startsWith("'") || s1.endsWith("'")
                        || s2.startsWith("\"") || s2.startsWith("\"") || s2.startsWith("'") || s2.startsWith("'")) // 都是字符串*/
                {
                    auto removeQuote = [=](QString s) -> QString{
                        if (s.startsWith("\"") && s.endsWith("\""))
                            return s.mid(1, s.length()-2);
                        if (s.startsWith("'") && s.endsWith("'"))
                            return s.mid(1, s.length()-2);
                        return s;
                    };
                    s1 = removeQuote(s1);
                    s2 = removeQuote(s2);
                    CALC_DEB << "比较字符串" << s1 << op << s2;
                    if (op == "~")
                    {
                        if (s2.contains("~") && !s2.endsWith("~")) // 特殊格式判断：文字1~文字2 ~ 文字3 [\u4e00-\u9fa5]+[\w]{3}
                        {
                            QString full = caps.at(0);
                            QRegularExpression re("^\\s*(.*)\\s*(~)\\s*([^~]*?)\\s*$");
                            if (!re.isValid())
                                qWarning() << "错误的正则表达式1：" << re.errorString();
                            if (full.indexOf(re, 0, &match) == -1)
                            {
                                qWarning() << "错误的~运算：" << full;
                                isTrue = false;
                                break;
                            }
                            caps = match.capturedTexts();
                            s1 = caps.at(1);
                            s2 = caps.at(3);
                            CALC_DEB << "纠正运算：" << s1 << "~" << s2;
                            // qWarning() << "错误的~表达式：" << s2;
                        }

                        QRegularExpression re(s2);
                        if (!re.isValid())
                            qWarning() << "错误的正则表达式2：" << re.errorString();
                        if (!s1.contains(QRegularExpression(s2)))
                        {
                            isTrue = false;
                            break;
                        }
                    }
                    else if (!isConditionTrue<QString>(s1, s2, op))
                    {
                        isTrue = false;
                        break;
                    }
                }
                /*else
                {
                    qCritical() << "error: 无法比较的表达式:" << match.capturedTexts().first();
                    qCritical() << "    原始语句：" << msg;
                }*/
            }
            if (isTrue)
                break;
        }
        return isTrue;
    }

    /**
     * 计算纯int、运算符组成的表达式
     * 不支持括号、优先级判断
     * 1 + 2 + 3
     * 1 + 2 * 3 / 4
     */
    static qint64 calcIntExpression(QString exp)
    {
        exp = exp.replace(QRegularExpression("\\s*"), ""); // 去掉所有空白
        QRegularExpression opRe("[\\+\\-\\*/%]");

        // 获取所有整型数值
        QStringList valss = exp.split(opRe); // 如果是-开头，那么会当做 0-x
        if (valss.size() == 0)
            return 0;
        QList<qint64> vals;
        foreach (QString val, valss)
        {
            bool ok = true;
            qint64 ll = val.toLongLong(&ok);
            if (!ok && !val.isEmpty())
            {
                qWarning() << "转换整数值失败：" << exp;
                qDebug() << "exp:" << exp << exp.startsWith("\"") << ll;
                if (val.length() > 18) // 19位数字，超出了ll的范围
                    ll = val.right(18).toLongLong();
            }
            vals << ll;
        }

        // 获取所有运算符
        QStringList ops;
        QRegularExpressionMatchIterator i = opRe.globalMatch(exp);
        while (i.hasNext())
        {
            ops << i.next().captured(0);
        }
        if (valss.size() != ops.size() + 1)
        {
            qCritical() << "错误的表达式：" << valss << ops << exp;
            return 0;
        }

        // 入栈：* / %
        for (int i = 0; i < ops.size(); i++)
        {
            // op[i] 操作 vals[i] x vals[i+1]
            if (ops[i] == "*")
            {
                vals[i] *= vals[i+1];
            }
            else if (ops[i] == "/")
            {
                // qDebug() << "除法" << ops << vals;
                if (vals[i+1] == 0)
                {
                    qWarning() << "!!!被除数是0 ：" << exp;
                    vals[i+1] = 1;
                }
                vals[i] /= vals[i+1];
            }
            else if (ops[i] == "%")
            {
                if (vals[i+1] == 0)
                {
                    qWarning() << "!!!被模数是0 ：" << exp;
                    vals[i+1] = 1;
                }
                vals[i] %= vals[i+1];
            }
            else
                continue;
            vals.removeAt(i+1);
            ops.removeAt(i);
            i--;
        }

        // 顺序计算：+ -
        qint64 val = vals.first();
        for (int i = 0; i < ops.size(); i++)
        {
            if (ops[i] == "-")
                val -= vals[i+1];
            else if (ops[i] == "+")
                val += vals[i+1];
        }

        return val;
    }

    template<typename T>
    static bool isConditionTrue(T a, T b, QString op)
    {
        if (op == "==" || op == "=")
            return a == b;
        if (op == "!=" || op == "<>")
            return a != b;
        if (op == ">")
            return a > b;
        if (op == ">=")
            return a >= b;
        if (op == "<")
            return a < b;
        if (op == "<=")
            return a <= b;
        qWarning() << "无法识别的比较模板类型：" << a << op << b;
        return false;
    }
};

#endif // CONDITIONUTIL_H
