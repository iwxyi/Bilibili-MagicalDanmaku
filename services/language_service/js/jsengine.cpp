#include "jsengine.h"
#include "danmakuwrapper.h"
#include "settingswrapper.h"
#include "usersettings.h"

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
    SettingsWrapper *settingsWrapper = new SettingsWrapper(us);
    engine->globalObject().setProperty("settings", engine->newQObject(settingsWrapper));
    SettingsWrapper *heapsWrapper = new SettingsWrapper(heaps);
    heapsWrapper->setDefaultPrefix("heaps");
    engine->globalObject().setProperty("heaps", engine->newQObject(heapsWrapper));

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
    DanmakuWrapper *danmakuWrapper = new DanmakuWrapper(danmaku);
    QJSValue danmakuObj = engine->newQObject(danmakuWrapper);
    engine->globalObject().setProperty("_danmaku", danmakuObj);

    // 组建function
    QString codeFrame;
    codeFrame = "function _processDanmaku(danmaku) {\n" + code + "\n}\n\n_processDanmaku(_danmaku)";

    // 运行代码
    QJSValue result = engine->evaluate(codeFrame.toStdString().c_str());
    delete danmakuWrapper;

    // 检查错误
    if (result.isError()) {
        qWarning() << "JS engine error:" << result.toString();
        qDebug() << "代码：" << code;
        emit signalError(result.toString());
        return "";
    }
    
    // 检查结果是否为 undefined
    if (result.isUndefined()) {
        return "";
    }
    
    return result.toString();
}

