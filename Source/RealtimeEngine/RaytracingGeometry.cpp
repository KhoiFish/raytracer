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

#include "RaytracingGeometry.h"
#include "RenderDevice.h"
#include "CommandContext.h"
#include <algorithm>

using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeEngine::RaytracingGeometry::RaytracingGeometry()
    : TLASBuffer(nullptr)
    , ScratchBuffer(nullptr)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeEngine::RaytracingGeometry::~RaytracingGeometry()
{
    for (size_t i = 0; i < BLASBuffers.size(); i++)
    {
        delete BLASBuffers[i];
        BLASBuffers[i] = nullptr;
    }

    if (TLASBuffer != nullptr)
    {
        delete TLASBuffer;
        TLASBuffer = nullptr;
    }

    if (ScratchBuffer != nullptr)
    {
        delete ScratchBuffer;
        ScratchBuffer = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingGeometry::AddGeometry(const GeometryInfo& info)
{
    GeometryInfoList.push_back(info);
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_GPU_VIRTUAL_ADDRESS RaytracingGeometry::GetTLASVirtualAddress()
{
    return TLASBuffer->GetGpuVirtualAddress();
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingGeometry::Build()
{
    // numBottomLevels == number of scene objects
    const int numBottomLevels = (int)GeometryInfoList.size();

    // Gather info on TLAS
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuildInfo;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    tlasDesc = {};
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = tlasDesc.Inputs;
        topLevelInputs.Type             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        topLevelInputs.NumDescs         = numBottomLevels;
        topLevelInputs.Flags            = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        topLevelInputs.pGeometryDescs   = nullptr;
        topLevelInputs.DescsLayout      = D3D12_ELEMENTS_LAYOUT_ARRAY;
        RenderDevice::Get().GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &tlasPrebuildInfo);
    }

    // Gather info on BLAS
    std::vector<UINT64>                                             blasSizes(GeometryInfoList.size());
    std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> blasDescs(GeometryInfoList.size());
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>                     geometryDescs(GeometryInfoList.size());
    UINT64                                                          scratchBufferSizeNeeded = tlasPrebuildInfo.ScratchDataSizeInBytes;
    {
        // Fill in the geometry descriptions
        for (int i = 0; i < (int)GeometryInfoList.size(); i++)
        {
            GeometryInfo& geometryInfo     = GeometryInfoList[i];
            const int     offsetToPosition = 0;
            const int     offsetToIndex    = 0;

            D3D12_RAYTRACING_GEOMETRY_DESC& desc = geometryDescs[i];
            desc.Type  = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& trianglesDesc = desc.Triangles;
            trianglesDesc.VertexFormat                  = DXGI_FORMAT_R32G32B32_FLOAT;
            trianglesDesc.VertexCount                   = geometryInfo.NumVertices;
            trianglesDesc.VertexBuffer.StartAddress     = geometryInfo.VertexBuffer->GetGpuVirtualAddress() + offsetToPosition;
            trianglesDesc.VertexBuffer.StrideInBytes    = geometryInfo.VertexBuffer->GetElementSize();
            trianglesDesc.IndexFormat                   = DXGI_FORMAT_R32_UINT;
            trianglesDesc.IndexCount                    = geometryInfo.NumIndices;
            trianglesDesc.IndexBuffer                   = geometryInfo.IndexBuffer->GetGpuVirtualAddress() + offsetToIndex;
            trianglesDesc.Transform3x4                  = 0;
        }

        // Gather prebuild info on blas
        for (int i = 0; i < numBottomLevels; i++)
        {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC&   blasDesc   = blasDescs[i];
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blasInputs = blasDesc.Inputs;
            blasInputs.Type              = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            blasInputs.NumDescs          = 1;
            blasInputs.pGeometryDescs    = &geometryDescs[i];
            blasInputs.Flags             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            blasInputs.DescsLayout       = D3D12_ELEMENTS_LAYOUT_ARRAY;

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blasPrebuildInfo;
            RenderDevice::Get().GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&blasInputs, &blasPrebuildInfo);

            blasSizes[i]            = blasPrebuildInfo.ResultDataMaxSizeInBytes;
            scratchBufferSizeNeeded = std::max(blasPrebuildInfo.ScratchDataSizeInBytes, scratchBufferSizeNeeded);
        }
    }

    // Allocate scratch buffer
    ByteAddressBuffer scratchBuffer;
    scratchBuffer.Create(L"Acceleration Structure Scratch Buffer", (UINT)scratchBufferSizeNeeded, 1);

    // Allocate TLAS buffer
    TLASBuffer = new StructuredBuffer();
    TLASBuffer->Create(L"TLAS Buffer", 1, (uint32_t)tlasPrebuildInfo.ResultDataMaxSizeInBytes, nullptr, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

    // Point TLAS description to the allocated buffers
    tlasDesc.DestAccelerationStructureData    = TLASBuffer->GetGpuVirtualAddress();
    tlasDesc.ScratchAccelerationStructureData = scratchBuffer.GetGpuVirtualAddress();

    // Allocate BLAS buffers
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(numBottomLevels);
    BLASBuffers.resize(numBottomLevels);
    for (UINT i = 0; i < blasDescs.size(); i++)
    {
        // Allocate buffer
        BLASBuffers[i] = new StructuredBuffer();
        BLASBuffers[i]->Create(L"BLAS Buffer", 1, (uint32_t)blasSizes[i], nullptr, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

        // Point to buffers
        blasDescs[i].DestAccelerationStructureData    = BLASBuffers[i]->GetGpuVirtualAddress();
        blasDescs[i].ScratchAccelerationStructureData = scratchBuffer.GetGpuVirtualAddress();

        // Fill in the desc
        D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDescs[i];

        // Fill in transform
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), GeometryInfoList[i].WorldMatrix);

        // Fill in the rest
        instanceDesc.AccelerationStructure                  = BLASBuffers[i]->GetGpuVirtualAddress();
        instanceDesc.Flags                                  = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instanceDesc.InstanceID                             = i;
        instanceDesc.InstanceMask                           = GeometryInfoList[i].InstanceMask;
        instanceDesc.InstanceContributionToHitGroupIndex    = i;
    }

    // Allocate instance data buffer and update TLAS desc
    InstanceDataBuffer.Create(L"Instance Data Buffer", numBottomLevels, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instanceDescs.data());
    tlasDesc.Inputs.InstanceDescs = InstanceDataBuffer.GetGpuVirtualAddress();
    tlasDesc.Inputs.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;

    // Finally, build the acceleration structures
    GraphicsContext& context = GraphicsContext::Begin("Create Acceleration Structure");
    {
        ID3D12GraphicsCommandList4* pCommandList = context.GetCommandList();
        auto                        uavBarrier   = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
        for (UINT i = 0; i < blasDescs.size(); i++)
        {
            pCommandList->BuildRaytracingAccelerationStructure(&blasDescs[i], 0, nullptr);
            pCommandList->ResourceBarrier(1, &uavBarrier);
        }
        pCommandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
    }
    context.Finish(true);
}
