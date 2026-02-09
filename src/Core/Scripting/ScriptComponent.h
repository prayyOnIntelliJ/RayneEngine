#ifndef RAYNEENGINE_SCRIPTCOMPONENT_H
#define RAYNEENGINE_SCRIPTCOMPONENT_H

#include <sol/sol.hpp>
#include <string>

class ScriptComponent
{
public:
    ScriptComponent(sol::state& lua, const std::string& path);

    void OnCreate();
    void OnUpdate(float dt);

private:
    sol::state& m_Lua;
    sol::table m_Env;

    sol::function m_OnCreate;
    sol::function m_OnUpdate;
};

#endif