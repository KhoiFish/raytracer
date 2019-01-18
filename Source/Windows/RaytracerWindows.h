#pragma once

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

#include <DirectXMath.h>

// ----------------------------------------------------------------------------------------------------------------------------

class RaytracerWindows : public Game
{
public:

    RaytracerWindows(Raytracer* tracer, const Camera& cam, IHitable* world, const std::wstring& name, int width, int height, bool vSync = false);
    virtual ~RaytracerWindows();

    virtual bool LoadContent() override;
    virtual void UnloadContent() override;

protected:

    virtual void OnUpdate(UpdateEventArgs& e) override;
    virtual void OnRender(RenderEventArgs& e) override;
    virtual void OnKeyPressed(KeyEventArgs& e) override;
    virtual void OnKeyReleased(KeyEventArgs& e);
    virtual void OnMouseMoved(MouseMotionEventArgs& e);
    virtual void OnMouseWheel(MouseWheelEventArgs& e) override;
    virtual void OnResize(ResizeEventArgs& e) override;

private:

    struct alignas(16) CameraData
    {
        DirectX::XMVECTOR InitialCamPos;
        DirectX::XMVECTOR InitialCamRot;
    };

    typedef Microsoft::WRL::ComPtr<ID3D12PipelineState> DX12PipeState;
    using super = Game;

private:

    Raytracer*      Tracer; 
    Camera          RaytracerCamera;
    IHitable*       World;

    Texture         CPURaytracerFrame;
    RenderTarget    RenderTarget;
    RootSignature   RootSignature;
    DX12PipeState   PipelineState;

    D3D12_VIEWPORT  Viewport;
    D3D12_RECT      ScissorRect;
    CameraDX12      RenderCamera;
    CameraData*     PtrAlignedCameraData;

    float           Forward;
    float           Backward;
    float           Left;
    float           Right;
    float           Up;
    float           Down;
    float           Pitch;
    float           Yaw;

    bool            AllowFullscreenToggle;
    bool            ShiftKeyPressed;

    int             Width;
    int             Height;
};