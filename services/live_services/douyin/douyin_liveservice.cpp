#include "douyin_liveservice.h"
#include "fileutil.h"
#include "stringutil.h"
#include "douyinsignaturehelper.h"
#include "douyinsignatureabogus.h"

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

QStringList DouyinLiveService::getCommonParams() const
{

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
#ifdef QT_DEBUG
        writeTextFile(rt->dataPath + "dy_live_room_info.json", json.toBa());
#endif
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
        MyJson headerUserInfo = userStore.o("headerUserInfo");
        bool isLogin = headerUserInfo.i("is_login");
        if (!isLogin)
            qWarning() << "未登录，无法获取用户信息";
        MyJson userInfo = headerUserInfo.o("info");
        if (!userInfo.isEmpty())
        {
            QString nickname = userInfo.s("nickname");
            QString uid = userInfo.s("uid"); // 抖音内部的主键ID，不可变
            QString shortId = userInfo.s("shortId");
            QString secUid = userInfo.s("secUid");
            QString uniqueId = userInfo.s("uniqueId"); // 自己输入的唯一ID，不和其他人重复，但可更改
            qint64 roomId = userInfo.l("roomId"); // 直播间？没有的话为0和str空
            QString avatarUrl = userInfo.s("avatarUrl");
            int awemeCount = userInfo.i("awemeCount"); // 作品数量
            int favoritingCount = userInfo.i("favoritingCount"); // 我的喜欢
            int followStatus = userInfo.i("followStatus"); // 关注状态，0未关注，1已关注，2互相关注
            int followerCount = userInfo.i("followerCount"); // 粉丝数
            int followingCount = userInfo.i("followingCount"); // 关注数
            QString desc = userInfo.s("desc"); // 签名
            QJsonArray descExtra = userInfo.a("descExtra"); // 如果desc有@的话，这是@对象的列表，没必要解析

            ac->cookieUid = uid;
            ac->cookieSecUid = secUid;
            ac->cookieUname = nickname;
            qInfo() << "访问直播间的机器人账号：" << nickname << uid;
        }
        else
        {
            MyJson odin = userStore.o("odin"); // 总是存在
            QString userId = odin.s("user_id"); // 抖音内部的主键ID，不可变
            QString userUniqueId = odin.s("user_unique_id"); // 这个才是抖音号，可自定义
            int userType = odin.i("user_type"); // 匿名是12
            ac->cookieUid = userUniqueId;
            qInfo() << "未登录用户信息：用户ID" << userId << userUniqueId << " userType:" << userType;
        }
        
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
    if (url.contains("douyin.com"))
    {
        if (!ac->browserCookie.isEmpty())
        {
            request->setHeader(QNetworkRequest::CookieHeader, ac->userCookies);
        }
        request->setRawHeader("Referer", "https://live.douyin.com/");
        request->setRawHeader("Origin", "https://live.douyin.com");
    }
}

void DouyinLiveService::autoAddCookie(QList<QNetworkCookie> cookies)
{
    emit signalAutoAddCookie(cookies);
}

/// 获取初次连接信息
void DouyinLiveService::getDanmuInfo()
{
    if (this->gettingDanmu)
    {
        qDebug() << "正在获取弹幕连接信息中...";
        return;
    }

    qInfo() << "获取连接信息";
    this->gettingDanmu = true;
    // QByteArray ba = imFetch(ac->roomRid, ac->cookieUid);
    qint64 timestamp10 = QDateTime::currentSecsSinceEpoch();
    QString cursor = QString("r-7497180536918546638_d-1_u-1_fh-7497179772733760010_t-%1").arg(timestamp10);
    QString internalExt = QString("internal_src:dim|wss_push_room_id:%2|wss_push_did:%3|first_req_ms:%1|fetch_time:%1|seq:1|wss_info:0-%1-0-0|wrds_v:7497180515443673855")
                                .arg(timestamp10).arg(ac->roomRid).arg(ac->cookieUid);
    imPush(cursor, internalExt);
}

