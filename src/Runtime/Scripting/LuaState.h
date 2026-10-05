#ifndef RAYNEENGINE_LUASTATE_H
#define RAYNEENGINE_LUASTATE_H

#define SOL_ALL_SAFETIES_ON 1
#include "sol/sol.hpp"

#include "../ECS/Entity.h"

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
    static void Init(Registry &registry, std::function<void(const std::string &)> loadSceneCallback);

    static sol::state &GetLua();

    static Entity GetCurrentEntity();
    static void SetCurrentEntity(Entity e);

    struct Scope
    {
        Entity prev;
        explicit Scope(Entity e);
        ~Scope();
    };

private:
    static sol::state s_Lua;
    static Entity s_CurrentScriptEntity;

    static void RegisterStatics();
};

#endif
