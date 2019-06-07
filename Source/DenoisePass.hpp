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

namespace DenoisePassRootSig
{
    enum EnumTypes
    {
        DenoiseCB = 0,

        PrevReprojMoments,
        OutpReprojMoments,

        PrevReprojHistory,
        OutpReprojHistory,

        PrevReprojDirect,
        OutpReprojDirect,

        PrevReprojIndirect,
        OutpReprojIndirect,

        Motion,

        PrevLinearZ,
        CurrLinearZ,
        
        SourceDirect,
        SourceIndirect,

        Num,
    };

    enum IndexTypes
    {
        Register = 0,
        Count,
        Space,
        RangeType,
    };

    // [register, count, space, D3D12_DESCRIPTOR_RANGE_TYPE]
    static UINT Range[DenoisePassRootSig::Num][4] =
    {
        { 0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // DenoiseCB

        { 0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // PrevReprojMoments
        { 1, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // OutpReprojMoments

        { 2, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // PrevReprojHistory
        { 3, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // OutpReprojHistory

        { 4, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // PrevReprojDirect
        { 5, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // OuptpReprojDirect

        { 6, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // PrevReprojIndirect
        { 7, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // OutpReprojIndirect

        { 0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // Motion

        { 1, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // PrevLinearZ
        { 2, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // CurrLinearZ

        { 3, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // SourceDirect
        { 4, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // SourceIndirect
    };

    static INT HeapOffsets[DenoisePassRootSig::Num];
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::CleanupDenoisePass()
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupDenoisePipeline()
{
    // Constant buffers
    DenoiseShaderConstantBuffer.Create(L"Denoise Constant Buffer", 1, (uint32_t)AlignUp(sizeof(DenoiseConstantBuffer), 256));

    // Root sig setup
    {
        DenoiseRootSig.Reset(DenoisePassRootSig::Num, 0);
        {
            for (int i = 0; i < DenoisePassRootSig::Num; i++)
            {
                DenoiseRootSig[i].InitAsDescriptorRange(
                    (D3D12_DESCRIPTOR_RANGE_TYPE)DenoisePassRootSig::Range[i][DenoisePassRootSig::RangeType],
                    DenoisePassRootSig::Range[i][DenoisePassRootSig::Register],
                    DenoisePassRootSig::Range[i][DenoisePassRootSig::Count],
                    DenoisePassRootSig::Range[i][DenoisePassRootSig::Space],
                    D3D12_SHADER_VISIBILITY_ALL);
            }
        }
        DenoiseRootSig.Finalize("DenoiseRootSig", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    // Reprojection PSO Setup
    {
        Microsoft::WRL::ComPtr<ID3DBlob> computeShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\SVGFReproject_CS.cso", &computeShaderBlob));

        DenoiseReprojectPSO.SetRootSignature(DenoiseRootSig);
        DenoiseReprojectPSO.SetComputeShader(computeShaderBlob->GetBufferPointer(), computeShaderBlob->GetBufferSize());
        DenoiseReprojectPSO.Finalize();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupDenoiseDescriptors()
{
    DenoiseDescriptorIndexStart = RendererDescriptorHeap->GetCount();

    UINT curHeapOffset = 0;

    // Denoise CB
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::DenoiseCB] = curHeapOffset++;
    RendererDescriptorHeap->AllocateBufferCbv(
        DenoiseShaderConstantBuffer.GetGpuVirtualAddress(),
        (UINT)DenoiseShaderConstantBuffer.GetBufferSize());

    // Reproj Moments
    curHeapOffset += 2;
    ReprojDescriptors[ReprojBufferType_Moments].Reset
    (
        RendererDescriptorHeap,

        DenoisePassRootSig::PrevReprojMoments,
        DenoisePassRootSig::OutpReprojMoments,

        RendererDescriptorHeap->AllocateBufferUav(
            *ReprojectionBuffers[0][ReprojBufferType_Moments].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            MomentsBufferType),
        RendererDescriptorHeap->AllocateBufferUav(
            *ReprojectionBuffers[1][ReprojBufferType_Moments].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            MomentsBufferType)
    );
    
    // Reproj History
    curHeapOffset += 2;
    ReprojDescriptors[ReprojBufferType_History].Reset
    (
        RendererDescriptorHeap,

        DenoisePassRootSig::PrevReprojHistory,
        DenoisePassRootSig::OutpReprojHistory,

        RendererDescriptorHeap->AllocateBufferUav(
            *ReprojectionBuffers[0][ReprojBufferType_History].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            HistoryBufferType),
        RendererDescriptorHeap->AllocateBufferUav(
            *ReprojectionBuffers[1][ReprojBufferType_History].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            HistoryBufferType)
    );

    // Reproj Direct
    curHeapOffset += 2;
    ReprojDescriptors[ReprojBufferType_Direct].Reset
    (
        RendererDescriptorHeap,

        DenoisePassRootSig::PrevReprojDirect,
        DenoisePassRootSig::OutpReprojDirect,

        RendererDescriptorHeap->AllocateBufferUav(
            *ReprojectionBuffers[0][ReprojBufferType_Direct].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            DirectIndirectRTBufferType),
        RendererDescriptorHeap->AllocateBufferUav(
            *ReprojectionBuffers[1][ReprojBufferType_Direct].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            DirectIndirectRTBufferType)
    );

    // Reproj Indirect
    curHeapOffset += 2;
    ReprojDescriptors[ReprojBufferType_Indirect].Reset
    (
        RendererDescriptorHeap,

        DenoisePassRootSig::PrevReprojIndirect,
        DenoisePassRootSig::OutpReprojIndirect,

        RendererDescriptorHeap->AllocateBufferUav(
            *ReprojectionBuffers[0][ReprojBufferType_Indirect].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            DirectIndirectRTBufferType),
        RendererDescriptorHeap->AllocateBufferUav(
            *ReprojectionBuffers[1][ReprojBufferType_Indirect].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            DirectIndirectRTBufferType)
    );

    // Motion
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::Motion] = curHeapOffset++;
    RendererDescriptorHeap->AllocateTexture2DSrv(
        GBuffers[GBufferType_SVGFMoVec].GetResource(),
        GBufferRTTypes[GBufferType_SVGFMoVec]);

    // PrevLinearZ
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::PrevLinearZ] = curHeapOffset++;
    RendererDescriptorHeap->AllocateTexture2DSrv(
        GBuffers[GBufferType_PrevSVGFLinearZ].GetResource(),
        GBufferRTTypes[GBufferType_PrevSVGFLinearZ]);

    // CurrLinearZ
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::CurrLinearZ] = curHeapOffset++;
    RendererDescriptorHeap->AllocateTexture2DSrv(
        GBuffers[GBufferType_CurrSVGFLinearZ].GetResource(),
        GBufferRTTypes[GBufferType_CurrSVGFLinearZ]);

    // Source Direct
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::SourceDirect] = curHeapOffset++;
    RendererDescriptorHeap->AllocateTexture2DSrv(
        DirectLightingBuffer[DirectIndirectBufferType_Results].GetResource(),
        DirectIndirectRTBufferType);

    // Source Indirect
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::SourceIndirect] = curHeapOffset++;
    RendererDescriptorHeap->AllocateTexture2DSrv(
        IndirectLightingBuffer[DirectIndirectBufferType_Results].GetResource(),
        DirectIndirectRTBufferType);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderDenoisePass()
{
    ComputeContext& computeContext = ComputeContext::Begin("Reprojection");
    {
        // Setup Denoise CB
        {
            DenoiseConstantBuffer denoiseCB;
            denoiseCB.Alpha        = 0.05f;
            denoiseCB.MomentsAlpha = 0.2f;

            DenoiseShaderConstantBuffer.Upload(&denoiseCB, sizeof(denoiseCB));
        }
        
        // Set root signature and heap
        computeContext.SetRootSignature(DenoiseRootSig);
        computeContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RendererDescriptorHeap->GetDescriptorHeap());

        // Reprojection
        {
            computeContext.SetPipelineState(DenoiseReprojectPSO);
            
            // Set descriptor tables
            for (int i = 0; i < DenoisePassRootSig::Num; i++)
            {
                uint32_t heapIndex = DenoiseDescriptorIndexStart + DenoisePassRootSig::HeapOffsets[i];
                computeContext.SetDescriptorTable(i, RendererDescriptorHeap->GetGpuHandle(heapIndex));
            }

            for (int i = 0; i < ReprojBufferType_Num; i++)
            {
                computeContext.SetDescriptorTable(ReprojDescriptors[i]);
            }

            // Dispatch
            computeContext.Dispatch2D(Width, Height, COMPUTE_THREAD_GROUPSIZE_X, COMPUTE_THREAD_GROUPSIZE_Y);
        }


        // Ping pong resources
        for (int i = 0; i < ReprojBufferType_Num; i++)
        {
            ReprojDescriptors[i].PingPong();
        }
        computeContext.CopyBuffer(GBuffers[GBufferType_PrevSVGFLinearZ], GBuffers[GBufferType_CurrSVGFLinearZ]);
    }
    computeContext.Finish();
}
