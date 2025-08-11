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
                        // qDebug() << "比较正则表达式：" << s1 << op << s2;
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
     * 支持嵌套括号、运算符优先级、负数
     * 1 + 2 + 3
     * 1 + 2 * 3 / 4
     * (1 + 2) * 3
     * -5 + 3 * (2 - 1)
     */
    static qint64 calcIntExpression(QString exp)
    {
        // 去掉所有空白字符
        exp.remove(QRegularExpression("\\s"));
        
        if (exp.isEmpty()) {
            qWarning() << "表达式为空，返回0";
            return 0;
        }
        
        return parseExpression(exp);
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

private:
    /**
     * 解析表达式（处理加减）
     */
    static qint64 parseExpression(QString& exp)
    {
        if (exp.isEmpty()) {
            return 0;
        }
        
        qint64 left = parseTerm(exp);
        
        while (!exp.isEmpty()) {
            QChar ch = exp[0];
            if (ch != '+' && ch != '-') break;
            
            char op = ch.toLatin1();
            exp = exp.mid(1);
            
            if (exp.isEmpty()) {
                qWarning() << "运算符后缺少操作数，默认为0：" << op;
                return left;
            }
            
            qint64 right = parseTerm(exp);
            
            if (op == '+')
                left += right;
            else
                left -= right;
        }
        
        return left;
    }
    
    /**
     * 解析项（处理乘除模）
     */
    static qint64 parseTerm(QString& exp)
    {
        if (exp.isEmpty()) {
            return 0;
        }
        
        qint64 left = parseFactor(exp);
        
        while (!exp.isEmpty()) {
            QChar ch = exp[0];
            if (ch != '*' && ch != '/' && ch != '%') break;
            
            char op = ch.toLatin1();
            exp = exp.mid(1);
            
            if (exp.isEmpty()) {
                qWarning() << "运算符后缺少操作数，默认为1：" << op;
                if (op == '/') left /= 1;
                else if (op == '%') left %= 1;
                else left *= 1;
                return left;
            }
            
            qint64 right = parseFactor(exp);
            
            if (op == '*')
                left *= right;
            else if (op == '/')
            {
                if (right == 0)
                {
                    qWarning() << "!!!被除数是0，使用1代替：" << exp;
                    right = 1;
                }
                left /= right;
            }
            else if (op == '%')
            {
                if (right == 0)
                {
                    qWarning() << "!!!被模数是0，使用1代替：" << exp;
                    right = 1;
                }
                left %= right;
            }
        }
        
        return left;
    }
    
    /**
     * 解析因子（处理数字、括号、负数）
     */
    static qint64 parseFactor(QString& exp)
    {
        if (exp.isEmpty()) {
            return 0;
        }
        
        QChar ch = exp[0];
        
        // 处理负数
        if (ch == '-')
        {
            exp = exp.mid(1);
            if (exp.isEmpty()) {
                qWarning() << "负号后缺少数字，默认为0";
                return 0;
            }
            return -parseFactor(exp);
        }
        
        // 处理正数符号（可选）
        if (ch == '+')
        {
            exp = exp.mid(1);
            if (exp.isEmpty()) {
                qWarning() << "正号后缺少数字，默认为0";
                return 0;
            }
        }
        
        // 处理括号
        if (ch == '(')
        {
            exp = exp.mid(1); // 跳过左括号
            qint64 result = parseExpression(exp);
            
            if (!exp.isEmpty() && exp[0] == ')')
            {
                exp = exp.mid(1); // 跳过右括号
                return result;
            }
            else
            {
                qWarning() << "缺少右括号，自动补全：" << exp;
                return result;
            }
        }
        
        // 处理数字 - 使用手动解析替代正则表达式
        if (ch.isDigit())
        {
            qint64 num = 0;
            int startPos = 0;
            int len = exp.length();
            
            // 手动解析数字，避免正则表达式开销
            while (startPos < len && exp[startPos].isDigit()) {
                num = num * 10 + (exp[startPos].digitValue());
                startPos++;
            }
            
            exp = exp.mid(startPos);
            return num;
        }
        
        // 处理其他无法识别的字符
        qWarning() << "无法解析的表达式，跳过第一个字符：" << exp;
        if (!exp.isEmpty()) {
            exp = exp.mid(1);
            return parseFactor(exp); // 递归尝试解析剩余部分
        }
        
        return 0;
    }
};

#endif // CONDITIONUTIL_H
