#include "livestatisticservice.h"

LiveStatisticService::LiveStatisticService()
{

}

void LiveStatisticService::startCalculateDailyData()
{
    if (dailySettings)
    {
        saveCalculateDailyData();
        dailySettings->deleteLater();
    }

    QDir dir;
    dir.mkdir(rt->dataPath + "live_daily");
    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    dailySettings = new QSettings(rt->dataPath + "live_daily/" + ac->roomId + "_" + date + ".ini", QSettings::Format::IniFormat);

    dailyCome = dailySettings->value("come", 0).toInt();
    dailyPeopleNum = dailySettings->value("people_num", 0).toInt();
    dailyDanmaku = dailySettings->value("danmaku", 0).toInt();
    dailyNewbieMsg = dailySettings->value("newbie_msg", 0).toInt();
    dailyNewFans = dailySettings->value("new_fans", 0).toInt();
    dailyTotalFans = dailySettings->value("total_fans", 0).toInt();
    dailyGiftSilver = dailySettings->value("gift_silver", 0).toInt();
    dailyGiftGold = dailySettings->value("gift_gold", 0).toInt();
    dailyGuard = dailySettings->value("guard", 0).toInt();
    dailyMaxPopul = dailySettings->value("max_popularity", 0).toInt();
    dailyAvePopul = 0;
    if (us->currentGuards.size())
        dailySettings->setValue("guard_count", us->currentGuards.size());
    else
        updateExistGuards(0);
}

void LiveStatisticService::saveCalculateDailyData()
{
    if (dailySettings)
    {
        dailySettings->setValue("come", dailyCome);
        dailySettings->setValue("people_num", qMax(dailySettings->value("people_num").toInt(), us->userComeTimes.size()));
        dailySettings->setValue("danmaku", dailyDanmaku);
        dailySettings->setValue("newbie_msg", dailyNewbieMsg);
        dailySettings->setValue("new_fans", dailyNewFans);
        dailySettings->setValue("total_fans", ac->currentFans);
        dailySettings->setValue("gift_silver", dailyGiftSilver);
        dailySettings->setValue("gift_gold", dailyGiftGold);
        dailySettings->setValue("guard", dailyGuard);
        if (us->currentGuards.size())
            dailySettings->setValue("guard_count", us->currentGuards.size());
    }
}
