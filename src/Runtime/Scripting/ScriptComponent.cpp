#include "../Scripting/ScriptComponent.h"
#include "../Resources/ResourceManager.h"

ScriptComponent::ScriptComponent(sol::state &lua, const std::string &path)
    : m_Lua(&lua), m_Path(ResourceManager::ResolveAssetPath(path))
{
    m_Env = sol::environment(*m_Lua, sol::create, m_Lua->globals());
    Reload();
    std::error_code ec;
    if (std::filesystem::exists(m_Path, ec))
    {
        m_LastWriteTime = std::filesystem::last_write_time(m_Path, ec);
        m_LastAttemptedWriteTime = m_LastWriteTime;
    }
}

bool ScriptComponent::Reload()
{
    std::error_code ec;
    if (!std::filesystem::exists(m_Path, ec)) { return false; }

    sol::load_result loadResult = m_Lua->load_file(m_Path);

    if (!loadResult.valid())
    {
        sol::error err = loadResult;
        std::cerr << "[ERROR] [Script] Failed to load Lua script (" << m_Path << "): " << err.what() << std::endl;
        return false;
    }

    sol::environment newEnv(*m_Lua, sol::create, m_Lua->globals());
    if (m_Entity != 0) { newEnv["self"] = m_Entity; }

    sol::protected_function scriptFunc = loadResult;
    sol::set_environment(newEnv, scriptFunc);

    sol::protected_function_result execResult = scriptFunc();

    if (!execResult.valid())
    {
        sol::error err = execResult;
        std::cerr << "[ERROR] [Script] Execution error in Lua script (" << m_Path << "): " << err.what() << std::endl;
        return false;
    }

    m_LastWriteTime = std::filesystem::last_write_time(m_Path, ec);
    m_LastAttemptedWriteTime = m_LastWriteTime;
    m_Env = newEnv;

    std::cout << "[INFO] [Script] Successfully compiled and attached script: " << m_Path << "\n";

    m_OnCreate = m_Env["OnCreate"];
    m_OnUpdate = m_Env["OnUpdate"];
    m_OnCollision = m_Env["OnCollision"];
    m_OnCollisionEnter = m_Env["OnCollisionEnter"];
    m_OnTriggerEnter = m_Env["OnTriggerEnter"];
    m_OnDestroy = m_Env["OnDestroy"];
    m_OnButtonClicked = m_Env["OnButtonClicked"];
    m_OnButtonHovered = m_Env["OnButtonHovered"];
    m_OnSliderChanged = m_Env["OnSliderChanged"];
    m_OnCheckboxChanged = m_Env["OnCheckboxChanged"];
    m_OnTextInputChanged = m_Env["OnTextInputChanged"];
    m_OnTextInputSubmitted = m_Env["OnTextInputSubmitted"];
    m_OnUIHover = m_Env["OnUIHover"];
    m_OnUIFocus = m_Env["OnUIFocus"];
    m_OnInputReceived = m_Env["OnInputReceived"];
    if (!m_OnInputReceived.valid())
        m_OnInputReceived = m_Env["OnInputReceiced"];
    m_OnInputReceiced = m_Env["OnInputReceiced"];
    return true;
}

bool ScriptComponent::ReloadIfNeeded()
{
    std::error_code ec;
    if (m_Path.empty() || !std::filesystem::exists(m_Path, ec)) return false;

    auto currentWriteTime = std::filesystem::last_write_time(m_Path, ec);
    if (ec) return false;

    if (currentWriteTime > m_LastAttemptedWriteTime)
    {
        m_LastAttemptedWriteTime = currentWriteTime;
        std::cout << "[INFO] [Script] Hot-reloading script: " << m_Path << "\n";
        return Reload();
    }
    return false;
}

void ScriptComponent::OnCreate() const
{
    if (m_OnCreate.valid())
    {
        auto res = m_OnCreate(m_Env["self"].get_or(0));
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnCreate execution error (" << m_Path << "): " << err.what() << "\n";
        }
    }
}

void ScriptComponent::OnUpdate(float dt) const { if (m_OnUpdate.valid()) m_OnUpdate(m_Env["self"].get_or(0), dt); }

