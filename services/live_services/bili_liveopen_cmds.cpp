#include "bili_liveopenservice.h"

bool BiliLiveOpenService::handleUncompMessage(QString cmd, MyJson json)
{
    if (cmd == "LIVE_OPEN_PLATFORM_DM")
    {
        /*{
            "cmd": "LIVE_OPEN_PLATFORM_DM",
            "data": {
                "dm_type": 0,
                "emoji_img_url": "",
                "fans_medal_level": 21,
                "fans_medal_name": "懒一夕",
                "fans_medal_wearing_status": false,
                "guard_level": 0,
                "msg": "哈哈哈",
                "msg_id": "121e44ca-db53-4350-a8a8-bbab04474255",
                "room_id": 11584296,
                "timestamp": 1694279601,
                "uface": "https://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg",
                "uid": 20285041,
                "uname": "懒一夕智能科技官方"
            }
        }*/

        MyJson data = json.data();
        LiveDanmaku danmaku(data.s("uname"),
                            data.s("msg"),
                            data.l("uid"),
                            0,
                            QDateTime::currentDateTime(),
                            "",
                            ""
                    );
        danmaku.setMedal("", data.s("fans_medal_name"), data.i("fans_medal_level"), "");
        danmaku.setGuardLevel(data.i("guard_level"));
        danmaku.with(data);
        receiveDanmaku(danmaku);
    }
    else
    {
        qWarning() << "未处理的未压缩命令=" << cmd << "   正文=" << json;
        return false;
    }

    triggerCmdEvent(cmd, LiveDanmaku(json.value("data").toObject()));

    return true;
}
