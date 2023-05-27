#include <zlib.h>
#include <QFileDialog>
#include <QInputDialog>
#include <QTableView>
#include <QStandardItemModel>
#include <QClipboard>
#include <QLabel>
#include "bili_liveservice.h"
#include "netutil.h"
#include "bili_api_util.h"
#include "stringutil.h"
#include "facilemenu.h"
#include "textinputdialog.h"
#include "fileutil.h"
#include "httpuploader.h"

BiliLiveService::BiliLiveService(QObject *parent) : LiveRoomService(parent)
{
    initWS();

    xliveHeartBeatTimer = new QTimer(this);
    xliveHeartBeatTimer->setInterval(60000);
    connect(xliveHeartBeatTimer, &QTimer::timeout, this, [=]{
        if (isLiving())
            sendXliveHeartBeatX();
    });

    QTimer::singleShot(0, [=]{
        // 获取wbi加密
        getNavInfo([=]{
            if (!ac->cookieUid.isEmpty() && robotFace.isNull())
                getRobotInfo();
            if (!ac->upUid.isEmpty() && upFace.isNull())
                getUpInfo(ac->upUid);
        });
    });
}

void BiliLiveService::readConfig()
{
    LiveRoomService::readConfig();
    todayHeartMinite = us->value("danmaku/todayHeartMinite").toInt();
    emit signalHeartTimeNumberChanged(todayHeartMinite/5, todayHeartMinite);
}

/**
 * 断开连接/下播时清空数据
 * @param prepare 是否是下播
 */
void BiliLiveService::releaseLiveData(bool prepare)
{
    liveTimestamp = QDateTime::currentMSecsSinceEpoch();
    xliveHeartBeatTimer->stop();
}

void BiliLiveService::initWS()
{
    liveSocket = new QWebSocket();

    connect(liveSocket, &QWebSocket::connected, this, [=]{
        SOCKET_DEB << "socket connected";
        emit signalStatusChanged("状态：已连接");

        // 5秒内发送认证包
        sendVeriPacket(liveSocket, ac->roomId, ac->cookieToken);

        // 定时发送心跳包
        heartTimer->start();
        minuteTimer->start();
    });

    connect(liveSocket, &QWebSocket::disconnected, this, [=]{
        // 正在直播的时候突然断开了
        if (isLiving())
        {
            qWarning() << "正在直播的时候突然断开，" << (reconnectWSDuration/1000) << "秒后重连..." << "    " << QDateTime::currentDateTime().toString("HH:mm:ss");
            localNotify("[连接断开，重连...]");
            ac->liveStatus = false;
            // 尝试5秒钟后重连
            // TODO: 每次重连间隔翻倍
            connectServerTimer->setInterval(reconnectWSDuration);
            reconnectWSDuration *= 2;
            if (reconnectWSDuration > INTERVAL_RECONNECT_WS_MAX)
                reconnectWSDuration = INTERVAL_RECONNECT_WS_MAX;
        }

        SOCKET_DEB << "disconnected";
        emit signalStatusChanged("状态：未连接");
        emit signalLiveStatusChanged("");

        heartTimer->stop();
        minuteTimer->stop();

        // 如果不是主动连接的话，这个会断开
        if (!connectServerTimer->isActive())
            connectServerTimer->start();
    });

    connect(liveSocket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
//        qInfo() << "binaryMessageReceived" << message;
//        for (int i = 0; i < 100; i++) // 测试内存泄漏
        try {
            slotBinaryMessageReceived(message);
        } catch (...) {
            qCritical() << "!!!!!!!error:slotBinaryMessageReceived";
        }

    });

    connect(liveSocket, &QWebSocket::textFrameReceived, this, [=](const QString &frame, bool isLastFrame){
        qInfo() << "textFrameReceived";
    });

    connect(liveSocket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
        qInfo() << "textMessageReceived";
    });

    connect(liveSocket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        SOCKET_DEB << "stateChanged" << state;
        QString str = "未知";
        if (state == QAbstractSocket::UnconnectedState)
            str = "未连接";
        else if (state == QAbstractSocket::ConnectingState)
            str = "连接中";
        else if (state == QAbstractSocket::ConnectedState)
            str = "已连接";
        else if (state == QAbstractSocket::BoundState)
            str = "已绑定";
        else if (state == QAbstractSocket::ClosingState)
            str = "断开中";
        else if (state == QAbstractSocket::ListeningState)
            str = "监听中";
        emit signalStatusChanged(str);
    });

    heartTimer = new QTimer(this);
    heartTimer->setInterval(30000);
    connect(heartTimer, &QTimer::timeout, this, [=]{
        if (liveSocket->state() == QAbstractSocket::ConnectedState)
        {
            // 刚刚连接上，发送心跳包
            sendHeartPacket(liveSocket);
        }
        else if (liveSocket->state() == QAbstractSocket::UnconnectedState)
        {
            emit signalStartConnectRoom();
            return;
        }

        /// 发送其他Socket的心跳包
        // PK Socket
        if (pkLiveSocket && pkLiveSocket->state() == QAbstractSocket::ConnectedState)
        {
            sendHeartPacket(pkLiveSocket);
        }

        // 机器人 Socket
        for (int i = 0; i < robots_sockets.size(); i++)
        {
            QWebSocket* socket = robots_sockets.at(i);
            if (socket->state() == QAbstractSocket::ConnectedState)
            {
                sendHeartPacket(socket);
            }
        }
    });
}

/**
 * 通过Cookie获取机器人账号信息
 * 仅获取UID和昵称
 */
void BiliLiveService::getCookieAccount()
{
    if (ac->browserCookie.isEmpty())
        return ;
    gettingUser = true;
    get("https://api.bilibili.com/x/member/web/account", [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            showError("登录返回不为0", json.value("message").toString());
            gettingUser = false;
            if (!gettingRoom)
                triggerCmdEvent("LOGIN_FINISHED", LiveDanmaku());
            emit signalUpdatePermission();
            return ;
        }

        // 获取用户信息
        QJsonObject dataObj = json.value("data").toObject();
        ac->cookieUid = snum(static_cast<qint64>(dataObj.value("mid").toDouble()));
        ac->cookieUname = dataObj.value("uname").toString();
        qInfo() << "当前账号：" << ac->cookieUid << ac->cookieUname;
        
        emit signalRobotAccountChanged();

        if (!wbiMixinKey.isEmpty())
            getRobotInfo();
    });
}

void BiliLiveService::getNavInfo(NetVoidFunc finalFunc)
{
    get("https://api.bilibili.com/x/web-interface/nav", [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            showError("获取导航信息返回不为0", json.value("message").toString());
            return ;
        }
        QJsonObject dataObj = json.value("data").toObject();
        QJsonObject wbi_img_obj = dataObj.value("wbi_img").toObject();
        QString img_url = wbi_img_obj.value("img_url").toString();
        QString sub_url = wbi_img_obj.value("sub_url").toString();
        
        QRegularExpression re("/(\\w{32})\\.png");
        QRegularExpressionMatch match = re.match(img_url);
        QString wbi_img_ba = match.captured(1);
        match = re.match(sub_url);
        QString wbi_sub_ba = match.captured(1);
        QString wbi_img_sub = wbi_img_ba + wbi_sub_ba;
        // qInfo() << "wbi_img_sub:" << wbi_img_sub;

        QList<int> mixinKeyEncTab = {
            46, 47, 18, 2, 53, 8, 23, 32, 15, 50, 10, 31, 58, 3, 45, 35, 27, 43, 5, 49,
            33, 9, 42, 19, 29, 28, 14, 39, 12, 38, 41, 13, 37, 48, 7, 16, 24, 55, 40,
            61, 26, 17, 0, 1, 60, 51, 30, 4, 22, 25, 54, 21, 56, 59, 6, 63, 57, 62, 11,
            36, 20, 34, 44, 52
        };
        QByteArray newString;
        for (int i = 0; i < mixinKeyEncTab.size(); i++)
        {
            newString.append(wbi_img_sub[mixinKeyEncTab[i]].toLatin1());
        }
        this->wbiMixinKey = newString.left(32);
        qInfo() << "wbiMixinKey:" << wbiMixinKey;
    }, [=](const QString&){
        if (finalFunc)
            finalFunc();
    });
}

/**
 * 使用获取到的 wbiCode 对网址参数进行加密
 * @param uparams 按字母排序的url参数，例如：mid=1234&token=&platform=web&web_location=1550101
 * @return
 */
QString BiliLiveService::toWbiParam(QString params) const
{
    if (!params.contains("wts"))
        params += "&wts=" + snum(QDateTime::currentSecsSinceEpoch());
    QString md5 = QCryptographicHash::hash((params + wbiMixinKey).toLocal8Bit(), QCryptographicHash::Md5).toHex().toLower();
    params += "&w_rid=" + md5;
    return params;
}

/**
 * 获取所有用户信息
 * 目前其实只有头像、弹幕字数
 */
void BiliLiveService::getRobotInfo()
{
    QString url = "https://api.bilibili.com/x/space/wbi/acc/info?" + toWbiParam("mid=" + ac->cookieUid + "&platform=web&token=&web_location=1550101");
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取账号信息返回结果不为0：") << json.value("message").toString();
            return ;
        }
        QJsonObject data = json.value("data").toObject();

        // 开始下载头像
        QString face = data.value("face").toString();
        get(face, [=](QNetworkReply* reply){
            QByteArray jpegData = reply->readAll();
            QPixmap pixmap;
            pixmap.loadFromData(jpegData);
            if (pixmap.isNull())
            {
                showError("获取账号头像出错");
                return ;
            }

            // 设置成圆角
            int side = qMin(pixmap.width(), pixmap.height());
            QPixmap p(side, side);
            p.fill(Qt::transparent);
            QPainter painter(&p);
            painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
            QPainterPath path;
            path.addEllipse(0, 0, side, side);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, side, side, pixmap);

            // 设置到Robot头像
            emit signalRobotHeadChanged(p);
        });
    });

    // 检查是否需要用户信息
    if (!us->adjustDanmakuLongest)
        return ;

    // 获取弹幕字数
    url = "https://api.vc.bilibili.com/user_ex/v1/user/detail?uid=" + ac->cookieUid + "&user[]=role&user[]=level&room[]=live_status&room[]=room_link&feed[]=fans_count&feed[]=feed_count&feed[]=is_followed&feed[]=is_following&platform=pc";
    get(url, [=](MyJson json) {
        /*{
            "code": 0,
            "msg": "success",
            "message": "success",
            "data": {
                "user": {
                    "role": 2,
                    "user_level": 19,
                    "master_level": 5,
                    "next_master_level": 6,
                    "need_master_score": 704,
                    "master_rank": 100010,
                    "verify": 0
                },
                "feed": {
                    "fans_count": 7084,
                    "feed_count": 41,
                    "is_followed": 0,
                    "is_following": 0
                },
                "room": {
                    "live_status": 0,
                    "room_id": 11584296,
                    "short_room_id": 0,
                    "title": "用户20285041的直播间",
                    "cover": "",
                    "keyframe": "",
                    "online": 0,
                    "room_link": "http://live.bilibili.com/11584296?src=draw"
                },
                "uid": "20285041"
            }
        }*/
        MyJson data = json.data();
        ac->cookieULevel = data.o("user").i("user_level");
        if (us->adjustDanmakuLongest)
            adjustDanmakuLongest();
    });
}

/**
 * 获取直播间信息，然后再开始连接
 * @param reconnect       是否是重新获取信息
 * @param reconnectCount  连接失败的重连次数
 */
void BiliLiveService::getRoomInfo(bool reconnect, int reconnectCount)
{
    gettingRoom = true;
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByRoom?room_id=" + ac->roomId;
    get(url, [=](QJsonObject json) {
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取房间信息返回结果不为0：") << json.value("message").toString();
            emit signalRoomCoverChanged(QPixmap(":/bg/bg"));
            emit signalConnectionStateTextChanged("连接失败" + snum(reconnectCount+1));

            if (reconnectCount >= 5)
            {
                emit signalConnectionStateTextChanged("无法连接");
                return ;
            }
            qInfo() << "尝试重新获取房间信息：" << (reconnectCount + 1);
            QTimer::singleShot(5000, [=]{
                getRoomInfo(reconnect, reconnectCount + 1);
            });
            return ;
        }

        QJsonObject dataObj = json.value("data").toObject();
        QJsonObject roomInfo = dataObj.value("room_info").toObject();
        QJsonObject anchorInfo = dataObj.value("anchor_info").toObject();

        // 获取房间信息
        ac->roomId = QString::number(roomInfo.value("room_id").toInt()); // 应当一样，但防止是短ID
        ac->shortId = QString::number(roomInfo.value("short_id").toInt());
        ac->upUid = QString::number(static_cast<qint64>(roomInfo.value("uid").toDouble()));
        ac->liveStatus = roomInfo.value("live_status").toInt();
        int pkStatus = roomInfo.value("pk_status").toInt();
        ac->roomTitle = roomInfo.value("title").toString();
        ac->upName = anchorInfo.value("base_info").toObject().value("uname").toString();
        ac->roomDescription = roomInfo.value("description").toString();
        ac->roomTags = roomInfo.value("tags").toString().split(",", QString::SkipEmptyParts);

#ifdef ZUOQI_ENTRANCE
        fakeEntrance->setRoomName(ac->roomTitle);
#endif
        ac->roomNews = dataObj.value("news_info").toObject().value("content").toString();

        qInfo() << "房间信息: roomid=" << ac->roomId
                 << "  shortid=" << ac->shortId
                 << "  upName=" << ac->upName
                 << "  uid=" << ac->upUid;

        // 设置PK状态
        if (pkStatus)
        {
            QJsonObject battleInfo = dataObj.value("battle_info").toObject();
            QString pkId = QString::number(static_cast<qint64>(battleInfo.value("pk_id").toDouble()));
            if (pkId.toLongLong() > 0 && reconnect)
            {
                // 这个 pk_status 不是 battle_type
                pking = true;
                // pkVideo = pkStatus == 2; // 注意：如果是匹配到后、开始前，也算是1/2,
                getPkInfoById(ac->roomId, pkId);
                qInfo() << "正在大乱斗：" << pkId << "   pk_status=" << pkStatus;
            }
        }
        else
        {
            // TODO: 换成两个数据都获取到
            QTimer::singleShot(5000, [=]{ // 延迟5秒，等待主播UID和机器人UID都获取到
                emit signalImUpChanged(ac->cookieUid == ac->upUid);
            });
        }

        // 发送心跳要用到的直播信息
        ac->areaId = snum(roomInfo.value("area_id").toInt());
        ac->areaName = roomInfo.value("area_name").toString();
        ac->parentAreaId = snum(roomInfo.value("parent_area_id").toInt());
        ac->parentAreaName = roomInfo.value("parent_area_name").toString();


        // 疑似在线人数
        online = roomInfo.value("online").toInt();

        // 获取主播信息
        ac->currentFans = anchorInfo.value("relation_info").toObject().value("attention").toInt();
        ac->currentFansClub = anchorInfo.value("medal_info").toObject().value("fansclub").toInt();

        // 获取主播等级
        QJsonObject liveInfo = anchorInfo.value("live_info").toObject();
        ac->anchorLiveLevel = liveInfo.value("level").toInt();
        ac->anchorLiveScore = qint64(liveInfo.value("upgrade_score").toDouble());
        ac->anchorUpgradeScore = qint64(liveInfo.value("score").toDouble());
        // TODO: 显示主播等级和积分

        // 获取热门榜信息
        QJsonObject hotRankInfo = dataObj.value("hot_rank_info").toObject();
        ac->roomRank = hotRankInfo.value("rank").toInt();
        ac->rankArea = hotRankInfo.value("area_name").toString();
        ac->countdown = hotRankInfo.value("countdown").toInt();

        // 获取直播排行榜
        QJsonObject areaRankInfo = dataObj.value("area_rank_info").toObject();
        ac->areaRank = areaRankInfo.value("areaRank").toObject().value("rank").toString();
        ac->liveRank = areaRankInfo.value("liveRank").toObject().value("rank").toString(); // ==anchor_info.live_info.rank

        // 看过
        ac->watchedShow = dataObj.value("watched_show").toObject().value("text_small").toString();

        // 获取大乱斗段位（现在可能是空了）
        QJsonObject battleRankEntryInfo = dataObj.value("battle_rank_entry_info").toObject();
        if (!battleRankEntryInfo.isEmpty())
        {
            ac->battleRankName = battleRankEntryInfo.value("rank_name").toString();
            QString battleRankUrl = battleRankEntryInfo.value("first_rank_img_url").toString(); // 段位图片
            if (!ac->battleRankName.isEmpty())
            {
                get(battleRankUrl, [=](QNetworkReply* reply1){
                    QPixmap pixmap;
                    pixmap.loadFromData(reply1->readAll());
                    emit signalBattleRankIconChanged(pixmap);
                });
                updateWinningStreak(false);
            }
        }
        else // 房间直接读取PK的接口已经更改，需要单独获取
        {
            getRoomBattleInfo();
        }

        // 异步获取房间封面
        getRoomCover(roomInfo.value("cover").toString());

        // 获取主播头像
        if (!wbiMixinKey.isEmpty())
            getUpInfo(ac->upUid);
        gettingRoom = false;
        if (!gettingUser)
            emit signalGetRoomAndRobotFinished();

        // 发送信号
        emit signalRoomIdChanged(ac->roomId);
        emit signalUpUidChanged(ac->upUid);
        emit signalRoomInfoChanged();

        // 判断房间，未开播则暂停连接，等待开播
        if (!isLivingOrMayLiving())
        {
            emit signalConnectionStateTextChanged("等待开播");
            return ;
        }

        // 开始工作
        if (isLiving())
            emit signalStartWork();

        if (!reconnect)
        {
            if (liveSocket)
            {
                if (liveSocket->state() == QAbstractSocket::ConnectedState)
                {
                    emit signalConnectionStateTextChanged("已连接");
                }
                else if (liveSocket->state() == QAbstractSocket::ConnectingState)
                {
                    emit signalConnectionStateTextChanged("连接中");
                }
                else
                {
                    emit signalConnectionStateTextChanged("未连接");
                }
            }
            else
            {
                emit signalConnectionStateTextChanged("未初始化");
            }
            return ;
        }

        // 获取弹幕信息
        getDanmuInfo();

        // 录播
        emit signalCanRecord();

        // 获取礼物
        getGiftList();
    });
    emit signalConnectionStateTextChanged("获取房间信息...");
}

