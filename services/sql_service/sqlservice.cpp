#include <QDebug>
#include "sqlservice.h"
#include "accountinfo.h"
#include "myjson.h"

SqlService::SqlService(QObject *parent) : QObject(parent)
{
}

SqlService::~SqlService()
{
    close();
}

bool SqlService::isOpen() const
{
    return db.isOpen();
}

void SqlService::setDbPath(const QString &path)
{
    this->dbPath = path;
}

QString SqlService::getDbPath() const
{
    return dbPath;
}

QSqlDatabase SqlService::getDb() const
{
    return db;
}

QSqlQuery SqlService::getQuery(const QString &sql) const
{
    QSqlQuery query;
    query.exec(sql);
    return query;
}

void SqlService::open()
{
    if (db.isOpen())
        return ;

    if (QSqlDatabase::contains("qt_sql_default_connection"))
    {
        db = QSqlDatabase::database("qt_sql_default_connection");
    }
    else
    {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(dbPath);
    }

    if (!db.open())
    {
        qWarning() << "无法打开Sqlite数据库：" << db.lastError();
    }

    qInfo() << "连接 SQLite 数据库：" << db.databaseName();
    initTables();
}

void SqlService::close()
{
    if (!db.isOpen())
        return ;

    db.close();
}

void SqlService::initTables()
{
    if (!db.isOpen())
    {
        qCritical() << "未打开数据库";
        return ;
    }

    // #判断数据库存不存在

    // 弹幕
    if (!hasTable("danmu"))
    {
        createTable(R"(CREATE TABLE danmu(
id INTEGER PRIMARY KEY AUTOINCREMENT,
room_id TEXT NOT NULL,
uname TEXT NOT NULL,
uid TEXT NOT NULL,
msg TEXT NOT NULL,
ulevel INTEGER,
admin BOOLEAN,
guard INTEGER,
anchor_room_id TEXT,
medal_name TEXT,
medal_level INTEGER,
medal_up TEXT,
price INTEGER,
create_time time NOT NULL))");
    }
    else
    {
        tryExec("alter table danmu add guard INTERGER");
    }

    // 送礼（不包括舰长）
    if (!hasTable("gift"))
    {
        createTable(R"(CREATE TABLE gift(
id INTEGER PRIMARY KEY AUTOINCREMENT,
room_id TEXT NOT NULL,
uname TEXT NOT NULL,
uid TEXT NOT NULL,
gift_name TEXT NOT NULL,
gift_id INTEGER NOT NULL,
gift_type INTEGER,
coin_type TEXT,
total_coin INTEGER NOT NULL,
number INTEGER NOT NULL,
ulevel INTEGER,
admin BOOLEAN,
guard INTEGER,
anchor_room_id TEXT,
medal_name TEXT,
medal_level INTEGER,
medal_up TEXT,
create_time time NOT NULL))");
    }
    else
    {
        tryExec("alter table gift add guard INTERGER");
    }

    // 舰长
    if (!hasTable("guard"))
    {
        createTable(R"(CREATE TABLE guard(
id INTEGER PRIMARY KEY AUTOINCREMENT,
room_id TEXT NOT NULL,
uname TEXT NOT NULL,
uid TEXT NOT NULL,
gift_name TEXT NOT NULL,
gift_id INTEGER NOT NULL,
guard_level INTEGER NOT NULL,
price INTEGER,
number INTEGER,
start_time TIMESTAMP,
end_time TIMESTAMP,
create_time time NOT NULL))");
    }

    // 进入/关注
    if (!hasTable("interact"))
    {
        createTable(R"(CREATE TABLE interact(
id INTEGER PRIMARY KEY AUTOINCREMENT,
room_id TEXT NOT NULL,
uname TEXT NOT NULL,
uid TEXT NOT NULL,
msg_type INTEGER NOT NULL,
admin BOOLEAN,
guard INTEGER,
anchor_room_id TEXT,
medal_name TEXT,
medal_level INTEGER,
medal_up TEXT,
special INTEGER,
spread_desc TEXT,
spread_info TEXT,
create_time time NOT NULL))");
    }
    else
    {
        tryExec("alter table interact add guard INTERGER");
    }

    // 勋章
    if (!hasTable("medal"))
    {
        createTable(R"(CREATE TABLE medal(
id INTEGER PRIMARY KEY AUTOINCREMENT,
anchor_room_id TEXT,
medal_name TEXT,
medal_level INTEGER,
medal_up TEXT,
create_time time NOT NULL))");
    }

    // 点歌
    if (!hasTable("music"))
    {
        createTable(R"(CREATE TABLE music(
id INTEGER PRIMARY KEY AUTOINCREMENT,
room_id TEXT NOT NULL,
uname TEXT NOT NULL,
uid TEXT NOT NULL,
music_name TEXT NOT NULL,
ulevel INTEGER,
admin BOOLEAN,
guard INTEGER,
anchor_room_id TEXT,
medal_name TEXT,
medal_level INTEGER,
medal_up TEXT,
create_time time NOT NULL))");
    }
    else
    {
        tryExec("alter table music add guard INTERGER");
    }

    // cmd
    if (!hasTable("cmd"))
    {
        createTable(R"(CREATE TABLE cmd(
id INTEGER PRIMARY KEY AUTOINCREMENT,
cmd TEXT,
data TEXT,
create_time time NOT NULL))");
    }

    // AI粉丝档案
    if (!hasTable("fans_archive"))
    {
        createTable(R"(CREATE TABLE fans_archive(
id INTEGER PRIMARY KEY AUTOINCREMENT,
uid TEXT NOT NULL,
uname TEXT NOT NULL,
archive TEXT,
create_time time NOT NULL,
update_time time NOT NULL))");
    }
}

