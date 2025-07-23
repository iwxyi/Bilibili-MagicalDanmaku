#include "douyin_liveservice.h"
#include "nanopb/pb_decode.h"
#include "protobuf/douyin.pb.h"
#include "douyin_api_util.h"
#include "douyinackgenerator.h"

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

struct MessageListContext {
    // 你可以在这里存放你需要的结果，比如QList、vector等
    // 这里只做演示，实际可根据需要扩展
    QObject* service; // 传递this指针，方便调用成员函数
};

struct HeaderListContext {
    QVector<QPair<QString, QString>> headers;
};

/// 解析 headersList 回调
bool headersList_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    HeaderListContext* ctx = static_cast<HeaderListContext*>(*arg);
    douyin_HeadersList header = douyin_HeadersList_init_zero;
    if (!pb_decode(stream, douyin_HeadersList_fields, &header)) {
        qWarning() << "decode HeadersList failed";
        return false;
    }
    QString key = QString::fromUtf8(header.key);
    QString value = QString::fromUtf8(header.value);
    // qDebug() << "frame.header：" << key << value;
    ctx->headers.append(qMakePair(key, value));
    return true;
}

/// 解析 messagesList 回调
bool messagesList_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    MessageListContext* ctx = static_cast<MessageListContext*>(*arg);

    // 1. 解码一条Message
    douyin_Message msg = douyin_Message_init_zero;

    // 1.1 设置payload回调，收集payload内容
    struct {
        uint8_t data[81920];
        size_t size;
        size_t max_size;
    } payloadBuf = { {0}, 0, sizeof(payloadBuf.data) };

    auto payload_callback = [](pb_istream_t *stream, const pb_field_t *, void **arg) -> bool {
        auto* buf = static_cast<decltype(payloadBuf)*>(*arg);
        if (stream->bytes_left > buf->max_size) return false;
        buf->size = stream->bytes_left;
        return pb_read(stream, buf->data, buf->size);
    };
    msg.payload.funcs.decode = payload_callback;
    msg.payload.arg = &payloadBuf;

    if (!pb_decode(stream, douyin_Message_fields, &msg)) {
        qWarning() << "decode Message failed";
        return false;
    }

    // 2. 处理method字段，判断类型
    QString method = QString::fromUtf8(msg.method);
    if (msg.msgType != 0)
        qWarning() << "【收到msgType!=0消息】 method:" << method << "msgType:" << msg.msgType;

    // 3. 解析payload为具体消息体
    QByteArray payloadArr(reinterpret_cast<const char*>(payloadBuf.data), payloadBuf.size);

    if (!ctx)
    {
        qWarning() << "PB解析失败，无法获取**arg";
        return true;
    }
    static_cast<DouyinLiveService*>(ctx->service)->processMessage(method, payloadArr);

    return true; // 继续处理下一个Message
}

void DouyinLiveService::onBinaryMessageReceived(const QByteArray &message)
{
    // --- 步骤 1: 解析外层的 PushFrame ---

    // 1.1 创建一个足够大的缓冲区来存放 payload
    uint8_t payload_buffer_data[81920]; // 8KB，如果不够可以再加大
    PayloadBuffer payload_buf = { payload_buffer_data, 0, sizeof(payload_buffer_data) };

    // 1.2 初始化 PushFrame 结构体，并设置 payload 的回调
    douyin_PushFrame pushFrame = douyin_PushFrame_init_zero;
    pushFrame.payload.funcs.decode = &payload_decode_callback;
    pushFrame.payload.arg = &payload_buf;
    HeaderListContext headerCtx;
    pushFrame.headersList.funcs.decode = headersList_callback;
    pushFrame.headersList.arg = &headerCtx;

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
    qint64 logId = pushFrame.logId;
    QString payloadEncoding = QString::fromUtf8(pushFrame.payloadEncoding);
    QString payloadType = QString::fromUtf8(pushFrame.payloadType);
    // qDebug() << "PushFrame decoded:" << seqId << payloadEncoding << payloadType;

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
    // 设置messagesList回调
    MessageListContext ctx;
    ctx.service = this; // 你可以传递this指针，方便在回调里emit信号等
    response.messagesList.funcs.decode = messagesList_callback;
    response.messagesList.arg = &ctx;
    response.routeParams.arg = nullptr;

    pb_istream_t response_stream = pb_istream_from_buffer(
        reinterpret_cast<const uint8_t*>(decompressedPayload.constData()),
        decompressedPayload.size()
        );

    // 2.4 解码 Response
    if (!pb_decode(&response_stream, douyin_Response_fields, &response)) {
        qWarning() << "Decoding Response failed:" << PB_GET_ERROR(&response_stream) << decompressedPayload.size();
        return;
    }

    QString cursor = response.cursor;
    QString internalExt = response.internalExt;
    bool needAck = response.needAck;
    // qDebug() << "Response decoded. needAck:" << needAck;

    // --- 步骤 3: 进一步处理 Response 里的具体消息 ---
    // 为 messagesList 设置回调来逐个处理 Message，在回调函数里面处理

    // --- 步骤 4: 处理返回情况
    if (needAck)
    {
        DouyinAckGenerator dag;
        dag.sendAck(this->liveSocket, internalExt, logId);
    }

    if (payloadType == "msg") // 消息
    {
        // 每个都会跳转到回调函数 messagesList_callback
    }
    else if (payloadType == "ack")
    {

    }
    else if (payloadType == "hb")
    {

    }
    else if (payloadType == "close") // 关闭连接
    {
        qInfo() << "关闭连接，payloadType=close";
    }
}

