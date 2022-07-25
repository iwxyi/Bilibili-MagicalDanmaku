#ifndef SQLSERVICE_H
#define SQLSERVICE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "livedanmaku.h"

class SqlService : public QObject
{
    Q_OBJECT
public:
    explicit SqlService(QObject *parent = nullptr);
    virtual ~SqlService();

signals:

public slots:
    void open();
    void close();
    void initTables();
    void upgradeDb(const QString& version);
    void insertDanmaku(LiveDanmaku danmaku);

private:
    QSqlDatabase db;
};

#endif // SQLSERVICE_H
