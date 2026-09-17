#include "../Scripting/ScriptComponent.h"

ScriptComponent::ScriptComponent(sol::state &lua, const std::string &path)
    : m_Lua(&lua), m_Path(path)
{
    m_Env = sol::environment(*m_Lua, sol::create, m_Lua->globals());
    Reload();
}

void ScriptComponent::Reload()
{
    if (std::filesystem::exists(m_Path))
    {
        m_LastWriteTime = std::filesystem::last_write_time(m_Path);
    }

    sol::load_result loadResult = m_Lua->load_file(m_Path);

    if (!loadResult.valid())
    {
        sol::error err = loadResult;
        std::cerr << "[ERROR] [Script] Failed to load Lua script (" << m_Path << "): " << err.what() << std::endl;
        return;
    }

    sol::protected_function scriptFunc = loadResult;
    sol::set_environment(m_Env, scriptFunc);

    sol::protected_function_result execResult = scriptFunc();

    if (!execResult.valid())
    {
        sol::error err = execResult;
        std::cerr << "[ERROR] [Script] Execution error in Lua script (" << m_Path << "): " << err.what() << std::endl;
        return;
    }

    std::cout << "[INFO] [Script] Successfully compiled and attached script: " << m_Path << "\n";

    m_OnCreate = m_Env["OnCreate"];
    m_OnUpdate = m_Env["OnUpdate"];
    m_OnCollision = m_Env["OnCollision"];
}

void ScriptComponent::ReloadIfNeeded()
{
    if (!std::filesystem::exists(m_Path)) return;
    
    auto currentWriteTime = std::filesystem::last_write_time(m_Path);
    if (currentWriteTime > m_LastWriteTime)
    {
        std::cout << "[INFO] [Script] Hot-reloading script: " << m_Path << "\n";
        Reload();
    }
}

void ScriptComponent::OnCreate() const { if (m_OnCreate.valid()) m_OnCreate(m_Env["self"].get_or(0)); }

void ScriptComponent::OnUpdate(float dt) const { if (m_OnUpdate.valid()) m_OnUpdate(m_Env["self"].get_or(0), dt); }

void ScriptComponent::OnCollision(Entity other) const
{
    if (m_OnCollision.valid()) m_OnCollision(m_Env["self"].get_or(0), other);
}

void ScriptComponent::SetEntity(Entity e) { m_Env["self"] = e; }
