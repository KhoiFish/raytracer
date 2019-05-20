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
#include "RealtimeEngine/ColorBuffer.h"
#include "RealtimeEngine/RealtimeCamera.h"
#include "RealtimeEngine/RealtimeScene.h"
#include "RealtimeEngine/DescriptorHeap.h"
#include "RendererShaderInclude.h"

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

    virtual void    OnDeviceLost() override;
    virtual void    OnDeviceRestored() override;
    virtual void    OnInit() override;
    virtual void    OnKeyDown(UINT8 key) override;
    virtual void    OnKeyUp(uint8_t key) override;
    virtual void    OnMouseMove(uint32_t x, uint32_t y) override;
    virtual void    OnLeftButtonDown(uint32_t x, uint32_t y) override;
    virtual void    OnLeftButtonUp(uint32_t x, uint32_t y) override;
    virtual void    OnUpdate(float dtSeconds) override;
    virtual void    OnRender() override;
    virtual void    OnSizeChanged(uint32_t width, uint32_t height, bool minimized) override;
    virtual void    OnDestroy() override;
    virtual bool    OverrideImguiInput() override;

private:

    void            SetupRealtimeRaytracingPipeline();
    void            SetupRealtimePipeline();
    void            SetupRenderBuffers();
    void            SetupGui();
    void            LoadScene();

    void            OnResizeCpuRaytracer();
    void            OnResizeGpuRaytracer();
    void            OnResizeRealtimeRenderer();

    void            CleanupGpuRaytracer();
    void            CleanupRealtimeRender();

    void            ToggleCpuRaytracer();
    static void     OnCpuRaytraceComplete(Core::Raytracer* tracer, bool actuallyFinished);

    void            SetEnableCpuRaytrace(bool enable);
    void            SetCameraDirty();
    void            SetupSceneConstantBuffer(const FXMMATRIX& model, SceneConstantBuffer& sceneCB);

    void            RenderSceneList(GraphicsContext& renderContext);
    void            RenderGeometryPass();
    void            RenderCompositePass();
    void            RenderGui();
    void            RenderGpuRaytracing(); 

    const char*     GetSelectedBufferName();

private:

    enum DeferredBufferType
    {
        DeferredBufferType_Position = 0,
        DeferredBufferType_Normal,
        DeferredBufferType_TexCoordAndDepth,
        DeferredBufferType_Albedo,

        DeferredBufferType_Num
    };

    static constexpr const char* DeferredBufferTypeStrings[DeferredBufferType_Num] =
    {
        "Positions",
        "Normals",
        "TexCoordsAnd Depth",
        "Albedo",
    };

    static constexpr const char* DefaultTextureName = "RuntimeData/guitar.jpg";

    struct UserInputData
    {
        float   Forward                 = 0;
        float   Backward                = 0;
        float   Left                    = 0;
        float   Right                   = 0;
        float   Up                      = 0;
        float   Down                    = 0;
        int     PrevMouseX              = 0;
        int     PrevMouseY              = 0;
        int     MouseDx                 = 0;
        int     MouseDy                 = 0;
        bool    LeftMouseButtonPressed  = false;
        bool    ShiftKeyPressed         = false;

        int     SampleScene             = SceneMesh;
        float   VertFov                 = 40.f;

        int     CpuNumSamplesPerRay     = 500;
        int     CpuMaxScatterDepth      = 50;
        int     CpuNumThreads           = 4;

        int     GpuMaxRayRecursionDepth = 1;
        int     GpuNumRaysPerPixel      = 25;
        float   GpuAORadius             = 400.0f;
        bool    GpuCameraJitter         = true;
    };

private:

    Core::Raytracer*                TheRaytracer;
    Core::WorldScene*               TheWorldScene;
    RealtimeEngine::RealtimeScene*  TheRenderScene;
    UserInputData                   UserInput;

    int                             FrameCount;
    int                             AccumCount;
    int                             SelectedBufferIndex;
    int                             CpuResultsBufferIndex;
    bool                            IsCameraDirty;
    bool                            LoadSceneRequested;
    int                             MaxNumCpuThreads;

    DXGI_FORMAT                     BackbufferFormat;
    DXGI_FORMAT                     DeferredBuffersRTTypes[DeferredBufferType_Num];
    DXGI_FORMAT                     RaytracingBufferType;
    DXGI_FORMAT                     CPURaytracerTexType;

    ColorBuffer                     CPURaytracerTex;
    ColorBuffer                     DeferredBuffers[DeferredBufferType_Num];
    ColorBuffer                     AmbientOcclusionOutput[2];

    RootSignature                   RealtimeRootSignature;
    GraphicsPSO                     RealtimeGeometryPassPSO;
    GraphicsPSO                     RealtimeCompositePassPSO;
    DescriptorHeapStack*            RealtimeDescriptorHeap;

    RootSignature                   RaytracingGlobalRootSig;
    RootSignature                   RaytracingLocalRootSig;
    RaytracingPSO                   TheRaytracingPSO;
    DescriptorHeapStack*            RaytracingGlobalDescriptorHeap;
    DescriptorHeapStack*            RaytracingLocalDescriptorHeap;
    ByteAddressBuffer               RaytracingSceneConstantBuffer;
};
