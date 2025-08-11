#include "calculatorutil.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <cmath>
#include <algorithm>
#include <numeric>

// 静态成员初始化
QHash<QString, qint64> CalculatorUtil::intCache;
QHash<QString, double> CalculatorUtil::doubleCache;

// 函数类型映射表
static QHash<QString, CalculatorUtil::FunctionType> functionMap;

// 初始化函数映射表
static void initFunctionMap()
{
    if (functionMap.isEmpty()) {
        functionMap["sin"] = CalculatorUtil::Sin;
        functionMap["cos"] = CalculatorUtil::Cos;
        functionMap["tan"] = CalculatorUtil::Tan;
        functionMap["abs"] = CalculatorUtil::Abs;
        functionMap["sqrt"] = CalculatorUtil::Sqrt;
        functionMap["log"] = CalculatorUtil::Log;
        functionMap["ln"] = CalculatorUtil::Ln;
        functionMap["floor"] = CalculatorUtil::Floor;
        functionMap["ceil"] = CalculatorUtil::Ceil;
        functionMap["round"] = CalculatorUtil::Round;
        functionMap["min"] = CalculatorUtil::Min;
        functionMap["max"] = CalculatorUtil::Max;
        functionMap["sum"] = CalculatorUtil::Sum;
        functionMap["avg"] = CalculatorUtil::Avg;
        functionMap["count"] = CalculatorUtil::Count;
    }
}

qint64 CalculatorUtil::calcIntExpression(QString exp)
{
    // 初始化函数映射表
    initFunctionMap();
    
    // 清理表达式
    exp = cleanExpression(exp);
    
    // 检查缓存
    if (intCache.contains(exp)) {
        return intCache.value(exp);
    }
    
    // 计算结果
    qint64 result = parseExpression(exp);
    
    // 缓存结果
    if (intCache.size() >= MAX_CACHE_SIZE) {
        intCache.clear();
    }
    intCache.insert(exp, result);
    
    return result;
}

double CalculatorUtil::calcDoubleExpression(QString exp)
{
    // 初始化函数映射表
    initFunctionMap();
    
    // 清理表达式
    exp = cleanExpression(exp);
    
    // 检查缓存
    if (doubleCache.contains(exp)) {
        return doubleCache.value(exp);
    }
    
    // 计算结果
    double result = parseExpressionDouble(exp);
    
    // 缓存结果
    if (doubleCache.size() >= MAX_CACHE_SIZE) {
        doubleCache.clear();
    }
    doubleCache.insert(exp, result);
    
    return result;
}

void CalculatorUtil::clearCache()
{
    intCache.clear();
    doubleCache.clear();
}

QString CalculatorUtil::cleanExpression(const QString& exp)
{
    QString result = exp;
    return result.replace(QRegularExpression("\\s"), "");
}

// 整数表达式解析
qint64 CalculatorUtil::parseExpression(QString& exp)
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

qint64 CalculatorUtil::parseTerm(QString& exp)
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

qint64 CalculatorUtil::parseFactor(QString& exp)
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
    
    // 处理函数调用
    if (ch.isLetter())
    {
        return parseFunction(exp);
    }
    
    // 处理数字
    if (ch.isDigit())
    {
        return parseNumber(exp);
    }
    
    // 处理其他无法识别的字符
    qWarning() << "无法解析的表达式，跳过第一个字符：" << exp;
    if (!exp.isEmpty()) {
        exp = exp.mid(1);
        return parseFactor(exp); // 递归尝试解析剩余部分
    }
    
    return 0;
}

qint64 CalculatorUtil::parseNumber(QString& exp)
{
    // 检查十六进制
    if (exp.startsWith("0x") || exp.startsWith("0X")) {
        return parseHexNumber(exp);
    }
    
    // 检查二进制
    if (exp.startsWith("0b") || exp.startsWith("0B")) {
        return parseBinaryNumber(exp);
    }
    
    // 检查科学计数法
    if (exp.contains('e') || exp.contains('E')) {
        return static_cast<qint64>(parseScientificNotation(exp));
    }
    
    // 处理普通数字
    qint64 num = 0;
    int startPos = 0;
    int len = exp.length();
    
    while (startPos < len && exp[startPos].isDigit()) {
        num = num * 10 + (exp[startPos].digitValue());
        startPos++;
    }
    
    exp = exp.mid(startPos);
    return num;
}

