#ifndef QT_COMPAT_RANDOM_H
#define QT_COMPAT_RANDOM_H

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <random>
static inline int qrand() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return static_cast<int>(gen());
}
static inline void qsrand(uint seed) {
    // Qt6 不需要手动设置种子（随机设备已自动初始化）
}
#else

#endif

#endif // QT_COMPAT_RANDOM_H
