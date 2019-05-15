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

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)
#define ArraySize(a) (sizeof(a)/sizeof(a[0]))

// ----------------------------------------------------------------------------------------------------------------------------

namespace RaytracingGlobalRootSigSlot
{
    enum EnumTypes
    {
        OutputView = 0,
        AccelerationStructure,
        VertexBuffer,
        IndexBuffer,
        SceneConstant,

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
        { 0, 0 },
        { 2, 0 },
        { 3, 0 },
        { 0, 0 },
    };
}

namespace RaytracingLocalRootSigSlot
{
    enum Enum
    {
        MaterialConstant = 0,

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
        { 1, SizeOfInUint32(PrimitiveConstantBuffer) },
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
    static const wchar_t* sMissHitExportName[]  = { sMissShaderName, sClosestHitShaderName };

    RaytracingBufferType = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Global root sig
    {
        //RaytracingGlobalRootSig.Reset(RaytracingGlobalRootSigSlot::Num, 0);
        RaytracingGlobalRootSig.Reset(2, 0);

        RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::OutputView].InitAsDescriptorRange(
            D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
            RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::OutputView][RaytracingGlobalRootSigSlot::Register],
            RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::OutputView][RaytracingGlobalRootSigSlot::Count],
            D3D12_SHADER_VISIBILITY_ALL);

        RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::AccelerationStructure].InitAsBufferSRV(
            RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::AccelerationStructure][RaytracingGlobalRootSigSlot::Register],
            D3D12_SHADER_VISIBILITY_ALL);
#if 0
        RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::VertexBuffer].InitAsBufferSRV(
            RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::VertexBuffer][RaytracingGlobalRootSigSlot::Register],
            D3D12_SHADER_VISIBILITY_ALL);

        RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::IndexBuffer].InitAsBufferSRV(
            RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::IndexBuffer][RaytracingGlobalRootSigSlot::Register],
            D3D12_SHADER_VISIBILITY_ALL);

        RaytracingGlobalRootSig[RaytracingGlobalRootSigSlot::SceneConstant].InitAsConstantBuffer(
            RaytracingGlobalRootSigSlot::Range[RaytracingGlobalRootSigSlot::SceneConstant][RaytracingGlobalRootSigSlot::Register],
            D3D12_SHADER_VISIBILITY_ALL);
#endif

        RaytracingGlobalRootSig.Finalize("RealtimeRaytracingGlobalRoot", D3D12_ROOT_SIGNATURE_FLAG_NONE);
    }

    // Raygen local root sig setup
    {
        RaygenLocalRootSig.Reset(0, 0);
        RaygenLocalRootSig.Finalize("RaygenLocalRootSig", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
#if 0
        RaygenLocalRootSig.Reset(RaytracingLocalRootSigSlot::Num, 0);

        RaygenLocalRootSig[RaytracingLocalRootSigSlot::MaterialConstant].InitAsConstants(
            RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::MaterialConstant][RaytracingLocalRootSigSlot::Register],
            RaytracingLocalRootSigSlot::Range[RaytracingLocalRootSigSlot::MaterialConstant][RaytracingLocalRootSigSlot::Count],
            D3D12_SHADER_VISIBILITY_ALL);

        RaygenLocalRootSig.Finalize("RaygenLocalRootSig", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
#endif
    }

    // Hit miss local root sig setup
    {
        HitMissLocalRootSig.Reset(0, 0);
        HitMissLocalRootSig.Finalize("HitMissLocalRootSig", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
    }

    // Setup shaders and configs for the PSO
    {
        // Setup dxlib
        {
            ID3DBlobPtr pDxilLib;
            ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\Raytracing.cso", &pDxilLib));
            RealtimeRaytracingPSO.SetDxilLibrary(pDxilLib, sDxilLibEntryPoints, ArraySize(sDxilLibEntryPoints));
        }

        // Setup hit program
        RealtimeRaytracingPSO.SetHitProgram(nullptr, sClosestHitShaderName, sHitGroupName);

        // Add local root signatures
        RealtimeRaytracingPSO.SetRootSignature(RaytracingGlobalRootSig);
        RealtimeRaytracingPSO.AddLocalRootSignature(&RaygenLocalRootSig);
        RealtimeRaytracingPSO.AddLocalRootSignature(&HitMissLocalRootSig);

        // Associate ray gen shader with local root signature
        RealtimeRaytracingPSO.AddExportAssociationWithLocalRootSignature(&sRaygenShaderName, 1, &RaygenLocalRootSig);

        // Associate miss and closet hit shader with the local root signature
        RealtimeRaytracingPSO.AddExportAssociationWithLocalRootSignature(sMissHitExportName, ArraySize(sMissHitExportName), &HitMissLocalRootSig);

        // Shader config
        RealtimeRaytracingPSO.SetShaderConfig(D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES, sizeof(RayPayload));

        // Associate shaders with shader config
        RealtimeRaytracingPSO.AddExportAssociationWithShaderConfig(sDxilLibEntryPoints, ArraySize(sDxilLibEntryPoints));

        // Pipeline config
        RealtimeRaytracingPSO.SetPipelineConfig(MAX_RAY_RECURSION_DEPTH);

        // Set shader names for the shader tables
        RealtimeRaytracingPSO.GetRaygenShaderTable().SetShaderName(sRaygenShaderName);
        RealtimeRaytracingPSO.GetHitGroupShaderTable().SetShaderName(sHitGroupName);
        RealtimeRaytracingPSO.GetMissShaderTable().SetShaderName(sMissShaderName);
    }

    // Setup resource views
    {

    }

    // Done, finalize
    RealtimeRaytracingPSO.Finalize();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::ComputeRaytracingResults()
{
    ComputeContext& computeContext = ComputeContext::Begin("Raytracing");
    {
        computeContext.SetRootSignature(RaytracingGlobalRootSig);
        computeContext.SetPipelineState(RealtimeRaytracingPSO);
        computeContext.SetDynamicDescriptor(RaytracingGlobalRootSigSlot::OutputView, 0, RaytracingOutputBuffer.GetUAV());
        computeContext.SetBufferSRV(RaytracingGlobalRootSigSlot::AccelerationStructure, TheRenderScene->GetTopLevelAccelerationStructurePointer().GpuVA);
        computeContext.DispatchRays(RealtimeRaytracingPSO, Width, Height);
    }
    computeContext.Finish(true);
}

