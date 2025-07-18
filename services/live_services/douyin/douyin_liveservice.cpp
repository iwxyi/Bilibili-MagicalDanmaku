#include "douyin_liveservice.h"
#include "fileutil.h"
#include "stringutil.h"
#include "douyinsignaturehelper.h"

DouyinLiveService::DouyinLiveService(QObject *parent) : LiveServiceBase(parent)
{
    initWS();
}

bool DouyinLiveService::isLiving() const
{
    return liveStatus == 2;
}

QString DouyinLiveService::getLiveStatusStr() const
{
    switch (liveStatus)
    {
    case 0:
        return "未开播";
    case 1:
        return "准备中";
    case 2:
        return "直播中";
    case 3:
        return "暂停中";
    case 4:
        return "已下播";
    default:
        return "未知状态" + snum(liveStatus);
    }
}

void DouyinLiveService::initWS()
{
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
        // qDebug() << "收到二进制消息：" << message;
        onBinaryMessageReceived(message);
    });
}

void DouyinLiveService::startConnect()
{
    getRoomInfo(true);
}

MyJson parseLiveHtml(QString html)
{
    if (!html.startsWith("<!DOCTYPE html>"))
        qWarning() << "!抖音返回的似乎不是正确的目标内容";
    QRegularExpression re(R"(<script\snonce="\S+?"\s>self\.__pace_f\.push\(\[1,"[a-z]?:\[\\"\$\\",\\"\$L\d+\\",null,(.+?)\]\\n"\]\)<\/script>)");
    // 循环匹配
    MyJson dataFull;
    QRegularExpressionMatchIterator i = re.globalMatch(html);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString json = match.captured(1);
        if (!json.contains("\"state"))
        {
            continue;
        }

        // 2. 一系列正则替换，修正为标准 JSON
        struct RegReplace {
            QRegularExpression reg;
            QString str;
        };
        std::vector<RegReplace> regList = {
            { QRegularExpression(R"(\\{1,7}\")"), QStringLiteral("\"") },
            { QRegularExpression(R"("\{)"), QStringLiteral("{") },
            { QRegularExpression(R"(\}")"), QStringLiteral("}") },
            { QRegularExpression(R"("\[(.*)]\")"), QStringLiteral("[$1]") },
            { QRegularExpression(R"(([^:,{\[])\\"([^:,}\]]))"), QStringLiteral(R"($1"$2)") }
        };
        for (const auto& reg : regList) {
            json.replace(reg.reg, reg.str);
        }

        // 3. 解析 JSON
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject())
        {
            continue;
        }
        MyJson data = doc.object();
        dataFull.merge(data);
        break;
    }

    return dataFull;
}

