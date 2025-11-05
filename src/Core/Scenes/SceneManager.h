#ifndef RAYNEENGINE_SCENEMANAGER_H
#define RAYNEENGINE_SCENEMANAGER_H

#include <memory>
#include <vector>

#include "Scene.h"


class SceneManager
{
public:
    SceneManager();

    Scene* GetCurrentScene() const;
    Scene* GetSceneByName(std::string sceneName) const;

    void AddScene(std::unique_ptr<Scene> newScene);

    void SetSceneByName(const std::string &sceneName);
    void SetSceneByReference(Scene* newScene);
    void SetSceneByIndex(int index);
private:
    std::vector<std::unique_ptr<Scene>> scenes;
    Scene* currentScene = nullptr;
};


#endif