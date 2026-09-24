#include "LuaState.h"

#include <iostream>

#include "../ECS/Components.h"
#include "../ECS/Registry.h"
#include "../Input/InputManager.h"
#include "../Math/MathR.h"
#include "../Resources/ResourceManager.h"
#include "../Audio/AudioManager.h"
#include "../UI/UIManager.h"
#include "../Application/Application.h"
#include "TimerManager.h"
#include "TweenManager.h"

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
                                           "y", &TransformComponent::y,
                                           "rotation", &TransformComponent::rotation,
                                           "scaleX", &TransformComponent::scaleX,
                                           "scaleY", &TransformComponent::scaleY);

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

    s_Lua.set_function("SetRotation", [&](const Entity e, const float r) {
        if (registry.HasComponent<TransformComponent>(e))
        {
            auto &t = registry.GetComponent<TransformComponent>(e);
            t.rotation = r;
        }
    });

    s_Lua.set_function("SetScale", [&](const Entity e, const float sx, const float sy) {
        if (registry.HasComponent<TransformComponent>(e))
        {
            auto &t = registry.GetComponent<TransformComponent>(e);
            t.scaleX = sx;
            t.scaleY = sy;
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

    s_Lua.set_function("SetCollisionType", [&](const Entity e, const std::string& typeStr) {
        if (registry.HasComponent<CollisionComponent>(e)) {
            if (typeStr == "solid")
                registry.GetComponent<CollisionComponent>(e).type = CollisionType::Solid;
            else
                registry.GetComponent<CollisionComponent>(e).type = CollisionType::Static;
        }
    });

    s_Lua.set_function("GetCollisionType", [&](const Entity e) -> std::string {
        if (registry.HasComponent<CollisionComponent>(e)) {
            return registry.GetComponent<CollisionComponent>(e).type == CollisionType::Solid ? "solid" : "static";
        }
        return "static";
    });

    s_Lua.set_function("AddTag", [&](const Entity e, const std::string& tag) {
        registry.AddComponent(e, TagComponent{tag});
    });

    s_Lua.set_function("GetTag", [&](const Entity e) -> std::string {
        if (!registry.HasComponent<TagComponent>(e)) return "";
        return registry.GetComponent<TagComponent>(e).tag;
    });

    s_Lua.set_function("HasTag", [&](const Entity e) -> bool {
        return registry.HasComponent<TagComponent>(e);
    });

    s_Lua.set_function("SetTag", [&](const Entity e, const std::string& tag) {
        if (registry.HasComponent<TagComponent>(e)) {
            registry.GetComponent<TagComponent>(e).tag = tag;
        } else {
            registry.AddComponent(e, TagComponent{tag});
        }
    });

    s_Lua.set_function("FindEntityWithTag", [&](const std::string& tag) -> Entity {
        Entity found = 0;
        registry.ForEach<TagComponent>([&](Entity e, TagComponent& tc) {
            if (tc.tag == tag && found == 0) found = e;
        });
        return found;
    });

    s_Lua.set_function("SetColor", [&](const Entity e, int r, int g, int b, sol::optional<int> a) {
        if (registry.HasComponent<RenderComponent>(e))
        {
            auto &rc = registry.GetComponent<RenderComponent>(e);
            rc.color = sf::Color(r, g, b, a.value_or(255));
        }
    });

    sol::table timerTable = s_Lua.create_named_table("Timer");
    timerTable.set_function("After", [](float seconds, sol::function cb) {
        TimerManager::Get().After(seconds, cb);
    });

    sol::table tweenTable = s_Lua.create_named_table("Tween");
    tweenTable.set_function("Position", [&registry](Entity e, float targetX, float targetY, float duration, sol::optional<std::string> ease) {
        TweenManager::Get().Position(e, targetX, targetY, duration, ease.value_or("Linear"), registry);
    });

    std::cout << "[INFO] [Lua] Registering Engine Utility bindings...\n";
    sol::table engineTable = s_Lua.create_named_table("Engine");

    engineTable.set_function("Quit", []() {
        if (g_App) g_App->Quit();
    });
    engineTable.set_function("RestartScene", []() {
        if (g_App) g_App->RestartCurrentScene();
    });
    engineTable.set_function("LoadScene", [loadSceneCallback](const std::string &sceneName) {
        if (g_App) g_App->LoadGameScene(sceneName);
        else if (loadSceneCallback) loadSceneCallback(sceneName);
    });

    engineTable.set_function("SetPaused", [](bool paused) {
        if (g_App) g_App->SetPaused(paused);
    });
    engineTable.set_function("IsPaused", []() -> bool {
        return g_App ? g_App->IsPaused() : false;
    });
    engineTable.set_function("TogglePause", []() {
        if (g_App) g_App->TogglePause();
    });
    engineTable.set_function("SetTimeScale", [](float scale) {
        if (g_App) g_App->SetTimeScale(scale);
    });
    engineTable.set_function("GetTimeScale", []() -> float {
        return g_App ? g_App->GetTimeScale() : 1.0f;
    });

    engineTable.set_function("SetFullscreen", [](bool fullscreen) {
        if (g_App) g_App->SetFullscreen(fullscreen);
    });
    engineTable.set_function("ToggleFullscreen", []() {
        if (g_App) g_App->ToggleFullscreen();
    });
    engineTable.set_function("IsFullscreen", []() -> bool {
        return g_App ? g_App->IsFullscreen() : false;
    });
    engineTable.set_function("SetCursorVisible", [](bool visible) {
        if (g_App) g_App->SetCursorVisible(visible);
    });

    engineTable.set_function("TakeScreenshot", [](sol::optional<std::string> path) -> std::string {
        return g_App ? g_App->TakeScreenshot(path.value_or("")) : "";
    });
    engineTable.set_function("OpenURL", [](const std::string &url) {
        if (g_App) g_App->OpenURL(url);
    });

    engineTable.set_function("GetFPS", []() -> float {
        return g_App ? g_App->GetFPS() : 0.f;
    });
    engineTable.set_function("GetDeltaTime", []() -> float {
        return g_App ? g_App->GetDeltaTime() : 0.f;
    });
    engineTable.set_function("ShowFPS", [](bool show) {
        if (g_App) g_App->SetShowFPSOverlay(show);
    });
    engineTable.set_function("IsFPSShown", []() -> bool {
        return g_App ? g_App->IsFPSOverlayShown() : false;
    });

    s_Lua.set_function("QuitGame", []() {
        if (g_App) g_App->Quit();
    });
    s_Lua.set_function("RestartScene", []() {
        if (g_App) g_App->RestartCurrentScene();
    });
    s_Lua.set_function("PauseGame", [](bool paused) {
        if (g_App) g_App->SetPaused(paused);
    });
    s_Lua.set_function("SetTimeScale", [](float scale) {
        if (g_App) g_App->SetTimeScale(scale);
    });
    s_Lua.set_function("GetFPS", []() -> float {
        return g_App ? g_App->GetFPS() : 0.f;
    });
    s_Lua.set_function("LoadScene", [loadSceneCallback](const std::string &sceneName) {
        if (g_App) g_App->LoadGameScene(sceneName);
        else if (loadSceneCallback) loadSceneCallback(sceneName);
    });

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

    s_Lua.set_function("UI_IsButtonHovered", [](const std::string &id) -> bool {
        return UIManager::Get().IsButtonHovered(id);
    });

    s_Lua.set_function("UI_SetVisible", [](const std::string &id, bool visible) {
        UIManager::Get().SetVisible(id, visible);
    });

    s_Lua.set_function("UI_GetVisible", [](const std::string &id) -> bool {
        return UIManager::Get().GetVisible(id);
    });

    s_Lua.set_function("UI_SetOpacity", [](const std::string &id, float opacity) {
        UIManager::Get().SetOpacity(id, opacity);
    });

    s_Lua.set_function("UI_SetTextStyle", [](const std::string &id, int style) {
        UIManager::Get().SetTextStyle(id, style);
    });

    s_Lua.set_function("UI_SetTextAlign", [](const std::string &id, int align) {
        UIManager::Get().SetTextAlign(id, align);
    });

    s_Lua.set_function("UI_SetUpperCase", [](const std::string &id, bool upper) {
        UIManager::Get().SetUpperCase(id, upper);
    });

    s_Lua.set_function("UI_SetFontSize", [](const std::string &id, int size) {
        UIManager::Get().SetFontSize(id, size);
    });

    s_Lua.set_function("UI_SetLetterSpacing", [](const std::string &id, float spacing) {
        UIManager::Get().SetLetterSpacing(id, spacing);
    });

    s_Lua.set_function("UI_SetLineSpacing", [](const std::string &id, float spacing) {
        UIManager::Get().SetLineSpacing(id, spacing);
    });

    s_Lua.set_function("UI_SetTextOutline", [](const std::string &id, int r, int g, int b, int a, float thickness) {
        UIManager::Get().SetTextOutline(id, r, g, b, a, thickness);
    });

    s_Lua.set_function("UI_SetTextOffset", [](const std::string &id, float ox, float oy) {
        UIManager::Get().SetTextOffset(id, ox, oy);
    });

    s_Lua.set_function("UI_SetTextColor", [](const std::string &id, int r, int g, int b, int a) {
        UIManager::Get().SetTextColor(id, r, g, b, a);
    });

    s_Lua.set_function("UI_SetOutline", [](const std::string &id, int r, int g, int b, int a, float thickness) {
        UIManager::Get().SetOutline(id, r, g, b, a, thickness);
    });

    s_Lua.set_function("UI_SetDisabled", [](const std::string &id, bool disabled) {
        UIManager::Get().SetDisabled(id, disabled);
    });

    s_Lua.set_function("UI_SetTexture", [](const std::string &id, const std::string &path) {
        UIManager::Get().SetTexture(id, path);
    });

    s_Lua.set_function("UI_SetHoverTexture", [](const std::string &id, const std::string &path) {
        UIManager::Get().SetHoverTexture(id, path);
    });

    s_Lua.set_function("UI_SetPressedTexture", [](const std::string &id, const std::string &path) {
        UIManager::Get().SetPressedTexture(id, path);
    });

    s_Lua.set_function("UI_SetCheckedTexture", [](const std::string &id, const std::string &path) {
        UIManager::Get().SetCheckedTexture(id, path);
    });

    s_Lua.set_function("UI_SetChecked", [](const std::string &id, bool checked) {
        UIManager::Get().SetChecked(id, checked);
    });

    s_Lua.set_function("UI_GetChecked", [](const std::string &id) -> bool {
        return UIManager::Get().GetChecked(id);
    });

    s_Lua.set_function("UI_SetSliderValue", [](const std::string &id, float value) {
        UIManager::Get().SetSliderValue(id, value);
    });

    s_Lua.set_function("UI_GetSliderValue", [](const std::string &id) -> float {
        return UIManager::Get().GetSliderValue(id);
    });

    s_Lua.set_function("UI_SetProgressValue", [](const std::string &id, float value) {
        UIManager::Get().SetProgressValue(id, value);
    });

    s_Lua.set_function("UI_GetProgressValue", [](const std::string &id) -> float {
        return UIManager::Get().GetProgressValue(id);
    });
    
    s_Lua.set_function("UI_SetFocused", [](const std::string &id, bool focused) {
        UIManager::Get().SetFocused(id, focused);
    });
    
    s_Lua.set_function("UI_GetFocused", [](const std::string &id) -> bool {
        return UIManager::Get().GetFocused(id);
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
