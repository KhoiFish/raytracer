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
#include "RealtimeEngine/RTTexture.h"
#include "Windows/ShaderStructs.h"
#include <DirectXMath.h>

using namespace RealtimeEngine;
using namespace DirectX;

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderVertex
{
    RenderVertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT2& texCoord)
        : Position(position)
        , Normal(normal)
        , TexCoord(texCoord)
    {}

    RenderVertex(DirectX::FXMVECTOR position, DirectX::FXMVECTOR normal, DirectX::FXMVECTOR textureCoordinate)
    {
        XMStoreFloat3(&this->Position, position);
        XMStoreFloat3(&this->Normal, normal);
        XMStoreFloat2(&this->TexCoord, textureCoordinate);
    }

    static const int                        InputElementCount = 3;
    static const D3D12_INPUT_ELEMENT_DESC   InputElements[InputElementCount];
    DirectX::XMFLOAT3                       Position;
    DirectX::XMFLOAT3                       Normal;
    DirectX::XMFLOAT2                       TexCoord;
};

// ----------------------------------------------------------------------------------------------------------------------------

using VertexBuffer = std::vector<RenderVertex>;
using IndexBuffer  = std::vector<uint32_t>;

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderSceneNode
{
    RenderSceneNode() : Hitable(nullptr), DiffuseTexture(nullptr) {}

    const Core::IHitable*           Hitable;
    VertexBuffer                    VertexData;
    IndexBuffer                     IndexData;
    XMMATRIX                        WorldMatrix;
    RenderMaterial                  Material;
    const RealtimeEngine::Texture*  DiffuseTexture;
};

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
    virtual void               OnSizeChanged(uint32_t width, uint32_t height, bool minimized);
    virtual void               OnDestroy();

private:

    void                       SetupFullscreenQuadPipeline();
    void                       LoadScene();
    void                       Raytrace(bool enable);
    void                       OnResizeRaytracer();
    static void                OnRaytraceComplete(Core::Raytracer* tracer, bool actuallyFinished);

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
        float Forward;
        float Backward;
        float Left;
        float Right;
        float Up;
        float Down;
        int   MouseDx;
        int   MouseDy;
    };

private:

    RenderingMode                   RenderMode;
    Core::Raytracer*                TheRaytracer;
    Core::WorldScene*               Scene;
    std::vector< RenderSceneNode*>  RealtimeSceneList;

    ColorBuffer                     CPURaytracerTex;
    RootSignature                   FullscreenQuadRootSignature;
    GraphicsPSO                     FullscreenPipelineState;

    UserInputData                   UserInput;
};
