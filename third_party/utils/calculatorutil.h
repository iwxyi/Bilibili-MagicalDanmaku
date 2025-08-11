#ifndef CALCULATORUTIL_H
#define CALCULATORUTIL_H

#include <QString>
#include <QHash>
#include <QList>
#include <QDebug>

#define CALC_DEB if (0) qDebug() // 输出计算相关的信息

class CalculatorUtil
{
public:
    // 函数类型枚举
    enum FunctionType {
        Sin, Cos, Tan, Abs, Sqrt, Log, Ln, Floor, Ceil, Round,
        Min, Max, Sum, Avg, Count
    };
    
    /**
     * 计算整数表达式
     * 支持四则运算、括号、函数等
     */
    static qint64 calcIntExpression(QString exp);
    
    /**
     * 计算浮点数表达式
     * 支持小数点运算
     */
    static double calcDoubleExpression(QString exp);
    
    /**
     * 清理缓存
     */
    static void clearCache();

private:
    // 缓存相关
    static QHash<QString, qint64> intCache;
    static QHash<QString, double> doubleCache;
    static const int MAX_CACHE_SIZE = 1000;
    
    // 解析函数
    static qint64 parseExpression(QString& exp);
    static double parseExpressionDouble(QString& exp);
    
    static qint64 parseTerm(QString& exp);
    static double parseTermDouble(QString& exp);
    
    static qint64 parseFactor(QString& exp);
    static double parseFactorDouble(QString& exp);
    
    static qint64 parseNumber(QString& exp);
    static double parseNumberDouble(QString& exp);
    
    // 特殊数字解析
    static qint64 parseHexNumber(QString& exp);
    static qint64 parseBinaryNumber(QString& exp);
    static double parseScientificNotation(QString& exp);
    
    // 函数解析
    static qint64 parseFunction(QString& exp);
    static double parseFunctionDouble(QString& exp);
    
    // 统计函数
    static qint64 parseStatFunction(QString& exp, FunctionType type);
    static double parseStatFunctionDouble(QString& exp, FunctionType type);
    
    // 随机数函数
    static qint64 parseRandomFunction(QString& exp);
    
    // 辅助函数
    static QStringList extractFunctionArgs(QString& exp);
    static FunctionType getFunctionType(const QString& funcName);
    static qint64 applyMathFunction(FunctionType type, qint64 value);
    static double applyMathFunctionDouble(FunctionType type, double value);
    static qint64 applyStatFunction(FunctionType type, const QList<qint64>& values);
    static double applyStatFunctionDouble(FunctionType type, const QList<double>& values);
    static qint64 generateRandomNumber(qint64 min, qint64 max);
    static double generateRandomDouble(double min, double max);
    
    // 位运算
    static qint64 bitwiseAnd(qint64 a, qint64 b);
    static qint64 bitwiseOr(qint64 a, qint64 b);
    static qint64 bitwiseXor(qint64 a, qint64 b);
    static qint64 bitwiseShiftLeft(qint64 a, qint64 b);
    static qint64 bitwiseShiftRight(qint64 a, qint64 b);
    
    // 单位转换
    static double convertUnit(double value, const QString& fromUnit, const QString& toUnit);
    
    // 清理空白字符
    static QString cleanExpression(const QString& exp);
};

#endif // CALCULATORUTIL_H
