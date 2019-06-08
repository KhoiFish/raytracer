// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

#include "Renderer.h"
#include "RealtimeEngine/RealtimeScene.h"
#include "RealtimeEngine/RenderDevice.h"
#include "RealtimeEngine/PlatformApp.h"
#include "RealtimeEngine/CommandList.h"
#include "RealtimeEngine/CommandContext.h"
#include <d3dcompiler.h>

using namespace Core;
using namespace RealtimeEngine;

#include "RaytracingPass.hpp"
#include "RasterPass.hpp"
#include "DenoisePass.hpp"
#include "HandleInput.hpp"
#include "HandleGui.hpp"

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::Renderer(uint32_t width, uint32_t height)
    : RenderInterface("Raytracer", width, height)
    , TheAppState(AppState_Loading)
    , TheRaytracer(nullptr)
    , TheWorldScene(nullptr)
    , TheRenderScene(nullptr)
    , TheLoadingThread(nullptr)
    , CurrentDeltaTime(0)
    , FrameCount(0)
    , SelectedBufferIndex(0)
    , AccumCount(0)
    , RendererDescriptorHeapCollection(4096 * 64, 0)
    , RendererDescriptorHeap(nullptr)
    , IsCameraDirty(true)
    , LoadSceneRequested(false)
    , RasterDescriptorIndexStart(0)
    , RaytracingGlobalSigDataIndexStart(0)
    , RaytracingLocalSigDataIndexStart(0)
{
    BackbufferFormat                             = DXGI_FORMAT_R10G10B10A2_UNORM;
    CPURaytracerTexType                          = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DirectIndirectRTBufferType                   = DXGI_FORMAT_R32G32B32A32_FLOAT;
    MomentsBufferType                            = DXGI_FORMAT_R32G32B32A32_FLOAT;
    HistoryBufferType                            = DXGI_FORMAT_R16_FLOAT;
    GBufferRTTypes[GBufferType_Position]         = DXGI_FORMAT_R32G32B32A32_FLOAT;
    GBufferRTTypes[GBufferType_Normal]           = DXGI_FORMAT_R16G16B16A16_FLOAT;
    GBufferRTTypes[GBufferType_TexCoordAndDepth] = DXGI_FORMAT_R8G8B8A8_UNORM;
    GBufferRTTypes[GBufferType_Albedo]           = DXGI_FORMAT_R16G16B16A16_FLOAT;
    GBufferRTTypes[GBufferType_CurrSVGFLinearZ]  = DXGI_FORMAT_R32G32B32A32_FLOAT;
    GBufferRTTypes[GBufferType_PrevSVGFLinearZ]  = DXGI_FORMAT_R32G32B32A32_FLOAT;
    GBufferRTTypes[GBufferType_SVGFMoVec]        = DXGI_FORMAT_R32G32B32A32_FLOAT;
    GBufferRTTypes[GBufferType_SVGFCompact]      = DXGI_FORMAT_R32G32B32A32_FLOAT;
}

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::~Renderer()
{
    CommandListManager::Get().IdleGPU();

    CleanupLoadingThread();
    CleanupGpuRaytracerPass();
    CleanupRasterPass();
    CleanupDenoisePass();

    if (TheRaytracer != nullptr)
    {
        delete TheRaytracer;
    }

    if (TheWorldScene != nullptr)
    {
        delete TheWorldScene;
        TheWorldScene = nullptr;
    }

    if (TheRenderScene != nullptr)
    {
        delete TheRenderScene;
        TheRenderScene = nullptr;
    }

    RenderDevice::Shutdown();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDeviceLost()
{
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDeviceRestored()
{
    OnResizeCpuRaytracer();
    SetupRenderBuffers();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnInit()
{
    // Init random generator
    {
        auto timeInMillisec = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now());
        RandGen = std::mt19937(uint32_t(timeInMillisec.time_since_epoch().count()));
    }

    // Get the number of cores on this system
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    MaxNumCpuThreads = TheUserInputData.CpuNumThreads = sysInfo.dwNumberOfProcessors;

    // Init the render device
    RenderDevice::Initialize(Width, Height, this, &RendererDescriptorHeapCollection, BackbufferFormat);
    RendererDescriptorHeap = &RendererDescriptorHeapCollection.Get(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Render setup
    SetupGui();
    SetupRenderBuffers();
    SetupRasterRootSignatures();
    SetupGpuRaytracingRootSignatures();
    SetupDenoisePipeline();

    // Load the scene data
    StartSceneLoad();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRenderBuffers()
{
    uint32_t width  = RenderDevice::Get().GetBackbufferWidth();
    uint32_t height = RenderDevice::Get().GetBackbufferHeight();

    for (int i = 0; i < GBufferType_Num; i++)
    {
        GBuffers[i].Destroy();
        GBuffers[i].Create(GBufferTypeStrings[i], width, height, 1, GBufferRTTypes[i]);
    }

    CPURaytracerTex.Destroy();
    CPURaytracerTex.Create("CpuRaytracerTex", width, height, 1, CPURaytracerTexType);

    for (int i = 0; i < DirectIndirectBufferType_Num; i++)
    {
        DirectLightingBuffer[i].Destroy();
        IndirectLightingBuffer[i].Destroy();

        DirectLightingBuffer[i].CreateEx(DirectIndirectBufferTypeStrings[i], width, height, 1, DirectIndirectRTBufferType, nullptr, D3D12_HEAP_TYPE_DEFAULT, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
        IndirectLightingBuffer[i].CreateEx(DirectIndirectBufferTypeStrings[i], width, height, 1, DirectIndirectRTBufferType, nullptr, D3D12_HEAP_TYPE_DEFAULT, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
    }

    for (int i = 0; i < 2; i++)
    {
        ReprojectionBuffers[i][ReprojBufferType_Direct].Destroy();
        ReprojectionBuffers[i][ReprojBufferType_Indirect].Destroy();
        ReprojectionBuffers[i][ReprojBufferType_Moments].Destroy();
        ReprojectionBuffers[i][ReprojBufferType_History].Destroy();

        ReprojectionBuffers[i][ReprojBufferType_Direct].CreateEx(ReprojBufferTypeStrings[ReprojBufferType_Direct], width, height, 1, DirectIndirectRTBufferType, nullptr, D3D12_HEAP_TYPE_DEFAULT, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
        ReprojectionBuffers[i][ReprojBufferType_Indirect].CreateEx(ReprojBufferTypeStrings[ReprojBufferType_Indirect], width, height, 1, DirectIndirectRTBufferType, nullptr, D3D12_HEAP_TYPE_DEFAULT, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
        ReprojectionBuffers[i][ReprojBufferType_Moments].CreateEx(ReprojBufferTypeStrings[ReprojBufferType_Moments], width, height, 1, MomentsBufferType, nullptr, D3D12_HEAP_TYPE_DEFAULT, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
        ReprojectionBuffers[i][ReprojBufferType_History].CreateEx(ReprojBufferTypeStrings[ReprojBufferType_History], width, height, 1, HistoryBufferType, nullptr, D3D12_HEAP_TYPE_DEFAULT, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
    }

    for (int i = 0; i < 2; i++)
    {
        DenoiseDirectOutputBuffer[i].Destroy();
        DenoiseIndirectOutputBuffer[i].Destroy();

        DenoiseDirectOutputBuffer[i].CreateEx("Denoise Direct Output Buffer", width, height, 1, DirectIndirectRTBufferType, nullptr, D3D12_HEAP_TYPE_DEFAULT, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
        DenoiseIndirectOutputBuffer[i].CreateEx("Denoise Indirect Output Buffer", width, height, 1, DirectIndirectRTBufferType, nullptr, D3D12_HEAP_TYPE_DEFAULT, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::StartSceneLoad()
{
    // Change app state and kick off the load
    CleanupLoadingThread();

    //RenderDevice::Get().ResetDescriptors();
    TheAppState      = AppState_Loading;
    TheLoadingThread = new std::thread(LoadingThread, this);
    UpdateWindowTitle();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::LoadingThread(Renderer* pRenderer)
{
    // Clean up previous scene
    if (pRenderer->TheWorldScene != nullptr)
    {
        delete pRenderer->TheWorldScene;
        pRenderer->TheWorldScene = nullptr;
    }

    if (pRenderer->TheRenderScene != nullptr)
    {
        delete pRenderer->TheRenderScene;
        pRenderer->TheRenderScene = nullptr;
    }

    // Load the selected scene
    pRenderer->TheWorldScene = GetSampleScene(SampleScene(pRenderer->TheUserInputData.SampleScene));
    pRenderer->TheWorldScene->GetCamera().SetAspect((float)RenderDevice::Get().GetBackbufferWidth() / (float)RenderDevice::Get().GetBackbufferHeight());
    pRenderer->TheWorldScene->GetCamera().SetFocusDistanceToLookAt();
    pRenderer->TheUserInputData.VertFov = pRenderer->TheWorldScene->GetCamera().GetVertFov();
    pRenderer->TheRenderScene           = new RealtimeEngine::RealtimeScene(pRenderer->TheWorldScene, pRenderer->GetNumberHitPrograms());
    
    // Setup the rest of the pipeline
    pRenderer->TheRenderScene->SetupResourceViews(*pRenderer->RendererDescriptorHeap);
    pRenderer->SetupRasterDescriptors();
    pRenderer->SetupGpuRaytracingDescriptors();
    pRenderer->SetupGpuRaytracingPSO();
    pRenderer->SetupDenoiseDescriptors();
    pRenderer->SetCameraDirty();

    // Done loading
    pRenderer->TheAppState = AppState_RenderScene;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::CleanupLoadingThread()
{
    // Kill loading thread
    if (TheLoadingThread != nullptr)
    {
        TheLoadingThread->join();
        delete TheLoadingThread;
        TheLoadingThread = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::UpdateWindowTitle()
{
    std::string newTitle =
        std::string("Raytracer -- ") +
        std::string(SampleSceneNames[TheUserInputData.SampleScene]) + std::string("  [") +
        GetSelectedBufferName() + std::string("] ");

    PlatformApp::SetWindowTitle(newTitle);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeCpuRaytracer(bool startRaytrace)
{
    // Don't resize if there's no cpu raytracer active
    if (!startRaytrace && TheRaytracer == nullptr)
    {
        return;
    }

    int backbufferWidth  = RenderDevice::Get().GetBackbufferWidth();
    int backbufferHeight = RenderDevice::Get().GetBackbufferHeight();

    // Delete old ray tracer
    bool isTracing = false;
    if (TheRaytracer != nullptr)
    {
        isTracing = TheRaytracer->IsTracing();
        delete TheRaytracer;
        TheRaytracer = nullptr;
    }

    // Create the ray tracer
    TheRaytracer = new Raytracer(backbufferWidth, backbufferHeight, TheUserInputData.CpuNumSamplesPerRay, TheUserInputData.CpuMaxScatterDepth, TheUserInputData.CpuNumThreads, true);

    if (isTracing || startRaytrace)
    {
        TheRaytracer->BeginRaytrace(TheWorldScene, OnCpuRaytraceComplete);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool Renderer::OnSizeChanged(uint32_t width, uint32_t height, bool minimized)
{
    // Bail during scene loads, this size request will get re-queued
    if (TheAppState == AppState_Loading && TheLoadingThread != nullptr)
    {
        return false;
    }

    if (!RenderDevice::Get().WindowSizeChanged(width, height, minimized))
    {
        return true;
    }

    // Reset the aspect ratio on the scene camera
    if (TheWorldScene != nullptr)
    {
        TheWorldScene->GetCamera().SetAspect((float)width / (float)height);
    }

    TheRenderScene->SetupResourceViews(*RendererDescriptorHeap);
    OnResizeCpuRaytracer();
    SetupRenderBuffers();
    SetupRasterDescriptors();
    SetupGpuRaytracingDescriptors();
    SetupGpuRaytracingPSO();
    SetupDenoiseDescriptors();
    SetCameraDirty();

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDestroy()
{
    // Let GPU finish before releasing D3D resources
    CommandListManager::Get().IdleGPU();
    OnDeviceLost();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetEnableCpuRaytrace(bool enable)
{
    if (enable)
    {
        OnResizeCpuRaytracer(true);
        SelectedBufferIndex = CpuResultsBufferIndex;
    }
    else
    {
        delete TheRaytracer;
        TheRaytracer = nullptr;

        SelectedBufferIndex = 0;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetCameraDirty()
{
    IsCameraDirty = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::ToggleCpuRaytracer()
{
    SetEnableCpuRaytrace(TheRaytracer == nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnCpuRaytraceComplete(Raytracer* tracer, bool actuallyFinished)
{
    if (actuallyFinished)
    {
        WriteImageAndLog(tracer, "RaytracerWindows");
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnUpdate(float dtSeconds)
{
    CurrentDeltaTime = dtSeconds;

    // Handle new scene load requests
    if (LoadSceneRequested)
    {
        StartSceneLoad();
        LoadSceneRequested = false;
        return;
    }

    // Handle camera updates
    if (TheAppState == AppState_RenderScene)
    {
        // Temp code to test animation
        if (false)
        {
            static float                timeAccum      = 0;
            int                         index          = 6;
            float                       posY           = 50.0f + (cosf(timeAccum) * 100.0f);
            static DirectX::XMMATRIX    origMatrix     = TheRenderScene->GetRenderSceneList()[index]->WorldMatrix;
            DirectX::XMMATRIX           newWorldMatrix = XMMatrixTranslation(0, posY, 0) * origMatrix;

            TheRenderScene->GetRenderSceneList()[index]->WorldMatrix = newWorldMatrix;
            timeAccum     += (dtSeconds * 4);
            IsCameraDirty  = true;
        }

        bool  mouseLButton   =  TheUserInputData.LeftMouseButtonPressed;
        float scale          = (TheUserInputData.ShiftKeyPressed ? 64.f : 32.0f) * 16.f * dtSeconds;
        float forwardAmount  = (TheUserInputData.Forward - TheUserInputData.Backward) * scale;
        float strafeAmount   = (TheUserInputData.Left    - TheUserInputData.Right)    * scale;
        float upDownAmount   = (TheUserInputData.Up      - TheUserInputData.Down)     * scale;

        // Camera needs update
        if (mouseLButton || IsCameraDirty || !CompareFloatEqual(forwardAmount, 0) || !CompareFloatEqual(strafeAmount, 0) || !CompareFloatEqual(upDownAmount, 0))
        {
            TheRenderScene->UpdateCamera(TheUserInputData.GpuNearPlane, TheUserInputData.GpuFarPlane, TheUserInputData.VertFov, forwardAmount, strafeAmount, upDownAmount, TheUserInputData.MouseDx, TheUserInputData.MouseDy, TheWorldScene->GetCamera());
            TheUserInputData.MouseDx = 0;
            TheUserInputData.MouseDy = 0;

            // Restart raytrace
            if (TheRaytracer != nullptr && TheRaytracer->IsTracing())
            {
                TheRaytracer->RestartCurrentRaytrace();
            }

            IsCameraDirty = false;
            AccumCount = 0;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnRender()
{
    if (TheAppState == AppState_RenderScene)
    {
        // Allow multiple temporal accumulation per frame
        for (int i = 0; i < TheUserInputData.GpuNumAccumPasses; i++)
        {
            RenderGeometryPass();
            RenderGpuRaytracing();
            FrameCount++;
        }

        // Denoise and composite
        RenderDenoisePass();
        RenderCompositePass();
    }

    RenderGui();

    // Present
    RenderDevice::Get().Present();
}
