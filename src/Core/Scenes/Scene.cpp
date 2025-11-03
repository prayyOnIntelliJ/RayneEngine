#include "Scene.h"

#include <iostream>

Scene::Scene(std::string sceneName) : name(sceneName)
{
    std::cout << "Created Scene " << sceneName << std::endl;
}

Scene::~Scene() {}