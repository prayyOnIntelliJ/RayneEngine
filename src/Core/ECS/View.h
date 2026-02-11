#ifndef RAYNEENGINE_VIEW_H
#define RAYNEENGINE_VIEW_H
#include <tuple>

#include "Pool.h"

template <typename... Components>
class View
{
    std::tuple<Pool<Components>*...> m_Pools;
    const std::vector<Entity>& m_FirstPoolEntities;

public:
    View(std::tuple<Pool<Components>*...> pools, const std::vector<Entity>& firstPoolEntities)
        : m_Pools(pools), m_FirstPoolEntities(firstPoolEntities) { }

    struct Iterator
    {
        const std::vector<Entity>& entities;
        size_t index;
        std::tuple<Pool<Components>*...> pools;

        Iterator(const std::vector<Entity>& e, size_t i, std::tuple<Pool<Components>*...> p)
            : entities(e), index(i), pools(p)
        {
            CheckValid();
        }

        void CheckValid()
        {
            while (index < entities.size() && !IsValid(entities[index]))
            {
                index++;
            }
        }

        bool IsValid(Entity e)
        {
            return (std::get<Pool<Components>*>(pools)->Has(e) && ...);
        }

        Entity operator*() const { return entities[index]; }

        Iterator& operator++()
        {
            index++;
            CheckValid();
            return *this;
        }

        bool operator!=(const Iterator& other) const
        {
            return index != other.index;
        }
    };

    Iterator begin() { return Iterator(m_FirstPoolEntities, 0, m_Pools); }
    Iterator end() { return Iterator(m_FirstPoolEntities, m_FirstPoolEntities.size(), m_Pools); }
};


#endif