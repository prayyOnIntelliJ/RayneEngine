#include "LuaState.h"

#include <fstream>
#include <iostream>

#include "../ECS/Components.h"
#include "../ECS/Registry.h"
#include "../Input/InputManager.h"
#include "../Math/MathR.h"
#include "../Resources/ResourceManager.h"
#include "../Audio/AudioManager.h"

sol::state LuaState::s_Lua;
std::vector<LuaApiDoc> s_ApiDocs;

void LuaState::Init(Registry &registry)
{
    s_Lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::table,
        sol::lib::string
    );

    RegisterStatics();

    s_Lua.new_usertype<TransformComponent>("Transform",
                                           "x", &TransformComponent::x,
                                           "y", &TransformComponent::y);

    s_Lua.new_usertype<VelocityComponent>("Velocity",
                                          "dx", &VelocityComponent::dx,
                                          "dy", &VelocityComponent::dy);

    // --- Entity Lifecycle ---
    s_Lua.set_function("CreateEntity", [&]() { return registry.CreateEntity(); });

    s_Lua.set_function("DestroyEntity", [&](const Entity e) { return registry.DestroyEntity(e); });

    // --- Transform ---
    s_Lua.set_function("AddTransform", [&](const Entity e, const float x, const float y) {
        registry.AddComponent(e, TransformComponent{x, y});
    });

    s_Lua.set_function("GetTransform", [&](const Entity e) -> TransformComponent * {
        if (!registry.HasComponent<TransformComponent>(e)) return nullptr;
        return &registry.GetComponent<TransformComponent>(e);
    });

    s_Lua.set_function("HasTransform", [&](const Entity e) -> bool {
        return registry.HasComponent<TransformComponent>(e);
    });

    s_Lua.set_function("SetPosition", [&](const Entity e, const float x, const float y) {
        if (registry.HasComponent<TransformComponent>(e))
        {
            auto &t = registry.GetComponent<TransformComponent>(e);
            t.x = x;
            t.y = y;
        }
    });

    // --- Velocity ---
    s_Lua.set_function("AddVelocity", [&](const Entity e, const float dx, const float dy) {
        registry.AddComponent(e, VelocityComponent{dx, dy});
    });

    s_Lua.set_function("GetVelocity", [&](const Entity e) -> VelocityComponent * {
        if (!registry.HasComponent<VelocityComponent>(e)) return nullptr;
        return &registry.GetComponent<VelocityComponent>(e);
    });

    s_Lua.set_function("HasVelocity", [&](const Entity e) -> bool {
        return registry.HasComponent<VelocityComponent>(e);
    });

    s_Lua.set_function("SetVelocity", [&](const Entity e, const float dx, const float dy) {
        if (registry.HasComponent<VelocityComponent>(e))
        {
            auto &v = registry.GetComponent<VelocityComponent>(e);
            v.dx = dx;
            v.dy = dy;
        }
    });

    // --- Sprite & Render ---
    s_Lua.set_function("AddSprite", [&](const Entity e, const std::string &path, const float w, const float h) {
        registry.AddComponent(e, SpriteComponent(path, sf::Vector2f(w, h)));
    });

    s_Lua.set_function("SetSprite", [&](const Entity e, const std::string &path) {
        if (registry.HasComponent<SpriteComponent>(e))
        {
            auto &sc = registry.GetComponent<SpriteComponent>(e);
            sc = SpriteComponent(path, sc.size);
        }
    });

    s_Lua.set_function("SetSpriteSize", [&](const Entity e, const float w, const float h) {
        if (registry.HasComponent<SpriteComponent>(e))
        {
            auto &sc = registry.GetComponent<SpriteComponent>(e);
            sc.size = {w, h};
            if (sc.texture)
            {
                const auto texSize = sc.texture->getSize();
                if (texSize.x > 0 && texSize.y > 0)
                {
                    sc.sprite.setScale(w / texSize.x, h / texSize.y);
                }
            }
        }
    });

    s_Lua.set_function("HasSprite", [&](const Entity e) -> bool { return registry.HasComponent<SpriteComponent>(e); });

    // --- Camera ---
    s_Lua.set_function("AddCamera", [&](const Entity e) {
        registry.AddComponent(e, CameraComponent{true});
    });

    s_Lua.set_function("RemoveCamera", [&](const Entity e) {
        if (registry.HasComponent<CameraComponent>(e))
            registry.RemoveComponent<CameraComponent>(e);
    });

    s_Lua.set_function("HasCamera", [&](const Entity e) -> bool {
        return registry.HasComponent<CameraComponent>(e);
    });

    s_Lua.set_function("SetColor", [&](const Entity e, int r, int g, int b, sol::optional<int> a) {
        if (registry.HasComponent<RenderComponent>(e))
        {
            auto &rc = registry.GetComponent<RenderComponent>(e);
            rc.color = sf::Color(r, g, b, a.value_or(255));
        }
    });

    std::cout << "[LuaState] Initialized Lua with Engine Functions\n";
}

sol::state &LuaState::GetLua() { return s_Lua; }

void LuaState::RegisterStatics()
{
    MathR::RegisterLua(s_Lua);
    InputManager::RegisterLua(s_Lua);
    ResourceManager::RegisterLua(s_Lua);
    AudioManager::RegisterLua(s_Lua);
}
