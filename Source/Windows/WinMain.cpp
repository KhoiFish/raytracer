#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Realtime/Renderer.h"
#include "Realtime/RenderInterface.h"
#include "Realtime/PlatformApp.h"

// ----------------------------------------------------------------------------------------------------------------------------

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    yart::Renderer renderer(1280, 720);
    return yart::PlatformApp::Run(&renderer, hInstance, nCmdShow);
}