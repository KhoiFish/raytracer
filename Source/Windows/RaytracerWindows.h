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

#include <DirectXMath.h>
#include "Core/Raytracer.h"
#include "Core/Camera.h"
#include "CameraDX12.h"

#include <Game.h>
#include <IndexBuffer.h>
#include <Window.h>
#include <Mesh.h>
#include <RenderTarget.h>
#include <RootSignature.h>
#include <Texture.h>
#include <VertexBuffer.h>

#include "ShaderStructs.h"

#include <stack>
#include <inttypes.h>

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderSceneNode;

// ----------------------------------------------------------------------------------------------------------------------------

class RaytracerWindows : public Game
{
public:

    RaytracerWindows(const std::wstring& name, bool vSync = false);
    virtual ~RaytracerWindows();

    virtual bool LoadContent() override;
    virtual void UnloadContent() override;

protected:

    virtual void OnUpdate(UpdateEventArgs& e) override;
    virtual void OnRender(RenderEventArgs& e) override;
    virtual void OnKeyPressed(KeyEventArgs& e) override;
    virtual void OnKeyReleased(KeyEventArgs& e) override;
    virtual void OnMouseButtonPressed(MouseButtonEventArgs& e) override;
    virtual void OnMouseButtonReleased(MouseButtonEventArgs& e) override;
    virtual void OnMouseMoved(MouseMotionEventArgs& e) override;
    virtual void OnMouseWheel(MouseWheelEventArgs& e) override;
    virtual void OnResize(ResizeEventArgs& e) override;

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

    typedef Microsoft::WRL::ComPtr<ID3D12PipelineState> DX12PipeState;
    using super = Game;

    typedef std::vector<RenderSceneNode*> RenderNodeList;

private:

    int  InitShadowmap(int width, int height);
    void RenderShadowmaps(std::shared_ptr<CommandList>& commandList);
    void RenderPreviewObjects(std::shared_ptr<CommandList>& commandList, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projectionMatrix, bool shadowmapRender);
    
    void OnGUI();
    void OnResizeRaytracer();
    void Raytrace(bool enable);
    void NextRenderMode();
    void LoadScene(std::shared_ptr<CommandList> commandList);

    static void OnRaytraceComplete(Raytracer* tracer, bool actuallyFinished);

private:

    RenderingMode              RenderMode;
    bool                       WireframeViewEnabled;

    Raytracer*                 TheRaytracer;
    WorldScene*                Scene;
    RenderNodeList             RenderSceneList;

    Texture                    CPURaytracerTex;
    Texture                    PreviewTex;
    Texture                    WhiteTex;

    DXGI_FORMAT                BackBufferFormat;
    DXGI_FORMAT                DepthBufferFormat;
    DXGI_FORMAT                ShadowmapFormat;
    DXGI_SAMPLE_DESC           SampleDesc;
    DXGI_SAMPLE_DESC           ShadowmapSampleDesc;
    RenderTarget               DisplayRenderTarget;
    RootSignature              RootSignature;

    DX12PipeState              FullscreenPipelineState;
    DX12PipeState              PreviewPipelineState;
    DX12PipeState              WireframePreviewPipelineState;
    DX12PipeState              ShadowmapPipelineState;

    D3D12_VIEWPORT             Viewport;
    D3D12_RECT                 ScissorRect;
    RenderCamera                 RenderCamera;

    std::vector<SpotLight>     SpotLightsList;
    std::vector<DirLight>      DirLightsList;

    int                        ShadowmapWidth, ShadowmapHeight;
    std::vector<RenderTarget*> ShadowmapRTList;
    DirectX::XMMATRIX*         shadowViewProj;

    float                      Forward;
    float                      Backward;
    float                      Left;
    float                      Right;
    float                      Up;
    float                      Down;
    int                        MouseDx;
    int                        MouseDy;

    bool                       AllowFullscreenToggle;
    bool                       ShiftKeyPressed;
    bool                       ShowHelperWindow;
    bool                       LoadSceneRequested;

    int                        BackbufferWidth;
    int                        BackbufferHeight;
};