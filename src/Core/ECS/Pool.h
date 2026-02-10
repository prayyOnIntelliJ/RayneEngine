#ifndef RAYNEENGINE_POOL_H
#define RAYNEENGINE_POOL_H
#include <unordered_map>
#include <vector>

#include "Entity.h"


struct IPool
{
    virtual ~IPool() = default;
};

template <typename T>
class Pool : public IPool
{
    std::vector<T> data;

    std::unordered_map<Entity, size_t> entityToIndex;
    std::unordered_map<size_t, Entity> indexToEntity;

    bool Has(const Entity e)
    {
        return entityToIndex.find(e) != entityToIndex.end();
    }

    void Add(const Entity e, T component)
    {
        if (Has(e)) return;

        const size_t index = data.size();
        data.push_back(component);
        entityToIndex[e] = index;
        indexToEntity[index] = e;
    }

    T& Get(const Entity e)
    {
        return data[entityToIndex[e]];
    }
};


#endif