void ScriptComponent::OnCollision(Entity other) const
{
    if (m_OnCollision.valid()) m_OnCollision(m_Env["self"].get_or(0), other);
}

void ScriptComponent::OnCollisionEnter(Entity other, float normalX, float normalY) const
{
    if (m_OnCollisionEnter.valid()) m_OnCollisionEnter(m_Env["self"].get_or(0), other, normalX, normalY);
    else if (m_OnCollision.valid()) m_OnCollision(m_Env["self"].get_or(0), other);
}

void ScriptComponent::OnTriggerEnter(Entity other) const
{
    if (m_OnTriggerEnter.valid()) m_OnTriggerEnter(m_Env["self"].get_or(0), other);
    else if (m_OnCollision.valid()) m_OnCollision(m_Env["self"].get_or(0), other);
}

void ScriptComponent::OnDestroy() const
{
    if (m_OnDestroy.valid())
    {
        auto res = m_OnDestroy(m_Env["self"].get_or(0));
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnDestroy execution error (" << m_Path << "): " << err.what() << "\n";
        }
    }
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
                std::cerr << "[ERROR] [Script] UI.OnButtonClicked execution error (" << m_Path << "): " << err.what() <<
                        "\n";
            }
        }
    }
}

void ScriptComponent::OnButtonHovered(const std::string &buttonId) const
{
    if (m_OnButtonHovered.valid())
    {
        auto res = m_OnButtonHovered(buttonId);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnButtonHovered execution error (" << m_Path << "): " << err.what() << "\n";
        }
    }

    sol::object uiObj = m_Env["UI"];
    if (uiObj.is<sol::table>())
    {
        sol::object cb = uiObj.as<sol::table>()["OnButtonHovered"];
        if (cb.is<sol::protected_function>())
        {
            sol::protected_function pfn = cb.as<sol::protected_function>();
            auto res = pfn(buttonId);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] UI.OnButtonHovered execution error (" << m_Path << "): " << err.what() <<
                        "\n";
            }
        }
    }
}

void ScriptComponent::OnSliderChanged(const std::string &sliderId, float value) const
{
    if (m_OnSliderChanged.valid())
    {
        auto res = m_OnSliderChanged(sliderId, value);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnSliderChanged execution error (" << m_Path << "): " << err.what() << "\n";
        }
    }

    sol::object uiObj = m_Env["UI"];
    if (uiObj.is<sol::table>())
    {
        sol::object cb = uiObj.as<sol::table>()["OnSliderChanged"];
        if (cb.is<sol::protected_function>())
        {
            sol::protected_function pfn = cb.as<sol::protected_function>();
            auto res = pfn(sliderId, value);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] UI.OnSliderChanged execution error (" << m_Path << "): " << err.what() <<
                        "\n";
            }
        }
    }
}

void ScriptComponent::OnCheckboxChanged(const std::string &checkboxId, bool checked) const
{
    if (m_OnCheckboxChanged.valid())
    {
        auto res = m_OnCheckboxChanged(checkboxId, checked);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnCheckboxChanged execution error (" << m_Path << "): " << err.what() <<
                    "\n";
        }
    }

    sol::object uiObj = m_Env["UI"];
    if (uiObj.is<sol::table>())
    {
        sol::object cb = uiObj.as<sol::table>()["OnCheckboxChanged"];
        if (cb.is<sol::protected_function>())
        {
            sol::protected_function pfn = cb.as<sol::protected_function>();
            auto res = pfn(checkboxId, checked);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] UI.OnCheckboxChanged execution error (" << m_Path << "): " << err.what()
                        << "\n";
            }
        }
    }
}

void ScriptComponent::OnTextInputChanged(const std::string &inputId, const std::string &text) const
{
    if (m_OnTextInputChanged.valid())
    {
        auto res = m_OnTextInputChanged(inputId, text);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnTextInputChanged execution error (" << m_Path << "): " << err.what() <<
                    "\n";
        }
    }

    sol::object uiObj = m_Env["UI"];
    if (uiObj.is<sol::table>())
    {
        sol::object cb = uiObj.as<sol::table>()["OnTextInputChanged"];
        if (cb.is<sol::protected_function>())
        {
            sol::protected_function pfn = cb.as<sol::protected_function>();
            auto res = pfn(inputId, text);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] UI.OnTextInputChanged execution error (" << m_Path << "): " << err.what()
                        << "\n";
            }
        }
    }
}