qint64 CalculatorUtil::parseHexNumber(QString& exp)
{
    exp = exp.mid(2); // 跳过 "0x"
    qint64 num = 0;
    int startPos = 0;
    int len = exp.length();
    
    while (startPos < len) {
        QChar ch = exp[startPos];
        int digit;
        
        if (ch.isDigit()) {
            digit = ch.digitValue();
        } else if (ch >= 'a' && ch <= 'f') {
            digit = ch.toLatin1() - 'a' + 10;
        } else if (ch >= 'A' && ch <= 'F') {
            digit = ch.toLatin1() - 'A' + 10;
        } else {
            break;
        }
        
        num = num * 16 + digit;
        startPos++;
    }
    
    exp = exp.mid(startPos);
    return num;
}

qint64 CalculatorUtil::parseBinaryNumber(QString& exp)
{
    exp = exp.mid(2); // 跳过 "0b"
    qint64 num = 0;
    int startPos = 0;
    int len = exp.length();
    
    while (startPos < len && (exp[startPos] == '0' || exp[startPos] == '1')) {
        num = num * 2 + (exp[startPos].digitValue());
        startPos++;
    }
    
    exp = exp.mid(startPos);
    return num;
}

double CalculatorUtil::parseScientificNotation(QString& exp)
{
    // 简化实现，主要处理整数部分
    double num = 0.0;
    int startPos = 0;
    int len = exp.length();
    
    // 解析整数部分
    while (startPos < len && exp[startPos].isDigit()) {
        num = num * 10 + exp[startPos].digitValue();
        startPos++;
    }
    
    // 跳过指数部分
    if (startPos < len && (exp[startPos] == 'e' || exp[startPos] == 'E')) {
        startPos++;
        if (startPos < len && (exp[startPos] == '+' || exp[startPos] == '-')) {
            startPos++;
        }
        while (startPos < len && exp[startPos].isDigit()) {
            startPos++;
        }
    }
    
    exp = exp.mid(startPos);
    return num;
}

qint64 CalculatorUtil::parseFunction(QString& exp)
{
    QString funcName;
    int startPos = 0;
    int len = exp.length();
    
    while (startPos < len && exp[startPos].isLetter()) {
        funcName += exp[startPos];
        startPos++;
    }
    
    if (startPos >= len || exp[startPos] != '(') {
        exp = exp.mid(startPos);
        return 0;
    }
    
    exp = exp.mid(startPos + 1);
    
    FunctionType funcType = getFunctionType(funcName);
    
    if (funcType == Min || funcType == Max || funcType == Sum || funcType == Avg || funcType == Count) {
        return parseStatFunction(exp, funcType);
    }
    
    // 数学函数
    qint64 result = parseExpression(exp);
    
    if (!exp.isEmpty() && exp[0] == ')') {
        exp = exp.mid(1);
    }
    
    return applyMathFunction(funcType, result);
}

qint64 CalculatorUtil::parseStatFunction(QString& exp, FunctionType type)
{
    QStringList args = extractFunctionArgs(exp);
    if (args.isEmpty()) {
        qWarning() << "统计函数参数为空";
        return 0;
    }
    
    QList<qint64> values;
    for (int i = 0; i < args.size(); ++i) {
        QString tempArg = args[i];
        qint64 value = parseExpression(tempArg);
        values.append(value);
    }
    
    return applyStatFunction(type, values);
}