/// 通过本地 Python HTTP 服务器进行计算。
/// 需要先启动 douyin_signature_server.py
QString DouyinLiveService::getSignature(QString roomId, QString uniqueId)
{
    QString url = QString("http://212.64.18.225:5531/signature?roomId=%1&uniqueId=%2")
                     .arg(roomId).arg(uniqueId);
    
    MyJson response = getToJson(url);
    if (response.b("success") == true) {
        QString signature = response.s("signature");
        qDebug() << "在线签名返回：" << response;
        if (signature.isEmpty())
        {
            qWarning() << "获取signature为空：" << response << url;
        }
        else
        {
            qInfo() << "signature:" << signature << "   args:" << roomId << uniqueId;
        }
        return signature;
    } else {
        qWarning() << "获取signature失败：" << response.s("error") << url;
        qWarning() << "请确保已启动本地 signature 服务器 (python douyin_signature_server.py)";
        return "";
    }
}

/// [废弃，无法使用]
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

/// 连接直播间WebSocket
void DouyinLiveService::imPush(QString cursor, QString internalExt)
{
    QString roomId = ac->roomRid;
    QString uniqueId = ac->cookieUid;
    QString serverUrl = "wss://webcast5-ws-web-lf.douyin.com/webcast/im/push/v2/";

    // DouyinSignatureHelper helper;
    // QString signature = helper.getSignature(roomId, uniqueId);
    // qDebug() << "浏览器获取Signature:" << signature;
    QString signature = getSignature(roomId, uniqueId);
    qDebug() << "获取Signature：" << signature;

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

    QString paramsStr = toUrlParam(params);

    // 开始连接
    QString url = serverUrl + "?" + paramsStr;
    url.replace("webcast3-ws-web-lq", "webcast5-ws-web-lf");
    qInfo() << "开始连接：" << url;

    // 需要带相同的Cookie，否则会Refuse。但是实测可以用其他账号的cookie
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Cookie", ac->browserCookie.toUtf8());
    request.setHeader(QNetworkRequest::UserAgentHeader, "5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36");
    request.setRawHeader("Referer", "https://live.douyin.com/");
    request.setRawHeader("Origin", "https://live.douyin.com");

    liveSocket->open(request);
    gettingDanmu = false;
}

void DouyinLiveService::getCookieAccount()
{
    if (gettingUser)
    {
        qDebug() << "正在获取Cookie账号中...";
        return;
    }
    gettingUser = true;
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
        "cookir_enabled", "true",
        "room_id", "0",
        "msToken", urlEncodePercent(getRandomKey(120))
    };

    QString paramsStr = toUrlParam(params);

    DouyinSignatureAbogus signer;
    QString xBogus = signer.signDetail(paramsStr, getUserAgent());

    QString url = "https://live.douyin.com/webcast/user/me/?" + paramsStr;
    url += "&a_bogus=" + urlEncodePercent(xBogus);

    qInfo() << "获取Cookie账号";
    get(url, [=](MyJson json) {
        if (json.i("status_code") != 0)
        {
            QString msg = json.data().s("prompts");
            return showError("获取Cookie账号", "状态不为0：" + snum(json.i("status_code")) + "\n" + msg);
        }

        MyJson data = json.data();
        qint64 id = data.l("id");
        QString sec_uid = data.s("sec_uid");
        QString webcast_uid = data.s("webcast_uid");
        qint64 short_id = data.l("short_id");
        QString display_id = data.s("display_id");
        QString nickname = data.s("nickname");
        QString avatar = data.o("avatar_thumb").a("url_list").first().toString();
        QString signature = data.s("signature");
        QString location_city = data.s("location_city");

        ac->cookieUid = snum(id);
        ac->cookieSecUid = sec_uid;
        ac->cookieUname = nickname;
        qInfo() << "当前登录的Cookie账号：" << id << nickname;

        downloadRobotCover(avatar);

        emit signalRobotAccountChanged();

        // 登录后websocket重新连接
        getDanmuInfo();
        }, [=](QString s){
            gettingUser = false;
        });
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

    QString paramsStr = toUrlParam(params);

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

