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
#include "RealtimeEngine/RenderScene.h"
#include "RealtimeEngine/RenderDevice.h"
#include "RealtimeEngine/PlatformApp.h"
#include "RealtimeEngine/CommandList.h"
#include "RealtimeEngine/CommandContext.h"
#include <d3dcompiler.h>

using namespace Core;
using namespace RealtimeEngine;

#include "FullscreenRender.hpp"
#include "RealtimeRender.hpp"
#include "HandleInput.hpp"

// ----------------------------------------------------------------------------------------------------------------------------

static int     sNumSamplesPerRay = 500;
static int     sMaxScatterDepth  = 50;
static int     sNumThreads       = 4;
static float   sVertFov          = 40.f;
static int     sSampleScene      = SceneMesh;
static float   sClearColor[]     = { 0.2f, 0.2f, 0.2f, 1.0f };

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::Renderer(uint32_t width, uint32_t height)
    : RenderInterface(width, height)
    , RenderMode(RenderingMode_Realtime)
    , TheRaytracer(nullptr)
    , TheWorldScene(nullptr)
    , TheRenderScene(nullptr)
{
}

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::~Renderer()
{
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
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDeviceLost()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDeviceRestored()
{
    OnResizeRaytracer();
    SetupRenderBuffers();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnInit()
{
    // Get the number of cores on this system
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    sNumThreads = sysInfo.dwNumberOfProcessors;

    // Init the render device
    RenderDevice::Initialize(PlatformApp::GetHwnd(), Width, Height, this);

    // Setup render pipelines
    SetupFullscreenQuadPipeline();
    SetupRealtimePipeline();
    SetupRenderBuffers();

    // Load the scene data
    LoadScene();
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

    ZPrePassBuffer.Destroy();
    ZPrePassBuffer.Create("ZPrePass Buffer", width, height, 1, ZBufferRTType);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::LoadScene()
{
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
    TheWorldScene = GetSampleScene(SampleScene(sSampleScene));
    TheWorldScene->GetCamera().SetAspect((float)RenderDevice::Get().GetBackbufferWidth() / (float)RenderDevice::Get().GetBackbufferHeight());
    TheWorldScene->GetCamera().SetFocusDistanceToLookAt();
    
    TheRenderScene = new RealtimeEngine::RenderScene(TheWorldScene);
    sVertFov       = TheWorldScene->GetCamera().GetVertFov();

    // Load default texture
    RealtimeEngine::TextureManager::LoadFromFile(DefaultTextureName);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeRaytracer()
{
    int backbufferWidth  = RenderDevice::Get().GetBackbufferWidth();
    int backbufferHeight = RenderDevice::Get().GetBackbufferHeight();

    if (TheRaytracer != nullptr)
    {
        delete TheRaytracer;
        TheRaytracer = nullptr;
    }

    CPURaytracerTex.Destroy();
    CPURaytracerTex.Create("CpuRaytracerTex", backbufferWidth, backbufferHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM);

    // Create the ray tracer
    TheRaytracer = new Raytracer(backbufferWidth, backbufferHeight, sNumSamplesPerRay, sMaxScatterDepth, sNumThreads, true);

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

    OnResizeRaytracer();
    SetupRenderBuffers();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDestroy()
{
    // Let GPU finish before releasing D3D resources
    CommandListManager::Get().IdleGPU();
    OnDeviceLost();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::Raytrace(bool enable)
{
    if (TheRaytracer)
    {
        delete TheRaytracer;
        TheRaytracer = nullptr;
    }

    if (enable)
    {
        if (!TheRaytracer)
        {
            OnResizeRaytracer();
        }

        RenderMode = RenderingMode_Cpu;
        TheRaytracer->BeginRaytrace(TheWorldScene, OnRaytraceComplete);
    }
    else
    {
        RenderMode = RenderingMode_Realtime;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnRaytraceComplete(Raytracer* tracer, bool actuallyFinished)
{
    if (actuallyFinished)
    {
        WriteImageAndLog(tracer, "RaytracerWindows");
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static void ComputeMatrices(FXMMATRIX model, CXMMATRIX view, CXMMATRIX projection, RenderMatrices& mat)
{
    XMStoreFloat4x4(&mat.ModelMatrix, XMMatrixTranspose(model));
    XMStoreFloat4x4(&mat.ViewMatrix, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mat.ProjectionMatrix, XMMatrixTranspose(projection));
    XMStoreFloat4x4(&mat.ModelViewMatrix, XMMatrixTranspose(model * view));
    XMStoreFloat4x4(&mat.InverseTransposeModelViewMatrix, XMMatrixTranspose(XMMatrixTranspose(XMMatrixInverse(nullptr, model * view))));
    XMStoreFloat4x4(&mat.ModelViewProjectionMatrix, XMMatrixTranspose(model * view * projection));
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderSceneList(GraphicsContext& renderContext, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
    {
        // Don't render light shapes
        if (TheRenderScene->GetRenderSceneList()[i]->Hitable->IsALightShape())
        {
            continue;
        }

        RenderMatrices matrices;
        ComputeMatrices(TheRenderScene->GetRenderSceneList()[i]->WorldMatrix, viewMatrix, projectionMatrix, matrices);

        const RealtimeEngine::Texture* defaultTexture = RealtimeEngine::TextureManager::LoadFromFile(DefaultTextureName);
        const RealtimeEngine::Texture* diffuseTex     = (TheRenderScene->GetRenderSceneList()[i]->DiffuseTexture != nullptr) ? TheRenderScene->GetRenderSceneList()[i]->DiffuseTexture : defaultTexture;

        renderContext.SetDynamicConstantBufferView(RealtimeRenderingRootIndex_ConstantBuffer0, sizeof(matrices), &matrices);
        renderContext.SetDynamicDescriptor(RealtimeRenderingRootIndex_Texture0, 0, diffuseTex->GetSRV());
        renderContext.SetVertexBuffer(0, TheRenderScene->GetRenderSceneList()[i]->VertexBuffer.VertexBufferView());
        renderContext.SetIndexBuffer(TheRenderScene->GetRenderSceneList()[i]->IndexBuffer.IndexBufferView());
        renderContext.DrawIndexed((uint32_t)TheRenderScene->GetRenderSceneList()[i]->Indices.size());
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnUpdate(float dtSeconds)
{
    float scale          = (UserInput.ShiftKeyPressed ? 64.f : 32.0f) * 16.f * dtSeconds;
    float forwardAmount  = (UserInput.Forward - UserInput.Backward) * scale;
    float strafeAmount   = (UserInput.Left    - UserInput.Right)    * scale;
    float upDownAmount   = (UserInput.Up      - UserInput.Down)     * scale;

    TheRenderScene->UpdateCamera(sVertFov, forwardAmount, strafeAmount, upDownAmount, UserInput.MouseDx, UserInput.MouseDy, TheWorldScene->GetCamera());
    UserInput.MouseDx = 0;
    UserInput.MouseDy = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnRender()
{
    // Render the right stuff
    switch (RenderMode)
    {
    case RenderingMode_Cpu:
        RenderCPUResults();
        break;

    case RenderingMode_Realtime:
        RenderRealtimeResults();
        break;

    default:
        break;
    }

    // Present
    RenderDevice::Get().Present();
}
