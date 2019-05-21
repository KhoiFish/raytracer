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

#include "RaytracingSetup.hpp"
#include "RealtimeRender.hpp"
#include "HandleInput.hpp"
#include "HandleGui.hpp"

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::Renderer(uint32_t width, uint32_t height)
    : RenderInterface("Raytracer", width, height)
    , TheRaytracer(nullptr)
    , TheWorldScene(nullptr)
    , TheRenderScene(nullptr)
    , FrameCount(0)
    , SelectedBufferIndex(0)
    , AccumCount(0)
    , RealtimeDescriptorHeap(nullptr)
    , RaytracingDescriptorHeap(nullptr)
    , IsCameraDirty(true)
    , LoadSceneRequested(false)
    , HitProgramCount(0)
    , LocalSigDataIndexStart(0)
{
    BackbufferFormat                                            = DXGI_FORMAT_R8G8B8A8_UNORM;
    CPURaytracerTexType                                         = DXGI_FORMAT_R8G8B8A8_UNORM;
    RaytracingBufferType                                        = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DeferredBuffersRTTypes[DeferredBufferType_Position]         = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DeferredBuffersRTTypes[DeferredBufferType_Normal]           = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DeferredBuffersRTTypes[DeferredBufferType_TexCoordAndDepth] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DeferredBuffersRTTypes[DeferredBufferType_Albedo]           = DXGI_FORMAT_R32G32B32A32_FLOAT;
}

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::~Renderer()
{
    CommandListManager::Get().IdleGPU();

    CleanupGpuRaytracer();
    CleanupRealtimeRender();

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
    ;
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
    // Get the number of cores on this system
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    MaxNumCpuThreads = UserInput.CpuNumThreads = sysInfo.dwNumberOfProcessors;

    // Init the render device
    RenderDevice::Initialize(PlatformApp::GetHwnd(), Width, Height, this, BackbufferFormat);

    // Setup render pipelines
    SetupRenderBuffers();
    SetupRealtimePipeline();

    // Load the scene data
    LoadScene();

    // Raytracing setup last (scene data needs to be loaded first)
    SetupRealtimeRaytracingPipeline();

    SetupGui();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRenderBuffers()
{
    uint32_t width  = RenderDevice::Get().GetBackbufferWidth();
    uint32_t height = RenderDevice::Get().GetBackbufferHeight();

    for (int i = 0; i < DeferredBufferType_Num; i++)
    {
        DeferredBuffers[i].Destroy();
        DeferredBuffers[i].Create(DeferredBufferTypeStrings[i], width, height, 1, DeferredBuffersRTTypes[i]);
    }

    CPURaytracerTex.Destroy();
    CPURaytracerTex.Create("CpuRaytracerTex", width, height, 1, CPURaytracerTexType);

    for (int i = 0; i < 2; i++)
    {
        LambertAndAOBuffer[i].Destroy();
        LambertAndAOBuffer[i].CreateEx("AO Buffer", width, height, 1, RaytracingBufferType, nullptr, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::LoadScene()
{
    CommandListManager::Get().IdleGPU();

    // Clean up previous scene
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

    // Load the selected scene
    TheWorldScene = GetSampleScene(SampleScene(UserInput.SampleScene));
    TheWorldScene->GetCamera().SetAspect((float)RenderDevice::Get().GetBackbufferWidth() / (float)RenderDevice::Get().GetBackbufferHeight());
    TheWorldScene->GetCamera().SetFocusDistanceToLookAt();
    
    TheRenderScene    = new RealtimeEngine::RealtimeScene(TheWorldScene);
    UserInput.VertFov = TheWorldScene->GetCamera().GetVertFov();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeCpuRaytracer()
{
    int backbufferWidth  = RenderDevice::Get().GetBackbufferWidth();
    int backbufferHeight = RenderDevice::Get().GetBackbufferHeight();

    if (TheRaytracer != nullptr)
    {
        delete TheRaytracer;
        TheRaytracer = nullptr;
    }

    // Create the ray tracer
    TheRaytracer = new Raytracer(backbufferWidth, backbufferHeight, UserInput.CpuNumSamplesPerRay, UserInput.CpuMaxScatterDepth, UserInput.CpuNumThreads, true);

    // Reset the aspect ratio on the scene camera
    if (TheWorldScene != nullptr)
    {
        TheWorldScene->GetCamera().SetAspect((float)backbufferWidth / (float)backbufferHeight);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnSizeChanged(uint32_t width, uint32_t height, bool minimized)
{
    if (!RenderDevice::Get().WindowSizeChanged(width, height, minimized))
    {
        return;
    }

    CommandListManager::Get().IdleGPU();
    SetupRenderBuffers();
    OnResizeCpuRaytracer();
    OnResizeGpuRaytracer();
    OnResizeRealtimeRenderer();
    SetCameraDirty();
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
        OnResizeCpuRaytracer();
        TheRaytracer->BeginRaytrace(TheWorldScene, OnCpuRaytraceComplete);
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
    // Handle new scene load requests
    if (LoadSceneRequested)
    {
        LoadScene();
        OnResizeGpuRaytracer();
        SetCameraDirty();
        LoadSceneRequested = false;
    }

    // Handle camera updates
    {
        float scale          = (UserInput.ShiftKeyPressed ? 64.f : 32.0f) * 16.f * dtSeconds;
        float forwardAmount  = (UserInput.Forward - UserInput.Backward) * scale;
        float strafeAmount   = (UserInput.Left    - UserInput.Right)    * scale;
        float upDownAmount   = (UserInput.Up      - UserInput.Down)     * scale;

        // Camera needs update
        if (IsCameraDirty || !CompareFloatEqual(forwardAmount, 0) || !CompareFloatEqual(strafeAmount, 0) || !CompareFloatEqual(upDownAmount, 0))
        {
            TheRenderScene->UpdateCamera(UserInput.VertFov, forwardAmount, strafeAmount, upDownAmount, UserInput.MouseDx, UserInput.MouseDy, TheWorldScene->GetCamera());
            UserInput.MouseDx = 0;
            UserInput.MouseDy = 0;

            // Restart raytrace
            if (TheRaytracer != nullptr && SelectedBufferIndex == CpuResultsBufferIndex)
            {
                TheRaytracer->BeginRaytrace(TheWorldScene, OnCpuRaytraceComplete);
            }

            IsCameraDirty = false;
            AccumCount = 0;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnRender()
{
    // Render passes
    RenderGeometryPass();
    RenderGpuRaytracing();
    RenderCompositePass();
    RenderGui();

    // Present
    RenderDevice::Get().Present();
    FrameCount++;
}
