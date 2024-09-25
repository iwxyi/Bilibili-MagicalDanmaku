#include "bili_liveservice.h"
#include "utils/bili_api_util.h"
#include "coderunner.h"

/**
 * 接收到原始数据
 * 进行解压操作
 */
void BiliLiveService::slotBinaryMessageReceived(const QByteArray &message)
{
    int operation = ((uchar)message[8] << 24)
            + ((uchar)message[9] << 16)
            + ((uchar)message[10] << 8)
            + ((uchar)message[11]);
    QByteArray body = message.right(message.length() - 16);
    SOCKET_DEB << "操作码=" << operation << "  大小=" << body.size() << "  正文=" << (body.left(1000)) << "...";

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(body, &error);
    MyJson json;
    if (error.error == QJsonParseError::NoError)
        json = document.object();

    if (operation == OP_AUTH_REPLY) // 认证包回复
    {
        if (json.value("code").toInt() != 0)
        {
            qCritical() << s8("认证出错");
        }
    }
    else if (operation == OP_HEARTBEAT_REPLY) // 心跳包回复（人气值）
    {
        qint32 popularity = ((uchar)body[0] << 24)
                + ((uchar)body[1] << 16)
                + ((uchar)body[2] << 8)
                + (uchar)body[3];
        SOCKET_DEB << "人气值=" << popularity;
        popularVal = ac->currentPopul = popularity;
        if (isLiving())
            emit signalPopularChanged(popularity);
    }
    else if (operation == OP_SEND_MSG_REPLY) // 普通包
    {
        int protover = (message[6]<<8) + message[7]; // short类型
        SOCKET_DEB << "协议版本：" << protover;

        if (protover == 2) // 默认协议版本，zlib解压
        {
            splitUncompressedBody(BiliApiUtil::zlibToQtUncompr(body.data(), body.size()+1));
        }
        else if (protover == 3) // brotli解压
        {
            splitUncompressedBody(BiliApiUtil::decompressData(body.data(), body.size()));
        }
        else if (protover == 0)
        {
            QString cmd;
            if (!json.isEmpty())
            {
                cmd = json.value("cmd").toString();
                qInfo() << "普通CMD：" << cmd;
                SOCKET_INF << json;
            }
            else
            {
                qWarning() << "空的json体：" << body;
                return ;
            }

            if (us->saveToSqlite && us->saveCmdToSqlite)
            {
                sqlService->insertCmd(cmd, body);
            }

            qInfo() << ">消息命令UNZC：" << cmd;

            if (!handleUncompMessage(cmd, json))
            {
                qWarning() << QString(body);
            }
        }
        else
        {
            qWarning() << s8("未知协议：") << protover << s8("，若有必要请处理");
            qWarning() << s8("未知正文：") << body;
        }
    }
    else
    {
        qWarning() << "未处理的包类型：operation =" << operation;
    }
//    delete[] body.data();
//    delete[] message.data();
    SOCKET_DEB << "消息处理结束";
}

void BiliLiveService::splitUncompressedBody(const QByteArray &unc)
{
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
            qCritical() << s8("解析解压后的JSON出错：") << error.errorString();
            qCritical() << s8("包数值：") << offset << packSize << "  解压后大小：" << unc.size();
            qCritical() << s8(">>当前JSON") << jsonBa;
            qCritical() << s8(">>解压正文") << unc;
            return ;
        }
        QJsonObject json = document.object();
        QString cmd = json.value("cmd").toString();
        SOCKET_INF << "解压后获取到CMD：" << cmd;
        if (us->saveToSqlite && us->saveCmdToSqlite)
        {
            sqlService->insertCmd(cmd, jsonBa);
        }
        if (cmd != "ROOM_BANNER" && cmd != "ACTIVITY_BANNER_UPDATE_V2" && cmd != "PANEL"
                && cmd != "ONLINERANK")
            SOCKET_INF << "单个JSON消息：" << offset << packSize << QString(jsonBa);
        try {
            handleMessage(json);
        } catch (...) {
            qCritical() << s8("出错啦") << jsonBa;
        }

        offset += packSize;
    }
}

bool BiliLiveService::handleUncompMessage(QString cmd, MyJson json)
{
    if (cmd == "STOP_LIVE_ROOM_LIST" || cmd == "WIDGET_BANNER" || cmd == "NOTICE_MSG")
    {
        return true;
    }
    else if (cmd == "ROOM_RANK")
    {
        /*{
            "cmd": "ROOM_RANK",
            "data": {
                "color": "#FB7299",
                "h5_url": "https://live.bilibili.com/p/html/live-app-rankcurrent/index.html?is_live_half_webview=1&hybrid_half_ui=1,5,85p,70p,FFE293,0,30,100,10;2,2,320,100p,FFE293,0,30,100,0;4,2,320,100p,FFE293,0,30,100,0;6,5,65p,60p,FFE293,0,30,100,10;5,5,55p,60p,FFE293,0,30,100,10;3,5,85p,70p,FFE293,0,30,100,10;7,5,65p,60p,FFE293,0,30,100,10;&anchor_uid=688893202&rank_type=master_realtime_area_hour&area_hour=1&area_v2_id=145&area_v2_parent_id=1",
                "rank_desc": "娱乐小时榜 7",
                "roomid": 22532956,
                "timestamp": 1605749940,
                "web_url": "https://live.bilibili.com/blackboard/room-current-rank.html?rank_type=master_realtime_area_hour&area_hour=1&area_v2_id=145&area_v2_parent_id=1"
            }
        }*/
        QJsonObject data = json.value("data").toObject();
        QString color = data.value("color").toString();
        QString desc = data.value("rank_desc").toString();
        emit signalRoomRankChanged(desc, color);
        if (desc != roomRankDesc) // 排名有更新
            localNotify("当前排名：" + desc);
        roomRankDesc = desc;

        triggerCmdEvent(cmd, LiveDanmaku().with(data));
    }
    else if (handlePK(json))
    {
    }
    else if (cmd == "ROOM_REAL_TIME_MESSAGE_UPDATE") // 实时信息改变
    {
        // {"cmd":"ROOM_REAL_TIME_MESSAGE_UPDATE","data":{"roomid":22532956,"fans":1022,"red_notice":-1,"fans_club":50}}
        QJsonObject data = json.value("data").toObject();
        int fans = data.value("fans").toInt();
        int fans_club = data.value("fans_club").toInt();
        int delta_fans = 0, delta_club = 0;
        if (ac->currentFans || ac->currentFansClub)
        {
            delta_fans = fans - ac->currentFans;
            delta_club = fans_club - ac->currentFansClub;
        }
        ac->currentFans = fans;
        ac->currentFansClub = fans_club;
        qInfo() << s8("粉丝数量：") << fans << s8("  粉丝团：") << fans_club;
        // appendNewLiveDanmaku(LiveDanmaku(fans, fans_club, delta_fans, delta_club));

        dailyNewFans += delta_fans;
        if (dailySettings)
        {
            dailySettings->setValue("new_fans", dailyNewFans);
            dailySettings->setValue("total_fans", ac->currentFans);
        }
        currentLiveNewFans += delta_fans;
        if (currentLiveSettings)
        {
            currentLiveSettings->setValue("new_fans", currentLiveNewFans);
            currentLiveSettings->setValue("total_fans", ac->currentFans);
        }
        emit signalFansCountChanged(fans);
    }
    else if (cmd == "WIDGET_BANNER") // 无关的横幅广播
    {}
    else if (cmd == "HOT_RANK_CHANGED") // 热门榜
    {
        /*{
            "cmd": "HOT_RANK_CHANGED",
            "data": {
                "rank": 14,
                "trend": 2, // 趋势：1上升，2下降
                "countdown": 1705,
                "timestamp": 1610168495,
                "web_url": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=2\\u0026area_id=1",
                "live_url": "……（太长了）",
                "blink_url": "……（太长了）",
                "live_link_url": "……（太长了）",
                "pc_link_url": "……（太长了）",
                "icon": "https://i0.hdslb.com/bfs/live/3f833451003cca16a284119b8174227808d8f936.png",
                "area_name": "娱乐"
            }
        }*/
        QJsonObject data = json.value("data").toObject();
        int rank = data.value("rank").toInt();
        int trend = data.value("trend").toInt(); // 趋势：1上升，2下降
        QString area_name = data.value("area_name").toString();
        if (area_name.endsWith("榜"))
            area_name.replace(area_name.length()-1, 1, "");
        QString msg = QString("热门榜 " + area_name + "榜 排名：" + snum(rank) + " " + (trend == 1 ? "↑" : "↓"));
        qInfo() << msg;
        emit signalHotRankChanged(rank, area_name, msg);
    }
    else if (cmd == "HOT_RANK_SETTLEMENT")
    {
        /*{
            "cmd": "HOT_RANK_SETTLEMENT",
            "data": {
                "rank": 9,
                "uname": "xxx",
                "face": "http://i2.hdslb.com/bfs/face/17f1f3994cb4b2bba97f1557ffc7eb34a05e119b.jpg",
                "timestamp": 1610173800,
                "icon": "https://i0.hdslb.com/bfs/live/3f833451003cca16a284119b8174227808d8f936.png",
                "area_name": "娱乐",
                "url": "https://live.bilibili.com/p/html/live-app-hotrank/result.html?is_live_half_webview=1\\u0026hybrid_half_ui=1,5,250,200,f4eefa,0,30,0,0,0;2,5,250,200,f4eefa,0,30,0,0,0;3,5,250,200,f4eefa,0,30,0,0,0;4,5,250,200,f4eefa,0,30,0,0,0;5,5,250,200,f4eefa,0,30,0,0,0;6,5,250,200,f4eefa,0,30,0,0,0;7,5,250,200,f4eefa,0,30,0,0,0;8,5,250,200,f4eefa,0,30,0,0,0\\u0026areaId=1\\u0026cache_key=4417cab3fa8b15ad1b250ee29fd91c52",
                "cache_key": "4417cab3fa8b15ad1b250ee29fd91c52",
                "dm_msg": "恭喜主播 \\u003c% xxx %\\u003e 荣登限时热门榜娱乐榜top9! 即将获得热门流量推荐哦！"
            }
        }*/
        QJsonObject data = json.value("data").toObject();
        int rank = data.value("rank").toInt();
        QString uname = data.value("uname").toString();
        QString area_name = data.value("area_name").toString();
        QString msg = QString("恭喜荣登热门榜" + area_name + "榜 top" + snum(rank) + "!");
        // 还没想好这个分区榜要放到总榜还是小时榜里？
        qInfo() << "rank:" << msg;
        emit signalHotRankChanged(rank, area_name, msg);
        triggerCmdEvent("HOT_RANK", LiveDanmaku(area_name + "榜 top" + snum(rank)).with(data), true);
        localNotify(msg);
    }
    else if (cmd == "HOT_RANK_SETTLEMENT_V2")
    {
        // 和上面的V1一样，不管了
    }
    else if (handlePK(json))
    {
    }
    else if (cmd == "HOT_RANK_CHANGED_V2")
    {
        /*{
            "cmd": "HOT_RANK_CHANGED_V2",
            "data": {
                "rank": 0,
                "trend": 0,
                "countdown": 1070,
                "timestamp": 1652929930,
                "web_url": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=2\\u0026area_id=1\\u0026parent_area_id=1\\u0026second_area_id=145",
                "live_url": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=1\\u0026area_id=1\\u0026parent_area_id=1\\u0026second_area_id=145\\u0026is_live_half_webview=1\\u0026hybrid_rotate_d=1\\u0026hybrid_half_ui=1,3,100p,70p,ffffff,0,30,100,12,0;2,2,375,100p,ffffff,0,30,100,0,0;3,3,100p,70p,ffffff,0,30,100,12,0;4,2,375,100p,ffffff,0,30,100,0,0;5,3,100p,70p,ffffff,0,30,100,0,0;6,3,100p,70p,ffffff,0,30,100,0,0;7,3,100p,70p,ffffff,0,30,100,0,0;8,3,100p,70p,ffffff,0,30,100,0,0",
                "blink_url": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=3\\u0026area_id=1\\u0026parent_area_id=1\\u0026second_area_id=145\\u0026is_live_half_webview=1\\u0026hybrid_rotate_d=1\\u0026is_cling_player=1\\u0026hybrid_half_ui=1,3,100p,70p,ffffff,0,30,100,0,0;2,2,375,100p,ffffff,0,30,100,0,0;3,3,100p,70p,ffffff,0,30,100,0,0;4,2,375,100p,ffffff,0,30,100,0,0;5,3,100p,70p,ffffff,0,30,100,0,0;6,3,100p,70p,ffffff,0,30,100,0,0;7,3,100p,70p,ffffff,0,30,100,0,0;8,3,100p,70p,ffffff,0,30,100,0,0",
                "live_link_url": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=5\\u0026area_id=1\\u0026parent_area_id=1\\u0026second_area_id=145\\u0026is_live_half_webview=1\\u0026hybrid_rotate_d=1\\u0026is_cling_player=1\\u0026hybrid_half_ui=1,3,100p,70p,f4eefa,0,30,100,0,0;2,2,375,100p,f4eefa,0,30,100,0,0;3,3,100p,70p,f4eefa,0,30,100,0,0;4,2,375,100p,f4eefa,0,30,100,0,0;5,3,100p,70p,f4eefa,0,30,100,0,0;6,3,100p,70p,f4eefa,0,30,100,0,0;7,3,100p,70p,f4eefa,0,30,100,0,0;8,3,100p,70p,f4eefa,0,30,100,0,0",
                "pc_link_url": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=4\\u0026is_live_half_webview=1\\u0026area_id=1\\u0026parent_area_id=1\\u0026second_area_id=145\\u0026pc_ui=338,465,f4eefa,0",
                "icon": "https://i0.hdslb.com/bfs/live/cb2e160ac4f562b347bb5ae6e635688ebc69580f.png",
                "area_name": "视频聊天",
                "rank_desc": ""
            }
        }*/
        QJsonObject data = json.value("data").toObject();
        int countdown = data.value("countdown").toInt();
    }
    else if (cmd == "HOT_ROOM_NOTIFY")
    {
    }
    else if (cmd == "GUARD_HONOR_THOUSAND") // 千舰变化
    {
        /*{
            "cmd": "GUARD_HONOR_THOUSAND",
            "data": {
                "add": [],
                "del": [
                    2051617240,
                    1501380958,
                    1484169431,
                    1398337493,
                    1084222017,
                    672353429,
                    672346917,
                    672342685,
                    672328094,
                    480680646,
                    401315430,
                    226102317,
                    194484313,
                    8881297,
                    8739477,
                    6713974,
                    1521415,
                    745493,
                    114866,
                    67141
                ]
            }
        }*/
    }
    else if (cmd == "MESSAGEBOX_USER_MEDAL_CHANGE") // 太久没发言，点亮粉丝牌
    {
        /*{
            "cmd": "MESSAGEBOX_USER_MEDAL_CHANGE",
            "data": {
                "guard_level": 0,
                "is_lighted": 1,
                "medal_color_border": 1725515,
                "medal_color_end": 5414290,
                "medal_color_start": 1725515,
                "medal_level": 22,
                "medal_name": "莓钱啦",
                "multi_unlock_level": "",
                "type": 2,
                "uid": 20285041,
                "unlock": 0,
                "unlock_level": 0,
                "up_uid": 1654676647,
                "upper_bound_content": ""
            },
            "is_report": false,
            "msg_id": "916936486563841",
            "send_time": 1690302797554
        }*/
    }
    else
    {
        qWarning() << "未处理的未压缩命令=" << cmd << "   正文=" << json;
        return false;
    }

    triggerCmdEvent(cmd, LiveDanmaku(json.value("data").toObject()));
    return true;
}

