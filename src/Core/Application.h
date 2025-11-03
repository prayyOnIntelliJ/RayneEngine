#ifndef RAYNEENGINE_APPLICATION_H
#define RAYNEENGINE_APPLICATION_H
#include "SFML/Graphics/RenderWindow.hpp"

class Application
{
public:
    Application();

    void CreateWindow();
    void Run();
    void SetIcon();
private:
    sf::RenderWindow renderWindow;

    void Update(float deltaTime);
    void Render();

    sf::Clock deltaTimeClock;
};


#endif