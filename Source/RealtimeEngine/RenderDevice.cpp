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
#include "CommandList.h"
#include "CommandContext.h"
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_win32.h>
#include <imgui/examples/imgui_impl_dx12.h>
#include <algorithm>

// ----------------------------------------------------------------------------------------------------------------------------

using namespace RealtimeEngine;
using namespace std;

using Microsoft::WRL::ComPtr;

// ----------------------------------------------------------------------------------------------------------------------------

class ReportOnExit
{
public:

    static void DumpLiveObjectsInfo()
    {
        #ifdef _DEBUG
        {
            ComPtr<IDXGIDebug1> dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            }
        }
        #endif
    }

    ~ReportOnExit()
    {
        DumpLiveObjectsInfo();
    }
};

// ----------------------------------------------------------------------------------------------------------------------------

static RenderDevice* sRenderDevicePtr = nullptr;
static ReportOnExit  sReportOnExitObject;

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::Initialize(int width, int height, IDeviceNotify* deviceNotify, DescriptorHeapCollection* pDescriptorHeap,
    DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat, uint32_t backBufferCount, D3D_FEATURE_LEVEL minFeatureLevel, uint32_t flags, uint32_t adapterIDoverride, float depthClearValue)
{
    if (sRenderDevicePtr == nullptr)
    {
        // Render device create
        sRenderDevicePtr = new RenderDevice
        (
            pDescriptorHeap,
            backBufferFormat,
            depthBufferFormat,
            backBufferCount,
            minFeatureLevel,
            flags,
            adapterIDoverride,
            depthClearValue
        );

        sRenderDevicePtr->RegisterDeviceNotify(deviceNotify);
        sRenderDevicePtr->SetWindow(PlatformApp::GetHwnd(), width, height);
        sRenderDevicePtr->SetupDevice();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::Shutdown()
{
    if (sRenderDevicePtr != nullptr)
    {
        delete sRenderDevicePtr;
        sRenderDevicePtr = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::SetupDevice()
{
    // Create the main device
    InitializeDXGIAdapter();
    CreateDeviceResources();

    // Command manager
    CommandListManager::Get().Create(D3DDevice.Get());

    // Setup descriptor
    DescriptorHeapPtr->Reset();
    RenderTarget::Init(DescriptorHeapPtr);

    // Init the rest
    CreateWindowSizeDependentResources();
    SetupImgui();
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::CleanupDevice()
{
    ShutdownImgui();

    for (uint32_t n = 0; n < BackBufferCount; n++)
    {
        ColorTargets[n].Destroy();
    }

    if (ImguiDescriptorStack != nullptr)
    { 
        delete ImguiDescriptorStack;
        ImguiDescriptorStack = nullptr;
    }

    CommandContext::DestroyAllContexts();
    CommandListManager::Get().Shutdown();
    RootSignature::DestroyAll();
    PSO::DestroyAll();
    DepthStencil.Destroy();
    SwapChain.Reset();
    RenderTarget::Shutdown();
    D3DDevice.Reset();
    DXGIFactory.Reset();
    Adapter.Reset();

    DescriptorHeapPtr = nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------

RenderDevice& RenderDevice::Get()
{
    return *sRenderDevicePtr;
}

// ----------------------------------------------------------------------------------------------------------------------------

RenderDevice::RenderDevice(DescriptorHeapCollection* pDescriptorHeap, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat, uint32_t backBufferCount, D3D_FEATURE_LEVEL minFeatureLevel, uint32_t flags, uint32_t adapterIDoverride, float depthClearValue) :
    BackBufferFormat(backBufferFormat),
    DepthBufferFormat(depthBufferFormat),
    DepthStencil(depthClearValue),
    BackBufferIndex(0),
    ScreenViewport{},
    ScissorRect{},
    BackBufferCount(backBufferCount),
    D3dMinFeatureLevel(minFeatureLevel),
    Window(nullptr),
    D3dFeatureLevel(D3D_FEATURE_LEVEL_11_0),
    OutputSize{ 0, 0, 1, 1 },
    Options(flags),
    DeviceNotify(nullptr),
    IsWindowVisibleState(true),
    AdapterIDoverride(adapterIDoverride),
    AdapterID(UINT_MAX),
    DescriptorHeapPtr(pDescriptorHeap)
{
    if (backBufferCount > MAX_BACK_BUFFER_COUNT)
    {
        throw out_of_range("backBufferCount too large");
    }

    if (minFeatureLevel < D3D_FEATURE_LEVEL_11_0)
    {
        throw out_of_range("minFeatureLevel too low");
    }
    if (Options & RENDERDEVICE_FLAGS_REQUIRETEARINGSUPPORT)
    {
        Options |= RENDERDEVICE_FLAGS_ALLOWTEARING;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

RenderDevice::~RenderDevice()
{
    CommandListManager::Get().IdleGPU();
    CleanupDevice();
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
    if (Options & (RENDERDEVICE_FLAGS_ALLOWTEARING | RENDERDEVICE_FLAGS_REQUIRETEARINGSUPPORT))
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
            DEBUG_PRINTF("WARNING: Variable refresh rate displays are not supported.\n");
            if (Options & RENDERDEVICE_FLAGS_REQUIRETEARINGSUPPORT)
            {
                DEBUG_PRINTF("Error: Sample must be run on an OS with tearing support.\n");
                throw HrException(hr);
            }
            Options &= ~RENDERDEVICE_FLAGS_ALLOWTEARING;
        }
    }

    InitializeAdapter(&Adapter);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::CreateDeviceResources()
{
    // Create the DX12 API device object.
    ThrowIfFailed(D3D12CreateDevice(Adapter.Get(), D3dMinFeatureLevel, IID_PPV_ARGS(&D3DDevice)));

    // Check for raytracing support
    {
        HasRaytracingSupport = false;
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
        if (SUCCEEDED(D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
            && featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            HasRaytracingSupport = true;
        }
    }

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
    static const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
    {
        _countof(featureLevels), featureLevels, D3D_FEATURE_LEVEL_11_0
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
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::CreateWindowSizeDependentResources()
{
    if (!Window)
    {
        DEBUG_PRINTF("Call SetWindow with a valid Win32 window handle.\n");
        throw HrException(E_HANDLE);
    }

    // Wait until all previous GPU work is complete.
    CommandListManager::Get().IdleGPU();

    // Release resources that are tied to the swap chain and update fence values.
    for (uint32_t n = 0; n < BackBufferCount; n++)
    {
        ColorTargets[n].Destroy();
    }

    // Determine the render target size in pixels.
    uint32_t     backBufferWidth  = GetBackbufferWidth();
    uint32_t     backBufferHeight = GetBackbufferHeight();
    DXGI_FORMAT  backBufferFormat = GetBackBufferFormat();

    // If the swap chain already exists, resize it, otherwise create one.
    if (SwapChain)
    {
        // If the swap chain already exists, resize it.
        HRESULT hr = SwapChain->ResizeBuffers(
            BackBufferCount,
            backBufferWidth,
            backBufferHeight,
            backBufferFormat,
            (Options & RENDERDEVICE_FLAGS_ALLOWTEARING) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
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
        swapChainDesc.Width              = backBufferWidth;
        swapChainDesc.Height             = backBufferHeight;
        swapChainDesc.Format             = backBufferFormat;
        swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount        = BackBufferCount;
        swapChainDesc.SampleDesc.Count   = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags              = (Options & RENDERDEVICE_FLAGS_ALLOWTEARING) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

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

        ThrowIfFailed(DXGIFactory->CreateSwapChainForHwnd(CommandListManager::Get().GetCommandQueue(), Window, &swapChainDesc, &fsSwapChainDesc, nullptr, &swapChain));

        if (prevIsFullscreen)
        {
            PlatformApp::SetWindowZorderToTopMost(true);
        }

        ThrowIfFailed(swapChain->QueryInterface(IID_PPV_ARGS(&SwapChain)));

        // With tearing support enabled we will handle ALT+Enter key presses in the
        // window message loop rather than let DXGI handle it by calling SetFullscreenState.
        if (IsTearingSupported())
        {
            DXGIFactory->MakeWindowAssociation(Window, DXGI_MWA_NO_ALT_ENTER);
        }
    }

    // Create display render targets (backbuffer + depth stencil)
    CreateDisplayTargets();
   
    // Reset the index to the current back buffer.
    BackBufferIndex         = SwapChain->GetCurrentBackBufferIndex();

    // Set the 3D rendering viewport and scissor rectangle to target the entire window.
    ScreenViewport.TopLeftX = ScreenViewport.TopLeftY = 0.f;
    ScreenViewport.Width    = static_cast<float>(backBufferWidth);
    ScreenViewport.Height   = static_cast<float>(backBufferHeight);
    ScreenViewport.MinDepth = D3D12_MIN_DEPTH;
    ScreenViewport.MaxDepth = D3D12_MAX_DEPTH;

    ScissorRect.left        = ScissorRect.top = 0;
    ScissorRect.right       = backBufferWidth;
    ScissorRect.bottom      = backBufferHeight;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::CreateDisplayTargets()
{
    // Create render targets from swap chain
    for (uint32_t i = 0; i < BackBufferCount; ++i)
    {
        ComPtr<ID3D12Resource> renderTargetResource;
        RTL_HRESULT_SUCCEEDED(SwapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargetResource)));
        ColorTargets[i].CreateFromSwapChain("Primary SwapChain Buffer", renderTargetResource.Detach());
    }

    // Create depth buffer
    DepthStencil.Create("Scene Depth Buffer", GetBackbufferWidth(), GetBackbufferHeight(), DepthBufferFormat);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::SetWindow(HWND window, int width, int height)
{
    Window = window;

    OutputSize.left   = OutputSize.top = 0;
    OutputSize.right  = width;
    OutputSize.bottom = height;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool RenderDevice::WindowSizeChanged(int width, int height, bool minimized)
{
    IsWindowVisibleState = !minimized;

    // Present the last frame. This fixes a weird case where command queue errors out with
    // multiple sequence of re-size without a present call
    // TODO: Investigate why this is happening.
    Present();

    if (minimized || width == 0 || height == 0)
    {
        return false;
    }

    RECT newRc;
    newRc.left    = newRc.top = 0;
    newRc.right   = width;
    newRc.bottom  = height;
    if (   newRc.left == OutputSize.left
        && newRc.top == OutputSize.top
        && newRc.right == OutputSize.right
        && newRc.bottom == OutputSize.bottom)
    {
        return false;
    }

    OutputSize = newRc;

    ResetDescriptors();

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::ResetDescriptors()
{
    DescriptorHeapPtr->Reset();
    CreateWindowSizeDependentResources();
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::HandleDeviceLost()
{
    if (DeviceNotify)
    {
        DeviceNotify->OnDeviceLost();
    }

    CleanupDevice();
    ReportOnExit::DumpLiveObjectsInfo();
    SetupDevice();

    if (DeviceNotify)
    {
        DeviceNotify->OnDeviceRestored();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::Present()
{
    // Now prepare for present
    GraphicsContext& context = GraphicsContext::Begin("Present");
    {
        context.TransitionResource(ColorTargets[BackBufferIndex], D3D12_RESOURCE_STATE_PRESENT);
    }
    context.Finish(true);

    HRESULT hr;
    if (Options & RENDERDEVICE_FLAGS_ALLOWTEARING)
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

    ThrowIfFailed(hr);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        DEBUG_PRINTF("Device Lost on Present: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? D3DDevice->GetDeviceRemovedReason() : hr);
        HandleDeviceLost();
    }
    else
    {
        // Update the back buffer index
        BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::InitializeAdapter(IDXGIAdapter1 * *ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    for (uint32_t adapterID = 0; DXGI_ERROR_NOT_FOUND != DXGIFactory->EnumAdapters1(adapterID, &adapter); ++adapterID)
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

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::SetupImgui()
{
    ImguiDescriptorStack = new DescriptorHeapStack(128, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0);
    ImguiDescriptorStack->Reset();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(Window);
    ImGui_ImplDX12_Init(
        D3DDevice.Get(),
        1,
        GetBackBufferFormat(),
        ImguiDescriptorStack->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
        ImguiDescriptorStack->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDevice::ShutdownImgui()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

// ----------------------------------------------------------------------------------------------------------------------------

DescriptorHeapStack& RenderDevice::GetImguiDescriptorHeapStack()
{
    return *ImguiDescriptorStack;
}
