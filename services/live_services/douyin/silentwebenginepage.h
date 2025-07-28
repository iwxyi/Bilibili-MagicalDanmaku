#ifndef SILENTWEBENGINEPAGE_H
#define SILENTWEBENGINEPAGE_H

#ifdef ENABLE_WEBENGINE
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QDebug>

class SilentWebEnginePage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit SilentWebEnginePage(QObject *parent = nullptr) : QWebEnginePage(parent) {}

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceID) override
    {
        // 只输出你关心的内容，或者全部屏蔽
        // 例如：只输出 error 级别
        // if (level == QWebEnginePage::JavaScriptConsoleMessageLevel::ErrorMessageLevel)
        //     qDebug() << "[WebEngine JS Error]" << message;

        // 什么都不做就完全屏蔽了

#ifdef QT_DEBUG
        // Debug模式下输出所有网页JS控制台信息
        // qDebug() << "[WebEngine]" << message << "Line:" << lineNumber << "Source:" << sourceID;
#else
        // Release模式下不输出
        Q_UNUSED(level)
        Q_UNUSED(message)
        Q_UNUSED(lineNumber)
        Q_UNUSED(sourceID)
#endif
    }
};
#endif

#endif // SILENTWEBENGINEPAGE_H
