#ifndef RAYNEENGINE_SCRIPTCOMPONENT_H
#define RAYNEENGINE_SCRIPTCOMPONENT_H

#include <sol/sol.hpp>
#include <string>
#include <filesystem>

#include "../ECS/Entity.h"

class ScriptComponent
{
public:
    ScriptComponent(sol::state &lua, const std::string &path);

    void OnCreate() const;

    void OnUpdate(float dt) const;

    void OnCollision(Entity other) const;

    void SetEntity(Entity e);

    sol::environment &GetEnv() { return m_Env; }

    void Reload();
    void ReloadIfNeeded();

private:
    std::string m_Path;
    std::filesystem::file_time_type m_LastWriteTime;

    sol::environment m_Env;
    sol::state *m_Lua;

    sol::function m_OnCreate;
    sol::function m_OnUpdate;
    sol::function m_OnCollision;
};

#endif
