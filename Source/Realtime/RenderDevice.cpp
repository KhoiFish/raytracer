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

#include "RenderDevice.h"
#include "PlatformApp.h"
#include <algorithm>

// ----------------------------------------------------------------------------------------------------------------------------

#define FRAME_COUNT 3

// ----------------------------------------------------------------------------------------------------------------------------

using namespace yart;
using namespace std;

using Microsoft::WRL::ComPtr;

// ----------------------------------------------------------------------------------------------------------------------------

std::unique_ptr<RenderDevice> RenderDevice::RenderDevicePtr = nullptr;

// ----------------------------------------------------------------------------------------------------------------------------

static inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
    default:                                return fmt;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

RenderDevice* RenderDevice::Get()
{
    if (RenderDevicePtr == nullptr)
    {
        RenderDevicePtr = std::make_unique<yart::RenderDevice>
        (
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_UNKNOWN,
            FRAME_COUNT,
            D3D_FEATURE_LEVEL_12_1,
            RequireTearingSupport,
            UINT_MAX
        );
    }

    return RenderDevicePtr.get();
}

// ----------------------------------------------------------------------------------------------------------------------------

// Constructor for DeviceResources.
RenderDevice::RenderDevice(DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat, UINT backBufferCount, D3D_FEATURE_LEVEL minFeatureLevel, UINT flags, UINT adapterIDoverride) :
    BackBufferIndex(0),
    FenceValues{},
    RtvDescriptorSize(0),
    ScreenViewport{},
    ScissorRect{},
    BackBufferFormat(backBufferFormat),
    DepthBufferFormat(depthBufferFormat),
    BackBufferCount(backBufferCount),
    D3dMinFeatureLevel(minFeatureLevel),
    Window(nullptr),
    D3dFeatureLevel(D3D_FEATURE_LEVEL_11_0),
    OutputSize{ 0, 0, 1, 1 },
    Options(flags),
    DeviceNotify(nullptr),
    IsWindowVisibleState(true),
    AdapterIDoverride(adapterIDoverride),
    AdapterID(UINT_MAX)
{
    if (backBufferCount > MAX_BACK_BUFFER_COUNT)
    {
        throw out_of_range("backBufferCount too large");
    }

    if (minFeatureLevel < D3D_FEATURE_LEVEL_11_0)
    {
        throw out_of_range("minFeatureLevel too low");
    }
    if (Options & RequireTearingSupport)
    {
        Options |= AllowTearing;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

RenderDevice::~RenderDevice()
{
    // Ensure that the GPU is no longer referencing resources that are about to be destroyed.
    WaitForGpu();
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::InitializeDXGIAdapter()
{
    bool debugDXGI = false;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
        {
            debugDXGI = true;

            ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&DXGIFactory)));

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        }
    }