/// 这里就需要Cookie了，否则获取到的是无法解析的字符集
/// 第一次不带 __ac_nonce，需要 SetCookie 之后进行第二次请求
void DouyinLiveService::getRoomInfo(bool reconnect, int reconnectCount)
{
    static bool getFirst = true;
    qInfo() << "获取弹幕信息";
    if (ac->browserCookie.isEmpty())
        qWarning() << "未设置Cookie，获取到的信息可能不准确";
    QString url = "https://live.douyin.com/" + ac->roomId;
    get(url, [=](QString html) {
        qDebug() << "返回的HTML：" << html.length() << "字符";
#ifdef QT_DEBUG
        writeTextFile(rt->dataPath + "dy_live_room.html", html);
#endif
        MyJson json = parseLiveHtml(html);
        if (json.isEmpty())
        {
            if (getFirst)
            {
                qInfo() << "第一次无法获取弹幕信息，尝试在设置Cookie后再次获取";
                getFirst = false;
                QTimer::singleShot(10, this, [=]{
                    getRoomInfo(reconnect, reconnectCount);
                });
                return;
            }
            qWarning() << "无法解析弹幕信息，url=" << url;
        }

        /// 解析数据
        MyJson state = json.o("state");

        // 直播间信息
        MyJson roomInfo = state.o("roomStore").o("roomInfo");
        ac->roomRid = roomInfo.s("roomId"); // 和入口的roomid不是同一个，而且很长
        MyJson room = roomInfo.o("room");
        ac->roomTitle = room.s("title");
        QJsonArray adminUserIds = room.a("admin_user_ids"); // UID列表
        DouyinLiveStatus roomStatus = (DouyinLiveStatus)room.i("status");
        this->liveStatus = (int)roomStatus;
        QJsonArray roomCoverList = room.o("cover").a("url_list");
        QString roomCover = roomCoverList.size() ? roomCoverList.first().toString() : "";
        int likeCount = room.i("like_count"); // 点赞数
        ac->watchedShow = room.o("room_view_stats").s("display_short");
        qInfo() << "直播间信息：" << ac->roomTitle << ac->roomId << "(" + ac->roomRid + ")" << getLiveStatusStr();

        // 分区信息
        MyJson partitionRoadMap = roomInfo.o("partition_road_map"); // 可能为空
        MyJson partition = partitionRoadMap.o("partition");
        ac->parentAreaId = partition.s("id_str");
        ac->parentAreaName = partition.s("title");
        MyJson subPartition = partitionRoadMap.o("sub_partition");
        partition = subPartition.o("partition");
        ac->areaId = partition.s("id_str");
        ac->areaName = partition.s("title");
        qInfo() << "分区信息：" << ac->parentAreaName + "/" + ac->areaName;

        // 主播信息
        MyJson anchor = roomInfo.o("anchor");
        ac->upUid = anchor.s("id_str");
        ac->upSecUid = anchor.s("sec_uid");
        QJsonArray avatarThumb = anchor.o("avatar_thumb").a("url_list");
        QString avatar = avatarThumb.size() ? avatarThumb.first().toString() : "";
        ac->upName = anchor.s("nickname");
        qInfo() << "主播信息：" << ac->upName << ac->upUid << ac->upSecUid;

        // 自己的信息
        MyJson userStore = state.o("userStore");
        MyJson odin = userStore.o("odin");
        QString userId = odin.s("user_id"); // 抖音内部的主键ID，不可变
        QString userUniqueId = odin.s("user_unique_id"); // 这个才是抖音号，可自定义
        int userType = odin.i("user_type"); // 匿名是12
        ac->cookieUid = userUniqueId;
        qInfo() << "用户信息：用户ID" << userId << userUniqueId << " userType:" << userType;

        // 获取主播与机器人账号的信息
        getRobotInfo();
        getUpInfo();

        emit signalRoomIdChanged(ac->roomId);
        emit signalUpUidChanged(ac->upUid);
        emit signalRoomInfoChanged();
        emit signalLiveStatusChanged(getLiveStatusStr());
        emit signalLikeChanged(likeCount);

        getDanmuInfo();
        downloadRoomCover(roomCover);
        downloadUpCover(avatar);

        triggerCmdEvent("GET_ROOM_INFO", LiveDanmaku().with(json));
    });
}

/// 动态的cookie字段：
/// __ac_signature=xxx.xx;
/// xg_device_score=7.666742553508002;
/// IsDouyinActive=true;
/// live_can_add_dy_2_desktop=%221%22
void DouyinLiveService::setUrlCookie(const QString &url, QNetworkRequest *request)
{
    if (url.contains("douyin.com") && !ac->browserCookie.isEmpty())
        request->setHeader(QNetworkRequest::CookieHeader, ac->userCookies);
}

void DouyinLiveService::autoAddCookie(QList<QNetworkCookie> cookies)
{
    emit signalAutoAddCookie(cookies);
}

/// 获取初次连接信息
void DouyinLiveService::getDanmuInfo()
{
    qInfo() << "获取连接信息";
    // QByteArray ba = imFetch(ac->roomRid, ac->cookieUid);
    qint64 timestamp10 = QDateTime::currentSecsSinceEpoch();
    QString cursor = QString("r-7497180536918546638_d-1_u-1_fh-7497179772733760010_t-%1").arg(timestamp10);
    QString internalExt = QString("internal_src:dim|wss_push_room_id:%2|wss_push_did:%3|first_req_ms:%1|fetch_time:%1|seq:1|wss_info:0-%1-0-0|wrds_v:7497180515443673855")
                                .arg(timestamp10).arg(ac->roomRid).arg(ac->cookieUid);
    imPush(cursor, internalExt);
}

