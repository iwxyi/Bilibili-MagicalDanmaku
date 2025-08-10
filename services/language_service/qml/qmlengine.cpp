#include "qmlengine.h"
#include "danmakuwrapper.h"
#include "settingswrapper.h"
#include "usersettings.h"
#include <QQmlContext>
#include <QQuickView>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QApplication>
#include <QScreen>

QmlEngine::QmlEngine(QObject *parent) 
    : LanguageServiceBase(parent)
    , m_qmlEngine(nullptr)
    , m_timeoutTimer(nullptr)
{
    m_qmlEngine = new QQmlEngine(this);
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    
    // 连接信号槽，用于接收QML发送的信号
    connect(this, &QmlEngine::qmlSignal, this, &QmlEngine::handleQmlSignal);
}

QString QmlEngine::runCode(const LiveDanmaku &danmaku, const QString &code)
{
    // 创建QML引擎
    if (!m_qmlEngine) {
        m_qmlEngine = new QQmlEngine(this);
    }
    
    // 创建QML上下文
    QQmlContext *context = m_qmlEngine->rootContext();
    
    // 创建弹幕包装器并注册到QML上下文
    DanmakuWrapper *danmakuWrapper = new DanmakuWrapper(danmaku, this);
    context->setContextProperty("danmaku", danmakuWrapper);
    
    // 创建设置包装器并注册到QML上下文
    SettingsWrapper *settingsWrapper = new SettingsWrapper(us, this);
    context->setContextProperty("settings", settingsWrapper);
    
    SettingsWrapper *heapsWrapper = new SettingsWrapper(heaps, this);
    heapsWrapper->setDefaultPrefix("heaps");
    context->setContextProperty("heaps", heapsWrapper);
    
    // 注册C++引擎对象到QML上下文，用于接收信号
    context->setContextProperty("qmlEngine", this);
    
    // 创建QML代码模板 - 支持完整的UI组件
    QString qmlCode = code;
    if (!qmlCode.contains("import QtQuick"))
        qmlCode = QString(R"(
import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Shapes 1.12
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0
import Qt.labs.platform 1.1
import QtQuick.LocalStorage 2.0
import Qt.labs.folderlistmodel 2.12
import QtMultimedia 5.12
import QtWebSockets 1.12
import QtQuick.Dialogs 1.3

ApplicationWindow {
    id: window
    visible: true

    // 窗口属性，交由用户来控制
    // width: 600
    // height: 400
    // title: "弹幕处理界面"
    // modality: Qt.ApplicationModal
    
    // 定义信号，用于向C++发送数据
    signal sendSignal(string signalName, var args)
    
    // 返回结果
    property string result: ""
    
    // 设置结果的函数
    function setResult(value) {
        result = value
    }
    
    // 日志函数
    function log(message) {
        sendSignal("log", [message])
    }
    
    // 错误函数
    function error(message) {
        sendSignal("error", [message])
    }
    
    // 发送结果函数
    function sendCmd(value) {
        sendSignal("cmd", [value])
    }
    
    // 关闭对话框函数
    function closeDialog() {
        window.close()
    }
    
    // 连接信号到C++引擎并执行用户代码
    Component.onCompleted: {
        sendSignal.connect(function(signalName, args) {
            qmlEngine.qmlSignal(signalName, args)
        })
    }
    
    // 窗口关闭时发送结果
    onClosing: {
        sendSignal("dialogClosed", [result])
    }
    
    // 用户代码
    %1
}
)").arg(code);
    
    // 创建QML组件
    QQmlComponent component(m_qmlEngine);
    component.setData(qmlCode.toUtf8(), QUrl());
    
    if (component.isError()) {
        QString errorMsg = "QML编译错误:\n";
        for (const QQmlError &error : component.errors()) {
            errorMsg += error.toString() + "\n";
        }
        emit signalError(errorMsg);
        return "";
    }
    
    // 创建QML对象
    QObject *qmlObject = component.create(context);
    if (!qmlObject) {
        emit signalError("无法创建QML对象");
        return "";
    }
    
    // 连接QML对象的信号
    connect(qmlObject, SIGNAL(sendSignal(QString, QVariantList)), 
            this, SLOT(handleQmlSignal(QString, QVariantList)));
    
    // 设置超时定时器
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    } else {
        m_timeoutTimer = new QTimer(this);
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout, this, [this, qmlObject]() {
            if (qmlObject) {
                QMetaObject::invokeMethod(qmlObject, "closeDialog");
            }
        });
    }
    m_timeoutTimer->start(30000); // 30秒超时
    return "";
}

void QmlEngine::handleQmlSignal(const QString &signalName, const QVariantList &args)
{
    qDebug() << "收到QML信号:" << signalName << "参数:" << args;
    
    // 处理不同类型的信号
    if (signalName == "log") {
        if (!args.isEmpty()) {
            emit signalLog(args.first().toString());
        }
    } else if (signalName == "error") {
        if (!args.isEmpty()) {
            emit signalError(args.first().toString());
        }
    } else if (signalName == "cmd") {
        if (!args.isEmpty()) {
            emit signalCmd(args.first().toString());
        }
    } else if (signalName == "dialogClosed") {
        if (!args.isEmpty()) {
            QString result = args.first().toString();
            emit dialogClosed(result);
        }
    }
    
    // 可以在这里添加更多信号处理逻辑
}
