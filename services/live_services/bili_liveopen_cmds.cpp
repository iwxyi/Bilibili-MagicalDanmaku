#include "bili_liveopenservice.h"

bool BiliLiveOpenService::handleUncompMessage(QString cmd, MyJson json)
{
    if (cmd == "LIVE_OPEN_PLATFORM_DM")
    {

    }
    else
    {
        qWarning() << "未处理的未压缩命令=" << cmd << "   正文=" << json;
        return false;
    }

    triggerCmdEvent(cmd, LiveDanmaku(json.value("data").toObject()));

    return true;
}