/**
 * 这是真正开始连接的
 * 获取到长链的信息
 */
void BiliLiveService::getDanmuInfo()
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getDanmuInfo?id="+ac->roomId+"&type=0";
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray dataBa = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(dataBa, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << "获取弹幕信息出错：" << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取弹幕信息返回结果不为0：") << json.value("message").toString();
            return ;
        }

        QJsonObject data = json.value("data").toObject();
        ac->cookieToken = data.value("token").toString();
        QJsonArray hostArray = data.value("host_list").toArray();
        hostList.clear();
        foreach (auto val, hostArray)
        {
            QJsonObject o = val.toObject();
            hostList.append(HostInfo{
                                o.value("host").toString(),
                                o.value("port").toInt(),
                                o.value("wss_port").toInt(),
                                o.value("ws_port").toInt(),
                            });
        }
        SOCKET_DEB << s8("getDanmuInfo: host数量=") << hostList.size() << "  token=" << ac->cookieToken;

        startMsgLoop();

        updateExistGuards(0);
        updateOnlineGoldRank();
    });
    manager->get(*request);
    emit signalConnectionStateTextChanged("获取弹幕信息...");
}

/**
 * socket连接到直播间，但不一定已开播
 */
void BiliLiveService::startMsgLoop()
{
    // 开启保存房间弹幕
    if (us->saveDanmakuToFile && !danmuLogFile)
        startSaveDanmakuToFile();

    // 开始连接
    // int hostUseIndex = 0; // 循环遍历可连接的host（意思一下，暂时未使用）
    if (hostUseIndex < 0 || hostUseIndex >= hostList.size())
        hostUseIndex = 0;
    HostInfo hostServer = hostList.at(hostUseIndex);
    us->setValue("live/hostIndex", hostUseIndex);
    hostUseIndex++; // 准备用于尝试下一次的遍历，连接成功后-1

    QString host = QString("wss://%1:%2/sub").arg(hostServer.host).arg(hostServer.wss_port);
    SOCKET_DEB << "hostServer:" << host << "   hostIndex:" << (hostUseIndex-1);

    // 设置安全套接字连接模式（不知道有啥用）
    QSslConfiguration config = liveSocket->sslConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setProtocol(QSsl::TlsV1SslV3);
    liveSocket->setSslConfiguration(config);

    liveSocket->open(host);
}

void BiliLiveService::sendVeriPacket(QWebSocket *socket, QString roomId, QString token)
{
    QByteArray ba;
    ba.append("{\"uid\": 0, \"roomid\": "+roomId+", \"protover\": 2, \"platform\": \"web\", \"clientver\": \"1.14.3\", \"type\": 2, \"key\": \""+token+"\"}");
    ba = BiliApiUtil::makePack(ba, OP_AUTH);
    SOCKET_DEB << "发送认证包：" << ba;
    socket->sendBinaryMessage(ba);
}

/**
 * 每隔半分钟发送一次心跳包
 */
void BiliLiveService::sendHeartPacket(QWebSocket* socket)
{
    static QByteArray ba = BiliApiUtil::makePack(QByteArray("[object Object]"), OP_HEARTBEAT);
//    SOCKET_DEB << "发送心跳包：" << ba;
    socket->sendBinaryMessage(ba);
}

/**
 * 获取用户在当前直播间的信息
 * 会触发进入直播间，就不能偷看了……
 */
void BiliLiveService::getRoomUserInfo()
{
    /*{
        "code": 0,
        "message": "0",
        "ttl": 1,
        "data": {
            "user_level": {
                "level": 13,
                "next_level": 14,
                "color": 6406234,
                "level_rank": ">50000"
            },
            "vip": {
                "vip": 0,
                "vip_time": "0000-00-00 00:00:00",
                "svip": 0,
                "svip_time": "0000-00-00 00:00:00"
            },
            "title": {
                "title": ""
            },
            "badge": {
                "is_room_admin": false
            },
            "privilege": {
                "target_id": 0,
                "privilege_type": 0,
                "privilege_uname_color": "",
                "buy_guard_notice": null,
                "sub_level": 0,
                "notice_status": 1,
                "expired_time": "",
                "auto_renew": 0,
                "renew_remind": null
            },
            "info": {
                "uid": 20285041,
                "uname": "懒一夕智能科技",
                "uface": "http://i1.hdslb.com/bfs/face/97ae8f0f0e09fbc22fa680c4f5ed93f92678c9eb.jpg",
                "main_rank": 10000,
                "bili_vip": 2,
                "mobile_verify": 1,
                "identification": 1
            },
            "property": {
                "uname_color": "",
                "bubble": 0,
                "danmu": {
                    "mode": 1,
                    "color": 16777215,
                    "length": 20,
                    "room_id": 13328782
                },
                "bubble_color": ""
            },
            "recharge": {
                "status": 0,
                "type": 1,
                "value": "",
                "color": "fb7299",
                "config_id": 0
            },
            "relation": {
                "is_followed": false,
                "is_fans": false,
                "is_in_fansclub": false
            },
            "wallet": {
                "gold": 6100,
                "silver": 3294
            },
            "medal": {
                "cnt": 16,
                "is_weared": false,
                "curr_weared": null,
                "up_medal": {
                    "uid": 358629230,
                    "medal_name": "石乐志",
                    "medal_color": 6067854,
                    "level": 1
                }
            },
            "extra_config": {
                "show_bag": false,
                "show_vip_broadcast": true
            },
            "mailbox": {
                "switch_status": 0,
                "red_notice": 0
            },
            "user_reward": {
                "entry_effect": {
                    "id": 0,
                    "privilege_type": 0,
                    "priority": 0,
                    "web_basemap_url": "",
                    "web_effective_time": 0,
                    "web_effect_close": 0,
                    "web_close_time": 0,
                    "copy_writing": "",
                    "copy_color": "",
                    "highlight_color": "",
                    "mock_effect": 0,
                    "business": 0,
                    "face": "",
                    "basemap_url": "",
                    "show_avatar": 0,
                    "effective_time": 0
                },
                "welcome": {
                    "allow_mock": 1
                }
            },
            "shield_info": {
                "shield_user_list": [],
                "keyword_list": [],
                "shield_rules": {
                    "rank": 0,
                    "verify": 0,
                    "level": 0
                }
            },
            "super_chat_message": {
                "list": []
            },
            "lpl_info": {
                "lpl": 0
            },
            "cd": {
                "guide_free_medal_cost": 0,
                "guide_light_medal": 0,
                "guide_follow": 1,
                "guard_compensate": 0,
                "interact_toasts": []
            }
        }
    }*/

    if (ac->browserCookie.isEmpty())
        return ;
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByUser?room_id=" + ac->roomId;
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取房间本用户信息返回结果不为0：") << json.value("message").toString();
            return ;
        }

        // 获取用户在这个房间信息
        QJsonObject info = json.value("data").toObject().value("info").toObject();
        ac->cookieUid = snum(static_cast<qint64>(info.value("uid").toDouble()));
        ac->cookieUname = info.value("uname").toString();
        QString uface = info.value("uface").toString(); // 头像地址
        qInfo() << "当前cookie用户：" << ac->cookieUid << ac->cookieUname;
    });
}

void BiliLiveService::getRoomCover(const QString &url)
{
    get(url, [=](QNetworkReply* reply){
        QPixmap pixmap;
        pixmap.loadFromData(reply->readAll());
        emit signalRoomCoverChanged(pixmap);
    });
}

void BiliLiveService::getUpInfo(const QString &uid)
{
    QString url = "https://api.bilibili.com/x/space/wbi/acc/info?" + toWbiParam("mid=" + uid + "&platform=web&token=&web_location=1550101");
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取主播信息返回结果不为0：") << json.value("message").toString();
            return ;
        }
        QJsonObject data = json.value("data").toObject();

        // 设置签名
        QString sign = data.value("sign").toString();
        {
            // 删除首尾空
            sign.replace(QRegularExpression("\n[\n\r ]*\n"), "\n")
                    .replace(QRegularExpression("^[\n\r ]+"), "")
                    .replace(QRegularExpression("[\n\r ]+$"), "");
            emit signalUpSignatureChanged(sign.trimmed());
        }

        // 开始下载头像
        QString faceUrl = data.value("face").toString();
        get(faceUrl, [=](QNetworkReply* reply){
            QPixmap pixmap;
            pixmap.loadFromData(reply->readAll());
            upFace = pixmap;
            emit signalUpFaceChanged(pixmap);
        });
    });
}

void BiliLiveService::updateExistGuards(int page)
{
    if (page == 0) // 表示是入口
    {
        if (updateGuarding)
            return ;

        page = 1;
        ac->currentGuards.clear();
        guardInfos.clear();
        updateGuarding = true;

        // 参数是0的话，自动判断是否需要
        if (ac->browserCookie.isEmpty())
            return ;
    }

    const int pageSize = 29;

    auto judgeGuard = [=](QJsonObject user){
        /*{
            "face": "http://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg",
            "guard_level": 3,
            "guard_sub_level": 0,
            "is_alive": 1,
            "medal_info": {
                "medal_color_border": 6809855,
                "medal_color_end": 5414290,
                "medal_color_start": 1725515,
                "medal_level": 23,
                "medal_name": "翊中人"
            },
            "rank": 71,
            "ruid": 5988102,
            "uid": 20285041,
            "username": "懒一夕智能科技"
        }*/
        QString username = user.value("username").toString();
        qint64 uid = static_cast<qint64>(user.value("uid").toDouble());
        int guardLevel = user.value("guard_level").toInt();
        guardInfos.append(LiveDanmaku(guardLevel, username, uid, QDateTime::currentDateTime()));
        ac->currentGuards[uid] = username;

        // 自己是自己的舰长
        if (uid == ac->cookieUid.toLongLong())
        {
            ac->cookieGuardLevel = guardLevel;
            emit signalAutoAdjustDanmakuLongest();
        }

        int count = us->danmakuCounts->value("guard/" + snum(uid), 0).toInt();
        if (!count)
        {
            int count = 1;
            if (guardLevel == 3)
                count = 1;
            else if (guardLevel == 2)
                count = 10;
            else if (guardLevel == 1)
                count = 100;
            else
                qWarning() << "错误舰长等级：" << username << uid << guardLevel;
            us->danmakuCounts->setValue("guard/" + snum(uid), count);
            // qInfo() << "设置舰长：" << username << uid << count;
        }
    };

    QString _upUid = ac->upUid;
    QString url = "https://api.live.bilibili.com/xlive/app-room/v2/guardTab/topList?roomid="
            +ac->roomId+"&page="+snum(page)+"&ruid="+ac->upUid+"&page_size="+snum(pageSize);
    get(url, [=](QJsonObject json){
        if (_upUid != ac->upUid)
        {
            updateGuarding = false;
            return ;
        }

        QJsonObject data = json.value("data").toObject();

        if (page == 1)
        {
            QJsonArray top3 = data.value("top3").toArray();
            foreach (QJsonValue val, top3)
            {
                judgeGuard(val.toObject());
            }
        }

        QJsonArray list = data.value("list").toArray();
        foreach (QJsonValue val, list)
        {
            judgeGuard(val.toObject());
        }

        // 下一页
        QJsonObject info = data.value("info").toObject();
        int num = info.value("num").toInt();
        if (page * pageSize + 3 < num)
            updateExistGuards(page + 1);
        else // 全部结束了
        {
            emit signalGuardsChanged();
        }
    });
}

/**
 * 有新上船后调用（不一定是第一次，可能是掉船了）
 * @param danmaku 发送信号的参数
 */
void BiliLiveService::getGuardCount(const LiveDanmaku &danmaku)
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByRoom?room_id=" + ac->roomId;
    get(url, [=](MyJson json) {
        int count = json.data().o("guard_info").i("count");
        LiveDanmaku ld = danmaku;
        ld.setNumber(count);
        triggerCmdEvent("NEW_GUARD_COUNT", ld.with(json.data()), true);
    });
}
/**
 * 获取高能榜上的用户（仅取第一页就行了）
 */
void BiliLiveService::updateOnlineGoldRank()
{
    /*{
        "code": 0,
        "message": "0",
        "ttl": 1,
        "data": {
            "onlineNum": 12,
            "OnlineRankItem": [
                {
                    "userRank": 1,
                    "uid": 8160635,
                    "name": "嘻嘻家の第二帅",
                    "face": "http://i2.hdslb.com/bfs/face/494fcc986807a944b79a027559d964c8b6b3addb.jpg",
                    "score": 3300,
                    "medalInfo": null,
                    "guard_level": 2
                },
                {
                    "userRank": 2,
                    "uid": 1274248,
                    "name": "贪睡的熊猫",
                    "face": "http://i1.hdslb.com/bfs/face/6241c9080e98a8988a3acc2df146236bad897be3.gif",
                    "score": 1782,
                    "medalInfo": {
                        "guardLevel": 3,
                        "medalColorStart": 1725515,
                        "medalColorEnd": 5414290,
                        "medalColorBorder": 6809855,
                        "medalName": "戒不掉",
                        "level": 21,
                        "targetId": 300702024,
                        "isLight": 1
                    },
                    "guard_level": 3
                },
                ...剩下10个...
            ],
            "ownInfo": {
                "uid": 20285041,
                "name": "懒一夕智能科技",
                "face": "http://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg",
                "rank": -1,
                "needScore": 1,
                "score": 0,
                "guard_level": 0
            }
        }
    }*/
    QString _upUid = ac->upUid;
    QString url = "https://api.live.bilibili.com/xlive/general-interface/v1/rank/getOnlineGoldRank?roomId="
            +ac->roomId+"&page="+snum(1)+"&ruid="+ac->upUid+"&pageSize="+snum(50);
    onlineGoldRank.clear();
    get(url, [=](QJsonObject json){
        if (_upUid != ac->upUid)
        {
            qWarning() << "已切换直播间，忽略高能榜结果";
            return ;
        }
        QStringList names;
        QJsonObject data = json.value("data").toObject();
        QJsonArray array = data.value("OnlineRankItem").toArray();
        foreach (auto val, array)
        {
            QJsonObject item = val.toObject();
            qint64 uid = qint64(item.value("uid").toDouble());
            QString name = item.value("name").toString();
            int guard_level = item.value("guard_level").toInt(); // 没戴牌子也会算进去
            int score = item.value("score").toInt(); // 金瓜子数量
            int rank = item.value("userRank").toInt(); // 1,2,3...

            names.append(name + " " + snum(score));
            LiveDanmaku danmaku(name, uid, QDateTime(), false,
                                "", "", "");
            danmaku.setFirst(rank);
            danmaku.setTotalCoin(score);
            danmaku.extraJson = item;

            if (guard_level)
                danmaku.setGuardLevel(guard_level);

            if (!item.value("medalInfo").isNull())
            {
                QJsonObject medalInfo = item.value("medalInfo").toObject();

                QString anchorId = snum(qint64(medalInfo.value("targetId").toDouble()));
                if (medalInfo.contains("guardLevel") && anchorId == ac->roomId)
                    danmaku.setGuardLevel(medalInfo.value("guardLevel").toInt());

                qint64 medalColor = qint64(medalInfo.value("medalColorStart").toDouble());
                QString cs = QString::number(medalColor, 16);
                while (cs.size() < 6)
                    cs = "0" + cs;

                danmaku.setMedal(anchorId,
                                 medalInfo.value("medalName").toString(),
                                 medalInfo.value("level").toInt(),
                                 "",
                                 "");
            }

            onlineGoldRank.append(danmaku);
        }
        // qInfo() << "高能榜：" << names;
        emit signalOnlineRankChanged();
    });
}

