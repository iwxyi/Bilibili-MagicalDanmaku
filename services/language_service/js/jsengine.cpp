#include "jsengine.h"
#include "jsarg.h"
#include <QDebug>

JSEngine::JSEngine(QObject *parent) : LanguageServiceBase{parent}
{}

void JSEngine::init()
{
    if (engine) {
        return;
    }
    engine = new QJSEngine(this);

    // 设置控制台
    console = new JsConsole(engine, this);
    engine->globalObject().setProperty("console", engine->newQObject(console));

    connect(console, &JsConsole::signalLog, this, [this](const QString &log) {
        emit signalLog(log);
    });
    engine->evaluate("function log(value) { console.log(value); return value; }");
    engine->globalObject().setProperty("log", engine->globalObject().property("log"));
}

QString JSEngine::runCode(const LiveDanmaku &danmaku, const QString &code)
{
    init();

    // 注入变量
    JSArg *jsArg = new JSArg(danmaku);
    QJSValue danmakuObj = engine->newQObject(jsArg);
    engine->globalObject().setProperty("danmaku", danmakuObj);

    // 组建function
    QString codeFrame;
    codeFrame = "function _processDanmaku(danmaku) {\n" + code + "\n}\n\n_processDanmaku(danmaku)";

    QJSValue result = engine->evaluate(codeFrame.toStdString().c_str());
    delete jsArg;

    if (result.isError()) {
        qWarning() << "JS engine error:" << result.toString();
        emit signalError(result.toString());
        return "";
    }
    
    // 检查结果是否为 undefined
    if (result.isUndefined()) {
        return "";
    }
    
    return result.toString();
}

