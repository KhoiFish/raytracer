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
#include "RealtimeEngine/RenderDevice.h"
#include "RealtimeEngine/PlatformApp.h"
#include "RealtimeEngine/CommandList.h"

using namespace yart;

// ----------------------------------------------------------------------------------------------------------------------------

static int          sNumSamplesPerRay = 500;
static int          sMaxScatterDepth  = 50;
static int          sNumThreads       = 4;
static float        sVertFov          = 40.f;
static int          sSampleScene      = SceneFinal;
static float        sClearColor[]     = { 0.2f, 0.2f, 0.2f, 1.0f };
static const float  sShadowmapNear    = 0.1f;
static const float  sShadowmapFar     = 1000.f;

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::Renderer(uint32_t width, uint32_t height)
    : RenderInterface(width, height)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

Renderer::~Renderer()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDeviceLost()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDeviceRestored()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnInit()
{
    RenderDevice::Initialize(PlatformApp::GetHwnd(), Width, Height, this);
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

        RenderMode = ModeRaytracer;
        TheRaytracer->BeginRaytrace(Scene, OnRaytraceComplete);
    }
    else
    {
        RenderMode = ModePreview;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeRaytracer()
{
    int backbufferWidth  = RenderDevice::Get().GetBackbufferWidth();
    int backbufferHeight = RenderDevice::Get().GetBackbufferHeight();

    // Is this the first time running?
    if (TheRaytracer == nullptr)
    {
        /*auto device = Application::Get().GetDevice();

        D3D12_RESOURCE_DESC textureDesc
            = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, backbufferWidth, backbufferHeight, 1);

        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&textureResource)));

        CPURaytracerTex.SetTextureUsage(TextureUsage::Albedo);
        CPURaytracerTex.SetD3D12Resource(textureResource);
        CPURaytracerTex.CreateViews();
        CPURaytracerTex.SetName(L"RaytraceSourceTexture");*/
    }
    else
    {
        // Resize
        //CPURaytracerTex.Resize(backbufferWidth, backbufferHeight);

        delete TheRaytracer;
        TheRaytracer = nullptr;
    }

    // Create the ray tracer
    TheRaytracer = new Raytracer(backbufferWidth, backbufferHeight, sNumSamplesPerRay, sMaxScatterDepth, sNumThreads, true);

    // Reset the aspect ratio on the scene camera
    if (Scene != nullptr)
    {
        Scene->GetCamera().SetAspect((float)backbufferWidth / (float)backbufferHeight);
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

void Renderer::CreateDeviceDependentResources()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::CreateWindowSizeDependentResources()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::ReleaseDeviceDependentResources()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::ReleaseWindowSizeDependentResources()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnKeyDown(UINT8 key)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnUpdate()
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnRender()
{
    RenderDevice::Get().BeginRendering();
    RenderDevice::Get().Present();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnSizeChanged(UINT width, UINT height, bool minimized)
{
    if (!RenderDevice::Get().WindowSizeChanged(width, height, minimized))
    {
        return;
    }

    OnResizeRaytracer();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDestroy()
{
    // Let GPU finish before releasing D3D resources.
    CommandListManager::Get().IdleGPU();
    OnDeviceLost();
}

