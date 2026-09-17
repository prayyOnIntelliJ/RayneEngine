#include "Core/Application/Application.h"
#include "Core/Scenes/ConsolePanel.h"

int main()
{
    ConsolePanel::InitRedirectors();
    
    Application application;
    
    application.Run();
}