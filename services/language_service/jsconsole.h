#ifndef JSCONSOLE_H
#define JSCONSOLE_H

#include <QCoreApplication>
#include <QJSEngine>
#include <QDebug>
#include <QJSValueList>

class JsConsole : public QObject {
    Q_OBJECT
public:
    explicit JsConsole(QJSEngine *engine, QObject *parent = nullptr)
        : QObject(parent), m_engine(engine) {}

    Q_INVOKABLE void log(const QJSValue &value) {
        qDebug().noquote() << "[JS LOG]" << value.toString();
        emit signalLog(value.toString());
    }

    Q_INVOKABLE void logMultiple(const QJSValueList &values) {
        QStringList parts;
        for (const QJSValue &val : values) {
            if (val.isObject()) {
                QJSValue jsonStringify = m_engine->evaluate("JSON.stringify");
                QJSValue result = jsonStringify.call({val});
                parts << (result.isError() ? "[Object]" : result.toString());
            } else {
                parts << val.toString();
            }
        }
        qDebug().noquote() << "[JS LOG]" << parts.join(" ");
        emit signalLog(parts.join(" "));
    }

signals:
    void signalLog(const QString &log);

private:
    QJSEngine *m_engine;
};

#endif // JSCONSOLE_H
