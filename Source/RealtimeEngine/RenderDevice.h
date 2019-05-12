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

#pragma once

#include "Globals.h"
#include "GPUResource.h"
#include "DescriptorHeap.h"
#include "RootSignature.h"
#include "PipelineStateObject.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include <algorithm>

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    class GraphicsContext;

    // ----------------------------------------------------------------------------------------------------------------------------

    struct IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class RenderDevice
    {
    public:

        static const unsigned int AllowTearing          = 0x1;
        static const unsigned int RequireTearingSupport = 0x2;

    public:

        static void                                         Initialize(
                                                                HWND window, int width, int height, IDeviceNotify* deviceNotify,
                                                                DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
                                                                DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
                                                                uint32_t backBufferCount = 2,
                                                                D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_12_1,
                                                                uint32_t flags = 0,
                                                                uint32_t adapterIDoverride = UINT_MAX,
                                                                float depthClearValue = 1.0f);

        static void                                         Shutdown();
        static RenderDevice&                                Get();

    public:

        void                                                Present();
        D3D12_CPU_DESCRIPTOR_HANDLE                         AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count = 1);
        bool                                                WindowSizeChanged(int width, int height, bool minimized);
        
    public:

        IDXGIAdapter1*                                      GetAdapter()                     { return Adapter.Get(); }
        ID3D12Device5*                                      GetD3DDevice()                   { return D3DDevice.Get(); }
        IDXGIFactory4*                                      GetDXGIFactory()                 { return DXGIFactory.Get(); }
        IDXGISwapChain3*                                    GetSwapChain()                   { return SwapChain.Get(); }
        ColorBuffer&                                        GetRenderTarget()                { return RenderTargets[BackBufferIndex]; }
        DepthBuffer&                                        GetDepthStencil()                { return DepthStencil; }

        bool                                                IsWindowVisible() const          { return IsWindowVisibleState; }
        bool                                                IsTearingSupported() const       { return Options & AllowTearing; }

        D3D_FEATURE_LEVEL                                   GetDeviceFeatureLevel() const    { return D3dFeatureLevel; }
        DXGI_FORMAT                                         GetBackBufferFormat() const      { return BackBufferFormat; }
        DXGI_FORMAT                                         GetDepthBufferFormat() const     { return DepthBufferFormat; }
        D3D12_VIEWPORT                                      GetScreenViewport() const        { return ScreenViewport; }
        D3D12_RECT                                          GetScissorRect() const           { return ScissorRect; }
        RECT                                                GetOutputSize() const            { return OutputSize; }
        uint32_t                                            GetBackbufferWidth() const       { return std::max(uint32_t(OutputSize.right - OutputSize.left), (uint32_t)1); }
        uint32_t                                            GetBackbufferHeight() const      { return std::max(uint32_t(OutputSize.bottom - OutputSize.top), (uint32_t)1); }

        uint32_t                                            GetCurrentFrameIndex() const     { return BackBufferIndex; }
        uint32_t                                            GetPreviousFrameIndex() const    { return BackBufferIndex == 0 ? BackBufferCount - 1 : BackBufferIndex - 1; }
        uint32_t                                            GetBackBufferCount() const       { return BackBufferCount; }
        unsigned int                                        GetDeviceOptions() const         { return Options; }
        LPCWSTR                                             GetAdapterDescription() const    { return AdapterDescription.c_str(); }
        uint32_t                                            GetAdapterID() const             { return AdapterID; }

        ComputePSO*                                         GetGenerateMipsLinearPSO()       { return GenerateMipsLinearPSO; }
        ComputePSO*                                         GetGenerateMipsGammaPSO()        { return GenerateMipsGammaPSO; }
        RootSignature&                                      GetGenerateMipsRootSig()         { return GenerateMipsRS; }

    private:

        RenderDevice(DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat, uint32_t backBufferCount, D3D_FEATURE_LEVEL minFeatureLevel, uint32_t flags, uint32_t adapterIDoverride, float depthClearValue);
        ~RenderDevice();

        void                                                SetupDevice();
        void                                                CleanupDevice();
        void                                                InitializeDXGIAdapter();
        void                                                InitializeAdapter(IDXGIAdapter1** ppAdapter);
        void                                                SetAdapterOverride(uint32_t adapterID) { AdapterIDoverride = adapterID; }
        void                                                CreateDeviceResources();
        void                                                CreateWindowSizeDependentResources();
        void                                                CreateDisplayTargets();
        void                                                SetWindow(HWND window, int width, int height);
        void                                                HandleDeviceLost();
        void                                                RegisterDeviceNotify(IDeviceNotify* deviceNotify);

    private:

        const static size_t                                 MAX_BACK_BUFFER_COUNT = 3;

        uint32_t                                            AdapterIDoverride;
        Microsoft::WRL::ComPtr<IDXGIAdapter1>               Adapter;
        uint32_t                                            AdapterID;
        std::wstring                                        AdapterDescription;
        Microsoft::WRL::ComPtr<ID3D12Device5>               D3DDevice;
        Microsoft::WRL::ComPtr<IDXGIFactory4>               DXGIFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain3>             SwapChain;

        DXGI_FORMAT                                         BackBufferFormat;
        DXGI_FORMAT                                         DepthBufferFormat;
        uint32_t                                            BackBufferIndex;
        uint32_t                                            BackBufferCount;
        ColorBuffer                                         RenderTargets[MAX_BACK_BUFFER_COUNT];
        DepthBuffer                                         DepthStencil;

        D3D12_VIEWPORT                                      ScreenViewport;
        D3D12_RECT                                          ScissorRect;
        D3D_FEATURE_LEVEL                                   D3dMinFeatureLevel;
        D3D_FEATURE_LEVEL                                   D3dFeatureLevel;

        HWND                                                Window;
        RECT                                                OutputSize;
        bool                                                IsWindowVisibleState;
        unsigned int                                        Options;
        IDeviceNotify*                                      DeviceNotify;

        RootSignature                                       GenerateMipsRS;
        ComputePSO                                          GenerateMipsLinearPSO[4];
        ComputePSO                                          GenerateMipsGammaPSO[4];

        DescriptorAllocator                                 DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        };
    };
}
