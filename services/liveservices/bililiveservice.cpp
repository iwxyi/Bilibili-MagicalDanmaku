#include "bililiveservice.h"

BiliLiveService::BiliLiveService(QObject *parent) : LiveRoomService(parent)
{
}

void BiliLiveService::startConnectRoom(const QString &roomId)
{
}

void BiliLiveService::getRoomInfo()
{
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