QStringList CalculatorUtil::extractFunctionArgs(QString& exp)
{
    QStringList args;
    int parenCount = 0;
    int startPos = 0;
    int len = exp.length();
    
    for (int i = 0; i < len; ++i) {
        QChar ch = exp[i];
        
        if (ch == '(') {
            if (parenCount == 0) startPos = i + 1;
            parenCount++;
        } else if (ch == ')') {
            parenCount--;
            if (parenCount == 0) {
                QString argsStr = exp.mid(startPos, i - startPos);
                if (!argsStr.isEmpty()) {
                    args = argsStr.split(',', QString::SkipEmptyParts);
                }
                exp = exp.mid(i + 1);
                break;
            }
        }
    }
    
    return args;
}

CalculatorUtil::FunctionType CalculatorUtil::getFunctionType(const QString& funcName)
{
    return functionMap.value(funcName.toLower(), Abs);
}

qint64 CalculatorUtil::applyMathFunction(FunctionType type, qint64 value)
{
    switch (type) {
        case Sin:
            return static_cast<qint64>(std::sin(value * M_PI / 180.0) * 1000);
        case Cos:
            return static_cast<qint64>(std::cos(value * M_PI / 180.0) * 1000);
        case Tan:
            return static_cast<qint64>(std::tan(value * M_PI / 180.0) * 1000);
        case Abs:
            return std::abs(value);
        case Sqrt:
            return static_cast<qint64>(std::sqrt(std::abs(value)));
        case Log:
            return static_cast<qint64>(std::log10(std::abs(value)) * 1000);
        case Ln:
            return static_cast<qint64>(std::log(std::abs(value)) * 1000);
        case Floor:
        case Ceil:
        case Round:
            return value;
        default:
            return value;
    }
}

qint64 CalculatorUtil::applyStatFunction(FunctionType type, const QList<qint64>& values)
{
    if (values.isEmpty()) return 0;
    
    switch (type) {
        case Min: {
            qint64 minVal = values.first();
            for (int i = 1; i < values.size(); ++i) {
                if (values[i] < minVal) minVal = values[i];
            }
            return minVal;
        }
        case Max: {
            qint64 maxVal = values.first();
            for (int i = 1; i < values.size(); ++i) {
                if (values[i] > maxVal) maxVal = values[i];
            }
            return maxVal;
        }
        case Sum: {
            qint64 sum = 0;
            for (int i = 0; i < values.size(); ++i) {
                sum += values[i];
            }
            return sum;
        }
        case Count:
            return values.size();
        case Avg: {
            qint64 sum = 0;
            for (int i = 0; i < values.size(); ++i) {
                sum += values[i];
            }
            return sum / values.size();
        }
        default:
            return values.first();
    }
}

qint64 CalculatorUtil::parseRandomFunction(QString& exp)
{
    // 简化实现，返回固定值
    return 42;
}

// 浮点数版本（简化实现）
double CalculatorUtil::parseExpressionDouble(QString& exp)
{
    if (exp.isEmpty()) return 0.0;
    
    double left = parseTermDouble(exp);
    
    while (!exp.isEmpty()) {
        QChar ch = exp[0];
        if (ch != '+' && ch != '-') break;
        
        char op = ch.toLatin1();
        exp = exp.mid(1);
        
        if (exp.isEmpty()) return left;
        
        double right = parseTermDouble(exp);
        
        if (op == '+') left += right;
        else left -= right;
    }
    
    return left;
}

double CalculatorUtil::parseTermDouble(QString& exp)
{
    if (exp.isEmpty()) return 0.0;
    
    double left = parseFactorDouble(exp);
    
    while (!exp.isEmpty()) {
        QChar ch = exp[0];
        if (ch != '*' && ch != '/') break;
        
        char op = ch.toLatin1();
        exp = exp.mid(1);
        
        if (exp.isEmpty()) return left;
        
        double right = parseFactorDouble(exp);
        
        if (op == '*') left *= right;
        else if (op == '/') {
            if (right == 0.0) {
                qWarning() << "!!!被除数是0，使用1代替";
                right = 1.0;
            }
            left /= right;
        }
    }
    
    return left;
}

