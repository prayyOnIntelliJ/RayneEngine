#ifndef RAYNEENGINE_REGISTRY_H
#define RAYNEENGINE_REGISTRY_H
#include <typeindex>
#include <unordered_map>

#include "Entity.h"
#include "Pool.h"


class Registry
{
private:
    Entity m_EntityCounter = 1;
    std::unordered_map<std::type_index, std::shared_ptr<IPool>> m_ComponentPools;
    std::vector<Entity> m_EntitiesToAdd;

public:
    Entity CreateEntity();

    template <typename T>
    void AddComponent(Entity entity, T component)
    {
        GetPool<T>()->Add(entity, component);
    }

    template <typename T>
    T& GetComponent(Entity entity)
    {
        return GetPool<T>()->Get(entity);
    }

    template <typename T>
    bool HasComponent(Entity entity)
    {
        return GetPool<T>()->Has(entity);
    }

private:
    template <typename T>
    std::shared_ptr<Pool<T>> GetPool()
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        if (m_ComponentPools.find(typeIndex) == m_ComponentPools.end())
        {
            m_ComponentPools[typeIndex] = std::make_shared<Pool<T>>();
        }

        return std::static_pointer_cast<Pool<T>>(m_ComponentPools[typeIndex]);
    }
};


#endif