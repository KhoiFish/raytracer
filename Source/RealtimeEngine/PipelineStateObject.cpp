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

#include "PipelineStateObject.h"
#include "RenderDevice.h"
#include <memory.h>
#include <map>
#include <thread>
#include <mutex>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace yart;

// ----------------------------------------------------------------------------------------------------------------------------

static map< size_t, ComPtr<ID3D12PipelineState> > sGraphicsPSOHashMap;
static map< size_t, ComPtr<ID3D12PipelineState> > sComputePSOHashMap;

// ----------------------------------------------------------------------------------------------------------------------------

void PSO::DestroyAll(void)
{
    sGraphicsPSOHashMap.clear();
    sComputePSOHashMap.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

GraphicsPSO::GraphicsPSO()
{
    ZeroMemory(&PSODesc, sizeof(PSODesc));
    PSODesc.NodeMask                = 1;
    PSODesc.SampleMask              = 0xFFFFFFFFu;
    PSODesc.SampleDesc.Count        = 1;
    PSODesc.InputLayout.NumElements = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& BlendDesc)
{
    PSODesc.BlendState = BlendDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc)
{
    PSODesc.RasterizerState = RasterizerDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc)
{
    PSODesc.DepthStencilState = DepthStencilDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetSampleMask(UINT SampleMask)
{
    PSODesc.SampleMask = SampleMask;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
    ASSERT(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
    PSODesc.PrimitiveTopologyType = TopologyType;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
{
    PSODesc.IBStripCutValue = IBProps;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    ASSERT(NumRTVs == 0 || RTVFormats != nullptr, "Null format array conflicts with non-zero length");

    for (UINT i = 0; i < NumRTVs; ++i)
    {
        PSODesc.RTVFormats[i] = RTVFormats[i];
    }

    for (UINT i = NumRTVs; i < PSODesc.NumRenderTargets; ++i)
    {
        PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    }

    PSODesc.NumRenderTargets   = NumRTVs;
    PSODesc.DSVFormat          = DSVFormat;
    PSODesc.SampleDesc.Count   = MsaaCount;
    PSODesc.SampleDesc.Quality = MsaaQuality;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC * pInputElementDescs)
{
    PSODesc.InputLayout.NumElements = NumElements;

    if (NumElements > 0)
    {
        D3D12_INPUT_ELEMENT_DESC* NewElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements);
        memcpy(NewElements, pInputElementDescs, NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
        InputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC*)NewElements);
    }
    else
    {
        InputLayouts = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsPSO::Finalize()
{
    ASSERT(RootSig->GetSignature() != nullptr);

    // Make sure the root signature is finalized first
    PSODesc.pRootSignature = RootSig->GetSignature();
    PSODesc.InputLayout.pInputElementDescs = nullptr;
    PSODesc.InputLayout.pInputElementDescs = InputLayouts.get();

    size_t                hashCode = HashState(InputLayouts.get(), PSODesc.InputLayout.NumElements, HashState(&PSODesc));
    ID3D12PipelineState * *PSORef  = nullptr;
    bool firstCompile = false;
    {
        static mutex sHashMapMutex;
        lock_guard<mutex> CS(sHashMapMutex);
        auto iter = sGraphicsPSOHashMap.find(hashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == sGraphicsPSOHashMap.end())
        {
            firstCompile = true;
            PSORef = sGraphicsPSOHashMap[hashCode].GetAddressOf();
        }
        else
        {
            PSORef = iter->second.GetAddressOf();
        }
    }

    if (firstCompile)
    {
        ASSERT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSOObject)));
        sGraphicsPSOHashMap[hashCode].Attach(PSOObject);
    }
    else
    {
        while (*PSORef == nullptr)
        {
            this_thread::yield();
        }
        PSOObject = *PSORef;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputePSO::Finalize()
{
    // Make sure the root signature is finalized first
    PSODesc.pRootSignature = RootSig->GetSignature();
    ASSERT(PSODesc.pRootSignature != nullptr);

    size_t                hashCode = HashState(&PSODesc);
    ID3D12PipelineState * *PSORef = nullptr;
    bool firstCompile = false;
    {
        static mutex sHashMapMutex;
        lock_guard<mutex> CS(sHashMapMutex);
        auto iter = sComputePSOHashMap.find(hashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == sComputePSOHashMap.end())
        {
            firstCompile = true;
            PSORef = sComputePSOHashMap[hashCode].GetAddressOf();
        }
        else
        {
            PSORef = iter->second.GetAddressOf();
        }
    }

    if (firstCompile)
    {
        ASSERT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(&PSOObject)));
        sComputePSOHashMap[hashCode].Attach(PSOObject);
    }
    else
    {
        while (*PSORef == nullptr)
        {
            this_thread::yield();
        }
        PSOObject = *PSORef;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

ComputePSO::ComputePSO()
{
    ZeroMemory(&PSODesc, sizeof(PSODesc));
    PSODesc.NodeMask = 1;
}
