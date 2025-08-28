#include "anywebsocketservice.h"

AnyWebSocketService::AnyWebSocketService(QObject* parent) : LiveServiceBase(parent)
{
    initWS();
}

void AnyWebSocketService::initWS()
{
    liveSocket->ignoreSslErrors();

    connect(liveSocket, &QWebSocket::connected, this, [=](){
        qInfo() << "连接成功";
    });
    connect(liveSocket, &QWebSocket::disconnected, this, [=](){
        qInfo() << "连接断开";
    });
    connect(liveSocket, &QWebSocket::textMessageReceived, this, [=](QString message){
        qDebug() << "收到文本消息：" << message;
        processMessage(message);
    });
    connect(liveSocket, &QWebSocket::binaryMessageReceived, this, [=](QByteArray message){
        qDebug() << "收到二进制消息：" << message;
        processMessage(message);
    });
}

void AnyWebSocketService::autoSetProxy()
{
    // 检查系统代理设置
    QNetworkProxyQuery query(QUrl("http://www.google.com"));
    QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(query);
    
    if (!proxies.isEmpty() && proxies.first().type() != QNetworkProxy::NoProxy) {
        qInfo() << "检测到系统代理，应用代理设置";
        QNetworkProxy::setApplicationProxy(proxies.first());
    } else {
        qInfo() << "未检测到系统代理";
    }
}

void AnyWebSocketService::startConnect()
{
    autoSetProxy();
    qInfo() << "连接：" << ac->roomId;
    liveSocket->open(QUrl(ac->roomId));
}

void AnyWebSocketService::processMessage(const QString &message)
{
    MyJson json = MyJson(message.toUtf8());
    LiveDanmaku danmaku;
    danmaku.with(json);
    danmaku.setText(message);
    danmaku.setTime(QDateTime::currentDateTime());
    triggerCmdEvent("WS_MSG", danmaku);
}