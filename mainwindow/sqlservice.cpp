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
    auto hasTable = [=](const QString& name) ->bool {
        QSqlQuery query;
        query.exec(QString("select count(*) from sqlite_master where type='table' and name='%1'").arg(name));
        if(query.next())
        {
            return (query.value(0).toInt() != 0);
        }
        return false;
    };

    // 弹幕
    if (!hasTable("danmu_msg"))
    {

    }

    // 送礼（不包括舰长）
    if (!hasTable("gift"))
    {

    }

    // 舰长
    if (!hasTable("guard"))
    {

    }

    // 进入
    if (!hasTable("welcome"))
    {

    }

    // 关注
    if (!hasTable("follow"))
    {

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
