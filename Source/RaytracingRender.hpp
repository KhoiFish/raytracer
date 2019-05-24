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

// Concatenated entry points
static const wchar_t* sDxilLibEntryPoints[] =
{
    sRaygenShaderName,
    sDirectLightingMissShaderName, sDirectLightingClosestHitShaderName,
    sIndirectLightingMissShaderName, sIndirectLightingClosestHitShaderName,
    sAoMissShaderName, sAoClosestHitShaderName,
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
        Count,
        Space
    };

    static const UINT nL = RAYTRACING_MAX_NUM_LIGHTS;

    // [register, count, space]
    static UINT Range[RaytracingGlobalRootSigSlot::Num][3] =
    {
        { 0, 1,  0 },   // PrevDirectLightAO
        { 1, 1,  0 },   // CurrDirectLightAO
        { 2, 1,  0 },   // PrevIndirectLight
        { 3, 1,  0 },   // CurrIndirectLight

        { 0, 1,  0 },   // Scene CB
        { 1, nL, 0 },   // Lights CB

        { 0, 1,  0 },   // AccelerationStructure
        { 1, 1,  0 },   // Positions
        { 2, 1,  0 },   // Normals
        { 3, 1,  0 },   // TexCoordsAndDepth
        { 4, 1,  0 },   // Albedo
    };
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
        Space
    };

    // [register, count, space]
    static UINT Range[RaytracingLocalRootSigSlot::Num][3] =
    {
        { 0, 1, 1 }, // LocalCB
        { 1, 1, 1 }, // Material CB

        { 0, 1, 1 }, // VertexBuffer
        { 1, 1, 1 }, // IndexBuffer
    };
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::CleanupGpuRaytracer()
{
    if (RaytracingDescriptorHeap != nullptr)
    {
        delete RaytracingDescriptorHeap;
        RaytracingDescriptorHeap = nullptr;
    }

    if (TheRaytracingPSO != nullptr)
    {
        delete TheRaytracingPSO;
        TheRaytracingPSO = nullptr;
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
        RaytracingGlobalRootSig.Reset(RaytracingGlobalRootSigSlot::Num, 0);
        {
            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::PrevDirectLightAO].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::PrevDirectLightAO][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::PrevDirectLightAO][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::PrevDirectLightAO][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::CurrDirectLightAO].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::CurrDirectLightAO][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::CurrDirectLightAO][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::CurrDirectLightAO][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::PrevIndirectLight].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::PrevIndirectLight][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::PrevIndirectLight][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::PrevIndirectLight][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::CurrIndirectLight].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::CurrIndirectLight][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::CurrIndirectLight][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::CurrIndirectLight][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::SceneCB].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::SceneCB][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::SceneCB][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::SceneCB][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::LightsCB].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::LightsCB][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::LightsCB][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::LightsCB][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::AccelerationStructure].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::AccelerationStructure][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::AccelerationStructure][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::AccelerationStructure][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::Positions].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Positions][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Positions][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Positions][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::Normals].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Normals][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Normals][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Normals][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::TexCoordsAndDepth].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::TexCoordsAndDepth][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::TexCoordsAndDepth][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::TexCoordsAndDepth][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::Albedo].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Albedo][RaytracingGlobalRootSigSlot::Register],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Albedo][RaytracingGlobalRootSigSlot::Count],
                RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::Albedo][RaytracingGlobalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);
        }
        RaytracingGlobalRootSig.Finalize("RealtimeRaytracingGlobalRoot", D3D12_ROOT_SIGNATURE_FLAG_NONE);
    }

    // Local sig
    {
        // Setup local sig
        RaytracingLocalRootSig.Reset(RaytracingLocalRootSigSlot::Num, 0);
        {
            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::LocalCB].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::LocalCB][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::LocalCB][RaytracingLocalRootSigSlot::Count],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::LocalCB][RaytracingLocalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::MaterialCB].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::MaterialCB][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::MaterialCB][RaytracingLocalRootSigSlot::Count],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::MaterialCB][RaytracingLocalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::VertexBuffer].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::VertexBuffer][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::VertexBuffer][RaytracingLocalRootSigSlot::Count],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::VertexBuffer][RaytracingLocalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);

            RaytracingLocalRootSig[RaytracingLocalRootSigSlot::IndexBuffer].InitAsDescriptorRange(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::IndexBuffer][RaytracingLocalRootSigSlot::Register],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::IndexBuffer][RaytracingLocalRootSigSlot::Count],
                RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::IndexBuffer][RaytracingLocalRootSigSlot::Space],
                D3D12_SHADER_VISIBILITY_ALL);
        }
        RaytracingLocalRootSig.Finalize("RaytracerLocalRootSig", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupGpuRaytracingDescriptors()
{
    // Delete old one, if it exists
    if (RaytracingDescriptorHeap != nullptr)
    {
        delete RaytracingDescriptorHeap;
        RaytracingDescriptorHeap = nullptr;
    }

    // Allocate descriptor heap
    RaytracingDescriptorHeap = new DescriptorHeapStack(16384 * 4, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0);

    // Allocate descriptors for prev and current direct lighting + ao
    for (int i = 0; i < 2; i++)
    {
        RaytracingDescriptorHeap->AllocateBufferUav(
            *DirectLightingAOBuffer[i].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            RaytracingBufferType);
    }

    // Allocate descriptors for prev and current indirect lighting
    for (int i = 0; i < 2; i++)
    {
        RaytracingDescriptorHeap->AllocateBufferUav(
            *IndirectLightingBuffer[i].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            RaytracingBufferType);
    }

    // Scene CB
    RaytracingDescriptorHeap->AllocateBufferCbv(
        RaytracingSceneConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RaytracingSceneConstantBuffer.GetBufferSize());

    // Lights CB
    RaytracingDescriptorHeap->AllocateBufferCbv(
        TheRenderScene->GetAreaLightsBuffer().GetGpuVirtualAddress(),
        (UINT)TheRenderScene->GetAreaLightsBuffer().GetBufferSize());

    // Allocate descriptor for acceleration structures
    RaytracingDescriptorHeap->AllocateBufferSrvRaytracing(
        TheRenderScene->GetRaytracingGeometry()->GetTLASVirtualAddress(),
        D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
        D3D12_BUFFER_SRV_FLAG_NONE,
        DXGI_FORMAT_UNKNOWN,
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

    // TexCoords and Depth
    RaytracingDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_TexCoordAndDepth].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_TexCoordAndDepth],
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    // Albedo
    RaytracingDescriptorHeap->AllocateTexture2DSrv(
        DeferredBuffers[DeferredBufferType_Albedo].GetResource(),
        DeferredBuffersRTTypes[DeferredBufferType_Albedo],
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

    // Allocate descriptors for vertex and index buffers for all instances of geometry
    LocalSigDataIndexStart = RaytracingDescriptorHeap->GetCount();
    for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
    {
        RaytracingDescriptorHeap->AllocateBufferCbv(
            TheRenderScene->GetRenderSceneList()[i]->InstanceDataBuffer.GetGpuVirtualAddress(),
            static_cast<UINT>(TheRenderScene->GetRenderSceneList()[i]->InstanceDataBuffer.GetBufferSize())
        );

        RaytracingDescriptorHeap->AllocateBufferCbv(
            TheRenderScene->GetRenderSceneList()[i]->MaterialBuffer.GetGpuVirtualAddress(),
            static_cast<UINT>(TheRenderScene->GetRenderSceneList()[i]->MaterialBuffer.GetBufferSize())
        );

        RaytracingDescriptorHeap->AllocateBufferSrv(
            TheRenderScene->GetRenderSceneList()[i]->VertexBuffer.GetResource(),
            (static_cast<UINT>(TheRenderScene->GetRenderSceneList()[i]->Vertices.size()) * sizeof(RealtimeSceneVertex)) / sizeof(float),
            D3D12_SRV_DIMENSION_BUFFER,
            D3D12_BUFFER_SRV_FLAG_RAW,
            DXGI_FORMAT_R32_TYPELESS
        );

        RaytracingDescriptorHeap->AllocateBufferSrv(
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
    if (TheRaytracingPSO != nullptr)
    {
        delete TheRaytracingPSO;
        TheRaytracingPSO = nullptr;
    }

    // Create the PSO
    TheRaytracingPSO = new RaytracingPSO();

    // Setup shaders and configs for the PSO
    {
        // Setup dxlib
        {
            ID3DBlobPtr pDxilLib;
            ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\Raytracing.cso", &pDxilLib));
            TheRaytracingPSO->SetDxilLibrary(pDxilLib, sDxilLibEntryPoints, ARRAY_SIZE(sDxilLibEntryPoints));
        }

        // Add hit groups
        TheRaytracingPSO->AddHitGroup(nullptr, sAoClosestHitShaderName, sAoHitGroupName);
        TheRaytracingPSO->AddHitGroup(nullptr, sDirectLightingClosestHitShaderName, sDirectLightingHitGroupName);
        TheRaytracingPSO->AddHitGroup(nullptr, sIndirectLightingClosestHitShaderName, sIndirectLightingHitGroupName);
        HitProgramCount = 3;

        // Add local root signatures
        TheRaytracingPSO->SetRootSignature(RaytracingGlobalRootSig);
        TheRaytracingPSO->AddLocalRootSignature(&RaytracingLocalRootSig);

        // Associate shaders with local root signature
        TheRaytracingPSO->AddExportAssociationWithLocalRootSignature(sDxilLibEntryPoints, ARRAY_SIZE(sDxilLibEntryPoints), &RaytracingLocalRootSig);

        // Bind data (like descriptor data) to shaders
        {
            // Raygen
            TheRaytracingPSO->GetRaygenShaderTable().AddShaderRecordData(
                sRaygenShaderName,
                nullptr,
                0);

            // Miss
            RaytracingShaderIndex[RaytracingShaderType_DirectLightingMiss] = TheRaytracingPSO->GetMissShaderTable().AddShaderRecordData(
                sDirectLightingMissShaderName,
                nullptr,
                0);

            RaytracingShaderIndex[RaytracingShaderType_IndirectLightingMiss] = TheRaytracingPSO->GetMissShaderTable().AddShaderRecordData(
                sIndirectLightingMissShaderName,
                nullptr,
                0);

            RaytracingShaderIndex[RaytracingShaderType_AOMiss] = TheRaytracingPSO->GetMissShaderTable().AddShaderRecordData(
                sAoMissShaderName,
                nullptr,
                0);

            // Hit groups
            {
                // Prepare the local data for the hitgroups
                const uint32_t numDescriptorsPerInstance = 4;
                const uint32_t localDataStride           = sizeof(D3D12_GPU_DESCRIPTOR_HANDLE) * numDescriptorsPerInstance;
                uint8_t*       pHitGroupData             = new uint8_t[TheRenderScene->GetRenderSceneList().size() * localDataStride];
                for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
                {
                    uint32_t offset = (i * localDataStride);
                    {
                        const uint32_t              descOffset    = LocalSigDataIndexStart + (i * numDescriptorsPerInstance);
                        D3D12_GPU_DESCRIPTOR_HANDLE localCbVA     = RaytracingDescriptorHeap->GetGpuHandle(descOffset + 0);
                        D3D12_GPU_DESCRIPTOR_HANDLE materialCbVA  = RaytracingDescriptorHeap->GetGpuHandle(descOffset + 1);
                        D3D12_GPU_DESCRIPTOR_HANDLE vertBufferVA  = RaytracingDescriptorHeap->GetGpuHandle(descOffset + 2);
                        D3D12_GPU_DESCRIPTOR_HANDLE indexBufferVA = RaytracingDescriptorHeap->GetGpuHandle(descOffset + 3);

                        memcpy(pHitGroupData + offset, &localCbVA, sizeof(localCbVA));
                        offset += sizeof(localCbVA);
                        memcpy(pHitGroupData + offset, &materialCbVA, sizeof(materialCbVA));
                        offset += sizeof(materialCbVA);
                        memcpy(pHitGroupData + offset, &vertBufferVA, sizeof(vertBufferVA));
                        offset += sizeof(vertBufferVA);
                        memcpy(pHitGroupData + offset, &indexBufferVA, sizeof(indexBufferVA));
                        offset += sizeof(indexBufferVA);
                    }
                }

                // Add data to the hitgroups
                for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
                {
                    uint32_t dataOffset = (i * localDataStride);
                    uint32_t index = TheRaytracingPSO->GetHitGroupShaderTable().AddShaderRecordData(
                        sDirectLightingHitGroupName,
                        pHitGroupData + dataOffset,
                        localDataStride);

                    if (i == 0)
                    {
                        RaytracingShaderIndex[RaytracingShaderType_DirectLightingHitGroup] = index;
                    }
                }

                for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
                {
                    uint32_t dataOffset = (i * localDataStride);
                    uint32_t index = TheRaytracingPSO->GetHitGroupShaderTable().AddShaderRecordData(
                        sIndirectLightingHitGroupName,
                        pHitGroupData + dataOffset,
                        localDataStride);

                    if (i == 0)
                    {
                        RaytracingShaderIndex[RaytracingShaderType_IndirectLightingHitGroup] = index;
                    }
                }

                for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
                {
                    uint32_t dataOffset = (i * localDataStride);
                    uint32_t index = TheRaytracingPSO->GetHitGroupShaderTable().AddShaderRecordData(
                        sAoHitGroupName,
                        pHitGroupData + dataOffset,
                        localDataStride);

                    if (i == 0)
                    {
                        RaytracingShaderIndex[RaytracingShaderType_AOHitgroup] = index;
                    }
                }

                delete[] pHitGroupData;
                pHitGroupData = nullptr;
            }
        }

        // Shader config
        TheRaytracingPSO->SetShaderConfig(D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES, RAYTRACER_MAX_PAYLOAD_SIZE);

        // Associate shaders with shader config
        TheRaytracingPSO->AddExportAssociationWithShaderConfig(sDxilLibEntryPoints, ARRAY_SIZE(sDxilLibEntryPoints));

        // Pipeline config
        TheRaytracingPSO->SetPipelineConfig(UserInput.GpuMaxRayRecursionDepth);
    }

    // Done, finalize
    TheRaytracingPSO->Finalize();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGpuRaytracing()
{
    ComputeContext& computeContext = ComputeContext::Begin("Raytracing");
    {
        // Update global scene constants buffer
        {
            // Get updated scene constants
            RaytracingGlobalCB sceneCB;
            XMStoreFloat4(&sceneCB.CameraPosition, TheRenderScene->GetCamera().GetEye());
            XMStoreFloat4x4(&sceneCB.InverseTransposeViewProjectionMatrix, XMMatrixInverse(nullptr, TheRenderScene->GetCamera().GetViewMatrix() * TheRenderScene->GetCamera().GetProjectionMatrix()));
            sceneCB.OutputResolution                = DirectX::XMFLOAT2((float)Width, (float)Height);
            sceneCB.AORadius                        = UserInput.GpuAORadius;
            sceneCB.FrameCount                      = FrameCount;
            sceneCB.NumRays                         = UserInput.GpuNumRaysPerPixel;
            sceneCB.AccumCount                      = AccumCount++;
            sceneCB.HitProgramCount                 = HitProgramCount;
            sceneCB.NumLights                       = (UINT)TheRenderScene->GetAreaLights().size();
            sceneCB.AOHitGroupIndex                 = RaytracingShaderIndex[RaytracingShaderType_AOHitgroup];
            sceneCB.AOMissIndex                     = RaytracingShaderIndex[RaytracingShaderType_AOMiss];
            sceneCB.DirectLightingHitGroupIndex     = RaytracingShaderIndex[RaytracingShaderType_DirectLightingHitGroup];
            sceneCB.DirectLightingMissIndex         = RaytracingShaderIndex[RaytracingShaderType_DirectLightingMiss];
            sceneCB.IndirectLightingHitGroupIndex   = RaytracingShaderIndex[RaytracingShaderType_IndirectLightingHitGroup];
            sceneCB.IndirectLightingMissIndex       = RaytracingShaderIndex[RaytracingShaderType_IndirectLightingMiss];
            
            // Update GPU buffer
            computeContext.WriteBuffer(RaytracingSceneConstantBuffer, 0, &sceneCB, sizeof(sceneCB));
            computeContext.TransitionResource(RaytracingSceneConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        }

        computeContext.TransitionResource(TheRenderScene->GetAreaLightsBuffer(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        // Set root sig and pipeline state
        computeContext.SetRootSignature(RaytracingGlobalRootSig);
        computeContext.SetPipelineState(*TheRaytracingPSO);

        // Set descriptor heaps and tables
        computeContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RaytracingDescriptorHeap->GetDescriptorHeap());
        for (int i = 0; i < RaytracingGlobalRootSigSlot::Num; i++)
        {
            computeContext.SetDescriptorTable(i, RaytracingDescriptorHeap->GetGpuHandle(i));
        }
        
        // Transition all output buffers
        for (int i = 0; i < 2; i++)
        {
            computeContext.TransitionResource(DirectLightingAOBuffer[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            computeContext.TransitionResource(IndirectLightingBuffer[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }
        
        // Flush all resource barriers before dispatching rays
        computeContext.FlushResourceBarriers();

        // Finally dispatch rays
        computeContext.DispatchRays(*TheRaytracingPSO, Width, Height);

        // Copy current results to previous
        computeContext.CopyBuffer(DirectLightingAOBuffer[0], DirectLightingAOBuffer[1]);
        computeContext.CopyBuffer(IndirectLightingBuffer[0], IndirectLightingBuffer[1]);
    }
    computeContext.Finish(true);
}
