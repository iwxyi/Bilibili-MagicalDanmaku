#ifndef LUAENGINE_H
#define LUAENGINE_H

#include "languageservicebase.h"
#include "sol.hpp"

class LuaEngine : public LanguageServiceBase
{
public:
    explicit LuaEngine(QObject *parent = nullptr);

    QString runCode(const LiveDanmaku &danmaku, const QString &code) override;

private:
    void init();

private:
    sol::state lua;
};

#endif // LUAENGINE_H
