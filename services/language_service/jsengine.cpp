#include "jsengine.h"
#include "jsarg.h"
#include <QDebug>

JSEngine::JSEngine(QObject *parent)
    : QObject{parent}
{}

void JSEngine::init()
{
    if (engine) {
        return;
    }
    engine = new QJSEngine(this);
}

QString JSEngine::runCode(const LiveDanmaku &danmaku, const QString &code)
{
    init();

    JSArg *jsArg = new JSArg(danmaku);
    QJSValue danmakuObj = engine->newQObject(jsArg);
    engine->globalObject().setProperty("danmaku", danmakuObj);
    QString codeFrame;
    codeFrame = "function _processDanmaku(danmaku) {\n" + code + "\n}\n\n_processDanmaku(danmaku)";

    QJSValue result = engine->evaluate(codeFrame.toStdString().c_str());
    delete jsArg;

    if (result.isError()) {
        qWarning() << "JS engine error:" << result.toString();
        emit signalError(result.toString());
        return "";
    }
    return result.toString();
}

