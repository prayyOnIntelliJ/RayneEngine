#ifndef RAYNEENGINE_POOL_H
#define RAYNEENGINE_POOL_H
#include <unordered_map>
#include <vector>

#include "Entity.h"
#include "../Scripting/LuaState.h"

struct IPool
{
    virtual ~IPool() = default;

    virtual bool Has(Entity e) = 0;

    virtual void Remove(Entity e) = 0;

    virtual void Clear() = 0;
};

template<typename T>
class Pool : public IPool
{
public:
    std::vector<T> data;
    std::vector<Entity> entities;

    std::unordered_map<Entity, size_t> entityToIndex;

    bool Has(const Entity e) override { return entityToIndex.find(e) != entityToIndex.end(); }

    T &Add(const Entity e, T component)
    {
        if (Has(e)) return Get(e);

        entityToIndex[e] = data.size();
        data.push_back(component);
        entities.push_back(e);

        return data.back();
    }

    void Remove(Entity e) override
    {
        if (!Has(e)) return;

        size_t indexToRemove = entityToIndex[e];
        size_t lastIndex = data.size() - 1;

        data[indexToRemove] = data[lastIndex];
        entities[indexToRemove] = entities[lastIndex];

        const Entity entityThatMoved = entities[lastIndex];
        entityToIndex[entityThatMoved] = indexToRemove;

        data.pop_back();
        entities.pop_back();
        entityToIndex.erase(e);
    }

    T &Get(const Entity e) { return data[entityToIndex[e]]; }

    void Clear() override
    {
        data.clear();
        entities.clear();
        entityToIndex.clear();
    }
};

#endif
