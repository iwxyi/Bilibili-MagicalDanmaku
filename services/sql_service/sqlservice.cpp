#include <QDebug>
#include "sqlservice.h"
#include "accountinfo.h"

SqlService::SqlService(QObject *parent) : QObject(parent)
{

}

SqlService::~SqlService()
{
    close();
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
        createTable("CREATE TABLE danmu(\
id INTEGER PRIMARY KEY AUTOINCREMENT,\
room_id TEXT NOT NULL,\
uname TEXT NOT NULL,\
uid TEXT NOT NULL,\
msg TEXT NOT NULL,\
ulevel INTEGER,\
admin BOOLEAN,\
guard INTEGER,\
anchor_room_id TEXT,\
medal_name TEXT,\
medal_level INTEGER,\
medal_up TEXT,\
price INTEGER,\
create_time time NOT NULL)");
    }
    else
    {
        tryExec("alter table danmu add guard INTERGER");
    }

    // 送礼（不包括舰长）
    if (!hasTable("gift"))
    {
        createTable("CREATE TABLE gift(\
id INTEGER PRIMARY KEY AUTOINCREMENT,\
room_id TEXT NOT NULL,\
uname TEXT NOT NULL,\
uid TEXT NOT NULL,\
gift_name TEXT NOT NULL,\
gift_id INTEGER NOT NULL,\
gift_type INTEGER,\
coin_type TEXT,\
total_coin INTEGER NOT NULL,\
number INTEGER NOT NULL,\
ulevel INTEGER,\
admin BOOLEAN,\
guard INTEGER,\
anchor_room_id TEXT,\
medal_name TEXT,\
medal_level INTEGER,\
medal_up TEXT,\
create_time time NOT NULL)");
    }
    else
    {
        tryExec("alter table gift add guard INTERGER");
    }

    // 舰长
    if (!hasTable("guard"))
    {
        createTable("CREATE TABLE guard(\
id INTEGER PRIMARY KEY AUTOINCREMENT,\
room_id TEXT NOT NULL,\
uname TEXT NOT NULL,\
uid TEXT NOT NULL,\
gift_name TEXT NOT NULL,\
gift_id INTEGER NOT NULL,\
guard_level INTEGER NOT NULL,\
price INTEGER,\
number INTEGER,\
start_time TIMESTAMP,\
end_time TIMESTAMP,\
create_time time NOT NULL)");
    }

    // 进入/关注
    if (!hasTable("interact"))
    {
        createTable("CREATE TABLE interact(\
id INTEGER PRIMARY KEY AUTOINCREMENT,\
room_id TEXT NOT NULL,\
uname TEXT NOT NULL,\
uid TEXT NOT NULL,\
msg_type INTEGER NOT NULL,\
admin BOOLEAN,\
guard INTEGER,\
anchor_room_id TEXT,\
medal_name TEXT,\
medal_level INTEGER,\
medal_up TEXT,\
special INTEGER,\
spread_desc TEXT,\
spread_info TEXT,\
create_time time NOT NULL)");
    }
    else
    {
        tryExec("alter table interact add guard INTERGER");
    }

    // 勋章
    if (!hasTable("medal"))
    {
        createTable("CREATE TABLE medal(\
id INTEGER PRIMARY KEY AUTOINCREMENT,\
anchor_room_id TEXT,\
medal_name TEXT,\
medal_level INTEGER,\
medal_up TEXT,\
create_time time NOT NULL)");
    }

    // 点歌
    if (!hasTable("music"))
    {
        createTable("CREATE TABLE music(\
id INTEGER PRIMARY KEY AUTOINCREMENT,\
room_id TEXT NOT NULL,\
uname TEXT NOT NULL,\
uid TEXT NOT NULL,\
music_name TEXT NOT NULL,\
ulevel INTEGER,\
admin BOOLEAN,\
guard INTEGER,\
anchor_room_id TEXT,\
medal_name TEXT,\
medal_level INTEGER,\
medal_up TEXT,\
create_time time NOT NULL)");
    }
    else
    {
        tryExec("alter table music add guard INTERGER");
    }

    // cmd
    if (!hasTable("cmd"))
    {
        createTable("CREATE TABLE cmd(\
id INTEGER PRIMARY KEY AUTOINCREMENT,\
cmd TEXT,\
data TEXT,\
create_time time NOT NULL)");
    }

    // AI粉丝档案
    if (!hasTable("fans_archive"))
    {
        createTable("CREATE TABLE fans_archive(\
id INTEGER PRIMARY KEY AUTOINCREMENT,\
uid TEXT NOT NULL,\
uname TEXT NOT NULL,\
archive TEXT,\
create_time time NOT NULL,\
update_time time NOT NULL)");
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
    QSqlQuery query;

    // 判断是否存在
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
 * @return 需要处理的uid
 * 目标要求：
 * - 7天内发过弹幕
 * - 总弹幕数量超过30条
 * - 没有处理过（即没有档案，优先处理）；或处理过但超过1小时且弹幕超过10条（archives.update_time - danmu.create_time > 1小时）
 */
QString SqlService::getUnprocessedFansArchive()
{
    // 获取没有处理过的粉丝档案
    // 要求：
    // - 档案表中没有对应的id
    // - 7天内发过弹幕
    // - 总弹幕数量超过30条
    static QString getUnprocessedFansArchiveSql = "SELECT uid FROM danmu ";

    // 获取需要处理的粉丝档案
    // 要求：
    // - 处理过且超过1小时
    // - 未处理的总弹幕数量超过10条
    static QString getNeededFansArchiveSql = "SELECT room_id, uid FROM fans_archive WHERE processed = 1 AND update_time - create_time > 3600 AND danmu_count > 10";

    
}
