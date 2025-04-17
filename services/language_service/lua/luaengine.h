#ifndef LUAENGINE_H
#define LUAENGINE_H

#include "languageservicebase.h"

class LuaEngine : public LanguageServiceBase
{
public:
    explicit LuaEngine(QObject *parent = nullptr);

    QString runCode(const LiveDanmaku &danmaku, const QString &code) override;

private:
};

#endif // LUAENGINE_H
