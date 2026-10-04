#include "LuaState.h"

#include <iostream>

#include "../ECS/Components.h"
#include "../ECS/Registry.h"
#include "../ECS/PhysicsSystem.h"
#include "../ECS/HierarchySystem.h"
#include "../Input/InputManager.h"
#include "../Math/MathR.h"
#include "../Resources/ResourceManager.h"
#include "../Audio/AudioManager.h"
#include "../UI/UIManager.h"
#include "../Application/Application.h"
#include "TimerManager.h"
#include "TweenManager.h"
#include "../Scenes/SceneSerializer.h"
#include "../Scenes/CameraManager.h"
#include "ScriptComponent.h"
#include "../Profiler/Profiler.h"

sol::state LuaState::s_Lua;
std::vector<LuaApiDoc> s_ApiDocs;

void LuaState::Init(Registry & registry, std::function < void(const std::string &) > loadSceneCallback)
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
                                           "scaleY", &TransformComponent::scaleY,
                                           "worldX", &TransformComponent::worldX,
                                           "worldY", &TransformComponent::worldY,
                                           "worldRotation", &TransformComponent::worldRotation,
                                           "worldScaleX", &TransformComponent::worldScaleX,
                                           "worldScaleY", &TransformComponent::worldScaleY);

    s_Lua.new_usertype<VelocityComponent>("Velocity",
                                          "dx", &VelocityComponent::dx,
                                          "dy", &VelocityComponent::dy,
                                          "vx", &VelocityComponent::dx,
                                          "vy", &VelocityComponent::dy,
                                          "x", &VelocityComponent::dx,
                                          "y", &VelocityComponent::dy);

    s_Lua.set_function("CreateEntity", [&]() { return registry.CreateEntity(); });

    s_Lua.set_function("DestroyEntity", [&](const Entity e) { return registry.DestroyEntity(e); });

    s_Lua.set_function("Template", [&](sol::object pathObj) -> sol::table {
        std::string pathStr;
        if (pathObj.is<std::string>()) pathStr = pathObj.as<std::string>();
        sol::table t = s_Lua.create_table();
        t["__type"] = "template";
        t["path"] = pathStr;
        t["Instantiate"] = [&registry](sol::table self, float x, float y,
                                       sol::optional<Entity> parent) -> Entity {
            std::string p = self["path"].get_or(std::string(""));
            return SceneSerializer::InstantiateTemplate(registry, p, x, y, parent.value_or(0));
        };
        return t;
    });

    s_Lua.set_function("Image", [](sol::optional<std::string> pathOpt) -> sol::table {
        sol::table t = LuaState::GetLua().create_table();
        t["__type"] = "image";
        t["path"] = pathOpt.value_or("");
        return t;
    });

    s_Lua.set_function("Vec2", [](sol::optional<float> xOpt, sol::optional<float> yOpt) -> sol::table {
        sol::table t = LuaState::GetLua().create_table();
        t["__type"] = "vec2";
        t["x"] = xOpt.value_or(0.f);
        t["y"] = yOpt.value_or(0.f);
        return t;
    });

    s_Lua.set_function(
        "Color", [](sol::optional<int> rOpt, sol::optional<int> gOpt, sol::optional<int> bOpt) -> sol::table {
            sol::table t = LuaState::GetLua().create_table();
            t["__type"] = "color";
            t["r"] = rOpt.value_or(255);
            t["g"] = gOpt.value_or(255);
            t["b"] = bOpt.value_or(255);
            return t;
        });




    s_Lua.set_function("Instantiate",
                       [&registry](sol::object templateObj, float x, float y, sol::optional<Entity> parent) -> Entity {
                           std::string templatePath;
                           if (templateObj.is<std::string>()) { templatePath = templateObj.as<std::string>(); } else if
                           (templateObj.is<sol::table>())
                           {
                               sol::table t = templateObj.as<sol::table>();
                               templatePath = t["path"].get_or(std::string(""));
                           }
                           if (templatePath.empty())
                           {
                               std::cerr << "[ERROR] [Lua] Instantiate called with empty or invalid template!\n";
                               return 0;
                           }
                           return SceneSerializer::InstantiateTemplate(registry, templatePath, x, y,
                                                                       parent.value_or(0));
                       });

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

    s_Lua.set_function("SetParent",
                       [&](const Entity child, const Entity newParent, sol::optional<bool> keepWorldTransform) {
                           HierarchySystem::SetParent(registry, child, newParent, keepWorldTransform.value_or(true));
                       });

    s_Lua.set_function("GetParent", [&](const Entity child) -> Entity {
        return HierarchySystem::GetParent(registry, child);
    });

    s_Lua.set_function("GetChildren", [&](const Entity parent) -> std::vector<Entity> {
        return HierarchySystem::GetChildren(registry, parent);
    });

    s_Lua.set_function("GetWorldPosition", [&](const Entity e) -> std::tuple<float, float> {
        if (registry.HasComponent<TransformComponent>(e))
        {
            auto &t = registry.GetComponent<TransformComponent>(e);
            return std::make_tuple(t.worldX, t.worldY);
        }
        return std::make_tuple(0.f, 0.f);
    });

    s_Lua.set_function("GetWorldRotation", [&](const Entity e) -> float {
        if (registry.HasComponent<TransformComponent>(e))
        {
            return registry.GetComponent<TransformComponent>(e).worldRotation;
        }
        return 0.f;
    });

    s_Lua.set_function("GetWorldScale", [&](const Entity e) -> std::tuple<float, float> {
        if (registry.HasComponent<TransformComponent>(e))
        {
            auto &t = registry.GetComponent<TransformComponent>(e);
            return std::make_tuple(t.worldScaleX, t.worldScaleY);
        }
        return std::make_tuple(1.f, 1.f);
    });

    s_Lua.set_function("AddVelocity", [&](const Entity e, const float dx, const float dy) -> VelocityComponent & {
        if (registry.HasComponent<VelocityComponent>(e))
        {
            auto &v = registry.GetComponent<VelocityComponent>(e);
            v.dx = dx;
            v.dy = dy;
            return v;
        }
        return registry.AddComponent(e, VelocityComponent{dx, dy});
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
        } else { registry.AddComponent(e, VelocityComponent{dx, dy}); }
    });

    s_Lua.set_function("RemoveVelocity", [&](const Entity e) {
        if (registry.HasComponent<VelocityComponent>(e))
            registry.RemoveComponent<VelocityComponent>(e);
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

    s_Lua.set_function("SetCollisionType", [&](const Entity e, const std::string &typeStr) {
        if (registry.HasComponent<CollisionComponent>(e))
        {
            if (typeStr == "solid")
                registry.GetComponent<CollisionComponent>(e).type = CollisionType::Solid;
            else
                registry.GetComponent<CollisionComponent>(e).type = CollisionType::Static;
        }
    });

    s_Lua.set_function("GetCollisionType", [&](const Entity e) -> std::string {
        if (registry.HasComponent<CollisionComponent>(e))
        {
            return registry.GetComponent<CollisionComponent>(e).type == CollisionType::Solid
                       ? "solid"
                       : "static";
        }
        return "static";
    });

    s_Lua.set_function("AddTag", [&](const Entity e, const std::string &tag) {
        registry.AddComponent(e, TagComponent{tag});
    });

    s_Lua.set_function("GetTag", [&](const Entity e) -> std::string {
        if (!registry.HasComponent<TagComponent>(e)) return "";
        return registry.GetComponent<TagComponent>(e).tag;
    });

    s_Lua.set_function("HasTag", [&](const Entity e) -> bool { return registry.HasComponent<TagComponent>(e); });

    s_Lua.set_function("SetTag", [&](const Entity e, const std::string &tag) {
        if (registry.HasComponent<TagComponent>(e)) { registry.GetComponent<TagComponent>(e).tag = tag; } else
        {
            registry.AddComponent(e, TagComponent{tag});
        }
    });

    s_Lua.set_function("FindEntityWithTag", [&](const std::string &tag) -> Entity {
        Entity found = 0;
        registry.ForEach<TagComponent>([&](Entity e, TagComponent &tc) {
            if (tc.tag == tag && found == 0) found = e;
        });
        return found;
    });

    s_Lua.set_function("FindEntitiesWithTag", [&](const std::string &tag) -> std::vector<Entity> {
        std::vector<Entity> list;
        registry.ForEach<TagComponent>([&](Entity e, TagComponent &tc) {
            if (tc.tag == tag) list.push_back(e);
        });
        return list;
    });

    s_Lua.set_function("AddName", [&](const Entity e, const std::string &name) {
        registry.AddComponent(e, NameComponent{name});
    });

    s_Lua.set_function("GetName", [&](const Entity e) -> std::string {
        if (!registry.HasComponent<NameComponent>(e)) return "";
        return registry.GetComponent<NameComponent>(e).name;
    });

    s_Lua.set_function("HasName", [&](const Entity e) -> bool { return registry.HasComponent<NameComponent>(e); });

    s_Lua.set_function("SetName", [&](const Entity e, const std::string &name) {
        if (registry.HasComponent<NameComponent>(e)) { registry.GetComponent<NameComponent>(e).name = name; } else
        {
            registry.AddComponent(e, NameComponent{name});
        }
    });

    s_Lua.set_function("FindEntityWithName", [&](const std::string &name) -> Entity {
        Entity found = 0;
        registry.ForEach<NameComponent>([&](Entity e, NameComponent &nc) {
            if (nc.name == name && found == 0) found = e;
        });
        return found;
    });

    s_Lua.set_function("FindEntitiesWithName", [&](const std::string &name) -> std::vector<Entity> {
        std::vector<Entity> list;
        registry.ForEach<NameComponent>([&](Entity e, NameComponent &nc) {
            if (nc.name == name) list.push_back(e);
        });
        return list;
    });

    s_Lua.set_function("FindEntity", [&](const std::string &identifier) -> Entity {
        if (identifier.empty()) return 0;
        Entity found = 0;
        registry.ForEach<NameComponent>([&](Entity e, NameComponent &nc) {
            if (nc.name == identifier && found == 0) found = e;
        });
        if (found != 0) return found;
        registry.ForEach<TagComponent>([&](Entity e, TagComponent &tc) {
            if (tc.tag == identifier && found == 0) found = e;
        });
        return found;
    });

    s_Lua.set_function("IsEntityValid", [&](const Entity e) -> bool {
        if (e == 0) return false;
        return registry.HasComponent<TransformComponent>(e) ||
               registry.HasComponent<RenderComponent>(e) ||
               registry.HasComponent<TagComponent>(e) ||
               registry.HasComponent<NameComponent>(e) ||
               registry.HasComponent<ScriptComponent>(e);
    });

    auto resolveEntity = [&registry](sol::object obj) -> Entity {
        if (obj.is<Entity>()) {
            return obj.as<Entity>();
        }
        if (obj.is<int>()) {
            int id = obj.as<int>();
            return id > 0 ? static_cast<Entity>(id) : 0;
        }
        if (obj.is<std::string>()) {
            std::string str = obj.as<std::string>();
            if (str.empty()) return 0;
            Entity found = 0;
            registry.ForEach<NameComponent>([&](Entity e, NameComponent &nc) {
                if (found == 0 && nc.name == str) found = e;
            });
            if (found != 0) return found;
            registry.ForEach<TagComponent>([&](Entity e, TagComponent &tc) {
                if (found == 0 && tc.tag == str) found = e;
            });
            return found;
        }
        if (obj.is<sol::table>()) {
            sol::table t = obj.as<sol::table>();
            sol::object idObj = t["id"];
            if (idObj.is<int>() && idObj.as<int>() > 0) return static_cast<Entity>(idObj.as<int>());
            if (idObj.is<Entity>() && idObj.as<Entity>() > 0) return idObj.as<Entity>();
            sol::object nameObj = t["name"];
            if (nameObj.is<std::string>()) {
                std::string str = nameObj.as<std::string>();
                if (!str.empty()) {
                    Entity found = 0;
                    registry.ForEach<NameComponent>([&](Entity e, NameComponent &nc) {
                        if (found == 0 && nc.name == str) found = e;
                    });
                    if (found != 0) return found;
                    registry.ForEach<TagComponent>([&](Entity e, TagComponent &tc) {
                        if (found == 0 && tc.tag == str) found = e;
                    });
                    return found;
                }
            }
        }
        return 0;
    };

    s_Lua.set_function("GetScript", [&registry, resolveEntity](sol::object target) -> sol::object {
        Entity e = resolveEntity(target);
        if (e != 0 && registry.HasComponent<ScriptComponent>(e)) {
            return registry.GetComponent<ScriptComponent>(e).GetEnv();
        }
        return sol::nil;
    });

    s_Lua.set_function("HasScript", [&registry, resolveEntity](sol::object target) -> bool {
        Entity e = resolveEntity(target);
        return e != 0 && registry.HasComponent<ScriptComponent>(e);
    });

    s_Lua.set_function("CallScript", [&registry, resolveEntity](sol::object target, const std::string &fnName, sol::variadic_args va) -> sol::variadic_results {
        sol::variadic_results results;
        Entity e = resolveEntity(target);
        if (e == 0 || !registry.HasComponent<ScriptComponent>(e)) {
            return results;
        }
        auto &env = registry.GetComponent<ScriptComponent>(e).GetEnv();
        sol::object fnObj = env[fnName];
        if (!fnObj.is<sol::protected_function>()) {
            return results;
        }
        sol::protected_function pfn = fnObj.as<sol::protected_function>();
        auto res = pfn(sol::as_args(va));
        if (!res.valid()) {
            sol::error err = res;
            std::cerr << "[ERROR] [Script] CallScript error (" << fnName << "): " << err.what() << "\n";
            return results;
        }
        for (auto v : res) {
            results.push_back(v);
        }
        return results;
    });

    s_Lua.set_function("SetColor", [&](const Entity e, int r, int g, int b, sol::optional<int> a) {
        if (registry.HasComponent<RenderComponent>(e))
        {
            auto &rc = registry.GetComponent<RenderComponent>(e);
            rc.color = sf::Color(r, g, b, a.value_or(255));
        }
    });

    s_Lua.set_function("AddText", [&](const Entity e, const std::string &text, sol::optional<unsigned int> size) {
        TextComponent tc;
        tc.text = text;
        tc.characterSize = size.value_or(28);
        registry.AddComponent(e, tc);
    });

    s_Lua.set_function("SetText", [&](const Entity e, const std::string &text) {
        if (registry.HasComponent<TextComponent>(e))
        {
            registry.GetComponent<TextComponent>(e).text = text;
        } else
        {
            TextComponent tc;
            tc.text = text;
            registry.AddComponent(e, tc);
        }
    });

    s_Lua.set_function("GetText", [&](const Entity e) -> std::string {
        if (registry.HasComponent<TextComponent>(e))
            return registry.GetComponent<TextComponent>(e).text;
        return "";
    });

    s_Lua.set_function("SetTextSize", [&](const Entity e, unsigned int size) {
        if (registry.HasComponent<TextComponent>(e))
            registry.GetComponent<TextComponent>(e).characterSize = size;
    });

    s_Lua.set_function("SetTextColor", [&](const Entity e, int r, int g, int b, sol::optional<int> a) {
        if (registry.HasComponent<TextComponent>(e))
            registry.GetComponent<TextComponent>(e).color = sf::Color(r, g, b, a.value_or(255));
    });

    s_Lua.set_function("HasText", [&](const Entity e) -> bool { return registry.HasComponent<TextComponent>(e); });

    s_Lua.set_function("AddAudioSource",
                       [&](const Entity e, const std::string &path, sol::optional<float> volume,
                           sol::optional<float> pitch, sol::optional<bool> loop) {
                           AudioSourceComponent ac;
                           ac.soundPath = path;
                           ac.volume = volume.value_or(100.0f);
                           ac.pitch = pitch.value_or(1.0f);
                           ac.loop = loop.value_or(false);
                           registry.AddComponent(e, ac);
                       });

    s_Lua.set_function("PlayAudio", [&](const Entity e) {
        if (registry.HasComponent<AudioSourceComponent>(e))
        {
            const auto &ac = registry.GetComponent<AudioSourceComponent>(e);
            if (!ac.soundPath.empty())
            {
                AudioManager::Get().PlaySound(ac.soundPath, ac.volume, ac.pitch, ac.loop);
            }
        }
    });

    s_Lua.set_function("SetAudioClip", [&](const Entity e, const std::string &path) {
        if (registry.HasComponent<AudioSourceComponent>(e))
            registry.GetComponent<AudioSourceComponent>(e).soundPath = path;
    });

    s_Lua.set_function("SetAudioVolume", [&](const Entity e, float volume) {
        if (registry.HasComponent<AudioSourceComponent>(e))
            registry.GetComponent<AudioSourceComponent>(e).volume = volume;
    });

    s_Lua.set_function("SetAudioPitch", [&](const Entity e, float pitch) {
        if (registry.HasComponent<AudioSourceComponent>(e))
            registry.GetComponent<AudioSourceComponent>(e).pitch = pitch;
    });

    s_Lua.set_function("SetAudioLoop", [&](const Entity e, bool loop) {
        if (registry.HasComponent<AudioSourceComponent>(e))
            registry.GetComponent<AudioSourceComponent>(e).loop = loop;
    });

    s_Lua.set_function("HasAudioSource", [&](const Entity e) -> bool {
        return registry.HasComponent<AudioSourceComponent>(e);
    });

    s_Lua.set_function("AddParticleEmitter", [&](const Entity e) {
        registry.AddComponent(e, ParticleEmitterComponent{});
    });

    s_Lua.set_function("SetParticleEmitting", [&](const Entity e, bool emitting) {
        if (registry.HasComponent<ParticleEmitterComponent>(e))
            registry.GetComponent<ParticleEmitterComponent>(e).emitting = emitting;
    });

    s_Lua.set_function("IsParticleEmitting", [&](const Entity e) -> bool {
        if (registry.HasComponent<ParticleEmitterComponent>(e))
            return registry.GetComponent<ParticleEmitterComponent>(e).emitting;
        return false;
    });

    s_Lua.set_function("SetParticleRate", [&](const Entity e, float rate) {
        if (registry.HasComponent<ParticleEmitterComponent>(e))
            registry.GetComponent<ParticleEmitterComponent>(e).emissionRate = rate;
    });

    s_Lua.set_function("SetParticleSpeed", [&](const Entity e, float speed) {
        if (registry.HasComponent<ParticleEmitterComponent>(e))
            registry.GetComponent<ParticleEmitterComponent>(e).speed = speed;
    });

    s_Lua.set_function("HasParticleEmitter", [&](const Entity e) -> bool {
        return registry.HasComponent<ParticleEmitterComponent>(e);
    });

    sol::table timerTable = s_Lua.create_named_table("Timer");
    timerTable.set_function("After", [](float seconds, sol::function cb) { TimerManager::Get().After(seconds, cb); });

    sol::table tweenTable = s_Lua.create_named_table("Tween");
    tweenTable.set_function(
        "Position",
        [&registry](Entity e, float targetX, float targetY, float duration, sol::optional<std::string> ease) {
            TweenManager::Get().Position(e, targetX, targetY, duration, ease.value_or("Linear"), registry);
        });

    std::cout << "[INFO] [Lua] Registering Engine Utility bindings...\n";
    sol::table engineTable = s_Lua.create_named_table("Engine");

    engineTable.set_function("Quit", []() { if (g_App) g_App->Quit(); });
    engineTable.set_function("RestartScene", []() { if (g_App) g_App->RestartCurrentScene(); });
    engineTable.set_function("RestartCurrentScene", []() { if (g_App) g_App->RestartCurrentScene(); });
    engineTable.set_function("LoadScene", [loadSceneCallback](const std::string &sceneName) {
        if (g_App) g_App->LoadGameScene(sceneName);
        else if (loadSceneCallback) loadSceneCallback(sceneName);
    });

    engineTable.set_function("SetPaused", [](bool paused) { if (g_App) g_App->SetPaused(paused); });
    engineTable.set_function("IsPaused", []() -> bool { return g_App ? g_App->IsPaused() : false; });
    engineTable.set_function("GetPaused", []() -> bool { return g_App ? g_App->IsPaused() : false; });
    engineTable.set_function("TogglePause", []() { if (g_App) g_App->TogglePause(); });
    engineTable.set_function("SetTimeScale", [](float scale) { if (g_App) g_App->SetTimeScale(scale); });
    engineTable.set_function("GetTimeScale", []() -> float { return g_App ? g_App->GetTimeScale() : 1.0f; });

    engineTable.set_function("SetFullscreen", [](bool fullscreen) { if (g_App) g_App->SetFullscreen(fullscreen); });
    engineTable.set_function("ToggleFullscreen", []() { if (g_App) g_App->ToggleFullscreen(); });
    engineTable.set_function("IsFullscreen", []() -> bool { return g_App ? g_App->IsFullscreen() : false; });
    engineTable.set_function("SetCursorVisible", [](bool visible) { if (g_App) g_App->SetCursorVisible(visible); });

    engineTable.set_function("TakeScreenshot", [](sol::optional<std::string> path) -> std::string {
        return g_App ? g_App->TakeScreenshot(path.value_or("")) : "";
    });
    engineTable.set_function("OpenURL", [](const std::string &url) { if (g_App) g_App->OpenURL(url); });

    auto engineLogFunc = [](sol::variadic_args va) {
        std::string fullMsg;
        for (auto v: va)
        {
            if (!fullMsg.empty()) fullMsg += "  ";
            sol::object obj = v;
            if (obj.is<std::string>()) { fullMsg += obj.as<std::string>(); } else if (obj.is<bool>())
            {
                fullMsg += (obj.as<bool>() ? "true" : "false");
            } else if (obj.is<int>()) { fullMsg += std::to_string(obj.as<int>()); } else if (obj.is<double>())
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "%g", obj.as<double>());
                fullMsg += buf;
            } else if (obj.is<sol::nil_t>()) { fullMsg += "nil"; } else
            {
                sol::state_view sv(obj.lua_state());
                sol::function tostringFunc = sv["tostring"];
                if (tostringFunc.valid())
                {
                    sol::protected_function_result res = tostringFunc(obj);
                    if (res.valid()) fullMsg += res.get<std::string>();
                }
            }
        }
        std::cout << "[LOG] " << fullMsg << "\n";
    };
    engineTable.set_function("Log", engineLogFunc);
    engineTable.set_function("LogWarning", [](const std::string &msg) { std::cout << "[WARN] " << msg << "\n"; });
    engineTable.set_function("LogError", [](const std::string &msg) { std::cerr << "[ERROR] " << msg << "\n"; });
    s_Lua.set_function("print", engineLogFunc);

    auto screenLogFunc = [](const std::string &msg, sol::optional<float> duration, sol::optional<int> r,
                            sol::optional<int> g, sol::optional<int> b) {
        if (g_App)
        {
            float dur = duration.value_or(3.5f);
            sf::Color col = sf::Color(45, 212, 191);
            if (r.has_value() && g.has_value() && b.has_value()) { col = sf::Color(r.value(), g.value(), b.value()); }
            g_App->LogToScreen(msg, dur, col);
        }
    };
    engineTable.set_function("LogToScreen", screenLogFunc);
    s_Lua.set_function("LogToScreen", screenLogFunc);

    engineTable.set_function("GetFPS", []() -> float { return g_App ? g_App->GetFPS() : 0.f; });
    engineTable.set_function("GetDeltaTime", []() -> float { return g_App ? g_App->GetDeltaTime() : 0.f; });
    engineTable.set_function("ShowFPS", [](bool show) { if (g_App) g_App->SetShowFPSOverlay(show); });
    engineTable.set_function("IsFPSShown", []() -> bool { return g_App ? g_App->IsFPSOverlayShown() : false; });

    s_Lua.set_function("QuitGame", []() { if (g_App) g_App->Quit(); });
    s_Lua.set_function("RestartScene", []() { if (g_App) g_App->RestartCurrentScene(); });
    s_Lua.set_function("PauseGame", [](bool paused) { if (g_App) g_App->SetPaused(paused); });
    s_Lua.set_function("SetTimeScale", [](float scale) { if (g_App) g_App->SetTimeScale(scale); });
    s_Lua.set_function("GetFPS", []() -> float { return g_App ? g_App->GetFPS() : 0.f; });
    s_Lua.set_function("LoadScene", [loadSceneCallback](const std::string &sceneName) {
        if (g_App) g_App->LoadGameScene(sceneName);
        else if (loadSceneCallback) loadSceneCallback(sceneName);
    });

    std::cout << "[INFO] [Lua] Registering UI Manager bindings...\n";

    sol::table uiTable = s_Lua.create_named_table("UI");
    uiTable["OnButtonClicked"] = sol::nil;
    uiTable["OnButtonHovered"] = sol::nil;
    uiTable["OnSliderChanged"] = sol::nil;
    uiTable["OnCheckboxChanged"] = sol::nil;
    uiTable["OnTextInputChanged"] = sol::nil;
    uiTable["OnTextInputSubmitted"] = sol::nil;
    uiTable["OnUIHover"] = sol::nil;
    uiTable["OnUIFocus"] = sol::nil;

    auto regUI = [&](const std::string &name, auto func) {
        uiTable.set_function(name, func);
        s_Lua.set_function("UI_" + name, func);
    };

    regUI("SetText", [](const std::string &id, const std::string &text) { UIManager::Get().SetText(id, text); });

    regUI("GetText", [](const std::string &id) -> std::string { return UIManager::Get().GetText(id); });

    regUI("SetPosition", [](const std::string &id, float x, float y) { UIManager::Get().SetPosition(id, x, y); });

    regUI("SetParent", [](const std::string &childId, const std::string &parentId, sol::optional<bool> keepWorldPos) {
        UIManager::Get().SetParent(childId, parentId, keepWorldPos.value_or(true));
    });

    regUI("GetParent", [](const std::string &id) -> std::string { return UIManager::Get().GetParent(id); });

    regUI("GetChildren", [](const std::string &id) -> std::vector<std::string> {
        return UIManager::Get().GetChildren(id);
    });

    regUI("GetWorldPosition", [](const std::string &id) -> std::tuple<float, float> {
        auto pos = UIManager::Get().GetWorldPosition(id);
        return std::make_tuple(pos.x, pos.y);
    });

    regUI("SetSize", [](const std::string &id, float w, float h) { UIManager::Get().SetSize(id, w, h); });

    regUI("SetColor", [](const std::string &id, int r, int g, int b, int a) {
        UIManager::Get().SetColor(id, r, g, b, a);
    });

    regUI("SetZIndex", [](const std::string &id, int z) { UIManager::Get().SetZIndex(id, z); });

    regUI("GetZIndex", [](const std::string &id) -> int { return UIManager::Get().GetZIndex(id); });

    regUI("IsButtonClicked", [](const std::string &id) -> bool { return UIManager::Get().IsButtonClicked(id); });

    regUI("IsButtonHovered", [](const std::string &id) -> bool { return UIManager::Get().IsButtonHovered(id); });

    regUI("SetVisible", [](const std::string &id, bool visible) { UIManager::Get().SetVisible(id, visible); });

    regUI("GetVisible", [](const std::string &id) -> bool { return UIManager::Get().GetVisible(id); });

    regUI("SetOpacity", [](const std::string &id, float opacity) { UIManager::Get().SetOpacity(id, opacity); });

    regUI("SetTextStyle", [](const std::string &id, int style) { UIManager::Get().SetTextStyle(id, style); });

    regUI("SetTextAlign", [](const std::string &id, int align) { UIManager::Get().SetTextAlign(id, align); });

    regUI("SetUpperCase", [](const std::string &id, bool upper) { UIManager::Get().SetUpperCase(id, upper); });

    regUI("SetFontSize", [](const std::string &id, int size) { UIManager::Get().SetFontSize(id, size); });

    regUI("SetLetterSpacing", [](const std::string &id, float spacing) {
        UIManager::Get().SetLetterSpacing(id, spacing);
    });

    regUI("SetLineSpacing", [](const std::string &id, float spacing) { UIManager::Get().SetLineSpacing(id, spacing); });

    regUI("SetTextOutline", [](const std::string &id, int r, int g, int b, int a, float thickness) {
        UIManager::Get().SetTextOutline(id, r, g, b, a, thickness);
    });

    regUI("SetTextOffset",
          [](const std::string &id, float ox, float oy) { UIManager::Get().SetTextOffset(id, ox, oy); });

    regUI("SetTextColor", [](const std::string &id, int r, int g, int b, int a) {
        UIManager::Get().SetTextColor(id, r, g, b, a);
    });

    regUI("SetOutline", [](const std::string &id, int r, int g, int b, int a, float thickness) {
        UIManager::Get().SetOutline(id, r, g, b, a, thickness);
    });

    regUI("SetDisabled", [](const std::string &id, bool disabled) { UIManager::Get().SetDisabled(id, disabled); });

    regUI("SetTexture", [](const std::string &id, const std::string &path) { UIManager::Get().SetTexture(id, path); });

    regUI("SetHoverTexture", [](const std::string &id, const std::string &path) {
        UIManager::Get().SetHoverTexture(id, path);
    });

    regUI("SetPressedTexture", [](const std::string &id, const std::string &path) {
        UIManager::Get().SetPressedTexture(id, path);
    });

    regUI("SetCheckedTexture", [](const std::string &id, const std::string &path) {
        UIManager::Get().SetCheckedTexture(id, path);
    });

    regUI("SetChecked", [](const std::string &id, bool checked) { UIManager::Get().SetChecked(id, checked); });

    regUI("GetChecked", [](const std::string &id) -> bool { return UIManager::Get().GetChecked(id); });

    regUI("SetSliderValue", [](const std::string &id, float value) { UIManager::Get().SetSliderValue(id, value); });

    regUI("GetSliderValue", [](const std::string &id) -> float { return UIManager::Get().GetSliderValue(id); });

    regUI("SetProgressValue", [](const std::string &id, float value) { UIManager::Get().SetProgressValue(id, value); });

    regUI("GetProgressValue", [](const std::string &id) -> float { return UIManager::Get().GetProgressValue(id); });

    regUI("SetFocused", [](const std::string &id, bool focused) { UIManager::Get().SetFocused(id, focused); });

    regUI("GetFocused", [](const std::string &id) -> bool { return UIManager::Get().GetFocused(id); });

    regUI("SetTextVAlign", [](const std::string &id, int valign) { UIManager::Get().SetTextVAlign(id, valign); });

    regUI("SetSliderMin", [](const std::string &id, float min) {
        if (auto *el = UIManager::Get().GetElement(id))
        {
            el->sliderMin = min;
            el->UpdateDrawables();
        }
    });

    regUI("SetSliderMax", [](const std::string &id, float max) {
        if (auto *el = UIManager::Get().GetElement(id))
        {
            el->sliderMax = max;
            el->UpdateDrawables();
        }
    });

    regUI("GetSliderMin", [](const std::string &id) -> float {
        auto *el = UIManager::Get().GetElement(id);
        return el ? el->sliderMin : 0.f;
    });

    regUI("GetSliderMax", [](const std::string &id) -> float {
        auto *el = UIManager::Get().GetElement(id);
        return el ? el->sliderMax : 1.f;
    });

    regUI("IsHovered", [](const std::string &id) -> bool {
        auto *el = UIManager::Get().GetElement(id);
        return el ? el->isHovered : false;
    });

    regUI("IsPressed", [](const std::string &id) -> bool {
        auto *el = UIManager::Get().GetElement(id);
        return el ? el->isPressed : false;
    });

    regUI("IsDisabled", [](const std::string &id) -> bool {
        auto *el = UIManager::Get().GetElement(id);
        return el ? el->disabled : false;
    });

    std::cout << "[INFO] [Lua] Lua engine initialized with API bindings successfully.\n";

    PhysicsSystem::RegisterLua(s_Lua, registry);
    CameraManager::RegisterLua(s_Lua, registry);

    s_Lua.script(R"lua(
        local EntityMeta = {
            __type = "entity_meta"
        }

        local EntityMethods = {
            GetId = function(self)
                return self.id or 0
            end,
            GetName = function(self)
                if self.name and self.name ~= "" then return self.name end
                if self.id and self.id ~= 0 then return GetName(self.id) end
                return ""
            end,
            GetTag = function(self)
                if self.id and self.id ~= 0 then return GetTag(self.id) end
                return ""
            end,
            SetTag = function(self, tag)
                if self.id and self.id ~= 0 then SetTag(self.id, tag) end
            end,
            GetScript = function(self)
                local id = self.id
                if (not id or id == 0 or not IsEntityValid(id)) and self.name and self.name ~= "" then
                    id = FindEntity(self.name)
                    if id and id ~= 0 then self.id = id end
                end
                if id and id ~= 0 then
                    return GetScript(id)
                end
                return nil
            end,
            HasScript = function(self)
                local id = self.id
                if (not id or id == 0 or not IsEntityValid(id)) and self.name and self.name ~= "" then
                    id = FindEntity(self.name)
                    if id and id ~= 0 then self.id = id end
                end
                return id ~= nil and id ~= 0 and HasScript(id)
            end,
            IsValid = function(self)
                local id = self.id
                if (not id or id == 0 or not IsEntityValid(id)) and self.name and self.name ~= "" then
                    id = FindEntity(self.name)
                    if id and id ~= 0 then self.id = id end
                end
                return id ~= nil and id ~= 0 and IsEntityValid(id)
            end,
            Destroy = function(self)
                local id = self.id
                if id and id ~= 0 then DestroyEntity(id) end
            end,
            GetTransform = function(self)
                local id = self.id
                if id and id ~= 0 then return GetTransform(id) end
                return nil
            end,
            SetPosition = function(self, x, y)
                local id = self.id
                if id and id ~= 0 then SetPosition(id, x, y) end
            end,
            GetPosition = function(self)
                local id = self.id
                if id and id ~= 0 then return GetWorldPosition(id) end
                return 0, 0
            end,
            GetRotation = function(self)
                local id = self.id
                if id and id ~= 0 then return GetWorldRotation(id) end
                return 0
            end,
            SetRotation = function(self, r)
                local id = self.id
                if id and id ~= 0 then SetRotation(id, r) end
            end,
            GetScale = function(self)
                local id = self.id
                if id and id ~= 0 then return GetWorldScale(id) end
                return 1, 1
            end,
            SetScale = function(self, sx, sy)
                local id = self.id
                if id and id ~= 0 then SetScale(id, sx, sy) end
            end,
            GetVelocity = function(self)
                local id = self.id
                if id and id ~= 0 then
                    local v = GetVelocity(id)
                    if v then return v.dx, v.dy end
                end
                return 0, 0
            end,
            SetVelocity = function(self, dx, dy)
                local id = self.id
                if id and id ~= 0 then SetVelocity(id, dx, dy) end
            end
        }

        EntityMeta.__index = function(t, key)
            -- 1. Check built-in methods
            if EntityMethods[key] then
                return EntityMethods[key]
            end

            -- 2. Resolve entity ID if needed
            local id = t.id
            if (not id or id == 0 or not IsEntityValid(id)) and t.name and t.name ~= "" then
                id = FindEntity(t.name)
                if id and id ~= 0 then
                    t.id = id
                end
            end

            -- 3. Access target script environment
            if id and id ~= 0 then
                local scr = GetScript(id)
                if scr then
                    local val = scr[key]
                    if val ~= nil then
                        if type(val) == "function" then
                            -- Return wrapper function supporting both Enemy.BlaBlaBla(...) and Enemy:BlaBlaBla(...)
                            return function(...)
                                local n = select("#", ...)
                                if n > 0 and select(1, ...) == t then
                                    return val(select(2, ...))
                                else
                                    return val(...)
                                end
                            end
                        else
                            return val
                        end
                    end
                end
            end

            return nil
        end

        EntityMeta.__newindex = function(t, key, val)
            if key == "id" or key == "name" or key == "__type" then
                rawset(t, key, val)
                return
            end

            local id = t.id
            if (not id or id == 0 or not IsEntityValid(id)) and t.name and t.name ~= "" then
                id = FindEntity(t.name)
                if id and id ~= 0 then t.id = id end
            end

            if id and id ~= 0 then
                local scr = GetScript(id)
                if scr then
                    scr[key] = val
                    return
                end
            end

            rawset(t, key, val)
        end

        EntityMeta.__tostring = function(t)
            local desc = t.name
            if not desc or desc == "" then desc = tostring(t.id or 0) end
            return "Entity(" .. desc .. ")"
        end

        _G.Entity = function(nameOrId)
            local t = {
                __type = "entity",
                name = "",
                id = 0
            }
            if type(nameOrId) == "string" then
                t.name = nameOrId
                local found = FindEntity(nameOrId)
                if found and found ~= 0 then t.id = found end
            elseif type(nameOrId) == "number" then
                t.id = nameOrId
                local n = GetName(nameOrId)
                if n and n ~= "" then
                    t.name = n
                else
                    local tg = GetTag(nameOrId)
                    if tg and tg ~= "" then t.name = tg end
                end
            elseif type(nameOrId) == "table" and nameOrId.__type == "entity" then
                t.name = nameOrId.name or ""
                t.id = nameOrId.id or 0
            end
            setmetatable(t, EntityMeta)
            return t
        end

        _G.GetEntity = _G.Entity
    )lua");
}

sol::state &LuaState::GetLua() { return s_Lua; }

void LuaState::RegisterStatics()
{
    MathR::RegisterLua(s_Lua);
    InputManager::RegisterLua(s_Lua);
    ResourceManager::RegisterLua(s_Lua);
    AudioManager::RegisterLua(s_Lua);

    sol::table profilerTable = s_Lua.create_named_table("Profiler");
    profilerTable["BeginSample"] = [](const std::string &name) {
        Profiler::Get().BeginSample(name);
    };
    profilerTable["EndSample"] = [](const std::string &name) {
        Profiler::Get().EndSample(name);
    };
    profilerTable["GetFPS"] = []() -> float {
        return Profiler::Get().GetCurrentFPS();
    };
    profilerTable["GetFrameTime"] = []() -> float {
        return Profiler::Get().GetCurrentFrameTimeMs();
    };
    profilerTable["Sample"] = [](const std::string &name, sol::function func) {
        Profiler::Get().BeginSample(name);
        func();
        Profiler::Get().EndSample(name);
    };
}
