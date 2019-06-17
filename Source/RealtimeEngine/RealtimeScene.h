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
        enum Flags
        {
            Flags_AreaLightDirty = (1 << 0),
            Flags_InstanceDirty  = (1 << 1),
            Flags_MaterialDirty  = (1 << 2),
        };

        RealtimeSceneNode()
            : NodeFlags(0), Hitable(nullptr), InstanceId(0), LightIndex(-1)
            , InstanceDataHeapIndex(0), MaterialHeapIndex(0), DiffuseTextureIndex(0)
        {}

        ~RealtimeSceneNode() {}

        uint32_t                                NodeFlags;
        Core::IHitable*                         Hitable;
        uint32_t                                InstanceId;
        DirectX::XMMATRIX                       WorldMatrix;
        DirectX::XMMATRIX                       PrevWorldMatrix;

        std::vector<RealtimeSceneVertexEx>      Vertices;
        std::vector<uint32_t>                   Indices;
        RealtimeEngine::GpuBuffer               VertexBuffer;
        RealtimeEngine::GpuBuffer               IndexBuffer;

        uint32_t                                DiffuseTextureIndex;

        uint32_t                                InstanceDataHeapIndex;
        uint32_t                                MaterialHeapIndex;
        RenderMaterial                          Material;
        RealtimeEngine::GpuBuffer               InstanceDataBuffer;
        RealtimeEngine::GpuBuffer               MaterialBuffer;

        int32_t                                 LightIndex;
        RealtimeAreaLight                       AreaLight;
        RealtimeEngine::GpuBuffer               AreaLightBuffer;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class RealtimeScene
    {
    public:

        struct TextureList
        {
            void                                    Add(RealtimeEngine::Texture* pTexture)  { Textures.push_back(pTexture); }
            uint32_t                                GetCount()                              { return int32_t(Textures.size()); }

            uint32_t                                DescriptorHeapStartIndex;
            std::vector<RealtimeEngine::Texture*>   Textures;
        };

    public:

        RealtimeScene(Core::WorldScene* worldScene, uint32_t numHitPrograms);
        ~RealtimeScene();

        void                                SetupResourceViews(DescriptorHeapStack& descriptorHeap);
        void                                UpdateCamera(float nearPlane, float farPlane, float newVertFov, float forwardAmount, float strafeAmount, float upDownAmount, int mouseDx, int mouseDy, Core::Camera& worldCamera);
        void                                UpdateCamera(float nearPlane, float farPlane, float newVertFov, const Core::Vec4& eye, const Core::Vec4& target, Core::Camera& worldCamera);
        void                                UpdateAllGpuBuffers(CommandContext& context);

        std::vector<RealtimeSceneNode*>&    GetRenderSceneList();
        RealtimeCamera&                     GetCamera();
        RaytracingGeometry*                 GetRaytracingGeometry();
        std::vector<RealtimeSceneNode*>&    GetAreaLightsList();
        TextureList&                        GetDiffuseTextureList();

    private:

        void                                GenerateRenderListFromWorld(Core::IHitable* currentHead, std::vector<DirectX::XMMATRIX>& matrixStack, std::vector<bool>& flipNormalStack);
        void                                AddNewNode(RealtimeSceneNode* newNode, const DirectX::XMMATRIX& worldMatrix, Core::IHitable* pHitable);

        void                                UpdateAreaLightBuffer(CommandContext& context, RealtimeSceneNode* pNode);
        void                                UpdateInstanceBuffer(CommandContext& context, RealtimeSceneNode* pNode);
        void                                UpdateMaterialBuffer(CommandContext& context, RealtimeSceneNode* pNode);

    private:

        RealtimeCamera                      TheRenderCamera;
        std::vector<RealtimeSceneNode*>     RenderSceneList;
        std::vector<RealtimeSceneNode*>     AreaLightsList;
        TextureList                         DiffuseTextureList;
        RaytracingGeometry*                 RaytracingGeom;
        uint32_t                            ScratchCopyBufferSize;
        void*                               ScratchCopyBuffer;
    };
}
