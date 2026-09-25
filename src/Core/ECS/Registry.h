#ifndef RAYNEENGINE_REGISTRY_H
#define RAYNEENGINE_REGISTRY_H
#include <typeindex>
#include <unordered_map>

#include "Entity.h"
#include "Pool.h"
#include "View.h"

class Registry
{
private:
    Entity m_EntityCounter = 1;
    std::unordered_map<std::type_index, std::shared_ptr<IPool> > m_ComponentPools;
    std::vector<Entity> m_EntitiesToAdd;

public:
    Entity CreateEntity();

    Entity GetEntityCounter() const { return m_EntityCounter; }
    void SetEntityCounter(Entity counter) { m_EntityCounter = counter; }

    void DestroyEntity(Entity entity) { for (auto const &[type, pool]: m_ComponentPools) { pool->Remove(entity); } }

    void Clear() { for (auto const &[type, pool]: m_ComponentPools) { pool->Clear(); } }

    template<typename T>
    T &AddComponent(Entity entity, T component) { return GetPool<T>()->Add(entity, component); }

    template<typename T>
    T &GetComponent(Entity entity) { return GetPool<T>()->Get(entity); }

    template<typename T>
    bool HasComponent(Entity entity) { return GetPool<T>()->Has(entity); }

    template<typename T>
    void RemoveComponent(Entity entity) { GetPool<T>()->Remove(entity); }

    template<typename... Components>
    View<Components...> GetView()
    {
        return View<Components...>(
            std::make_tuple(GetPool<Components>().get()...),
            GetPool<std::tuple_element_t<0, std::tuple<Components...> > >()->entities);
    }

    template<typename... Components, typename Func>
    void ForEach(Func &&func)
    {
        auto view = GetView<Components...>();
        auto pools = std::make_tuple(GetPool<Components>().get()...);
        for (Entity entity : view)
            func(entity, std::get<Pool<Components>*>(pools)->Get(entity)...);
    }

private:
    template<typename T>
    std::shared_ptr<Pool<T>> GetPool()
    {
        const auto typeIndex = std::type_index(typeid(T));
        auto it = m_ComponentPools.find(typeIndex);
        if (it == m_ComponentPools.end())
        {
            auto pool = std::make_shared<Pool<T>>();
            m_ComponentPools[typeIndex] = pool;
            return pool;
        }
        return std::static_pointer_cast<Pool<T>>(it->second);
    }
};

inline Entity Registry::CreateEntity() { return m_EntityCounter++; }

#endif
