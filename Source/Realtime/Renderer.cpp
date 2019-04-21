/*
  Copyright 2019 Khoi Nguyen

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
  files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

     The above copyright notice and this permission notice shall be included in all copies or substantial
     portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// ----------------------------------------------------------------------------------------------------------------------------

#include "Renderer.h"
#include "DeviceResources.h"
#include "PlatformApp.h"

// ----------------------------------------------------------------------------------------------------------------------------

#define FRAME_COUNT 3

// ----------------------------------------------------------------------------------------------------------------------------

using namespace yart;

// ----------------------------------------------------------------------------------------------------------------------------

static int          overrideWidth = 1280;
static int          overrideHeight = 640;
static int          sNumSamplesPerRay = 500;
static int          sMaxScatterDepth = 50;
static int          sNumThreads = 4;
static float        sVertFov = 40.f;
static int          sSampleScene = SceneFinal;
static float        sClearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
static const float  sShadowmapNear = 0.1f;
static const float  sShadowmapFar = 1000.f;

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
    DeviceResourcePtr = std::make_unique<yart::DeviceResources>
    (
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_UNKNOWN,
        FRAME_COUNT,
        D3D_FEATURE_LEVEL_12_1,
        // Sample shows handling of use cases with tearing support, which is OS dependent and has been supported since TH2.
        // Since the Fallback Layer requires Fall Creator's update (RS3), we don't need to handle non-tearing cases.
        DeviceResources::c_RequireTearingSupport,
        AdapterIDoverride
    );

    DeviceResourcePtr->RegisterDeviceNotify(this);
    DeviceResourcePtr->SetWindow(PlatformApp::GetHwnd(), Width, Height);
    DeviceResourcePtr->InitializeDXGIAdapter();
    DeviceResourcePtr->CreateDeviceResources();
    DeviceResourcePtr->CreateWindowSizeDependentResources();
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
    DeviceResourcePtr->Prepare();

    auto commandList  = DeviceResourcePtr->GetCommandList();
    auto renderTarget = DeviceResourcePtr->GetRenderTarget();

    D3D12_RESOURCE_BARRIER postCopyBarriers[1];
    postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);

    DeviceResourcePtr->Present(D3D12_RESOURCE_STATE_PRESENT);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnSizeChanged(UINT width, UINT height, bool minimized)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDestroy()
{
    // Let GPU finish before releasing D3D resources.
    DeviceResourcePtr->WaitForGpu();
    OnDeviceLost();
}

// ----------------------------------------------------------------------------------------------------------------------------

IDXGISwapChain* Renderer::GetSwapchain()
{
    return DeviceResourcePtr->GetSwapChain();
}
