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
    , m_dialogView(nullptr)
    , m_dialogContainer(nullptr)
{
    m_qmlEngine = new QQmlEngine(this);
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    
    // 连接信号槽，用于接收QML发送的信号
    connect(this, &QmlEngine::qmlSignal, this, &QmlEngine::handleQmlSignal);
}

QString QmlEngine::runCode(const LiveDanmaku &danmaku, const QString &code)
{
    m_lastResult = "";
    
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
    
    // 创建QML代码模板
    QString qmlCode = code;
    if (!qmlCode.contains("import QtQuick"))
        qmlCode = QString(R"(
import QtQuick 2.12

Item {
    id: root
    
    // 定义信号，用于向C++发送数据
    signal sendSignal(string signalName, var args)
    
    // 连接信号到C++引擎
    Component.onCompleted: {
        sendSignal.connect(function(signalName, args) {
            qmlEngine.qmlSignal(signalName, args)
        })

        // 用户代码
        %1
    }
    
    // 返回结果
    property string result: ""
    
    // 设置结果的函数d
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
    function sendResult(value) {
        result = value
        sendSignal("result", [value])
    }
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
    
    // 设置超时定时器
    m_timeoutTimer->setInterval(30000); // 30秒超时
    m_timeoutTimer->start();
    
    // 等待QML对象完成初始化或超时
    QEventLoop loop;
    connect(m_timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(qmlObject, &QObject::destroyed, &loop, &QEventLoop::quit);
    
    // 等待一小段时间让QML完成初始化
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();
    
    // 停止定时器
    m_timeoutTimer->stop();
    
    // 获取结果
    QString result = qmlObject->property("result").toString();
    
    // 清理
    qmlObject->deleteLater();
    
    return result.isEmpty() ? m_lastResult : result;
}

void QmlEngine::showDialog(const LiveDanmaku &danmaku, const QString &code, QWidget *parent)
{
    // 清理之前的对话框
    if (m_dialogContainer) {
        m_dialogContainer->deleteLater();
        m_dialogContainer = nullptr;
    }
    if (m_dialogView) {
        m_dialogView->deleteLater();
        m_dialogView = nullptr;
    }
    
    // 创建对话框容器
    m_dialogContainer = new QDialog(parent);
    m_dialogContainer->setWindowTitle("QML 弹幕处理");
    m_dialogContainer->setModal(true);
    m_dialogContainer->resize(600, 400);
    
    // 创建QML视图
    m_dialogView = new QQuickView(m_qmlEngine, nullptr);
    m_dialogView->setResizeMode(QQuickView::SizeRootObjectToView);
    
    // 创建QML上下文
    QQmlContext *context = m_dialogView->rootContext();
    
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
    QString qmlCode = R"(
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: window
    visible: true
    width: 600
    height: 400
    title: "弹幕处理界面"
    
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
    function sendResult(value) {
        result = value
        sendSignal("result", [value])
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
        
        // 用户代码
        %1
    }
    
    // 默认内容 - 如果用户代码没有创建UI，则显示默认内容
    default property alias content: mainLayout.children
    
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10
        
        // 默认显示弹幕信息
        Text {
            text: "弹幕信息"
            font.pixelSize: 18
            font.bold: true
        }
        
        Text {
            text: "用户: " + danmaku.getNickname()
            font.pixelSize: 14
        }
        
        Text {
            text: "内容: " + danmaku.getText()
            font.pixelSize: 14
        }
        
        Text {
            text: "等级: " + danmaku.getLevel()
            font.pixelSize: 14
        }
        
        Item {
            Layout.fillHeight: true
        }
        
        RowLayout {
            Layout.fillWidth: true
            
            Item {
                Layout.fillWidth: true
            }
            
            Button {
                text: "确定"
                onClicked: {
                    sendResult("用户点击了确定")
                    closeDialog()
                }
            }
            
            Button {
                text: "取消"
                onClicked: {
                    sendResult("用户点击了取消")
                    closeDialog()
                }
            }
        }
    }
}
)";
    
    // 将用户代码插入到模板中
    QString fullQmlCode = qmlCode.arg(code);
    
    // 创建QML组件
    QQmlComponent component(m_qmlEngine);
    component.setData(fullQmlCode.toUtf8(), QUrl());
    
    if (component.isError()) {
        QString errorMsg = "QML编译错误:\n";
        for (const QQmlError &error : component.errors()) {
            errorMsg += error.toString() + "\n";
        }
        emit signalError(errorMsg);
        return;
    }
    
    // 创建QML对象
    QObject *qmlObject = component.create(context);
    if (!qmlObject) {
        emit signalError("无法创建QML对象");
        return;
    }
    
    // 将QML视图嵌入到对话框中
    QWidget *container = QWidget::createWindowContainer(m_dialogView, m_dialogContainer);
    
    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(m_dialogContainer);
    layout->addWidget(container);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // 连接对话框关闭信号
    connect(m_dialogContainer, &QDialog::finished, this, &QmlEngine::onDialogClosed);
    
    // 居中显示对话框
    if (parent) {
        m_dialogContainer->move(parent->geometry().center() - m_dialogContainer->rect().center());
    } else {
        // 如果没有父窗口，则居中到屏幕
        QScreen *screen = QApplication::primaryScreen();
        if (screen) {
            m_dialogContainer->move(screen->geometry().center() - m_dialogContainer->rect().center());
        }
    }
    
    // 显示对话框
    m_dialogContainer->show();
}

void QmlEngine::onDialogClosed()
{
    if (m_dialogView) {
        QObject *rootObject = m_dialogView->rootObject();
        if (rootObject) {
            QString result = rootObject->property("result").toString();
            emit dialogClosed(result);
        }
    }
    
    // 清理资源
    if (m_dialogContainer) {
        m_dialogContainer->deleteLater();
        m_dialogContainer = nullptr;
    }
    if (m_dialogView) {
        m_dialogView->deleteLater();
        m_dialogView = nullptr;
    }
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
    } else if (signalName == "result") {
        if (!args.isEmpty()) {
            m_lastResult = args.first().toString();
        }
    }
    
    // 可以在这里添加更多信号处理逻辑
}
