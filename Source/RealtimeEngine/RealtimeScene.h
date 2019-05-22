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
#include "RealtimeCamera.h"
#include "RaytracingGeometry.h"
#include "RealtimeSceneShaderInclude.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    // ----------------------------------------------------------------------------------------------------------------------------

    ALIGN_BEGIN(16)
    struct RealtimeSceneVertexEx : public RealtimeSceneVertex
    {
        RealtimeSceneVertexEx(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT2& texCoord)
        {
            Position = position;
            Normal   = normal;
            TexCoord = texCoord;
        }

        RealtimeSceneVertexEx(DirectX::FXMVECTOR position, DirectX::FXMVECTOR normal, DirectX::FXMVECTOR textureCoordinate)
        {
            XMStoreFloat3(&this->Position, position);
            XMStoreFloat3(&this->Normal, normal);
            XMStoreFloat2(&this->TexCoord, textureCoordinate);
        }

        static const int                        InputElementCount = 3;
        static const D3D12_INPUT_ELEMENT_DESC   InputElements[InputElementCount];
    };
    ALIGN_END

    // ----------------------------------------------------------------------------------------------------------------------------

    struct RealtimeSceneNode
    {
        RealtimeSceneNode() : Hitable(nullptr), DiffuseTexture(nullptr) {}
        ~RealtimeSceneNode() {}

        uint32_t                                InstanceId;
        const Core::IHitable*                   Hitable;
        std::vector<RealtimeSceneVertexEx>      Vertices;
        std::vector<uint32_t>                   Indices;
        RealtimeEngine::StructuredBuffer        VertexBuffer;
        RealtimeEngine::StructuredBuffer        IndexBuffer;
        RealtimeEngine::StructuredBuffer        InstanceDataBuffer;
        RealtimeEngine::StructuredBuffer        MaterialBuffer;
        DirectX::XMMATRIX                       WorldMatrix;
        RenderMaterial                          Material;
        RealtimeEngine::Texture*                DiffuseTexture;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class RealtimeScene
    {
    public:

        RealtimeScene(Core::WorldScene* worldScene);
        ~RealtimeScene();

        void                                UpdateCamera(float newVertFov, float forwardAmount, float strafeAmount, float upDownAmount, int mouseDx, int mouseDy, Core::Camera& worldCamera);
        std::vector<RealtimeSceneNode*>&    GetRenderSceneList();
        RealtimeCamera&                     GetCamera();
        RaytracingGeometry*                 GetRaytracingGeometry();
        const std::vector<PointLight>&      GetPointLights();

    private:

        void                                GenerateRenderListFromWorld(const Core::IHitable* currentHead, RealtimeEngine::Texture* defaultTexture, std::vector<RealtimeSceneNode*>& outSceneList, 
                                                std::vector<SpotLight>& spotLightsList, std::vector<DirectX::XMMATRIX>& matrixStack, std::vector<bool>& flipNormalStack);

    private:

        RealtimeCamera                  TheRenderCamera;
        std::vector<RealtimeSceneNode*> RenderSceneList;
        std::vector<PointLight>         PointLightsList;
        RaytracingGeometry*             RaytracingGeom;
    };
}
