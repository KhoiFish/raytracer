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

// Main ray gen
static const wchar_t* sRaygenShaderName                      = L"RayGeneration";

// AO shaders
static const wchar_t* sAoMissShaderName                      = L"AOMiss";
static const wchar_t* sAoClosestHitShaderName                = L"AOClosest";
static const wchar_t* sAoHitGroupName                        = L"AOHitGroup";

// Direct lighting shaders
static const wchar_t* sDirectLightingMissShaderName          = L"DirectLightingMiss";
static const wchar_t* sDirectLightingClosestHitShaderName    = L"DirectLightingClosest";
static const wchar_t* sDirectLightingHitGroupName            = L"DirectLightingHitGroup";

// Indirect lighting shaders
static const wchar_t* sIndirectLightingMissShaderName        = L"IndirectLightingMiss";
static const wchar_t* sIndirectLightingClosestHitShaderName  = L"IndirectLightingClosest";
static const wchar_t* sIndirectLightingHitGroupName          = L"IndirectLightingHitGroup";

// Color shaders
static const wchar_t* sColorMissShaderName                  = L"ColorMiss";
static const wchar_t* sColorClosestHitShaderName            = L"ColorClosest";
static const wchar_t* sColorHitGroupName                    = L"ColorHitGroup";

// Concatenated entry points
static const wchar_t* sDxilLibEntryPoints[] =
{
    sRaygenShaderName,
    sDirectLightingMissShaderName, sDirectLightingClosestHitShaderName,
    sIndirectLightingMissShaderName, sIndirectLightingClosestHitShaderName,
    sAoMissShaderName, sAoClosestHitShaderName,
    sColorMissShaderName, sColorClosestHitShaderName,
};

// ----------------------------------------------------------------------------------------------------------------------------

namespace RaytracingGlobalRootSigSlot
{
    enum EnumTypes
    {
        PrevDirectLightAO = 0,
        CurrDirectLightAO,
        PrevIndirectLight,
        CurrIndirectLight,

        SceneCB,
        LightsCB,
        MaterialsCB,

        AccelerationStructure,
        Positions,
        Normals,
        TexCoordsAndDepth,
        Albedo,
        AlbedoArray,

        VertexBufferArray,
        IndexBufferArray,

        Num
    };

    enum IndexTypes
    {
        Register = 0,
        Count,
        Space,
        RangeType,
    };

    static const UINT nL = RAYTRACING_MAX_NUM_LIGHTS;
    static const UINT nM = RAYTRACING_MAX_NUM_MATERIALS;
    static const UINT nD = RAYTRACING_MAX_NUM_DIFFUSETEXTURES;
    static const UINT nV = RAYTRACING_MAX_NUM_VERTEXBUFFERS;
    static const UINT nI = RAYTRACING_MAX_NUM_INDEXBUFFERS;

