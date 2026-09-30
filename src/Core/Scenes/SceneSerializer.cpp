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
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] [SceneSerializer] Scene file not found or unreadable: " << path << "\n";
        return;
    }

    registry.Clear();

    std::unordered_map<std::string, Entity> idToEntity;
    std::vector<std::pair<Entity, std::string>> parentLinks;

    json data = json::parse(file);
    for (auto &j: data["objects"])
    {
        Entity entity = registry.CreateEntity();
        std::string objId = j.value("id", "");
        if (!objId.empty()) {
            idToEntity[objId] = entity;
        }
        if (j.contains("parent") && !j["parent"].get<std::string>().empty()) {
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
        registry.AddComponent(entity, RenderComponent{color, size, shapeType, zIndex});

        if (j.contains("sprite"))
        {
            std::string sp = j["sprite"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) {
                sp = (std::filesystem::path(ASSET_PATH) / p).string();
            }
            registry.AddComponent(entity, SpriteComponent(sp, size));
        }

        if (j.contains("tag"))
        {
            registry.AddComponent(entity, TagComponent{j["tag"].get<std::string>()});
        }

        if (j.contains("velocity"))
        {
            registry.AddComponent(entity, VelocityComponent{
                                      j["velocity"]["dx"], j["velocity"]["dy"]
                                  });
        }

        if (j.contains("script"))
        {
            std::string sp = j["script"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) {
                sp = (std::filesystem::path(ASSET_PATH) / p).string();
            }
            auto &sc = registry.AddComponent(entity, ScriptComponent(LuaState::GetLua(), sp));
            sc.SetEntity(entity);

            if (j.contains("scriptProperties")) {
                for (auto it = j["scriptProperties"].begin(); it != j["scriptProperties"].end(); ++it) {
                    ScriptComponent::Property prop;
                    prop.name = it.key();
                    prop.type = static_cast<ScriptComponent::PropertyType>(it.value()["type"].get<int>());
                    if (prop.type == ScriptComponent::PropertyType::Int) prop.intVal = it.value()["value"].get<int>();
                    else if (prop.type == ScriptComponent::PropertyType::Float) prop.floatVal = it.value()["value"].get<float>();
                    else if (prop.type == ScriptComponent::PropertyType::Bool) prop.boolVal = it.value()["value"].get<bool>();
                    else if (prop.type == ScriptComponent::PropertyType::String || prop.type == ScriptComponent::PropertyType::Template)
                        prop.stringVal = it.value()["value"].get<std::string>();
                    
                    sc.SetExportedProperty(prop);
                }
            }
        }

        if (j.contains("camera") && j["camera"] == true) { registry.AddComponent(entity, CameraComponent{true}); }

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
        }
    }

    for (const auto& link : parentLinks) {
        if (idToEntity.count(link.second)) {
            Entity parentEntity = idToEntity[link.second];
            if (!registry.HasComponent<HierarchyComponent>(link.first)) {
                registry.AddComponent(link.first, HierarchyComponent{});
            }
            if (!registry.HasComponent<HierarchyComponent>(parentEntity)) {
                registry.AddComponent(parentEntity, HierarchyComponent{});
            }
            auto &childH = registry.GetComponent<HierarchyComponent>(link.first);
            auto &parentH = registry.GetComponent<HierarchyComponent>(parentEntity);
            childH.parent = parentEntity;
            parentH.children.push_back(link.first);
        }
    }

    HierarchySystem::UpdateWorldTransforms(registry);
}

