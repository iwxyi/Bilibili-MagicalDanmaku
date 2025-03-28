#include "fansarchivesservice.h"

FansArchivesService::FansArchivesService(SqlService* sqlService, QObject* parent)
    : QObject(parent), sqlService(sqlService)
{
    timer = new QTimer(this);
    timer->setInterval(5000);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &FansArchivesService::onTimer);
    timer->start();
}

void FansArchivesService::onTimer()
{
    QString uid = sqlService->getUnprocessedFansArchive();
    if (uid.isEmpty())
        return;
    if (uid == "0")
    {
        qWarning() << "使用了'0'作为uid的粉丝档案";
        return;
    }
    qInfo() << "准备处理粉丝档案：" << uid;

    // 获取现有粉丝档案

    // 获取未处理的弹幕

    // AI生成档案

    // 保存档案

}