#include "douyin_liveservice.h"
#include "fileutil.h"
#include "stringutil.h"
#include "douyinsignaturehelper.h"
#include "nanopb/pb_decode.h"
#include "protobuf/douyin.pb.h"
#include "douyin_api_util.h"

DouyinLiveService::DouyinLiveService(QObject *parent) : LiveServiceBase(parent)
{
    initWS();
}

void DouyinLiveService::initWS()
{
    liveSocket = new QWebSocket();
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
        qDebug() << "[socket.state]" << str;
    });
    connect(liveSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, [=](QAbstractSocket::SocketError error){
        qWarning() << "连接错误：" << error;
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

        // 解析数据
        MyJson state = json.o("state");
        MyJson roomInfo = state.o("roomStore").o("roomInfo");
        ac->roomId = roomInfo.s("roomId"); // 和入口的roomid不是同一个，而且很长
        MyJson room = roomInfo.o("room");
        ac->roomTitle = room.s("title");
        DouyinLiveStatus roomStatus = (DouyinLiveStatus)room.i("status");
        qInfo() << "直播间信息：" << ac->roomTitle << ac->roomId << roomStatus;
        
        MyJson anchor = roomInfo.o("anchor");
        QJsonArray avatarThumb = anchor.o("avatar_thumb").a("url_list");
        QString avatar = avatarThumb.size() ? avatarThumb.first().toString() : "";
        ac->upName = anchor.s("nickname");
        qInfo() << "主播信息：" << ac->upName;

        MyJson partitionRoadMap = roomInfo.o("partition_road_map");
        MyJson partition = partitionRoadMap.o("partition");
        ac->parentAreaId = partition.s("id_str");
        ac->parentAreaName = partition.s("title");
        MyJson subPartition = partitionRoadMap.o("sub_partition");
        partition = subPartition.o("partition");
        ac->areaId = partition.s("id_str");
        ac->areaName = partition.s("title");
        qInfo() << "分区信息：" << ac->parentAreaName + "/" + ac->areaName;

        MyJson userStore = state.o("userStore");
        MyJson odin = userStore.o("odin");
        QString userId = odin.s("user_id");
        QString userUniqueId = odin.s("user_unique_id");
        int userType = odin.i("user_type"); // 匿名是12
        ac->cookieUid = userUniqueId;
        qInfo() << "用户信息：用户唯一ID" << userUniqueId;

        emit signalRoomIdChanged(ac->roomId);
        emit signalUpUidChanged(ac->upUid);
        emit signalRoomInfoChanged();

        getDanmuInfo();
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
    // QByteArray ba = imFetch(ac->roomId, ac->cookieUid);
    qint64 timestamp10 = QDateTime::currentSecsSinceEpoch();
    QString cursor = QString("r-7497180536918546638_d-1_u-1_fh-7497179772733760010_t-%1").arg(timestamp10);
    QString internalExt = QString("internal_src:dim|wss_push_room_id:%2|wss_push_did:%3|first_req_ms:%1|fetch_time:%1|seq:1|wss_info:0-%1-0-0|wrds_v:7497180515443673855")
                                .arg(timestamp10).arg(ac->roomId).arg(ac->cookieUid);
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
    QString roomId = ac->roomId;
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
    QSslConfiguration config = liveSocket->sslConfiguration();
    config.setProtocol(QSsl::TlsV1_2OrLater);
    liveSocket->setSslConfiguration(config);

    // 需要带相同的Cookie，否则会Refuse。但是实测可以用其他账号的cookie
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Cookie", ac->browserCookie.toUtf8());
    request.setHeader(QNetworkRequest::UserAgentHeader, "5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36");
    request.setRawHeader("Referer", "https://live.douyin.com/");
    request.setRawHeader("Origin", "https://live.douyin.com");
    
    liveSocket->open(request);
}

// 放到 onBinaryMessageReceived 函数外面，或者类的私有成员/静态函数位置
struct PayloadBuffer {
    uint8_t *data;
    size_t size;
    size_t max_size;
};

// 回调函数，用于解码变长bytes字段
bool payload_decode_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    PayloadBuffer *buf = (PayloadBuffer*)(*arg);
    
    // 检查缓冲区是否足够大
    if (stream->bytes_left > buf->max_size) {
        // 如果数据太大，可以选择返回错误或者截断
        qWarning() << "Payload data is too large for buffer!" << stream->bytes_left << ">" << buf->max_size;
        return false; // 返回false表示解码失败
    }
    
    buf->size = stream->bytes_left;
    return pb_read(stream, (pb_byte_t*)buf->data, buf->size);
}