Entity SceneSerializer::InstantiateTemplate(Registry &registry, const std::string &templatePath, float x, float y, Entity parent)
{
    std::string fullPath = templatePath;
    std::filesystem::path p(templatePath);
    if (!p.is_absolute()) {
        std::error_code ec;
        std::filesystem::path root = std::filesystem::current_path();
        if (std::filesystem::exists(root / fullPath)) {
            fullPath = (root / fullPath).string();
        } else if (std::filesystem::exists(root / ASSET_PATH / fullPath)) {
            fullPath = (root / ASSET_PATH / fullPath).string();
        } else if (std::filesystem::exists(root / "assets/templates" / fullPath)) {
            fullPath = (root / "assets/templates" / fullPath).string();
        } else if (std::filesystem::exists(std::string(ASSET_PATH) + "/" + fullPath)) {
            fullPath = std::string(ASSET_PATH) + "/" + fullPath;
        }
    }

    std::ifstream file(fullPath);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] [SceneSerializer] Template file not found or unreadable: " << templatePath << " (" << fullPath << ")\n";
        return 0;
    }

    json data;
    try {
        file >> data;
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] [SceneSerializer] JSON parse error in template " << templatePath << ": " << e.what() << "\n";
        return 0;
    }

    if (!data.contains("objects") || !data["objects"].is_array() || data["objects"].empty()) {
        std::cerr << "[ERROR] [SceneSerializer] Template contains no objects: " << templatePath << "\n";
        return 0;
    }

    std::unordered_map<std::string, Entity> idToEntity;
    std::vector<std::pair<Entity, std::string>> parentLinks;
    std::vector<Entity> createdEntities;
    Entity rootEntity = 0;

    for (size_t i = 0; i < data["objects"].size(); ++i)
    {
        const auto &j = data["objects"][i];
        Entity entity = registry.CreateEntity();
        createdEntities.push_back(entity);

        std::string objId = j.value("id", "");
        if (!objId.empty()) {
            idToEntity[objId] = entity;
        }

        std::string parentId = j.value("parent", "");
        bool isRoot = (i == 0) || parentId.empty();

        if (isRoot && rootEntity == 0) {
            rootEntity = entity;
            if (parent != 0) {
                parentLinks.push_back({entity, "__EXTERNAL_PARENT__"});
            }
        } else if (!parentId.empty()) {
            parentLinks.push_back({entity, parentId});
        }

        TransformComponent t;
        if (isRoot) {
            t.x = x;
            t.y = y;
        } else {
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
        registry.AddComponent(entity, RenderComponent{color, size, shapeType, zIndex});

        if (j.contains("sprite"))
        {
            std::string sp = j["sprite"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) {
                sp = (std::filesystem::path(ASSET_PATH) / p).string();
            }
            registry.AddComponent(entity, SpriteComponent(sp, size));
        }

        if (j.contains("tag"))
        {
            registry.AddComponent(entity, TagComponent{j["tag"].get<std::string>()});
        }

        if (j.contains("velocity"))
        {
            registry.AddComponent(entity, VelocityComponent{
                                      j["velocity"].value("dx", 0.f), j["velocity"].value("dy", 0.f)
                                  });
        }

        if (j.contains("script"))
        {
            std::string sp = j["script"].get<std::string>();
            std::filesystem::path p(sp);
            if (!p.is_absolute()) {
                sp = (std::filesystem::path(ASSET_PATH) / p).string();
            }
            auto &sc = registry.AddComponent(entity, ScriptComponent(LuaState::GetLua(), sp));
            sc.SetEntity(entity);

            if (j.contains("scriptProperties")) {
                for (auto it = j["scriptProperties"].begin(); it != j["scriptProperties"].end(); ++it) {
                    ScriptComponent::Property prop;
                    prop.name = it.key();
                    prop.type = static_cast<ScriptComponent::PropertyType>(it.value()["type"].get<int>());
                    if (prop.type == ScriptComponent::PropertyType::Int) prop.intVal = it.value()["value"].get<int>();
                    else if (prop.type == ScriptComponent::PropertyType::Float) prop.floatVal = it.value()["value"].get<float>();
                    else if (prop.type == ScriptComponent::PropertyType::Bool) prop.boolVal = it.value()["value"].get<bool>();
                    else if (prop.type == ScriptComponent::PropertyType::String || prop.type == ScriptComponent::PropertyType::Template)
                        prop.stringVal = it.value()["value"].get<std::string>();
                    
                    sc.SetExportedProperty(prop);
                }
            }
        }

        if (j.contains("camera") && j["camera"] == true) { registry.AddComponent(entity, CameraComponent{true}); }

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
        }

        registry.AddComponent(entity, HierarchyComponent{});
    }

    for (const auto& link : parentLinks) {
        if (link.second == "__EXTERNAL_PARENT__" && parent != 0) {
            HierarchySystem::SetParent(registry, link.first, parent, true);
        } else if (idToEntity.count(link.second)) {
            Entity parentEntity = idToEntity[link.second];
            auto &childH = registry.GetComponent<HierarchyComponent>(link.first);
            auto &parentH = registry.GetComponent<HierarchyComponent>(parentEntity);
            childH.parent = parentEntity;
            parentH.children.push_back(link.first);
        }
    }

    HierarchySystem::UpdateWorldTransforms(registry);

    // Call OnCreate() on all newly instantiated entities with ScriptComponent
    for (Entity e : createdEntities) {
        if (registry.HasComponent<ScriptComponent>(e)) {
            registry.GetComponent<ScriptComponent>(e).OnCreate();
        }
    }

    return rootEntity;
}
