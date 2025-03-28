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

private slots:
    void onTimer();

private:
    SqlService* sqlService;
    QTimer* timer;
};

#endif // FANSARCHIVESSERVICE_H
