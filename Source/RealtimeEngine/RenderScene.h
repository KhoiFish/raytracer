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
#include <DX12/d3d12.h>
#include <vector>
#include "RealtimeEngine/RTTexture.h"
#include "Core/IHitable.h"
#include "Core/Camera.h"
#include "Core/WorldScene.h"
#include "RealtimeEngine/GpuBuffer.h"
#include "RenderCamera.h"
#include "ShaderStructs.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
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

    struct RenderSceneNode
    {
        RenderSceneNode() : Hitable(nullptr), DiffuseTexture(nullptr) {}
        ~RenderSceneNode() {}

        const Core::IHitable*                   Hitable;
        std::vector<RenderVertex>               Vertices;
        std::vector<uint32_t>                   Indices;
        RealtimeEngine::StructuredBuffer        VertexBuffer;
        RealtimeEngine::StructuredBuffer        IndexBuffer;
        DirectX::XMMATRIX                       WorldMatrix;
        RenderMaterial                          Material;
        const RealtimeEngine::Texture*          DiffuseTexture;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class RenderScene
    {
    public:

        RenderScene(Core::WorldScene* worldScene);
        ~RenderScene();

        void                            UpdateCamera(float newVertFov, float forwardAmount, float strafeAmount, float upDownAmount, int mouseDx, int mouseDy, Core::Camera& worldCamera);
        std::vector<RenderSceneNode*>&  GetRenderSceneList();
        RenderCamera&                   GetRenderCamera();

    private:

        void                            GenerateRenderListFromWorld(const Core::IHitable* currentHead, const RealtimeEngine::Texture* defaultTexture, std::vector<RenderSceneNode*>& outSceneList, 
                                            std::vector<SpotLight>& spotLightsList, std::vector<DirectX::XMMATRIX>& matrixStack, std::vector<bool>& flipNormalStack);

        void                            SetupForRaytracing();

    private:

        RenderCamera                    TheRenderCamera;
        std::vector<RenderSceneNode*>   RenderSceneList;
        std::vector<SpotLight>          SpotLightsList;

        GpuBuffer*                      TLASBuffer;
        std::vector<GpuBuffer*>         BLASBuffers;
        WrappedGPUPointer               TLASPointer;
        GpuBuffer*                      ScratchBuffer;
        ByteAddressBuffer               InstanceDataBuffer;
        GpuBuffer*                      TransformDataBuffer;
    };
}