void BiliLiveService::getPkOnlineGuardPage(int page)
{
    static int guard1 = 0, guard2 = 0, guard3 = 0;
    if (page == 0)
    {
        page = 1;
        guard1 = guard2 = guard3 = 0;
    }

    if (pkUid.isEmpty())
    {
        return;
    }

    auto addCount = [=](MyJson user) {
        int alive = user.i("is_alive");
        if (!alive)
            return ;
        QString username = user.s("username");
        int guard_level = user.i("guard_level");
        if (!alive)
            return ;
        if (guard_level == 1)
            guard1++;
        else if (guard_level == 2)
            guard2++;
        else
            guard3++;
    };

    QString url = "https://api.live.bilibili.com/xlive/app-room/v2/guardTab/topList?actionKey=appkey&appkey=27eb53fc9058f8c3&roomid=" + pkRoomId
            +"&page=" + QString::number(page) + "&ruid=" + pkUid + "&page_size=30";
    get(url, [=](MyJson json){
        MyJson data = json.data();
        // top3
        if (page == 1)
        {
            QJsonArray top3 = data.value("top3").toArray();
            foreach (QJsonValue val, top3)
                addCount(val.toObject());
        }

        // list
        QJsonArray list = data.value("list").toArray();
        foreach (QJsonValue val, list)
            addCount(val.toObject());

        // 下一页
        QJsonObject info = data.value("info").toObject();
        int page = info.value("page").toInt();
        int now = info.value("now").toInt();
        if (now < page)
            getPkOnlineGuardPage(now+1);
        else // 全部完成了
        {
            qInfo() << "舰长数量：" << guard1 << guard2 << guard3;
            LiveDanmaku danmaku;
            danmaku.setNumber(guard1 + guard2 + guard3);
            danmaku.extraJson.insert("guard1", guard1);
            danmaku.extraJson.insert("guard2", guard2);
            danmaku.extraJson.insert("guard3", guard3);
            triggerCmdEvent("PK_MATCH_ONLINE_GUARD", danmaku, true);
        }
    });
}

void BiliLiveService::getGiftList()
{
    get("https://api.live.bilibili.com/xlive/web-room/v1/giftPanel/giftConfig?platform=pc&room_id=" + ac->roomId,
        [=](MyJson json)
        {
            if (json.code() != 0)
            {
                qWarning() << json.err();
                return;
            }

            pl->allGiftMap.clear();
            auto list = json.data().a("list");
            for (QJsonValue val : list)
            {
                MyJson info = val.toObject();
                int id = info.i("id");
                int bag = info.i("bag_gift");
                if (!bag)
                    continue;
                QString name = info.s("name");
                QString coinType = info.s("coin_type");
                int coin = info.i("price");
                QString desc = info.s("desc");
                QString img = info.s("img_basic");

                LiveDanmaku gift("", id, name, 1, 0, QDateTime(), coinType, coin);
                gift.setFaceUrl(img);
                gift.with(info);
                pl->allGiftMap[id] = gift;
            }
            // qInfo() << "直播间礼物数量：" << pl->allGiftMap.size();
        });
}

void BiliLiveService::getEmoticonList()
{
    get("https://api.live.bilibili.com/xlive/web-ucenter/v1/emoticon/GetEmoticonGuide",
        {"room_id", ac->roomId}, [=](MyJson json)
        {
        MyJson data = json.data();
        QJsonArray array = data.a("data");
        for (int i = 0; i < array.size(); i++)
        {
            MyJson     emoObj   = array.at(i).toObject();
            QJsonArray emotions = emoObj.a("emoticons");
            QString    name     = emoObj.s("pkg_name");      // 通用表情
            QString    descript = emoObj.s("pkg_descript");  // "官方表情(系统)"  "房间专属表情"
            qint64     pkg_id   = emoObj.l("pkg_id");        // 1  109028
            int        pkg_type = emoObj.i("pkg_type");      // 1  2

            for (int j = 0; j < emotions.size(); j++)
            {
                MyJson o = emotions.at(j).toObject();
                Emoticon e;
                e.name        = o.s("emoji");            // 赞
                e.description = o.s("descript");         // 可空
                e.id          = o.l("emoticon_id");      // 147
                e.unique      = o.s("emoticon_unique");  // official_147
                e.identity    = o.i("identity");
                e.width       = o.i("width");
                e.height      = o.i("height");
                e.id_dynamic  = o.i("id_dynamic");
                e.url         = o.s("url");

                e.pkg_type = pkg_type;
                e.pkg_id = pkg_id;

                pl->emoticons.insert(e.unique, e);
            }
        } });
}

void BiliLiveService::sendGift(int giftId, int giftNum)
{
    if (ac->roomId.isEmpty() || ac->browserCookie.isEmpty())
    {
        qWarning() << "房间为空，或未登录";
        return ;
    }

    if (us->localMode)
    {
        localNotify("赠送礼物 -> " + snum(giftId) + " x " + snum(giftNum));
        return ;
    }


    // 设置数据（JSON的ByteArray）
    QStringList datas;
    datas << "uid=" + ac->cookieUid;
    datas << "gift_id=" + snum(giftId);
    datas << "ruid=" + ac->upUid;
    datas << "send_ruid=0";
    datas << "gift_num=" + snum(giftNum);
    datas << "coin_type=gold";
    datas << "bag_id=0";
    datas << "platform=pc";
    datas << "biz_code=live";
    datas << "biz_id=" + ac->roomId;
    datas << "rnd=" + snum(QDateTime::currentSecsSinceEpoch());
    datas << "storm_beat_id=0";
    datas << "metadata=";
    datas << "price=0";
    datas << "csrf_token=" + ac->csrf_token;
    datas << "csrf=" + ac->csrf_token;
    datas << "visit_id=";

    QByteArray ba(datas.join("&").toStdString().data());

    QString url("https://api.live.bilibili.com/gift/v2/Live/send");
    post(url, ba, [=](QJsonObject json){
        QString message = json.value("message").toString();
        emit signalStatusChanged("");
        if (message != "success")
        {
            showError("赠送礼物", message);
        }
    });
}

void BiliLiveService::sendBagGift(int giftId, int giftNum, qint64 bagId)
{
    if (ac->roomId.isEmpty() || ac->browserCookie.isEmpty())
    {
        qWarning() << "房间为空，或未登录";
        return ;
    }

    if (us->localMode)
    {
        localNotify("赠送包裹礼物 -> " + snum(giftId) + " x " + snum(giftNum));
        return ;
    }

    // 设置数据（JSON的ByteArray）
    QStringList datas;
    datas << "uid=" + ac->cookieUid;
    datas << "gift_id=" + snum(giftId);
    datas << "ruid=" + ac->upUid;
    datas << "send_ruid=0";
    datas << "gift_num=" + snum(giftNum);
    datas << "bag_id=" + snum(bagId);
    datas << "platform=pc";
    datas << "biz_code=live";
    datas << "biz_id=" + ac->roomId;
    datas << "rnd=" + snum(QDateTime::currentSecsSinceEpoch());
    datas << "storm_beat_id=0";
    datas << "metadata=";
    datas << "price=0";
    datas << "csrf_token=" + ac->csrf_token;
    datas << "csrf=" + ac->csrf_token;
    datas << "visit_id=";

    QByteArray ba(datas.join("&").toStdString().data());

    QString url("https://api.live.bilibili.com/gift/v2/live/bag_send");
    post(url, ba, [=](QJsonObject json){
        QString message = json.value("message").toString();
        emit signalStatusChanged("");
        if (message != "success")
        {
            showError("赠送包裹礼物失败", message);
            qCritical() << s8("warning: 发送失败：") << message << datas.join("&");
        }
    });
}

void BiliLiveService::doSign()
{
    if (ac->csrf_token.isEmpty())
    {
        emit signalSignInfoChanged("机器人账号未登录");
        QTimer::singleShot(10000, [=]{
            emit signalSignInfoChanged("每日自动签到");
        });
        return ;
    }

    QString url("https://api.live.bilibili.com/xlive/web-ucenter/v1/sign/DoSign");

    // 建立对象
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            QString msg = json.value("message").toString();
            qCritical() << s8("自动签到返回结果不为0：") << msg;
            emit signalSignInfoChanged(msg);
        }
        else
        {
            emit signalSignInfoChanged("签到成功");
            emit signalSignDescChanged("最近签到时间：" + QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss"));
        }
        QTimer::singleShot(10000, [=]{
            emit signalSignInfoChanged("每日自动签到");
        });
    });
}

void BiliLiveService::joinLOT(qint64 id, bool follow)
{
    if (!id)
        return ;
    if (ac->csrf_token.isEmpty())
    {
        emit signalLOTInfoChanged("机器人账号未登录");
        QTimer::singleShot(10000, [=]{
            emit signalLOTInfoChanged("自动参与活动");
        });
        return ;
    }

    QString url("https://api.live.bilibili.com/xlive/lottery-interface/v1/Anchor/Join"
             "?id="+QString::number(id)+(follow?"&follow=true":"")+"&platform=pc&csrf_token="+ac->csrf_token+"&csrf="+ac->csrf_token+"&visit_id=");
    qInfo() << "参与天选：" << id << follow << url;

    post(url, QByteArray(), [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            QString msg = json.value("message").toString();
            qCritical() << s8("参加天选返回结果不为0：") << msg;
            emit signalLOTInfoChanged(msg);
            emit signalLOTDescChanged(msg);
        }
        else
        {
            emit signalLOTInfoChanged("参与成功");
            qInfo() << "参与天选成功！";
            emit signalLOTDescChanged("最近参与时间：" + QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss"));
        }
        QTimer::singleShot(10000, [=]{
            emit signalLOTInfoChanged("自动参与活动");
        });
    });
}

void BiliLiveService::joinStorm(qint64 id)
{
    if (!id)
        return ;
    if (ac->csrf_token.isEmpty())
    {
        emit signalLOTInfoChanged("机器人账号未登录");
        QTimer::singleShot(10000, [=]{
            emit signalLOTInfoChanged("自动参与活动");
        });
        return ;
    }

    QString url("https://api.live.bilibili.com/xlive/lottery-interface/v1/storm/Join"
             "?id="+QString::number(id)+"&color=5566168&csrf_token="+ac->csrf_token+"&csrf="+ac->csrf_token+"&visit_id=");
    qInfo() << "参与节奏风暴：" << id << url;

    post(url, QByteArray(), [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            QString msg = json.value("message").toString();
            qCritical() << s8("参加节奏风暴返回结果不为0：") << msg;
            emit signalLOTInfoChanged(msg);
            emit signalLOTDescChanged(msg);
        }
        else
        {
            emit signalLOTInfoChanged("参与成功");
            qInfo() << "参与节奏风暴成功！";
            QString content = json.value("data").toObject().value("content").toString();
            emit signalLOTDescChanged("最近参与时间：" +
                                            QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss")
                                            + "\n\n" + content);
        }
        QTimer::singleShot(10000, [=]{
            emit signalLOTInfoChanged("自动参与活动");
        });
    });
}

void BiliLiveService::openUserSpacePage(QString uid)
{
    if (uid.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl("https://space.bilibili.com/" + uid));
}

void BiliLiveService::openLiveRoomPage(QString roomId)
{
    if (roomId.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl("https://live.bilibili.com/" + roomId));
}

void BiliLiveService::openAreaRankPage(QString areaId, QString parentAreaId)
{
    if (areaId.isEmpty() || parentAreaId.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl("https://live.bilibili.com/p/eden/area-tags?areaId=" + areaId + "&parentAreaId=" + parentAreaId));
}

void BiliLiveService::switchMedalToUp(QString upId, int page)
{
    qInfo() << "自动切换勋章：upId=" << upId;
    QString url = "https://api.live.bilibili.com/fans_medal/v1/fans_medal/get_home_medals?uid=" + ac->cookieUid + "&source=2&need_rank=false&master_status=0&page=" + snum(page);
    get(url, [=](MyJson json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("切换勋章返回结果不为0：") << json.value("message").toString();
            return ;
        }

        // 获取用户信息
        MyJson data = json.data();
        JI(data, cnt); Q_UNUSED(cnt) // 粉丝勋章个数
        JI(data, max); Q_UNUSED(max) // 1000
        JI(data, curr_page);
        JI(data, total_page);
        JA(data, list);

        /*  can_delete: true
            day_limit: 250000
            guard_level: 0
            guard_type: 0
            icon_code: 0
            icon_text: ""
            intimacy: 1380
            is_lighted: 1
            is_receive: 1
            last_wear_time: 1625119143
            level: 21
            live_stream_status: 0
            lpl_status: 0
            master_available: 1
            master_status: 0
            medal_color: 1725515
            medal_color_border: 1725515
            medal_color_end: 5414290
            medal_color_start: 1725515
            medal_id: 373753
            medal_level: 21
            medal_name: "181mm"
            next_intimacy: 2000
            rank: "-"
            receive_channel: 4
            receive_time: "2021-01-10 09:33:22"
            score: 50001380
            source: 1
            status: 0
            target_face: "https://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg"
            target_id: 20285041
            target_name: "懒一夕智能科技官方"
            today_feed: 0
            today_intimacy: 0
            uid: 20285041  */
        foreach (QJsonValue val, list)
        {
            MyJson medal = val.toObject();
            JI(medal, status); // 1佩戴，0未佩戴
            JL(medal, target_id); // 主播

            if (snum(target_id) == upId)
            {
                if (status) // 已佩戴，就不用管了
                {
                    qInfo() << "已佩戴当前直播间的粉丝勋章";
                    return ;
                }

                // 佩带牌子
                /*int isLighted = medal.value("is_lighted").toBool(); // 1能用，0变灰
                if (!isLighted) // 牌子是灰的，可以不用管，发个弹幕就点亮了
                    return ;
                */

                qint64 medalId = static_cast<qint64>(medal.value("medal_id").toDouble());
                wearMedal(medalId);
                return ;
            }
        }

        // 未检测到勋章
        if (curr_page >= total_page) // 已经是最后一页了
        {
            qInfo() << "未检测到勋章，无法佩戴";
        }
        else // 没有勋章
        {
            switchMedalToUp(upId, page+1);
        }
    });
}

void BiliLiveService::wearMedal(qint64 medalId)
{
    qInfo() << "佩戴勋章：medalId=" << medalId;
    QString url("https://api.live.bilibili.com/xlive/web-room/v1/fansMedal/wear");
    QStringList datas;
    datas << "medal_id=" + QString::number(medalId);
    datas << "csrf_token=" + ac->csrf_token;
    datas << "csrf=" + ac->csrf_token;
    datas << "visit_id=";
    QByteArray ba(datas.join("&").toStdString().data());

    // 连接槽
    post(url, ba, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("戴勋章返回结果不为0：") << json.value("message").toString();
            return ;
        }
        qInfo() << "佩戴主播粉丝勋章成功";
    });
}

