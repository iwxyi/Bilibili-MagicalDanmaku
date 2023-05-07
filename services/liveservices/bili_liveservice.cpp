#include <zlib.h>
#include <QFileDialog>
#include <QInputDialog>
#include <QTableView>
#include <QStandardItemModel>
#include <QClipboard>
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
    xliveHeartBeatTimer = new QTimer(this);
    xliveHeartBeatTimer->setInterval(60000);
    connect(xliveHeartBeatTimer, &QTimer::timeout, this, [=]{
        if (isLiving())
            sendXliveHeartBeatX();
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
            return ;

        // 开始工作
        if (isLiving())
            emit signalStartWork();

        if (!reconnect)
            return ;

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
void BiliLiveService::sendHeartPacket()
{
    QByteArray ba;
    ba.append("[object Object]");
    ba = BiliApiUtil::makePack(ba, OP_HEARTBEAT);
//    SOCKET_DEB << "发送心跳包：" << ba;
    liveSocket->sendBinaryMessage(ba);
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
    QString url = "https://api.bilibili.com/x/space/acc/info?mid=" + uid;
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

void BiliLiveService::getCookieAccount()
{
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
    get("https://api.bilibili.com/x/space/acc/info?mid=" + snum(talkerId), [=](MyJson info) {
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

void BiliLiveService::slotBinaryMessageReceived(const QByteArray &message)
{

}

void BiliLiveService::slotPkBinaryMessageReceived(const QByteArray &message)
{
    int operation = ((uchar)message[8] << 24)
            + ((uchar)message[9] << 16)
            + ((uchar)message[10] << 8)
            + ((uchar)message[11]);
    QByteArray body = message.right(message.length() - 16);
    SOCKET_INF << "pk操作码=" << operation << "  正文=" << (body.left(35)) << "...";

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(body, &error);
    QJsonObject json;
    if (error.error == QJsonParseError::NoError)
        json = document.object();

    if (operation == OP_SEND_MSG_REPLY) // 普通包
    {
        QString cmd;
        if (!json.isEmpty())
        {
            cmd = json.value("cmd").toString();
            qInfo() << "pk普通CMD：" << cmd;
            SOCKET_INF << json;
        }

        if (cmd == "NOTICE_MSG") // 全站广播（不用管）
        {
        }
        else if (cmd == "ROOM_RANK")
        {
        }
        else // 压缩弹幕消息
        {
            short protover = (message[6]<<8) + message[7];
            SOCKET_INF << "pk协议版本：" << protover;
            if (protover == 2) // 默认协议版本，zlib解压
            {
                uncompressPkBytes(body);
            }
        }
    }

//    delete[] body.data();
//    delete[] message.data();
    SOCKET_DEB << "PkSocket消息处理结束";
}

void BiliLiveService::uncompressPkBytes(const QByteArray &body)
{
    QByteArray unc = BiliApiUtil::zlibToQtUncompr(body.data(), body.size()+1);
    int offset = 0;
    short headerSize = 16;
    while (offset < unc.size() - headerSize)
    {
        int packSize = ((uchar)unc[offset+0] << 24)
                + ((uchar)unc[offset+1] << 16)
                + ((uchar)unc[offset+2] << 8)
                + (uchar)unc[offset+3];
        QByteArray jsonBa = unc.mid(offset + headerSize, packSize - headerSize);
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(jsonBa, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << s8("pk解析解压后的JSON出错：") << error.errorString();
            qCritical() << s8("pk包数值：") << offset << packSize << "  解压后大小：" << unc.size();
            qCritical() << s8(">>pk当前JSON") << jsonBa;
            qCritical() << s8(">>pk解压正文") << unc;
            return ;
        }
        QJsonObject json = document.object();
        QString cmd = json.value("cmd").toString();
        SOCKET_INF << "pk解压后获取到CMD：" << cmd;
        if (cmd != "ROOM_BANNER" && cmd != "ACTIVITY_BANNER_UPDATE_V2" && cmd != "PANEL"
                && cmd != "ONLINERANK")
            SOCKET_INF << "pk单个JSON消息：" << offset << packSize << QString(jsonBa);
        handlePkMessage(json);

        offset += packSize;
    }
}

void BiliLiveService::handlePkMessage(QJsonObject json)
{
    QString cmd = json.value("cmd").toString();
    qInfo() << s8(">pk消息命令：") << cmd;
    if (cmd == "DANMU_MSG" || cmd.startsWith("DANMU_MSG:")) // 收到弹幕
    {
        if (!pkMsgSync || (pkMsgSync == 1 && !pkVideo))
            return ;
        QJsonArray info = json.value("info").toArray();
        QJsonArray array = info[0].toArray();
        qint64 textColor = array[3].toInt(); // 弹幕颜色
        qint64 timestamp = static_cast<qint64>(array[4].toDouble()); // 13位
        QString msg = info[1].toString();
        QJsonArray user = info[2].toArray();
        qint64 uid = static_cast<qint64>(user[0].toDouble());
        QString username = user[1].toString();
        QString unameColor = user[7].toString();
        int level = info[4].toArray()[0].toInt();
        QJsonArray medal = info[3].toArray();
        int medal_level = 0;

        // 是否是串门的
        bool toView = pkBattleType &&
                ((!oppositeAudience.contains(uid) && myAudience.contains(uid))
                 || (!pkRoomId.isEmpty() && medal.size() >= 4 &&
                     snum(static_cast<qint64>(medal[3].toDouble())) == ac->roomId));

        // !弹幕的时间戳是13位，其他的是10位！
        qInfo() << s8("pk接收到弹幕：") << username << msg << QDateTime::fromMSecsSinceEpoch(timestamp);

        // 添加到列表
        QString cs = QString::number(textColor, 16);
        while (cs.size() < 6)
            cs = "0" + cs;
        LiveDanmaku danmaku(username, msg, uid, level, QDateTime::fromMSecsSinceEpoch(timestamp),
                                                 unameColor, "#"+cs);
        if (medal.size() >= 4)
        {
            medal_level = medal[0].toInt();
            danmaku.setMedal(snum(static_cast<qint64>(medal[3].toDouble())),
                    medal[1].toString(), medal_level, medal[2].toString());
        }
        /*if (noReplyMsgs.contains(msg) && snum(uid) == cookieUid)
        {
            danmaku.setNoReply();
            noReplyMsgs.removeOne(msg);
        }
        else
            minuteDanmuPopular++;*/
        danmaku.setToView(toView);
        danmaku.setPkLink(true);
        emit signalAppendNewLiveDanmaku(danmaku);

        triggerCmdEvent("PK_DANMU_MSG", danmaku.with(json));
    }
    else if (cmd == "SEND_GIFT") // 有人送礼
    {
        if (!pkMsgSync || (pkMsgSync == 1 && !pkVideo))
            return ;
        QJsonObject data = json.value("data").toObject();
        int giftId = data.value("giftId").toInt();
        int giftType = data.value("giftType").toInt(); // 不知道是啥，金瓜子1，银瓜子（小心心、辣条）5？
        QString giftName = data.value("giftName").toString();
        QString username = data.value("uname").toString();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        int num = data.value("num").toInt();
        qint64 timestamp = static_cast<qint64>(data.value("timestamp").toDouble()); // 秒
        timestamp = QDateTime::currentSecsSinceEpoch(); // *不管送出礼物的时间，只管机器人接收到的时间
        QString coinType = data.value("coin_type").toString();
        int totalCoin = data.value("total_coin").toInt();

        qInfo() << s8("接收到送礼：") << username << giftId << giftName << num << s8("  总价值：") << totalCoin << coinType;
        QString localName = us->getLocalNickname(uid);
        /*if (!localName.isEmpty())
            username = localName;*/
        LiveDanmaku danmaku(username, giftId, giftName, num, uid, QDateTime::fromSecsSinceEpoch(timestamp), coinType, totalCoin);
        QString anchorRoomId;
        if (!data.value("medal_info").isNull())
        {
            QJsonObject medalInfo = data.value("medal_info").toObject();
            anchorRoomId = snum(qint64(medalInfo.value("anchor_room_id").toDouble())); // !注意：这个一直为0！
            QString anchorUname = medalInfo.value("anchor_uname").toString(); // !注意：也是空的
            int guardLevel = medalInfo.value("guard_level").toInt();
            int isLighted = medalInfo.value("is_lighted").toInt();
            int medalColor = medalInfo.value("medal_color").toInt();
            int medalColorBorder = medalInfo.value("medal_color_border").toInt();
            int medalColorEnd = medalInfo.value("medal_color_end").toInt();
            int medalColorStart = medalInfo.value("medal_color_start").toInt();
            int medalLevel = medalInfo.value("medal_level").toInt();
            QString medalName = medalInfo.value("medal_name").toString();
            QString spacial = medalInfo.value("special").toString();
            QString targetId = snum(qint64(medalInfo.value("target_id").toDouble())); // 目标用户ID
            if (!medalName.isEmpty())
            {
                QString cs = QString::number(medalColor, 16);
                while (cs.size() < 6)
                    cs = "0" + cs;
                danmaku.setMedal(anchorRoomId, medalName, medalLevel, cs, anchorUname);
            }
        }

        // 是否是串门的
        bool toView = pkBattleType &&
                ((!oppositeAudience.contains(uid) && myAudience.contains(uid))
                 || (!pkRoomId.isEmpty() && anchorRoomId == ac->roomId));

        danmaku.setPkLink(true);

        if (toView) // 自己这边过去送礼物，居心何在！
            emit signalAppendNewLiveDanmaku(danmaku);

        triggerCmdEvent("PK_" + cmd, danmaku.with(data));
    }
    else if (cmd == "INTERACT_WORD")
    {
        if (!pkChuanmenEnable) // 可能是中途关了
            return ;
        QJsonObject data = json.value("data").toObject();
        int msgType = data.value("msg_type").toInt(); // 1进入直播间，2关注，3分享直播间，4特别关注
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString username = data.value("uname").toString();
        qint64 timestamp = static_cast<qint64>(data.value("timestamp").toDouble());
        bool isadmin = data.value("isadmin").toBool();
        QString unameColor = data.value("uname_color").toString();
        bool isSpread = data.value("is_spread").toBool();
        QString spreadDesc = data.value("spread_desc").toString();
        QString spreadInfo = data.value("spread_info").toString();
        QJsonObject fansMedal = data.value("fans_medal").toObject();
        QString roomId = snum(qint64(data.value("room_id").toDouble()));
        bool toView = pkBattleType &&
                ((!oppositeAudience.contains(uid) && myAudience.contains(uid))
                 || (!pkRoomId.isEmpty() &&
                     snum(static_cast<qint64>(fansMedal.value("anchor_roomid").toDouble())) == ac->roomId));
        bool attentionToMyRoom = false;
        if (!toView) // 不是自己方过去串门的
        {
            if (roomId == ac->roomId && msgType == 2) // 在对面关注当前主播
                attentionToMyRoom = true;
            else
                if (!pkMsgSync || (pkMsgSync == 1 && !pkVideo))
                    return ;
        }
        if (toView || attentionToMyRoom)
        {
            if (!cmAudience.contains(uid))
                cmAudience.insert(uid, timestamp);
        }

        // qInfo() << s8("pk观众互动：") << username << spreadDesc;
        QString localName = us->getLocalNickname(uid);
        LiveDanmaku danmaku(username, uid, QDateTime::fromSecsSinceEpoch(timestamp), isadmin,
                            unameColor, spreadDesc, spreadInfo);
        danmaku.setMedal(snum(static_cast<qint64>(fansMedal.value("anchor_roomid").toDouble())),
                         fansMedal.value("medal_name").toString(),
                         fansMedal.value("medal_level").toInt(),
                         QString("#%1").arg(fansMedal.value("medal_color").toInt(), 6, 16, QLatin1Char('0')),
                         "");
        danmaku.setToView(toView);
        danmaku.setPkLink(true);

        if (attentionToMyRoom)
        {
            danmaku.transToAttention(timestamp);
            localNotify("对面的 " + username + " 关注了本直播间 " + snum(uid));
            triggerCmdEvent("ATTENTION_ON_OPPOSITE", danmaku.with(data));
        }
        else if (msgType == 1)
        {
            if (toView)
            {
                localNotify(username + " 跑去对面串门 " + snum(uid)); // 显示一个短通知，就不作为一个弹幕了
                triggerCmdEvent("CALL_ON_OPPOSITE", danmaku.with(data));
            }
            triggerCmdEvent("PK_" + cmd, danmaku.with(data));
        }
        else if (msgType == 2)
        {
            danmaku.transToAttention(timestamp);
            if (toView)
            {
                localNotify(username + " 关注了对面直播间 " + snum(uid)); // XXX
                triggerCmdEvent("ATTENTION_OPPOSITE", danmaku.with(data));
            }
            triggerCmdEvent("PK_ATTENTION", danmaku.with(data));
        }
        else if (msgType == 3)
        {
            danmaku.transToShare();
            if (toView)
            {
                localNotify(username + " 分享了对面直播间 " + snum(uid)); // XXX
                triggerCmdEvent("SHARE_OPPOSITE", danmaku.with(data));
            }
            triggerCmdEvent("PK_SHARE", danmaku.with(data));
        }
        // appendNewLiveDanmaku(danmaku);
    }
    else if (cmd == "CUT_OFF")
    {
        localNotify("对面直播间被超管切断");
    }
}