#endif

    if (!debugDXGI)
    {
        ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&DXGIFactory)));
    }

    // Determines whether tearing support is available for fullscreen borderless windows.
    if (Options & (AllowTearing | RequireTearingSupport))
    {
        BOOL allowTearing = FALSE;

        ComPtr<IDXGIFactory5> factory5;
        HRESULT hr = DXGIFactory.As(&factory5);
        if (SUCCEEDED(hr))
        {
            hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        if (FAILED(hr) || !allowTearing)
        {
            OutputDebugStringA("WARNING: Variable refresh rate displays are not supported.\n");
            if (Options & RequireTearingSupport)
            {
                ThrowIfFailed(false, L"Error: Sample must be run on an OS with tearing support.\n");
            }
            Options &= ~AllowTearing;
        }
    }

    InitializeAdapter(&Adapter);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::CreateDeviceResources()
{
    // Create the DX12 API device object.
    ThrowIfFailed(D3D12CreateDevice(Adapter.Get(), D3dMinFeatureLevel, IID_PPV_ARGS(&D3DDevice)));

#ifndef NDEBUG
    // Configure debug device (if active).
    ComPtr<ID3D12InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(D3DDevice.As(&d3dInfoQueue)))
    {
#ifdef _DEBUG
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
        D3D12_MESSAGE_ID hide[] =
        {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
#endif

    // Determine maximum supported feature level for this device
    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
    {
        _countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = D3DDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (SUCCEEDED(hr))
    {
        D3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
    }
    else
    {
        D3dFeatureLevel = D3dMinFeatureLevel;
    }

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(D3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue)));

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = BackBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&RtvDescriptorHeap)));

    RtvDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    if (DepthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&DsvDescriptorHeap)));
    }

    // Create a command allocator for each back buffer that will be rendered to.
    for (UINT n = 0; n < BackBufferCount; n++)
    {
        ThrowIfFailed(D3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocators[n])));
    }

    // Create a command list for recording graphics commands.
    ThrowIfFailed(D3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&CommandList)));
    ThrowIfFailed(CommandList->Close());

    // Create a fence for tracking GPU execution progress.
    ThrowIfFailed(D3DDevice->CreateFence(FenceValues[BackBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
    FenceValues[BackBufferIndex]++;

    FenceEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!FenceEvent.IsValid())
    {
        ThrowIfFailed(E_FAIL, L"CreateEvent failed.\n");
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::CreateWindowSizeDependentResources()
{
    if (!Window)
    {
        ThrowIfFailed(E_HANDLE, L"Call SetWindow with a valid Win32 window handle.\n");
    }

    // Wait until all previous GPU work is complete.
    WaitForGpu();

    // Release resources that are tied to the swap chain and update fence values.
    for (UINT n = 0; n < BackBufferCount; n++)
    {
        RenderTargets[n].Reset();
        FenceValues[n] = FenceValues[BackBufferIndex];
    }

    // Determine the render target size in pixels.
    UINT backBufferWidth  = max(UINT(OutputSize.right - OutputSize.left), (UINT)1);
    UINT backBufferHeight = max(UINT(OutputSize.bottom - OutputSize.top), (UINT)1);
    DXGI_FORMAT backBufferFormat = NoSRGB(BackBufferFormat);

    // If the swap chain already exists, resize it, otherwise create one.
    if (SwapChain)
    {
        // If the swap chain already exists, resize it.
        HRESULT hr = SwapChain->ResizeBuffers(
            BackBufferCount,
            backBufferWidth,
            backBufferHeight,
            backBufferFormat,
            (Options & AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? D3DDevice->GetDeviceRemovedReason() : hr);
            OutputDebugStringA(buff);
#endif
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            ThrowIfFailed(hr);
        }
    }
    else
    {
        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = BackBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = (Options & AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = { 0 };
        fsSwapChainDesc.Windowed = TRUE;

        // Create a swap chain for the window.
        ComPtr<IDXGISwapChain1> swapChain;

        // DXGI does not allow creating a swapchain targeting a window which has fullscreen styles(no border + topmost).
        // Temporarily remove the topmost property for creating the swapchain.
        bool prevIsFullscreen = PlatformApp::IsFullscreen();
        if (prevIsFullscreen)
        {
            PlatformApp::SetWindowZorderToTopMost(false);
        }

        ThrowIfFailed(DXGIFactory->CreateSwapChainForHwnd(CommandQueue.Get(), Window, &swapChainDesc, &fsSwapChainDesc, nullptr, &swapChain));

        if (prevIsFullscreen)
        {
            PlatformApp::SetWindowZorderToTopMost(true);
        }

        ThrowIfFailed(swapChain.As(&SwapChain));

        // With tearing support enabled we will handle ALT+Enter key presses in the
        // window message loop rather than let DXGI handle it by calling SetFullscreenState.
        if (IsTearingSupported())
        {
            DXGIFactory->MakeWindowAssociation(Window, DXGI_MWA_NO_ALT_ENTER);
        }
    }

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    for (UINT n = 0; n < BackBufferCount; n++)
    {
        ThrowIfFailed(SwapChain->GetBuffer(n, IID_PPV_ARGS(&RenderTargets[n])));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        RenderTargets[n]->SetName(name);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = BackBufferFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), n, RtvDescriptorSize);
        D3DDevice->CreateRenderTargetView(RenderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    // Reset the index to the current back buffer.
    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

    if (DepthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
        // on this surface.
        CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DepthBufferFormat,
            backBufferWidth,
            backBufferHeight,
            1, // This depth stencil view has only one texture.
            1  // Use a single mipmap level.
        );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DepthBufferFormat;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(D3DDevice->CreateCommittedResource(&depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&DepthStencil)
        ));

        DepthStencil->SetName(L"Depth stencil");

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DepthBufferFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        D3DDevice->CreateDepthStencilView(DepthStencil.Get(), &dsvDesc, DsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // Set the 3D rendering viewport and scissor rectangle to target the entire window.
    ScreenViewport.TopLeftX = ScreenViewport.TopLeftY = 0.f;
    ScreenViewport.Width = static_cast<float>(backBufferWidth);
    ScreenViewport.Height = static_cast<float>(backBufferHeight);
    ScreenViewport.MinDepth = D3D12_MIN_DEPTH;
    ScreenViewport.MaxDepth = D3D12_MAX_DEPTH;

    ScissorRect.left = ScissorRect.top = 0;
    ScissorRect.right = backBufferWidth;
    ScissorRect.bottom = backBufferHeight;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::SetWindow(HWND window, int width, int height)
{
    Window = window;

    OutputSize.left = OutputSize.top = 0;
    OutputSize.right = width;
    OutputSize.bottom = height;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool RenderDevice::WindowSizeChanged(int width, int height, bool minimized)
{
    IsWindowVisibleState = !minimized;

    if (minimized || width == 0 || height == 0)
    {
        return false;
    }

    RECT newRc;
    newRc.left = newRc.top = 0;
    newRc.right = width;
    newRc.bottom = height;
    if (newRc.left == OutputSize.left
        && newRc.top == OutputSize.top
        && newRc.right == OutputSize.right
        && newRc.bottom == OutputSize.bottom)
    {
        return false;
    }

    OutputSize = newRc;
    CreateWindowSizeDependentResources();
    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::HandleDeviceLost()
{
    if (DeviceNotify)
    {
        DeviceNotify->OnDeviceLost();
    }

    for (UINT n = 0; n < BackBufferCount; n++)
    {
        CommandAllocators[n].Reset();
        RenderTargets[n].Reset();
    }

    DepthStencil.Reset();
    CommandQueue.Reset();
    CommandList.Reset();
    Fence.Reset();
    RtvDescriptorHeap.Reset();
    DsvDescriptorHeap.Reset();
    SwapChain.Reset();
    D3DDevice.Reset();
    DXGIFactory.Reset();
    Adapter.Reset();

#ifdef _DEBUG
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif
    InitializeDXGIAdapter();
    CreateDeviceResources();
    CreateWindowSizeDependentResources();

    if (DeviceNotify)
    {
        DeviceNotify->OnDeviceRestored();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::Prepare(D3D12_RESOURCE_STATES beforeState)
{
    // Reset command list and allocator.
    ThrowIfFailed(CommandAllocators[BackBufferIndex]->Reset());
    ThrowIfFailed(CommandList->Reset(CommandAllocators[BackBufferIndex].Get(), nullptr));

    if (beforeState != D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
        // Transition the render target into the correct state to allow for drawing into it.
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[BackBufferIndex].Get(), beforeState, D3D12_RESOURCE_STATE_RENDER_TARGET);
        CommandList->ResourceBarrier(1, &barrier);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::Present(D3D12_RESOURCE_STATES beforeState)
{
    if (beforeState != D3D12_RESOURCE_STATE_PRESENT)
    {
        // Transition the render target to the state that allows it to be presented to the display.
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(RenderTargets[BackBufferIndex].Get(), beforeState, D3D12_RESOURCE_STATE_PRESENT);
        CommandList->ResourceBarrier(1, &barrier);
    }

    ExecuteCommandList();

    HRESULT hr;
    if (Options & AllowTearing)
    {
        // Recommended to always use tearing if supported when using a sync interval of 0.
        // Note this will fail if in true 'fullscreen' mode.
        hr = SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    }
    else
    {
        // The first argument instructs DXGI to block until VSync, putting the application
        // to sleep until the next VSync. This ensures we don't waste any cycles rendering
        // frames that will never be displayed to the screen.
        hr = SwapChain->Present(1, 0);
    }

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? D3DDevice->GetDeviceRemovedReason() : hr);
        OutputDebugStringA(buff);
#endif
        HandleDeviceLost();
    }
    else
    {
        ThrowIfFailed(hr);

        MoveToNextFrame();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::ExecuteCommandList()
{
    ThrowIfFailed(CommandList->Close());
    ID3D12CommandList* commandLists[] = { CommandList.Get() };
    CommandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::WaitForGpu() noexcept
{
    if (CommandQueue && Fence && FenceEvent.IsValid())
    {
        // Schedule a Signal command in the GPU queue.
        UINT64 fenceValue = FenceValues[BackBufferIndex];
        if (SUCCEEDED(CommandQueue->Signal(Fence.Get(), fenceValue)))
        {
            // Wait until the Signal has been processed.
            if (SUCCEEDED(Fence->SetEventOnCompletion(fenceValue, FenceEvent.Get())))
            {
                WaitForSingleObjectEx(FenceEvent.Get(), INFINITE, FALSE);

                // Increment the fence value for the current frame.
                FenceValues[BackBufferIndex]++;
            }
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = FenceValues[BackBufferIndex];
    ThrowIfFailed(CommandQueue->Signal(Fence.Get(), currentFenceValue));

    // Update the back buffer index.
    BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (Fence->GetCompletedValue() < FenceValues[BackBufferIndex])
    {
        ThrowIfFailed(Fence->SetEventOnCompletion(FenceValues[BackBufferIndex], FenceEvent.Get()));
        WaitForSingleObjectEx(FenceEvent.Get(), INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    FenceValues[BackBufferIndex] = currentFenceValue + 1;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::InitializeAdapter(IDXGIAdapter1 * *ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterID = 0; DXGI_ERROR_NOT_FOUND != DXGIFactory->EnumAdapters1(adapterID, &adapter); ++adapterID)
    {
        if (AdapterIDoverride != UINT_MAX && adapterID != AdapterIDoverride)
        {
            continue;
        }

        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            AdapterID = adapterID;
            AdapterDescription = desc.Description;
#ifdef _DEBUG
            wchar_t buff[256] = {};
            swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterID, desc.VendorId, desc.DeviceId, desc.Description);
            OutputDebugStringW(buff);
#endif
            break;
        }
    }

#if !defined(NDEBUG)
    if (!adapter && AdapterIDoverride == UINT_MAX)
    {
        // Try WARP12 instead
        if (FAILED(DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))))
        {
            throw exception("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    if (!adapter)
    {
        if (AdapterIDoverride != UINT_MAX)
        {
            throw exception("Unavailable adapter requested.");
        }
        else
        {
            throw exception("Unavailable adapter.");
        }
    }

    *ppAdapter = adapter.Detach();
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::RegisterDeviceNotify(IDeviceNotify* deviceNotify)
{

    DeviceNotify = deviceNotify;

    // On RS4 and higher, applications that handle device removal 
    // should declare themselves as being able to do so
    __if_exists(DXGIDeclareAdapterRemovalSupport)
    {
        if (deviceNotify)
        {
            if (FAILED(DXGIDeclareAdapterRemovalSupport()))
            {
                OutputDebugString(L"Warning: application failed to declare adapter removal support.\n");
            }
        }
    }
}
