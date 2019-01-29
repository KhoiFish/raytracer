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
    virtual void OnMouseMoved(MouseMotionEventArgs& e) override;
    virtual void OnMouseWheel(MouseWheelEventArgs& e) override;
    virtual void OnResize(ResizeEventArgs& e) override;

private:

    int  InitShadowmap(int width, int height);
    void RenderShadowmaps(std::shared_ptr<CommandList>& commandList);
    void RenderPreviewObjects(std::shared_ptr<CommandList>& commandList, const DirectX::XMMATRIX& viewMatrix, const DirectX::XMMATRIX& projectionMatrix, bool shadowmapRender);

    void OnResizeRaytracer();
    void Raytrace(bool enable);
    void NextRenderMode();
    void LoadScene(std::shared_ptr<CommandList> commandList);
    void GenerateRenderListFromWorld(std::shared_ptr<CommandList> commandList, const IHitable* currentHead, 
        std::vector<RenderSceneNode*>& outSceneList, std::vector<DirectX::XMMATRIX>& matrixStack, std::vector<bool>& flipNormalStack);

    static void OnRaytraceComplete(Raytracer* tracer, bool actuallyFinished);

private:

    enum RenderingMode
    {
        ModePreview = 0,
        ModeRaytracer,

        MaxRenderModes
    };

    typedef Microsoft::WRL::ComPtr<ID3D12PipelineState> DX12PipeState;
    using super = Game;

    typedef std::vector<RenderSceneNode*> RenderNodeList;

private:

    RenderingMode              RenderMode;
    bool                       WireframeViewEnabled;

    Raytracer*                 TheRaytracer;
    Camera                     RaytracerCamera;
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
    CameraDX12                 RenderCamera;

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

    int                        BackbufferWidth;
    int                        BackbufferHeight;
};