void BiliLiveService::sendPrivateMsg(QString uid, QString msg)
{
    if (ac->csrf_token.isEmpty())
    {
        return ;
    }

    QString url("https://api.vc.bilibili.com/web_im/v1/web_im/send_msg");

    QStringList params;
    params << "msg%5Bsender_uid%5D=" + ac->cookieUid;
    params << "msg%5Breceiver_id%5D=" + uid;
    params << "msg%5Breceiver_type%5D=1";
    params << "msg%5Bmsg_type%5D=1";
    params << "msg%5Bmsg_status%5D=0";
    params << "msg%5Bcontent%5D=" + QUrl::toPercentEncoding("{\"content\":\"" + msg.replace("\n", "\\n") + "\"}");
    params << "msg%5Btimestamp%5D="+snum(QDateTime::currentSecsSinceEpoch());
    params << "msg%5Bnew_face_version%5D=0";
    params << "msg%5Bdev_id%5D=81872DC0-FBC0-4CF8-8E93-093DE2083F51";
    params << "from_firework=0";
    params << "build=0";
    params << "mobi_app=web";
    params << "csrf_token=" + ac->csrf_token;
    params << "csrf=" + ac->csrf_token;
    QByteArray ba(params.join("&").toStdString().data());

    // 连接槽
    post(url, ba, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            QString msg = json.value("message").toString();
            qCritical() << s8("发送消息出错，返回结果不为0：") << msg;
            return ;
        }
    });
}

void BiliLiveService::joinBattle(int type)
{
    if (!isLiving() || ac->cookieUid != ac->upUid)
    {
        showError("大乱斗", "未开播或不是主播本人");
        return ;
    }

    QStringList params{
        "room_id", ac->roomId,
        "platform", "pc",
        "battle_type", snum(type),
        "csrf_token", ac->csrf_token,
        "csrf", ac->csrf_token
    };
    post("https://api.live.bilibili.com/av/v1/Battle/join", params, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
            showError("开启大乱斗", json.value("message").toString());
        else
            emit signalBattleStartMatch();
        qInfo() << "匹配大乱斗" << json;
    });
}

/**
 * 监听勋章升级
 * 一个小问题：如果用户一点一点的点击送礼物，那么升级那段时间获取到的亲密度刚好在送礼物边缘
 * 可能会多播报几次，或者压根就不播报
 */
void BiliLiveService::detectMedalUpgrade(LiveDanmaku danmaku)
{
    /* {
        "code": 0,
        "msg": "",
        "message": "",
        "data": {
            "guard_type": 3,
            "intimacy": 2672,
            "is_receive": 1,
            "last_wear_time": 1616941910,
            "level": 23,
            "lpl_status": 0,
            "master_available": 1,
            "master_status": 0,
            "medal_id": 37075,
            "medal_name": "蘑菇云",
            "receive_channel": 30726000,
            "receive_time": "2020-12-11 21:41:39",
            "score": 50007172,
            "source": 1,
            "status": 0,
            "target_id": 13908357,
            "today_intimacy": 4,
            "uid": 20285041,
            "target_name": "娇羞的蘑菇",
            "target_face": "https://i1.hdslb.com/bfs/face/180d0e87a0e88cb6c04ce6504c3f04003dd77392.jpg",
            "live_stream_status": 0,
            "icon_code": 0,
            "icon_text": "",
            "rank": "-",
            "medal_color": 1725515,
            "medal_color_start": 1725515,
            "medal_color_end": 5414290,
            "guard_level": 3,
            "medal_color_border": 6809855,
            "is_lighted": 1,
            "today_feed": 4,
            "day_limit": 250000,
            "next_intimacy": 3000,
            "can_delete": false
        }
    } */
    // 如果是一点一点的点过去，则会出问题
    qint64 uid = danmaku.getUid();
    if (medalUpgradeWaiting.contains(uid)) // 只计算第一次
        return ;

    QList<int> specialGifts { 30607 };
    if (ac->upUid.isEmpty() || (!danmaku.getTotalCoin() && !specialGifts.contains(danmaku.getGiftId()))) // 亲密度为0，不需要判断
    {
        if (us->debugPrint)
            localNotify("[勋章升级：免费礼物]");
        return ;
    }
    long long giftIntimacy = danmaku.getTotalCoin() / 100;
    if (danmaku.getGiftId() == 30607)
    {
        if ((danmaku.getAnchorRoomid() == ac->roomId && danmaku.getMedalLevel() < 21) || !danmaku.isGuard())
        {
            giftIntimacy = danmaku.getNumber() * 50; // 21级以下的小心心有效，一个50
        }
        else
        {
            if (us->debugPrint)
                localNotify("[勋章升级：小心心无效]");
            return ;
        }
    }
    if (!giftIntimacy) // 0瓜子，不知道什么小礼物，就不算进去了
    {
        if (us->debugPrint)
            localNotify("[勋章升级：0电池礼物]");
        return ;
    }

    QString currentAnchorRoom = danmaku.getAnchorRoomid();
    int currentMedalLevel = danmaku.getMedalLevel();
    if (us->debugPrint)
        localNotify("[当前勋章：房间" + currentAnchorRoom + "，等级" + snum(currentMedalLevel) + "]");

    // 获取新的等级
    QString url = "https://api.live.bilibili.com/fans_medal/v1/fans_medal/get_fans_medal_info?source=1&uid="
            + snum(danmaku.getUid()) + "&target_id=" + ac->upUid;

    medalUpgradeWaiting.append(uid);
    QTimer::singleShot(0, [=]{
        medalUpgradeWaiting.removeOne(uid);
        get(url, [=](MyJson json){
            MyJson medalObject = json.data();
            if (medalObject.isEmpty())
            {
                if (us->debugPrint)
                    localNotify("[勋章升级：无勋章]");
                return ; // 没有勋章，更没有亲密度
            }
            int intimacy = medalObject.i("intimacy"); // 当前亲密度
            int nextIntimacy = medalObject.i("next_intimacy"); // 下一级亲密度
            if (us->debugPrint)
                localNotify("[亲密度：" + snum(intimacy) + "/" + snum(nextIntimacy) + "]");
            if (intimacy >= giftIntimacy) // 没有升级，或者刚拿到粉丝牌升到1级
            {
                if (us->debugPrint)
                    localNotify("[勋章升级：未升级，已有" + snum(intimacy) + ">=礼物" + snum(giftIntimacy) + "]");
                return ;
            }
            LiveDanmaku ld = danmaku;
            int level = medalObject.i("level");
            if (us->debugPrint)
            {
                localNotify("[勋章升级：" + snum(level) + "级]");
            }
            if (ld.getAnchorRoomid() != ac->roomId && (!ac->shortId.isEmpty() && ld.getAnchorRoomid() != ac->shortId)) // 没有戴本房间的牌子
            {
                if (us->debugPrint)
                    localNotify("[勋章升级：非本房间 " + ld.getAnchorRoomid() + "]");
            }
            ld.setMedalLevel(level); // 设置为本房间的牌子
            triggerCmdEvent("MEDAL_UPGRADE", ld.with(json), true);
        });
    });
}

/**
 * 触发进入直播间事件
 * 偶然看到的，未经测试
 */
void BiliLiveService::roomEntryAction()
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/roomEntryAction?room_id="
            + ac->roomId + "&platform=pc&csrf_token=" + ac->csrf_token + "&csrf=" + ac->csrf_token + "&visit_id=";
    post(url, QByteArray(), [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("进入直播间返回结果不为0：") << json.value("message").toString();
            return ;
        }
    });
}

void BiliLiveService::getBagList(qint64 sendExpire)
{
    if (ac->roomId.isEmpty() || ac->browserCookie.isEmpty())
    {
        qWarning() << "房间为空，或未登录";
        return ;
    }
    /*{
        "code": 0,
        "message": "0",
        "ttl": 1,
        "data": {
            "list": [
                {
                    "bag_id": 232151929,
                    "gift_id": 30607,
                    "gift_name": "小心心",
                    "gift_num": 12,
                    "gift_type": 5,             // 0 = 普通, 6 = 礼盒, 2 = 加分卡
                    "expire_at": 1612972800,
                    "corner_mark": "4天",
                    "corner_color": "",
                    "count_map": [
                        {
                            "num": 1,
                            "text": ""
                        },
                        {
                            "num": 12,
                            "text": "全部"
                        }
                    ],
                    "bind_roomid": 0,
                    "bind_room_text": "",
                    "type": 1,
                    "card_image": "",
                    "card_gif": "",
                    "card_id": 0,
                    "card_record_id": 0,
                    "is_show_send": false
                },
                ......
    }*/

    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/gift/bag_list?t=1612663775421&room_id=" + ac->roomId;
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取包裹返回结果不为0：") << json.value("message").toString();
            return ;
        }

        QJsonArray bagArray = json.value("data").toObject().value("list").toArray();

        if (true)
        {
            foreach (QJsonValue val, bagArray)
            {
                QJsonObject bag = val.toObject();
                // 赠送礼物
                QString giftName = bag.value("gift_name").toString();
                int giftId = bag.value("gift_id").toInt();
                int giftNum = bag.value("gift_num").toInt();
                qint64 bagId = qint64(bag.value("bag_id").toDouble());
                QString cornerMark = bag.value("corner_mark").toString();
                // qInfo() << "当前礼物：" << giftName << "×" << giftNum << cornerMark << giftId <<bagId;
            }
        }

        if (sendExpire) // 赠送过期礼物
        {
            QList<int> whiteList{1, 6, 30607}; // 辣条、亿圆、小心心

            qint64 timestamp = QDateTime::currentSecsSinceEpoch();
            qint64 expireTs = timestamp + sendExpire; // 超过这个时间的都送
            foreach (QJsonValue val, bagArray)
            {
                QJsonObject bag = val.toObject();
                qint64 expire = qint64(bag.value("expire_at").toDouble());
                int giftId = bag.value("gift_id").toInt();
                if (!whiteList.contains(giftId) || expire == 0 || expire > expireTs)
                    continue ;

                // 赠送礼物
                QString giftName = bag.value("gift_name").toString();
                int giftNum = bag.value("gift_num").toInt();
                qint64 bagId = qint64(bag.value("bag_id").toDouble());
                QString cornerMark = bag.value("corner_mark").toString();
                qInfo() << "赠送过期礼物：" << giftId << giftName << "×" << giftNum << cornerMark << (expire-timestamp)/3600 << "小时";
                sendBagGift(giftId, giftNum, bagId);
            }
        }
    });
}

void BiliLiveService::myLiveSelectArea(bool update)
{
    get("https://api.live.bilibili.com/xlive/web-interface/v1/index/getWebAreaList?source_id=2", [=](MyJson json) {
        if (json.code() != 0)
            return showError("获取分区列表", json.msg());
        FacileMenu *menu = (new FacileMenu(rt->mainwindow))->setSubMenuShowOnCursor(false);
        if (update)
            menu->addTitle("修改分区", 1);
        else
            menu->addTitle("选择分区", 1);

        json.data().each("data", [=](MyJson json){
            QString parentId = snum(json.i("id")); // 这是int类型的
            QString parentName = json.s("name");
            auto parentMenu = menu->addMenu(parentName);
            json.each("list", [=](MyJson json) {
                QString id = json.s("id"); // 这个是字符串的
                QString name = json.s("name");
                parentMenu->addAction(name, [=]{
                    ac->areaId = id;
                    ac->areaName = name;
                    ac->parentAreaId = parentId;
                    ac->parentAreaName = parentName;
                    us->setValue("myLive/areaId", ac->areaId);
                    us->setValue("myLive/parentAreaId", ac->parentAreaId);
                    qInfo() << "选择分区：" << parentName << parentId << ac->areaName << ac->areaId;
                    if (update)
                    {
                        myLiveUpdateArea(ac->areaId);
                    }
                });
            });
        });

        menu->exec();
    });
}

void BiliLiveService::myLiveUpdateArea(QString area)
{
    qInfo() << "更新AreaId:" << area;
    post("https://api.live.bilibili.com/room/v1/Room/update",
    {"room_id", ac->roomId, "area_id", area,
         "csrf_token", ac->csrf_token, "csrf", ac->csrf_token},
         [=](MyJson json) {
        if (json.code() != 0)
            return showError("修改分区失败", json.msg());
    });
}

void BiliLiveService::myLiveStartLive()
{
    int lastAreaId = us->value("myLive/areaId", 0).toInt();
    if (!lastAreaId)
    {
        showError("一键开播", "必须选择分类才能开播");
        return ;
    }
    post("https://api.live.bilibili.com/room/v1/Room/startLive",
    {"room_id", ac->roomId, "platform", "pc", "area_v2", snum(lastAreaId),
         "csrf_token", ac->csrf_token, "csrf", ac->csrf_token},
         [=](MyJson json) {
        qInfo() << "开播：" << json;
        if (json.code() != 0)
            return showError("一键开播失败", json.msg());
        MyJson rtmp = json.data().o("rtmp");
        myLiveRtmp = rtmp.s("addr");
        myLiveCode = rtmp.s("code");
    });
}

void BiliLiveService::myLiveStopLive()
{
    post("https://api.live.bilibili.com/room/v1/Room/stopLive",
    {"room_id", ac->roomId, "platform", "pc",
         "csrf_token", ac->csrf_token, "csrf", ac->csrf_token},
         [=](MyJson json) {
        qInfo() << "下播：" << json;
        if (json.code() != 0)
            return showError("一键下播失败", json.msg());
    });
}

void BiliLiveService::myLiveSetTitle(QString newTitle)
{
    if (ac->upUid != ac->cookieUid)
        return showError("只有主播才能操作");
    if (newTitle.isEmpty())
    {
        QString title = ac->roomTitle;
        bool ok = false;
        newTitle = QInputDialog::getText(rt->mainwindow, "修改直播间标题", "修改直播间标题，立刻生效", QLineEdit::Normal, title, &ok);
        if (!ok)
            return ;
    }

    post("https://api.live.bilibili.com/room/v1/Room/update",
         QStringList{
             "room_id", ac->roomId,
             "title", newTitle,
             "csrf_token", ac->csrf_token,
             "csrf", ac->csrf_token
         }, [=](MyJson json) {
        qInfo() << "设置直播间标题：" << json;
        if (json.code() != 0)
            return showError(json.msg());
        ac->roomTitle = newTitle;
        emit signalRoomTitleChanged(newTitle);
#ifdef ZUOQI_ENTRANCE
        fakeEntrance->setRoomName(newTitle);
#endif
    });
}

void BiliLiveService::myLiveSetNews()
{
    QString content = ac->roomNews;
    bool ok = false;
    content = TextInputDialog::getText(rt->mainwindow, "修改主播公告", "修改主播公告，立刻生效", content, &ok);
    if (!ok)
        return ;

    post("https://api.live.bilibili.com/room_ex/v1/RoomNews/update",
         QStringList{
             "room_id", ac->roomId,
             "uid", ac->cookieUid,
             "content", content,
             "csrf_token", ac->csrf_token,
             "csrf", ac->csrf_token
         }, [=](MyJson json) {
        qInfo() << "设置主播公告：" << json;
        if (json.code() != 0)
            return showError(json.msg());
        ac->roomNews = content;
    });
}

void BiliLiveService::myLiveSetDescription()
{
    QString content = ac->roomDescription;
    bool ok = false;
    content = TextInputDialog::getText(rt->mainwindow, "修改个人简介", "修改主播的个人简介，立刻生效", content, &ok);
    if (!ok)
        return ;

    post("https://api.live.bilibili.com/room/v1/Room/update",
         QStringList{
             "room_id", ac->roomId,
             "description", content,
             "csrf_token", ac->csrf_token,
             "csrf", ac->csrf_token
         }, [=](MyJson json) {
        qInfo() << "设置个人简介：" << json;
        if (json.code() != 0)
            return showError(json.msg());
        ac->roomDescription = content;
        emit signalRoomDescriptionChanged(content);
    });
}

