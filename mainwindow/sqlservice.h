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

    void setDbPath(const QString& dbDir);
    QString getDbPath() const;
    QSqlQuery getQuery(const QString& sql) const;

signals:
    void signalError(const QString& err);

public slots:
    void open();
    void close();
    void initTables();
    void upgradeDb(const QString& newVersion);
    void insertDanmaku(LiveDanmaku danmaku);
    void insertMusic(LiveDanmaku danmaku);
    void insertCmd(const QString& cmd, const QString &data);
    bool exec(const QString& sql);
    bool tryExec(const QString& sql);

private:
    bool hasTable(const QString& name) const;
    bool createTable(const QString& sql);

private:
    QString dbPath;
    QSqlDatabase db;
};

#endif // SQLSERVICE_H
