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
#include "SampleScenes.h"

#include "RealtimeEngine/Globals.h"
#include "RealtimeEngine/RenderInterface.h"
#include "RealtimeEngine/RootSignature.h"
#include "RealtimeEngine/PipelineStateObject.h"
#include "RealtimeEngine/ColorBuffer.h"


using namespace yart;

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
    virtual void               OnUpdate();
    virtual void               OnRender();
    virtual void               OnSizeChanged(UINT width, UINT height, bool minimized);
    virtual void               OnDestroy();

private:

    void                       Raytrace(bool enable);
    void                       OnResizeRaytracer();
    static void                OnRaytraceComplete(Raytracer* tracer, bool actuallyFinished);

    void                       CreateDeviceDependentResources();
    void                       CreateWindowSizeDependentResources();
    void                       ReleaseDeviceDependentResources();
    void                       ReleaseWindowSizeDependentResources();

private:

    enum RenderingMode
    {
        ModePreview = 0,
        ModeRaytracer,

        MaxRenderModes
    };

    static constexpr const char* RenderingModeStrings[MaxRenderModes] =
    {
        "DirectX 12 Render",
        "Software Tracing",
    };

private:

    RenderingMode              RenderMode;
    bool                       WireframeViewEnabled;

    Raytracer*                 TheRaytracer;
    WorldScene*                Scene;
    //RenderNodeList             RenderSceneList;

    //Texture                    CPURaytracerTex;
    //Texture                    PreviewTex;
    //Texture                    WhiteTex;

    ColorBuffer                SceneColorBuffer;
    RootSignature              MainRootSignature;

    GraphicsPSO                FullscreenPipelineState;
    GraphicsPSO                PreviewPipelineState;
    GraphicsPSO                WireframePreviewPipelineState;
    GraphicsPSO                ShadowmapPipelineState;
    GfxViewport                Viewport;
    GfxRect                    ScissorRect;

    float                      Forward;
    float                      Backward;
    float                      Left;
    float                      Right;
    float                      Up;
    float                      Down;
    int                        MouseDx;
    int                        MouseDy;
};

