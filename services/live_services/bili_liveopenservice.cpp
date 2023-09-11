#include <QMessageAuthenticationCode>
#include "bili_liveopenservice.h"
#include "accountinfo.h"
#include "fileutil.h"

BiliLiveOpenService::BiliLiveOpenService(QObject *parent) : BiliLiveService(parent)
{
    heartTimer->setInterval(20000);
}

qint64 BiliLiveOpenService::getAppId() const
{
    return BILI_APP_ID;
}

bool BiliLiveOpenService::isPlaying() const
{
    return heartTimer->isActive();
}

/**
 * [已废弃]
 */
void BiliLiveOpenService::startConnectIdentityCode(const QString &code)
{
    /* MyJson json;
    json.insert("code", code); // 主播身份码
    json.insert("app_id", (qint64)BILI_APP_ID);
    post(BILI_API_DOMAIN + "/v2/app/start", json, [=](MyJson json){
        if (json.code() != 0)
        {
            showError("解析身份码出错", snum(json.code()) + " " + json.msg());
            return ;
        }

        auto data = json.data();
        auto anchor = data.o("anchor_info");
        qint64 roomId = anchor.l("room_id");

        ac->roomId = snum(roomId);
        qInfo() << "身份码对应房间号：" << ac->roomId;
        getRoomInfo(true);
    }); */
    apiStart();
}

void BiliLiveOpenService::startConnect()
{
    apiStart();
}

void BiliLiveOpenService::sendVeriPacket(QWebSocket *liveSocket, QString roomId, QString token)
{
    // qInfo() << "开平Socket::connected，发送认证" << authBody;
    QByteArray ba = BiliApiUtil::makePack(authBody, OP_AUTH);
    liveSocket->sendBinaryMessage(ba);
}

void BiliLiveOpenService::sendHeartPacket(QWebSocket *socket)
{
    // 发送长连心跳（30秒一次）
    socket->sendBinaryMessage(BiliApiUtil::makePack(authBody, OP_HEARTBEAT));
    // qInfo() << "互动玩法：Proto心跳发送成功" << authBody;
    return;

    if (!gameId.isEmpty())
    {
        // 保持项目心跳（20秒一次）
        MyJson json;
        json.insert("game_id", gameId);
        post(BILI_API_DOMAIN + "/v2/app/heartbeat", json, [=](MyJson json){
            if (json.code() != 0)
            {
                qCritical() << "互动玩法心跳出错:" << json.code() << json.msg() << gameId;
                if (json.code() == 7003) // 心跳过期或者gameId出错，都没必要继续了
                    heartTimer->stop();
                return ;
            }
            // qInfo() << "互动玩法：心跳发送成功";
        });
    }
}

void BiliLiveOpenService::getDanmuInfo()
{
    startMsgLoop();
    updateExistGuards(0);
    updateOnlineGoldRank();
}

void BiliLiveOpenService::releaseLiveData(bool prepare)
{
    endIfStarted();
    BiliLiveService::releaseLiveData(prepare);
}

void BiliLiveOpenService::apiStart()
{
    if (!isValid())
    {
        qWarning() << "未设置身份码";
        return ;
    }

    MyJson json;
    json.insert("code", ac->identityCode); // 主播身份码
    json.insert("app_id", (qint64)BILI_APP_ID);
    post(BILI_API_DOMAIN + "/v2/app/start", json, [=](MyJson json){
        if (json.code() != 0)
        {
            qCritical() << "互动玩法开启出错：" << json.code() << json.msg();
            showError("连接出错", snum(json.code()) + " " + json.msg());
            emit signalStart(false);
            return ;
        }

        auto data = json.data();
        auto anchor = data.o("anchor_info");
        qint64 roomId = anchor.l("room_id");

        ac->roomId = snum(roomId);
        qInfo() << "身份码对应房间号：" << ac->roomId;

        // 游戏（不一定有）
        auto info = data.o("game_info");
        gameId = info.s("game_id");
        if (gameId.isEmpty())
        {
            qWarning() << "互动玩法 跳过空白的 game id";
            emit signalStart(false);
            return ;
        }
        qInfo() << "互动玩法开始，场次ID：" << gameId;
        heartTimer->start();
        emit signalStart(true);

        // 长链
        auto wsInfo = data.o("websocket_info");
        this->authBody = wsInfo.s("auth_body").toLatin1();
        auto links = wsInfo.a("wss_link");

        hostList.clear();
        for (int i = 0; i < links.size(); i++)
        {
            hostList.append(HostInfo(links.at(i).toString()));
        }

        BiliLiveService::getRoomInfo(true);

        // auto link = links.size() ? links.first().toString() : "";
        // connectWS(link, authBody);
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

void BiliLiveOpenService::apiEnd()
{
    if (gameId.isEmpty())
    {
        qWarning() << "未开启互动玩法，无需结束";
        return ;
    }

    MyJson json;
    json.insert("app_id", (qint64)BILI_APP_ID);
    json.insert("game_id", gameId);
    post(BILI_API_DOMAIN + "/v2/app/end", json, [=](MyJson json){
        if (json.code() != 0)
        {
            qCritical() << "互动玩法关闭失败：" << json.code() << json.msg() << gameId;
            emit signalEnd(false);
            return ;
        }
        gameId = "";
        heartTimer->stop();
        qInfo() << "关闭互动玩法";
        emit signalEnd(true);
    });
}

void BiliLiveOpenService::endIfStarted()
{
    if (isPlaying())
    {
        qInfo() << "结束游戏：" << gameId;
        QEventLoop loop;
        connect(this, SIGNAL(signalEnd(bool)), &loop, SLOT(quit()));
        apiEnd();
        loop.exec();
    }
}

void BiliLiveOpenService::connectWS(const QString &url, const QByteArray &authBody)
{

}

void BiliLiveOpenService::sendWSHeart()
{
    QByteArray ba;
    ba.append("[object Object]");
    ba = BiliApiUtil::makePack(ba, OP_HEARTBEAT);
    liveSocket->sendBinaryMessage(ba);
    LIVE_OPEN_SOCKET_DEB << "互动玩法发送心跳包：" << ba;
}

/**
 * 开平专有的加密格式
 */
void BiliLiveOpenService::post(QString url, MyJson json, NetJsonFunc func)
{
    // 秘钥
    static QStringList sl = readTextFile(":/documents/kk").split("\n");
    static const QByteArray accessKeyId = sl.size() > 0 ? QByteArray::fromBase64(sl.at(0).toLatin1()) : "";
    static const QByteArray accessKeySecret = sl.size() > 1 ? QByteArray::fromBase64(sl.at(1).toLatin1()) : "";

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

bool BiliLiveOpenService::isValid()
{
    if (ac->identityCode.isEmpty())
    {
        emit signalError("主播身份码不能为空");
        return false;
    }
    return true;
}

