#include "douyin_liveservice.h"
#include "fileutil.h"

DouyinLiveService::DouyinLiveService(QObject *parent) : LiveServiceBase(parent)
{
    initWS();
}

void DouyinLiveService::initWS()
{
    liveSocket = new QWebSocket();
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
        ac->roomId = roomInfo.s("roomId");
        MyJson room = roomInfo.o("room");
        ac->roomTitle = room.s("title");
        qInfo() << "直播间信息：" << ac->roomTitle << ac->roomId;
        
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
        qInfo() << "用户信息：用户唯一ID" << userUniqueId;

        emit signalRoomIdChanged(ac->roomId);
        emit signalUpUidChanged(ac->upUid);
        emit signalRoomInfoChanged();
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
