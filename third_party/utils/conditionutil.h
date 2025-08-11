#ifndef CONDITIONUTIL_H
#define CONDITIONUTIL_H

#include <QString>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "calculatorutil.h"

class ConditionUtil
{
public:
    /**
     * 判断条件表达式是否为真
     * 支持多种比较操作符：==, !=, <>, >, >=, <, <=
     * 支持嵌套括号、运算符优先级
     * 
     * 示例：
     * 1 == 2          -> false
     * 1 != 2          -> true
     * 1 > 0           -> true
     * (1 + 2) * 3 > 8 -> true
     * 1 + 2 * 3 > 8   -> false (因为 2*3=6, 1+6=7 < 8)
     */
    static bool judgeCondition(QString exprs)
    {
        try {
            // 去掉所有空白字符
            exprs = exprs.replace(QRegularExpression("\\s*"), "");
            if (exprs.isEmpty()) {
                qWarning() << "条件表达式为空，返回false";
                return false;
            }
            
            // 查找比较操作符
            QString op = findOperator(exprs);
            if (op.isEmpty()) {
                qWarning() << "未找到有效的比较操作符：" << exprs;
                return false;
            }
            
            // 分割左右操作数
            int opIndex = exprs.indexOf(op);
            QString leftExpr = exprs.left(opIndex);
            QString rightExpr = exprs.mid(opIndex + op.length());
            
            if (leftExpr.isEmpty() || rightExpr.isEmpty()) {
                qWarning() << "操作数不完整：" << exprs;
                return false;
            }
            
            // 计算左右操作数的值
            qint64 leftValue = CalculatorUtil::calcIntExpression(leftExpr);
            qint64 rightValue = CalculatorUtil::calcIntExpression(rightExpr);
            
            // 执行比较
            return isConditionTrue(leftValue, rightValue, op);
            
        } catch (...) {
            qWarning() << "判断条件时发生异常：" << exprs;
            return false;
        }
    }

private:
    /**
     * 查找比较操作符
     * 按优先级返回找到的第一个操作符
     */
    static QString findOperator(QString exprs)
    {
        // 按优先级顺序查找操作符
        QStringList operators = {"==", "!=", "<>", ">=", "<=", ">", "<", "="};
        
        for (const QString& op : operators) {
            int index = exprs.indexOf(op);
            if (index != -1) {
                return op;
            }
        }
        
        return QString();
    }
    
    /**
     * 判断两个值是否满足比较条件
     */
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
        qWarning() << "无法识别的比较操作符：" << a << op << b;
        return false;
    }
};

#endif // CONDITIONUTIL_H
