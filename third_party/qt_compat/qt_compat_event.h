#ifndef QT_COMPAT_EVENT_H
#define QT_COMPAT_EVENT_H

#include <QtGlobal>
#include <QEvent>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define ENTER_EVENT_TYPE QEvent
#else
#define ENTER_EVENT_TYPE QEnterEvent
#endif

#endif // QT_COMPAT_EVENT_H
