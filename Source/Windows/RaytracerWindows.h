#pragma once

#include "CameraDX12.h"
#include "Light.h"
#include <Game.h>
#include <IndexBuffer.h>
#include <Window.h>
#include <Mesh.h>
#include <RenderTarget.h>
#include <RootSignature.h>
#include <Texture.h>
#include <VertexBuffer.h>

#include <DirectXMath.h>

class RaytracerWindows : public Game
{
public:
    using super = Game;

    RaytracerWindows(const std::wstring& name, int width, int height, bool vSync = false);
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

    // Some geometry to render.
    std::unique_ptr<Mesh> m_CubeMesh;
    std::unique_ptr<Mesh> m_SphereMesh;
    std::unique_ptr<Mesh> m_ConeMesh;
    std::unique_ptr<Mesh> m_TorusMesh;
    std::unique_ptr<Mesh> m_PlaneMesh;

    Texture m_DefaultTexture;
    Texture m_DirectXTexture;
    Texture m_EarthTexture;
    Texture m_MonaLisaTexture;

    // Render target
    RenderTarget m_RenderTarget;

    // Root signature
    RootSignature m_RootSignature;

    // Pipeline state object.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    CameraDX12 m_Camera;
    struct alignas(16) CameraData
    {
        DirectX::XMVECTOR m_InitialCamPos;
        DirectX::XMVECTOR m_InitialCamRot;
    };
    CameraData* m_pAlignedCameraData;

    // Camera controller
    float m_Forward;
    float m_Backward;
    float m_Left;
    float m_Right;
    float m_Up;
    float m_Down;

    float m_Pitch;
    float m_Yaw;

    // Rotate the lights in a circle.
    bool m_AnimateLights;
    // Set to true if the Shift key is pressed.
    bool m_Shift;

    int m_Width;
    int m_Height;

    // Define some lights.
    std::vector<PointLight> m_PointLights;
    std::vector<SpotLight> m_SpotLights;
};