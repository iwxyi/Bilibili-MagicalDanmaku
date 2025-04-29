#ifndef LANGUAGESERVICEBASE_H
#define LANGUAGESERVICEBASE_H

#include <QSettings>
#include "livedanmaku.h"

class LanguageServiceBase : public QObject
{
    Q_OBJECT
public:
    explicit LanguageServiceBase(QObject *parent = nullptr) : QObject(parent) {}

    void setHeaps(QSettings* heaps)
    {
        this->heaps = heaps;
    }

signals:
    void signalError(const QString &error);
    void signalLog(const QString &log);

protected:
    QSettings* heaps = nullptr;
};

#endif // LANGUAGESERVICEBASE_H