void BiliLiveService::myLiveSetCover(QString path)
{
    if (ac->upUid != ac->cookieUid)
        return showError("只有主播才能操作");
    if (path.isEmpty())
    {
        // 选择封面
        QString oldPath = us->value("recent/coverPath", "").toString();
        path = QFileDialog::getOpenFileName(rt->mainwindow, "选择上传的封面", oldPath, "Image (*.jpg *.png *.jpeg *.gif)");
        if (path.isEmpty())
            return ;
        us->setValue("recent/coverPath", path);
    }
    else
    {
        if (!isFileExist(path))
            return showError("封面文件不存在", path);
    }

    // 裁剪图片：大致是 470 / 293 = 1.6 = 8 : 5
    QPixmap pixmap(path);
    // if (!ClipDialog::clip(pixmap, AspectRatio, 8, 5))
    //     return ;

    // 压缩图片
    const int width = 470;
    const QString clipPath = rt->dataPath + "temp_cover.jpeg";
    pixmap = pixmap.scaledToWidth(width, Qt::SmoothTransformation);
    pixmap.save(clipPath);

    // 开始上传
    HttpUploader* uploader = new HttpUploader("https://api.bilibili.com/x/upload/web/image?csrf=" + ac->csrf_token);
    uploader->setCookies(ac->userCookies);
    uploader->addTextField("bucket", "live");
    uploader->addTextField("dir", "new_room_cover");
    uploader->addFileField("file", "blob", clipPath, "image/jpeg");
    uploader->post();
    connect(uploader, &HttpUploader::finished, this, [=](QNetworkReply* reply) {
        MyJson json(reply->readAll());
        qInfo() << "上传封面：" << json;
        if (json.code() != 0)
            return showError("上传封面失败", json.msg());

        auto data = json.data();
        QString location = data.s("location");
        QString etag = data.s("etag");

        if (roomCover.isNull()) // 仅第一次上传封面，调用 add
        {
            post("https://api.live.bilibili.com/room/v1/Cover/add",
            {"room_id", ac->roomId,
             "url", location,
             "type", "cover",
             "csrf_token", ac->csrf_token,
             "csrf", ac->csrf_token,
             "visit_id", getRandomKey(12)
                 }, [=](MyJson json) {
                qInfo() << "添加封面：" << json;
                if (json.code() != 0)
                    return showError("添加封面失败", json.msg());
            });
        }
        else // 后面就要调用替换的API了，需要参数 pic_id
        {
            // 获取 pic_id
            get("https://api.live.bilibili.com/room/v1/Cover/new_get_list?room_id=" +ac->roomId, [=](MyJson json) {
                qInfo() << "获取封面ID：" << json;
                if (json.code() != 0)
                    return showError("设置封面失败", "无法获取封面ID");
                auto array = json.a("data");
                if (!array.size())
                    return showError("设置封面失败", "获取不到封面数据");
                const qint64 picId = static_cast<long long>(array.first().toObject().value("id").toDouble());

                post("https://api.live.bilibili.com/room/v1/Cover/new_replace_cover",
                {"room_id", ac->roomId,
                 "url", location,
                 "pic_id", snum(picId),
                 "type", "cover",
                 "csrf_token", ac->csrf_token,
                 "csrf", ac->csrf_token,
                 "visit_id", getRandomKey(12)
                     }, [=](MyJson json) {
                    qInfo() << "设置封面：" << json;
                    if (json.code() != 0)
                        return showError("设置封面失败", json.msg());
                });
            });
        }
    });
}

void BiliLiveService::myLiveSetTags()
{
    QString content = ac->roomTags.join(" ");
    bool ok = false;
    content = QInputDialog::getText(rt->mainwindow, "修改我的个人标签", "多个标签之间使用空格分隔\n短时间修改太多标签可能会被临时屏蔽", QLineEdit::Normal, content, &ok);
    if (!ok)
        return ;

    QStringList oldTags = ac->roomTags;
    QStringList newTags = content.split(" ", QString::SkipEmptyParts);

    auto toPost = [=](QString action, QString tag){
        QString rst = NetUtil::postWebData("https://api.live.bilibili.com/room/v1/Room/update",
                             QStringList{
                                 "room_id", ac->roomId,
                                 action, tag,
                                 "csrf_token", ac->csrf_token,
                                 "csrf", ac->csrf_token
                             }, ac->userCookies);
        MyJson json(rst.toUtf8());
        qInfo() << "修改个人标签：" << json;
        if (json.code() != 0)
            return showError(json.msg());
        if (action == "add_tag")
            ac->roomTags.append(tag);
        else
            ac->roomTags.removeOne(tag);
    };

    // 对比新旧
    foreach (auto tag, newTags)
    {
        if (oldTags.contains(tag)) // 没变的部分
        {
            oldTags.removeOne(tag);
            continue;
        }

        // 新增的
        qInfo() << "添加个人标签：" << tag;
        toPost("add_tag", tag);
    }
    foreach (auto tag, oldTags)
    {
        qInfo() << "删除个人标签：" << tag;
        toPost("del_tag", tag);
    }

    // 刷新界面
    emit signalRoomTagsChanged(ac->roomTags);
}

void BiliLiveService::showPkMenu()
{
    FacileMenu* menu = new FacileMenu(rt->mainwindow);

    menu->addAction("大乱斗规则", [=]{
        if (pkRuleUrl.isEmpty())
        {
            showError("大乱斗规则", "未找到规则说明");
            return ;
        }
        QDesktopServices::openUrl(QUrl(pkRuleUrl));
    });

    menu->addAction("最佳助攻列表", [=]{
        showPkAssists();
    });

    menu->split()->addAction("赛季匹配记录", [=]{
        showPkHistories();
    });

    menu->addAction("最后匹配的直播间", [=]{
        if (!ac->lastMatchRoomId)
        {
            showError("匹配记录", "没有最后匹配的直播间");
            return ;
        }
        QDesktopServices::openUrl(QUrl("https://live.bilibili.com/" + snum(ac->lastMatchRoomId)));
    });

    menu->exec();
}

void BiliLiveService::showPkAssists()
{
    QString url = "https://api.live.bilibili.com/av/v1/Battle/anchorBattleRank?uid=" + ac->upUid + "&room_id=" + ac->roomId + "&_=" + snum(QDateTime::currentMSecsSinceEpoch());
    // qInfo() << "pk assists:" << url;
    get(url, [=](MyJson json) {
        JO(json, data);
        JA(data, assist_list); // 助攻列表

        QString title = "助攻列表";
        auto view = new QTableView(rt->mainwindow);
        auto model = new QStandardItemModel(view);
        QStringList columns {
            "名字", "积分", "UID"
        };
        model->setColumnCount(columns.size());
        model->setHorizontalHeaderLabels(columns);
        model->setRowCount(assist_list.size());


        for (int row = 0; row < assist_list.count(); row++)
        {
            MyJson assist = assist_list.at(row).toObject();
            JI(assist, rank); Q_UNUSED(rank)
            JL(assist, uid);
            JS(assist, name);
            JS(assist, face);
            JL(assist, pk_score);

            int col = 0;
            model->setItem(row, col++, new QStandardItem(name));
            model->setItem(row, col++, new QStandardItem(snum(pk_score)));
            model->setItem(row, col++, new QStandardItem(snum(uid)));
        }

        // 创建控件
        view->setModel(model);
        view->setAttribute(Qt::WA_ShowModal, true);
        view->setAttribute(Qt::WA_DeleteOnClose, true);
        view->setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::Dialog);

        // 自适应宽度
        view->resizeColumnsToContents();
        for(int i = 0; i < model->columnCount(); i++)
            view->setColumnWidth(i, view->columnWidth(i) + 10); // 加一点，不然会显得很挤

        // 显示位置
        QRect rect = rt->mainwindow->geometry();
        // int titleHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
        rect.setTop(rect.top());
        view->setWindowTitle(title);
        view->setGeometry(rect);
        view->show();

        // 菜单
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, view, [=]{
            auto index = view->currentIndex();
            if (!index.isValid())
                return ;
            int row = index.row();
            QString uname = model->item(row, 0)->data(Qt::DisplayRole).toString();
            QString uid = model->item(row, 2)->data(Qt::DisplayRole).toString();

            FacileMenu* menu = new FacileMenu(rt->mainwindow);
            menu->addAction("复制昵称", [=]{
                QApplication::clipboard()->setText(uname);
            });
            menu->addAction("复制UID", [=]{
                QApplication::clipboard()->setText(uid);
            });
            menu->split()->addAction("查看首页", [=]{
                QDesktopServices::openUrl(QUrl("https://space.bilibili.com/" + uid));
            });
            menu->exec();
        });
    });
}

void BiliLiveService::showPkHistories()
{
    QString url = "https://api.live.bilibili.com/av/v1/Battle/getPkRecord?"
                  "ruid=" + ac->upUid + "&room_id=" + ac->roomId + "&season_id=" + snum(currentSeasonId) + "&_=" + snum(QDateTime::currentMSecsSinceEpoch());
    // qInfo() << "pk histories" << url;
    get(url, [=](MyJson json) {
        if (json.code())
            return showError("获取PK历史失败", json.msg());
        QString title = "大乱斗历史";
        auto view = new QTableView(rt->mainwindow);
        auto model = new QStandardItemModel(view);
        int rowCount = 0;
        QStringList columns {
            "时间", "主播", "结果", "积分", "票数"/*自己:对面*/, "房间ID"
        };
        model->setColumnCount(columns.size());
        model->setHorizontalHeaderLabels(columns);

        auto pkInfo = json.data().o("pk_info");
        auto dates = pkInfo.keys();
        std::sort(dates.begin(), dates.end(), [=](const QString& s1, const QString& s2) {
            return s1 > s2;
        });
        foreach (auto date, dates)
        {
            pkInfo.o(date).each("list", [&](MyJson info) {
                int result = info.i("result_type"); // 2胜利，1平局，-1失败
                QString resultText = result < 0 ? "失败" : result > 1 ? "胜利" : "平局";
                auto matchInfo = info.o("match_info");

                int row = rowCount++;
                int col = 0;
                model->setRowCount(rowCount);
                model->setItem(row, col++, new QStandardItem(date + " " + info.s("pk_end_time")));
                model->setItem(row, col++, new QStandardItem(matchInfo.s("name")));
                auto resultItem = new QStandardItem(resultText);
                model->setItem(row, col++, resultItem);
                auto scoreItem = new QStandardItem(snum(info.l("pk_score")));
                model->setItem(row, col++, scoreItem);
                scoreItem->setTextAlignment(Qt::AlignCenter);
                auto votesItem = new QStandardItem(snum(info.l("current_pk_votes")) + " : " + snum(matchInfo.l("pk_votes")));
                model->setItem(row, col++, votesItem);
                votesItem->setTextAlignment(Qt::AlignCenter);
                if (result > 1)
                    resultItem->setForeground(Qt::green);
                else if (result < 0)
                    resultItem->setForeground(Qt::red);
                model->setItem(row, col++, new QStandardItem(snum(matchInfo.l("room_id"))));
            });
        }

        // 创建控件
        view->setModel(model);
        view->setAttribute(Qt::WA_ShowModal, true);
        view->setAttribute(Qt::WA_DeleteOnClose, true);
        view->setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::Dialog);

        // 自适应宽度
        view->resizeColumnsToContents();
        for(int i = 0; i < model->columnCount(); i++)
            view->setColumnWidth(i, view->columnWidth(i) + 10); // 加一点，不然会显得很挤

        // 显示位置
        QRect rect = rt->mainwindow->geometry();
        // int titleHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
        rect.setTop(rect.top());
        view->setWindowTitle(title);
        view->setGeometry(rect);
        view->show();

        // 菜单
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, view, [=]{
            auto index = view->currentIndex();
            if (!index.isValid())
                return ;
            int row = index.row();
            QString uname = model->item(row, 1)->data(Qt::DisplayRole).toString();
            QString roomId = model->item(row, 5)->data(Qt::DisplayRole).toString();

            FacileMenu* menu = new FacileMenu(rt->mainwindow);
            menu->addAction("复制昵称", [=]{
                QApplication::clipboard()->setText(uname);
            });
            menu->addAction("复制房间号", [=]{
                QApplication::clipboard()->setText(roomId);
            });
            menu->split()->addAction("前往直播间", [=]{
                QDesktopServices::openUrl(QUrl("https://live.bilibili.com/" + roomId));
            });
            menu->exec();
        });
    });
}

void BiliLiveService::refreshPrivateMsg()
{
    if (ac->cookieUid.isEmpty())
        return ;

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    get("https://api.vc.bilibili.com/session_svr/v1/session_svr/get_sessions?session_type=1&group_fold=1&unfollow_fold=0&build=0&mobi_app=web",
    // get("https://api.vc.bilibili.com/session_svr/v1/session_svr/new_sessions?begin_ts=0&build=0&mobi_app=web",
        [=](MyJson json) {
        if (json.code() != 0)
        {
            if (json.code() == -412)
            {
                showError("接收私信：" + snum(json.code()), "过于频繁，15分钟后自动重试");
                emit signalRefreshPrivateMsgEnabled(false);
                QTimer::singleShot(15 * 60000, [=]{
                    emit signalRefreshPrivateMsgEnabled(true);
                });
            }
            else
            {
                showError("接收私信：" + snum(json.code()), json.msg());
                emit signalRefreshPrivateMsgEnabled(false);
            }
            return ;
        }

        MyJson data = json.data();
        QJsonArray sessionList = data.a("session_list");
        foreach (QJsonValue sessionV, sessionList) // 默认按最新时间排序
        {
            MyJson session = sessionV.toObject();

            // 判断未读消息
            /* qint64 unreadCount = session.i("unread_count");
            if (!unreadCount) // TODO:有时候自动回复不会显示未读
                continue; */

            qint64 sessionTs = session.l("session_ts") / 1000; // 会话时间，纳秒
            if (sessionTs > currentTimestamp) // 连接中间的时间
                continue;
            if (sessionTs <= privateMsgTimestamp) // 之前已处理
                break;

            // 接收到会话信息
            receivedPrivateMsg(session);
        }
        privateMsgTimestamp = currentTimestamp;
    });
}

void BiliLiveService::receivedPrivateMsg(MyJson session)
{
    // 解析消息内容
    qint64 talkerId = session.l("talker_id"); // 用户ID
    if (!talkerId)
        return ;
    qint64 sessionTs = session.l("session_ts") / 1000; // 会话时间，纳秒
    int isFollow = session.i("is_follow"); Q_UNUSED(isFollow)
    int isDnd = session.i("is_dnd"); Q_UNUSED(isDnd)
    MyJson lastMsg = session.o("last_msg");
    QString content = "";
    int msgType = 0;
    if (!lastMsg.isEmpty())
    {
        // 1纯文本，2图片，3撤回消息，6自定义表情，7分享稿件，10通知消息，11发布视频，12发布专栏，13卡片消息，14分享直播，
        msgType = lastMsg.i("msg_type"); // 使用 %.msg_type% 获取
        qint64 senderUid = lastMsg.l("sender_uid"); // 自己或者对面发送的
        if (senderUid != talkerId) // 自己已经回复了
            return ;
        qint64 receiverId = lastMsg.l("receiver_id");
        Q_ASSERT(receiverId == ac->cookieUid.toLongLong());
        content = lastMsg.s("content");
        MyJson lastContent = MyJson::from(content.toUtf8());
        if (lastContent.contains("content"))
            content = lastContent.s("content");
        else if (lastContent.contains("text"))
            content = lastContent.s("text");
        else
            content.replace("\\n", "\n"); // 肯定有问题
    }
    qInfo() << "接收到私信：" << session;

    // 获取发送者信息
    get("https://api.bilibili.com/x/space/space/wbi/info?mid=" + snum(talkerId), [=](MyJson info) {
        MyJson newJson = session;

        // 解析信息
        QString name = snum(talkerId);
        QString faceUrl = "";
        if (info.code() == 0)
        {
            MyJson data = info.data();
            name = data.s("name");
            faceUrl = data.s("face");
            newJson.insert("sender", info);
        }
        else
        {
            qWarning() << "获取私信发送者信息失败：" << info.msg();
        }
        qInfo() << "接收到私信: " << name << msgType << ":" << content
                << "    (" << sessionTs << "  in "
                << privateMsgTimestamp << "~" << QDateTime::currentMSecsSinceEpoch() << ")";

        // 触发事件
        LiveDanmaku danmaku(talkerId, name, content);
        danmaku.with(session);
        danmaku.setUid(talkerId);
        danmaku.setTime(QDateTime::fromMSecsSinceEpoch(sessionTs));
        triggerCmdEvent("RECEIVE_PRIVATE_MSG", danmaku);
    });
}

QString BiliLiveService::getApiUrl(ApiType type, qint64 id)
{
    switch (type)
    {
    case ApiType::PlatformHomePage:
        return "https://bilibili.com";
    case ApiType::RoomPage:
        return "https://live.bilibili.com/" + snum(id);
    case ApiType::UserSpace:
        return "https://space.bilibili.com/" + snum(id);
    case ApiType::UserFollows:
        return "https://space.bilibili.com/" + snum(id) + "/fans/follow";
    case ApiType::UserFans:
        return "https://space.bilibili.com/" + snum(id) + "/fans/fans";
    case ApiType::UserVideos:
        return "https://space.bilibili.com/" + snum(id) + "/video";
    case ApiType::UserArticles:
        return "https://space.bilibili.com/" + snum(id) + "/article";
    case ApiType::UserDynamics:
        return "https://space.bilibili.com/" + snum(id) + "/dynamic";
    case ApiType::AppStorePage:
        return "https://play-live.bilibili.com/details/1653383145397";
    case ApiType::UserHead:
        if (wbiMixinKey.isEmpty())
            return WAIT_INIT;
        QString url = "https://api.bilibili.com/x/space/wbi/acc/info?" + toWbiParam("mid=" + snum(id) + "&platform=web&token=&web_location=1550101");
        MyJson json(NetUtil::getWebData(url));
        return json.data().s("face");
    }
    return "";
}

