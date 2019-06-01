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

#include "Globals.h"
#include <DirectXMath.h>
#include <DX12/d3d12.h>
#include <vector>
#include "RealtimeEngine/GpuBuffer.h"
#include "RealtimeSceneShaderInclude.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    class RaytracingGeometry
    {
    public:

        struct GeometryInfo
        {
            GeometryInfo(
                uint32_t instanceId,
                uint32_t instanceMask,
                uint32_t numVertices, uint32_t numIndices,
                GpuBuffer* vertexBuffer, GpuBuffer* indexBuffer,
                DirectX::XMMATRIX* pWorldMatrix)
                    : InstanceId(instanceId), InstanceMask(instanceMask),
                      NumVertices(numVertices), NumIndices(numIndices), 
                      VertexBuffer(vertexBuffer), IndexBuffer(indexBuffer),
                      WorldMatrix(pWorldMatrix)
            {}

            uint32_t            InstanceId      = 0;
            uint32_t            InstanceMask    = RAYTRACING_INSTANCEMASK_ALL;
            uint32_t            NumVertices     = 0;
            uint32_t            NumIndices      = 0;
            GpuBuffer*          VertexBuffer    = nullptr;
            GpuBuffer*          IndexBuffer     = nullptr;
            DirectX::XMMATRIX*  WorldMatrix;
        };

    public:

        RaytracingGeometry(uint32_t hitProgramCount);
        ~RaytracingGeometry();

        void                                AddGeometry(const GeometryInfo& info);
        void                                Build();
        void                                UpdateTLASTransforms(CommandContext& context);

        D3D12_GPU_VIRTUAL_ADDRESS           GetTLASVirtualAddress();
        std::vector<GeometryInfo>&          GetGeometryList();

    private:

        void                                BuildTLAS(CommandContext& context);
        void                                BuildBLAS(CommandContext& context);

    private:

        uint32_t                                    HitProgramCount;
        int32_t                                     CurrentTLASIndex;
        std::vector<GeometryInfo>                   GeometryInfoList;
        std::vector<GpuBuffer>                      BLASBuffers;
        GpuBuffer                                   TLASBuffer[2];
        GpuBuffer                                   BLASScratchBuffer;
        GpuBuffer                                   TLASScratchBuffer;
        GpuBuffer                                   InstanceDataBuffer;
        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> InstanceDescs;
    };

}