void ScriptComponent::OnTextInputSubmitted(const std::string &inputId, const std::string &text) const
{
    if (m_OnTextInputSubmitted.valid())
    {
        auto res = m_OnTextInputSubmitted(inputId, text);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnTextInputSubmitted execution error (" << m_Path << "): " << err.what() <<
                    "\n";
        }
    }

    sol::object uiObj = m_Env["UI"];
    if (uiObj.is<sol::table>())
    {
        sol::object cb = uiObj.as<sol::table>()["OnTextInputSubmitted"];
        if (cb.is<sol::protected_function>())
        {
            sol::protected_function pfn = cb.as<sol::protected_function>();
            auto res = pfn(inputId, text);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] UI.OnTextInputSubmitted execution error (" << m_Path << "): " << err.
                        what() << "\n";
            }
        }
    }
}

void ScriptComponent::OnUIHover(const std::string &elementId, bool hovered) const
{
    if (m_OnUIHover.valid())
    {
        auto res = m_OnUIHover(elementId, hovered);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnUIHover execution error (" << m_Path << "): " << err.what() << "\n";
        }
    }

    sol::object uiObj = m_Env["UI"];
    if (uiObj.is<sol::table>())
    {
        sol::object cb = uiObj.as<sol::table>()["OnUIHover"];
        if (cb.is<sol::protected_function>())
        {
            sol::protected_function pfn = cb.as<sol::protected_function>();
            auto res = pfn(elementId, hovered);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] UI.OnUIHover execution error (" << m_Path << "): " << err.what() << "\n";
            }
        }
    }
}

void ScriptComponent::OnUIFocus(const std::string &elementId, bool focused) const
{
    if (m_OnUIFocus.valid())
    {
        auto res = m_OnUIFocus(elementId, focused);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnUIFocus execution error (" << m_Path << "): " << err.what() << "\n";
        }
    }

    sol::object uiObj = m_Env["UI"];
    if (uiObj.is<sol::table>())
    {
        sol::object cb = uiObj.as<sol::table>()["OnUIFocus"];
        if (cb.is<sol::protected_function>())
        {
            sol::protected_function pfn = cb.as<sol::protected_function>();
            auto res = pfn(elementId, focused);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] UI.OnUIFocus execution error (" << m_Path << "): " << err.what() << "\n";
            }
        }
    }
}

void ScriptComponent::OnInputReceived(const sol::table &eventTable) const
{
    if (m_OnInputReceived.valid())
    {
        auto res = m_OnInputReceived(m_Env["self"].get_or(0), eventTable);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnInputReceived execution error (" << m_Path << "): " << err.what() << "\n";
        }
    } else if (m_OnInputReceiced.valid())
    {
        auto res = m_OnInputReceiced(m_Env["self"].get_or(0), eventTable);
        if (!res.valid())
        {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] OnInputReceiced execution error (" << m_Path << "): " << err.what() << "\n";
        }
    }

    sol::object uiObj = m_Env["UI"];
    if (uiObj.is<sol::table>())
    {
        sol::table uiTable = uiObj.as<sol::table>();
        sol::object cb = uiTable["OnInputReceived"];
        if (!cb.is<sol::protected_function>())
            cb = uiTable["OnInputReceiced"];
        if (cb.is<sol::protected_function>())
        {
            sol::protected_function pfn = cb.as<sol::protected_function>();
            auto res = pfn(eventTable);
            if (!res.valid())
            {
                sol::error err = res;
                std::cerr << "[ERROR] [Script] UI.OnInputReceived execution error (" << m_Path << "): " << err.what() <<
                        "\n";
            }
        }
    }
}

void ScriptComponent::SetEntity(Entity e)
{
    m_Entity = e;
    m_Env["self"] = e;
}

