#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Windows/RaytracerWindows.h"
#include "Realtime/RenderInterface.h"
#include "Realtime/PlatformApp.h"
#include "Realtime/Renderer.h"

// ----------------------------------------------------------------------------------------------------------------------------

using namespace yart;

// ----------------------------------------------------------------------------------------------------------------------------

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    Renderer renderer(1280, 720);
    return PlatformApp::Run(&renderer, hInstance, nCmdShow);
}