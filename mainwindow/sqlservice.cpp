#include <QDebug>
#include "sqlservice.h"

SqlService::SqlService(QObject *parent) : QObject(parent)
{

}

SqlService::~SqlService()
{
    close();
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
        db.setDatabaseName("danmus.db");
        // db.setUserName("danmu");
        // db.setPassword("danmu");
    }

    if (!db.open())
    {
        qWarning() << "无法打开Sqlite数据库：" << db.lastError();
    }

    qInfo() << "连接Sqlite数据库：" << db.databaseName();
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

    // 判断数据库存不存在
    // 判断表格存不存在
    auto hasTable = [=](const QString& name) ->bool {
        QSqlQuery query;
        query.exec(QString("select count(*) from sqlite_master where type='table' and name='%1'").arg(name));
        if(query.next())
        {
            return (query.value(0).toInt() != 0);
        }
        return false;
    };

    // 创建表格
    auto createTable = [=](const QString& sql) {
        QSqlQuery query;
        query.prepare(sql);
        if (!query.exec())
        {
            qWarning() << "创建Table失败：" << sql;
        }
        else
        {
            qInfo() << "创建Table：" << (sql.left(sql.indexOf("(")));
        }
    };

    // 弹幕
    if (!hasTable("danmu"))
    {
        createTable("CREATE TABLE danmu(\
id INT PRIMARY KEY NOT NULL,\
uname TEXT NOT NULL,\
uid TEXT NOT NULL,\
msg TEXT NOT NULL,\
ulevel INTEGER,\
admin BOOLEAN,\
uguard INTEGER,\
uname_color TEXT,\
text_color TEXT,\
anchor_room_id TEXT,\
medal_name TEXT,\
medal_level INTEGER,\
medal_up TEXT,\
create_time time NOT NULL)");
    }

    // 送礼（不包括舰长）
    if (!hasTable("gift"))
    {
        createTable("CREATE TABLE gift(\
id INT PRIMARY KEY NOT NULL,\
uname TEXT NOT NULL,\
uid TEXT NOT NULL,\
gift_name TEXT NOT NULL,\
gift_id INTEGER NOT NULL,\
gift_type INTEGER,\
coin_type TEXT,\
total_coin INTEGER,\
number INTEGER,\
ulevel INTEGER,\
admin BOOLEAN,\
anchor_room_id TEXT,\
medal_name TEXT,\
medal_level INTEGER,\
medal_up TEXT,\
create_time time NOT NULL)");
    }

    // 舰长
    if (!hasTable("guard"))
    {
        createTable("CREATE TABLE guard(\
id INT PRIMARY KEY NOT NULL,\
uname TEXT NOT NULL,\
uid TEXT NOT NULL,\
uguard INTEGER,\
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
id INT PRIMARY KEY NOT NULL,\
uname TEXT NOT NULL,\
uid TEXT NOT NULL,\
msg_type INTEGER NOT NULL,\
admin BOOLEAN,\
anchor_room_id TEXT,\
medal_name TEXT,\
medal_level INTEGER,\
medal_up TEXT,\
special TEXT,\
special_desc TEXT,\
special_info TEXT,\
create_time time NOT NULL)");
    }

    // 勋章
    if (!hasTable("medal"))
    {
        createTable("CREATE TABLE medal(\
id INT PRIMARY KEY NOT NULL,\
anchor_room_id TEXT,\
medal_name TEXT,\
medal_level INTEGER,\
medal_up TEXT,\
create_time time NOT NULL)");
    }
}

/**
 * 随着版本更新，数据库也要相应调整
 * @param version 升级后的版本（不包括前面的v），如4.8.0
 */
void SqlService::upgradeDb(const QString &version)
{

}

void SqlService::insertDanmaku(LiveDanmaku danmaku)
{
    if (!db.isOpen())
    {
        qCritical() << "未打开数据库";
        return ;
    }
    qDebug() << "插入数据库：" << danmaku.toString();

    switch (danmaku.getMsgType())
    {
    case MessageType::MSG_DANMAKU:
    {

    }
        break;
    case MessageType::MSG_GIFT:
    {

    }
        break;
    case MessageType::MSG_GUARD_BUY:
    {

    }
        break;
    case MessageType::MSG_WELCOME:
    {

    }
        break;
    case MessageType::MSG_ATTENTION:
    {

    }
        break;

    }
}