std::vector<ScriptComponent::Property> ScriptComponent::GetExportedProperties()
{
    std::vector<Property> props;
    sol::object exportObj = m_Env["Export"];
    if (exportObj.is<sol::table>())
    {
        sol::table exportTable = exportObj.as<sol::table>();
        for (auto &kv: exportTable)
        {
            if (kv.second.is<std::string>())
            {
                std::string propName = kv.second.as<std::string>();
                Property prop;
                prop.name = propName;
                sol::object val = m_Env[propName];
                if (val.is<sol::table>())
                {
                    sol::table t = val.as<sol::table>();
                    sol::object typeObj = t["__type"];
                    if (typeObj.is<std::string>() && typeObj.as<std::string>() == "template")
                    {
                        prop.type = PropertyType::Template;
                        prop.stringVal = t["path"].get_or(std::string(""));
                    } else if (typeObj.is<std::string>() && typeObj.as<std::string>() == "image")
                    {
                        prop.type = PropertyType::Image;
                        prop.stringVal = t["path"].get_or(std::string(""));
                    } else if (typeObj.is<std::string>() && typeObj.as<std::string>() == "vec2")
                    {
                        prop.type = PropertyType::Vec2;
                        prop.floatVal = t["x"].get_or(0.f);
                        prop.vec2Y = t["y"].get_or(0.f);
                    } else if (typeObj.is<std::string>() && typeObj.as<std::string>() == "color")
                    {
                        prop.type = PropertyType::Color;
                        prop.colorR = t["r"].get_or(255);
                        prop.colorG = t["g"].get_or(255);
                        prop.colorB = t["b"].get_or(255);
                    } else if (typeObj.is<std::string>() && typeObj.as<std::string>() == "entity")
                    {
                        prop.type = PropertyType::Entity;
                        prop.stringVal = t["name"].get_or(std::string(""));
                    }
                } else if (val.is<int>())
                {
                    prop.type = PropertyType::Int;
                    prop.intVal = val.as<int>();
                    prop.floatVal = val.as<float>();
                } else if (val.is<float>())
                {
                    prop.type = PropertyType::Float;
                    prop.floatVal = val.as<float>();
                } else if (val.is<bool>())
                {
                    prop.type = PropertyType::Bool;
                    prop.boolVal = val.as<bool>();
                } else if (val.is<std::string>())
                {
                    prop.type = PropertyType::String;
                    prop.stringVal = val.as<std::string>();
                } else { prop.type = PropertyType::Unknown; }

                if (prop.type != PropertyType::Unknown)
                    props.push_back(prop);
            }
        }
    }
    return props;
}

void ScriptComponent::SetExportedProperty(const Property &prop)
{
    if (prop.type == PropertyType::Int) m_Env[prop.name] = prop.intVal;
    else if (prop.type == PropertyType::Float) m_Env[prop.name] = prop.floatVal;
    else if (prop.type == PropertyType::Bool) m_Env[prop.name] = prop.boolVal;
    else if (prop.type == PropertyType::String) m_Env[prop.name] = prop.stringVal;
    else if (prop.type == PropertyType::Template)
    {
        sol::table t = m_Env.create();
        t["__type"] = "template";
        t["path"] = prop.stringVal;
        m_Env[prop.name] = t;
    } else if (prop.type == PropertyType::Image)
    {
        sol::table t = m_Env.create();
        t["__type"] = "image";
        t["path"] = prop.stringVal;
        m_Env[prop.name] = t;
    } else if (prop.type == PropertyType::Vec2)
    {
        sol::table t = m_Env.create();
        t["__type"] = "vec2";
        t["x"] = prop.floatVal;
        t["y"] = prop.vec2Y;
        m_Env[prop.name] = t;
    } else if (prop.type == PropertyType::Color)
    {
        sol::table t = m_Env.create();
        t["__type"] = "color";
        t["r"] = prop.colorR;
        t["g"] = prop.colorG;
        t["b"] = prop.colorB;
        m_Env[prop.name] = t;
    } else if (prop.type == PropertyType::Entity)
    {
        sol::table t = m_Env.create();
        t["__type"] = "entity";
        t["name"] = prop.stringVal;
        m_Env[prop.name] = t;
    }
}
