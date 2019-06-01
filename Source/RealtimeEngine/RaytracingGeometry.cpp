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

RealtimeEngine::RaytracingGeometry::RaytracingGeometry(uint32_t hitProgramCount)
    : HitProgramCount(hitProgramCount)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeEngine::RaytracingGeometry::~RaytracingGeometry()
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingGeometry::AddGeometry(const GeometryInfo& info)
{
    GeometryInfoList.push_back(info);
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_GPU_VIRTUAL_ADDRESS RaytracingGeometry::GetTLASVirtualAddress()
{
    return TLASBuffer[CurrentTLASIndex].GetGpuVirtualAddress();
}

// ----------------------------------------------------------------------------------------------------------------------------

std::vector<RaytracingGeometry::GeometryInfo>& RaytracingGeometry::GetGeometryList()
{
    return GeometryInfoList;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingGeometry::Build()
{
    GraphicsContext& context = GraphicsContext::Begin("Build Acceleration Structures");
    {
        BuildBLAS(context);
        BuildTLAS(context);
    }
    context.Finish(true);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingGeometry::BuildBLAS(CommandContext& context)
{
     // Gather info on BLAS
    std::vector<UINT64>                                             blasSizes(GeometryInfoList.size());
    std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> blasDescs(GeometryInfoList.size());
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>                     geometryDescs(GeometryInfoList.size());
    UINT64                                                          scratchBufferSizeNeeded = 0;
    {
        // Fill in the geometry descriptions
        for (size_t i = 0; i < GeometryInfoList.size(); i++)
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
        for (size_t i = 0; i < GeometryInfoList.size(); i++)
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
    BLASScratchBuffer.Create(L"Acceleration Structure Scratch Buffer", (UINT)scratchBufferSizeNeeded, 1, nullptr, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    // Allocate BLAS buffers
    InstanceDescs.resize(GeometryInfoList.size());
    BLASBuffers.resize(GeometryInfoList.size());
    for (size_t i = 0; i < blasDescs.size(); i++)
    {
        // Allocate buffer
        BLASBuffers[i].Create(L"BLAS Buffer", 1, (uint32_t)blasSizes[i], nullptr, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        // Point to buffers
        blasDescs[i].DestAccelerationStructureData    = BLASBuffers[i].GetGpuVirtualAddress();
        blasDescs[i].ScratchAccelerationStructureData = BLASScratchBuffer.GetGpuVirtualAddress();

        // Fill in the instance desc
        D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = InstanceDescs[i];
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), *GeometryInfoList[i].WorldMatrix);
        instanceDesc.AccelerationStructure                  = BLASBuffers[i].GetGpuVirtualAddress();
        instanceDesc.Flags                                  = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instanceDesc.InstanceID                             = GeometryInfoList[i].InstanceId;
        instanceDesc.InstanceMask                           = GeometryInfoList[i].InstanceMask;
        instanceDesc.InstanceContributionToHitGroupIndex    = (i * HitProgramCount);
    }

    // Finally, build the acceleration structures
    {
        ID3D12GraphicsCommandList4* pCommandList = context.GetCommandList();
        auto                        uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
        for (UINT i = 0; i < blasDescs.size(); i++)
        {
            pCommandList->BuildRaytracingAccelerationStructure(&blasDescs[i], 0, nullptr);
            pCommandList->ResourceBarrier(1, &uavBarrier);
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingGeometry::BuildTLAS(CommandContext& context)
{
    // Allocate instance data buffer
    InstanceDataBuffer.Create(L"Instance Data Buffer", (uint32_t)GeometryInfoList.size(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC), InstanceDescs.data());

    // Gather info on TLAS
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuildInfo;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    tlasDesc = {};
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = tlasDesc.Inputs;
        topLevelInputs.Type             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        topLevelInputs.NumDescs         = (UINT)GeometryInfoList.size();
        topLevelInputs.Flags            = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        topLevelInputs.pGeometryDescs   = nullptr;
        topLevelInputs.DescsLayout      = D3D12_ELEMENTS_LAYOUT_ARRAY;
        topLevelInputs.InstanceDescs    = InstanceDataBuffer.GetGpuVirtualAddress();

        RenderDevice::Get().GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &tlasPrebuildInfo);
    }

    // Allocate scratch buffer
    TLASScratchBuffer.Create(L"Acceleration Structure Scratch Buffer", (UINT)tlasPrebuildInfo.ScratchDataSizeInBytes, 1, nullptr, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
   
    // Allocate TLAS buffer
    CurrentTLASIndex = 0;
    for (int i = 0; i < 2; i++)
    {
        TLASBuffer[i].Create(L"TLAS Buffer", 1, (uint32_t)tlasPrebuildInfo.ResultDataMaxSizeInBytes, nullptr, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    }

    // Point TLAS description to the allocated buffers
    tlasDesc.DestAccelerationStructureData    = TLASBuffer[0].GetGpuVirtualAddress();
    tlasDesc.ScratchAccelerationStructureData = TLASScratchBuffer.GetGpuVirtualAddress();

    // Finally, build the acceleration structures
    {
        ID3D12GraphicsCommandList4* pCommandList = context.GetCommandList();
        auto                        uavBarrier   = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
        pCommandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
        pCommandList->ResourceBarrier(1, &uavBarrier);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RaytracingGeometry::UpdateTLASTransforms(CommandContext& context)
{
    // Update all the transforms in the geometry list
    for (size_t i = 0; i < InstanceDescs.size(); i++)
    {
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(InstanceDescs[i].Transform), *GeometryInfoList[i].WorldMatrix);
    }

    // Update the instance data buffer with updated transforms
    context.WriteBuffer(InstanceDataBuffer, 0, InstanceDescs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * InstanceDescs.size());
    context.TransitionResource(InstanceDataBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, true);

    // Update TLAS
    int32_t                                            nextTLASIndex = (CurrentTLASIndex + 1) % 2;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc      = {};

    tlasDesc.Inputs.Type                        = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tlasDesc.Inputs.NumDescs                    = (UINT)GeometryInfoList.size();
    tlasDesc.Inputs.Flags                       = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    tlasDesc.Inputs.pGeometryDescs              = nullptr;
    tlasDesc.Inputs.DescsLayout                 = D3D12_ELEMENTS_LAYOUT_ARRAY;
    tlasDesc.Inputs.InstanceDescs               = InstanceDataBuffer.GetGpuVirtualAddress();
    tlasDesc.SourceAccelerationStructureData    = TLASBuffer[CurrentTLASIndex].GetGpuVirtualAddress();
    tlasDesc.DestAccelerationStructureData      = TLASBuffer[nextTLASIndex].GetGpuVirtualAddress();
    tlasDesc.ScratchAccelerationStructureData   = TLASScratchBuffer.GetGpuVirtualAddress();

    // Call to update TLAS
    {
        ID3D12GraphicsCommandList4* pCommandList = context.GetCommandList();
        auto                        uavBarrier   = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
        pCommandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
        pCommandList->ResourceBarrier(1, &uavBarrier);
    }

    // Ping-pong
    CurrentTLASIndex = nextTLASIndex;
}
