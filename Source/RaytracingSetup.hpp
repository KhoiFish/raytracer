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

#include "RaytracingShaderInclude.h"
#include "RealtimeEngine/RealtimeSceneShaderInclude.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RaytracingGlobalRootSigSlot
{
    enum EnumTypes
    {
        PrevAOOutput = 0,
        CurAOOutput,
        AccelerationStructure,
        Positions,
        Normals,
        TexCoordsAndDepth,
        Albedo,

        Num
    };

    enum IndexTypes
    {
        Register = 0,
        Count
    };

    // [register, count]
    static UINT Range[RaytracingGlobalRootSigSlot::Num][2] =
    {
        { 0, 1 },   // PrevAOOutput
        { 1, 1 },   // CurAOOutput
        { 0, 1 },   // AccelerationStructure
        { 1, 1 },   // Positions
        { 2, 1 },   // Normals
        { 3, 1 },   // TexCoordsAndDepth
        { 4, 1 },   // Albedo
    };
}

namespace RaytracingLocalRootSigSlot
{
    enum Enum
    {
        SceneCB = 0,

        Num
    };

    enum IndexTypes
    {
        Register = 0,
        Count
    };

    // [register, count]
    static UINT Range[RaytracingLocalRootSigSlot::Num][2] =
    {
        { 0, 1 },
    };
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeGpuRaytracer()
{
    // Delete old one, if it exists
    if (RaytracingGlobalDescriptorHeap != nullptr)
    {
        delete RaytracingGlobalDescriptorHeap;
        RaytracingGlobalDescriptorHeap = nullptr;
    }

    // Allocate descriptor heap
    RaytracingGlobalDescriptorHeap = new DescriptorHeapStack(64, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0);

    // Allocate descriptor for output view
    RaytracingGlobalDescriptorHeap->AllocateBufferUav(
        *AmbientOcclusionOutput[0].GetResource(),
        D3D12_UAV_DIMENSION_TEXTURE2D,
        D3D12_BUFFER_UAV_FLAG_NONE,
        DXGI_FORMAT_R8G8B8A8_UNORM);

    RaytracingGlobalDescriptorHeap->AllocateBufferUav(
        *AmbientOcclusionOutput[1].GetResource(),
        D3D12_UAV_DIMENSION_TEXTURE2D,
        D3D12_BUFFER_UAV_FLAG_NONE,
        DXGI_FORMAT_R8G8B8A8_UNORM);

    // Allocate descriptor for acceleration structures
    RaytracingGlobalDescriptorHeap->AllocateBufferSrvRaytracing(
        TheRenderScene->GetTLASVirtualAddress(),
        D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
        D3D12_BUFFER_SRV_FLAG_NONE,
        DXGI_FORMAT_UNKNOWN,
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    // Positions
    RaytracingGlobalDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_Position].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_Position],
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    // Normals
    RaytracingGlobalDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_Normal].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_Normal],
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    // TexCoords and Depth
    RaytracingGlobalDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_TexCoordAndDepth].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_TexCoordAndDepth],
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    // Albedo
    RaytracingGlobalDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_Albedo].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_Albedo],
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRealtimeRaytracingPipeline()
{
    static const wchar_t* sRaygenShaderName     = L"RayGenerationShader";
    static const wchar_t* sMissShaderName       = L"MissShader";
    static const wchar_t* sClosestHitShaderName = L"ClosestHitShader";
    static const wchar_t* sHitGroupName         = L"HitGroup";
    static const wchar_t* sDxilLibEntryPoints[] = { sRaygenShaderName, sMissShaderName, sClosestHitShaderName };

    // Allocate scene constant buffer
    RaytracingSceneConstantBuffer.Create(L"Raytracing Scene Globals Buffer", 1, (uint32_t)AlignUp(sizeof(RaytracingGlobalCB), 256));

    // Global root sig
    {
        RaytracingGlobalRootSig.Reset(RaytracingGlobalRootSigSlot::Num, 0);
        {
            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::PrevAOOutput].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::PrevAOOutput][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::PrevAOOutput][RaytracingGlobalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::CurAOOutput].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::CurAOOutput][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::CurAOOutput][RaytracingGlobalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::AccelerationStructure].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::AccelerationStructure][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::AccelerationStructure][RaytracingGlobalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::Positions].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Positions][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Positions][RaytracingGlobalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::Normals].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Normals][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Normals][RaytracingGlobalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::TexCoordsAndDepth].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::TexCoordsAndDepth][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::TexCoordsAndDepth][RaytracingGlobalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::Albedo].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Albedo][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Albedo][RaytracingGlobalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);
        }
        RaytracingGlobalRootSig.Finalize("RealtimeRaytracingGlobalRoot", D3D12_ROOT_SIGNATURE_FLAG_NONE);

        // This will allocate descriptor heap
        OnResizeGpuRaytracer();
    }

    // Local sig
    {
        // Setup local sig
        RaytracingLocalRootSig.Reset(RaytracingLocalRootSigSlot::Num, 0);
        {
            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::SceneCB].InitAsConstantBuffer(
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::SceneCB][RaytracingLocalRootSigSlot::Register],
                D3D12_SHADER_VISIBILITY_ALL);
        }
        RaytracingLocalRootSig.Finalize("RaytracerLocalRootSig", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

        // Allocator descriptor for scene constants
        RaytracingLocalDescriptorHeap = new DescriptorHeapStack(64, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0);
        RaytracingLocalDescriptorHeap->AllocateBufferCbv(
            RaytracingSceneConstantBuffer.GetGpuVirtualAddress(), 
            (UINT)RaytracingSceneConstantBuffer.GetBufferSize());
    }

    // Setup shaders and configs for the PSO
    {
        // Setup dxlib
        {
            ID3DBlobPtr pDxilLib;
            ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\Raytracing.cso", &pDxilLib));
            TheRaytracingPSO.SetDxilLibrary(pDxilLib, sDxilLibEntryPoints, ARRAY_SIZE(sDxilLibEntryPoints));
        }

        // Setup hit program
        TheRaytracingPSO.SetHitProgram(nullptr, sClosestHitShaderName, sHitGroupName);

        // Add local root signatures
        TheRaytracingPSO.SetRootSignature(RaytracingGlobalRootSig);
        TheRaytracingPSO.AddLocalRootSignature(&RaytracingLocalRootSig);

        // Associate shaders with local root signature
        TheRaytracingPSO.AddExportAssociationWithLocalRootSignature(sDxilLibEntryPoints, ARRAY_SIZE(sDxilLibEntryPoints), &RaytracingLocalRootSig);

        // Bind data (like descriptor data) to shaders
        {
            // Prepare the data to bind to the shader
            uint8_t* pTempBuffer = new uint8_t[256];
            uint32_t numBytes = 0;
            {
                // Copy scene constants address
                D3D12_GPU_VIRTUAL_ADDRESS cbVA = RaytracingSceneConstantBuffer.GetGpuVirtualAddress();
                memcpy(pTempBuffer + numBytes, &cbVA, sizeof(cbVA));
                numBytes += sizeof(cbVA);
            }

            TheRaytracingPSO.GetRaygenShaderTable().AddShaderRecordData(
                pTempBuffer,
                numBytes);

            TheRaytracingPSO.GetHitGroupShaderTable().AddShaderRecordData(
                pTempBuffer,
                numBytes);

            TheRaytracingPSO.GetMissShaderTable().AddShaderRecordData(
                pTempBuffer,
                numBytes);

            delete [] pTempBuffer;
            pTempBuffer = nullptr;
        }

        // Shader config
        TheRaytracingPSO.SetShaderConfig(D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES, sizeof(RayPayload));

        // Associate shaders with shader config
        TheRaytracingPSO.AddExportAssociationWithShaderConfig(sDxilLibEntryPoints, ARRAY_SIZE(sDxilLibEntryPoints));

        // Pipeline config
        TheRaytracingPSO.SetPipelineConfig(UserInput.GpuMaxRayRecursionDepth);

        // Set shader names for the shader tables
        TheRaytracingPSO.GetRaygenShaderTable().SetShaderName(sRaygenShaderName);
        TheRaytracingPSO.GetHitGroupShaderTable().SetShaderName(sHitGroupName);
        TheRaytracingPSO.GetMissShaderTable().SetShaderName(sMissShaderName);
    }

    // Done, finalize
    TheRaytracingPSO.Finalize();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::ComputeRaytracingResults()
{
    ComputeContext& computeContext = ComputeContext::Begin("Raytracing");
    {
        // Update scene constants buffer
        {
            // Get updated scene constants
            RaytracingGlobalCB sceneCB;
            XMStoreFloat4(&sceneCB.CameraPosition, TheRenderScene->GetCamera().GetEye());
            XMStoreFloat4x4(&sceneCB.InverseTransposeViewProjectionMatrix, XMMatrixInverse(nullptr, TheRenderScene->GetCamera().GetViewMatrix() * TheRenderScene->GetCamera().GetProjectionMatrix()));
            sceneCB.OutputResolution = DirectX::XMFLOAT2((float)Width, (float)Height);
            sceneCB.AORadius         = UserInput.GpuAORadius;
            sceneCB.FrameCount       = FrameCount;
            sceneCB.NumRays          = UserInput.GpuNumRaysPerPixel;
            sceneCB.AccumCount       = AccumCount++;
            
            // Update GPU buffer
            computeContext.WriteBuffer(RaytracingSceneConstantBuffer, 0, &sceneCB, sizeof(sceneCB));
            computeContext.TransitionResource(RaytracingSceneConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            computeContext.FlushResourceBarriers();
        }

        // Set root sig and pipeline state
        computeContext.SetRootSignature(RaytracingGlobalRootSig);
        computeContext.SetPipelineState(TheRaytracingPSO);

        // Set descriptor heaps and tables
        computeContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RaytracingGlobalDescriptorHeap->GetDescriptorHeap());
        for (int i = 0; i < RaytracingGlobalRootSigSlot::Num; i++)
        {
            computeContext.SetDescriptorTable(i, RaytracingGlobalDescriptorHeap->GetGpuHandle(i));
        }
        
        // Transition all output buffers
        computeContext.TransitionResource(AmbientOcclusionOutput[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
        computeContext.TransitionResource(AmbientOcclusionOutput[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

        // Finally dispatch rays
        computeContext.DispatchRays(TheRaytracingPSO, Width, Height);

        // Copy current results to previous
        computeContext.CopyBuffer(AmbientOcclusionOutput[0], AmbientOcclusionOutput[1]);
    }
    computeContext.Finish(true);
}