void DouyinLiveService::onBinaryMessageReceived(const QByteArray &message)
{
    // --- 步骤 1: 解析外层的 PushFrame ---

    // 1.1 创建一个足够大的缓冲区来存放 payload
    uint8_t payload_buffer_data[8192]; // 8KB，如果不够可以再加大
    PayloadBuffer payload_buf = { payload_buffer_data, 0, sizeof(payload_buffer_data) };
    
    // 1.2 初始化 PushFrame 结构体，并设置 payload 的回调
    douyin_PushFrame pushFrame = douyin_PushFrame_init_zero;
    pushFrame.payload.funcs.decode = &payload_decode_callback;
    pushFrame.payload.arg = &payload_buf;
    // headersList 也是 callback，如果需要解析，也要设置回调，这里暂时忽略
    pushFrame.headersList.arg = nullptr;

    // 1.3 创建输入流
    pb_istream_t stream = pb_istream_from_buffer(
        reinterpret_cast<const uint8_t*>(message.constData()), 
        message.size()
    );

    // 1.4 解码 PushFrame
    if (!pb_decode(&stream, douyin_PushFrame_fields, &pushFrame)) {
        qWarning() << "Decoding PushFrame failed:" << PB_GET_ERROR(&stream);
        return;
    }

    // pushFrame说明：
    // seqId：消息序列号，用于标识消息的顺序，从1开始递增
    // logId：日志ID，用于标识消息的唯一性，19位数字
    // service：服务ID，用于标识消息所属的服务，例如 8888
    // method：方法ID，用于标识消息的类型，例如 8
    // payloadEncoding：payload的编码方式，例如 "pb"
    // payloadType：payload的类型，例如 "msg"
    qint64 seqId = pushFrame.seqId;
    QString payloadEncoding = QString::fromUtf8(pushFrame.payloadEncoding);
    QString payloadType = QString::fromUtf8(pushFrame.payloadType);
    qDebug() << "PushFrame decoded:" << seqId << payloadEncoding << payloadType;


    // --- 步骤 2: 解压并解析内层的 Response ---
    
    // 2.1 检查 payload 是否为 gzip 压缩
    if (payloadEncoding != "gzip" && payloadEncoding != "pb") {
        qWarning() << "Unsupported payload encoding:" << payloadEncoding;
        return;
    }

    // 2.2 解压 gzip 数据
    QByteArray compressedPayload(reinterpret_cast<const char*>(payload_buf.data), payload_buf.size);
    QByteArray decompressedPayload = DouyinApiUtil::decompressGzip(compressedPayload);

    if (decompressedPayload.isEmpty()) {
        qWarning() << "Gzip decompression failed.";
        return;
    }

    // 2.3 创建 Response 结构体和输入流
    douyin_Response response = douyin_Response_init_zero;
    // 注意：Response 内部也有 callback 字段 (messagesList)，也需要设置回调！
    // 暂时我们先不解析 messagesList，所以可以忽略
    response.messagesList.arg = nullptr; 
    response.routeParams.arg = nullptr;

    pb_istream_t response_stream = pb_istream_from_buffer(
        reinterpret_cast<const uint8_t*>(decompressedPayload.constData()),
        decompressedPayload.size()
    );

    // 2.4 解码 Response
    if (!pb_decode(&response_stream, douyin_Response_fields, &response)) {
        qWarning() << "Decoding Response failed:" << PB_GET_ERROR(&response_stream);
        return;
    }

    qDebug() << "Response decoded. Cursor:" << response.cursor << "Need Ack:" << response.needAck;
    
    // --- 步骤 3: 进一步处理 Response 里的具体消息 ---
    // 你需要为 messagesList 设置回调来逐个处理 Message
    // 这里我们先跳过，因为那又是另一个回调
    // for (size_t i = 0; i < response.messagesList_count; ++i) { ... }
}
