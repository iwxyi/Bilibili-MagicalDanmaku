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
        MyJson json = MyJson(message.toUtf8());
        processMessage(json);
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

void KeyuLiveService::processMessage(const MyJson &json)
{
    QString type = json.s("type"); // 启动:"welcome"
    if (type == "welcome")
    {
        qInfo() << "[启动]" << json.s("message");
        ac->roomTitle = "已连接";
        ac->upName = "可遇";
        emit signalRoomInfoChanged();
        emit signalUpFaceChanged(upFace = QPixmap(":/icons/platform/keyu"));
        return;
    }
    
    QString msgType = json.s("msgType"); // "弹幕/礼物/点赞/关注/进房"
    QString newType = json.s("newType"); // 类型对应的图标

    LiveDanmaku danmaku;
    danmaku.with(json);
    danmaku.setUid(json.s("uid"));
    danmaku.setNickname(json.s("name"));
    danmaku.setLevel(json.i("level"));
    danmaku.setWealthLevel(json.i("payGrade"));
    danmaku.setAvatar(json.s("avatar"));
    danmaku.setTime(QDateTime::currentDateTime());
    if (json.contains("msgId"))
        danmaku.setLogId(snum(json.l("msgId")));
    
    QString eventName;
    if (msgType == "弹幕")
    {
        danmaku.setMsgType(MSG_DANMAKU);
        danmaku.setText(json.s("content"));
        eventName = "DANMU_MSG";

        qInfo() << "[弹幕]" << danmaku.getNickname() << ":" << danmaku.getText();
        receiveDanmaku(danmaku);
    }
    else if (msgType == "礼物")
    {
        danmaku.setMsgType(MSG_GIFT);
        // 有些礼物是没有价值的，以及微信礼物没有钱
        qint64 count = json.i("giftCount");
        qint64 diamondCount = json.l("diamondCount");
        danmaku.setGift(json.l("giftId"), json.s("giftName"), count, diamondCount * count);

        qInfo() << "[礼物]" << danmaku.getNickname() << ":" << danmaku.getGiftName() << "x" << danmaku.getNumber() << "(" + snum(danmaku.getTotalCoin()) + ")";
        eventName = "SEND_GIFT";
        receiveGift(danmaku);
        appendLiveGift(danmaku);


        if (count == 0)
            qWarning() << "礼物数量为0：" << json;
    }
    else if (msgType == "点赞")
    {
        danmaku.setMsgType(MSG_LIKE);
        danmaku.setNumber(json.i("count"));

        qInfo() << "[点赞]" << danmaku.getNickname() << "点赞" << danmaku.getNumber();
        eventName = "LIKE";
    }
    else if (msgType == "关注")
    {
        danmaku.setMsgType(MSG_ATTENTION);
        danmaku.setAttention(1); // 0是取消关注
        danmaku.setNumber(json.i("followCount"));

        qInfo() << "[关注]" << danmaku.getNickname() << "关注";
        eventName = "ATTENTION";
        appendNewLiveDanmaku(danmaku);
    }
    else if (msgType == "进房")
    {
        danmaku.setMsgType(MSG_WELCOME);
        int action = json.i("action"); // 进房动作类型
        Q_UNUSED(action);
        danmaku.setNumber(json.i("memberCount"));

        qInfo() << "[进房]" << danmaku.getNickname() << "进入直播间";
        eventName = "WELCOME";
        receiveUserCome(danmaku);
    }
    else if (msgType == "分享")
    {
        danmaku.setMsgType(MSG_SHARE);
        eventName = "SHARE";

        qInfo() << "[分享]" << danmaku.getNickname() << "分享";
        appendNewLiveDanmaku(danmaku);
    }
    else
    {
        qWarning() << "未知消息类型：" << msgType << json;
    }
    
    triggerCmdEvent(eventName, danmaku);
}