QStringList BiliLiveService::getRoomShieldKeywordsAsync(bool* ok)
{
    MyJson roomSK(NetUtil::getWebData("https://api.live.bilibili.com/xlive/web-ucenter/v1/banned/GetShieldKeywordList?room_id=" + ac->roomId, ac->userCookies));
    if (roomSK.code() != 0)
    {
        emit signalShowError("获取房间屏蔽词失败", roomSK.msg());
        if (ok)
            *ok = false;
        return QStringList{};
    }
    *ok = true;
    QStringList wordsList;
    foreach (auto k, roomSK.data().a("keyword_list"))
        wordsList.append(k.toObject().value("keyword").toString());
    qInfo() << "当前直播间屏蔽词：" << wordsList.count() << "个";
    return wordsList;
}

void BiliLiveService::addRoomShieldKeywordsAsync(const QString &word)
{
    MyJson json(NetUtil::postWebData("https://api.live.bilibili.com/xlive/web-ucenter/v1/banned/AddShieldKeyword",
    { "room_id", ac->roomId, "keyword", word, "scrf_token", ac->csrf_token, "csrf", ac->csrf_token, "visit_id", ""}, ac->userCookies).toLatin1());
    if (json.code() != 0)
        qWarning() << "添加直播间屏蔽词失败：" << json.msg();
}

void BiliLiveService::removeRoomShieldKeywordAsync(const QString &word)
{
    MyJson json(NetUtil::postWebData("https://api.live.bilibili.com/xlive/web-ucenter/v1/banned/DelShieldKeyword",
    { "room_id", ac->roomId, "keyword", word, "scrf_token", ac->csrf_token, "csrf", ac->csrf_token, "visit_id", ""}, ac->userCookies).toLatin1());
    if (json.code() != 0)
        qWarning() << "移除直播间屏蔽词失败：" << json.msg();
}

void BiliLiveService::updatePositiveVote()
{
    get("https://api.live.bilibili.com/xlive/virtual-interface/v1/app/detail?app_id=1653383145397", [=](MyJson json) {
        if (json.code() != 0)
        {
            qWarning() << "获取饭贩商店应用失败：" << json;
            return ;
        }
        MyJson data = json.data();
        fanfanOwn = data.b("own");
        fanfanLike = data.b("is_like");
        fanfanLikeCount = data.i("like_count");
        emit signalPositiveVoteStateChanged(fanfanLike);
        emit signalPositiveVoteCountChanged("好评(" + snum(fanfanLikeCount) + ")");


        // 获取到的数值可能是有问题的
        if (fanfanLikeCount == 0)
            return ;

        // 添加到自己的库中
        if (!fanfanOwn)
        {
            qInfo() << "饭贩商店获取本应用";
            fanfanAddOwn();
        }
    });
}

/**
 * 饭贩好评
 * 需要 Cookie 带有 PEA_AU 字段
 */
void BiliLiveService::setPositiveVote()
{
    // 浏览器复制的cookie，一键好评
    if (!ac->browserCookie.contains("PEA_AU="))
    {
        // 先获取登录信息，再进行好评
        fanfanLogin(true);
        return;
    }

    QString url = "https://api.live.bilibili.com/xlive/virtual-interface/v1/app/like";
    MyJson json;
    json.insert("app_id", (qint64)1653383145397);
    json.insert("csrf_token", ac->csrf_token);
    json.insert("csrf", ac->csrf_token);
    json.insert("visit_id", "");

    postJson(url, json.toBa(), [=](QNetworkReply* reply){
        QByteArray ba(reply->readAll());
        MyJson json(ba);
        if (json.code() != 0)
        {
            qWarning() << "饭贩好评失败";
            QDesktopServices::openUrl(QUrl("https://play-live.bilibili.com/details/1653383145397"));
        }

        QTimer::singleShot(0, [=]{
            updatePositiveVote();
        });
    });
}

void BiliLiveService::fanfanLogin(bool autoVote)
{
    post("https://api.live.bilibili.com/xlive/virtual-interface/v1/user/login", "", [=](QNetworkReply* reply){
        QVariant variantCookies = reply->header(QNetworkRequest::SetCookieHeader);
        QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie> >(variantCookies);
        if (!cookies.size())
        {
            qWarning() << "login 没有返回 set cookie。" << variantCookies;
            return ;
        }
        QNetworkCookie cookie = cookies.at(0);
        QString DataAsString =cookie.toRawForm(); // 转换为QByteArray
        qInfo() << "饭贩连接：" << DataAsString;
        autoSetCookie(ac->browserCookie + "; " + DataAsString);

        if (!ac->browserCookie.contains("PEA_AU="))
        {
            qCritical() << "饭贩登录后不带有PEA_AU信息";
            return;
        }

        if (autoVote)
        {
            QTimer::singleShot(3000, [=]{
                if (!fanfanLike)
                {
                    setPositiveVote();
                }
            });
        }
    });
}

void BiliLiveService::fanfanAddOwn()
{
    QString url = "https://api.live.bilibili.com/xlive/virtual-interface/v1/app/addOwn";
    MyJson json;
    json.insert("app_id", (qint64)1653383145397);
    json.insert("csrf_token", ac->csrf_token);
    json.insert("csrf", ac->csrf_token);
    json.insert("visit_id", "");

    postJson(url, json.toBa(), [=](QNetworkReply* reply){
        QByteArray ba(reply->readAll());
        MyJson json(ba);
        if (json.code() == 1666501)
        {
            qInfo() << "饭贩已拥有，无需重复添加";
        }
        else if (json.code() != 0)
        {
            qWarning() << "饭贩添加失败：" << json.msg();
        }
    });
}

int BiliLiveService::getPositiveVoteCount() const
{
    return fanfanLikeCount;
}

bool BiliLiveService::isPositiveVote() const
{
    return fanfanLike;
}

void BiliLiveService::getRoomBattleInfo()
{
    get("https://api.live.bilibili.com/av/v1/Battle/anchorBattleRank",
        {"uid", ac->upUid, "_", snum(QDateTime::currentMSecsSinceEpoch())}, [=](MyJson json) {
        MyJson jdata = json.data();
        MyJson pkInfo = jdata.o("anchor_pk_info");
        ac->battleRankName = pkInfo.s("pk_rank_name");
        emit signalBattleRankNameChanged(ac->battleRankName);
        QString battleRankUrl = pkInfo.s("first_rank_img_url");
        emit signalBattleEnabled(!ac->battleRankName.isEmpty());
        if (!ac->battleRankName.isEmpty())
        {
            get(battleRankUrl, [=](QNetworkReply* reply1){
                QPixmap pixmap;
                pixmap.loadFromData(reply1->readAll());
                emit signalBattleRankIconChanged(pixmap);
            });
            updateWinningStreak(false);
        }
    });
}

void BiliLiveService::updateWinningStreak(bool emitWinningStreak)
{
    QString url = "https://api.live.bilibili.com/av/v1/Battle/anchorBattleRank?uid=" + ac->upUid + "&room_id=" + ac->roomId + "&_=" + snum(QDateTime::currentMSecsSinceEpoch());
    // qInfo() << "winning streak:" << url;
    get(url, [=](MyJson json) {
        JO(json, data);
        JO(data, anchor_pk_info);
        JS(anchor_pk_info, pk_rank_name); // 段位名字：钻石传说
        JI(anchor_pk_info, pk_rank_star); Q_UNUSED(pk_rank_star) // 段位级别：2
        JS(anchor_pk_info, first_rank_img_url); // 图片
        if (pk_rank_name.isEmpty())
            return ;

        // if (!battleRankName.contains(pk_rank_name)) // 段位有变化

        JO(data, season_info);
        JI(season_info, current_season_id); // 赛季ID：38
        JS(season_info, current_season_name); // 赛季名称：PK大乱斗S12赛季
        currentSeasonId = current_season_id;
        emit signalBattleSeasonInfoChanged(current_season_name + " (id:" + snum(current_season_id) + ")");

        JO(data, score_info);
        JI(score_info, total_win_num); // 总赢：309
        JI(score_info, max_win_num); // 最高连胜：16
        JI(score_info, total_num); // 总次数（>总赢）：454
        JI(score_info, total_tie_count); Q_UNUSED(total_tie_count) // 平局：3
        JI(score_info, total_lose_count); // 总失败数：142
        JI(score_info, win_count); // 当前连胜？：5
        JI(score_info, win_rate); // 胜率：69
        ac->winningStreak = win_count;

        QStringList nums {
            "总场次　：" + snum(total_num),
            "胜　　场：" + snum(total_win_num),
            "败　　场：" + snum(total_lose_count),
            "最高连胜：" + snum(max_win_num),
            "胜　　率：" + snum(win_rate) + "%"
        };
        emit signalBattleNumsChanged(nums.join("\n"));

        JA(data, assist_list); // 助攻列表

        JO(data, pk_url_info);
        pkRuleUrl = pk_url_info.s("pk_rule_url");

        JO(data, rank_info);
        JI(rank_info, rank); // 排名
        JL(rank_info, score); // 积分
        JL(anchor_pk_info, next_rank_need_score); // 下一级需要积分
        QStringList scores {
            "排名：" + snum(rank),
            "积分：" + snum(score),
            "升级需要积分：" + snum(next_rank_need_score)
        };
        emit signalBattleScoreChanged(scores.join("\n"));

        JO(data, last_pk_info);
        JO(last_pk_info, match_info);
        JL(match_info, room_id); // 最后匹配的主播
        ac->lastMatchRoomId = room_id;

        emit signalBattleRankGot();

        if (emitWinningStreak && ac->winningStreak > 0 && ac->winningStreak == win_count - 1)
        {
            LiveDanmaku danmaku;
            danmaku.setNumber(win_count);
            danmaku.with(match_info);
            triggerCmdEvent("PK_WINNING_STREAK", danmaku);
        }
    });
}

void BiliLiveService::getPkInfoById(const QString &roomId, const QString &pkId)
{
    /*{
        "code": 0,
        "msg": "ok",
        "message": "ok",
        "data": {
            "battle_type": 2,
            "match_type": 8,
            "status_msg": "",
            "pk_id": 200456233,
            "season_id": 31,
            "status": 201,
            "match_status": 0,
            "match_max_time": 300,
            "match_end_time": 0,
            "pk_pre_time": 1611844801,
            "pk_start_time": 1611844811,
            "pk_frozen_time": 1611845111,
            "pk_end_time": 1611845121,
            "timestamp": 1611844942,
            "final_hit_votes": 0,
            "pk_votes_add": 0,
            "pk_votes_name":"乱斗值",
            "pk_votes_type": 0,
            "cdn_id": 72,
            "init_info": {
                "room_id": 22580649,
                "uid": 356772517,
                "uname":"呱什么呱",
                "face": "https://i1.hdslb.com/bfs/face/3a4d357fdf88d73110b6b0cb31f3417f70c785af.jpg",
                "votes": 0,
                "final_hit_status": 0,
                "resist_crit_status": 0,
                "resist_crit_num": "",
                "best_uname": "",
                "winner_type": 0,
                "end_win_task": null
            },
            "match_info": {
                "room_id": 4720666,
                "uid": 13908357,
                "uname":"娇羞的蘑菇",
                "face": "https://i1.hdslb.com/bfs/face/180d0e87a0e88cb6c04ce6504c3f04003dd77392.jpg",
                "votes": 0,
                "final_hit_status": 0,
                "resist_crit_status": 0,
                "resist_crit_num": "",
                "best_uname": "",
                "winner_type": 0,
                "end_win_task": null
            }
        }
    }*/

    QString url = "https://api.live.bilibili.com/av/v1/Battle/getInfoById?pk_id="+pkId+"&roomid=" + roomId;\
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qWarning() << "PK查询结果不为0：" << json.value("message").toString();
            return ;
        }

        // 获取用户信息
        // pk_pre_time  pk_start_time  pk_frozen_time  pk_end_time
        QJsonObject pkData = json.value("data").toObject();
        qint64 pkStartTime = static_cast<qint64>(pkData.value("pk_start_time").toDouble());
        pkEndTime = static_cast<qint64>(pkData.value("pk_frozen_time").toDouble());

        QJsonObject initInfo = pkData.value("init_info").toObject();
        QJsonObject matchInfo = pkData.value("match_info").toObject();
        if (snum(qint64(matchInfo.value("room_id").toDouble())) == roomId)
        {
            QJsonObject temp = initInfo;
            initInfo = matchInfo;
            matchInfo = temp;
        }

        pkRoomId = snum(qint64(matchInfo.value("room_id").toDouble()));
        pkUid = snum(qint64(matchInfo.value("uid").toDouble()));
        pkUname = matchInfo.value("uname").toString();
        myVotes = initInfo.value("votes").toInt();
        matchVotes = matchInfo.value("votes").toInt();
        pkBattleType = pkData.value("battle_type").toInt();
        pkVideo = pkBattleType == 2;

        pkTimer->start();

        qint64 currentTime = QDateTime::currentSecsSinceEpoch();
        // 已经开始大乱斗
        if (currentTime > pkStartTime) // !如果是刚好开始，可能不能运行下面的，也可能不会触发"PK_START"，不管了
        {
            if (pkChuanmenEnable)
            {
                connectPkRoom();
            }

            // 监听尾声
            qint64 deltaEnd = pkEndTime - currentTime;
            if (deltaEnd*1000 > pkJudgeEarly)
            {
                pkEndingTimer->start(int(deltaEnd*1000 - pkJudgeEarly));

                // 怕刚好结束的一瞬间，没有收到大乱斗的消息
                QTimer::singleShot(qMax(0, int(deltaEnd)), [=]{
                    pkEnding = false;
                    pkVoting = 0;
                });
            }
        }
    });
}

void BiliLiveService::connectPkRoom()
{
    if (pkRoomId.isEmpty())
        return ;
    qInfo() << "连接PK房间:" << pkRoomId;

    // 根据弹幕消息
    myAudience.clear();
    oppositeAudience.clear();

    getRoomCurrentAudiences(ac->roomId, myAudience);
    getRoomCurrentAudiences(pkRoomId, oppositeAudience);

    // 额外保存的许多本地弹幕消息
    for (int i = 0; i < roomDanmakus.size(); i++)
    {
        qint64 uid = roomDanmakus.at(i).getUid();
        if (uid)
            myAudience.insert(uid);
    }

    // 保存自己主播、对面主播（带头串门？？？）
    myAudience.insert(ac->upUid.toLongLong());
    oppositeAudience.insert(pkUid.toLongLong());

    // 连接socket
    if (pkLiveSocket)
        pkLiveSocket->deleteLater();
    pkLiveSocket = new QWebSocket();

    connect(pkLiveSocket, &QWebSocket::connected, this, [=]{
        SOCKET_DEB << "pkSocket connected";
        hostUseIndex--;
        // 5秒内发送认证包
        sendVeriPacket(pkLiveSocket, pkRoomId, pkToken);
    });

    connect(pkLiveSocket, &QWebSocket::disconnected, this, [=]{
        // 正在直播的时候突然断开了，比如掉线
        if (isLiving() && pkLiveSocket)
        {
            if (pking && pkChuanmenEnable) // 需要继续连接
            {
                qWarning() << "pkSocket断开连接";

                // 判断是否要重新连接
                connectPkSocket();
            }
            else // 结束的情况，断开并清空
            {
                pkLiveSocket->deleteLater();
                pkLiveSocket = nullptr;
            }
            return ;
        }
    });

    connect(pkLiveSocket, &QWebSocket::binaryMessageReceived, this, &BiliLiveService::slotPkBinaryMessageReceived);

    // ========== 开始连接 ==========
    connectPkSocket();
}

void BiliLiveService::getRoomCurrentAudiences(QString roomId, QSet<qint64> &audiences)
{
    QString url = "https://api.live.bilibili.com/ajax/msg";
    QStringList param{"roomid", roomId};
    connect(new NetUtil(url, param), &NetUtil::finished, this, [&](QString result){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(result.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << "getRoomCurrentAudiences.ERROR:" << error.errorString();
            qCritical() << result;
            return ;
        }
        QJsonObject json = document.object();
        QJsonArray danmakus = json.value("data").toObject().value("room").toArray();
//        qInfo() << "初始化房间" << roomId << "观众：";
        for (int i = 0; i < danmakus.size(); i++)
        {
            LiveDanmaku danmaku = LiveDanmaku::fromDanmakuJson(danmakus.at(i).toObject());
//            if (!audiences.contains(danmaku.getUid()))
//                qInfo() << "    添加观众：" << danmaku.getNickname();
            audiences.insert(danmaku.getUid());
        }
    });
}

