#ifndef QMLENGINE_H
#define QMLENGINE_H

#include "languageservicebase.h"
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QTimer>
#include <QQuickView>
#include <QDialog>

class QmlEngine : public LanguageServiceBase
{
    Q_OBJECT
public:
    explicit QmlEngine(QObject *parent = nullptr);

    QString runCode(const LiveDanmaku &danmaku, const QString &code);
    void showDialog(const LiveDanmaku &danmaku, const QString &code, QWidget *parent = nullptr);

signals:
    void qmlSignal(const QString &signalName, const QVariantList &args);
    void dialogClosed(const QString &result);

private slots:
    void handleQmlSignal(const QString &signalName, const QVariantList &args);
    void onDialogClosed();

private:
    QQmlEngine *m_qmlEngine;
    QString m_lastResult;
    QTimer *m_timeoutTimer;
    QQuickView *m_dialogView;
    QDialog *m_dialogContainer;
};

#endif // QMLENGINE_H