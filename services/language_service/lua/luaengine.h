#ifndef LUAENGINE_H
#define LUAENGINE_H

#include "languageservicebase.h"
#ifdef ENABLE_LUA
#include "sol.hpp"
#endif

class LuaEngine : public LanguageServiceBase
{
    Q_OBJECT
public:
    explicit LuaEngine(QObject *parent = nullptr);

    QString runCode(const LiveDanmaku &danmaku, const QString &code) override;
};

#endif // LUAENGINE_H
