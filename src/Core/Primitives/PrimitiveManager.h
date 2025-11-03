#ifndef PRIMITIVEMANAGER_H
#define PRIMITIVEMANAGER_H
#include <memory>
#include <vector>

#include "Primitive.h"


class PrimitiveManager {
public:
    std::vector<std::unique_ptr<Primitive>> primitives;

    template<typename T, typename ...Args>
    T* Create(Args&&... args)
    {
        auto primitive = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = primitive.get();
        primitives.push_back(std::move(primitive));
        return ptr;
    }

    void Update(float deltaTime)
    {
        for (auto& primitive : primitives)
        {
            primitive->Update(deltaTime);
        }
    }

    void Draw(sf::RenderWindow& window)
    {
        for (auto& primitive : primitives)
        {
            primitive->Draw(window);
        }
    }
};

#endif