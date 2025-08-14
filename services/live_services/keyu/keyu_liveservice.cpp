#include "keyu_liveservice.h"

KeyuLiveService::KeyuLiveService(QObject* parent) : LiveServiceBase(parent)
{
    initWS();
}

void KeyuLiveService::initWS()
{
    liveSocket->ignoreSslErrors();

    connect(liveSocket, &QWebSocket::connected, this, [=](){
        qInfo() << "连接成功";
    });
    connect(liveSocket, &QWebSocket::disconnected, this, [=](){
        qInfo() << "连接断开";
    });
    connect(liveSocket, &QWebSocket::textMessageReceived, this, [=](QString message){
        qDebug() << "收到消息：" << message;
    });
    connect(liveSocket, &QWebSocket::binaryMessageReceived, this, [=](QByteArray message){
        qDebug() << "收到二进制消息：" << message;
    });
}

void KeyuLiveService::startConnect()
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
    
    qInfo() << "连接：" << ac->roomId;
    liveSocket->open(QUrl(ac->roomId));
}
