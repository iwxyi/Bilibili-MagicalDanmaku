#include "bili_liveservice.h"
#include "utils/bili_api_util.h"

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
        appendNewLiveDanmaku(danmaku);

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
            appendNewLiveDanmaku(danmaku);

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
