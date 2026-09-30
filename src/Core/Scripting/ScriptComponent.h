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
    void OnCollisionEnter(Entity other, float normalX, float normalY) const;
    void OnTriggerEnter(Entity other) const;
    void OnDestroy() const;

    void OnButtonClicked(const std::string &buttonId) const;
    void OnButtonHovered(const std::string &buttonId) const;
    void OnSliderChanged(const std::string &sliderId, float value) const;
    void OnCheckboxChanged(const std::string &checkboxId, bool checked) const;
    void OnTextInputChanged(const std::string &inputId, const std::string &text) const;
    void OnTextInputSubmitted(const std::string &inputId, const std::string &text) const;
    void OnUIHover(const std::string &elementId, bool hovered) const;
    void OnUIFocus(const std::string &elementId, bool focused) const;

    void SetEntity(Entity e);

    sol::environment &GetEnv() { return m_Env; }

    enum class PropertyType { Unknown, Int, Float, Bool, String, Template, Image, Vec2, Color, Entity };
    struct Property {
        std::string name;
        PropertyType type;
        std::string stringVal;   // Image path, Template path, Entity name, String value
        float floatVal = 0.0f;   // also x component for Vec2
        int intVal = 0;
        bool boolVal = false;
        float vec2Y = 0.0f;      // y component for Vec2
        int colorR = 255, colorG = 255, colorB = 255;  // Color components
    };

    std::vector<Property> GetExportedProperties();
    void SetExportedProperty(const Property& prop);

    bool Reload();
    bool ReloadIfNeeded();

private:
    std::string m_Path;
    std::filesystem::file_time_type m_LastWriteTime;
    std::filesystem::file_time_type m_LastAttemptedWriteTime;
    Entity m_Entity = 0;

    sol::environment m_Env;
    sol::state *m_Lua;

    sol::function m_OnCreate;
    sol::function m_OnUpdate;
    sol::function m_OnCollision;
    sol::function m_OnCollisionEnter;
    sol::function m_OnTriggerEnter;
    sol::function m_OnDestroy;
    sol::function m_OnButtonClicked;
    sol::function m_OnButtonHovered;
    sol::function m_OnSliderChanged;
    sol::function m_OnCheckboxChanged;
    sol::function m_OnTextInputChanged;
    sol::function m_OnTextInputSubmitted;
    sol::function m_OnUIHover;
    sol::function m_OnUIFocus;
};

#endif
