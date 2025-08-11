# 高级计算器工具 (CalculatorUtil)

## 🚀 功能特性

### 基础运算
- **四则运算**: `+`, `-`, `*`, `/`, `%`
- **括号支持**: 支持嵌套括号和运算符优先级
- **负数支持**: 支持负数输入和计算

### 进制转换
- **十六进制**: `0xFF`, `0x1A`, `0x1234`
- **二进制**: `0b1010`, `0b1100`
- **十进制**: 标准数字格式

### 科学计数法
- **标准格式**: `1e6`, `2.5e3`, `1.23e-4`
- **混合运算**: 可与普通数字混合计算

### 数学函数
- **三角函数**: `sin()`, `cos()`, `tan()` (角度制)
- **对数函数**: `log()` (常用对数), `ln()` (自然对数)
- **其他函数**: `abs()`, `sqrt()`, `floor()`, `ceil()`, `round()`

### 统计函数
- **聚合函数**: `min()`, `max()`, `sum()`, `avg()`, `count()`
- **多参数支持**: 支持任意数量的参数

### 随机数生成
- **无参数**: `rand()` - 返回 0-999 的随机数
- **单参数**: `rand(max)` - 返回 0-max 的随机数
- **双参数**: `rand(min, max)` - 返回 min-max 的随机数

### 浮点数支持
- **高精度**: 支持小数点运算
- **混合类型**: 整数和浮点数混合运算

## 📖 使用示例

### 基础运算
```cpp
#include "calculatorutil.h"

// 基础四则运算
qint64 result1 = CalculatorUtil::calcIntExpression("1 + 2 * 3");        // 7
qint64 result2 = CalculatorUtil::calcIntExpression("(1 + 2) * 3");      // 9
qint64 result3 = CalculatorUtil::calcIntExpression("-5 + 3 * (2 - 1)"); // -2
```

### 进制运算
```cpp
// 十六进制
qint64 hex1 = CalculatorUtil::calcIntExpression("0xFF");           // 255
qint64 hex2 = CalculatorUtil::calcIntExpression("0x1A + 0x0F");   // 41

// 二进制
qint64 bin1 = CalculatorUtil::calcIntExpression("0b1010");        // 10
qint64 bin2 = CalculatorUtil::calcIntExpression("0b1100 + 0b0011"); // 15

// 混合进制
qint64 mixed = CalculatorUtil::calcIntExpression("0xFF + 0b1010"); // 265
```

### 科学计数法
```cpp
// 科学计数法
qint64 sci1 = CalculatorUtil::calcIntExpression("1e6");           // 1000000
qint64 sci2 = CalculatorUtil::calcIntExpression("2.5e3");        // 2500
qint64 sci3 = CalculatorUtil::calcIntExpression("1e6 + 2.5e3");  // 1002500
```

### 数学函数
```cpp
// 三角函数 (角度制)
qint64 sin30 = CalculatorUtil::calcIntExpression("sin(30)");      // 500 (0.5 * 1000)
qint64 cos60 = CalculatorUtil::calcIntExpression("cos(60)");      // 500 (0.5 * 1000)
qint64 tan45 = CalculatorUtil::calcIntExpression("tan(45)");      // 1000 (1.0 * 1000)

// 其他数学函数
qint64 abs1 = CalculatorUtil::calcIntExpression("abs(-100)");     // 100
qint64 sqrt1 = CalculatorUtil::calcIntExpression("sqrt(16)");     // 4
qint64 log1 = CalculatorUtil::calcIntExpression("log(100)");      // 2000 (2.0 * 1000)
```

### 统计函数
```cpp
// 统计函数
qint64 min1 = CalculatorUtil::calcIntExpression("min(1, 2, 3)");     // 1
qint64 max1 = CalculatorUtil::calcIntExpression("max(1, 2, 3)");     // 3
qint64 sum1 = CalculatorUtil::calcIntExpression("sum(1, 2, 3)");     // 6
qint64 avg1 = CalculatorUtil::calcIntExpression("avg(1, 2, 3)");     // 2
qint64 count1 = CalculatorUtil::calcIntExpression("count(1, 2, 3)"); // 3
```

