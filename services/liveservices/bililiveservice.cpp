#include "bililiveservice.h"

BiliLiveService::BiliLiveService(QObject *parent) : LiveRoomService(parent)
{
}

void BiliLiveService::startConnectRoom(const QString &roomId)
{

}

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
    QString url = "http://api.bilibili.com/x/space/acc/info?mid=" + uid;
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

                LiveDanmaku gift("", id, name, 1, 0, QDateTime(), coinType, coin);
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
        JI(anchor_pk_info, pk_rank_star); // 段位级别：2
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
        JI(score_info, total_tie_count); // 平局：3
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
            emit signalTriggerCmdEvent("PK_WINNING_STREAK", danmaku);
        }
    });
}
