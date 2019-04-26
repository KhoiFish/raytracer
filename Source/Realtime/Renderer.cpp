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
#include "RenderDevice.h"
#include "PlatformApp.h"
#include "CommandList.h"

// ----------------------------------------------------------------------------------------------------------------------------

using namespace yart;

// ----------------------------------------------------------------------------------------------------------------------------

static int          overrideWidth     = 1280;
static int          overrideHeight    = 640;
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
    RenderDevice::Get()->TransitionRenderTarget();
    RenderDevice::Get()->Present();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnSizeChanged(UINT width, UINT height, bool minimized)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnDestroy()
{
    // Let GPU finish before releasing D3D resources.
    CommandListManager::Get().IdleGPU();
    OnDeviceLost();
}
