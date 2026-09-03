#ifndef RAYNEENGINE_LUASTATE_H
#define RAYNEENGINE_LUASTATE_H

#define SOL_ALL_SAFETIES_ON 1
#include "sol/sol.hpp"

class Registry;

struct LuaApiDoc
{
    std::string name;
    std::string returnType;
    std::vector<std::pair<std::string, std::string> > params;
    std::string description;
};

class LuaState
{
public:
    static void Init(Registry &registry, std::function<void(const std::string&)> loadSceneCallback);

    static sol::state &GetLua();

private:
    static sol::state s_Lua;

    static void RegisterStatics();
};

#endif