/**
 * 随着版本更新，数据库也要相应调整
 * @param version 升级后的版本（不包括前面的v），如4.8.0
 */
void SqlService::upgradeDb(const QString &newVersion)
{

}

void SqlService::insertDanmaku(const LiveDanmaku &danmaku)
{
    if (!db.isOpen())
    {
        qCritical() << "未打开数据库";
        return ;
    }
    // qInfo() << ">>插入数据库：" << danmaku.getMsgType() << danmaku.toString();

    QSqlQuery query;
    switch (danmaku.getMsgType())
    {
    case MessageType::MSG_DANMAKU:
    case MessageType::MSG_SUPER_CHAT:
    {
        query.prepare("INSERT INTO \
danmu(room_id, uname, uid, msg, ulevel, admin, guard, anchor_room_id, medal_name, medal_level, medal_up, price, create_time) \
VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)");
        query.addBindValue(danmaku.getRoomId().isEmpty() ? ac->roomId : danmaku.getRoomId());
        query.addBindValue(danmaku.getNickname());
        query.addBindValue(danmaku.getUid());
        query.addBindValue(danmaku.getText());
        query.addBindValue(danmaku.getLevel());
        query.addBindValue(danmaku.isAdmin());
        query.addBindValue(danmaku.getGuard());
        query.addBindValue(danmaku.getAnchorRoomid());
        query.addBindValue(danmaku.getMedalName());
        query.addBindValue(danmaku.getMedalLevel());
        query.addBindValue(danmaku.getMedalUp());
        query.addBindValue(danmaku.getTotalCoin());
        query.addBindValue(danmaku.getTimeline());
    }
        break;
    case MessageType::MSG_GIFT:
    {
        query.prepare("INSERT INTO \
gift(room_id, uname, uid, gift_name, gift_id, coin_type, total_coin, number, ulevel, admin, anchor_room_id, medal_name, medal_level, medal_up, create_time) \
VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        query.addBindValue(danmaku.getRoomId().isEmpty() ? ac->roomId : danmaku.getRoomId());
        query.addBindValue(danmaku.getNickname());
        query.addBindValue(danmaku.getUid());
        query.addBindValue(danmaku.getGiftName());
        query.addBindValue(danmaku.getGiftId());
        query.addBindValue(danmaku.getCoinType());
        query.addBindValue(danmaku.getTotalCoin());
        query.addBindValue(danmaku.getNumber());
        query.addBindValue(danmaku.getLevel());
        query.addBindValue(danmaku.isAdmin());
        query.addBindValue(danmaku.getAnchorRoomid());
        query.addBindValue(danmaku.getMedalName());
        query.addBindValue(danmaku.getMedalLevel());
        query.addBindValue(danmaku.getMedalUp());
        query.addBindValue(danmaku.getTimeline());
    }
        break;
    case MessageType::MSG_GUARD_BUY:
    {
        query.prepare("INSERT INTO \
guard(room_id, uname, uid, gift_name, gift_id, guard_level, price, number, start_time, end_time, create_time) \
VALUES(?,?,?,?,?,?,?,?,?,?,?)");
        query.addBindValue(danmaku.getRoomId().isEmpty() ? ac->roomId : danmaku.getRoomId());
        query.addBindValue(danmaku.getNickname());
        query.addBindValue(danmaku.getUid());
        query.addBindValue(danmaku.getGiftName());
        query.addBindValue(danmaku.getGiftId());
        query.addBindValue(danmaku.getGuard());
        query.addBindValue(danmaku.getTotalCoin());
        query.addBindValue(danmaku.getNumber());
        // start_time 和 end_time 是同一个时间
        query.addBindValue(QDateTime::fromSecsSinceEpoch(static_cast<qint64>(danmaku.extraJson.value("start_time").toDouble())));
        query.addBindValue(QDateTime::fromSecsSinceEpoch(static_cast<qint64>(danmaku.extraJson.value("end_time").toDouble())));
        query.addBindValue(danmaku.getTimeline());
    }
        break;
    case MessageType::MSG_WELCOME:
    case MessageType::MSG_ATTENTION:
    case MessageType::MSG_SHARE:
    {
        query.prepare("INSERT INTO \
interact(room_id, uname, uid, msg_type, admin, guard, anchor_room_id, medal_name, medal_level, medal_up, special, spread_desc, spread_info, create_time) \
VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        query.addBindValue(danmaku.getRoomId().isEmpty() ? ac->roomId : danmaku.getRoomId());
        query.addBindValue(danmaku.getNickname());
        query.addBindValue(danmaku.getUid());
        query.addBindValue(danmaku.extraJson.value("msg_type").toInt());
        query.addBindValue(danmaku.isAdmin());
        query.addBindValue(danmaku.getGuard());
        query.addBindValue(danmaku.getAnchorRoomid());
        query.addBindValue(danmaku.getMedalName());
        query.addBindValue(danmaku.getMedalLevel());
        query.addBindValue(danmaku.getMedalUp());
        query.addBindValue(danmaku.getSpecial());
        query.addBindValue(danmaku.getSpreadDesc());
        query.addBindValue(danmaku.getSpreadInfo());
        query.addBindValue(danmaku.getTimeline());
    }
        break;
    default:
        return ;
    }
    if (!query.exec())
    {
        qWarning() << "执行SQL语句失败：" << query.lastError() << query.executedQuery();
    }
}

void SqlService::insertMusic(const LiveDanmaku &danmaku)
{
    QSqlQuery query;
    query.prepare("INSERT INTO music\
(room_id, uname, uid, music_name, ulevel, admin, guard, anchor_room_id, medal_name, medal_level, medal_up, create_time)\
VALUES(?,?,?,?,?,?,?,?,?,?,?,?)");
    query.addBindValue(danmaku.getRoomId().isEmpty() ? ac->roomId : danmaku.getRoomId());
    query.addBindValue(danmaku.getNickname());
    query.addBindValue(danmaku.getUid());
    query.addBindValue(danmaku.getText());
    query.addBindValue(danmaku.getLevel());
    query.addBindValue(danmaku.isAdmin());
    query.addBindValue(danmaku.getGuard());
    query.addBindValue(danmaku.getAnchorRoomid());
    query.addBindValue(danmaku.getMedalName());
    query.addBindValue(danmaku.getMedalLevel());
    query.addBindValue(danmaku.getMedalUp());
    query.addBindValue(danmaku.getTimeline());

    if (!query.exec())
    {
        qWarning() << "执行SQL语句失败：" << query.lastError() << query.executedQuery();
    }
}

void SqlService::insertCmd(const QString &cmd, const QString &data)
{
    QSqlQuery query;
    query.prepare("INSERT INTO cmd\
(cmd, data, create_time) VALUES(?,?,?)");
    query.addBindValue(cmd);
    query.addBindValue(data);
    query.addBindValue(QDateTime::currentDateTime());

    if (!query.exec())
    {
        qWarning() << "执行SQL语句失败：" << query.lastError() << query.executedQuery();
    }
}

void SqlService::insertFansArchive(const QString &uid, const QString &uname, const QString &archive)
{
    // 判断是否存在
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM fans_archive WHERE uid = ?");
    query.addBindValue(uid);
    if (!query.exec())
    {
        qWarning() << "执行SQL语句失败：" << query.lastError() << query.executedQuery();
    }
    if (query.next())
    {
        if (query.value(0).toInt() > 0)
        {
            // 更新
            query.prepare("UPDATE fans_archive SET archive = ?, update_time = ? WHERE uid = ?");
            query.addBindValue(archive);
            query.addBindValue(QDateTime::currentDateTime());
            query.addBindValue(uid);
            if (!query.exec())
            {
                qWarning() << "执行SQL语句失败：" << query.lastError() << query.executedQuery();
            }
            return ;
        }
        else
        {
            qWarning() << "粉丝档案不存在：" << uid << "，进行插入";
        }
    }
    
    // 插入
    query.prepare("INSERT INTO fans_archive\
                    (uid, uname, archive, create_time, update_time) VALUES(?,?,?,?,?)");
    query.addBindValue(uid);
    query.addBindValue(uname);
    query.addBindValue(archive);
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(QDateTime::currentDateTime());

    if (!query.exec())
    {
        qWarning() << "执行SQL语句失败：" << query.lastError() << query.executedQuery();
    }
}

bool SqlService::exec(const QString &sql)
{
    if (!db.isOpen())
    {
        emit signalError("SQL.EXEC失败：未打开数据库");
        return false;
    }

    qInfo() << "SQL.exec" << sql;

    QSqlQuery query;
    query.prepare(sql);
    if (!query.exec())
    {
        emit signalError("执行失败：" + query.lastError().text());
        return false;
    }
    return true;
}

/**
 * 不报错的执行SQL
 * 可能是补充字段、修改字段等等，大概率会运行失败
 */
bool SqlService::tryExec(const QString &sql)
{
    if (!db.isOpen())
    {
        return false;
    }

    QSqlQuery query;
    return query.exec(sql);
}

bool SqlService::hasTable(const QString &name) const
{
    if (!db.isOpen())
    {
        qCritical() << "未打开数据库";
        return false;
    }

    QSqlQuery query;
    query.exec(QString("select count(*) from sqlite_master where type='table' and name='%1'").arg(name));
    if(query.next())
    {
        return (query.value(0).toInt() != 0);
    }
    return false;
}

bool SqlService::createTable(const QString &sql)
{
    if (!db.isOpen())
    {
        qCritical() << "未打开数据库";
        return false;
    }

    QSqlQuery query;
    query.prepare(sql);
    if (!query.exec())
    {
        qWarning() << "创建Table失败：" << query.lastError() << sql;
        return false;
    }
    else
    {
        qInfo() << "创建Table：" << (sql.left(sql.indexOf("(")));
        return true;
    }
}

/**
 * 获取一个未处理过的粉丝档案目标
 * @return 需要处理的uid。如果没有，返回空字符串
 * 目标要求：
 * - 7天内发过弹幕
 * - 总弹幕数量超过30条
 * - 没有处理过（即没有档案，优先处理）；或处理过但超过1小时且弹幕超过10条（archives.update_time - danmu.create_time > 1小时）
 */
QString SqlService::getNextFansArchive()
{
    // 获取没有处理过的粉丝档案
    // 要求：
    // - 档案表中没有对应的uid，且uid不为0
    // - 7天内发过弹幕
    // - 总弹幕数量超过30条
    static QString getUnprocessedFansArchiveSql = R"(
SELECT 
    d.uid,
    MAX(d.uname) AS uname,
    COUNT(*) AS danmu_count,
    MAX(d.create_time) AS last_danmu_time
FROM danmu d
WHERE 
    d.uid NOT IN (SELECT uid FROM fans_archive)
    AND d.uid != '0'
GROUP BY d.uid
HAVING 
    danmu_count > 30 
    AND last_danmu_time >= datetime('now', '-7 days')
ORDER BY last_danmu_time DESC
LIMIT 1;
)";

    // 获取需要处理的粉丝档案
    // 要求：
    // - 处理过且1小时内没有弹幕
    // - 未处理的总弹幕数量超过10条
    static QString getNeedUpdateFansArchiveSql = R"(
WITH update_candidates AS (
  SELECT 
    d.uid,
    MAX(d.create_time) AS last_danmu_time,
    MAX(fa.update_time) AS last_update_time,
    COUNT(*) AS new_danmu_count
  FROM danmu d
  JOIN fans_archive fa ON d.uid = fa.uid
  WHERE d.create_time > fa.update_time
  GROUP BY d.uid
  HAVING 
    new_danmu_count >= 10
    AND datetime(last_danmu_time) <= datetime('now', '-1 hour', 'localtime')
)
SELECT 
  uc.uid,
  fa.uname,
  fa.archive AS current_archive,
  uc.last_update_time,
  uc.last_danmu_time,
  uc.new_danmu_count
FROM update_candidates uc
JOIN fans_archive fa ON uc.uid = fa.uid
ORDER BY uc.last_danmu_time DESC;
)";

    // #开始执行
    // 优先处理没有处理过的粉丝档案
    QSqlQuery query;
    query.exec(getUnprocessedFansArchiveSql);
    if (query.next())
    {
        qInfo() << "找到没有处理过的粉丝档案：" << query.value(0).toString();
        return query.value(0).toString();
    }

    // 然后处理已经处理过的
    query.exec(getNeedUpdateFansArchiveSql);
    if (query.next())
    {
        qInfo() << "更新已经处理过的粉丝档案：" << query.value(0).toString();
        return query.value(0).toString();
    }
    return "";
}