bool BiliLiveService::handlePK(QJsonObject json)
{
    QString cmd = json.value("cmd").toString();

    if (cmd == "PK_BATTLE_PRE") // 开始前的等待状态
    {
        return true;
    }
    else if (cmd == "PK_BATTLE_PRE_NEW")
    {
        /*{
            "cmd": "PK_BATTLE_PRE_NEW",
            "data": {
                "battle_type": 1,
                "end_win_task": null,
                "face": "http://i0.hdslb.com/bfs/face/4c0e444dbabe86a3c4a3c47b72e2e63bd4a96684.jpg",
                "match_type": 1,
                "pk_votes_name":"\xE4\xB9\xB1\xE6\x96\x97\xE5\x80\xBC",
                "pre_timer": 10,
                "room_id": 4857111,
                "season_id": 31,
                "uid": 14833326,
                "uname":"\xE5\x8D\x83\xE9\xAD\x82\xE5\x8D\xB0"
            },
            "pk_id": 200271102,
            "pk_status": 101,
            "roomid": 22532956,
            "timestamp": 1611152119
        }*/
        pkPre(json);
        triggerCmdEvent("PK_BATTLE_PRE", LiveDanmaku(json));
        return true;
    }
    if (cmd == "PK_BATTLE_START") // 开始大乱斗
    {
        return true;
    }
    else if (cmd == "PK_BATTLE_START_NEW")
    {
        /*{
            "cmd": "PK_BATTLE_START_NEW",
            "pk_id": 200271102,
            "pk_status": 201,
            "timestamp": 1611152129,
            "data": {
                "battle_type": 1,
                "final_hit_votes": 0,
                "pk_start_time": 1611152129,
                "pk_frozen_time": 1611152429,
                "pk_end_time": 1611152439,
                "pk_votes_type": 0,
                "pk_votes_add": 0,
                "pk_votes_name": "\\u4e71\\u6597\\u503c"
            }
        }*/
        pkStart(json);
        triggerCmdEvent("PK_BATTLE_START", LiveDanmaku(json));
        return true;
    }
    else if (cmd == "PK_BATTLE_PROCESS") // 双方送礼信息
    {
        return true;
    }
    else if (cmd == "PK_BATTLE_PROCESS_NEW")
    {
        /*{
            "cmd": "PK_BATTLE_PROCESS_NEW",
            "pk_id": 200270835,
            "pk_status": 201,
            "timestamp": 1611151874,
            "data": {
                "battle_type": 1,
                "init_info": {
                    "room_id": 2603963,
                    "votes": 55,
                    "best_uname": "\\u963f\\u5179\\u963f\\u5179\\u7684\\u77db"
                },
                "match_info": {
                    "room_id": 22532956,
                    "votes": 184,
                    "best_uname": "\\u591c\\u7a7a\\u3001"
                }
            }
        }*/
        pkProcess(json);
        triggerCmdEvent("PK_BATTLE_PROCESS", LiveDanmaku(json));
        return true;
    }
    else if (cmd == "PK_BATTLE_RANK_CHANGE")
    {
        /*{
            "cmd": "PK_BATTLE_RANK_CHANGE",
            "timestamp": 1611152461,
            "data": {
                "first_rank_img_url": "https:\\/\\/i0.hdslb.com\\/bfs\\/live\\/078e242c4e2bb380554d55d0ac479410d75a0efc.png",
                "rank_name": "\\u767d\\u94f6\\u6597\\u58ebx1\\u661f"
            }
        }*/
    }
    else if (cmd == "PK_BATTLE_FINAL_PROCESS") // 绝杀时刻开始：3分钟~4分钟
    {
        /*{
            "cmd": "PK_BATTLE_FINAL_PROCESS",
            "data": {
                "battle_type": 2, // 1
                "pk_frozen_time": 1628089118
            },
            "pk_id": 205052526,
            "pk_status": 301,     // 201
            "timestamp": 1628088939
        }*/

        triggerCmdEvent("PK_BATTLE_FINAL_PROCESS", LiveDanmaku(json));
        return true;
    }
    else if (cmd == "PK_BATTLE_END") // 结束信息
    {
        /**
         * 结束事件依次是：
         * 1. PK_BATTLE_END（触发 PK_BEST_UNAME & PK_END）
         * 2. PK_BATTLE_SETTLE_USER
         * 3. PK_BATTLE_SETTLE_V2
         */
        pkEnd(json);
    }
    else if (cmd == "PK_BATTLE_SETTLE") // 这个才是真正的PK结束消息！
    {
        /*{
            "cmd": "PK_BATTLE_SETTLE",
            "pk_id": 100729259,
            "pk_status": 401,
            "settle_status": 1,
            "timestamp": 1605748006,
            "data": {
                "battle_type": 1,
                "result_type": 2
            },
            "roomid": "22532956"
        }*/
        // result_type: 2赢，-1输

        // 因为这个不只是data，比较特殊
        // triggerCmdEvent(cmd, LiveDanmaku().with(json));
        return true;
    }
    else if (cmd == "PK_BATTLE_SETTLE_NEW")
    {
        /*{
            "cmd": "PK_BATTLE_SETTLE_NEW",
            "pk_id": 200933662,
            "pk_status": 601,
            "timestamp": 1613959764,
            "data": {
                "pk_id": 200933662,
                "pk_status": 601,
                "settle_status": 1,
                "punish_end_time": 1613959944,
                "timestamp": 1613959764,
                "battle_type": 6,
                "init_info": {
                    "room_id": 7259049,
                    "result_type": -1,
                    "votes": 0,
                    "assist_info": []
                },
                "match_info": {
                    "room_id": 21839758,
                    "result_type": 2,
                    "votes": 3,
                    "assist_info": [
                        {
                            "rank": 1,
                            "uid": 412357310,
                            "face": "http:\\/\\/i0.hdslb.com\\/bfs\\/face\\/e97fbf0e412b936763033055821e1ff5df56565a.jpg",
                            "uname": "\\u6cab\\u58a8\\u58a8\\u58a8\\u58a8\\u58a8\\u58a8\\u58a8\\u58a8"
                        }
                    ]
                },
                "dm_conf": {
                    "font_color": "#FFE10B",
                    "bg_color": "#72C5E2"
                }
            }
        }*/
        pkSettle(json);
        triggerCmdEvent("PK_BATTLE_SETTLE", LiveDanmaku(json));
        return true;
    }
    else if (cmd == "PK_BATTLE_SETTLE_USER")
    {
        /*{
            "cmd": "PK_BATTLE_SETTLE_USER",
            "data": {
                "battle_type": 0,
                "level_info": {
                    "first_rank_img": "https://i0.hdslb.com/bfs/live/078e242c4e2bb380554d55d0ac479410d75a0efc.png",
                    "first_rank_name": "白银斗士",
                    "second_rank_icon": "https://i0.hdslb.com/bfs/live/1f8c2a959f92592407514a1afeb705ddc55429cd.png",
                    "second_rank_num": 1
                },
                "my_info": {
                    "best_user": {
                        "award_info": null,
                        "award_info_list": [],
                        "badge": {
                            "desc": "",
                            "position": 0,
                            "url": ""
                        },
                        "end_win_award_info_list": {
                            "list": []
                        },
                        "exp": {
                            "color": 6406234,
                            "level": 1
                        },
                        "face": "http://i2.hdslb.com/bfs/face/d3d6f8659be3e309d6e58dd77accb3bb300215d5.jpg",
                        "face_frame": "http://i0.hdslb.com/bfs/live/78e8a800e97403f1137c0c1b5029648c390be390.png",
                        "pk_votes": 10,
                        "pk_votes_name": "乱斗值",
                        "uid": 64494330,
                        "uname": "我今天超可爱0"
                    },
                    "exp": {
                        "color": 9868950,
                        "master_level": {
                            "color": 5805790,
                            "level": 17
                        },
                        "user_level": 2
                    },
                    "face": "http://i2.hdslb.com/bfs/face/5b96b6ba5b078001e8159406710a8326d67cee5c.jpg",
                    "face_frame": "https://i0.hdslb.com/bfs/vc/d186b7d67d39e0894ebcc7f3ca5b35b3b56d5192.png",
                    "room_id": 22532956,
                    "uid": 688893202,
                    "uname": "娇娇子er"
                },
                "pk_id": "100729259",
                "result_info": {
                    "pk_crit_score": -1,
                    "pk_done_times": 17,
                    "pk_extra_score": 0,
                    "pk_extra_score_slot": "",
                    "pk_extra_value": 0,
                    "pk_resist_crit_score": -1,
                    "pk_task_score": 0,
                    "pk_times_score": 0,
                    "pk_total_times": -1,
                    "pk_votes": 10,
                    "pk_votes_name": "乱斗值",
                    "result_type_score": 12,
                    "task_score_list": [],
                    "total_score": 12,
                    "win_count": 2,
                    "win_final_hit": -1,
                    "winner_count_score": 0
                },
                "result_type": "2",
                "season_id": 29,
                "settle_status": 1,
                "winner": {
                    "best_user": {
                        "award_info": null,
                        "award_info_list": [],
                        "badge": {
                            "desc": "",
                            "position": 0,
                            "url": ""
                        },
                        "end_win_award_info_list": {
                            "list": []
                        },
                        "exp": {
                            "color": 6406234,
                            "level": 1
                        },
                        "face": "http://i2.hdslb.com/bfs/face/d3d6f8659be3e309d6e58dd77accb3bb300215d5.jpg",
                        "face_frame": "http://i0.hdslb.com/bfs/live/78e8a800e97403f1137c0c1b5029648c390be390.png",
                        "pk_votes": 10,
                        "pk_votes_name": "乱斗值",
                        "uid": 64494330,
                        "uname": "我今天超可爱0"
                    },
                    "exp": {
                        "color": 9868950,
                        "master_level": {
                            "color": 5805790,
                            "level": 17
                        },
                        "user_level": 2
                    },
                    "face": "http://i2.hdslb.com/bfs/face/5b96b6ba5b078001e8159406710a8326d67cee5c.jpg",
                    "face_frame": "https://i0.hdslb.com/bfs/vc/d186b7d67d39e0894ebcc7f3ca5b35b3b56d5192.png",
                    "room_id": 22532956,
                    "uid": 688893202,
                    "uname": "娇娇子er"
                }
            },
            "pk_id": 100729259,
            "pk_status": 401,
            "settle_status": 1,
            "timestamp": 1605748006
        }*/
        // PK结束更新大乱斗信息
        updateWinningStreak(true);
        return true;
    }
    else if (cmd == "PK_BATTLE_SETTLE_V2")
    {
        /*{
            "cmd": "PK_BATTLE_SETTLE_V2",
            "data": {
                "assist_list": [
                    {
                        "face": "http://i2.hdslb.com/bfs/face/d3d6f8659be3e309d6e58dd77accb3bb300215d5.jpg",
                        "id": 64494330,
                        "score": 10,
                        "uname": "我今天超可爱0"
                    }
                ],
                "level_info": {
                    "first_rank_img": "https://i0.hdslb.com/bfs/live/078e242c4e2bb380554d55d0ac479410d75a0efc.png",
                    "first_rank_name": "白银斗士",
                    "second_rank_icon": "https://i0.hdslb.com/bfs/live/1f8c2a959f92592407514a1afeb705ddc55429cd.png",
                    "second_rank_num": 1,
                    "uid": "688893202"
                },
                "pk_id": "100729259",
                "pk_type": "1",
                "result_info": {
                    "pk_extra_value": 0,
                    "pk_votes": 10,
                    "pk_votes_name": "乱斗值",
                    "total_score": 12
                },
                "result_type": 2,
                "season_id": 29
            },
            "pk_id": 100729259,
            "pk_status": 401,
            "settle_status": 1,
            "timestamp": 1605748006
        }*/
        return true;
    }
    else if (cmd == "PK_BATTLE_PUNISH_END")
    {
        /* {
            "cmd": "PK_BATTLE_PUNISH_END",
            "pk_id": "203882854",
            "pk_status": 1001,
            "status_msg": "",
            "timestamp": 1624466237,
            "data": {
                "battle_type": 6
            }
        } */
    }
    else if (cmd == "PK_LOTTERY_START") // 大乱斗胜利后的抽奖，触发未知，实测在某次大乱斗送天空之翼后有
    {
        /*{
            "cmd": "PK_LOTTERY_START",
            "data": {
                "asset_animation_pic": "https://i0.hdslb.com/bfs/vc/03be4c2912a4bd9f29eca3dac059c0e3e3fc69ce.gif",
                "asset_icon": "https://i0.hdslb.com/bfs/vc/44c367b09a8271afa22853785849e65797e085a1.png",
                "from_user": {
                    "face": "http://i2.hdslb.com/bfs/face/f25b706762e00a9adfe13e6147650891dd6f69a0.jpg",
                    "uid": 688893202,
                    "uname": "娇娇子er"
                },
                "id": 200105856,
                "max_time": 120,
                "pk_id": 200105856,
                "room_id": 22532956,
                "thank_text": "恭喜<%娇娇子er%>赢得大乱斗PK胜利",
                "time": 120,
                "time_wait": 0,
                "title": "恭喜主播大乱斗胜利",
                "weight": 0
            }
        }*/
    }
    else
    {
        return false;
    }

    triggerCmdEvent(cmd, LiveDanmaku(json));
    return true;
}

