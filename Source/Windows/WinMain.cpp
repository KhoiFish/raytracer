#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Renderer.h"
#include "RealtimeEngine/RenderInterface.h"
#include "RealtimeEngine/PlatformApp.h"


// ----------------------------------------------------------------------------------------------------------------------------

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    yart::Renderer renderer(1280, 720);
    return yart::PlatformApp::Run(&renderer, hInstance, nCmdShow);
}