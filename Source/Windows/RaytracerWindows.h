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

#include <stack>

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderSceneNode;

// ----------------------------------------------------------------------------------------------------------------------------

class RaytracerWindows : public Game
{
public:

    RaytracerWindows(const std::wstring& name, int width, int height, bool vSync = false);
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

    void OnResizeRaytracer();
    void Raytrace(bool enable);
    void NextRenderMode();
    void LoadScene(std::shared_ptr<CommandList> commandList);
    void GenerateRenderListFromWorld(std::shared_ptr<CommandList> commandList, const IHitable* currentHead, std::vector<RenderSceneNode*>& outSceneList, std::vector<DirectX::XMMATRIX>& matrixStack);

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

    RenderingMode       RenderMode;

    Raytracer*          TheRaytracer;
    Camera              RaytracerCamera;
    IHitable*           World;
    RenderNodeList      RenderSceneList;

    Texture             CPURaytracerTex;
    Texture             PreviewTex;
    RenderTarget        RenderTarget;
    RootSignature       RootSignature;
    DX12PipeState       FullscreenPipelineState;
    DX12PipeState       PreviewPipelineState;

    D3D12_VIEWPORT      Viewport;
    D3D12_RECT          ScissorRect;
    CameraDX12          RenderCamera;

    float               Forward;
    float               Backward;
    float               Left;
    float               Right;
    float               Up;
    float               Down;
    int                 MouseDx;
    int                 MouseDy;

    bool                AllowFullscreenToggle;
    bool                ShiftKeyPressed;

    int                 BackbufferWidth;
    int                 BackbufferHeight;
};