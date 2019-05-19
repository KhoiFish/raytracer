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
    virtual void    OnInit();
    virtual void    OnKeyDown(UINT8 key);
    virtual void    OnKeyUp(uint8_t key);
    virtual void    OnMouseMove(uint32_t x, uint32_t y);
    virtual void    OnLeftButtonDown(uint32_t x, uint32_t y);
    virtual void    OnLeftButtonUp(uint32_t x, uint32_t y);
    virtual void    OnUpdate(float dtSeconds);
    virtual void    OnRender();
    virtual void    OnSizeChanged(uint32_t width, uint32_t height, bool minimized);
    virtual void    OnDestroy();
    virtual bool    OverrideImguiInput();

private:

    void            SetupRealtimeRaytracingPipeline();
    void            SetupRealtimePipeline();
    void            SetupRenderBuffers();
    void            SetupGui();
    void            LoadScene();
    void            OnResizeRaytracer();
    void            ToggleCpuRaytracer();
    void            SetEnableCpuRaytrace(bool enable);
    static void     OnCpuRaytraceComplete(Core::Raytracer* tracer, bool actuallyFinished);
    void            RenderSceneList(GraphicsContext& renderContext);
    void            RenderGeometryPass();
    void            RenderCompositePass();
    void            RenderGui();
    void            ComputeRaytracingResults();
    void            SetupSceneConstantBuffer(const FXMMATRIX& model, SceneConstantBuffer& sceneCB);

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
        UserInputData()
            : Forward(0), Backward(0), Left(0), Right(0), Up(0), Down(0),
            MouseDx(0), MouseDy(0), PrevMouseX(0), PrevMouseY(0),
            ShiftKeyPressed(false), LeftMouseButtonPressed(false) {}

        float   Forward;
        float   Backward;
        float   Left;
        float   Right;
        float   Up;
        float   Down;
        int     PrevMouseX;
        int     PrevMouseY;
        int     MouseDx;
        int     MouseDy;
        bool    LeftMouseButtonPressed;
        bool    ShiftKeyPressed;
        bool    InputDirty;
    };

private:

    Core::Raytracer*                TheRaytracer;
    Core::WorldScene*               TheWorldScene;
    RealtimeEngine::RealtimeScene*  TheRenderScene;

    int                             FrameCount;
    int                             AccumCount;
    int                             MaxRayRecursionDepth;
    int                             NumRaysPerPixel;
    float                           AORadius;
    int                             SelectedBufferIndex;
    int                             CpuResultsBufferIndex;
    bool                            LoadSceneRequested;

    DXGI_FORMAT                     BackbufferFormat;
    DXGI_FORMAT                     DeferredBuffersRTTypes[DeferredBufferType_Num];
    DXGI_FORMAT                     RaytracingBufferType;
    DXGI_FORMAT                     CPURaytracerTexType;

    ColorBuffer                     CPURaytracerTex;
    ColorBuffer                     DeferredBuffers[DeferredBufferType_Num];
    ColorBuffer                     AmbientOcclusionOutput[2];

    RootSignature                   FullscreenQuadRootSignature;
    GraphicsPSO                     FullscreenPipelineState;

    RootSignature                   RealtimeRootSignature;
    GraphicsPSO                     RealtimeGeometryPassPSO;
    GraphicsPSO                     RealtimeCompositePassPSO;
    DescriptorHeapStack*            RealtimeDescriptorHeap;

    RootSignature                   RaytracingGlobalRootSig;
    RootSignature                   RaytracingLocalRootSig;
    RaytracingPSO                   TheRaytracingPSO;
    DescriptorHeapStack*            RaytracingDescriptorHeap;
    ByteAddressBuffer               RaytracingSceneConstantBuffer;

    UserInputData                   UserInput;
};
