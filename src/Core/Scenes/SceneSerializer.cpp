#include "SceneSerializer.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../ECS/Components.h"
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

    json data = json::parse(file);
    for (auto &j: data["objects"])
    {
        Entity entity = registry.CreateEntity();
        
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

        registry.AddComponent(entity, RenderComponent{color, size, shapeType});

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
        }

        if (j.contains("camera") && j["camera"] == true) { registry.AddComponent(entity, CameraComponent{true}); }

        if (j.contains("collision")) 
        { 
            CollisionType cType = CollisionType::Static;
            if (j["collision"].contains("type") && j["collision"]["type"] == "solid")
                cType = CollisionType::Solid;
            registry.AddComponent(entity, CollisionComponent{j["collision"]["channel"], cType}); 
        }
    }
}