void BiliLiveService::connectPkSocket()
{
    if (pkRoomId.isEmpty())
        return ;

    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getDanmuInfo";
    url += "?id="+pkRoomId+"&type=0";
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray dataBa = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(dataBa, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << "获取弹幕信息出错：" << error.errorString();
            return ;
        }
        QJsonObject json = document.object();

        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("pk对面直播间返回结果不为0：") << json.value("message").toString();
            return ;
        }

        QJsonObject data = json.value("data").toObject();
        pkToken = data.value("token").toString();
        QJsonArray hostArray = data.value("host_list").toArray();
        QString link = hostArray.first().toObject().value("host").toString();
        int port = hostArray.first().toObject().value("wss_port").toInt();
        QString host = QString("wss://%1:%2/sub").arg(link).arg(port);
        SOCKET_DEB << "pk.socket.host:" << host;

        // ========== 连接Socket ==========
        qInfo() << "连接 PK Socket：" << pkRoomId;
        if (!pkLiveSocket)
            pkLiveSocket = new QWebSocket();
        QSslConfiguration config = pkLiveSocket->sslConfiguration();
        config.setPeerVerifyMode(QSslSocket::VerifyNone);
        config.setProtocol(QSsl::TlsV1SslV3);
        pkLiveSocket->setSslConfiguration(config);
        pkLiveSocket->open(host);
    });
    manager->get(*request);
}

void BiliLiveService::getPkMatchInfo()
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByRoom?room_id=" + pkRoomId;
    get(url, [=](MyJson json) {
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取PK匹配返回结果不为0：") << json.value("message").toString();
            return ;
        }

        MyJson data = json.data();
        LiveDanmaku danmaku;
        danmaku.with(data);

        // 计算高能榜综合
        qint64 sum = 0;
        data.o("online_gold_rank_info_v2").each("list", [&](MyJson usr){
            sum += usr.s("score").toLongLong(); // 这是String类型， 不是int
        });
        danmaku.setTotalCoin(sum); // 高能榜总积分
        danmaku.setNumber(data.o("online_gold_rank_info_v2").a("list").size()); // 高能榜总人数
        qInfo() << "对面高能榜积分总和：" << sum;
        // qDebug() << data.o("online_gold_rank_info_v2").a("list");
        triggerCmdEvent("PK_MATCH_INFO", danmaku, true);
    });
}

void BiliLiveService::getRoomLiveVideoUrl(StringFunc func)
{
    if (ac->roomId.isEmpty())
        return ;
    /*QString url = "https://api.live.bilibili.com/room/v1/Room/playUrl?cid=" + ac->roomId
            + "&quality=4&qn=10000&platform=web&otype=json";
    get(url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("获取视频URL返回结果不为0：") << json.value("message").toString();
            return ;
        }

        // 获取链接
        QJsonArray array = json.value("data").toObject().value("durl").toArray();
        /*"url": "http://d1--cn-gotcha04.bilivideo.com/live-bvc/521719/live_688893202_7694436.flv?cdn=cn-gotcha04\\u0026expires=1607505058\\u0026len=0\\u0026oi=1944862322\\u0026pt=\\u0026qn=10000\\u0026trid=40938d869aca4cb39041730f74ff9051\\u0026sigparams=cdn,expires,len,oi,pt,qn,trid\\u0026sign=ac701ba60c346bf3a173e56bcb14b7b9\\u0026ptype=0\\u0026src=9\\u0026sl=1\\u0026order=1",
          "length": 0,
          "order": 1,
          "stream_type": 0,
          "p2p_type": 0*-/
        QString url = array.first().toObject().value("url").toString();
        if (func)
            func(url);
    });*/

    QString full_url = "https://api.live.bilibili.com/xlive/web-room/v2/index/getRoomPlayInfo?room_id=" + ac->roomId
            + "&no_playurl=0&mask=1&qn=10000&platform=web&protocol=0,1&format=0,1,2&codec=0,1&dolby=5&panorama=1";
    get(full_url, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            qWarning() << ("返回结果不为0：") << json.value("message").toString();
            return ;
        }

        QJsonObject play_url = json.value("data").toObject().value("playurl_info").toObject().value("playurl").toObject();
        QJsonArray streams = play_url.value("stream").toArray();
        QString url;

        // 先抓取m3u8的视频
        foreach (auto _stream, streams)
        {
            auto stream = _stream.toObject();
            // m3u8:http_hls, flv:http_stream
            if (stream.value("protocol_name").toString() != "http_hls")
                continue;

            auto formats = stream.value("format").toArray(); // 这里有多个可播放的格式
            foreach (auto _format, formats)
            {
                auto format = _format.toObject();
                auto codecs = format.value("codec").toArray();
                foreach (auto _codec, codecs)
                {
                    auto codec = _codec.toObject();
                    QString codec_name = codec.value("codec_name").toString();
                    QString base_url = codec.value("base_url").toString();
                    auto url_infos = codec.value("url_info").toArray();
                    foreach (auto _url_info, url_infos)
                    {
                        auto url_info = _url_info.toObject();
                        QString host = url_info.value("host").toString();
                        QString extra = url_info.value("extra").toString();
                        qint64 stream_ttl = url_info.value("stream_ttl").toInt();

                        url = host + base_url;

                        if (!url.isEmpty())
                            break;
                    }
                    if (!url.isEmpty())
                        break;
                }
                if (!url.isEmpty())
                    break;
            }
            if (!url.isEmpty())
                break;
        }

        if (url.isEmpty())
        {
            qWarning() << "录播无法获取下载地址：" << full_url;
            return ;
        }
        if (url.endsWith("?"))
            url = url.left(url.length() - 1);
        if (func)
            func(url);
    });
}

void BiliLiveService::startHeartConnection()
{
    sendXliveHeartBeatE();
}

void BiliLiveService::stopHeartConnection()
{
    if (xliveHeartBeatTimer)
        xliveHeartBeatTimer->stop();
}

void BiliLiveService::sendXliveHeartBeatE()
{
    if (ac->roomId.isEmpty() || ac->cookieUid.isEmpty() || !isLiving())
        return ;
    if (todayHeartMinite >= us->getHeartTimeCount) // 小心心已经收取满了
    {
        if (xliveHeartBeatTimer)
            xliveHeartBeatTimer->stop();
        return ;
    }

    xliveHeartBeatIndex = 0;

    QString url("https://live-trace.bilibili.com/xlive/data-interface/v1/x25Kn/E");

    // 设置数据（JSON的ByteArray）
    QStringList datas;
    datas << "id=" + QString("[%1,%2,%3,%4]").arg(ac->parentAreaId).arg(ac->areaId).arg(xliveHeartBeatIndex).arg(ac->roomId);
    datas << "device=[\"AUTO4115984068636104\",\"f5f08e2f-e4e3-4156-8127-616f79a17e1a\"]";
    datas << "ts=" + snum(QDateTime::currentMSecsSinceEpoch());
    datas << "is_patch=0";
    datas << "heart_beat=[]";
    datas << "ua=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.146 Safari/537.36";
    datas << "csrf_token=" + ac->csrf_token;
    datas << "csrf=" + ac->csrf_token;
    datas << "visit_id=";
    QByteArray ba(datas.join("&").toStdString().data());

    // 连接槽
    post(url, ba, [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            QString message = json.value("message").toString();
            emit signalShowError("获取小心心失败E", message);
            qCritical() << s8("warning: 发送直播心跳失败E：") << message << datas.join("&");
            /* if (message.contains("sign check failed")) // 没有勋章无法获取？
            {
                ui->acquireHeartCheck->setChecked(false);
                showError("获取小心心", "已临时关闭，可能没有粉丝勋章？");
            } */
            return ;
        }

        /*{
            "code": 0,
            "message": "0",
            "ttl": 1,
            "data": {
                "timestamp": 1612765538,
                "heartbeat_interval": 60,
                "secret_key": "seacasdgyijfhofiuxoannn",
                "secret_rule": [2,5,1,4],
                "patch_status": 2
            }
        }*/
        QJsonObject data = json.value("data").toObject();
        xliveHeartBeatBenchmark = data.value("secret_key").toString();
        xliveHeartBeatEts = qint64(data.value("timestamp").toDouble());
        xliveHeartBeatInterval = data.value("heartbeat_interval").toInt();
        xliveHeartBeatSecretRule = data.value("secret_rule").toArray();

        xliveHeartBeatTimer->start();
        xliveHeartBeatTimer->setInterval(xliveHeartBeatInterval * 1000 - 1000);
        // qDebug() << "直播心跳E：" << xliveHeartBeatBenchmark;
    });
}

void BiliLiveService::sendXliveHeartBeatX()
{
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    // 获取加密的数据
    QJsonObject postData;
    postData.insert("id",  QString("[%1,%2,%3,%4]").arg(ac->parentAreaId).arg(ac->areaId).arg(++xliveHeartBeatIndex).arg(ac->roomId));
    postData.insert("device", "[\"AUTO4115984068636104\",\"f5f08e2f-e4e3-4156-8127-616f79a17e1a\"]");
    postData.insert("ts", timestamp);
    postData.insert("ets", xliveHeartBeatEts);
    postData.insert("benchmark", xliveHeartBeatBenchmark);
    postData.insert("time", xliveHeartBeatInterval);
    postData.insert("ua", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.146 Safari/537.36");
    postData.insert("csrf_token", ac->csrf_token);
    postData.insert("csrf", ac->csrf_token);
    QJsonObject calcText;
    calcText.insert("t", postData);
    calcText.insert("r", xliveHeartBeatSecretRule);

    postJson(encServer, calcText, [=](QNetworkReply* reply){
        QByteArray repBa = reply->readAll();
        // qDebug() << "加密直播心跳包数据返回：" << repBa;
        // qDebug() << calcText;
        sendXliveHeartBeatX(MyJson(repBa).s("s"), timestamp);
    });
}

void BiliLiveService::sendXliveHeartBeatX(QString s, qint64 timestamp)
{
    QString url("https://live-trace.bilibili.com/xlive/data-interface/v1/x25Kn/X");

    // 设置数据（JSON的ByteArray）
    QStringList datas;
    datas << "s=" + s; // 生成的签名
    datas << "id=" + QString("[%1,%2,%3,%4]")
             .arg(ac->parentAreaId).arg(ac->areaId).arg(xliveHeartBeatIndex).arg(ac->roomId);
    datas << "device=[\"AUTO4115984068636104\",\"f5f08e2f-e4e3-4156-8127-616f79a17e1a\"]";
    datas << "ets=" + snum(xliveHeartBeatEts);
    datas << "benchmark=" + xliveHeartBeatBenchmark;
    datas << "time=" + snum(xliveHeartBeatInterval);
    datas << "ts=" + snum(timestamp);
    datas << "ua=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.146 Safari/537.36";
    datas << "csrf_token=" + ac->csrf_token;
    datas << "csrf=" + ac->csrf_token;
    datas << "visit_id=";
    QByteArray ba(datas.join("&").toStdString().data());

    // 连接槽
    // qDebug() << "发送直播心跳X：" << url << ba;
    post(url, ba, [=](QJsonObject json){
        // qDebug() << "直播心跳返回：" << json;
        if (json.value("code").toInt() != 0)
        {
            QString message = json.value("message").toString();
            emit signalShowError("发送直播心跳失败X", message);
            qCritical() << s8("warning: 发送直播心跳失败X：") << message << datas.join("&");
            return ;
        }

        /*{
            "code": 0,
            "message": "0",
            "ttl": 1,
            "data": {
                "heartbeat_interval": 60,
                "timestamp": 1612765598,
                "secret_rule": [2,5,1,4],
                "secret_key": "seacasdgyijfhofiuxoannn"
            }
        }*/
        QJsonObject data = json.value("data").toObject();
        xliveHeartBeatBenchmark = data.value("secret_key").toString();
        xliveHeartBeatEts = qint64(data.value("timestamp").toDouble());
        xliveHeartBeatInterval = data.value("heartbeat_interval").toInt();
        xliveHeartBeatSecretRule = data.value("secret_rule").toArray();
        us->setValue("danmaku/todayHeartMinite", ++todayHeartMinite);
        emit signalHeartTimeNumberChanged(todayHeartMinite/5, todayHeartMinite);
        if (todayHeartMinite >= us->getHeartTimeCount)
            if (xliveHeartBeatTimer)
                xliveHeartBeatTimer->stop();
    });
}

void BiliLiveService::appointAdmin(qint64 uid)
{
    if (ac->upUid != ac->cookieUid)
    {
        showError("房管操作", "仅主播可用");
        return ;
    }
    post("https://api.live.bilibili.com/xlive/web-ucenter/v1/roomAdmin/appoint",
         ("admin=" + snum(uid) + "&admin_level=1&csrf_token=" + ac->csrf_token + "&csrf=" + ac->csrf_token + "&visit_id=").toLatin1(),
         [=](MyJson json) {
        qInfo() << "任命房管：" << json;
        if (json.code() != 0)
        {
            showError("任命房管", json.msg());
            return ;
        }

    });
}

void BiliLiveService::dismissAdmin(qint64 uid)
{
    if (ac->upUid != ac->cookieUid)
    {
        showError("房管操作", "仅主播可用");
        return ;
    }
    post("https://api.live.bilibili.com/xlive/app-ucenter/v1/roomAdmin/dismiss",
         ("uid=" + snum(uid) + "&csrf_token=" + ac->csrf_token + "&csrf=" + ac->csrf_token + "&visit_id=").toLatin1(),
         [=](MyJson json) {
        qInfo() << "取消任命房管：" << json;
        if (json.code() != 0)
        {
            showError("取消任命房管", json.msg());
            return ;
        }
    });
}

void BiliLiveService::addBlockUser(qint64 uid, QString roomId, int hour, QString msg)
{
    if(ac->browserData.isEmpty())
    {
        showError("请先设置登录信息");
        return ;
    }

    if (us->localMode)
    {
        localNotify("禁言用户 -> " + snum(uid) + " " + snum(hour) + " 小时");
        return ;
    }

    QString url = "https://api.live.bilibili.com/banned_service/v2/Silent/add_block_user";
    QString data = QString("roomid=%1&block_uid=%2&hour=%3&csrf_token=%4&csrd=%5&visit_id=&msg=%6")
                    .arg(roomId).arg(uid).arg(hour).arg(ac->csrf_token).arg(ac->csrf_token).arg(urlEncode(msg));
    qInfo() << "禁言：" << uid << hour;
    post(url, data.toStdString().data(), [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            showError("禁言失败", json.value("message").toString());
            return ;
        }
        QJsonObject d = json.value("data").toObject();
        qint64 id = static_cast<qint64>(d.value("id").toDouble());
        us->userBlockIds[uid] = id;
        qInfo() << "禁言成功：" << uid;
    });
}

void BiliLiveService::delBlockUser(qint64 uid, QString roomId)
{
    if(ac->browserData.isEmpty())
    {
        showError("请先设置登录信息");
        return ;
    }

    if (us->localMode)
    {
        localNotify("取消禁言 -> " + snum(uid));
        return ;
    }

    if (us->userBlockIds.contains(uid))
    {
        qInfo() << "取消禁言：" << uid << "  id =" << us->userBlockIds.value(uid);
        delRoomBlockUser(us->userBlockIds.value(uid));
        us->userBlockIds.remove(uid);
        return ;
    }

    // 获取直播间的网络ID，再取消屏蔽
    QString url = "https://api.live.bilibili.com/liveact/ajaxGetBlockList?roomid=" + roomId + "&page=1";
    get(url, [=](QJsonObject json){
        int code = json.value("code").toInt();
        if (code != 0)
        {
            if (code == 403)
                showError("取消禁言", "您没有权限");
            else
                showError("取消禁言", json.value("message").toString());
            return ;
        }
        QJsonArray list = json.value("data").toArray();
        foreach (QJsonValue val, list)
        {
            QJsonObject obj = val.toObject();
            if (static_cast<qint64>(obj.value("uid").toDouble()) == uid)
            {
                delRoomBlockUser(static_cast<qint64>(obj.value("id").toDouble())); // 获取房间ID
                break;
            }
        }
    });
}

