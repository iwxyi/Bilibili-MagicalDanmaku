#ifndef PYTHONENGINE_H
#define PYTHONENGINE_H

#include "languageservicebase.h"

class PythonEngine : public LanguageServiceBase
{
    Q_OBJECT
public:
    explicit PythonEngine(QObject *parent = nullptr);

    QString runCode(const LiveDanmaku &danmaku, const QString& execName, const QString &code);
};

#endif // PYTHONENGINE_H
