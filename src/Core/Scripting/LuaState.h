#ifndef RAYNEENGINE_LUASTATE_H
#define RAYNEENGINE_LUASTATE_H

#define SOL_ALL_SAFETIES_ON 1
#include "sol/sol.hpp"

class Registry;

class LuaState
{
public:
    static void Init(Registry& registry);
    static sol::state& GetLua();

private:
    static sol::state s_Lua;

    static void RegisterStatics();
};


#endif