/// 通过在线库进行计算。
/// !已经失效，计算出来的库不准确！
QString DouyinLiveService::getSignature(QString roomId, QString uniqueId)
{
    MyJson signatureObj = postToJson("https://dy.nsapps.cn/signature", QString(R"({"roomId": "%1", "uniqueId": "%2"})").arg(roomId).arg(uniqueId).toUtf8(),
                                     "", QList<QPair<QString, QString>>{{"content-type", "application/json"}});
    QString signature = signatureObj.data().s("signature");
    qInfo() << "signature:" << signature;
    return signature;
}

/// 组建初次连接的信息
/// 主要是为了获取 cursor、internalExt 这两个值
/// !返回一直是 json code=0 的返回值，有问题
QByteArray DouyinLiveService::imFetch(QString roomId, QString uniqueId)
{
    // 默认参数
    QStringList params{
        "aid", "6383",
        "app_name", "douyin_web",
        "browser_language", "zh-CN",
        "browser_name", "Mozilla",
        "browser_online", "true",
        "browser_platform", "Win32",
        "browser_version", "5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36",
        "compress", "gzip",
        "cookie_enabled", "true",
        "cursor", "",
        "device_platform", "web",
        "did_rule", "3",
        "endpoint", "live_pc",
        "heartbeatDuration", "0",
        "host", "https://live.douyin.com",
        "identity", "audience",
        "insert_task_id", "",
        "internal_ext", "",
        "last_rtt", "0",
        "live_id", "1",
        "live_reason", "",
        "need_persist_msg_count", "15",
        "resp_content_type", "protobuf",
        "screen_height", "1080",
        "screen_width", "1920",
        "support_wrds", "1",
        "tz_name", "Asia/Shanghai",
        "update_version_code", "1.0.14-beta.0",
        "version_code", "180800",       // 版本号
        "webcast_sdk_version", "1.0.14-beta.0"
    };

    // 请求需要一些关键参数：msToken、a_bogus
    QString msToken = getRandomKey(182);
    msToken = "cBgW7_8D2uXhhUXswTHAzp3REMGP67Ap-VEarQHwuu8ixtEtii4A-yZE5B-XEFDUa51Do27Ym3Ob7DbFNs-BoY6a_IVog5MfNgXfYYFBvIZzlqNmod7TKxzGUfIW0D-sypQKVxxGmFqq-ElXNAvhi3PyLCixJUSaQZX_TG0bA8YPotTCA7P7dBM%3D";
    params << "room_id" << roomId << "user_unique_id" << uniqueId << "msToken" << msToken;

    // 一个加密参数，须通过上侧 params 参数计算，感兴趣自己去逆向，这里不解析，不一定验证
    QString aBogus = "E7sjkw6LDZ85ed%2FtYCjoy4dlVUglrs8yieiORKIK9PK%2FGHlP0RPRhPaiaxzKm%2FAzYSB0ho37vEzAbxVc%2FxUk1ZnkwmpfSMzWOz%2FV9Smo%2FqZDGBk2I3b%2FeGbEwi4PUS4PKACvE5E160UqILQfprVolMKy7Aen5ub8Tqp9d3UZexK15SDqpZ18unjZi7tq9j%3D%3D";
    params << "live_pc" << roomId << "a_bogus" << aBogus;

    QString paramsStr;
    for (int i = 0; i < params.size(); i += 2)
    {
        paramsStr += params[i] + "=" + urlDecode(params[i + 1]) + "&";
    }
    paramsStr.chop(1);

    QString url = "https://live.douyin.com/webcast/im/fetch/?" + paramsStr;
    QByteArray ba = getToBytes(url);
    qDebug() << ba;
    return ba;
}

