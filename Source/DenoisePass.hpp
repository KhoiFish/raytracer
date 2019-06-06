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

        Moments,
        History,
        Direct,
        Indirect,

        Motion,
        PrevLinearZ,
        CurrLinearZ,

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

        { 0, 2, 0, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // Moments
        { 0, 2, 1, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // History
        { 0, 3, 2, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // Direct
        { 0, 3, 3, D3D12_DESCRIPTOR_RANGE_TYPE_UAV },   // Indirect

        { 0, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // Motion   
        { 1, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // PrevLinearZ
        { 2, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // CurrLinearZ  
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

    // Moments
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::Moments] = curHeapOffset;
    curHeapOffset += ARRAYSIZE(MomentsBuffer);
    for (size_t i = 0; i < ARRAYSIZE(MomentsBuffer); i++)
    {
        RendererDescriptorHeap->AllocateBufferUav(
            *MomentsBuffer[i].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            MomentsBufferType);
    }
    
    // History
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::History] = curHeapOffset;
    curHeapOffset += ARRAYSIZE(HistoryBuffer);
    for (size_t i = 0; i < ARRAYSIZE(HistoryBuffer); i++)
    {
        RendererDescriptorHeap->AllocateBufferUav(
            *HistoryBuffer[i].GetResource(),
            D3D12_UAV_DIMENSION_TEXTURE2D,
            D3D12_BUFFER_UAV_FLAG_NONE,
            HistoryBufferType);
    }

    // Direct
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::Direct] = curHeapOffset;
    curHeapOffset += 3;
    RendererDescriptorHeap->AllocateBufferUav(
        *DirectLightingBuffer[LightingBufferType_PrevResults].GetResource(),
        D3D12_UAV_DIMENSION_TEXTURE2D,
        D3D12_BUFFER_UAV_FLAG_NONE,
        DirectIndirectBufferType);

    RendererDescriptorHeap->AllocateBufferUav(
        *DirectLightingBuffer[LightingBufferType_CurrResults].GetResource(),
        D3D12_UAV_DIMENSION_TEXTURE2D,
        D3D12_BUFFER_UAV_FLAG_NONE,
        DirectIndirectBufferType);

    RendererDescriptorHeap->AllocateBufferUav(
        *DirectLightingBuffer[LightingBufferType_DenoiseOutput].GetResource(),
        D3D12_UAV_DIMENSION_TEXTURE2D,
        D3D12_BUFFER_UAV_FLAG_NONE,
        DirectIndirectBufferType);

    // Indirect
    DenoisePassRootSig::HeapOffsets[DenoisePassRootSig::Indirect] = curHeapOffset;
    curHeapOffset += 3;
    RendererDescriptorHeap->AllocateBufferUav(
        *IndirectLightingBuffer[LightingBufferType_PrevResults].GetResource(),
        D3D12_UAV_DIMENSION_TEXTURE2D,
        D3D12_BUFFER_UAV_FLAG_NONE,
        DirectIndirectBufferType);

    RendererDescriptorHeap->AllocateBufferUav(
        *IndirectLightingBuffer[LightingBufferType_CurrResults].GetResource(),
        D3D12_UAV_DIMENSION_TEXTURE2D,
        D3D12_BUFFER_UAV_FLAG_NONE,
        DirectIndirectBufferType);

    RendererDescriptorHeap->AllocateBufferUav(
        *IndirectLightingBuffer[LightingBufferType_DenoiseOutput].GetResource(),
        D3D12_UAV_DIMENSION_TEXTURE2D,
        D3D12_BUFFER_UAV_FLAG_NONE,
        DirectIndirectBufferType);

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

        computeContext.SetRootSignature(DenoiseRootSig);
        computeContext.SetPipelineState(DenoiseReprojectPSO);
        computeContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RendererDescriptorHeap->GetDescriptorHeap());

        for (uint32_t i = 0; i < DenoisePassRootSig::Num; i++)
        {
            uint32_t heapIndex = DenoiseDescriptorIndexStart + DenoisePassRootSig::HeapOffsets[i];
            computeContext.SetDescriptorTable(i, RendererDescriptorHeap->GetGpuHandle(heapIndex));
        }

        computeContext.Dispatch2D(Width, Height, COMPUTE_THREAD_GROUPSIZE_X, COMPUTE_THREAD_GROUPSIZE_Y);
    }
    computeContext.Finish();
}
