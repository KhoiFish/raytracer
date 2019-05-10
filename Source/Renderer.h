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
#include "Windows/RenderCamera.h"
#include "RenderScene.h"

using namespace RealtimeEngine;
using namespace DirectX;

// ----------------------------------------------------------------------------------------------------------------------------

class Renderer : public RenderInterface
{
public:

    Renderer(uint32_t width, uint32_t height);
    virtual ~Renderer();

public:

    virtual void               OnDeviceLost() override;
    virtual void               OnDeviceRestored() override;

    virtual void               OnInit();
    virtual void               OnKeyDown(UINT8 key);
    virtual void               OnMouseMove(uint32_t x, uint32_t y);
    virtual void               OnLeftButtonDown(uint32_t x, uint32_t y);
    virtual void               OnUpdate();
    virtual void               OnRender();
    virtual void               OnSizeChanged(uint32_t width, uint32_t height, bool minimized);
    virtual void               OnDestroy();

private:

    void                       SetupFullscreenQuadPipeline();
    void                       SetupRealtimePipeline();
    void                       LoadScene();
    void                       Raytrace(bool enable);
    void                       OnResizeRaytracer();
    static void                OnRaytraceComplete(Core::Raytracer* tracer, bool actuallyFinished);
    void                       RenderSceneList(GraphicsContext& renderContext, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
    void                       SetupRenderBuffers();

private:

    enum RenderingMode
    {
        RenderingMode_Realtime = 0,
        RenderingMode_Cpu,

        MaxRenderModes
    };

    static constexpr const char* RenderingModeStrings[MaxRenderModes] =
    {
        "Realtime Rendering",
        "CPU Raytracing",
    };

    struct UserInputData
    {
        UserInputData() : Forward(0), Backward(0), Left(0), Right(0), Up(0), Down(0), MouseDx(0), MouseDy(0) {}

        float   Forward;
        float   Backward;
        float   Left;
        float   Right;
        float   Up;
        float   Down;
        int     MouseDx;
        int     MouseDy;
    };

    enum DeferredBufferType
    {
        DeferredBufferType_Position = 0,
        DeferredBufferType_Normal,
        DeferredBufferType_TexCoord,
        DeferredBufferType_Diffuse,

        DeferredBufferType_Num
    };

    static constexpr const char* DeferredBufferTypeStrings[DeferredBufferType_Num] =
    {
        "Position Buffer",
        "Normal Buffer",
        "TexCoord Buffer",
        "Diffuse Buffer",
    };

private:

    RenderingMode                   RenderMode;
    Core::Raytracer*                TheRaytracer;
    Core::WorldScene*               TheWorldScene;
    std::vector<RenderSceneNode*>   TheRealtimeSceneList;
    RenderCamera                    TheRenderCamera;

    ColorBuffer                     CPURaytracerTex;
    RootSignature                   FullscreenQuadRootSignature;
    GraphicsPSO                     FullscreenPipelineState;

    DXGI_FORMAT                     DeferredBuffersRTTypes[DeferredBufferType_Num];
    DXGI_FORMAT                     ZBufferRTType;
    ColorBuffer                     DeferredBuffers[DeferredBufferType_Num];
    ColorBuffer                     ZPrePassBuffer;
    RootSignature                   RealtimeRootSignature;
    GraphicsPSO                     RealtimeZPrePassPSO;
    GraphicsPSO                     RealtimeGeometryPassPSO;

    UserInputData                   UserInput;
};
