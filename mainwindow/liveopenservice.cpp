#include <QMessageAuthenticationCode>
#include "liveopenservice.h"
#include "accountinfo.h"
#include "fileutil.h"

LiveOpenService::LiveOpenService(QObject *parent) : QObject(parent)
{
    heartTimer = new QTimer(this);
    heartTimer->setInterval(20000);
    heartTimer->setSingleShot(false);
    connect(heartTimer, SIGNAL(timeout()), this, SLOT(sendHeart()));
}

qint64 LiveOpenService::getAppId() const
{
    return APP_ID;
}

bool LiveOpenService::isPlaying() const
{
    return heartTimer->isActive();
}

void LiveOpenService::start()
{
    if (!isValid())
        return ;

    MyJson json;
    json.insert("code", ac->identityCode); // 主播身份码
    json.insert("app_id", APP_ID);
    post(API_DOMAIN + "/v2/app/start", json, [=](MyJson json){
        if (json.code() != 0)
        {
            qCritical() << "互动玩法开启出错：" << json.code() <<json.msg();
            return ;
        }

        auto data = json.data();

        // 游戏（不一定有）
        auto info = data.o("game_info");
        gameId = info.s("game_id");
        if (gameId.isEmpty())
        {
            qWarning() << "互动玩法 跳过空白的 game id";
            return ;
        }
        qInfo() << "互动玩法开始，场次ID：" << gameId;
        heartTimer->start();

        // 长链
        /* auto wsInfo = data.o("websocket_info");
        auto authBody = wsInfo.s("auth_body").toLatin1();
        auto links = wsInfo.a("wss_link");
        auto link = links.size() ? links.first().toString() : "";
        connectWS(link, authBody); */
    });

    /* {
        "code": 0,
        "data": {
            "anchor_info": {
                "room_id": 11584296,
                "uface": "http://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg",
                "uid": 20285041,
                "uname": "懒一夕智能科技官方"
            },
            "game_info": {
                "game_id": ""
            },
            "websocket_info": {
                "auth_body": "{\"roomid\":11584296,\"protover\":2,\"uid\":885834200,\"key\":\"7Y_rjOrP7DVqCNi5BGCxnAT-utRVvVNd9s8zMg8rTbqNzFLdxvyONxfdLF2ZtaZ7Plrgg5xMIVtrx7FwMEC\",\"group\":\"open\"}",
                "wss_link": [
                    "wss://hw-sh-live-comet-02.chat.bilibili.com:443/sub",
                    "wss://tx-bj-live-comet-02.chat.bilibili.com:443/sub",
                    "wss://broadcastlv.chat.bilibili.com:443/sub"
                ]
            }
        },
        "message": "0",
        "request_id": "1542075081492774912"
    } */
}

void LiveOpenService::end()
{
    if (gameId.isEmpty())
    {
        qWarning() << "未开启互动玩法，无需结束";
        return ;
    }

    MyJson json;
    json.insert("app_id", APP_ID);
    json.insert("game_id", gameId);
    post(API_DOMAIN + "/v2/app/end", json, [=](MyJson json){
        if (json.code() != 0)
        {
            qCritical() << "互动玩法关闭失败：" << json.code() << json.msg() << gameId;
            return ;
        }
        gameId = "";
        heartTimer->stop();
        qInfo() << "关闭互动玩法";
    });
}

void LiveOpenService::sendHeart()
{
    if (gameId.isEmpty())
    {
        qWarning() << "未开启互动玩法，无法发送心跳";
        heartTimer->stop();
        return ;
    }

    MyJson json;
    json.insert("game_id", gameId);
    post(API_DOMAIN + "/v2/app/heartbeat", json, [=](MyJson json){
        if (json.code() != 0)
        {
            qCritical() << "互动玩法心跳出错:" << json.code() << json.msg() << gameId;
            if (json.code() == 7003) // 心跳过期或者gameId出错，都没必要继续了
                heartTimer->stop();
            return ;
        }
        qInfo() << "互动玩法：心跳发送成功";
    });
}

void LiveOpenService::connectWS(const QString &url, const QByteArray &authBody)
{
    if (!websocket)
    {
        websocket = new QWebSocket;

        connect(websocket, &QWebSocket::connected, this, [=]{
            LIVE_OPEN_SOCKET_DEB << "互动玩法Socket：connected";
            LIVE_OPEN_SOCKET_DEB << "互动玩法Socket：发送认证" << authBody;
            QByteArray ba = BiliApiUtil::makePack(authBody, OP_AUTH);
            websocket->sendBinaryMessage(ba);
        });

        connect(websocket, &QWebSocket::disconnected, this, [=]{
            LIVE_OPEN_SOCKET_DEB << "互动玩法Socket：disconnected";
        });

        connect(websocket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
            LIVE_OPEN_SOCKET_DEB << "互动玩法Socket：接收" << message;
        });
    }

    websocket->open(url);
}

void LiveOpenService::sendWSHeart()
{
    QByteArray ba;
    ba.append("[object Object]");
    ba = BiliApiUtil::makePack(ba, OP_HEARTBEAT);
    websocket->sendBinaryMessage(ba);
    LIVE_OPEN_SOCKET_DEB << "互动玩法发送心跳包：" << ba;
}

void LiveOpenService::post(QString url, MyJson json, NetJsonFunc func)
{
    // 秘钥
    static QStringList sl = readTextFile(":/documents/kk").split("\n");
    static const QByteArray accessKeyId = QByteArray::fromBase64(sl.at(0).toLatin1());
    static const QByteArray accessKeySecret = QByteArray::fromBase64(sl.at(1).toLatin1());

    // 变量
    const QByteArray data = json.toBa();
    const QByteArray contentMd5 = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex().toLower();
    const QByteArray signatureNonceValue = "nonce" + QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    const QByteArray timestamp = QByteArray::number(QDateTime::currentSecsSinceEpoch());

    // 拼接
    const QByteArray pinjie = "x-bili-accesskeyid:" + accessKeyId + "\n" +
            "x-bili-content-md5:" + contentMd5 + "\n" +
            "x-bili-signature-method:HMAC-SHA256" + "\n" +
            "x-bili-signature-nonce:" + signatureNonceValue + "\n" +
            "x-bili-signature-version:1.0" + "\n" +
            "x-bili-timestamp:" + timestamp;

    // 生成签名
    const QByteArray signature = QMessageAuthenticationCode::hash(pinjie, accessKeySecret, QCryptographicHash::Sha256).toHex().toLower();

    // 请求头
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setRawHeader("Accept", "application/json");
    request->setRawHeader("Content-Type", "application/json");
    request->setRawHeader("x-bili-content-md5", contentMd5);
    request->setRawHeader("x-bili-timestamp", timestamp);
    request->setRawHeader("x-bili-signature-method", "HMAC-SHA256");
    request->setRawHeader("x-bili-signature-nonce", signatureNonceValue);
    request->setRawHeader("x-bili-accesskeyid", accessKeyId);
    request->setRawHeader("x-bili-signature-version", "1.0");
    request->setRawHeader("Authorization", signature);

    // 开始联网
    QObject::connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray ba = reply->readAll();
        MyJson json(ba);
        LIVE_OPEN_DEB << "互动玩法POST:" << url << json;
        func(json);

        manager->deleteLater();
        delete request;
        reply->deleteLater();
    });
    manager->post(*request, data);
}

bool LiveOpenService::isValid()
{
    if (ac->identityCode.isEmpty())
    {
        emit signalError("主播身份码不能为空");
        return false;
    }
    return true;
}

