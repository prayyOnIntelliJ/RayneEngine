#ifndef SCENE_H
#define SCENE_H
#include <string>


class Scene {
public:
    Scene(std::string sceneName);
    ~Scene();

    std::string name;
};



#endif