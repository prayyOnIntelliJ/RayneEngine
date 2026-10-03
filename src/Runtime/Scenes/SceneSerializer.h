#ifndef RAYNEENGINE_SCENESERIALIZER_H
#define RAYNEENGINE_SCENESERIALIZER_H

#include <string>
#include "../ECS/Registry.h"

class SceneSerializer
{
public:
    static void LoadIntoRegistry(Registry &registry, const std::string &path);

    static Entity InstantiateTemplate(Registry &registry, const std::string &templatePath, float x, float y,
                                      Entity parent = 0);
};

#endif
