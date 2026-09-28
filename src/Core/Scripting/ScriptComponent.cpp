#include "../Scripting/ScriptComponent.h"
#include "../Resources/ResourceManager.h"

ScriptComponent::ScriptComponent(sol::state &lua, const std::string &path)
    : m_Lua(&lua), m_Path(ResourceManager::ResolveAssetPath(path))
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
    m_OnButtonClicked = m_Env["OnButtonClicked"];
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

void ScriptComponent::OnButtonClicked(const std::string &buttonId) const
{
    if (m_OnButtonClicked.valid())
    {
        auto res = m_OnButtonClicked(buttonId);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnButtonClicked execution error (" << m_Path << "): " << err.what() << "\n";
        }
    }

    sol::object uiObj = m_Env["UI"];
    if (uiObj.is<sol::table>())
    {
        sol::object cb = uiObj.as<sol::table>()["OnButtonClicked"];
        if (cb.is<sol::protected_function>())
        {
            sol::protected_function pfn = cb.as<sol::protected_function>();
            auto res = pfn(buttonId);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] UI.OnButtonClicked execution error (" << m_Path << "): " << err.what() << "\n";
            }
        }
    }
}

void ScriptComponent::SetEntity(Entity e) { m_Env["self"] = e; }

std::vector<ScriptComponent::Property> ScriptComponent::GetExportedProperties()
{
    std::vector<Property> props;
    sol::object exportObj = m_Env["Export"];
    if (exportObj.is<sol::table>())
    {
        sol::table exportTable = exportObj.as<sol::table>();
        for (auto& kv : exportTable)
        {
            if (kv.second.is<std::string>())
            {
                std::string propName = kv.second.as<std::string>();
                Property prop;
                prop.name = propName;
                sol::object val = m_Env[propName];
                
                // Int must be checked before float, or we can just use float. Actually Lua only has numbers.
                // Sol2 can check is<int>() or is<float>(). Let's use is<float>() since Lua numbers are doubles.
                // Actually, Sol2 can distinguish integers in 5.3+.
                if (val.is<int>()) { prop.type = PropertyType::Int; prop.intVal = val.as<int>(); prop.floatVal = val.as<float>(); }
                else if (val.is<float>()) { prop.type = PropertyType::Float; prop.floatVal = val.as<float>(); }
                else if (val.is<bool>()) { prop.type = PropertyType::Bool; prop.boolVal = val.as<bool>(); }
                else if (val.is<std::string>()) { prop.type = PropertyType::String; prop.stringVal = val.as<std::string>(); }
                else { prop.type = PropertyType::Unknown; }
                
                if (prop.type != PropertyType::Unknown)
                    props.push_back(prop);
            }
        }
    }
    return props;
}

void ScriptComponent::SetExportedProperty(const Property& prop)
{
    if (prop.type == PropertyType::Int) m_Env[prop.name] = prop.intVal;
    else if (prop.type == PropertyType::Float) m_Env[prop.name] = prop.floatVal;
    else if (prop.type == PropertyType::Bool) m_Env[prop.name] = prop.boolVal;
    else if (prop.type == PropertyType::String) m_Env[prop.name] = prop.stringVal;
}