void DouyinLiveService::imPush(QString cursor, QString internalExt)
{
    QString roomId = ac->roomRid;
    QString uniqueId = ac->cookieUid;
    QString serverUrl = "wss://webcast5-ws-web-lf.douyin.com/webcast/im/push/v2/";
    // QString signature = getSignature(roomId, uniqueId);

    DouyinSignatureHelper helper;
    QString signature = helper.getSignature(roomId, uniqueId);

    QStringList params{
        "aid", "6383",
        "app_name", "douyin_web",
        "browser_language", "zh-CN",
        "browser_name", "Mozilla",
        "browser_online", "true",
        "browser_platform", "Win32",
        "browser_version", "5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36",
        "compress", "gzip",
        "cookie_enabled", "true",
        "device_platform", "web",
        "did_rule", "3",
        "endpoint", "live_pc",
        "heartbeatDuration", "0",
        "host", "https://live.douyin.com",
        "identity", "audience",
        "im_path", "/webcast/im/fetch/",
        "insert_task_id", "",
        "last_rtt", "0",
        "live_id", "1",
        "live_reason", "",
        "maxCacheMessageNumber", "20",
        "need_persist_msg_count", "15",
        "resp_content_type", "protobuf",
        "room_id", roomId,
        "screen_height", "1080",
        "screen_width", "1920",
        "support_wrds", "1",
        "tz_name", "Asia/Shanghai",
        "update_version_code", "1.0.14-beta.0",
        "user_unique_id", uniqueId,
        "version_code", "180800",       // 版本号
        "webcast_sdk_version", "1.0.14-beta.0",
        "cursor", cursor,
        "internal_ext", internalExt,
        "signature", signature,
    };

    QString paramsStr;
    for (int i = 0; i < params.size(); i += 2)
    {
        paramsStr += params[i] + "=" + urlDecode(params[i + 1]) + "&";
    }
    paramsStr.chop(1);

    QString backupUrl = serverUrl;
    backupUrl.replace("lq", "lf");

    // 开始连接
    QString url = serverUrl + "?" + paramsStr;
    qInfo() << "开始连接：" << url;

    // 需要带相同的Cookie，否则会Refuse。但是实测可以用其他账号的cookie
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Cookie", ac->browserCookie.toUtf8());
    request.setHeader(QNetworkRequest::UserAgentHeader, "5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36");
    request.setRawHeader("Referer", "https://live.douyin.com/");
    request.setRawHeader("Origin", "https://live.douyin.com");

    liveSocket->open(request);
}

void DouyinLiveService::getAccountInfo(const QString &uid, NetJsonFunc func)
{
    if (uid.isEmpty())
        return;

    QStringList params{
        "aid", "6383",
        "app_name", "douyin_web",
        "browser_language", "zh-CN",
        "browser_name", "Mozilla",
        "browser_platform", "Win32",
        "browser_version", "5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36",
        "cookie_enabled", "true",
        "device_platform", "web",
        "language", "zh-CN",
        "live_id", "1",
        "live_reason", "",
        "screen_height", "1080",
        "screen_width", "1920",
        "target_uid", uid,
        // "sec_target_uid", "", // uid和secId只需要一个即可
        "anchor_id", ac->upUid,
        // "sec_anchor_id", "",
        "current_room_id", ac->roomId // 哪个ID都行
    };

    QString paramsStr;
    for (int i = 0; i < params.size(); i += 2)
    {
        paramsStr += params[i] + "=" + urlDecode(params[i + 1]) + "&";
    }
    paramsStr.chop(1);

    get("https://live.douyin.com/webcast/user/profile/?" + paramsStr, [=](MyJson json) {
        if (json.i("status_code") != 0)
        {
            qWarning() << json;
            QString msg = json.data().s("prompts");
            return showError("获取账号信息" + uid, "状态不为0：" + snum(json.i("status_code")) + "\n" + msg);
        }
        func(json.data());
    });

}

void DouyinLiveService::getRobotInfo()
{
    getAccountInfo(ac->cookieUid, [=](MyJson data) {

    });
}

void DouyinLiveService::getUpInfo()
{
    getAccountInfo(ac->upUid, [=](MyJson data) {
        MyJson userData = data.o("user_data");
        MyJson followInfo = userData.o("follow_info");
        int followerCount = followInfo.i("follower_count");
        int followingCount = followInfo.i("following_count");
        emit signalFansCountChanged(followerCount);
    });
}
