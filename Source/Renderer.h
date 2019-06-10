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

#include "Core/Raytracer.h"
#include "Core/Camera.h"
#include "Core/IHitable.h"
#include "SampleScenes.h"
#include "RealtimeEngine/Globals.h"
#include "RealtimeEngine/RenderInterface.h"
#include "RealtimeEngine/RootSignature.h"
#include "RealtimeEngine/PipelineStateObject.h"
#include "RealtimeEngine/RenderTarget.h"
#include "RealtimeEngine/RealtimeCamera.h"
#include "RealtimeEngine/RealtimeScene.h"
#include "RealtimeEngine/DescriptorHeapStack.h"
#include "RendererShaderInclude.h"
#include <chrono>
#include <random>
#include <thread>

// ----------------------------------------------------------------------------------------------------------------------------

using namespace RealtimeEngine;
using namespace DirectX;

struct RenderSceneNode;

// ----------------------------------------------------------------------------------------------------------------------------

class Renderer : public RenderInterface
{
public:

    Renderer(uint32_t width, uint32_t height);
    virtual ~Renderer();

public:

    virtual void                OnDeviceLost() override;
    virtual void                OnDeviceRestored() override;
    virtual void                OnInit() override;
    virtual void                OnKeyDown(UINT8 key) override;
    virtual void                OnKeyUp(uint8_t key) override;
    virtual void                OnMouseMove(int32_t x, int32_t y) override;
    virtual void                OnLeftButtonDown(int32_t x, int32_t y) override;
    virtual void                OnLeftButtonUp(int32_t x, int32_t y) override;
    virtual void                OnUpdate(float dtSeconds) override;
    virtual void                OnRender() override;
    virtual bool                OnSizeChanged(uint32_t width, uint32_t height, bool minimized) override;
    virtual void                OnDestroy() override;

private:

    static void                 LoadingThread(Renderer* pRenderer);
    void                        CleanupLoadingThread();

    void                        SetupGpuRaytracingPipeline();
    void                        SetupDenoisePipeline();
    void                        SetupRasterRootSignatures();
    void                        SetupRenderBuffers();
    void                        SetupGui();
    void                        StartSceneLoad();

    void                        SetupRasterDescriptors();
    void                        SetupGpuRaytracingRootSignatures();
    void                        SetupGpuRaytracingDescriptors();
    void                        SetupDenoiseDescriptors();
    void                        SetupGpuRaytracingPSO();

    void                        OnResizeCpuRaytracer(bool startRaytrace = false);

    void                        CleanupGpuRaytracerPass();
    void                        CleanupRasterPass();
    void                        CleanupDenoisePass();

    void                        ToggleCpuRaytracer();
    static void                 OnCpuRaytraceComplete(Core::Raytracer* tracer, bool actuallyFinished);

    void                        SetEnableCpuRaytrace(bool enable);
    void                        SetCameraDirty();
    void                        SetupSceneConstantBuffer(SceneConstantBuffer& sceneCB);
    void                        UpdateWindowTitle();

    void                        RenderSceneList(GraphicsContext& renderContext);
    void                        RenderGeometryPass();
    void                        RenderGpuRaytracing();
    void                        RenderDenoisePass();
    void                        RenderCompositePass();
    void                        RenderGui();
    void                        RenderGuiOptionsWindow();
    void                        RenderGuiLoadingScreen();

    const char*                 GetSelectedBufferName();
    uint32_t                    GetNumberHitPrograms();
    D3D12_SAMPLER_DESC          GetLinearSamplerDesc();
    D3D12_SAMPLER_DESC          GetAnisoSamplerDesc();
    D3D12_RASTERIZER_DESC       GetDefaultRasterState(bool frontCounterClockwise = false);
    D3D12_BLEND_DESC            GetBlendDisabledState();
    D3D12_DEPTH_STENCIL_DESC    GetDepthEnabledState();
    D3D12_DEPTH_STENCIL_DESC    GetDepthDisabledState();

private:

    enum AppState
    {
        AppState_Loading,
        AppState_RenderScene,
    };

    enum GBufferType
    {
        GBufferType_Position = 0,
        GBufferType_Normal,
        GBufferType_TexCoordAndDepth,
        GBufferType_Albedo,
        GBufferType_CurrSVGFLinearZ,
        GBufferType_PrevSVGFLinearZ,
        GBufferType_SVGFMoVec,
        GBufferType_SVGFCompact,

        GBufferType_Num
    };

    static constexpr const char* GBufferTypeStrings[GBufferType_Num] =
    {
        "Positions",
        "Normals",
        "TexCoordsAnd Depth",
        "Albedo",
        "Current SVGFLinearZ",
        "Previous SVGFLinearZ",
        "SVGFMoVec",
        "SVGFCompact",
    };
    
    enum DirectIndirectBufferType
    {
        DirectIndirectBufferType_Results,
        DirectIndirectBufferType_Albedo,

        DirectIndirectBufferType_Num
    };

    static constexpr const char* DirectIndirectBufferTypeStrings[DirectIndirectBufferType_Num] =
    {
        "Results",
        "Albedo",
    };

    enum ReprojBufferType
    {
        ReprojBufferType_Direct,
        ReprojBufferType_Indirect,
        ReprojBufferType_Moments,
        ReprojBufferType_History,

        ReprojBufferType_Num
    };