/**
 * 获取指定用户的粉丝档案信息
 * @param uid 用户ID
 * @return 包含档案信息的QJsonObject。如果未找到返回空对象
 */
MyJson SqlService::getFansArchives(const QString &uid)
{
    if (!db.isOpen())
    {
        qWarning() << "未打开数据库";
        return MyJson();
    }

    QSqlQuery query;
    query.prepare("SELECT uid, uname, archive, create_time, update_time FROM fans_archive WHERE uid = ?");
    query.addBindValue(uid);
    
    if (!query.exec())
    {
        qWarning() << "执行SQL语句失败：" << query.lastError() << query.executedQuery();
        return MyJson();
    }

    if (query.next())
    {
        MyJson result;
        result.insert("uid", query.value("uid").toString());
        result.insert("uname", query.value("uname").toString());
        result.insert("archive", query.value("archive").toString());
        result.insert("create_time", query.value("create_time").toDateTime().toString(Qt::ISODate));
        result.insert("update_time", query.value("update_time").toDateTime().toString(Qt::ISODate));
        return result;
    }

    return MyJson();
}

/**
 * 获取指定用户的最近弹幕记录
 * @param uid 用户ID
 * @param startTime 开始时间，一直到最新一条
 * @return 包含弹幕信息的QList<QJsonObject>。如果未找到返回空列表
 */
QList<MyJson> SqlService::getUserDanmakuList(const QString &uid, qint64 startTime, int maxCount)
{
    QList<MyJson> result;
    QSqlQuery query;
    query.prepare("SELECT uname, uid, msg, create_time FROM danmu WHERE uid = ? AND create_time >= ? ORDER BY create_time DESC LIMIT ?");
    query.addBindValue(uid);
    query.addBindValue(startTime);
    query.addBindValue(maxCount);

    if (!query.exec())
    {
        qWarning() << "执行SQL语句失败：" << query.lastError() << query.executedQuery();
        return result;
    }
    
    while (query.next())
    {
        MyJson item;
        item.insert("uname", query.value("uname").toString());
        item.insert("uid", query.value("uid").toString());
        item.insert("msg", query.value("msg").toString());
        item.insert("create_time", query.value("create_time").toDateTime().toString(Qt::ISODate));
        result.append(item);
    }
    return result;
}
