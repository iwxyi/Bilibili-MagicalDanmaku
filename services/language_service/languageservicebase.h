#ifndef LANGUAGESERVICEBASE_H
#define LANGUAGESERVICEBASE_H

#include "livedanmaku.h"

class LanguageServiceBase : public QObject
{
    Q_OBJECT
public:
    explicit LanguageServiceBase(QObject *parent = nullptr) : QObject(parent) {}

    virtual QString runCode(const LiveDanmaku &danmaku, const QString &code) = 0;

signals:
    void signalError(const QString &error);
    void signalLog(const QString &log);
};

#endif // LANGUAGESERVICEBASE_H