/**
 * 数据包解析： https://segmentfault.com/a/1190000017328813?utm_source=tag-newest#tagDataPackage
 */
void BiliLiveService::handleMessage(QJsonObject json)
{
    QString cmd = json.value("cmd").toString();
    qInfo() << s8(">消息命令ZCOM：") << cmd;
    if (cmd == "LIVE") // 开播？
    {
        emit signalLiveStarted();

        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "PREPARING") // 下播
    {
        emit signalLiveStopped();

        releaseLiveData(true);
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "ROOM_CHANGE")
    {
        getRoomInfo(false);
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "DANMU_MSG" || cmd.startsWith("DANMU_MSG:")) // 收到弹幕
    {
        QJsonArray info = json.value("info").toArray();
        if (info.size() <= 2)
            showError("弹幕数据 info", QString(QJsonDocument(info).toJson()));
        QJsonArray array = info[0].toArray();
        if (array.size() <= 3)
            showError("弹幕数据 array", QString(QJsonDocument(array).toJson()));
        qint64 textColor = array[3].toInt(); // 弹幕颜色
        qint64 timestamp = static_cast<qint64>(array[4].toDouble());
        QString msg = info[1].toString();
        QJsonArray user = info[2].toArray();
        if (user.size() <= 1)
            showError("弹幕数据 user", QString(QJsonDocument(user).toJson()));
        qint64 uid = static_cast<qint64>(user[0].toDouble());
        QString username = user[1].toString();
        int admin = user[2].toInt(); // 是否为房管（实测现在主播不属于房管了）
        int vip = user[3].toInt(); // 是否为老爷
        int svip = user[4].toInt(); // 是否为年费老爷
        int uidentity = user[5].toInt(); // 是否为非正式会员或正式会员（5000非，10000正）
        int iphone = user[6].toInt(); // 是否绑定手机
        QString unameColor = user[7].toString();
        int level = info[4].toArray()[0].toInt();
        QJsonArray medal = info[3].toArray();
        int uguard = info[7].toInt(); // 用户本房间舰队身份：0非，1总督，2提督，3舰长
        int medal_level = 0;
        if (array.size() >= 15) // info[0][14]: voice object
        {
            MyJson voice = array.at(14).toObject();
            JS(voice, voice_url); // 下载直链
            JS(voice, text); // 语音文字
            JI(voice, file_duration); // 秒数

            // 唔，好吧，啥都不用做
        }

        QString cs = QString::number(textColor, 16);
        while (cs.size() < 6)
            cs = "0" + cs;
        LiveDanmaku danmaku(username, msg, uid, level, QDateTime::fromMSecsSinceEpoch(timestamp),
                                                 unameColor, "#"+cs);
        danmaku.setUserInfo(admin, vip, svip, uidentity, iphone, uguard);
        if (medal.size() >= 4)
        {
            medal_level = medal[0].toInt();
            danmaku.setMedal(snum(static_cast<qint64>(medal[3].toDouble())),
                    medal[1].toString(), medal_level, medal[2].toString());
        }

        if (info.at(0).toArray().size() >= 16) // 截止2024.04.20是18个，信息字段在下标15
        {
            MyJson detail = info.at(0).toArray().at(15).toObject();
            MyJson user = detail.o("user");
            MyJson base = user.o("base");
            danmaku.setFaceUrl(base.s("face"));

            QString extra_s = detail.s("extra");
            /*
            {
                "send_from_me": false,
                "mode": 0,
                "color": 16772431,
                "dm_type": 0,
                "font_size": 25,
                "player_mode": 1,
                "show_player_type": 0,
                "content": " 居然还能@人了",
                "user_hash": "3676333332",
                "emoticon_unique": "",
                "bulge_display": 0,
                "recommend_score": 3,
                "main_state_dm_color": "",
                "objective_state_dm_color": "",
                "direction": 0,
                "pk_direction": 0,
                "quartet_direction": 0,
                "anniversary_crowd": 0,
                "yeah_space_type": "",
                "yeah_space_url": "",
                "jump_to_url": "",
                "space_type": "",
                "space_url": "",
                "animation": {},
                "emots": null,
                "is_audited": false,
                "id_str": "5b07eaab49ab75ed743f954e8566f3b694",
                "icon": null,
                "show_reply": true,
                "reply_mid": 20285041,
                "reply_uname": "懒一夕智能科技官方",
                "reply_uname_color": "#FB7299",
                "reply_is_mystery": false,
                "reply_type_enum": 1,
                "hit_combo": 0,
                "esports_jump_url": ""
            }
            */
            MyJson extra = MyJson::from(extra_s.toLatin1());
            if (!extra.isEmpty())
            {
                bool show_reply = extra.b("show_reply");
                qint64 reply_mid = extra.l("reply_mid");
                QString reply_uname = extra.s("reply_uname");
                QString reply_uname_color = extra.s("reply_uname_color");
                bool reply_is_mystery = extra.b("reply_is_mystery");
                int reply_type_enum = extra.i("reply_type_enum");

                danmaku.setReplyInfo(show_reply, reply_mid, reply_uname, reply_uname_color, reply_is_mystery, reply_type_enum);
            }
            else
            {
                if (!extra_s.isEmpty())
                {
                    qWarning() << "解析reply失败：" << extra_s;
                }
            }
        }

        receiveDanmaku(danmaku.with(json));
    }
    else if (cmd == "SEND_GIFT") // 有人送礼
    {
        /*{
            "cmd": "SEND_GIFT",
            "data": {
                "draw": 0,
                "gold": 0,
                "silver": 0,
                "num": 1,
                "total_coin": 0,
                "effect": 0,
                "broadcast_id": 0,
                "crit_prob": 0,
                "guard_level": 0,
                "rcost": 200773,
                "uid": 20285041,
                "timestamp": 1614439816,
                "giftId": 30607,
                "giftType": 5,
                "super": 0,
                "super_gift_num": 1,
                "super_batch_gift_num": 1,
                "remain": 19,
                "price": 0,
                "beatId": "",
                "biz_source": "Live",
                "action": "投喂",
                "coin_type": "silver",
                "uname": "懒一夕智能科技",
                "face": "http://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg",
                "batch_combo_id": "batch:gift:combo_id:20285041:2070473390:30607:1614439816.1655",      // 多个以及最后的 COMBO_SEND 是一样的
                "rnd": "34158224",
                "giftName": "小心心",
                "combo_send": null,
                "batch_combo_send": null,
                "tag_image": "",
                "top_list": null,
                "send_master": null,
                "is_first": true,                   // 不是第一个就是false
                "demarcation": 1,
                "combo_stay_time": 3,
                "combo_total_coin": 1,
                "tid": "1614439816120100003",
                "effect_block": 1,
                "is_special_batch": 0,
                "combo_resources_id": 1,
                "magnification": 1,
                "name_color": "",
                "medal_info": {
                    "target_id": 0,
                    "special": "",
                    "icon_id": 0,
                    "anchor_uname": "",
                    "anchor_roomid": 0,
                    "medal_level": 0,
                    "medal_name": "",
                    "medal_color": 0,
                    "medal_color_start": 0,
                    "medal_color_end": 0,
                    "medal_color_border": 0,
                    "is_lighted": 0,
                    "guard_level": 0
                },
                "svga_block": 0
            }
        }*/
        /*{
            "cmd": "SEND_GIFT",
            "data": {
                "draw": 0,
                "gold": 0,
                "silver": 0,
                "num": 1,
                "total_coin": 1000,
                "effect": 0,                // 太便宜的礼物没有
                "broadcast_id": 0,
                "crit_prob": 0,
                "guard_level": 0,
                "rcost": 200795,
                "uid": 20285041,
                "timestamp": 1614440753,
                "giftId": 30823,
                "giftType": 0,
                "super": 0,
                "super_gift_num": 1,
                "super_batch_gift_num": 1,
                "remain": 0,
                "price": 1000,
                "beatId": "",
                "biz_source": "Live",
                "action": "投喂",
                "coin_type": "gold",
                "uname": "懒一夕智能科技",
                "face": "http://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg",
                "batch_combo_id": "batch:gift:combo_id:20285041:2070473390:30823:1614440753.1786",
                "rnd": "245758485",
                "giftName": "小巧花灯",
                "combo_send": {
                    "uid": 20285041,
                    "gift_num": 1,
                    "combo_num": 1,
                    "gift_id": 30823,
                    "combo_id": "gift:combo_id:20285041:2070473390:30823:1614440753.1781",
                    "gift_name": "小巧花灯",
                    "action": "投喂",
                    "uname": "懒一夕智能科技",
                    "send_master": null
                },
                "batch_combo_send": {
                    "uid": 20285041,
                    "gift_num": 1,
                    "batch_combo_num": 1,
                    "gift_id": 30823,
                    "batch_combo_id": "batch:gift:combo_id:20285041:2070473390:30823:1614440753.1786",
                    "gift_name": "小巧花灯",
                    "action": "投喂",
                    "uname": "懒一夕智能科技",
                    "send_master": null
                },
                "tag_image": "",
                "top_list": null,
                "send_master": null,
                "is_first": true,
                "demarcation": 2,
                "combo_stay_time": 3,
                "combo_total_coin": 1000,
                "tid": "1614440753121100001",
                "effect_block": 0,
                "is_special_batch": 0,
                "combo_resources_id": 1,
                "magnification": 1,
                "name_color": "",
                "medal_info": {
                    "target_id": 0,
                    "special": "",
                    "icon_id": 0,
                    "anchor_uname": "",
                    "anchor_roomid": 0,
                    "medal_level": 0,
                    "medal_name": "",
                    "medal_color": 0,
                    "medal_color_start": 0,
                    "medal_color_end": 0,
                    "medal_color_border": 0,
                    "is_lighted": 0,
                    "guard_level": 0
                },
                "svga_block": 0
            }
        }*/
        QJsonObject data = json.value("data").toObject();
        int giftId = data.value("giftId").toInt(); // 盲盒是爆出来的ID
        int giftType = data.value("giftType").toInt(); // 不知道是啥，金瓜子1，银瓜子（小心心、辣条）5？
        QString giftName = data.value("giftName").toString(); // 盲盒是爆出来的礼物名字
        QString username = data.value("uname").toString();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        int num = data.value("num").toInt();
        qint64 timestamp = static_cast<qint64>(data.value("timestamp").toDouble()); // 秒
        timestamp = QDateTime::currentSecsSinceEpoch(); // *不管送出礼物的时间，只管机器人接收到的时间
        QString coinType = data.value("coin_type").toString();
        qint64 totalCoin = data.value("total_coin").toDouble();
        qint64 discountPrice = data.value("discount_price").toDouble(); // 盲盒实际爆出的单个礼物价值

        qInfo() << s8("接收到送礼：") << username << giftId << giftName << num << s8("  总价值：") << totalCoin << discountPrice << coinType;
        QString localName = us->getLocalNickname(uid);
        /*if (!localName.isEmpty())
            username = localName;*/
        LiveDanmaku danmaku(username, giftId, giftName, num, uid, QDateTime::fromSecsSinceEpoch(timestamp), coinType, totalCoin);
        danmaku.setDiscountPrice(discountPrice);
        if (!data.value("medal_info").isNull())
        {
            QJsonObject medalInfo = data.value("medal_info").toObject();
            QString anchorRoomId = snum(qint64(medalInfo.value("anchor_room_id").toDouble())); // !注意：这个一直为0！
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

        // 触发事件
        receiveGift(danmaku.with(json));

        // 进行统计
        if (coinType == "silver")
        {
            qint64 userSilver = us->danmakuCounts->value("silver/" + snum(uid)).toLongLong();
            userSilver += totalCoin;
            us->danmakuCounts->setValue("silver/"+snum(uid), userSilver);

            dailyGiftSilver += totalCoin;
            if (dailySettings)
                dailySettings->setValue("gift_silver", dailyGiftSilver);
            currentLiveGiftSilver += totalCoin;
            if (currentLiveSettings)
                currentLiveSettings->setValue("gift_silver", currentLiveGiftSilver);
        }
        if (coinType == "gold")
        {
            qint64 userGold = us->danmakuCounts->value("gold/" + snum(uid)).toLongLong();
            userGold += totalCoin;
            us->danmakuCounts->setValue("gold/"+snum(uid), userGold);

            dailyGiftGold += totalCoin;
            if (dailySettings)
                dailySettings->setValue("gift_gold", dailyGiftGold);
            currentLiveGiftGold += totalCoin;
            if (currentLiveSettings)
                currentLiveSettings->setValue("gift_gold", currentLiveGiftGold);

            // 正在PK，保存弹幕历史
            // 因为最后的大乱斗最佳助攻只提供名字，所以这里需要保存 uname->uid 的映射
            // 方便起见，直接全部保存下来了
            pkGifts.append(danmaku);

            // 添加礼物记录
            appendLiveGift(danmaku);

            // 正在偷塔阶段
            if (pkEnding && uid == ac->cookieUid.toLongLong()) // 机器人账号
            {
//                pkVoting -= totalCoin;
//                if (pkVoting < 0) // 自己用其他设备送了更大的礼物
//                {
//                    pkVoting = 0;
//                }
            }
        }
    }
    else if (cmd == "COMBO_SEND") // 连击礼物
    {
        /*{
            "cmd": "COMBO_SEND",
            "data": {
                "action": "投喂",
                "batch_combo_id": "batch:gift:combo_id:8833188:354580019:30607:1610168283.0188",
                "batch_combo_num": 9,
                "combo_id": "gift:combo_id:8833188:354580019:30607:1610168283.0182",
                "combo_num": 9,
                "combo_total_coin": 0,
                "gift_id": 30607,
                "gift_name": "小心心",
                "gift_num": 0,
                "is_show": 1,
                "medal_info": {
                    "anchor_roomid": 0,
                    "anchor_uname": "",
                    "guard_level": 0,
                    "icon_id": 0,
                    "is_lighted": 1,
                    "medal_color": 1725515,
                    "medal_color_border": 1725515,
                    "medal_color_end": 5414290,
                    "medal_color_start": 1725515,
                    "medal_level": 23,
                    "medal_name": "好听",
                    "special": "",
                    "target_id": 354580019
                },
                "name_color": "",
                "r_uname": "薄薄的温酱",
                "ruid": 354580019,
                "send_master": null,
                "total_num": 9,
                "uid": 8833188,
                "uname": "南酱的可露儿"
            }
        }*/
        /*{
            "cmd": "COMBO_SEND",
            "data": {
                "uid": 20285041,
                "ruid": 2070473390,
                "uname": "懒一夕智能科技",
                "r_uname": "喵大王_cat",
                "combo_num": 4,
                "gift_id": 30607,
                "gift_num": 0,
                "batch_combo_num": 4,
                "gift_name": "小心心",
                "action": "投喂",
                "combo_id": "gift:combo_id:20285041:2070473390:30607:1614439816.1648",
                "batch_combo_id": "batch:gift:combo_id:20285041:2070473390:30607:1614439816.1655",
                "is_show": 1,
                "send_master": null,
                "name_color": "",
                "total_num": 4,
                "medal_info": {
                    "target_id": 0,
                    "special": "",
                    "icon_id": 0,
                    "anchor_uname": "",
                    "anchor_roomid": 0,
                    "medal_level": 0,
                    "medal_name": "",
                    "medal_color": 0,
                    "medal_color_start": 0,
                    "medal_color_end": 0,
                    "medal_color_border": 0,
                    "is_lighted": 0,
                    "guard_level": 0
                },
                "combo_total_coin": 0
            }
        }*/

        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "SUPER_CHAT_MESSAGE") // 醒目留言
    {
        /*{
            "cmd": "SUPER_CHAT_MESSAGE",
            "data": {
                "background_bottom_color": "#2A60B2",
                "background_color": "#EDF5FF",
                "background_color_end": "#405D85",
                "background_color_start": "#3171D2",
                "background_icon": "",
                "background_image": "https://i0.hdslb.com/bfs/live/a712efa5c6ebc67bafbe8352d3e74b820a00c13e.png",
                "background_price_color": "#7497CD",
                "color_point": 0.7,
                "end_time": 1613125905,
                "gift": {
                    "gift_id": 12000,
                    "gift_name": "醒目留言", // 这个是特殊礼物？
                    "num": 1
                },
                "id": 1278390,
                "is_ranked": 0,
                "is_send_audit": "0",
                "medal_info": {
                    "anchor_roomid": 1010,
                    "anchor_uname": "KB呆又呆",
                    "guard_level": 3,
                    "icon_id": 0,
                    "is_lighted": 1,
                    "medal_color": "#6154c",
                    "medal_color_border": 6809855,
                    "medal_color_end": 6850801,
                    "medal_color_start": 398668,
                    "medal_level": 25,
                    "medal_name": "KKZ",
                    "special": "",
                    "target_id": 389088
                },
                "message": "最右边可以爬上去",
                "message_font_color": "#A3F6FF",
                "message_trans": "",
                "price": 30,
                "rate": 1000,
                "start_time": 1613125845,
                "time": 60,
                "token": "767AB474",
                "trans_mark": 0,
                "ts": 1613125845,
                "uid": 35030958,
                "user_info": {
                    "face": "http://i0.hdslb.com/bfs/face/cdd8fbb13b2034dc3651096cbeef4b5e89765c35.jpg",
                    "face_frame": "http://i0.hdslb.com/bfs/live/78e8a800e97403f1137c0c1b5029648c390be390.png",
                    "guard_level": 3,
                    "is_main_vip": 1,
                    "is_svip": 0,
                    "is_vip": 0,
                    "level_color": "#61c05a",
                    "manager": 0,
                    "name_color": "#00D1F1",
                    "title": "0",
                    "uname": "么么么么么么么么句号",
                    "user_level": 14
                }
            },
            "roomid": "1010"
        }*/

        MyJson data = json.value("data").toObject();
        JL(data, uid);
        JS(data, message);
        JS(data, message_font_color);
        JL(data, end_time); // 秒
        JI(data, price); // 注意这个是价格（单位元），不是金瓜子，并且30起步

        JO(data, gift);
        JI(gift, gift_id);
        JS(gift, gift_name);
        JI(gift, num);

        JO(data, user_info);
        JS(user_info, uname);
        JI(user_info, user_level);
        JS(user_info, name_color);
        JI(user_info, is_main_vip);
        JI(user_info, is_vip);
        JI(user_info, is_svip);

        JO(data, medal_info);
        JL(medal_info, anchor_roomid);
        JS(medal_info, anchor_uname);
        JI(medal_info, guard_level);
        JI(medal_info, medal_level);
        JS(medal_info, medal_color);
        JS(medal_info, medal_name);
        JL(medal_info, target_id);

        if (gift_id != 12000) // 醒目留言
        {
            qWarning() << "非醒目留言的特殊聊天消息：" << json;
            return ;
        }

        LiveDanmaku danmaku(uname, message, uid, user_level, QDateTime::fromSecsSinceEpoch(end_time), name_color, message_font_color,
                    gift_id, gift_name, num, price);
        danmaku.setMedal(snum(anchor_roomid), medal_name, medal_level, medal_color, anchor_uname);
        appendNewLiveDanmaku(danmaku);

        pkGifts.append(danmaku);
        triggerCmdEvent(cmd, danmaku.with(data));
    }
    else if (cmd == "SUPER_CHAT_MESSAGE_JPN") // 醒目留言日文翻译
    {
        /*{
            "cmd": "SUPER_CHAT_MESSAGE_JPN",
            "data": {
                "background_bottom_color": "#2A60B2",
                "background_color": "#EDF5FF",
                "background_icon": "",
                "background_image": "https://i0.hdslb.com/bfs/live/a712efa5c6ebc67bafbe8352d3e74b820a00c13e.png",
                "background_price_color": "#7497CD",
                "end_time": 1613125905,
                "gift": {
                    "gift_id": 12000,
                    "gift_name": "醒目留言",
                    "num": 1
                },
                "id": "1278390",
                "is_ranked": 0,
                "medal_info": {
                    "anchor_roomid": 1010,
                    "anchor_uname": "KB呆又呆",
                    "icon_id": 0,
                    "medal_color": "#6154c",
                    "medal_level": 25,
                    "medal_name": "KKZ",
                    "special": "",
                    "target_id": 389088
                },
                "message": "最右边可以爬上去",
                "message_jpn": "",
                "price": 30,
                "rate": 1000,
                "start_time": 1613125845,
                "time": 60,
                "token": "767AB474",
                "ts": 1613125845,
                "uid": "35030958",
                "user_info": {
                    "face": "http://i0.hdslb.com/bfs/face/cdd8fbb13b2034dc3651096cbeef4b5e89765c35.jpg",
                    "face_frame": "http://i0.hdslb.com/bfs/live/78e8a800e97403f1137c0c1b5029648c390be390.png",
                    "guard_level": 3,
                    "is_main_vip": 1,
                    "is_svip": 0,
                    "is_vip": 0,
                    "level_color": "#61c05a",
                    "manager": 0,
                    "title": "0",
                    "uname": "么么么么么么么么句号",
                    "user_level": 14
                }
            },
            "roomid": "1010"
        }*/
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "SUPER_CHAT_MESSAGE_DELETE") // 删除醒目留言
    {
        qInfo() << "删除醒目留言：" << json;
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "SPECIAL_GIFT") // 节奏风暴（特殊礼物？）
    {
        /*{
            "cmd": "SPECIAL_GIFT",
            "data": {
                "39": {
                    "action": "start",
                    "content": "糟了，是心动的感觉！",
                    "hadJoin": 0,
                    "id": "3072200788973",
                    "num": 1,
                    "storm_gif": "http://static.hdslb.com/live-static/live-room/images/gift-section/mobilegift/2/jiezou.gif?2017011901",
                    "time": 90
                }
            }
        }*/

        QJsonObject data = json.value("data").toObject();
        QJsonObject sg = data.value("39").toObject();
        QString text = sg.value("content").toString();
        qint64 id = qint64(sg.value("id").toDouble());
        int time = sg.value("time").toInt();
        emit signalDanmakuAddBlockText(text, time);

        qInfo() << "节奏风暴：" << text << us->autoJoinLOT;
        if (us->autoJoinLOT)
        {
            joinStorm(id);
        }

        triggerCmdEvent(cmd, LiveDanmaku().with(data));
    }
    else if (cmd == "WELCOME_GUARD") // 舰长进入（不会触发），通过guard_level=1/2/3分辨总督/提督/舰长
    {
        QJsonObject data = json.value("data").toObject();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString username = data.value("username").toString();
        qint64 startTime = static_cast<qint64>(data.value("start_time").toDouble());
        qint64 endTime = static_cast<qint64>(data.value("end_time").toDouble());
        qint64 timestamp = static_cast<qint64>(data.value("timestamp").toDouble());
        QString unameColor = data.value("uname_color").toString();
        QString spreadDesc = data.value("spread_desc").toString();
        QString spreadInfo = data.value("spread_info").toString();
        qInfo() << s8("舰长进入：") << username;
        /*QString localName = danmakuWindow->getLocalNickname(uid);
        if (!localName.isEmpty())
            username = localName;*/
        LiveDanmaku danmaku(LiveDanmaku(username, uid, QDateTime::fromSecsSinceEpoch(timestamp)
                                        , true, unameColor, spreadDesc, spreadInfo));
        appendNewLiveDanmaku(danmaku);

        triggerCmdEvent(cmd, danmaku.with(data));
    }
    else  if (cmd == "ENTRY_EFFECT") // 舰长进入、高能榜（不知道到榜几）、姥爷的同时会出现
    {
        // 欢迎舰长
        /*{
            "cmd": "ENTRY_EFFECT",
            "data": {
                "id": 4,
                "uid": 20285041,
                "target_id": 688893202,
                "mock_effect": 0,
                "face": "https://i2.hdslb.com/bfs/face/24420cdcb6eeb119dbcd1f1843fdd8ada5b7d045.jpg",
                "privilege_type": 3,
                "copy_writing": "欢迎舰长 \\u003c%心乂鸽鸽%\\u003e 进入直播间",
                "copy_color": "#ffffff",
                "highlight_color": "#E6FF00",
                "priority": 70,
                "basemap_url": "https://i0.hdslb.com/bfs/live/mlive/f34c7441cdbad86f76edebf74e60b59d2958f6ad.png",
                "show_avatar": 1,
                "effective_time": 2,
                "web_basemap_url": "",
                "web_effective_time": 0,
                "web_effect_close": 0,
                "web_close_time": 0,
                "business": 1,
                "copy_writing_v2": "欢迎 \\u003c^icon^\\u003e 舰长 \\u003c%心乂鸽鸽%\\u003e 进入直播间",
                "icon_list": [
                    2
                ],
                "max_delay_time": 7
            }
        }*/

        // 欢迎姥爷
        /*{
            "cmd": "ENTRY_EFFECT",
            "data": {
                "basemap_url": "https://i0.hdslb.com/bfs/live/mlive/586f12135b6002c522329904cf623d3f13c12d2c.png",
                "business": 3,
                "copy_color": "#000000",
                "copy_writing": "欢迎 <%___君陌%> 进入直播间",
                "copy_writing_v2": "欢迎 <^icon^> <%___君陌%> 进入直播间",
                "effective_time": 2,
                "face": "https://i1.hdslb.com/bfs/face/8fb8336e1ae50001ca76b80c30b01d23b07203c9.jpg",
                "highlight_color": "#FFF100",
                "icon_list": [
                    2
                ],
                "id": 136,
                "max_delay_time": 7,
                "mock_effect": 0,
                "priority": 1,
                "privilege_type": 0,
                "show_avatar": 1,
                "target_id": 5988102,
                "uid": 453364,
                "web_basemap_url": "https://i0.hdslb.com/bfs/live/mlive/586f12135b6002c522329904cf623d3f13c12d2c.png",
                "web_close_time": 900,
                "web_effect_close": 0,
                "web_effective_time": 2
            }
        }*/
        QJsonObject data = json.value("data").toObject();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString copy_writing = data.value("copy_writing").toString();
        QStringList results = QRegularExpression("欢迎(舰长|提督|总督)?.+?<%(.+)%>").match(copy_writing).capturedTexts();
        LiveDanmaku danmaku;
        if (results.size() < 2 || results.at(1).isEmpty()) // 不是船员
        {
            qInfo() << "高能榜进入：" << copy_writing;
            QStringList results = QRegularExpression("^欢迎(尊享用户)?\\s*<%(.+)%>").match(copy_writing).capturedTexts();
            if (results.size() < 2)
            {
                qWarning() << "识别舰长进入失败：" << copy_writing;
                qWarning() << data;
                return ;
            }

            // 高能榜上的用户
            for (int i = 0; i < onlineGoldRank.size(); i++)
            {
                if (onlineGoldRank.at(i).getUid() == uid)
                {
                    danmaku = onlineGoldRank.at(i); // 就是这个用户了
                    danmaku.setTime(QDateTime::currentDateTime());
                    qInfo() << "                guard:" << danmaku.getGuard();
                    break;
                }
            }
            if (danmaku.getUid() == 0)
            {
                // qWarning() << "未在已有高能榜上找到，立即更新高能榜";
                // updateOnlineGoldRank();
                return ;
            }

            // 高能榜的，不能有guard，不然会误判为牌子
            // danmaku.setGuardLevel(0);
        }
        else // 舰长进入
        {
            qInfo() << ">>>>>>舰长进入：" << copy_writing;

            QString gd = results.at(1);
            QString uname = results.at(2); // 这个昵称会被系统自动省略（太长后面会是两个点）
            if (ac->currentGuards.contains(uid))
                uname = ac->currentGuards[uid];
            int guardLevel = 0;
            if (gd == "总督")
                guardLevel = 1;
            else if (gd == "提督")
                guardLevel = 2;
            else if (gd == "舰长")
                guardLevel = 3;

            danmaku = LiveDanmaku(guardLevel, uname, uid, QDateTime::currentDateTime());
        }

        // userComeEvent(danmaku); // 用户进入就有提示了（舰长提示会更频繁）
        triggerCmdEvent(cmd, danmaku.with(data));
    }
    else if (cmd == "WELCOME") // 欢迎老爷，通过vip和svip区分月费和年费老爷
    {
        QJsonObject data = json.value("data").toObject();
        qInfo() << data;
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString username = data.value("uname").toString();
        bool isAdmin = data.value("isAdmin").toBool();
        qInfo() << s8("欢迎观众：") << username << isAdmin;

        triggerCmdEvent(cmd, LiveDanmaku().with(data));
    }
    else if (cmd == "INTERACT_WORD")
    {
        /* {
            "cmd": "INTERACT_WORD",
            "data": {
                "contribution": {
                    "grade": 0
                },
                "fans_medal": {
                    "anchor_roomid": 0,
                    "guard_level": 0,
                    "icon_id": 0,
                    "is_lighted": 0,
                    "medal_color": 0,
                    "medal_color_border": 12632256,
                    "medal_color_end": 12632256,
                    "medal_color_start": 12632256,
                    "medal_level": 0,
                    "medal_name": "",
                    "score": 0,
                    "special": "",
                    "target_id": 0
                },
                "identities": [
                    1
                ],
                "is_spread": 0,
                "msg_type": 4,
                "roomid": 22639465,
                "score": 1617974941375,
                "spread_desc": "",
                "spread_info": "",
                "tail_icon": 0,
                "timestamp": 1617974941,
                "uid": 20285041,
                "uname": "懒一夕智能科技",
                "uname_color": ""
            }
        } */

        QJsonObject data = json.value("data").toObject();
        int msgType = data.value("msg_type").toInt(); // 1进入直播间，2关注，3分享直播间，4特别关注
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString username = data.value("uname").toString();
        qint64 timestamp = static_cast<qint64>(data.value("timestamp").toDouble());
        bool isadmin = data.value("isadmin").toBool(); // TODO:这里没法判断房管？
        QString unameColor = data.value("uname_color").toString();
        bool isSpread = data.value("is_spread").toBool();
        QString spreadDesc = data.value("spread_desc").toString();
        QString spreadInfo = data.value("spread_info").toString();
        QJsonObject fansMedal = data.value("fans_medal").toObject();
        QString roomId = snum(qint64(data.value("room_id").toDouble()));

        qInfo() << s8("观众交互：") << username << msgType;
        QString localName = us->getLocalNickname(uid);
        /*if (!localName.isEmpty())
            username = localName;*/
        LiveDanmaku danmaku(username, uid, QDateTime::fromSecsSinceEpoch(timestamp), isadmin,
                            unameColor, spreadDesc, spreadInfo);
        danmaku.setMedal(snum(static_cast<qint64>(fansMedal.value("anchor_roomid").toDouble())),
                         fansMedal.value("medal_name").toString(),
                         fansMedal.value("medal_level").toInt(),
                         QString("#%1").arg(fansMedal.value("medal_color").toInt(), 6, 16, QLatin1Char('0')),
                         "");

        bool opposite = pking &&
                ((oppositeAudience.contains(uid) && !myAudience.contains(uid))
                 || (!pkRoomId.isEmpty() &&
                     snum(static_cast<qint64>(fansMedal.value("anchor_roomid").toDouble())) == pkRoomId));
        danmaku.setOpposite(opposite);
        danmaku.with(data);

        for (const auto& guard: guardInfos)
            if (guard.getUid() == uid)
            {
                danmaku.setGuardLevel(guard.getGuard());
                break;
            }

        if (roomId != "0" && roomId != ac->roomId) // 关注对面主播，也会引发关注事件
        {
            qInfo() << "不是本房间，已忽略：" << roomId << "!=" << ac->roomId;
            return ;
        }

        if (msgType == 1) // 进入直播间
        {
            receiveUserCome(danmaku);

            triggerCmdEvent(cmd, danmaku.with(data));
        }
        else if (msgType == 2) // 2关注 4特别关注
        {
            danmaku.transToAttention(timestamp);
            appendNewLiveDanmaku(danmaku);

            emit signalSendAttentionThank(danmaku);

            triggerCmdEvent("ATTENTION", danmaku.with(data)); // !这个是单独修改的
        }
        else if (msgType == 3) // 分享直播间
        {
            danmaku.transToShare();
            localNotify(username + "分享了直播间", uid);

            triggerCmdEvent("SHARE", danmaku.with(data));
        }
        else if (msgType == 4) // 特别关注
        {
            danmaku.transToAttention(timestamp);
            danmaku.setSpecial(1);
            appendNewLiveDanmaku(danmaku);

            emit signalSendAttentionThank(danmaku);

            triggerCmdEvent("SPECIAL_ATTENTION", danmaku.with(data)); // !这个是单独修改的
        }
        else if (msgType == 5)
        {
            // TODO:没懂是啥，难道是取关？
        }
        else
        {
            qWarning() << "~~~~~~~~~~~~~~~~~~~~~~~~新的进入msgType" << msgType << json;
        }
    }
    else if (cmd == "ROOM_BLOCK_MSG") // 被禁言
    {
        /*{
            "cmd": "ROOM_BLOCK_MSG",
            "data": {
                "dmscore": 30,
                "operator": 1,
                "uid": 536522379,
                "uname": "sescuerYOKO"
            },
            "uid": 536522379,
            "uname": "sescuerYOKO"
        }*/
        QString nickname = json.value("uname").toString();
        qint64 uid = static_cast<qint64>(json.value("uid").toDouble());
        LiveDanmaku danmaku(LiveDanmaku(nickname, uid));
        appendNewLiveDanmaku(danmaku);
        rt->blockedQueue.append(danmaku);

        triggerCmdEvent(cmd, danmaku.with(json));
    }
    else if (handlePK(json))
    {
    }
    else if (cmd == "GUARD_BUY") // 有人上舰
    {
        // {"end_time":1611343771,"gift_id":10003,"gift_name":"舰长","guard_level":3,"num":1,"price":198000,"start_time":1611343771,"uid":67756641,"username":"31119657605_bili"}
        QJsonObject data = json.value("data").toObject();
        qint64 uid = static_cast<qint64>(data.value("uid").toDouble());
        QString username = data.value("username").toString();
        QString giftName = data.value("gift_name").toString();
        int price = data.value("price").toInt();
        int gift_id = data.value("gift_id").toInt();
        int guard_level = data.value("guard_level").toInt();
        int num = data.value("num").toInt();
        // start_time和end_time都是当前时间？
        int guardCount = us->danmakuCounts->value("guard/" + snum(uid), 0).toInt();
        qInfo() << username << s8("购买") << giftName << num << guardCount;
        LiveDanmaku danmaku(username, uid, giftName, num, guard_level, gift_id, price,
                            guardCount == 0 ? 1 : ac->currentGuards.contains(uid) ? 0 : 2);
        danmaku.with(data);
        danmaku.setFirst(guardCount == 0);
        appendNewLiveDanmaku(danmaku);
        appendLiveGuard(danmaku);

        emit signalNewGuardBuy(danmaku);

        qint64 userGold = us->danmakuCounts->value("gold/" + snum(uid)).toLongLong();
        userGold += price;
        us->danmakuCounts->setValue("gold/"+snum(uid), userGold);

        int addition = 1;
        if (giftName == "舰长")
            addition = 1;
        else if (giftName == "提督")
            addition = 10;
        else if (giftName == "总督")
            addition = 100;
        guardCount += addition;
        us->danmakuCounts->setValue("guard/" + snum(uid), guardCount);

        dailyGuard += num;
        if (dailySettings)
            dailySettings->setValue("guard", dailyGuard);
        currentLiveGuard += num;
        if (currentLiveSettings)
            currentLiveSettings->setValue("guard", currentLiveGuard);

        triggerCmdEvent(cmd, danmaku.with(data));
    }
    else if (cmd == "USER_TOAST_MSG") // 续费舰长会附带的；购买不知道
    {
        /*{
            "cmd": "USER_TOAST_MSG",
            "data": {
                "anchor_show": true,
                "color": "#00D1F1",
                "end_time": 1610167490,
                "guard_level": 3,
                "is_show": 0,
                "num": 1,
                "op_type": 3,
                "payflow_id": "2101091244192762134756573",
                "price": 138000,
                "role_name": "舰长",
                "start_time": 1610167490,
                "svga_block": 0,
                "target_guard_count": 128,
                "toast_msg": "<%分说的佛酱%> 自动续费了舰长",
                "uid": 480643475,
                "unit": "月",
                "user_show": true,
                "username": "分说的佛酱"
            }
        }*/

        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "ONLINE_RANK_V2") // 礼物榜（高能榜）更新
    {
        /*{
            "cmd": "ONLINE_RANK_V2",
            "data": {
                "list": [
                    {
                        "face": "http://i0.hdslb.com/bfs/face/5eae154b4ed09c8ae4017325f5fa1ed8fa3757a9.jpg",
                        "guard_level": 3,
                        "rank": 1,
                        "score": "1380",
                        "uid": 480643475,
                        "uname": "分说的佛酱"
                    },
                    {
                        "face": "http://i2.hdslb.com/bfs/face/65536122b97302b86b93847054d4ab8cc155afe3.jpg",
                        "guard_level": 3,
                        "rank": 2,
                        "score": "610",
                        "uid": 407543009,
                        "uname": "布可人"
                    },
                    ..........
                ],
                "rank_type": "gold-rank"
            }
        }*/
        QJsonArray array = json.value("data").toObject().value("list").toArray();
        // 因为高能榜上的只有名字和ID，没有粉丝牌，有需要的话还是需要手动刷新一下
        if (array.size() != onlineGoldRank.size())
        {
            // 还是不更新了吧，没有必要
            // updateOnlineGoldRank();
        }
        else
        {
            // 如果仅仅是排名和金瓜子，那么就挨个修改吧
            foreach (auto val, array)
            {
                auto user = val.toObject();
                qint64 uid = static_cast<qint64>(user.value("uid").toDouble());
                int score = user.value("score").toInt();
                int rank = user.value("rank").toInt();

                for (int i = 0; i < onlineGoldRank.size(); i++)
                {
                    if (onlineGoldRank.at(i).getUid() == uid)
                    {
                        onlineGoldRank[i].setFirst(rank);
                        onlineGoldRank[i].setTotalCoin(score);
                        break;
                    }
                }
            }
        }

        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "ONLINE_RANK_TOP3") // 高能榜前3变化（不知道会不会跟着 ONLINE_RANK_V2）
    {
        /*{
            "cmd": "ONLINE_RANK_TOP3",
            "data": {
                "list": [
                    {
                        "msg": "恭喜 <%分说的佛酱%> 成为高能榜",
                        "rank": 1
                    }
                ]
            }
        }*/

        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "ONLINE_RANK_COUNT") // 高能榜数量变化（但必定会跟着 ONLINE_RANK_V2）
    {
        /*{
            "cmd": "ONLINE_RANK_COUNT",
            "data": {
                "count": 9
            }
        }*/

        // 数量变化了，那还是得刷新一下
        updateOnlineGoldRank();

        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "NOTICE_MSG") // 为什么压缩的消息还有一遍？
    {
        /*{
            "business_id": "",
            "cmd": "NOTICE_MSG",
            "full": {
                "background": "#FFB03CFF",
                "color": "#FFFFFFFF",
                "head_icon": "https://i0.hdslb.com/bfs/live/72337e86020b8d0874d817f15c48a610894b94ff.png",
                "head_icon_fa": "https://i0.hdslb.com/bfs/live/72337e86020b8d0874d817f15c48a610894b94ff.png",
                "head_icon_fan": 1,
                "highlight": "#B25AC1FF",
                "tail_icon": "https://i0.hdslb.com/bfs/live/822da481fdaba986d738db5d8fd469ffa95a8fa1.webp",
                "tail_icon_fa": "https://i0.hdslb.com/bfs/live/38cb2a9f1209b16c0f15162b0b553e3b28d9f16f.png",
                "tail_icon_fan": 4,
                "time": 10
            },
            "half": {
                "background": "",
                "color": "",
                "head_icon": "",
                "highlight": "",
                "tail_icon": "",
                "time": 0
            },
            "link_url": "",
            "msg_common": "",
            "msg_self": "<%分说的佛酱%> 自动续费了主播的 <%舰长%>",
            "msg_type": 3,
            "real_roomid": 12735949,
            "roomid": 12735949,
            "scatter": {
                "max": 0,
                "min": 0
            },
            "shield_uid": -1,
            "side": {
                "background": "#FFE9C8FF",
                "border": "#FFCFA4FF",
                "color": "#EF903AFF",
                "head_icon": "https://i0.hdslb.com/bfs/live/31566d8cd5d468c30de8c148c5d06b3b345d8333.png",
                "highlight": "#D54900FF"
            }
        }*/

        triggerCmdEvent(cmd, LiveDanmaku());
    }
    else if (cmd == "SPECIAL_GIFT")
    {
        /*{
            "cmd": "SPECIAL_GIFT",
            "data": {
                "39": {
                    "action": "end",
                    "id": 3032328093737
                }
            }
        }*/

        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "ANCHOR_LOT_CHECKSTATUS") //  开启天选前的审核，审核过了才是真正开启
    {
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "ANCHOR_LOT_START") // 开启天选
    {
        /*{
            "cmd": "ANCHOR_LOT_START",
            "data": {
                "asset_icon": "https://i0.hdslb.com/bfs/live/992c2ccf88d3ea99620fb3a75e672e0abe850e9c.png",
                "award_image": "",
                "award_name": "5.2元红包",
                "award_num": 1,
                "cur_gift_num": 0,
                "current_time": 1610529938,
                "danmu": "娇娇赛高",
                "gift_id": 0,
                "gift_name": "",
                "gift_num": 1,
                "gift_price": 0,
                "goaway_time": 180,
                "goods_id": -99998,
                "id": 773667,
                "is_broadcast": 1,
                "join_type": 0,
                "lot_status": 0,
                "max_time": 600,
                "require_text": "关注主播",
                "require_type": 1, // 1是关注？
                "require_value": 0,
                "room_id": 22532956,
                "send_gift_ensure": 0,
                "show_panel": 1,
                "status": 1,
                "time": 599,
                "url": "https://live.bilibili.com/p/html/live-lottery/anchor-join.html?is_live_half_webview=1&hybrid_biz=live-lottery-anchor&hybrid_half_ui=1,5,100p,100p,000000,0,30,0,0,1;2,5,100p,100p,000000,0,30,0,0,1;3,5,100p,100p,000000,0,30,0,0,1;4,5,100p,100p,000000,0,30,0,0,1;5,5,100p,100p,000000,0,30,0,0,1;6,5,100p,100p,000000,0,30,0,0,1;7,5,100p,100p,000000,0,30,0,0,1;8,5,100p,100p,000000,0,30,0,0,1",
                "web_url": "https://live.bilibili.com/p/html/live-lottery/anchor-join.html"
            }
        }*/

        /*{
            "cmd": "ANCHOR_LOT_START",
            "data": {
                "asset_icon": "https://i0.hdslb.com/bfs/live/992c2ccf88d3ea99620fb3a75e672e0abe850e9c.png",
                "award_image": "",
                "award_name": "5.2元红包",
                "award_num": 1,
                "cur_gift_num": 0,
                "current_time": 1610535661,
                "danmu": "我就是天选之人！", // 送礼物的话也是需要发弹幕的（自动发送+自动送礼）
                "gift_id": 20008, // 冰阔落ID
                "gift_name": "冰阔落",
                "gift_num": 1,
                "gift_price": 1000,
                "goaway_time": 180,
                "goods_id": 15, // 物品ID？
                "id": 773836,
                "is_broadcast": 1,
                "join_type": 1,
                "lot_status": 0,
                "max_time": 600,
                "require_text": "无",
                "require_type": 0,
                "require_value": 0,
                "room_id": 22532956,
                "send_gift_ensure": 0,
                "show_panel": 1,
                "status": 1,
                "time": 599,
                "url": "https://live.bilibili.com/p/html/live-lottery/anchor-join.html?is_live_half_webview=1&hybrid_biz=live-lottery-anchor&hybrid_half_ui=1,5,100p,100p,000000,0,30,0,0,1;2,5,100p,100p,000000,0,30,0,0,1;3,5,100p,100p,000000,0,30,0,0,1;4,5,100p,100p,000000,0,30,0,0,1;5,5,100p,100p,000000,0,30,0,0,1;6,5,100p,100p,000000,0,30,0,0,1;7,5,100p,100p,000000,0,30,0,0,1;8,5,100p,100p,000000,0,30,0,0,1",
                "web_url": "https://live.bilibili.com/p/html/live-lottery/anchor-join.html"
            }
        }*/

        QJsonObject data = json.value("data").toObject();
        qint64 id = static_cast<qint64>(data.value("id").toDouble());
        QString danmu = data.value("danmu").toString();
        int giftId = data.value("gift_id").toInt();
        int time = data.value("time").toInt();
        qInfo() << "天选弹幕：" << danmu;
        if (!danmu.isEmpty() && giftId <= 0 && us->autoJoinLOT)
        {
            int requireType = data.value("require_type").toInt();
            joinLOT(id, requireType);
        }
        emit signalDanmakuAddBlockText(danmu, time);

        triggerCmdEvent(cmd, LiveDanmaku().with(data));
    }
    else if (cmd == "ANCHOR_LOT_END") // 天选结束
    {
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "ANCHOR_LOT_AWARD") // 天选结果推送，在结束后的不到一秒左右
    {
        /* {
            "cmd": "ANCHOR_LOT_AWARD",
            "data": {
                "award_image": "",
                "award_name": "兔の自拍",
                "award_num": 1,
                "award_users": [
                    {
                        "color": 5805790,
                        "face": "http://i1.hdslb.com/bfs/face/f07a981e34985819367eb709baefd80ad5ab4746.jpg",
                        "level": 26,
                        "uid": 44916867,
                        "uname": "-领主-"
                    }
                ],
                "id": 1014952,
                "lot_status": 2,
                "url": "https://live.bilibili.com/p/html/live-lottery/anchor-join.html?is_live_half_webview=1&hybrid_biz=live-lottery-anchor&hybrid_half_ui=1,5,100p,100p,000000,0,30,0,0,1;2,5,100p,100p,000000,0,30,0,0,1;3,5,100p,100p,000000,0,30,0,0,1;4,5,100p,100p,000000,0,30,0,0,1;5,5,100p,100p,000000,0,30,0,0,1;6,5,100p,100p,000000,0,30,0,0,1;7,5,100p,100p,000000,0,30,0,0,1;8,5,100p,100p,000000,0,30,0,0,1",
                "web_url": "https://live.bilibili.com/p/html/live-lottery/anchor-join.html"
            }
        } */

        QJsonObject data = json.value("data").toObject();
        QString awardName = data.value("award_name").toString();
        int awardNum = data.value("award_num").toInt();
        QJsonArray awardUsers = data.value("award_users").toArray();
        QStringList names;
        qint64 firstUid = 0;
        foreach (QJsonValue val, awardUsers)
        {
            QJsonObject user = val.toObject();
            QString uname = user.value("uname").toString();
            names.append(uname);
            if (!firstUid)
                firstUid = qint64(user.value("uid").toDouble());
        }

        QString awardRst = awardName + (awardNum > 1 ? "×" + snum(awardNum) : "");
        localNotify("[天选] " + names.join(",") + " 中奖：" + awardRst);

        triggerCmdEvent(cmd, LiveDanmaku(firstUid, names.join(","), awardRst));
    }
    else if (cmd == "VOICE_JOIN_ROOM_COUNT_INFO") // 等待连麦队列数量变化
    {
        /*{
            "cmd": "VOICE_JOIN_ROOM_COUNT_INFO",
            "data": {
                "apply_count": 1, // 猜测：1的话就是添加申请连麦，0是取消申请连麦
                "notify_count": 0,
                "red_point": 0,
                "room_id": 22532956,
                "room_status": 1,
                "root_status": 1
            },
            "roomid": 22532956
        }*/

        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "VOICE_JOIN_LIST") // 连麦申请、取消连麦申请；和VOICE_JOIN_ROOM_COUNT_INFO一起收到
    {
        /*{
            "cmd": "VOICE_JOIN_LIST",
            "data": {
                "apply_count": 1, // 等同于VOICE_JOIN_ROOM_COUNT_INFO的apply_count
                "category": 1,
                "red_point": 1,
                "refresh": 1,
                "room_id": 22532956
            },
            "roomid": 22532956
        }*/

        QJsonObject data = json.value("data").toObject();
        int point = data.value("red_point").toInt();
        localNotify("连麦队列：" + snum(point));

        triggerCmdEvent(cmd, LiveDanmaku().with(data));
    }
    else if (cmd == "VOICE_JOIN_STATUS") // 连麦状态，连麦开始/结束
    {
        /*{
            "cmd": "VOICE_JOIN_STATUS",
            "data": {
                "channel": "voice320168",
                "channel_type": "voice",
                "current_time": 1610802781,
                "guard": 3,
                "head_pic": "http://i1.hdslb.com/bfs/face/5bbf173c5cf4f70481e5814e34bbdf6db564ef80.jpg",
                "room_id": 22532956,
                "start_at": 1610802781,
                "status": 1,                   // 1是开始连麦
                "uid": 1324369,
                "user_name":"\xE6\xB0\xB8\xE8\xBF\x9C\xE5\x8D\x95\xE6\x8E\xA8\xE5\xA8\x87\xE5\xA8\x87\xE7\x9A\x84\xE8\x82\x89\xE5\xA4\xB9\xE9\xA6\x8D",
                "web_share_link": "https://live.bilibili.com/h5/22532956"
            },
            "roomid": 22532956
        }*/
        /*{
            "cmd": "VOICE_JOIN_STATUS",
            "data": {
                "channel": "",
                "channel_type": "voice",
                "current_time": 1610802959,
                "guard": 0,
                "head_pic": "",
                "room_id": 22532956,
                "start_at": 0,
                "status": 0,                   // 0是取消连麦
                "uid": 0,
                "user_name": "",
                "web_share_link": "https://live.bilibili.com/h5/22532956"
            },
            "roomid": 22532956
        }*/

        QJsonObject data = json.value("data").toObject();
        int status = data.value("status").toInt();
        QString uname = data.value("username").toString();
        if (status) // 开始连麦
        {
            if (!uname.isEmpty())
                localNotify((uname + " 接入连麦"));
        }
        else // 取消连麦
        {
            if (!uname.isEmpty())
                localNotify(uname + " 结束连麦");
        }

        triggerCmdEvent(cmd, LiveDanmaku().with(data));
    }
    else if (cmd == "WARNING") // 被警告
    {
        /*{
            "cmd": "WARNING",
            "msg":"违反直播分区规范，未摄像头露脸",
            "roomid": 22532956
        }*/
        QString msg = json.value("msg").toString();
        localNotify(msg);

        triggerCmdEvent(cmd, LiveDanmaku(msg).with(json));
    }
    else if (cmd == "room_admin_entrance")
    {
        /*{
            "cmd": "room_admin_entrance",
            "msg":"系统提示：你已被主播设为房管",
            "uid": 20285041
        }*/
        QString msg = json.value("msg").toString();
        qint64 uid = static_cast<qint64>(json.value("uid").toDouble());
        if (snum(uid) == ac->cookieUid)
        {
            localNotify(msg, uid);
        }
        else
        {
            localNotify(cr->uidToName(uid) + " 被任命为房管", uid);
        }
        triggerCmdEvent(cmd, LiveDanmaku(msg).with(json));
    }
    else if (cmd == "ROOM_ADMIN_REVOKE")
    {
        /*{
            "cmd": "ROOM_ADMIN_REVOKE",
            "msg":"撤销房管",
            "uid": 324495090
        }*/
        QString msg = json.value("msg").toString();
        qint64 uid = static_cast<qint64>(json.value("uid").toDouble());
        localNotify(cr->uidToName(uid) + " 被取消房管", uid);
        triggerCmdEvent(cmd, LiveDanmaku(msg).with(json));
    }
    else if (cmd == "ROOM_ADMINS")
    {
        /*{
            "cmd": "ROOM_ADMINS",
            "uids": [
                36272011, 145884036, 10823381, 67756641, 35001804, 64494330, 295742464, 250439508, 90400995, 384733451, 632481, 41090958, 691018830, 33283612, 13381878, 1324369, 49912988, 2852700, 26472148, 415374297, 20285041
            ]
        }*/

        triggerCmdEvent(cmd, LiveDanmaku().with(json));
    }
    else if (cmd == "LIVE_INTERACTIVE_GAME")
    {
        /*{
            "cmd": "LIVE_INTERACTIVE_GAME",
            "data": {
                "gift_name": "",
                "gift_num": 0,
                "msg": "哈哈哈哈哈哈哈哈哈",
                "price": 0,
                "type": 2,
                "uname": "每天都要学习混凝土"
            }
        }*/
    }
    else if (cmd == "CUT_OFF")
    {
        localNotify("直播间被超管切断");
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "STOP_LIVE_ROOM_LIST")
    {
        return ;
    }
    else if (cmd == "COMMON_NOTICE_DANMAKU") // 大乱斗一堆提示
    {
        /*{
            "cmd": "COMMON_NOTICE_DANMAKU",P
            "data": {
                "content_segments": [
                    {
                        "font_color": "#FB7299",
                        "text": "本场PK大乱斗我方获胜！达成<$3$>连胜！感谢<$平平无奇怜怜小天才$>为胜利做出的贡献",
                        "type": 1
                    }
                ],
                "dmscore": 144,
                "terminals": [ 1, 2, 3, 4, 5 ]
            }
        }*/
        MyJson data = json.value("data").toObject();
        QJsonArray array = data.a("content_segments");
        if (array.size() > 0)
        {
            QString text = array.first().toObject().value("text").toString();
            if (text.isEmpty())
            {
                qInfo() << "不含文本的 COMMON_NOTICE_DANMAKU：" << json;
            }
            else
            {
                LiveDanmaku danmaku = LiveDanmaku(text).with(json);
                qInfo() << "NOTICE:" << array;
                if (cr->isFilterRejected("FILTER_DANMAKU_NOTICE", danmaku))
                    return ;
                text.replace("<$", "").replace("$>", "");
                localNotify(text);
                triggerCmdEvent(cmd, danmaku);
            }
        }
        else
        {
            qWarning() << "未处理的 COMMON_NOTICE_DANMAKU：" << json;
        }
    }
    else if (cmd == "VIDEO_CONNECTION_MSG")
    {
        MyJson data = json.value("data").toObject();
        QString channelId = data.s("channel_id");
        qint64 currentTime = data.i("current_time"); // 10位
        int dmscore = data.i("dmscore");
        QString toast = data.s("toast"); // "主播结束了视频连线"
        localNotify(toast);

        triggerCmdEvent(cmd, data);
    }
    else if (cmd == "VIDEO_CONNECTION_JOIN_END") // 视频连线结束，和上面重复的消息（会连续收到许多次）
    {
        MyJson data = json.value("data").toObject();
        QString channelId = data.s("channel_id");
        qint64 currentTime = data.i("current_time"); // 10位
        qint64 startAt = data.i("start_at"); // 10位
        int dmscore = data.i("dmscore");
        QString toast = data.s("toast"); // 主播结束了视频连线
        qint64 roomId = json.value("roomid").toDouble(); // 10位

        triggerCmdEvent(cmd, data);
    }
    else if (cmd == "VIDEO_CONNECTION_JOIN_START") // 视频连线开始（会连续收到许多次）
    {
        MyJson data = json.value("data").toObject();
        QString channelId = data.s("channel_id");
        qint64 currentTime = data.i("current_time"); // 10位
        QString invitedFace = data.s("invitedFace"); // 连线头像URL
        qint64 invitedUid = data.i("invited_uid"); // 连续UID
        QString invitedUname = data.s("invited_uname"); // 连接名字
        qint64 startAt = data.i("start_at"); // 10位
        qint64 roomId = json.value("roomid").toDouble(); // 10位

        triggerCmdEvent(cmd, data);
    }
    else if (cmd == "WATCHED_CHANGE")
    {
        /*{
            "cmd": "WATCHED_CHANGE",
            "data": {
                "num": 83,
                "text_large": "83人看过",
                "text_small": "83"
            }
        }*/
        MyJson data = json.value("data").toObject();
        QString textLarge = data.s("text_large");
        emit signalWatchCountChanged(textLarge);

        triggerCmdEvent(cmd, data);
    }
    else if (cmd == "DANMU_AGGREGATION") // 弹幕聚合，就是天选之类的弹幕
    {
        /*{
            "cmd": "DANMU_AGGREGATION",
            "data": {
                "activity_identity": "2806688",
                "activity_source": 1,
                "aggregation_cycle": 1,
                "aggregation_icon": "https://i0.hdslb.com/bfs/live/c8fbaa863bf9099c26b491d06f9efe0c20777721.png",
                "aggregation_num": 20,
                "dmscore": 144,
                "msg": "给粗粗写情书",
                "show_rows": 1,
                "show_time": 2,
                "timestamp": 1656331099
            }
        }*/
        MyJson data = json.value("data").toObject();
        QString msg = data.s("msg");

        triggerCmdEvent(cmd, data);
    }
    else if (cmd == "LIVE_OPEN_PLATFORM_GAME") // 开启互动玩法
    {
        qInfo() << "互动玩法" << json;
        /*{
            "cmd": "LIVE_OPEN_PLATFORM_GAME",
            "data": {
                "block_uids": null,
                "game_code": "1658282676661",
                "game_conf": "",
                "game_id": "e40cf1c4-ff25-4f60-90a4-9af24ed221c3",
                "game_msg": "",
                "game_name": "神奇弹幕互动版",
                "game_status": "",
                "interactive_panel_conf": "",
                "msg_sub_type": "game_end",
                "msg_type": "game_end",
                "timestamp": 0
            }
        }*/
        MyJson data = json.value("data").toObject();
        QString gameCode = data.s("game_code"); // APP_ID，用来判断是不是本应用的
        QString gameId = data.s("game_id"); // 场次ID

        /* if (liveOpenService && gameCode == snum(liveOpenService->getAppId()))
        {
            liveOpenService->startGame(gameId);
        } */

        triggerCmdEvent(cmd, data);
    }
    else if (cmd == "LIVE_PANEL_CHANGE") // 直播面板改变，已知开启互动玩法后会触发
    {
        qInfo() << "互动玩法" << json;
        /*{
            "cmd": "LIVE_PANEL_CHANGE",
            "data": {
                "scatter": {
                    "max": 150,
                    "min": 5
                },
                "type": 2
            }
        }*/
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "PLAY_TOGETHER")
    {
        /*{
            "cmd": "PLAY_TOGETHER",
            "data": {
                "action": "switch_on",
                "apply_number": 0,
                "cur_fleet_num": 0,
                "jump_url": "",
                "max_fleet_num": 0,
                "message": "",
                "message_type": 0,
                "refresh_tool": false,
                "roomid": 24733392,
                "ruid": 1592628332,
                "timestamp": 1658747488,
                "uid": 0,
                "web_url": ""
            } */

        /*{
            "cmd": "PLAY_TOGETHER",
            "data": {
                "action": "switch_on",
                "apply_number": 0,
                "cur_fleet_num": 0,
                "jump_url": "",
                "max_fleet_num": 0,
                "message": "系统提示：主播已切换分区",
                "message_type": 3,
                "refresh_tool": true,
                "roomid": 24733392,
                "ruid": 1592628332,
                "timestamp": 1658747491,
                "uid": 0,
                "web_url": ""
            }
        }*/
        MyJson data = json.value("data").toObject();
        int type = data.i("message_type");
        QString action = data.s("action");
        if (action == "switch_on")
        {
            // TODO:切换分区
        }
        triggerCmdEvent(cmd, data);
    }
    else if (cmd == "POPULARITY_RED_POCKET_START")
    {
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "POPULARITY_RED_POCKET_WINNER_LIST") // 抽奖红包
    {
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "LIKE_INFO_V3_CLICK")
    {
        /*{
            "cmd": "LIKE_INFO_V3_CLICK",
            "data": {
                "contribution_info": {
                    "grade": 0
                },
                "dmscore": 20,
                "fans_medal": {
                    "anchor_roomid": 0,
                    "guard_level": 0,
                    "icon_id": 0,
                    "is_lighted": 0,
                    "medal_color": 12478086,
                    "medal_color_border": 12632256,
                    "medal_color_end": 12632256,
                    "medal_color_start": 12632256,
                    "medal_level": 14,
                    "medal_name": "猛男",
                    "score": 45450,
                    "special": "",
                    "target_id": 183430
                },
                "identities": [
                    1
                ],
                "like_icon": "https://i0.hdslb.com/bfs/live/23678e3d90402bea6a65251b3e728044c21b1f0f.png",
                "like_text": "为主播点赞了",
                "msg_type": 6,
                "show_area": 0,
                "uid": 308873328,
                "uname": "Xiamuの",
                "uname_color": ""
            }
        }*/
        triggerCmdEvent(cmd, LiveDanmaku().with(json.value("data").toObject()));
    }
    else if (cmd == "AREA_RANK_CHANGED")
    {
        /*{
            "cmd": "AREA_RANK_CHANGED",
            "data": {
                "action_type": 1,
                "conf_id": 18,
                "icon_url_blue": "https://i0.hdslb.com/bfs/live/18e2990a546d33368200f9058f3d9dbc4038eb5c.png",
                "icon_url_grey": "https://i0.hdslb.com/bfs/live/cb7444b1faf1d785df6265bfdc1fcfc993419b76.png",
                "icon_url_pink": "https://i0.hdslb.com/bfs/live/a6c490c36e88c7b191a04883a5ec15aed187a8f7.png",
                "jump_url_link": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=3&ruid=1424343672&conf_id=18&is_live_half_webview=1&hybrid_rotate_d=1&is_cling_player=1&hybrid_half_ui=1,3,100p,70p,f4eefa,0,30,100,0,0;2,2,375,100p,f4eefa,0,30,100,0,0;3,3,100p,70p,f4eefa,0,30,100,0,0;4,2,375,100p,f4eefa,0,30,100,0,0;5,3,100p,70p,f4eefa,0,30,100,0,0;6,3,100p,70p,f4eefa,0,30,100,0,0;7,3,100p,70p,f4eefa,0,30,100,0,0;8,3,100p,70p,f4eefa,0,30,100,0,0#/area-rank",
                "jump_url_pc": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=4&ruid=1424343672&conf_id=18&pc_ui=338,465,f4eefa,0#/area-rank",
                "jump_url_pink": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=1&ruid=1424343672&conf_id=18&is_live_half_webview=1&hybrid_rotate_d=1&hybrid_half_ui=1,3,100p,70p,ffffff,0,30,100,12,0;2,2,375,100p,ffffff,0,30,100,0,0;3,3,100p,70p,ffffff,0,30,100,12,0;4,2,375,100p,ffffff,0,30,100,0,0;5,3,100p,70p,ffffff,0,30,100,0,0;6,3,100p,70p,ffffff,0,30,100,0,0;7,3,100p,70p,ffffff,0,30,100,0,0;8,3,100p,70p,ffffff,0,30,100,0,0#/area-rank",
                "jump_url_web": "https://live.bilibili.com/p/html/live-app-hotrank/index.html?clientType=2&ruid=1424343672&conf_id=18#/area-rank",
                "msg_id": "d09b58ac-278f-4bf6-bd1e-481b548fc336",
                "rank": 36,
                "rank_name": "聊天热榜",
                "timestamp": 1675690660,
                "uid": 1424343672
            }
        }*/
        MyJson data = json.value("data").toObject();
        int rank = data.i("rank");
        QString name = data.s("rank_name");
    }
    else if (cmd == "WIDGET_GIFT_STAR_PROCESS")
    {
        /*{
            "cmd": "WIDGET_GIFT_STAR_PROCESS",
            "data": {
                "ddl_timestamp": 1678636800,
                "finished": false,
                "process_list": [
                    {
                        "completed_num": 4,
                        "gift_id": 31036,
                        "gift_img": "https://s1.hdslb.com/bfs/live/8b40d0470890e7d573995383af8a8ae074d485d9.png",
                        "gift_name": "礼物星球",
                        "target_num": 200
                    },
                    {
                        "completed_num": 1,
                        "gift_id": 31037,
                        "gift_img": "https://s1.hdslb.com/bfs/live/461be640f60788c1d159ec8d6c5d5cf1ef3d1830.png",
                        "gift_name": "礼物星球",
                        "target_num": 100
                    },
                    {
                        "completed_num": 0,
                        "gift_id": 31883,
                        "gift_img": "https://s1.hdslb.com/bfs/live/d819113c3c3edde56d0eb872d3723ac2025f19bc.png",
                        "gift_name": "礼物星球",
                        "target_num": 1
                    }
                ],
                "reward_gift": 32268,
                "reward_gift_img": "https://s1.hdslb.com/bfs/live/7ca35670d343096c4bd9cd6d5491aa8a5305f82c.png",
                "reward_gift_name": "礼物星球",
                "start_date": 20230306,
                "version": 1678105731045
            }
        }*/
    }
    else if (cmd == "POPULAR_RANK_CHANGED")
    {
        // {"cmd":"POPULAR_RANK_CHANGED","data":{"cache_key":"rank_change:0d70f402c7fd4794952cdebd4f404e7f","countdown":2722,"rank":45,"timestamp":1686467679,"uid":27314812}}
        MyJson data = json.value("data").toObject();
        int rank = data.i("rank");
        int countdown = data.i("countdown");
        // localNotify("人气榜：" + snum(rank) + "/" + snum(countdown));
    }
    else if (cmd == "LIKE_INFO_V3_UPDATE") // 点赞数量
    {
        /*{
            "cmd": "LIKE_INFO_V3_UPDATE",
            "data": {
                "click_count": 159
            },
            "is_report": false,
            "msg_id": "1413957529653760",
            "send_time": 1691250789961
        }*/
        MyJson data = json.value("data").toObject();
        int count = data.i("click_count");
        emit signalLikeChanged(count);
    }
    else if (cmd == "USER_TASK_PROGRESS_V2")
    {

    }
    else
    {
        qWarning() << "未处理的命令：" << cmd << json;
        triggerCmdEvent(cmd, LiveDanmaku().with(json));
    }
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
        danmaku.setToView(toView);
        danmaku.setPkLink(true);
        if (info.at(0).toArray().size() >= 16)
        {
            MyJson detail = info.at(0).toArray().at(15).toObject();
            MyJson user = detail.o("user");
            MyJson base = user.o("base");
            danmaku.setFaceUrl(base.s("face"));
        }
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
