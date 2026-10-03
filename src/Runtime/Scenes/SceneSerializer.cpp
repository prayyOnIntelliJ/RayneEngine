#include "SceneSerializer.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "../ECS/Components.h"
#include "../ECS/HierarchySystem.h"
#include "../Scripting/ScriptComponent.h"
#include "../Scripting/LuaState.h"
#include "../Application/Application.h"
using json = nlohmann::json;

void SceneSerializer::LoadIntoRegistry(Registry &registry, const std::string &path)
{
    std::cout << "[INFO] [SceneSerializer] Loading scene from " << path << "...\n";
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] [SceneSerializer] Scene file not found or unreadable: " << path << "\n";
        return;
    }

    registry.Clear();

    std::unordered_map<std::string, Entity> idToEntity;
    std::vector<std::pair<Entity, std::string> > parentLinks;

    json data = json::parse(file);
    for (auto &j: data["objects"])
    {
        Entity entity = registry.CreateEntity();
        std::string objId = j.value("id", "");
        if (!objId.empty()) { idToEntity[objId] = entity; }
        if (j.contains("parent") && !j["parent"].get<std::string>().empty())
        {
            parentLinks.push_back({entity, j["parent"].get<std::string>()});
        }

        TransformComponent t;
        t.x = j["x"];
        t.y = j["y"];
        if (j.contains("rotation")) t.rotation = j["rotation"];
        if (j.contains("scaleX")) t.scaleX = j["scaleX"];
        if (j.contains("scaleY")) t.scaleY = j["scaleY"];

        registry.AddComponent(entity, t);

        sf::Color color = sf::Color(j["color"][0], j["color"][1], j["color"][2]);
        sf::Vector2f size = {j["width"], j["height"]};

        ShapeType shapeType = ShapeType::Rectangle;
        std::string typeStr = j.value("type", "rectangle");
        if (typeStr == "circle") shapeType = ShapeType::Circle;
        else if (typeStr == "triangle") shapeType = ShapeType::Triangle;
        else if (typeStr == "pentagon") shapeType = ShapeType::Pentagon;
        else if (typeStr == "hexagon") shapeType = ShapeType::Hexagon;

        int zIndex = j.value("zIndex", 0);
        bool defVisible = !(typeStr == "spawn_point" || typeStr == "audio_source" || typeStr == "particle_emitter" ||
                            typeStr == "camera" || typeStr == "empty" || typeStr == "trigger_zone");
        bool visibleInGame = j.value("visibleInGame", defVisible);
        registry.AddComponent(entity, RenderComponent{color, size, shapeType, zIndex, visibleInGame});

        if (j.contains("sprite"))
        {
            std::string sp = j["sprite"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) { sp = (std::filesystem::path(ASSET_PATH) / p).string(); }
            registry.AddComponent(entity, SpriteComponent(sp, size));
        }

        if (j.contains("tag")) { registry.AddComponent(entity, TagComponent{j["tag"].get<std::string>()}); }

        if (j.contains("velocity") && j["velocity"].is_object())
        {
            registry.AddComponent(entity, VelocityComponent{
                                      j["velocity"].value("dx", 0.f), j["velocity"].value("dy", 0.f)
                                  });
        }

        if (j.contains("script"))
        {
            std::string sp = j["script"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) { sp = (std::filesystem::path(ASSET_PATH) / p).string(); }
            auto &sc = registry.AddComponent(entity, ScriptComponent(LuaState::GetLua(), sp));
            sc.SetEntity(entity);

            if (j.contains("scriptProperties"))
            {
                for (auto it = j["scriptProperties"].begin(); it != j["scriptProperties"].end(); ++it)
                {
                    ScriptComponent::Property prop;
                    prop.name = it.key();
                    prop.type = static_cast<ScriptComponent::PropertyType>(it.value()["type"].get<int>());
                    if (prop.type == ScriptComponent::PropertyType::Int) prop.intVal = it.value()["value"].get<int>();
                    else if (prop.type == ScriptComponent::PropertyType::Float)
                        prop.floatVal = it.value()["value"].get<float>();
                    else if (prop.type == ScriptComponent::PropertyType::Bool)
                        prop.boolVal = it.value()["value"].get<bool>();
                    else if (prop.type == ScriptComponent::PropertyType::String || prop.type ==
                             ScriptComponent::PropertyType::Template)
                        prop.stringVal = it.value()["value"].get<std::string>();

                    sc.SetExportedProperty(prop);
                }
            }
        }

        if ((j.contains("camera") && (j["camera"] == true || j["camera"].is_object())) || j.value("type", "") ==
            "camera")
        {
            CameraComponent cam{true};
            if (j.contains("camera") && j["camera"].is_object())
            {
                cam.active = j["camera"].value("active", true);
                cam.smoothSpeed = j["camera"].value("smoothSpeed", 0.0f);
                cam.offsetX = j["camera"].value("offsetX", 0.0f);
                cam.offsetY = j["camera"].value("offsetY", 0.0f);
                cam.zoom = j["camera"].value("zoom", 1.0f);
                cam.priority = j["camera"].value("priority", 0);
                cam.multiFollowMode = static_cast<CameraMultiFollowMode>(j["camera"].value("multiFollowMode", 1));
                cam.minZoom = j["camera"].value("minZoom", 0.3f);
                cam.maxZoom = j["camera"].value("maxZoom", 3.0f);
                cam.autoFramePadding = j["camera"].value("autoFramePadding", 200.0f);
            }
            registry.AddComponent(entity, cam);
        }

        if (j.contains("collision"))
        {
            CollisionType cType = CollisionType::Solid;
            if (j["collision"].contains("type") && j["collision"]["type"] == "static")
                cType = CollisionType::Static;
            int ch = j["collision"].value("channel", 0);
            bool isTrig = j["collision"].value("isTrigger", false);
            ColliderShape shape = ColliderShape::Box;
            if (j["collision"].contains("shape") && j["collision"]["shape"] == "circle")
                shape = ColliderShape::Circle;
            else if (j.contains("type") && j["type"] == "circle")
                shape = ColliderShape::Circle;
            registry.AddComponent(entity, CollisionComponent{ch, cType, isTrig, shape});
        }

        if (j.contains("rigidbody"))
        {
            Rigidbody2DComponent rb;
            std::string bt = j["rigidbody"].value("bodyType", "dynamic");
            if (bt == "kinematic") rb.bodyType = BodyType::Kinematic;
            else if (bt == "static") rb.bodyType = BodyType::Static;
            else rb.bodyType = BodyType::Dynamic;

            rb.mass = j["rigidbody"].value("mass", 1.0f);
            rb.gravityScale = j["rigidbody"].value("gravityScale", 1.0f);
            rb.restitution = j["rigidbody"].value("restitution", 0.0f);
            rb.drag = j["rigidbody"].value("drag", 0.05f);
            rb.freezeRotation = j["rigidbody"].value("freezeRotation", true);
            registry.AddComponent(entity, rb);
            if (!registry.HasComponent<VelocityComponent>(entity))
                registry.AddComponent(entity, VelocityComponent{0.f, 0.f});
        }

        if (j.contains("text") && (j["text"].is_object() || j["text"].is_string()))
        {
            TextComponent tc;
            if (j["text"].is_string()) { tc.text = j["text"].get<std::string>(); } else
            {
                tc.text = j["text"].value("text", "World Text");
                tc.characterSize = j["text"].value("size", 28u);
                tc.alignment = j["text"].value("align", 0);
                if (j["text"].contains("color") && j["text"]["color"].is_array() && j["text"]["color"].size() >= 3)
                {
                    tc.color = sf::Color(j["text"]["color"][0], j["text"]["color"][1], j["text"]["color"][2]);
                }
                tc.outlineThickness = j["text"].value("outlineThickness", 0.0f);
            }
            registry.AddComponent(entity, tc);
        }

        if (j.contains("audioSource") && j["audioSource"].is_object())
        {
            AudioSourceComponent ac;
            ac.soundPath = j["audioSource"].value("soundPath", "");
            ac.volume = j["audioSource"].value("volume", 100.0f);
            ac.pitch = j["audioSource"].value("pitch", 1.0f);
            ac.loop = j["audioSource"].value("loop", false);
            ac.playOnStart = j["audioSource"].value("playOnStart", true);
            ac.isSpatial = j["audioSource"].value("isSpatial", false);
            ac.minDistance = j["audioSource"].value("minDistance", 150.0f);
            ac.attenuation = j["audioSource"].value("attenuation", 1.0f);
            registry.AddComponent(entity, ac);
        }

        if (j.contains("particleEmitter") && j["particleEmitter"].is_object())
        {
            ParticleEmitterComponent pec;
            pec.emitting = j["particleEmitter"].value("emitting", true);
            pec.maxParticles = j["particleEmitter"].value("maxParticles", 120);
            pec.emissionRate = j["particleEmitter"].value("rate", 25.0f);
            pec.lifetime = j["particleEmitter"].value("lifetime", 1.5f);
            pec.speed = j["particleEmitter"].value("speed", 120.0f);
            pec.speedVariance = j["particleEmitter"].value("speedVariance", 40.0f);
            pec.angle = j["particleEmitter"].value("angle", -90.0f);
            pec.spreadAngle = j["particleEmitter"].value("spread", 45.0f);
            pec.startSize = j["particleEmitter"].value("startSize", 8.0f);
            pec.endSize = j["particleEmitter"].value("endSize", 2.0f);
            pec.gravityX = j["particleEmitter"].value("gravityX", 0.0f);
            pec.gravityY = j["particleEmitter"].value("gravityY", 60.0f);
            if (j["particleEmitter"].contains("startColor") && j["particleEmitter"]["startColor"].is_array() && j[
                    "particleEmitter"]["startColor"].size() >= 3)
            {
                pec.startColor = sf::Color(j["particleEmitter"]["startColor"][0], j["particleEmitter"]["startColor"][1],
                                           j["particleEmitter"]["startColor"][2]);
            }
            if (j["particleEmitter"].contains("endColor") && j["particleEmitter"]["endColor"].is_array() && j[
                    "particleEmitter"]["endColor"].size() >= 3)
            {
                pec.endColor = sf::Color(j["particleEmitter"]["endColor"][0], j["particleEmitter"]["endColor"][1],
                                         j["particleEmitter"]["endColor"][2]);
            }
            registry.AddComponent(entity, pec);
        }
    }

    for (const auto &link: parentLinks)
    {
        if (idToEntity.count(link.second))
        {
            Entity parentEntity = idToEntity[link.second];
            if (!registry.HasComponent<HierarchyComponent>(link.first))
            {
                registry.AddComponent(link.first, HierarchyComponent{});
            }
            if (!registry.HasComponent<HierarchyComponent>(parentEntity))
            {
                registry.AddComponent(parentEntity, HierarchyComponent{});
            }
            auto &childH = registry.GetComponent<HierarchyComponent>(link.first);
            auto &parentH = registry.GetComponent<HierarchyComponent>(parentEntity);
            childH.parent = parentEntity;
            parentH.children.push_back(link.first);
        }
    }

    HierarchySystem::UpdateWorldTransforms(registry);
    std::cout << "[INFO] [SceneSerializer] Scene '" << path << "' loaded successfully (" << data["objects"].size() <<
            " objects).\n";
}