    // [register, count, space, D3D12_DESCRIPTOR_RANGE_TYPE]
    static UINT Range[RaytracingGlobalRootSigSlot::Num][4] =
    {
        { 0, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // PrevDirectLightAO
        { 1, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // CurrDirectLightAO
        { 2, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // PrevIndirectLight
        { 3, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // CurrIndirectLight

        { 0, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // Scene CB
        { 0, nL, 1, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // Lights CB
        { 0, nM, 2, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // Materials CB

        { 0, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // AccelerationStructure
        { 1, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // Positions
        { 2, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // Normals
        { 3, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // TexCoordsAndDepth
        { 4, 1,  0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // Albedo

        { 0, nD, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // AlbedoArray
        { 0, nV, 2, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // VertexBufferArray
        { 0, nI, 3, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // IndexBufferArray
    };

    // To be filled in by descriptors creation
    static UINT DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::Num];
}

namespace RaytracingLocalRootSigSlot
{
    enum Enum
    {
        LocalCB = 0,
        MaterialCB,
        VertexBuffer,
        IndexBuffer,

        Num
    };

    enum IndexTypes
    {
        Register = 0,
        Count,
        Space,
        RangeType,
    };

    // [register, count, space]
    static UINT Range[RaytracingLocalRootSigSlot::Num][4] =
    {
        { 0, 1, 4, D3D12_DESCRIPTOR_RANGE_TYPE_CBV }, // LocalCB
        { 1, 1, 4, D3D12_DESCRIPTOR_RANGE_TYPE_CBV }, // Material CB

        { 0, 1, 4, D3D12_DESCRIPTOR_RANGE_TYPE_SRV }, // VertexBuffer
        { 1, 1, 4, D3D12_DESCRIPTOR_RANGE_TYPE_SRV }, // IndexBuffer
    };
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::CleanupGpuRaytracer()
{
    if (RaytracingPSOPtr != nullptr)
    {
        delete RaytracingPSOPtr;
        RaytracingPSOPtr = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnResizeGpuRaytracer()
{
    SetupGpuRaytracingDescriptors();
    SetupGpuRaytracingPSO();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupGpuRaytracingPipeline()
{
    SetupGpuRaytracingRootSignatures();
    SetupGpuRaytracingDescriptors();
    SetupGpuRaytracingPSO();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupGpuRaytracingRootSignatures()
{
    // Allocate scene constant buffer
    RaytracingSceneConstantBuffer.Create(L"Raytracing Scene Globals Buffer", 1, (uint32_t)AlignUp(sizeof(RaytracingGlobalCB), 256));

    // Global root sig
    {
        RaytracingGlobalRootSig.Reset(RaytracingGlobalRootSigSlot::Num, 1);
        for (int i = 0; i < RaytracingGlobalRootSigSlot::Num; i++)
        {
            RaytracingGlobalRootSig[i].InitAsDescriptorRange(
                
                (D3D12_DESCRIPTOR_RANGE_TYPE)RaytracingGlobalRootSigSlot::Range[i][RaytracingGlobalRootSigSlot::RangeType],
                RaytracingGlobalRootSigSlot::Range[i][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[i][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[i][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);
        }
        RaytracingGlobalRootSig.InitStaticSampler(0, GetAnisoSamplerDesc(), D3D12_SHADER_VISIBILITY_ALL);
        RaytracingGlobalRootSig.Finalize("RealtimeRaytracingGlobalRoot", D3D12_ROOT_SIGNATURE_FLAG_NONE);
    }

    // Local sig
    {
        // Setup local sig
        RaytracingLocalRootSig.Reset(RaytracingLocalRootSigSlot::Num, 0);
        for (int i = 0; i < RaytracingLocalRootSigSlot::Num; i++)
        {
            RaytracingLocalRootSig[i].InitAsDescriptorRange(
                (D3D12_DESCRIPTOR_RANGE_TYPE)RaytracingLocalRootSigSlot::Range[i][RaytracingLocalRootSigSlot::RangeType],
                RaytracingLocalRootSigSlot::Range[i][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[i][RaytracingLocalRootSigSlot::Count],
                RaytracingLocalRootSigSlot::Range[i][RaytracingLocalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);
        }
        RaytracingLocalRootSig.Finalize("RaytracerLocalRootSig", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupGpuRaytracingDescriptors()
{
    RaytracingGlobalSigDataIndexStart = RendererDescriptorHeap->GetCount();

    UINT currOffset = 0;

    // Allocate descriptors for prev and current direct lighting + ao
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::PrevDirectLightAO] = currOffset++;
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::CurrDirectLightAO] = currOffset++;
    for (int i = 0; i < 2; i++)
    {
        RendererDescriptorHeap->AllocateBufferUav(
            *DirectLightingAOBuffer[i].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            RaytracingBufferType);
    }

    // Allocate descriptors for prev and current indirect lighting
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::PrevIndirectLight] = currOffset++;
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::CurrIndirectLight] = currOffset++;
    for (int i = 0; i < 2; i++)
    {
        RendererDescriptorHeap->AllocateBufferUav(
            *IndirectLightingBuffer[i].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            RaytracingBufferType);
    }

    // Scene CB
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::SceneCB] = currOffset++;
    RendererDescriptorHeap->AllocateBufferCbv(
        RaytracingSceneConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RaytracingSceneConstantBuffer.GetBufferSize());

    // Lights CB
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::LightsCB] = currOffset;
    for (size_t i = 0; i < TheRenderScene->GetAreaLightsList().size(); i++)
    {
        RendererDescriptorHeap->AllocateBufferCbv(
            TheRenderScene->GetAreaLightsList()[i]->AreaLightBuffer.GetGpuVirtualAddress(),
            (UINT)TheRenderScene->GetAreaLightsList()[i]->AreaLightBuffer.GetBufferSize());

        currOffset++;
    }

    // Materials CB
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::MaterialsCB] = currOffset;
    for (size_t i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
    {
        RendererDescriptorHeap->AllocateBufferCbv(
            TheRenderScene->GetRenderSceneList()[i]->MaterialBuffer.GetGpuVirtualAddress(),
            (UINT)TheRenderScene->GetRenderSceneList()[i]->MaterialBuffer.GetBufferSize());

        currOffset++;
    }
    
    // Note this is a placeholder; we swap the descriptor table with the latest one
    // Allocate descriptor for acceleration structures
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::AccelerationStructure] = currOffset;
    {
        D3D12_GPU_VIRTUAL_ADDRESS vAddr[2];
        TheRenderScene->GetRaytracingGeometry()->GetTLASVirtualAddresses(vAddr[0], vAddr[1]);

        RendererDescriptorHeap->AllocateBufferSrvRaytracing(
            vAddr[0],
            D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
            D3D12_BUFFER_SRV_FLAG_NONE,
            DXGI_FORMAT_UNKNOWN,
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
        RendererDescriptorHeap->AllocateBufferSrvRaytracing(
            vAddr[1],
            D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
            D3D12_BUFFER_SRV_FLAG_NONE,
            DXGI_FORMAT_UNKNOWN,
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

        currOffset += 2;
    }

    // Positions
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::Positions] = currOffset++;
    RendererDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_Position].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_Position]);

    // Normals
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::Normals] = currOffset++;
    RendererDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_Normal].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_Normal]);

    // TexCoords and Depth
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::TexCoordsAndDepth] = currOffset++;
    RendererDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_TexCoordAndDepth].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_TexCoordAndDepth]);

    // Albedo
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::Albedo] = currOffset++;
    RendererDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_Albedo].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_Albedo]);

    // Vertex buffer array
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::VertexBufferArray] = currOffset;
    for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
    {
        RendererDescriptorHeap->AllocateBufferSrv(
            TheRenderScene->GetRenderSceneList()[i]->VertexBuffer.GetResource(),
            (static_cast<UINT>(TheRenderScene->GetRenderSceneList()[i]->Vertices.size()) * sizeof(RealtimeSceneVertex)) / sizeof(float),
            D3D12_SRV_DIMENSION_BUFFER,
            D3D12_BUFFER_SRV_FLAG_RAW,
            DXGI_FORMAT_R32_TYPELESS
        );

        currOffset++;
    }

    // Index buffer array
    RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::IndexBufferArray] = currOffset;
    for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
    {
        RendererDescriptorHeap->AllocateBufferSrv(
            TheRenderScene->GetRenderSceneList()[i]->IndexBuffer.GetResource(),
            (static_cast<UINT>(TheRenderScene->GetRenderSceneList()[i]->Indices.size()) * sizeof(uint32_t)) / sizeof(float),
            D3D12_SRV_DIMENSION_BUFFER,
            D3D12_BUFFER_SRV_FLAG_RAW,
            DXGI_FORMAT_R32_TYPELESS
        );

        currOffset++;
    }

    // Allocate descriptors for vertex and index buffers for all instances of geometry
    RaytracingLocalSigDataIndexStart = RendererDescriptorHeap->GetCount();
    for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
    {
        RendererDescriptorHeap->AllocateBufferCbv(
            TheRenderScene->GetRenderSceneList()[i]->InstanceDataBuffer.GetGpuVirtualAddress(),
            static_cast<UINT>(TheRenderScene->GetRenderSceneList()[i]->InstanceDataBuffer.GetBufferSize())
        );

        RendererDescriptorHeap->AllocateBufferCbv(
            TheRenderScene->GetRenderSceneList()[i]->MaterialBuffer.GetGpuVirtualAddress(),
            static_cast<UINT>(TheRenderScene->GetRenderSceneList()[i]->MaterialBuffer.GetBufferSize())
        );

        RendererDescriptorHeap->AllocateBufferSrv(
            TheRenderScene->GetRenderSceneList()[i]->VertexBuffer.GetResource(),
            (static_cast<UINT>(TheRenderScene->GetRenderSceneList()[i]->Vertices.size()) * sizeof(RealtimeSceneVertex)) / sizeof(float),
            D3D12_SRV_DIMENSION_BUFFER,
            D3D12_BUFFER_SRV_FLAG_RAW,
            DXGI_FORMAT_R32_TYPELESS
        );

        RendererDescriptorHeap->AllocateBufferSrv(
            TheRenderScene->GetRenderSceneList()[i]->IndexBuffer.GetResource(),
            (static_cast<UINT>(TheRenderScene->GetRenderSceneList()[i]->Indices.size()) * sizeof(uint32_t)) / sizeof(float),
            D3D12_SRV_DIMENSION_BUFFER,
            D3D12_BUFFER_SRV_FLAG_RAW,
            DXGI_FORMAT_R32_TYPELESS
        );
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupGpuRaytracingPSO()
{
    // Delete old one, if it exists
    if (RaytracingPSOPtr != nullptr)
    {
        delete RaytracingPSOPtr;
        RaytracingPSOPtr = nullptr;
    }

    // Create the PSO
    RaytracingPSOPtr = new RaytracingPSO();

    // Setup shaders and configs for the PSO
    {
        // Setup dxlib
        {
            ID3DBlobPtr pDxilLib;
            ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\Raytracing.cso", &pDxilLib));
            RaytracingPSOPtr->SetDxilLibrary(pDxilLib, sDxilLibEntryPoints, ARRAY_SIZE(sDxilLibEntryPoints));
        }

        // Add hit groups
        RaytracingPSOPtr->AddHitGroup(nullptr, sAoClosestHitShaderName, sAoHitGroupName);
        RaytracingPSOPtr->AddHitGroup(nullptr, sDirectLightingClosestHitShaderName, sDirectLightingHitGroupName);
        RaytracingPSOPtr->AddHitGroup(nullptr, sIndirectLightingClosestHitShaderName, sIndirectLightingHitGroupName);
        RaytracingPSOPtr->AddHitGroup(nullptr, sColorClosestHitShaderName, sColorHitGroupName);

        // Add local root signatures
        RaytracingPSOPtr->SetRootSignature(RaytracingGlobalRootSig);
        RaytracingPSOPtr->AddLocalRootSignature(&RaytracingLocalRootSig);

        // Associate shaders with local root signature
        RaytracingPSOPtr->AddExportAssociationWithLocalRootSignature(sDxilLibEntryPoints, ARRAY_SIZE(sDxilLibEntryPoints), &RaytracingLocalRootSig);

        // Bind data (like descriptor data) to shaders
        {
            // Raygen
            RaytracingPSOPtr->GetRaygenShaderTable().AddShaderRecordData(
                sRaygenShaderName,
                nullptr,
                0);

            // Miss
            RaytracingShaderIndex[RaytracingShaderType_DirectLightingMiss] = RaytracingPSOPtr->GetMissShaderTable().AddShaderRecordData(
                sDirectLightingMissShaderName,
                nullptr,
                0);

            RaytracingShaderIndex[RaytracingShaderType_IndirectLightingMiss] = RaytracingPSOPtr->GetMissShaderTable().AddShaderRecordData(
                sIndirectLightingMissShaderName,
                nullptr,
                0);

            RaytracingShaderIndex[RaytracingShaderType_AOMiss] = RaytracingPSOPtr->GetMissShaderTable().AddShaderRecordData(
                sAoMissShaderName,
                nullptr,
                0);

            RaytracingShaderIndex[RaytracingShaderType_ColorMiss] = RaytracingPSOPtr->GetMissShaderTable().AddShaderRecordData(
                sColorMissShaderName,
                nullptr,
                0);

            // Hit groups
            {
                // Set the right indexes to the hit program
                RaytracingShaderIndex[RaytracingShaderType_DirectLightingHitGroup]   = 0;
                RaytracingShaderIndex[RaytracingShaderType_IndirectLightingHitGroup] = 1;
                RaytracingShaderIndex[RaytracingShaderType_AOHitgroup]               = 2;
                RaytracingShaderIndex[RaytracingShaderType_ColorHitGroup]            = 3;

                // Setup hit group shader tables
                for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
                {
                    const uint32_t              numDesc      = 4;
                    const uint32_t              descOffset   = RaytracingLocalSigDataIndexStart + (i * numDesc);
                    D3D12_GPU_DESCRIPTOR_HANDLE descArray [] =
                    {
                        RendererDescriptorHeap->GetGpuHandle(descOffset + 0), // LocalCB
                        RendererDescriptorHeap->GetGpuHandle(descOffset + 1), // MaterialCB
                        RendererDescriptorHeap->GetGpuHandle(descOffset + 2), // Vertex Buffer
                        RendererDescriptorHeap->GetGpuHandle(descOffset + 3), // Index Buffer
                    };

                    RaytracingPSOPtr->GetHitGroupShaderTable().AddShaderRecordData(
                        sDirectLightingHitGroupName,
                        descArray,
                        sizeof(descArray));

                    RaytracingPSOPtr->GetHitGroupShaderTable().AddShaderRecordData(
                        sIndirectLightingHitGroupName,
                        descArray,
                        sizeof(descArray));

                    RaytracingPSOPtr->GetHitGroupShaderTable().AddShaderRecordData(
                        sAoHitGroupName,
                        descArray,
                        sizeof(descArray));

                    RaytracingPSOPtr->GetHitGroupShaderTable().AddShaderRecordData(
                        sColorHitGroupName,
                        descArray,
                        sizeof(descArray));
                }
            }
        }

        // Shader config
        RaytracingPSOPtr->SetShaderConfig(D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES, RAYTRACER_MAX_PAYLOAD_SIZE);

        // Associate shaders with shader config
        RaytracingPSOPtr->AddExportAssociationWithShaderConfig(sDxilLibEntryPoints, ARRAY_SIZE(sDxilLibEntryPoints));

        // Pipeline config
        RaytracingPSOPtr->SetPipelineConfig(TheUserInputData.GpuMaxRayRecursionDepth);
    }

    // Done, finalize
    RaytracingPSOPtr->Finalize();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGpuRaytracing()
{
    ComputeContext& computeContext = ComputeContext::Begin("Raytracing");
    {
        // Update TLAS transforms
        TheRenderScene->GetRaytracingGeometry()->UpdateTLASTransforms(computeContext);

        // Update global scene constants buffer
        {
            // Get updated scene constants
            RaytracingGlobalCB sceneCB;
            XMStoreFloat4(&sceneCB.CameraPosition, TheRenderScene->GetCamera().GetEye());
            XMStoreFloat4(&sceneCB.CameraTarget, TheRenderScene->GetCamera().GetTarget());
            XMStoreFloat4x4(&sceneCB.InverseTransposeViewProjectionMatrix, XMMatrixInverse(nullptr, TheRenderScene->GetCamera().GetViewMatrix() * TheRenderScene->GetCamera().GetProjectionMatrix()));
            sceneCB.OutputResolution                = DirectX::XMFLOAT2((float)Width, (float)Height);
            sceneCB.AORadius                        = TheUserInputData.GpuAORadius;
            sceneCB.FrameCount                      = FrameCount;
            sceneCB.NumRays                         = TheUserInputData.GpuNumRaysPerPixel;
            sceneCB.AccumCount                      = AccumCount++;
            sceneCB.MaxRayDepth                     = TheUserInputData.GpuMaxRayRecursionDepth;
            sceneCB.NumLights                       = (UINT)TheRenderScene->GetAreaLightsList().size();
            sceneCB.AOHitGroupIndex                 = RaytracingShaderIndex[RaytracingShaderType_AOHitgroup];
            sceneCB.AOMissIndex                     = RaytracingShaderIndex[RaytracingShaderType_AOMiss];
            sceneCB.DirectLightingHitGroupIndex     = RaytracingShaderIndex[RaytracingShaderType_DirectLightingHitGroup];
            sceneCB.DirectLightingMissIndex         = RaytracingShaderIndex[RaytracingShaderType_DirectLightingMiss];
            sceneCB.IndirectLightingHitGroupIndex   = RaytracingShaderIndex[RaytracingShaderType_IndirectLightingHitGroup];
            sceneCB.IndirectLightingMissIndex       = RaytracingShaderIndex[RaytracingShaderType_IndirectLightingMiss];
            sceneCB.ColorHitGroupIndex              = RaytracingShaderIndex[RaytracingShaderType_ColorHitGroup];
            sceneCB.ColorMissIndex                  = RaytracingShaderIndex[RaytracingShaderType_ColorMiss];
            
            // Update GPU buffer
            RaytracingSceneConstantBuffer.Upload(&sceneCB, sizeof(sceneCB));
        }

        // Set root sig and pipeline state
        computeContext.SetRootSignature(RaytracingGlobalRootSig);
        computeContext.SetPipelineState(*RaytracingPSOPtr);

        // Set descriptor heaps and tables
        computeContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RendererDescriptorHeap->GetDescriptorHeap());
        for (uint32_t i = 0; i < RaytracingGlobalRootSigSlot::Num; i++)
        {
            uint32_t heapIndex = (RaytracingGlobalSigDataIndexStart + RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[i]);
            computeContext.SetDescriptorTable(i, RendererDescriptorHeap->GetGpuHandle(heapIndex));
        }

        // Set to the current acceleration structure
        {
            uint32_t heapIndex = 
                                 RaytracingGlobalSigDataIndexStart + 
                                 RaytracingGlobalRootSigSlot::DescriptorHeapOffsets[RaytracingGlobalRootSigSlot::AccelerationStructure] +
                                 TheRenderScene->GetRaytracingGeometry()->GetCurrentTLASIndex();

            computeContext.SetDescriptorTable(RaytracingGlobalRootSigSlot::AccelerationStructure, RendererDescriptorHeap->GetGpuHandle(heapIndex));
        }

        // Get heap start of all the diffuse textures
        computeContext.SetDescriptorTable(RaytracingGlobalRootSigSlot::AlbedoArray, RendererDescriptorHeap->GetGpuHandle(TheRenderScene->GetDiffuseTextureList().DescriptorHeapStartIndex));
        
        // Transition all output buffers
        for (int i = 0; i < 2; i++)
        {
            computeContext.TransitionResource(DirectLightingAOBuffer[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            computeContext.TransitionResource(IndirectLightingBuffer[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }
        
        // Flush all resource barriers before dispatching rays
        computeContext.FlushResourceBarriers();

        // Finally dispatch rays
        computeContext.DispatchRays(*RaytracingPSOPtr, Width, Height);

        // Copy current results to previous
        computeContext.CopyBuffer(DirectLightingAOBuffer[0], DirectLightingAOBuffer[1]);
        computeContext.CopyBuffer(IndirectLightingBuffer[0], IndirectLightingBuffer[1]);
    }
    computeContext.Finish();
}
