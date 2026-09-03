#ifndef RAYNEENGINE_SCENESERIALIZER_H
#define RAYNEENGINE_SCENESERIALIZER_H

#include <string>
#include "../ECS/Registry.h"

class SceneSerializer
{
public:
    static void LoadIntoRegistry(Registry &registry, const std::string &path);
};

#endif
