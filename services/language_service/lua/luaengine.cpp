#include "luaengine.h"
#include "danmakuwrapper.h"

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
    DanmakuWrapper *jsArg = new DanmakuWrapper(danmaku);
    lua->new_usertype<DanmakuWrapper>("JSArg",
        sol::constructors<DanmakuWrapper(const LiveDanmaku&) >(),
        "toJson", &DanmakuWrapper::toJson,
        "getMsgType", &DanmakuWrapper::getMsgType,
        "getText", &DanmakuWrapper::getText,
        "getUid", &DanmakuWrapper::getUid,
        "getNickname", &DanmakuWrapper::getNickname,
        "getUname", &DanmakuWrapper::getUname,
        "getUnameColor", &DanmakuWrapper::getUnameColor,
        "getTextColor", &DanmakuWrapper::getTextColor,
        "getTimeline", &DanmakuWrapper::getTimeline,
        "isAdmin", &DanmakuWrapper::isAdmin,
        "getGuard", &DanmakuWrapper::getGuard,
        "isVip", &DanmakuWrapper::isVip,
        "isSvip", &DanmakuWrapper::isSvip,
        "isUidentity", &DanmakuWrapper::isUidentity,
        "getLevel", &DanmakuWrapper::getLevel,
        "getWealthLevel", &DanmakuWrapper::getWealthLevel,
        "getGiftId", &DanmakuWrapper::getGiftId,
        "getGiftName", &DanmakuWrapper::getGiftName,
        "getNumber", &DanmakuWrapper::getNumber,
        "getTotalCoin", &DanmakuWrapper::getTotalCoin,
        "isFirst", &DanmakuWrapper::isFirst,
        "isReplyMystery", &DanmakuWrapper::isReplyMystery,
        "getReplyTypeEnum", &DanmakuWrapper::getReplyTypeEnum,
        "getAIReply", &DanmakuWrapper::getAIReply,
        "getFaceUrl", &DanmakuWrapper::getFaceUrl
    );
    lua->set("_danmaku", jsArg);

    // 组建function
    QString codeFrame;
    codeFrame = "function _processDanmaku(danmaku) \n" + code + "\nend\n\nreturn _processDanmaku(_danmaku)";

    // 运行代码
    std::string result = lua->script(codeFrame.toStdString());
    delete jsArg;
    delete lua;
    return QString::fromStdString(result);
#else
    return "";
#endif
}