void BiliLiveService::delRoomBlockUser(qint64 id)
{
    QString url = "https://api.live.bilibili.com/banned_service/v1/Silent/del_room_block_user";
    QString data = QString("id=%1&roomid=%2&csrf_token=%4&csrd=%5&visit_id=")
                    .arg(id).arg(ac->roomId).arg(ac->csrf_token).arg(ac->csrf_token);

    post(url, data.toStdString().data(), [=](QJsonObject json){
        if (json.value("code").toInt() != 0)
        {
            showError("取消禁言", json.value("message").toString());
            return ;
        }
        qInfo() << "取消禁言成功：" << id;

        // if (userBlockIds.values().contains(id))
        //    userBlockIds.remove(userBlockIds.key(id));
    });
}

void BiliLiveService::refreshBlockList()
{
    if (ac->browserData.isEmpty())
    {
        showError("请先设置用户数据");
        return ;
    }

    // 刷新被禁言的列表
    QString url = "https://api.live.bilibili.com/liveact/ajaxGetBlockList?roomid="+ac->roomId+"&page=1";
    get(url, [=](QJsonObject json){
        int code = json.value("code").toInt();
        if (code != 0)
        {
            qWarning() << "获取禁言：" << json.value("message").toString();
            /* if (code == 403)
                showError("获取禁言", "您没有权限");
            else
                showError("获取禁言", json.value("message").toString()); */
            return ;
        }
        QJsonArray list = json.value("data").toArray();
        us->userBlockIds.clear();
        foreach (QJsonValue val, list)
        {
            QJsonObject obj = val.toObject();
            qint64 id = static_cast<qint64>(obj.value("id").toDouble());
            qint64 uid = static_cast<qint64>(obj.value("uid").toDouble());
            QString uname = obj.value("uname").toString();
            us->userBlockIds.insert(uid, id);
//            qInfo() << "已屏蔽:" << id << uname << uid;
        }
    });
}

void BiliLiveService::adjustDanmakuLongest()
{
    int longest = 20;

    // UL等级：20级30字
    if (ac->cookieULevel >= 20)
        longest = qMax(longest, 30);

    // 大航海：舰长20，提督/总督40
    if (ac->cookieGuardLevel == 1 || ac->cookieGuardLevel == 2)
        longest = qMax(longest, 40);
    
    ac->danmuLongest = longest;
    emit signalDanmakuLongestChanged(longest);
}

/**
 * 新的一天到来了
 * 可能是启动时就是新的一天
 * 也可能是运行时到了第二天
 */
void BiliLiveService::processNewDayData()
{
    us->setValue("danmaku/todayHeartMinite", todayHeartMinite = 0);
}

/**
 * 发送单条弹幕的原子操作
 */
void BiliLiveService::sendMsg(const QString& msg)
{
    if (us->localMode)
    {
        localNotify("发送弹幕 -> " + msg + "  (" + snum(msg.length()) + ")");
        return ;
    }

    sendRoomMsg(ac->roomId, msg);
}

/**
 * 向指定直播间发送弹幕
 */
void BiliLiveService::sendRoomMsg(QString roomId, const QString& msg)
{
    if (ac->browserCookie.isEmpty() || ac->browserData.isEmpty())
    {
        showError("发送弹幕", "机器人账号未登录");
#ifdef ZUOQI_ENTRANCE
        QMessageBox::warning(this, "发送弹幕", "请点击登录按钮，登录机器人账号方可发送弹幕");
#endif
        return ;
    }
    if (msg.isEmpty() || ac->roomId.isEmpty())
        return ;

    // 设置数据（JSON的ByteArray）
    QString s = ac->browserData;
    int posl = s.indexOf("msg=")+4;
    int posr = s.indexOf("&", posl);
    if (posr == -1)
        posr = s.length();
    s.replace(posl, posr-posl, msg);

    posl = s.indexOf("roomid=")+7;
    posr = s.indexOf("&", posl);
    if (posr == -1)
        posr = s.length();
    s.replace(posl, posr-posl, roomId);

    QByteArray ba(s.toStdString().data());

    // 连接槽
    post("https://api.live.bilibili.com/msg/send", ba, [=](QJsonObject json){
        QString errorMsg = json.value("message").toString();
        emit signalStatusChanged("");
        if (!errorMsg.isEmpty())
        {
            QString errorDesc = errorMsg;
            if (errorMsg == "f")
            {
                errorDesc = "包含屏蔽词";
            }
            else if (errorMsg == "k")
            {
                errorDesc = "包含直播间屏蔽词";
            }

            showError("发送弹幕失败", errorDesc);
            qWarning() << msg;
            localNotify(errorDesc + " -> " + msg);

            // 重试
            if (!us->retryFailedDanmaku)
                return ;

            if (errorMsg.contains("msg in 1s"))
            {
                localNotify("[5s后重试]");
                emit signalSendAutoMsgInFirst(msg, LiveDanmaku().withRetry().withRoomId(roomId), 5000);
            }
            else if (errorMsg.contains("msg repeat") || errorMsg.contains("频率过快"))
            {
                localNotify("[4s后重试]");
                emit signalSendAutoMsgInFirst(msg, LiveDanmaku().withRetry().withRoomId(roomId), 4200);
            }
            else if (errorMsg.contains("超出限制长度"))
            {
                if (msg.length() <= ac->danmuLongest)
                {
                    localNotify("[错误的弹幕长度：" + snum(msg.length()) + "字，但设置长度" + snum(ac->danmuLongest) + "]");
                }
                else
                {
                    localNotify("[自动分割长度]");
                    emit signalSendAutoMsgInFirst(splitLongDanmu(msg, ac->danmuLongest).join("\\n"), LiveDanmaku().withRetry().withRoomId(roomId), 1000);
                }
            }
            else if (errorMsg == "f") // 系统敏感词
            {
            }
            else if (errorMsg == "k") // 主播设置的直播间敏感词
            {
            }
        }
    });
}

void BiliLiveService::pullLiveDanmaku()
{
    if (ac->roomId.isEmpty())
        return ;
    QString url = "https://api.live.bilibili.com/ajax/msg";
    QStringList param{"roomid", ac->roomId};
    connect(new NetUtil(url, param), &NetUtil::finished, this, [=](QString result){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(result.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << "pullLiveDanmaku.ERROR:" << error.errorString();
            qCritical() << result;
            return ;
        }

        _loadingOldDanmakus = true;
        QJsonObject json = document.object();
        QJsonArray danmakus = json.value("data").toObject().value("room").toArray();
        QDateTime time = QDateTime::currentDateTime();
        qint64 removeTime = time.toMSecsSinceEpoch() - us->removeDanmakuInterval;
        for (int i = 0; i < danmakus.size(); i++)
        {
            LiveDanmaku danmaku = LiveDanmaku::fromDanmakuJson(danmakus.at(i).toObject());
            if (danmaku.getTimeline().toMSecsSinceEpoch() < removeTime)
                continue;
            danmaku.transToDanmu();
            danmaku.setTime(time);
            danmaku.setNoReply();
            appendNewLiveDanmaku(danmaku);
        }
        _loadingOldDanmakus = false;
    });
}

void BiliLiveService::showFollowCountInAction(qint64 uid, QLabel* statusLabel, QAction *action, QAction *action2) const
{
    QString url = "https://api.bilibili.com/x/relation/stat?vmid=" + snum(uid);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, action, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << "获取粉丝失败：" << error.errorString();
            return ;
        }
        QJsonObject json = document.object();

        int code = json.value("code").toInt();
        if (code != 0)
        {
            statusLabel->setText(json.value("message").toString());
            if(statusLabel->text().isEmpty() && code == 403)
                statusLabel->setText("您没有权限");
            return ;
        }
        QJsonObject obj = json.value("data").toObject();
        QString following = TO_SIMPLE_NUMBER(obj.value("following").toInt()); // 关注
        QString follower = TO_SIMPLE_NUMBER(obj.value("follower").toInt()); // 粉丝
        // int whisper = obj.value("whisper").toInt(); // 悄悄关注（自己关注）
        // int black = obj.value("black").toInt(); // 黑名单（自己登录）
        if (!action2)
        {
            action->setText(QString("关注:%1,粉丝:%2").arg(following).arg(follower));
        }
        else
        {
            action->setText(QString("关注数:%1").arg(following));
            action2->setText(QString("粉丝数:%1").arg(follower));
        }
    });
    manager->get(*request);
}

void BiliLiveService::showViewCountInAction(qint64 uid, QLabel* statusLabel, QAction *action, QAction *action2, QAction *action3) const
{
    QString url = "https://api.bilibili.com/x/space/upstat?mid=" + snum(uid);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, action, [=](QNetworkReply* reply){
        QByteArray ba = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << "获取播放量失败：" << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
//        qDebug() << url;
//        qDebug() << json;

        int code = json.value("code").toInt();
        if (code != 0)
        {
            statusLabel->setText(json.value("message").toString());
            if(statusLabel->text().isEmpty() && code == 403)
                statusLabel->setText("您没有权限");
            return ;
        }
        QJsonObject data = json.value("data").toObject();
        int achive_view = data.value("archive").toObject().value("view").toInt();
        int article_view = data.value("article").toObject().value("view").toInt();
        int article_like = data.value("likes").toInt();

        if (!action2 && !action3)
        {
            QStringList sl;
            if (achive_view)
                sl << "播放:" + TO_SIMPLE_NUMBER(achive_view);
            if (article_view)
                sl << "阅读:" + TO_SIMPLE_NUMBER(article_view);
            if (article_like)
                sl << "点赞:" + TO_SIMPLE_NUMBER(article_like);

            if (sl.size())
                action->setText(sl.join(","));
            else
                action->setText("没有投稿");
        }
        else
        {
            action->setText("播放数:" + TO_SIMPLE_NUMBER(achive_view));
            if (action2)
                action2->setText("阅读数:" + TO_SIMPLE_NUMBER(article_view));
            if (action3)
                action3->setText("获赞数:" + TO_SIMPLE_NUMBER(article_like));
        }
    });
    manager->get(*request);
}

void BiliLiveService::showGuardInAction(qint64 roomId, qint64 uid, QLabel* statusLabel, QAction *action) const
{
    QString url = "https://api.live.bilibili.com/xlive/app-room/v2/guardTab/topList?roomid="
            +snum(roomId)+"&page=1&ruid="+snum(uid);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, action, [=](QNetworkReply* reply){
        QByteArray ba = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << "获取舰长失败：" << error.errorString();
            return ;
        }
        QJsonObject json = document.object();

        int code = json.value("code").toInt();
        if (code != 0)
        {
            statusLabel->setText(json.value("message").toString());
            if(statusLabel->text().isEmpty() && code == 403)
                statusLabel->setText("您没有权限");
            return ;
        }
        QJsonObject data = json.value("data").toObject();
        QJsonObject info = data.value("info").toObject();
        int num = info.value("num").toInt();
        action->setText("船员数:" + snum(num));
    });
    manager->get(*request);
}

void BiliLiveService::showPkLevelInAction(qint64 roomId, QLabel* statusLabel, QAction *actionUser, QAction *actionRank) const
{
    QString url = "https://api.live.bilibili.com/xlive/web-room/v1/index/getInfoByRoom?room_id="+snum(roomId);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, actionUser, [=](QNetworkReply* reply){
        QByteArray ba = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << "获取PK等级失败：" << error.errorString();
            return ;
        }
        QJsonObject json = document.object();

        int code = json.value("code").toInt();
        if (code != 0)
        {
            statusLabel->setText(json.value("message").toString());
            if(statusLabel->text().isEmpty() && code == 403)
                statusLabel->setText("您没有权限");
            return ;
        }
        QJsonObject data = json.value("data").toObject();
        QJsonObject anchor = data.value("anchor_info").toObject();
        QString uname = anchor.value("base_info").toObject().value("uname").toString();
        int level = anchor.value("live_info").toObject().value("level").toInt();
        actionUser->setText(uname + " " + snum(level));

        QJsonObject battle = data.value("battle_rank_entry_info").toObject();
        QString rankName = battle.value("rank_name").toString();
        actionRank->setText(rankName);
    });
    manager->get(*request);
}

void BiliLiveService::judgeUserRobotByFans(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs)
{
    int val = robotRecord->value("robot/" + snum(danmaku.getUid()), 0).toInt();
    if (val == 1) // 是机器人
    {
        if (ifIs)
            ifIs(danmaku);
        return ;
    }
    else if (val == -1) // 是人
    {
        if (ifNot)
            ifNot(danmaku);
        return ;
    }
    else
    {
        if (danmaku.getMedalLevel() > 0 || danmaku.getLevel() > 1) // 使用等级判断
        {
            robotRecord->setValue("robot/" + snum(danmaku.getUid()), -1);
            if (ifNot)
                ifNot(danmaku);
            return ;
        }
    }

    // 网络判断
    QString url = "https://api.bilibili.com/x/relation/stat?vmid=" + snum(danmaku.getUid());
    get(url, [=](QJsonObject json){
        int code = json.value("code").toInt();
        if (code != 0)
        {
            if (code == 403)
                showError("机器人判断粉丝", "您没有权限");
            else
                showError("机器人判断:粉丝", json.value("message").toString());
            return ;
        }
        QJsonObject obj = json.value("data").toObject();
        int following = obj.value("following").toInt(); // 关注
        int follower = obj.value("follower").toInt(); // 粉丝
        // int whisper = obj.value("whisper").toInt(); // 悄悄关注（自己关注）
        // int black = obj.value("black").toInt(); // 黑名单（自己登录）
        bool robot =  (following >= 100 && follower <= 5) || (follower > 0 && following > follower * 100); // 机器人，或者小号
//        qInfo() << "判断机器人：" << danmaku.getNickname() << "    粉丝数：" << following << follower << robot;
        if (robot)
        {
            // 进一步使用投稿数量判断
            judgeUserRobotByUpstate(danmaku, ifNot, ifIs);
        }
        else // 不是机器人
        {
            robotRecord->setValue("robot/" + snum(danmaku.getUid()), -1);
            if (ifNot)
                ifNot(danmaku);
        }
    });
}

void BiliLiveService::judgeUserRobotByUpstate(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs)
{
    QString url = "https://api.bilibili.com/x/space/upstat?mid=" + snum(danmaku.getUid());
    get(url, [=](QJsonObject json){
        int code = json.value("code").toInt();
        if (code != 0)
        {
            if (code == 403)
                showError("机器人判断:点击量", "您没有权限");
            else
                showError("机器人判断:点击量", json.value("message").toString());
            return ;
        }
        QJsonObject obj = json.value("data").toObject();
        int achive_view = obj.value("archive").toObject().value("view").toInt();
        int article_view = obj.value("article").toObject().value("view").toInt();
        int article_like = obj.value("article").toObject().value("like").toInt();
        bool robot = (achive_view + article_view + article_like < 10); // 机器人，或者小号
//        qInfo() << "判断机器人：" << danmaku.getNickname() << "    视频播放量：" << achive_view
//                 << "  专栏阅读量：" << article_view << "  专栏点赞数：" << article_like << robot;
        robotRecord->setValue("robot/" + snum(danmaku.getUid()), robot ? 1 : -1);
        if (robot)
        {
            if (ifIs)
                ifIs(danmaku);
        }
        else // 不是机器人
        {
            if (ifNot)
            {
                ifNot(danmaku);
            }
        }
    });
}

void BiliLiveService::judgeUserRobotByUpload(LiveDanmaku danmaku, DanmakuFunc ifNot, DanmakuFunc ifIs)
{
    QString url = "http://api.vc.bilibili.com/link_draw/v1/doc/upload_count?uid=" + snum(danmaku.getUid());
    get(url, [=](QJsonObject json){
        int code = json.value("code").toInt();
        if (code != 0)
        {
            if (code == 403)
                showError("机器人判断:投稿", "您没有权限");
            else
                showError("机器人判断:投稿", json.value("message").toString());
            return ;
        }
        QJsonObject obj = json.value("data").toObject();
        int allCount = obj.value("all_count").toInt();
        bool robot = (allCount <= 1); // 机器人，或者小号（1是因为有默认成为会员的相簿）
        qInfo() << "判断机器人：" << danmaku.getNickname() << "    投稿数量：" << allCount << robot;
        robotRecord->setValue("robot/" + snum(danmaku.getUid()), robot ? 1 : -1);
        if (robot)
        {
            if (ifIs)
                ifIs(danmaku);
        }
        else // 不是机器人
        {
            if (ifNot)
                ifNot(danmaku);
        }
    });
}

void BiliLiveService::setUrlCookie(const QString & url, QNetworkRequest * request)
{
    if (url.contains("bilibili.com") && !ac->browserCookie.isEmpty())
        request->setHeader(QNetworkRequest::CookieHeader, ac->userCookies);
}