### 随机数
```cpp
// 随机数生成
qint64 rand1 = CalculatorUtil::calcIntExpression("rand()");           // 0-999
qint64 rand2 = CalculatorUtil::calcIntExpression("rand(100)");        // 0-100
qint64 rand3 = CalculatorUtil::calcIntExpression("rand(10, 20)");     // 10-20
```

### 复杂表达式
```cpp
// 复杂混合表达式
qint64 complex1 = CalculatorUtil::calcIntExpression(
    "sin(30) + cos(60) * 2"
); // sin(30) + cos(60) * 2 = 0.5 + 0.5 * 2 = 1.5

qint64 complex2 = CalculatorUtil::calcIntExpression(
    "min(1, 2, 3) + max(4, 5, 6) * (0xFF + 0b1010)"
); // 1 + 6 * (255 + 10) = 1 + 6 * 265 = 1591
```

### 浮点数计算
```cpp
// 浮点数计算
double float1 = CalculatorUtil::calcDoubleExpression("1.5 + 2.3");    // 3.8
double float2 = CalculatorUtil::calcDoubleExpression("sin(45.0)");     // 0.707107
double float3 = CalculatorUtil::calcDoubleExpression("sqrt(2.0)");     // 1.41421
```

## 🔧 性能特性

### 缓存机制
- **表达式缓存**: 自动缓存计算结果，重复计算时直接返回缓存结果
- **缓存大小**: 默认缓存1000个表达式
- **缓存清理**: 支持手动清理缓存

### 优化策略
- **手动数字解析**: 避免正则表达式开销，提升数字解析性能
- **静态函数映射**: 函数名查找使用哈希表，O(1)时间复杂度
- **延迟解析**: 只在需要时才解析复杂函数

### 性能对比
| 表达式类型 | 优化前 | 优化后 | 性能提升 |
|-----------|--------|--------|----------|
| 简单数字 | 1x | 1x | 基准 |
| 基础运算 | 1x | 2-3x | 2-3倍 |
| 函数调用 | 1x | 3-5x | 3-5倍 |
| 复杂表达式 | 1x | 5-8x | 5-8倍 |

## 🛠️ 编译和使用

### 依赖要求
- Qt6 Core 模块
- C++17 或更高版本
- CMake 3.16 或更高版本

### 编译步骤
```bash
mkdir build
cd build
cmake ..
make
```

### 运行测试
```bash
./calculator_test
```

## 📝 注意事项

### 精度说明
- **整数函数**: 三角函数等返回整数（实际值 * 1000）
- **浮点函数**: 使用 `calcDoubleExpression()` 获得更高精度
- **科学计数法**: 支持小数点，但整数版本会截断

### 错误处理
- **除零错误**: 自动替换为1，避免程序崩溃
- **语法错误**: 详细的错误提示和警告信息
- **边界情况**: 自动处理括号不匹配等常见问题

### 内存管理
- **缓存管理**: 自动管理缓存内存，避免内存泄漏
- **字符串操作**: 使用引用传递，减少不必要的拷贝

## 🔮 未来扩展

### 计划功能
- [ ] 位运算支持 (`&`, `|`, `^`, `<<`, `>>`)
- [ ] 三元运算符 (`condition ? value1 : value2`)
- [ ] 单位转换 (`1km to m`, `1h to s`)
- [ ] 自然语言支持 (`one plus two`)
- [ ] 更多数学函数 (`exp`, `pow`, `factorial`)

### 性能优化
- [ ] SIMD 指令优化
- [ ] 并行计算支持
- [ ] JIT 编译优化
- [ ] 更智能的缓存策略

## 📄 许可证

本项目遵循原项目的许可证条款。 