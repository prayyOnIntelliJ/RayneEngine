#include "../Scripting/ScriptComponent.h"

ScriptComponent::ScriptComponent(sol::state& lua, const std::string& path)
    : m_Lua(&lua)
{
    m_Env = sol::environment(*m_Lua, sol::create, m_Lua->globals());

    sol::load_result loadResult = m_Lua->load_file(path);

    if (!loadResult.valid())
    {
        sol::error err = loadResult;
        std::cerr << "Lua Load Error (" << path << "): " << err.what() << std::endl;
        return;
    }

    sol::protected_function scriptFunc = loadResult;

    sol::set_environment(m_Env, scriptFunc);

    sol::protected_function_result execResult = scriptFunc();

    if (!execResult.valid())
    {
        sol::error err = execResult;
        std::cerr << "Lua Execution Error (" << path << "): " << err.what() << std::endl;
        return;
    }

    m_OnCreate = m_Env["OnCreate"];
    m_OnUpdate = m_Env["OnUpdate"];
}

void ScriptComponent::OnCreate() const
{
    if (m_OnCreate.valid()) m_OnCreate(m_Env["self"].get_or(0));
}

void ScriptComponent::OnUpdate(float dt) const
{
    if (m_OnUpdate.valid()) m_OnUpdate(m_Env["self"].get_or(0), dt);
}

void ScriptComponent::OnCollision(Entity other) const
{
    if (m_OnCollision.valid()) m_OnCollision(m_Env["self"].get_or(0), other);
}

void ScriptComponent::SetEntity(Entity e)
{
    m_Env["self"] = e;
}
