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

#include "RaytracingShader.h"
#include "RealtimeEngine/RenderSceneShader.h"

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)
#define ArraySize(a) (sizeof(a)/sizeof(a[0]))

// ----------------------------------------------------------------------------------------------------------------------------

namespace RaytracingGlobalRootSigSlot
{
    enum EnumTypes
    {
        GlobalConstants,

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
        { 0, 1 },
    };
}

namespace RaytracingLocalRootSigSlot
{
    enum Enum
    {
        OutputView = 0,
        AccelerationStructure,
        Depth,
        Positions,
        Normals,
        SceneCB,

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
        { 0, 1 },
        { 1, 1 },
        { 2, 1 },
        { 3, 1 },
        { 0, 1 },
    };
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRealtimeRaytracingPipeline()
{
    static const wchar_t* sRaygenShaderName     = L"RayGenerationShader";
    static const wchar_t* sMissShaderName       = L"MissShader";
    static const wchar_t* sClosestHitShaderName = L"ClosestHitShader";
    static const wchar_t* sHitGroupName         = L"HitGroup";
    static const wchar_t* sDxilLibEntryPoints[] = { sRaygenShaderName, sMissShaderName, sClosestHitShaderName };

    // Allocate descriptor heap
    RaytracingDescriptorHeap = new DescriptorHeapStack(64, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0);

    // Allocate the output buffers
    AmbientOcclusionOutput.Destroy();
    AmbientOcclusionOutput.CreateEx("AO Buffer", Width, Height, 1, RaytracingBufferType, nullptr, D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

    // Allocate scene constant buffer
    RaytracingSceneConstantBuffer.Create(L"Raytracing Scene Globals Buffer", 1, (uint32_t)AlignUp(sizeof(RaytracingGlobalCB), 256));

    // Global root sig
    {
        RaytracingGlobalRootSig.Reset(0, 0);
        RaytracingGlobalRootSig.Finalize("RealtimeRaytracingGlobalRoot", D3D12_ROOT_SIGNATURE_FLAG_NONE);
    }

    // Raygen local root sig setup
    {
        // Setup local sig
        RaytracingLocalRootSig.Reset(RaytracingLocalRootSigSlot::Num, 0);
        {
            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::OutputView].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::OutputView][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::OutputView][RaytracingLocalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::AccelerationStructure].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::AccelerationStructure][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::AccelerationStructure][RaytracingLocalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::Depth].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::Depth][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::Depth][RaytracingLocalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::Positions].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::Positions][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::Positions][RaytracingLocalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::Normals].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::Normals][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::Normals][RaytracingLocalRootSigSlot::Count],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::SceneCB].InitAsConstantBuffer(
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::SceneCB][RaytracingLocalRootSigSlot::Register],
                D3D12_SHADER_VISIBILITY_ALL);
        }
        RaytracingLocalRootSig.Finalize("RaytracerLocalRootSig", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

        // Allocate descriptor for output view
        RaytracingDescriptorHeap->AllocateBufferUav(
            *AmbientOcclusionOutput.GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            DXGI_FORMAT_R8G8B8A8_UNORM);

        // Allocate descriptor for acceleration structures
        RaytracingDescriptorHeap->AllocateBufferSrvRaytracing(
            TheRenderScene->GetTLASVirtualAddress(),
            D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
            D3D12_BUFFER_SRV_FLAG_NONE,
            DXGI_FORMAT_UNKNOWN,
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

        // Depth
        RaytracingDescriptorHeap->AllocateTexture2DSrv(
            ZPrePassBuffer.GetResource(),
            ZBufferRTType,
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

        // Positions
        RaytracingDescriptorHeap->AllocateTexture2DSrv(
            DeferredBuffers[DeferredBufferType_Position].GetResource(),
            DeferredBuffersRTTypes[DeferredBufferType_Position],
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

        // Normals
        RaytracingDescriptorHeap->AllocateTexture2DSrv(
            DeferredBuffers[DeferredBufferType_Normal].GetResource(),
            DeferredBuffersRTTypes[DeferredBufferType_Normal],
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

        // Allocator descriptor for scene constants
        RaytracingDescriptorHeap->AllocateBufferCbv(
            RaytracingSceneConstantBuffer.GetGpuVirtualAddress(), 
            (UINT)RaytracingSceneConstantBuffer.GetBufferSize());
    }

    // Setup shaders and configs for the PSO
    {
        // Setup dxlib
        {
            ID3DBlobPtr pDxilLib;
            ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\Raytracing.cso", &pDxilLib));
            TheRaytracingPSO.SetDxilLibrary(pDxilLib, sDxilLibEntryPoints, ArraySize(sDxilLibEntryPoints));
        }

        // Setup hit program
        TheRaytracingPSO.SetHitProgram(nullptr, sClosestHitShaderName, sHitGroupName);

        // Add local root signatures
        TheRaytracingPSO.SetRootSignature(RaytracingGlobalRootSig);
        TheRaytracingPSO.AddLocalRootSignature(&RaytracingLocalRootSig);

        // Associate shaders with local root signature
        TheRaytracingPSO.AddExportAssociationWithLocalRootSignature(sDxilLibEntryPoints, ArraySize(sDxilLibEntryPoints), &RaytracingLocalRootSig);

        // Bind data (like descriptor data) to shaders
        {
            // Prepare the data to bind to the shader
            uint8_t* pTempBuffer = new uint8_t[256];
            uint32_t numBytes = 0;
            {
                UINT64 descriptorOffsets[] =
                {
                    RaytracingDescriptorHeap->GetGpuHandle(0).ptr,
                    RaytracingDescriptorHeap->GetGpuHandle(1).ptr,
                    RaytracingDescriptorHeap->GetGpuHandle(2).ptr,
                    RaytracingDescriptorHeap->GetGpuHandle(3).ptr,
                    RaytracingDescriptorHeap->GetGpuHandle(4).ptr,
                };

                // Copy scene descriptor addresses
                for (int i = 0; i < ArraySize(descriptorOffsets); i++)
                {
                    memcpy(pTempBuffer + numBytes, &descriptorOffsets[i], sizeof(descriptorOffsets[i]));
                    numBytes += sizeof(descriptorOffsets[i]);
                }
        
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
        TheRaytracingPSO.AddExportAssociationWithShaderConfig(sDxilLibEntryPoints, ArraySize(sDxilLibEntryPoints));

        // Pipeline config
        TheRaytracingPSO.SetPipelineConfig(MaxyRayRecursionDepth);

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
            XMStoreFloat4(&sceneCB.CameraPosition, TheRenderScene->GetRenderCamera().GetEye());
            XMStoreFloat4x4(&sceneCB.InverseTransposeViewProjectionMatrix, XMMatrixInverse(nullptr, TheRenderScene->GetRenderCamera().GetViewMatrix() * TheRenderScene->GetRenderCamera().GetProjectionMatrix()));
            sceneCB.OutputResolution = DirectX::XMFLOAT2((float)Width, (float)Height);
            sceneCB.AORadius         = AORadius;
            sceneCB.FrameCount       = FrameCount;
            sceneCB.NumRays          = NumRaysPerPixel;
            
            // Update GPU buffer
            computeContext.WriteBuffer(RaytracingSceneConstantBuffer, 0, &sceneCB, sizeof(sceneCB));
            computeContext.TransitionResource(RaytracingSceneConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            computeContext.FlushResourceBarriers();
        }

        computeContext.SetRootSignature(RaytracingGlobalRootSig);
        computeContext.SetPipelineState(TheRaytracingPSO);
        computeContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, &RaytracingDescriptorHeap->GetDescriptorHeap());
        computeContext.InsertUAVBarrier(AmbientOcclusionOutput, true);
        computeContext.DispatchRays(TheRaytracingPSO, Width, Height);
    }
    computeContext.Finish(true);
}

