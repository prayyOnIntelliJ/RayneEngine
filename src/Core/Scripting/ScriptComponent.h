#ifndef RAYNEENGINE_SCRIPTCOMPONENT_H
#define RAYNEENGINE_SCRIPTCOMPONENT_H

#include <sol/sol.hpp>
#include <string>

#include "../ECS/Entity.h"

class ScriptComponent
{
public:
    ScriptComponent(sol::state& lua, const std::string& path);

    void OnCreate() const;
    void OnUpdate(float dt) const;
    void SetEntity(Entity e);
    sol::environment& GetEnv() { return m_Env; }

private:
    sol::environment m_Env;
    sol::state* m_Lua;

    sol::function m_OnCreate;
    sol::function m_OnUpdate;
};

#endif