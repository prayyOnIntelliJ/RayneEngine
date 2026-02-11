#include "Scene.h"

#include <iostream>

Scene::Scene(std::string sceneName) : m_Name(sceneName)
{
    std::cout << "Created Scene " << sceneName << std::endl;
}

Scene::~Scene() {}