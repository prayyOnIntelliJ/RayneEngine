#include "SceneSerializer.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../ECS/Components.h"
#include "../Scripting/ScriptComponent.h"
#include "../Scripting/LuaState.h"

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
        registry.AddComponent(entity, TransformComponent{j["x"], j["y"]});
        
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
            registry.AddComponent(entity, SpriteComponent(j["sprite"].get<std::string>(), size));
        }

        if (j.contains("velocity"))
        {
            registry.AddComponent(entity, VelocityComponent{
                                        j["velocity"]["dx"], j["velocity"]["dy"]
                                    });
        }

        if (j.contains("script"))
        {
            const std::string sp = j["script"];
            auto &sc = registry.AddComponent(entity, ScriptComponent(LuaState::GetLua(), sp));
            sc.SetEntity(entity);
        }

        if (j.contains("camera") && j["camera"] == true)
        {
            registry.AddComponent(entity, CameraComponent{true});
        }

        if (j.contains("collision"))
        {
            registry.AddComponent(entity, CollisionComponent{j["collision"]["channel"]});
        }
    }
}