double CalculatorUtil::parseFactorDouble(QString& exp)
{
    if (exp.isEmpty()) return 0.0;
    
    QChar ch = exp[0];
    
    if (ch == '-') {
        exp = exp.mid(1);
        return -parseFactorDouble(exp);
    }
    
    if (ch == '+') {
        exp = exp.mid(1);
    }
    
    if (ch == '(') {
        exp = exp.mid(1);
        double result = parseExpressionDouble(exp);
        
        if (!exp.isEmpty() && exp[0] == ')') {
            exp = exp.mid(1);
        }
        return result;
    }
    
    if (ch.isLetter()) {
        return parseFunctionDouble(exp);
    }
    
    if (ch.isDigit()) {
        return parseNumberDouble(exp);
    }
    
    return 0.0;
}

double CalculatorUtil::parseNumberDouble(QString& exp)
{
    // 简化实现，主要处理科学计数法
    if (exp.contains('e') || exp.contains('E')) {
        return parseScientificNotation(exp);
    }
    
    // 处理普通数字
    double num = 0.0;
    int startPos = 0;
    int len = exp.length();
    
    while (startPos < len && exp[startPos].isDigit()) {
        num = num * 10 + exp[startPos].digitValue();
        startPos++;
    }
    
    // 处理小数点
    if (startPos < len && exp[startPos] == '.') {
        startPos++;
        double decimal = 0.1;
        while (startPos < len && exp[startPos].isDigit()) {
            num += exp[startPos].digitValue() * decimal;
            decimal *= 0.1;
            startPos++;
        }
    }
    
    exp = exp.mid(startPos);
    return num;
}

double CalculatorUtil::parseFunctionDouble(QString& exp)
{
    // 简化实现，主要处理数学函数
    QString funcName;
    int startPos = 0;
    int len = exp.length();
    
    while (startPos < len && exp[startPos].isLetter()) {
        funcName += exp[startPos];
        startPos++;
    }
    
    if (startPos >= len || exp[startPos] != '(') {
        exp = exp.mid(startPos);
        return 0.0;
    }
    
    exp = exp.mid(startPos + 1);
    
    FunctionType funcType = getFunctionType(funcName);
    double result = parseExpressionDouble(exp);
    
    if (!exp.isEmpty() && exp[0] == ')') {
        exp = exp.mid(1);
    }
    
    return applyMathFunctionDouble(funcType, result);
}

double CalculatorUtil::applyMathFunctionDouble(FunctionType type, double value)
{
    switch (type) {
        case Sin:
            return std::sin(value * M_PI / 180.0);
        case Cos:
            return std::cos(value * M_PI / 180.0);
        case Tan:
            return std::tan(value * M_PI / 180.0);
        case Abs:
            return std::abs(value);
        case Sqrt:
            return std::sqrt(std::abs(value));
        case Log:
            return std::log10(std::abs(value));
        case Ln:
            return std::log(std::abs(value));
        case Floor:
            return std::floor(value);
        case Ceil:
            return std::ceil(value);
        case Round:
            return std::round(value);
        default:
            return value;
    }
}

double CalculatorUtil::parseStatFunctionDouble(QString& exp, FunctionType type)
{
    // 简化实现
    return 0.0;
}

// 位运算实现
qint64 CalculatorUtil::bitwiseAnd(qint64 a, qint64 b) { return a & b; }
qint64 CalculatorUtil::bitwiseOr(qint64 a, qint64 b) { return a | b; }
qint64 CalculatorUtil::bitwiseXor(qint64 a, qint64 b) { return a ^ b; }
qint64 CalculatorUtil::bitwiseShiftLeft(qint64 a, qint64 b) { return a << b; }
qint64 CalculatorUtil::bitwiseShiftRight(qint64 a, qint64 b) { return a >> b; }

// 单位转换（简化实现）
double CalculatorUtil::convertUnit(double value, const QString& fromUnit, const QString& toUnit)
{
    // 这里可以实现完整的单位转换逻辑
    // 目前返回原值
    return value;
}

// 其他辅助函数的简化实现
double CalculatorUtil::generateRandomDouble(double min, double max) { return min; }