struct RoomRankContext {
    QVector<douyin_RoomRank> ranks;
};

/// 遍历每个RoomRank
bool ranks_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    douyin_RoomRank rank = douyin_RoomRank_init_zero;
    if (!pb_decode(stream, douyin_RoomRank_fields, &rank)) return false;
    auto* ctx = static_cast<RoomRankContext*>(*arg);
    ctx->ranks.append(rank);
    return true;
}

void DouyinLiveService::processMessage(const QString &method, const QByteArray &payload)
{
    const pb_byte_t *data = reinterpret_cast<const uint8_t *>(payload.data());
    auto size = payload.size();

    if (method == "WebcastChatMessage") // 弹幕
    {
        douyin_ChatMessage chat = douyin_ChatMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer(data, size);
        if (!pb_decode(&payload_stream, douyin_ChatMessage_fields, &chat))
        {
            qWarning() << PB_GET_ERROR(&payload_stream);
            return showError(method, "解析错误");
        }

        // 用户信息
        douyin_User user = chat.user;
        QString uname = QString::fromUtf8(chat.user.nickName);
        QString content = QString::fromUtf8(chat.content);
        qint64 uid = user.id;
        QString secUid = user.secUid;
        int level = user.Level;

        LiveDanmaku danmaku(uname, content, uid, level, QDateTime::currentDateTime(), "", "");
        danmaku.setFromRoomId(ac->roomId);
        danmaku.setSecUid(secUid);

        // 抖音弹幕是没有头像的，但是有这个字段
        QString avatar;
        {
            if (user.has_AvatarThumb)
                avatar = QString::fromUtf8(user.AvatarThumb.uri);
            if (user.has_AvatarMedium)
                avatar = QString::fromUtf8(user.AvatarMedium.uri);
            if (user.has_AvatarLarge)
                avatar = QString::fromUtf8(user.AvatarLarge.uri);
            danmaku.setFaceUrl(avatar);
        }

        // 关注与被关注
        if (user.has_FollowInfo)
        {
            douyin_FollowInfo followInfo = user.FollowInfo;
            int followerCount = followInfo.followerCount;
            int followingCount = followInfo.followingCount;
        }

        // 粉丝灯牌
        if (user.has_Medal)
        {
            douyin_Image medal = user.Medal;
        }

        qInfo() << "[弹幕]" << uname << "(" + snum(uid) + ")" << ":" << content;
        danmaku.setFaceUrl(avatar);
        receiveDanmaku(danmaku);
    }
    else if (method == "WebcastGiftMessage") // 送礼
    {
        douyin_GiftMessage gift = douyin_GiftMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer(data, size);
        if (!pb_decode(&payload_stream, douyin_GiftMessage_fields, &gift))
        {
            qWarning() << PB_GET_ERROR(&payload_stream);
            return showError(method, "解析错误");
        }

        douyin_Common common = gift.common;
        QString logId = common.logId;

        int giftId = gift.giftId;
        int comboCount = gift.comboCount;
        int totalCount = gift.totalCount;
        douyin_GiftStruct giftStruct = gift.gift;
        QString giftName = giftStruct.name;
        int singlePrice = giftStruct.diamondCount; // 单价，抖币。有些礼物单价为0
        int totalPrice = singlePrice * totalCount; // 总价，抖币

        douyin_User user = gift.user;
        QString uname = user.nickName;
        QString uid = snum(user.id);
        qInfo() << "[送礼]" << uname << "送出礼物" << giftId << giftName << "x" << totalCount << "总价" << totalPrice;

        LiveDanmaku danmaku(uname, giftId, giftName, totalCount, uid, QDateTime::currentDateTime(), "", totalPrice);
        danmaku.setFromRoomId(ac->roomId);
        danmaku.setCoinType("douyin_coin");
        danmaku.setLogId(logId);
        receiveGift(danmaku);
        appendLiveGift(danmaku);
    }
    else if (method == "WebcastMemberMessage") // 进房
    {
        douyin_MemberMessage member = douyin_MemberMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer(data, size);
        if (!pb_decode(&payload_stream, douyin_MemberMessage_fields, &member))
        {
            qWarning() << PB_GET_ERROR(&payload_stream);
            return showError(method, "解析错误");
        }

        douyin_User user = member.user;
        QString uname = QString::fromUtf8(member.user.nickName);
        QString secUid = user.secUid;
        int level = user.Level;
        int isTopUser = member.isTopUser;
        int isAdmin = member.isSetToAdmin;
        int rankScore = member.rankScore;
        // 如果是匿名的，user.id统一为“111111”，secUid为空，shortId为0
        qInfo() << "[进房]" << uname << "(" + snum(user.id) + ")" << "进入直播间"
                << " 等级:" << level << "房管:" << isAdmin << "Top:" << isTopUser
                << "积分:" << rankScore;

        LiveDanmaku danmaku(uname, snum(user.id), QDateTime::currentDateTime(), false, "", "", "");
        danmaku.setFromRoomId(ac->roomId);
        danmaku.setSecUid(secUid);
        danmaku.setLevel(level);
        danmaku.setAdmin(isAdmin);
        danmaku.setNumber(rankScore);
        receiveUserCome(danmaku);
    }
    else if (method == "WebcastSocialMessage") // 关注
    {
        douyin_SocialMessage social = douyin_SocialMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer(data, size);
        if (!pb_decode(&payload_stream, douyin_SocialMessage_fields, &social))
        {
            qWarning() << PB_GET_ERROR(&payload_stream);
            return showError(method, "解析错误");
        }
        douyin_User user = social.user;
        QString uname = QString::fromUtf8(user.nickName);
        qint64 uid = user.id;
        qInfo() << "[关注]" << uname << "关注了主播";

        LiveDanmaku danmaku;
        danmaku.setUser(uname, snum(uid), "", "");
        danmaku.setMsgType(MSG_ATTENTION);
        danmaku.setTime(QDateTime::currentDateTime());
        danmaku.setFromRoomId(ac->roomId);
        appendNewLiveDanmaku(danmaku);
        emit signalSendAttentionThank(danmaku);
        triggerCmdEvent("ATTENTION", danmaku);
    }
    else if (method == "WebcastLikeMessage") // 点赞
    {
        douyin_LikeMessage like = douyin_LikeMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer(data, size);
        if (!pb_decode(&payload_stream, douyin_LikeMessage_fields, &like))
        {
            qWarning() << PB_GET_ERROR(&payload_stream);
            return showError(method, "解析错误");
        }
        QString uname = like.has_user ? QString::fromUtf8(like.user.nickName) : "";
        qInfo() << "[点赞]" << uname << "点赞" << like.count << like.total;
        emit signalLikeChanged(like.total);
    }
    else if (method == "WebcastRoomUserSeqMessage") // 人气
    {
        douyin_RoomUserSeqMessage roomUserSeq = douyin_RoomUserSeqMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer(data, size);
        if (!pb_decode(&payload_stream, douyin_RoomUserSeqMessage_fields, &roomUserSeq))
        {
            qWarning() << PB_GET_ERROR(&payload_stream);
            return showError(method, "解析错误");
        }

        int total = roomUserSeq.total; // 在线观众总数
        int popularity = roomUserSeq.popularity; // 人气值，有时为0
        int totalUser = roomUserSeq.totalUser; // 累计进房总人数
        QString onlineUserForAnchor = QString::fromUtf8(roomUserSeq.onlineUserForAnchor); // 主播端在线人数，似乎等于总数
        QString totalPvForAnchor = QString::fromUtf8(roomUserSeq.totalPvForAnchor); // 总PV（总访问量，比总人数大），例如"13.2万"
        QString upRightStatsStr = QString::fromUtf8(roomUserSeq.upRightStatsStr); // 主播右上角统计，为空表示无特殊显示
        QString upRightStatsStrComplete = QString::fromUtf8(roomUserSeq.upRightStatsStrComplete); // 主播右上角统计完整，同上
        qDebug() << "[人气] 总数:" << total << "人气值:" << popularity << "总人数:" << totalUser
                 << "主播端在线人数:" << onlineUserForAnchor << "总PV:" << totalPvForAnchor
                 << "主播右上角:" << upRightStatsStr << upRightStatsStrComplete;
        emit signalOnlineCountChanged(total);
        emit signalPopularChanged(popularity);
        emit signalTotalComeUserChanged(totalUser);
        emit signalTotalPvChanged(totalPvForAnchor);
    }
    else if (method == "WebcastRoomStatsMessage") // 在线人数
    {
        douyin_RoomStatsMessage roomStats = douyin_RoomStatsMessage_init_zero;
        pb_istream_t payload_stream = pb_istream_from_buffer(data, size);
        if (!pb_decode(&payload_stream, douyin_RoomStatsMessage_fields, &roomStats))
        {
            qWarning() << PB_GET_ERROR(&payload_stream);
            return showError(method, "解析错误");
        }
        // 这个就是显示在线观众数量
        QString display = roomStats.display_long;
        int displayValue = roomStats.display_value;
        int total = roomStats.total;
        int displayType = roomStats.display_type;
        qDebug() << "[在线] 显示:" << display << "值:" << displayValue << "总数:" << total << "类型:" << displayType;
    }
    else if (method == "WebcastRoomRankMessage") // 排行榜
    {
        RoomRankContext ctx;
        douyin_RoomRankMessage roomRank = douyin_RoomRankMessage_init_zero;
        roomRank.ranks.funcs.decode = ranks_callback;
        roomRank.ranks.arg = &ctx;
        pb_istream_t payload_stream = pb_istream_from_buffer(data, size);
        if (!pb_decode(&payload_stream, douyin_RoomRankMessage_fields, &roomRank))
        {
            qWarning() << PB_GET_ERROR(&payload_stream);
            return showError(method, "解析错误");
        }

        auto ranks = ctx.ranks;
        qDebug() << "[房间内排行榜]" << ranks.size();
        onlineGoldRank.clear();
        foreach (auto rank, ranks)
        {
            QString score = rank.score_str; // 没有分数
            douyin_User user = rank.user;
            QString nickname = user.nickName;
            qint64 uid = user.id;
            QString avatar = user.AvatarThumb.uri; // 后缀参数
            avatar = "https://p3.douyinpic.com/aweme/" + avatar;

            LiveDanmaku u(nickname, snum(uid));
            u.setFaceUrl(avatar);
            onlineGoldRank.append(u);
        }
        emit signalOnlineRankChanged();
    }
    else if (method == "WebcastBackupSEIMessage")
    {}
    else
    {
        qInfo() << "[未知消息]" << method << "payload size:" << size;
    }

    triggerCmdEvent(method, LiveDanmaku());

    /* 常见消息类型：
WebcastLikeMessage: 直播间点赞消息
WebcastMemberMessage: 直播间成员消息
WebcastChatMessage: 直播间聊天消息
WebcastGiftMessage: 直播间礼物消息
WebcastSocialMessage: 直播间关注消息
WebcastRoomUserSeqMessage: 直播间用户序列消息
WebcastUpdateFanTicketMessage: 直播间粉丝票更新消息
WebcastCommonTextMessage: 直播间文本消息
WebcastMatchAgainstScoreMessage: 直播间对战积分消息
WebcastEcomFansClubMessage: 直播间电商粉丝团消息
WebcastRoomStatsMessage: 直播间统计消息
WebcastLiveShoppingMessage: 直播间购物消息
WebcastLiveEcomGeneralMessage: 直播间电商通用消息
WebcastRoomStreamAdaptationMessage: 直播间流适配消息
WebcastRanklistHourEntranceMessage: 直播间小时榜入口消息
WebcastProductChangeMessage: 直播间商品变更消息
WebcastNotifyEffectMessage: 直播间通知效果消息
WebcastLightGiftMessage: 直播间轻礼物消息
WebcastProfitInteractionScoreMessage: 直播间互动分数消息
WebcastRoomRankMessage: 直播间排行榜消息
WebcastFansclubMessage: 直播间粉丝团消息
WebcastHotRoomMessage: 直播间热门房间消息
WebcastInRoomBannerMessage: 直播间内横幅消息
WebcastScreenChatMessage: 直播间全局聊天消息
WebcastRoomDataSyncMessage: 直播间数据同步消息
WebcastLinkerContributeMessage: 直播间连麦贡献消息
WebcastEmojiChatMessage: 直播间表情聊天消息
WebcastLinkMicMethod: 处理直播间连麦消息(Mic)
WebcastLinkMessage: 直播间连麦消息
WebcastBattleTeamTaskMessage: 直播间战队任务消息
WebcastHotChatMessage: 直播间热聊消息
*/
}
