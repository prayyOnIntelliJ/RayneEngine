#include "LuaState.h"

#include <fstream>

#include "../ECS/Components.h"
#include "../ECS/Registry.h"
#include "../Math/MathR.h"

sol::state LuaState::s_Lua;
std::vector<LuaApiDoc> s_ApiDocs;

void LuaState::Init(Registry& registry)
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

    s_Lua.set_function("AddTransform", [&](const Entity e, const float x, const float y)
    {
    registry.AddComponent(e, TransformComponent{x, y});
    });

    s_Lua.set_function("AddVelocity", [&](const Entity e, const float dx, const float dy)
        {
        registry.AddComponent(e, VelocityComponent{dx, dy});
    });

    s_Lua.set_function("GetTransform", [&](const Entity e) -> TransformComponent*
    {
        return &registry.GetComponent<TransformComponent>(e);
    });

    s_Lua.set_function("CreateEntity", [&]()
    {
        return registry.CreateEntity();
    });

    s_Lua.set_function("DestroyEntity", [&](const Entity e)
    {
        return registry.DestroyEntity(e);
    });

    s_Lua.set_function("SetPosition", [&](const Entity e, const float x, const float y)
    {
        if(registry.HasComponent<TransformComponent>(e))
        {
            auto& t = registry.GetComponent<TransformComponent>(e);
            t.x = x;
            t.y = y;
        }
    });

    std::cout << "Initialized Lua with Functions\n";
}

sol::state& LuaState::GetLua()
{
    return s_Lua;
}

void LuaState::RegisterStatics()
{
    MathR::RegisterLua(s_Lua);
}


void LuaState::GenerateApiStub(const std::string &path)
{
    std::ofstream file(path);

    file << "-- AUTO GENERATED --\n";
    file << "-- Generated at the Engine-Start\n";

    for (const auto& doc : s_ApiDocs)
    {
        file << "--- " << doc.description << "\n";

        for (const auto& [pName, pType] : doc.params)
            file << "---@param " << pName << " " << pType << "\n";

        file << "---@return " << doc.returnType << "\n";

        file << "function " << doc.name << "(";
        for (size_t i = 0; i < doc.params.size(); i++)
        {
            if (i > 0) file << ", ";
            file << doc.params[i].first;
        }
        file << ") end\n\n";
    }

    std::cout << "Generated API Stub: " << path << " (" << s_ApiDocs.size() << " Functions)\n";
}