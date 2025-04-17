#include "luaengine.h"
#include "jsarg.h"

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
    JSArg *jsArg = new JSArg(danmaku);
    lua->new_usertype<JSArg>("JSArg",
        sol::constructors<JSArg(const LiveDanmaku&) >(),
        "toJson", &JSArg::toJson,
        "getMsgType", &JSArg::getMsgType,
        "getText", &JSArg::getText,
        "getUid", &JSArg::getUid,
        "getNickname", &JSArg::getNickname,
        "getUname", &JSArg::getUname,
        "getUnameColor", &JSArg::getUnameColor,
        "getTextColor", &JSArg::getTextColor,
        "getTimeline", &JSArg::getTimeline,
        "isAdmin", &JSArg::isAdmin,
        "getGuard", &JSArg::getGuard,
        "isVip", &JSArg::isVip,
        "isSvip", &JSArg::isSvip,
        "isUidentity", &JSArg::isUidentity,
        "getLevel", &JSArg::getLevel,
        "getWealthLevel", &JSArg::getWealthLevel,
        "getGiftId", &JSArg::getGiftId,
        "getGiftName", &JSArg::getGiftName,
        "getNumber", &JSArg::getNumber,
        "getTotalCoin", &JSArg::getTotalCoin,
        "isFirst", &JSArg::isFirst,
        "isReplyMystery", &JSArg::isReplyMystery,
        "getReplyTypeEnum", &JSArg::getReplyTypeEnum,
        "getAIReply", &JSArg::getAIReply,
        "getFaceUrl", &JSArg::getFaceUrl
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
