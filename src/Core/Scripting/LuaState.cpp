#include "LuaState.h"

#include <iostream>

#include "../ECS/Components.h"
#include "../ECS/Registry.h"
#include "../Input/InputManager.h"
#include "../Math/MathR.h"
#include "../Resources/ResourceManager.h"
#include "../Audio/AudioManager.h"
#include "../UI/UIManager.h"

sol::state LuaState::s_Lua;
std::vector<LuaApiDoc> s_ApiDocs;

void LuaState::Init(Registry &registry, std::function<void(const std::string &)> loadSceneCallback)
{
    std::cout << "[INFO] [Lua] Opening base standard libraries...\n";
    s_Lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::table,
        sol::lib::string
    );

    std::cout << "[INFO] [Lua] Registering engine statics and math functions...\n";
    RegisterStatics();

    std::cout << "[INFO] [Lua] Registering ECS component bindings...\n";

    s_Lua.new_usertype<TransformComponent>("Transform",
                                           "x", &TransformComponent::x,
                                           "y", &TransformComponent::y);

    s_Lua.new_usertype<VelocityComponent>("Velocity",
                                          "dx", &VelocityComponent::dx,
                                          "dy", &VelocityComponent::dy);

    s_Lua.set_function("CreateEntity", [&]() { return registry.CreateEntity(); });

    s_Lua.set_function("DestroyEntity", [&](const Entity e) { return registry.DestroyEntity(e); });

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

    s_Lua.set_function("AddCamera", [&](const Entity e) { registry.AddComponent(e, CameraComponent{true}); });

    s_Lua.set_function("RemoveCamera", [&](const Entity e) {
        if (registry.HasComponent<CameraComponent>(e))
            registry.RemoveComponent<CameraComponent>(e);
    });

    s_Lua.set_function("HasCamera", [&](const Entity e) -> bool { return registry.HasComponent<CameraComponent>(e); });

    s_Lua.set_function("AddCollision", [&](const Entity e, sol::optional<int> channel) {
        registry.AddComponent(e, CollisionComponent{channel.value_or(0)});
    });

    s_Lua.set_function("RemoveCollision", [&](const Entity e) {
        if (registry.HasComponent<CollisionComponent>(e))
            registry.RemoveComponent<CollisionComponent>(e);
    });

    s_Lua.set_function("HasCollision", [&](const Entity e) -> bool {
        return registry.HasComponent<CollisionComponent>(e);
    });

    s_Lua.set_function("SetCollisionChannel", [&](const Entity e, int channel) {
        if (registry.HasComponent<CollisionComponent>(e))
            registry.GetComponent<CollisionComponent>(e).channel = channel;
    });

    s_Lua.set_function("GetCollisionChannel", [&](const Entity e) -> int {
        if (registry.HasComponent<CollisionComponent>(e))
            return registry.GetComponent<CollisionComponent>(e).channel;
        return 0;
    });

    s_Lua.set_function("SetColor", [&](const Entity e, int r, int g, int b, sol::optional<int> a) {
        if (registry.HasComponent<RenderComponent>(e))
        {
            auto &rc = registry.GetComponent<RenderComponent>(e);
            rc.color = sf::Color(r, g, b, a.value_or(255));
        }
    });

    s_Lua.set_function("LoadScene", [loadSceneCallback](const std::string &sceneName) {
        if (loadSceneCallback) loadSceneCallback(sceneName);
    });

    // UI Manager bindings
    std::cout << "[INFO] [Lua] Registering UI Manager bindings...\n";

    s_Lua.set_function("UI_SetText", [](const std::string &id, const std::string &text) {
        UIManager::Get().SetText(id, text);
    });

    s_Lua.set_function("UI_GetText", [](const std::string &id) -> std::string { return UIManager::Get().GetText(id); });

    s_Lua.set_function("UI_SetPosition", [](const std::string &id, float x, float y) {
        UIManager::Get().SetPosition(id, x, y);
    });

    s_Lua.set_function("UI_SetSize",
                       [](const std::string &id, float w, float h) { UIManager::Get().SetSize(id, w, h); });

    s_Lua.set_function("UI_SetColor", [](const std::string &id, int r, int g, int b, int a) {
        UIManager::Get().SetColor(id, r, g, b, a);
    });

    s_Lua.set_function("UI_SetZIndex", [](const std::string &id, int z) { UIManager::Get().SetZIndex(id, z); });

    s_Lua.set_function("UI_GetZIndex", [](const std::string &id) -> int { return UIManager::Get().GetZIndex(id); });

    s_Lua.set_function("UI_IsButtonClicked", [](const std::string &id) -> bool {
        return UIManager::Get().IsButtonClicked(id);
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
