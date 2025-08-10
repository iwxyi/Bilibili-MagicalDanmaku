#include "douyinsignatureabogus.h"
#include "fileutil.h"

DouyinSignatureAbogus::DouyinSignatureAbogus(QObject *parent)
    : QObject(parent)
    , m_engine(new QJSEngine(this))
{
    initializeEngine();
}

bool DouyinSignatureAbogus::initializeEngine()
{
    try {
        // 加载JavaScript代码
        QString jsCode = readTextFile(":/scripts/douyin_signature", "UTF-8");
        if (jsCode.isEmpty()) {
            qWarning() << "Failed to load JavaScript file";
            return false;
        }

        // 执行JavaScript代码
        QJSValue result = m_engine->evaluate(jsCode);
        if (result.isError()) {
            qWarning() << "JavaScript evaluation error:" << result.toString();
            return false;
        }

        // 获取函数引用
        m_signDetailFunc = m_engine->globalObject().property("sign_datail");
        m_signReplyFunc = m_engine->globalObject().property("sign_reply");

        if (!m_signDetailFunc.isCallable() || !m_signReplyFunc.isCallable()) {
            qWarning() << "Failed to get JavaScript functions";
            return false;
        }

        return true;
    }
    catch (const std::exception &e) {
        qWarning() << "Exception during engine initialization:" << e.what();
        return false;
    }
}

QString DouyinSignatureAbogus::signDetail(const QString &params, const QString &userAgent)
{
    if (!m_signDetailFunc.isCallable()) {
        qWarning() << "sign_datail function is not available";
        return QString();
    }

    try {
        QJSValueList args;
        args << QJSValue(params);
        args << QJSValue(userAgent);

        QJSValue result = m_signDetailFunc.call(args);
        if (result.isError()) {
            qWarning() << "JavaScript execution error:" << result.toString();
            return QString();
        }

        return result.toString();
    }
    catch (const std::exception &e) {
        qWarning() << "Exception during signature generation:" << e.what();
        return QString();
    }
}

QString DouyinSignatureAbogus::signReply(const QString &params, const QString &userAgent)
{
    if (!m_signReplyFunc.isCallable()) {
        qWarning() << "sign_reply function is not available";
        return QString();
    }

    try {
        QJSValueList args;
        args << QJSValue(params);
        args << QJSValue(userAgent);

        QJSValue result = m_signReplyFunc.call(args);
        if (result.isError()) {
            qWarning() << "JavaScript execution error:" << result.toString();
            return QString();
        }

        return result.toString();
    }
    catch (const std::exception &e) {
        qWarning() << "Exception during signature generation:" << e.what();
        return QString();
    }
}
