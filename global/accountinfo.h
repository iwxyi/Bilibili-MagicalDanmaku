#ifndef ACCOUNT_INFO_H
#define ACCOUNT_INFO_H

#include <QHash>
#include <QVariant>

class AccountInfo
{
public:
    // 登录信息
    QString browserCookie;
    QString browserData;
    QString csrf_token;
    QVariant userCookies; // 网络连接使用的cookie

    // 当前账号
    QString cookieUid;        // 自己的UID
    QString cookieUname;      // 自己的昵称
    QString cookieToken;      // csrf_token
    int cookieULevel = 0;     // 自己的等级
    int cookieGuardLevel = 0; // 自己的大航海
    int danmuLongest = 20;    // 最长的弹幕
    QString buvid;

    // 连接的直播间信息
    QString roomId;          // 房间长ID
    QString shortId;         // 房间短号（有些没有，也没什么用）
    int liveStatus = 0;      // 是否正在直播
    QString roomTitle;       // 房间标题
    QString areaId;          // 例：21（整型，为了方便用字符串）
    QString areaName;        // 例：视频唱见
    QString parentAreaId;    // 例：1（整型，为了方便用字符串）
    QString parentAreaName;  // 例：娱乐
    QString roomNews;        // 主播公告
    QString roomDescription; // 主播个人简介
    QStringList roomTags;    // 主播个人标签
    QString areaRank;        // 分区排行（字符串，比如 >100）
    QString liveRank;        // 总排行（字符串），也是主播排行
    int roomRank = 0;
    QString rankArea;        // 是哪个榜的
    int countdown = 0;       // 当前人数
    QString watchedShow;     // 总的看过？
    int winningStreak = 0;   // 连胜
    int currentPopul = 0;    // 人气值
    qint64 lastMatchRoomId;  // 最后匹配的直播间
    qint64 liveStartTime = 0; // 开播时间（10位时间戳）

    // 主播信息
    QString upUid;                 // 主播的UID
    QString upName;                // 主播昵称
    QString identityCode;          // 身份码，获取一次应用后才有
    int anchorLiveLevel = 0;       // 主播等级
    qint64 anchorLiveScore = 0;    // 主播积分（金瓜子）
    qint64 anchorUpgradeScore = 0; // 升级剩余积分
    QString battleRankName;        // 大乱斗段位
    int currentFans = 0;           // 粉丝数量
    int currentFansClub = 0;       // 粉丝团数量
    QHash<qint64, QString> currentGuards; // 当前船员ID-Name
};

extern AccountInfo *ac;

#endif // ACCOUNT_INFO_H