/// 获取机器人信息
/// 没有必要，要获取的都在直播间信息里了
void DouyinLiveService::getRobotInfo()
{
    // getAccountInfo(ac->cookieUid, [=](MyJson data) { });
    emit signalRobotAccountChanged();
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

void DouyinLiveService::sendMsg(const QString &msg, const QString &cookie)
{
    sendRoomMsg(ac->roomRid, msg, cookie);
}

/// 发送弹幕
/// !注意点：
/// - 返回403的情况，会掉该Cookie登录的账号，需要重新登录。
/// - 1. 可能是编码问题，比如content的中文必须都用百分号的编码，a_bogus计算的参数也是url编码后的，再添加url编码后的a_bogus
/// - 2. 注意User-Agent，发送弹幕的UA必须和登录的平台一样（浏览器Cookie中拿到），但是params中的browser参数不重要，不会进行判断
void DouyinLiveService::sendRoomMsg(QString roomRid, const QString &msg, const QString &cookie)
{
    if (us->localMode)
    {
        localNotify("发送弹幕 -> " + msg + "  (" + snum(msg.length()) + ")");
        return ;
    }

    QStringList params{
        "aid", "6383",
        "app_name", "douyin_web",
        "live_id", "1",
        "device_platform", "web",
        "language", "zh-CN",
        "enter_from", "web_live",
        "cookie_enabled", "true",
        "screen_width", "1360",
        "screen_height", "908",
        "browser_language", "zh-CN",
        "browser_platform", "Win32",
        "browser_name", "Mozilla",
        "browser_version", "5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36",
        "room_id", roomRid,
        "content", msg,
        "type", "0",
        "rtf_content", "",
        "msToken", getRandomKey(120)
    };

    QString paramsStr = toUrlParam(params);

    DouyinSignatureAbogus signer;
    QString xBogus = signer.signDetail(paramsStr, getUserAgent());

    QString url = "https://live.douyin.com/webcast/room/chat/?" + paramsStr;
    url += "&a_bogus=" + urlEncodePercent(xBogus);
    qDebug() << "发送弹幕：" << url;

    get(url, [=](QString str) {
        MyJson json(str.toUtf8());
        if (json.hasError())
        {
            qWarning() << "无法解析弹幕返回：" << str;
            return;
        }
        if (json.i("status_code") != 0)
        {
            qWarning() << "弹幕发送失败：" << json;
            QString msg = json.data().s("prompts");
            return showError("弹幕发送失败", "状态不为0：" + snum(json.i("status_code")) + "\n" + msg);
        }

        MyJson data = json.data();
        // qint64 id = data.l("id"); // =0?
        qint64 msg_id = data.l("msg_id"); // 唯一ID
        QString content = data.s("content"); // 发送的消息内容

        MyJson user = data.o("user");
        qint64 uid = user.l("id");
        qint64 short_id = user.l("short_id");
        QString nickname = user.s("nickname");
        int gender = user.i("gender");
        QString signature = user.s("signature");
        int level = user.i("level"); // =0?
        MyJson avatar_thumb = user.o("avatar_thumb");
        QString avatar = avatar_thumb.a("url_list").first().toString();
        bool verified = user.b("verified"); // true
        QString city = user.s("city");
        int status = user.i("status"); // 1
        qint64 create_time = user.l("create_time"); // =0
        qint64 modify_time = user.l("modify_time"); // 10位时间戳
        QString display_id = user.s("display_id"); // 自定义的用户ID，可以是英文、下划线等
        QString sec_uid = user.s("sec_uid");
        int authorization_info = user.i("authorization_info"); // 3?
        QString location_city = user.s("location_city"); // 实际地址
        QString remark_name = user.s("remark_name"); // 空的
        int mystery_man = user.i("mystery_man"); // 1?
        QString webcast_uid = user.s("webcast_uid");

        MyJson follow_info = user.o("follow_info");
        int following_count = follow_info.i("following_count");
        int follower_count = follow_info.i("follower_count");

        MyJson pay_grade = user.o("pay_grade");
        int pay_level = pay_grade.i("level"); // 荣耀等级

        MyJson fans_club = user.o("fans_club").data(); // 因为我没加，这里是空数据

        // XXX: 实际上并没有发送成功，手动添加
        LiveDanmaku danmaku(nickname, content, uid, pay_level, QDateTime::fromSecsSinceEpoch(modify_time), "", "");
        danmaku.setFromRoomId(ac->roomId);
        danmaku.setLogId(snum(msg_id));
        danmaku.setFaceUrl(avatar);
        danmaku.with(data);
        danmaku.setNoReply();
        receiveDanmaku(danmaku);
    });
}
