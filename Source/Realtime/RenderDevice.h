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
#include "IDeviceNotify.h"
#include "GPUResource.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{
    class RenderDevice
    {
    public:

        static const unsigned int AllowTearing = 0x1;
        static const unsigned int RequireTearingSupport = 0x2;

    public:

        RenderDevice() {}

        RenderDevice(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
            UINT backBufferCount = 2,
            D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0,
            UINT flags = 0,
            UINT adapterIDoverride = UINT_MAX);

        ~RenderDevice();

    public:

        static RenderDevice*           Get();

        void                           InitializeDXGIAdapter();
        void                           SetAdapterOverride(UINT adapterID) { AdapterIDoverride = adapterID; }
        void                           CreateDeviceResources();
        void                           CreateWindowSizeDependentResources();
        void                           SetWindow(HWND window, int width, int height);
        bool                           WindowSizeChanged(int width, int height, bool minimized);
        void                           HandleDeviceLost();
        void                           RegisterDeviceNotify(IDeviceNotify* deviceNotify);
        void                           Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT);
        void                           Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void                           ExecuteCommandList();
        void                           WaitForGpu() noexcept;

        RECT                           GetOutputSize() const         { return OutputSize; }
        bool                           IsWindowVisible() const       { return IsWindowVisibleState; }
        bool                           IsTearingSupported() const    { return Options & AllowTearing; }
        IDXGIAdapter1*                 GetAdapter() const            { return Adapter.Get(); }
        ID3D12Device*                  GetD3DDevice() const          { return D3DDevice.Get(); }
        IDXGIFactory4*                 GetDXGIFactory() const        { return DXGIFactory.Get(); }
        IDXGISwapChain3*               GetSwapChain() const          { return SwapChain.Get(); }
        D3D_FEATURE_LEVEL              GetDeviceFeatureLevel() const { return D3dFeatureLevel; }
        ID3D12Resource*                GetRenderTarget() const       { return RenderTargets[BackBufferIndex].Get(); }
        ID3D12Resource*                GetDepthStencil() const       { return DepthStencil.Get(); }
        ID3D12CommandQueue*            GetCommandQueue() const       { return CommandQueue.Get(); }
        ID3D12CommandAllocator*        GetCommandAllocator() const   { return CommandAllocators[BackBufferIndex].Get(); }
        ID3D12GraphicsCommandList*     GetCommandList() const        { return CommandList.Get(); }
        DXGI_FORMAT                    GetBackBufferFormat() const   { return BackBufferFormat; }
        DXGI_FORMAT                    GetDepthBufferFormat() const  { return DepthBufferFormat; }
        D3D12_VIEWPORT                 GetScreenViewport() const     { return ScreenViewport; }
        D3D12_RECT                     GetScissorRect() const        { return ScissorRect; }
        UINT                           GetCurrentFrameIndex() const  { return BackBufferIndex; }
        UINT                           GetPreviousFrameIndex() const { return BackBufferIndex == 0 ? BackBufferCount - 1 : BackBufferIndex - 1; }
        UINT                           GetBackBufferCount() const    { return BackBufferCount; }
        unsigned int                   GetDeviceOptions() const      { return Options; }
        LPCWSTR                        GetAdapterDescription() const { return AdapterDescription.c_str(); }
        UINT                           GetAdapterID() const          { return AdapterID; }
        CD3DX12_CPU_DESCRIPTOR_HANDLE  GetRenderTargetView() const   { return CD3DX12_CPU_DESCRIPTOR_HANDLE(RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), BackBufferIndex, RtvDescriptorSize); }
        CD3DX12_CPU_DESCRIPTOR_HANDLE  GetDepthStencilView() const   { return CD3DX12_CPU_DESCRIPTOR_HANDLE(DsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }

    private:

        void                           MoveToNextFrame();
        void                           InitializeAdapter(IDXGIAdapter1** ppAdapter);

    private:

        const static size_t                                 MAX_BACK_BUFFER_COUNT = 3;

        static std::unique_ptr<RenderDevice>                RenderDevicePtr;

        UINT                                                AdapterIDoverride;
        UINT                                                BackBufferIndex;
        Microsoft::WRL::ComPtr<IDXGIAdapter1>               Adapter;
        UINT                                                AdapterID;
        std::wstring                                        AdapterDescription;

        // Direct3D objects.
        Microsoft::WRL::ComPtr<ID3D12Device>                D3DDevice;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          CommandQueue;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   CommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      CommandAllocators[MAX_BACK_BUFFER_COUNT];

        // Swap chain objects.
        Microsoft::WRL::ComPtr<IDXGIFactory4>               DXGIFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain3>             SwapChain;
        Microsoft::WRL::ComPtr<ID3D12Resource>              RenderTargets[MAX_BACK_BUFFER_COUNT];
        Microsoft::WRL::ComPtr<ID3D12Resource>              DepthStencil;

        // Presentation fence objects.
        Microsoft::WRL::ComPtr<ID3D12Fence>                 Fence;
        UINT64                                              FenceValues[MAX_BACK_BUFFER_COUNT];
        Microsoft::WRL::Wrappers::Event                     FenceEvent;

        // Direct3D rendering objects.
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        RtvDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        DsvDescriptorHeap;
        UINT                                                RtvDescriptorSize;
        D3D12_VIEWPORT                                      ScreenViewport;
        D3D12_RECT                                          ScissorRect;

        // Direct3D properties.
        DXGI_FORMAT                                         BackBufferFormat;
        DXGI_FORMAT                                         DepthBufferFormat;
        UINT                                                BackBufferCount;
        D3D_FEATURE_LEVEL                                   D3dMinFeatureLevel;

        // Cached device properties.
        HWND                                                Window;
        D3D_FEATURE_LEVEL                                   D3dFeatureLevel;
        RECT                                                OutputSize;
        bool                                                IsWindowVisibleState;

        // DeviceResources options (see flags above)
        unsigned int                                        Options;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify*                                      DeviceNotify;
    };
}
