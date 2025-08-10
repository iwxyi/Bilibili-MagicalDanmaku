#ifndef QMLENGINE_H
#define QMLENGINE_H

#include "languageservicebase.h"
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QTimer>

class QmlEngine : public LanguageServiceBase
{
    Q_OBJECT
public:
    explicit QmlEngine(QObject *parent = nullptr);

    QString runCode(const LiveDanmaku &danmaku, const QString &code);

signals:
    void qmlSignal(const QString &signalName, const QVariantList &args);
    void dialogClosed(const QString &result);
    void signalCmd(const QString &cmd);

private slots:
    void handleQmlSignal(const QString &signalName, const QVariantList &args);

private:
    QQmlEngine *m_qmlEngine;
    QString m_lastResult;
    QTimer *m_timeoutTimer;
};

#endif // QMLENGINE_H