    static constexpr const char* ReprojBufferTypeStrings[ReprojBufferType_Num] =
    {
        "Reproj Direct",
        "Reproj Indirect",
        "Reproj Moments",
        "Reproj History",
    };

    enum RaytracingShaderType
    {
        RaytracingShaderType_AOMiss = 0,
        RaytracingShaderType_AOHitgroup,

        RaytracingShaderType_AreaLightMiss,
        RaytracingShaderType_AreaLightHitGroup,

        RaytracingShaderType_ShadeMiss,
        RaytracingShaderType_ShadeHitGroup,

        RaytracingShaderType_Num
    };

    struct UserInputData
    {
        float   Forward                         = 0;
        float   Backward                        = 0;
        float   Left                            = 0;
        float   Right                           = 0;
        float   Up                              = 0;
        float   Down                            = 0;
        int     PrevMouseX                      = 0;
        int     PrevMouseY                      = 0;
        int     MouseDx                         = 0;
        int     MouseDy                         = 0;
        bool    LeftMouseButtonPressed          = false;
        bool    ShiftKeyPressed                 = false;

        int     SampleScene                     = SceneMesh;
        float   VertFov                         = 40.f;

        int     CpuNumSamplesPerRay             = 500;
        int     CpuMaxScatterDepth              = 50;
        int     CpuNumThreads                   = 4;

        int     GpuRayRecursionDepth            = 3;
        int     GpuNumRaysPerPixel              = 1;
        float   GpuAORadius                     = 100.0f;
        bool    GpuCameraJitter                 = false;
        float   GpuDirectLightMult              = 1.00f;
        float   GpuIndirectLightMult            = 0.35f;
        bool    GpuEnableToneMapping            = true;
        int     GpuDenoiseFilterIterations      = 1;
        int     GpuDenoiseFeedbackTap           = 1;
        float   GpuDenoiseAlpha                 = 0.025f;
        float   GpuDenoiseMomentsAlpha          = 0.90f;
        float   GpuDenoisePhiColor              = 0.25f;
        float   GpuDenoisePhiNormal             = 128.0f;
        float   GpuNearPlane                    = 0.1f;
        float   GpuFarPlane                     = 5000.0f;
    };

    typedef std::uniform_real_distribution<float> RandFloatDist;

private:

    AppState                        TheAppState;
    Core::Raytracer*                TheRaytracer;
    Core::WorldScene*               TheWorldScene;
    RealtimeEngine::RealtimeScene*  TheRenderScene;
    UserInputData                   TheUserInputData;
    std::thread*                    TheLoadingThread;

    float                           CurrentDeltaTime;
    int                             FrameCount;
    int                             AccumCount;
    int                             SelectedBufferIndex;
    int                             CpuResultsBufferIndex;
    bool                            IsCameraDirty;
    bool                            LoadSceneRequested;
    int                             MaxNumCpuThreads;

    RandFloatDist                   RandDist;
    std::mt19937                    RandGen;

    DXGI_FORMAT                     BackbufferFormat;
    DXGI_FORMAT                     GBufferRTTypes[GBufferType_Num];
    DXGI_FORMAT                     DirectIndirectRTBufferType;
    DXGI_FORMAT                     ReprojBufferRTTypes[ReprojBufferType_Num];
    DXGI_FORMAT                     CPURaytracerTexType;

    DescriptorHeapCollection        RendererDescriptorHeapCollection;
    DescriptorHeapStack*            RendererDescriptorHeap;
    PingPongDescriptor              ReprojDescriptors[ReprojBufferType_Num];
    PingPongDescriptor              DenoiseDirectOutputDescriptor;
    PingPongDescriptor              DenoiseIndirectOutputDescriptor;

    ColorTarget                     CPURaytracerTex;
    ColorTarget                     GBuffers[GBufferType_Num];
    ColorTarget                     DirectLightingBuffer[DirectIndirectBufferType_Num];
    ColorTarget                     IndirectLightingBuffer[DirectIndirectBufferType_Num];
    ColorTarget                     ReprojectionBuffers[2][ReprojBufferType_Num];
    ColorTarget                     DenoiseDirectOutputBuffer[2];
    ColorTarget                     DenoiseIndirectOutputBuffer[2];

    RootSignature                   RasterRootSignature;
    GraphicsPSO                     RasterGeometryPassPSO;
    GraphicsPSO                     RasterCompositePassPSO;
    uint32_t                        RasterDescriptorIndexStart;
    UploadBuffer                    RasterSceneConstantBuffer;
    UploadBuffer                    RasterCompositeConstantBuffer;
    
    RootSignature                   RaytracingGlobalRootSig;
    RootSignature                   RaytracingLocalRootSig;
    RaytracingPSO*                  RaytracingPSOPtr;
    UploadBuffer                    RaytracingSceneConstantBuffer;
    int32_t                         RaytracingShaderIndex[RaytracingShaderType_Num];
    uint32_t                        RaytracingGlobalSigDataIndexStart;
    uint32_t                        RaytracingLocalSigDataIndexStart;

    RootSignature                   DenoiseRootSig;
    ComputePSO                      DenoiseReprojectPSO;
    ComputePSO                      DenoiseFilterMomentsPSO;
    ComputePSO                      DenoiseAtrousPSO;
    uint32_t                        DenoiseDescriptorIndexStart;
    UploadBuffer                    DenoiseShaderConstantBuffer;
};
