#ifndef JSENGINE_H
#define JSENGINE_H

#include <QObject>
#include <QJSEngine>
#include "livedanmaku.h"
#include "jsconsole.h"

class JSEngine : public QObject
{
    Q_OBJECT
public:
    explicit JSEngine(QObject *parent = nullptr);

    QString runCode(const LiveDanmaku &danmaku, const QString &code);

private:
    void init();

signals:
    void signalError(const QString& err);
    void signalLog(const QString& log);

private:
    QJSEngine *engine = nullptr;
    JsConsole *console = nullptr;
};

#endif // JSENGINE_H
