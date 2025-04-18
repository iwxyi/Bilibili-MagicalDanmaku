#include "luaengine.h"
#include "danmakuwrapperstd.h"
#include "settingswrapper.h"
#include "usersettings.h"

LuaEngine::LuaEngine(QObject *parent) : LanguageServiceBase(parent) {}

QString LuaEngine::runCode(const LiveDanmaku &danmaku, const QString &code)
{
#ifdef ENABLE_LUA
    // 每次运行都要重新创建一个实例
    // 如果多线程共享一个实例，会导致线程安全问题，很容易报错
    sol::state* lua = new sol::state();
    lua->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::table, sol::lib::math);

    lua->set_function("print", [&](const std::string& message) {
        QString qMessage = QString::fromStdString(message);
        emit signalLog(qMessage);
    });

    // 注入变量
    DanmakuWrapperStd *danmakuWrapper = new DanmakuWrapperStd(danmaku);
    lua->new_usertype<DanmakuWrapperStd>("DanmakuWrapperStd",
        sol::constructors<DanmakuWrapperStd(const LiveDanmaku&) >(),
        "toJson", &DanmakuWrapperStd::toJson,
        "getMsgType", &DanmakuWrapperStd::getMsgType,
        "getText", &DanmakuWrapperStd::getText,
        "getUid", &DanmakuWrapperStd::getUid,
        "getNickname", &DanmakuWrapperStd::getNickname,
        "getUname", &DanmakuWrapperStd::getUname,
        "getUnameColor", &DanmakuWrapperStd::getUnameColor,
        "getTextColor", &DanmakuWrapperStd::getTextColor,
        "getTimeline", &DanmakuWrapperStd::getTimeline,
        "isAdmin", &DanmakuWrapperStd::isAdmin,
        "getGuard", &DanmakuWrapperStd::getGuard,
        "isVip", &DanmakuWrapperStd::isVip,
        "isSvip", &DanmakuWrapperStd::isSvip,
        "isUidentity", &DanmakuWrapperStd::isUidentity,
        "getLevel", &DanmakuWrapperStd::getLevel,
        "getWealthLevel", &DanmakuWrapperStd::getWealthLevel,
        "getGiftId", &DanmakuWrapperStd::getGiftId,
        "getGiftName", &DanmakuWrapperStd::getGiftName,
        "getNumber", &DanmakuWrapperStd::getNumber,
        "getTotalCoin", &DanmakuWrapperStd::getTotalCoin,
        "isFirst", &DanmakuWrapperStd::isFirst,
        "isReplyMystery", &DanmakuWrapperStd::isReplyMystery,
        "getReplyTypeEnum", &DanmakuWrapperStd::getReplyTypeEnum,
        "getAIReply", &DanmakuWrapperStd::getAIReply,
        "getFaceUrl", &DanmakuWrapperStd::getFaceUrl
    );
    lua->set("_danmaku", danmakuWrapper);

    // 注入settings变量
    lua->new_usertype<SettingsWrapper>("SettingsWrapper",
        sol::constructors<SettingsWrapper(QSettings*) >(),
        "read", &SettingsWrapper::read,
        "write", &SettingsWrapper::write,
        "contains", &SettingsWrapper::contains,
        "remove", &SettingsWrapper::remove
    );
    lua->set("settings", us);
    lua->set("heaps", heaps);

    // 组建function
    QString codeFrame;
    codeFrame = "function _processDanmaku(danmaku) \n" + code + "\nend\n\nreturn _processDanmaku(_danmaku)";

    // 运行代码
    std::string result = lua->script(codeFrame.toStdString());
    delete danmakuWrapper;
    delete lua;
    return QString::fromStdString(result);
#else
    return "";
#endif
}
