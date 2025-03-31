#ifndef FANSARCHIVESSERVICE_H
#define FANSARCHIVESSERVICE_H

#include <QObject>
#include <QTimer>
#include "sqlservice.h"
/**
 * 为粉丝生成档案
 */
class FansArchivesService : public QObject
{
    Q_OBJECT
public:
    FansArchivesService(SqlService* sqlService, QObject* parent = nullptr);

    void start();

signals:
    void signalFansArchivesLoadingStatusChanged(const QString& status);
    void signalFansArchivesUpdated(QString uid);
    void signalError(const QString& err); // 错误信号，会停止处理，除非再手动点开

private slots:
    void onTimer();

private:
    SqlService* sqlService;
    QTimer* timer;
};

#endif // FANSARCHIVESSERVICE_H