Entity SceneSerializer::InstantiateTemplate(Registry &registry, const std::string &templatePath, float x, float y,
                                            Entity parent)
{
    std::cout << "[INFO] [SceneSerializer] Instantiating template '" << templatePath << "' at (" << x << ", " << y <<
            ")...\n";
    std::string fullPath = templatePath;
    std::filesystem::path p(templatePath);
    if (!p.is_absolute())
    {
        std::error_code ec;
        std::filesystem::path root = std::filesystem::current_path();
        if (std::filesystem::exists(root / fullPath)) { fullPath = (root / fullPath).string(); } else if (
            std::filesystem::exists(root / ASSET_PATH / fullPath))
        {
            fullPath = (root / ASSET_PATH / fullPath).string();
        } else if (std::filesystem::exists(root / "assets/templates" / fullPath))
        {
            fullPath = (root / "assets/templates" / fullPath).string();
        } else if (std::filesystem::exists(std::string(ASSET_PATH) + "/" + fullPath))
        {
            fullPath = std::string(ASSET_PATH) + "/" + fullPath;
        }
    }

    std::ifstream file(fullPath);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] [SceneSerializer] Template file not found or unreadable: " << templatePath << " (" <<
                fullPath << ")\n";
        return 0;
    }

    json data;
    try { file >> data; } catch (const std::exception &e)
    {
        std::cerr << "[ERROR] [SceneSerializer] JSON parse error in template " << templatePath << ": " << e.what() <<
                "\n";
        return 0;
    }

    if (!data.contains("objects") || !data["objects"].is_array() || data["objects"].empty())
    {
        std::cerr << "[ERROR] [SceneSerializer] Template contains no objects: " << templatePath << "\n";
        return 0;
    }

    std::unordered_map<std::string, Entity> idToEntity;
    std::vector<std::pair<Entity, std::string> > parentLinks;
    std::vector<Entity> createdEntities;
    Entity rootEntity = 0;

    for (size_t i = 0; i < data["objects"].size(); ++i)
    {
        const auto &j = data["objects"][i];
        Entity entity = registry.CreateEntity();
        createdEntities.push_back(entity);

        std::string objId = j.value("id", "");
        if (!objId.empty()) { idToEntity[objId] = entity; }

        std::string parentId = j.value("parent", "");
        bool isRoot = (i == 0) || parentId.empty();

        if (isRoot && rootEntity == 0)
        {
            rootEntity = entity;
            if (parent != 0) { parentLinks.push_back({entity, "__EXTERNAL_PARENT__"}); }
        } else if (!parentId.empty()) { parentLinks.push_back({entity, parentId}); }

        TransformComponent t;
        if (isRoot)
        {
            t.x = x;
            t.y = y;
        } else
        {
            t.x = j.value("x", 0.f);
            t.y = j.value("y", 0.f);
        }
        t.rotation = j.value("rotation", 0.f);
        t.scaleX = j.value("scaleX", 1.f);
        t.scaleY = j.value("scaleY", 1.f);
        registry.AddComponent(entity, t);

        auto c = j.value("color", std::vector<int>{255, 255, 255});
        sf::Color color = sf::Color(c[0], c[1], c[2]);
        sf::Vector2f size = {j.value("width", 32.f), j.value("height", 32.f)};

        ShapeType shapeType = ShapeType::Rectangle;
        std::string typeStr = j.value("type", "rectangle");
        if (typeStr == "circle") shapeType = ShapeType::Circle;
        else if (typeStr == "triangle") shapeType = ShapeType::Triangle;
        else if (typeStr == "pentagon") shapeType = ShapeType::Pentagon;
        else if (typeStr == "hexagon") shapeType = ShapeType::Hexagon;

        int zIndex = j.value("zIndex", 0);
        bool defVisible = !(typeStr == "spawn_point" || typeStr == "audio_source" || typeStr == "particle_emitter" ||
                            typeStr == "camera" || typeStr == "empty" || typeStr == "trigger_zone");
        bool visibleInGame = j.value("visibleInGame", defVisible);
        registry.AddComponent(entity, RenderComponent{color, size, shapeType, zIndex, visibleInGame});

        if (j.contains("sprite"))
        {
            std::string sp = j["sprite"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) { sp = (std::filesystem::path(ASSET_PATH) / p).string(); }
            registry.AddComponent(entity, SpriteComponent(sp, size));
        }

        if (j.contains("tag")) { registry.AddComponent(entity, TagComponent{j["tag"].get<std::string>()}); }

        if (j.contains("velocity") && j["velocity"].is_object())
        {
            registry.AddComponent(entity, VelocityComponent{
                                      j["velocity"].value("dx", 0.f), j["velocity"].value("dy", 0.f)
                                  });
        }

        if (j.contains("script"))
        {
            std::string sp = j["script"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) { sp = (std::filesystem::path(ASSET_PATH) / p).string(); }
            auto &sc = registry.AddComponent(entity, ScriptComponent(LuaState::GetLua(), sp));
            sc.SetEntity(entity);

            if (j.contains("scriptProperties"))
            {
                for (auto it = j["scriptProperties"].begin(); it != j["scriptProperties"].end(); ++it)
                {
                    ScriptComponent::Property prop;
                    prop.name = it.key();
                    prop.type = static_cast<ScriptComponent::PropertyType>(it.value()["type"].get<int>());
                    if (prop.type == ScriptComponent::PropertyType::Int) prop.intVal = it.value()["value"].get<int>();
                    else if (prop.type == ScriptComponent::PropertyType::Float)
                        prop.floatVal = it.value()["value"].get<float>();
                    else if (prop.type == ScriptComponent::PropertyType::Bool)
                        prop.boolVal = it.value()["value"].get<bool>();
                    else if (prop.type == ScriptComponent::PropertyType::String || prop.type ==
                             ScriptComponent::PropertyType::Template)
                        prop.stringVal = it.value()["value"].get<std::string>();

                    sc.SetExportedProperty(prop);
                }
            }
        }

        if ((j.contains("camera") && (j["camera"] == true || j["camera"].is_object())) || j.value("type", "") ==
            "camera")
        {
            CameraComponent cam{true};
            if (j.contains("camera") && j["camera"].is_object())
            {
                cam.active = j["camera"].value("active", true);
                cam.smoothSpeed = j["camera"].value("smoothSpeed", 0.0f);
                cam.offsetX = j["camera"].value("offsetX", 0.0f);
                cam.offsetY = j["camera"].value("offsetY", 0.0f);
                cam.zoom = j["camera"].value("zoom", 1.0f);
                cam.priority = j["camera"].value("priority", 0);
                cam.multiFollowMode = static_cast<CameraMultiFollowMode>(j["camera"].value("multiFollowMode", 1));
                cam.minZoom = j["camera"].value("minZoom", 0.3f);
                cam.maxZoom = j["camera"].value("maxZoom", 3.0f);
                cam.autoFramePadding = j["camera"].value("autoFramePadding", 200.0f);
            }
            registry.AddComponent(entity, cam);
        }

        if (j.contains("collision"))
        {
            CollisionType cType = CollisionType::Solid;
            if (j["collision"].contains("type") && j["collision"]["type"] == "static")
                cType = CollisionType::Static;
            int ch = j["collision"].value("channel", 0);
            bool isTrig = j["collision"].value("isTrigger", false);
            ColliderShape shape = ColliderShape::Box;
            if (j["collision"].contains("shape") && j["collision"]["shape"] == "circle")
                shape = ColliderShape::Circle;
            else if (j.contains("type") && j["type"] == "circle")
                shape = ColliderShape::Circle;
            registry.AddComponent(entity, CollisionComponent{ch, cType, isTrig, shape});
        }

        if (j.contains("rigidbody"))
        {
            Rigidbody2DComponent rb;
            std::string bt = j["rigidbody"].value("bodyType", "dynamic");
            if (bt == "kinematic") rb.bodyType = BodyType::Kinematic;
            else if (bt == "static") rb.bodyType = BodyType::Static;
            else rb.bodyType = BodyType::Dynamic;

            rb.mass = j["rigidbody"].value("mass", 1.0f);
            rb.gravityScale = j["rigidbody"].value("gravityScale", 1.0f);
            rb.restitution = j["rigidbody"].value("restitution", 0.0f);
            rb.drag = j["rigidbody"].value("drag", 0.05f);
            rb.freezeRotation = j["rigidbody"].value("freezeRotation", true);
            registry.AddComponent(entity, rb);
            if (!registry.HasComponent<VelocityComponent>(entity))
                registry.AddComponent(entity, VelocityComponent{0.f, 0.f});
        }

        if (j.contains("text") && (j["text"].is_object() || j["text"].is_string()))
        {
            TextComponent tc;
            if (j["text"].is_string()) { tc.text = j["text"].get<std::string>(); } else
            {
                tc.text = j["text"].value("text", "World Text");
                tc.characterSize = j["text"].value("size", 28u);
                tc.alignment = j["text"].value("align", 0);
                if (j["text"].contains("color") && j["text"]["color"].is_array() && j["text"]["color"].size() >= 3)
                {
                    tc.color = sf::Color(j["text"]["color"][0], j["text"]["color"][1], j["text"]["color"][2]);
                }
                tc.outlineThickness = j["text"].value("outlineThickness", 0.0f);
            }
            registry.AddComponent(entity, tc);
        }

        if (j.contains("audioSource") && j["audioSource"].is_object())
        {
            AudioSourceComponent ac;
            ac.soundPath = j["audioSource"].value("soundPath", "");
            ac.volume = j["audioSource"].value("volume", 100.0f);
            ac.pitch = j["audioSource"].value("pitch", 1.0f);
            ac.loop = j["audioSource"].value("loop", false);
            ac.playOnStart = j["audioSource"].value("playOnStart", true);
            ac.isSpatial = j["audioSource"].value("isSpatial", false);
            ac.minDistance = j["audioSource"].value("minDistance", 150.0f);
            ac.attenuation = j["audioSource"].value("attenuation", 1.0f);
            registry.AddComponent(entity, ac);
        }

        if (j.contains("particleEmitter") && j["particleEmitter"].is_object())
        {
            ParticleEmitterComponent pec;
            pec.emitting = j["particleEmitter"].value("emitting", true);
            pec.maxParticles = j["particleEmitter"].value("maxParticles", 120);
            pec.emissionRate = j["particleEmitter"].value("rate", 25.0f);
            pec.lifetime = j["particleEmitter"].value("lifetime", 1.5f);
            pec.speed = j["particleEmitter"].value("speed", 120.0f);
            pec.speedVariance = j["particleEmitter"].value("speedVariance", 40.0f);
            pec.angle = j["particleEmitter"].value("angle", -90.0f);
            pec.spreadAngle = j["particleEmitter"].value("spread", 45.0f);
            pec.startSize = j["particleEmitter"].value("startSize", 8.0f);
            pec.endSize = j["particleEmitter"].value("endSize", 2.0f);
            pec.gravityX = j["particleEmitter"].value("gravityX", 0.0f);
            pec.gravityY = j["particleEmitter"].value("gravityY", 60.0f);
            if (j["particleEmitter"].contains("startColor") && j["particleEmitter"]["startColor"].is_array() && j[
                    "particleEmitter"]["startColor"].size() >= 3)
            {
                pec.startColor = sf::Color(j["particleEmitter"]["startColor"][0], j["particleEmitter"]["startColor"][1],
                                           j["particleEmitter"]["startColor"][2]);
            }
            if (j["particleEmitter"].contains("endColor") && j["particleEmitter"]["endColor"].is_array() && j[
                    "particleEmitter"]["endColor"].size() >= 3)
            {
                pec.endColor = sf::Color(j["particleEmitter"]["endColor"][0], j["particleEmitter"]["endColor"][1],
                                         j["particleEmitter"]["endColor"][2]);
            }
            registry.AddComponent(entity, pec);
        }

        registry.AddComponent(entity, HierarchyComponent{});
    }

    for (const auto &link: parentLinks)
    {
        if (link.second == "__EXTERNAL_PARENT__" && parent != 0)
        {
            HierarchySystem::SetParent(registry, link.first, parent, true);
        } else if (idToEntity.count(link.second))
        {
            Entity parentEntity = idToEntity[link.second];
            auto &childH = registry.GetComponent<HierarchyComponent>(link.first);
            auto &parentH = registry.GetComponent<HierarchyComponent>(parentEntity);
            childH.parent = parentEntity;
            parentH.children.push_back(link.first);
        }
    }

    HierarchySystem::UpdateWorldTransforms(registry);

    for (Entity e: createdEntities)
    {
        if (registry.HasComponent<ScriptComponent>(e)) { registry.GetComponent<ScriptComponent>(e).OnCreate(); }
    }

    std::cout << "[INFO] [SceneSerializer] Template '" << templatePath << "' instantiated (Root Entity: " << rootEntity
            << ", total entities: " << createdEntities.size() << ").\n";
    return rootEntity;
}
