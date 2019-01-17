#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Core/Raytracer.h"
#include "Core/IHitable.h"
#include "SampleScenes.h"
#include "Windows/RaytracerWindows.h"

#include <Application.h>
#include <dxgidebug.h>
#include <shellapi.h>
#include <Shlwapi.h>

// ----------------------------------------------------------------------------------------------------------------------------

static int          sOutputWidth = 512;
static int          sOutputHeight = 512;
static float        sAspect = float(sOutputWidth) / float(sOutputHeight);
static int          sNumSamplesPerRay = 100;
static int          sMaxScatterDepth = 50;
static int          sNumThreads = 8;

static Raytracer*   sRaytracer = nullptr;
static SampleScene  sSampleSceneSelected = SceneRandom;
static IHitable*    sScenes[MaxScene];
static Camera       sSampleCameras[MaxScene];

// ----------------------------------------------------------------------------------------------------------------------------

static const int   MAX_LOADSTRING = 100;
static HINSTANCE   hInst;
static WCHAR       szTitle[MAX_LOADSTRING];
static WCHAR       szWindowClass[MAX_LOADSTRING];
static HBITMAP     hBitmap;

// ----------------------------------------------------------------------------------------------------------------------------

static ATOM                myRegisterClass(HINSTANCE hInstance);
static BOOL                initInstance(HINSTANCE, int);
static LRESULT CALLBACK    wndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK    aboutBox(HWND, UINT, WPARAM, LPARAM);

// ----------------------------------------------------------------------------------------------------------------------------

static void InitalizeSampleScenes()
{
    sSampleCameras[SceneRandom]         = GetCameraForSample(SceneRandom, sAspect);
    sSampleCameras[SceneCornell]        = GetCameraForSample(SceneCornell, sAspect);
    sSampleCameras[SceneCornellSmoke]   = GetCameraForSample(SceneCornellSmoke, sAspect);
    sSampleCameras[SceneFinal]          = GetCameraForSample(SceneFinal, sAspect);

    sScenes[SceneRandom]                = SampleSceneRandom(sSampleCameras[SceneRandom]);
    sScenes[SceneCornell]               = SampleSceneRandom(sSampleCameras[SceneCornell]);
    sScenes[SceneCornellSmoke]          = SampleSceneRandom(sSampleCameras[SceneCornellSmoke]);
    sScenes[SceneFinal]                 = SampleSceneRandom(sSampleCameras[SceneFinal]);
}

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
    int retCode = 0;

    WCHAR path[MAX_PATH];

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
    if (argv)
    {
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
        std::shared_ptr<RaytracerWindows> demo = std::make_shared<RaytracerWindows>(L"Learning DirectX 12 - Lesson 3", 1280, 720);
        retCode = Application::Get().Run(demo);
    }
    Application::Destroy();

    atexit(&ReportLiveObjects);

    return retCode;
}