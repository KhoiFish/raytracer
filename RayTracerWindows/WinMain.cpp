#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Windows/RaytracerWindows.h"

#include <Application.h>
#include <dxgidebug.h>
#include <shellapi.h>
#include <Shlwapi.h>

// ----------------------------------------------------------------------------------------------------------------------------

void ReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}

// ----------------------------------------------------------------------------------------------------------------------------

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    int     retCode = 0;
    int     argc    = 0;
    LPWSTR* argv    = CommandLineToArgvW(lpCmdLine, &argc);
    
    if (argv)
    {
        WCHAR path[MAX_PATH];
        for (int i = 0; i < argc; ++i)
        {
            // -wd Specify the Working Directory.
            if (wcscmp(argv[i], L"-wd") == 0)
            {
                wcscpy_s(path, argv[++i]);
                SetCurrentDirectoryW(path);
            }
        }
        LocalFree(argv);
    }

    Application::Create(hInstance);
    {
        std::shared_ptr<RaytracerWindows> demo = std::make_shared<RaytracerWindows>(L"Raytracer", 1024, 1024);
        retCode = Application::Get().Run(demo);
    }
    Application::Destroy();

    atexit(&ReportLiveObjects);

    return retCode;
}