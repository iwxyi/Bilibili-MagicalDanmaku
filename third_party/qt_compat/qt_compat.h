#ifndef QT_COMPAT_H
#define QT_COMPAT_H
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#define TO_BYTE_ARRAY(x) (x).toByteArray()


#else
#define SKIP_EMPTY_PARTS QString::SkipEmptyParts
#define TO_BYTE_ARRAY(x) (x)
#endif

#endif // QT_COMPAT_H
