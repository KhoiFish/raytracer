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

#include "CommandContext.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "DescriptorHeap.h"
//#include "EngineProfiling.h"

#ifndef RELEASE
    #include <d3d11_2.h>
//    #include <pix3.h>
#endif

using namespace yart;

// ----------------------------------------------------------------------------------------------------------------------------

static ContextManager sContextManager;

// ----------------------------------------------------------------------------------------------------------------------------

void ContextManager::DestroyAllContexts(void)
{
    for (uint32_t i = 0; i < 4; ++i)
        sm_ContextPool[i].clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandContext* ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE Type)
{
    std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);

    auto& AvailableContexts = sm_AvailableContexts[Type];

    CommandContext* ret = nullptr;
    if (AvailableContexts.empty())
    {
        ret = new CommandContext(Type);
        sm_ContextPool[Type].emplace_back(ret);
        ret->Initialize();
    }
    else
    {
        ret = AvailableContexts.front();
        AvailableContexts.pop();
        ret->Reset();
    }
    ASSERT(ret != nullptr);

    ASSERT(ret->m_Type == Type);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

void ContextManager::FreeContext(CommandContext* UsedContext)
{
    ASSERT(UsedContext != nullptr);
    std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);
    sm_AvailableContexts[UsedContext->m_Type].push(UsedContext);
}

// ----------------------------------------------------------------------------------------------------------------------------

ContextManager& ContextManager::Get()
{
    return sContextManager;
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::DestroyAllContexts(void)
{
    LinearAllocator::DestroyAll();
    DynamicDescriptorHeap::DestroyAll();
    sContextManager.DestroyAllContexts();
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandContext& CommandContext::Begin( const std::wstring ID )
{
    CommandContext* NewContext = sContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
    NewContext->SetID(ID);

#if 0
    if (ID.length() > 0)
        EngineProfiling::BeginBlock(ID, NewContext);
#endif

    return *NewContext;
}

// ----------------------------------------------------------------------------------------------------------------------------

ComputeContext& ComputeContext::Begin(const std::wstring& ID, bool Async)
{
    ComputeContext& NewContext = sContextManager.AllocateContext(
        Async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT)->GetComputeContext();
    NewContext.SetID(ID);

#if 0
    if (ID.length() > 0)
        EngineProfiling::BeginBlock(ID, &NewContext);

#endif

    return NewContext;
}

// ----------------------------------------------------------------------------------------------------------------------------

uint64_t CommandContext::Flush(bool WaitForCompletion)
{
    FlushResourceBarriers();

    ASSERT(m_CurrentAllocator != nullptr);

    uint64_t FenceValue = CommandListManager::Get().GetQueue(m_Type).ExecuteCommandList(m_CommandList);

    if (WaitForCompletion)
    {
        CommandListManager::Get().WaitForFence(FenceValue);
    }

    //
    // Reset the command list and restore previous state
    //

    m_CommandList->Reset(m_CurrentAllocator, nullptr);

    if (m_CurGraphicsRootSignature)
    {
        m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature);
        m_CommandList->SetPipelineState(m_CurGraphicsPipelineState);
    }
    if (m_CurComputeRootSignature)
    {
        m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature);
        m_CommandList->SetPipelineState(m_CurComputePipelineState);
    }

    BindDescriptorHeaps();

    return FenceValue;
}

// ----------------------------------------------------------------------------------------------------------------------------

uint64_t CommandContext::Finish( bool WaitForCompletion )
{
    ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_DIRECT || m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE);

    FlushResourceBarriers();

#if 0
    if (m_ID.length() > 0)
        EngineProfiling::EndBlock(this);

#endif

    ASSERT(m_CurrentAllocator != nullptr);

    CommandQueue& Queue = CommandListManager::Get().GetQueue(m_Type);

    uint64_t FenceValue = Queue.ExecuteCommandList(m_CommandList);
    Queue.DiscardAllocator(FenceValue, m_CurrentAllocator);
    m_CurrentAllocator = nullptr;

    m_CpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_GpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_DynamicViewDescriptorHeap.CleanupUsedHeaps(FenceValue);
    m_DynamicSamplerDescriptorHeap.CleanupUsedHeaps(FenceValue);

    if (WaitForCompletion)
    {
        CommandListManager::Get().WaitForFence(FenceValue);
    }

    sContextManager.FreeContext(this);

    return FenceValue;
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE Type) :
    m_Type(Type),
    m_DynamicViewDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
    m_DynamicSamplerDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
    m_CpuLinearAllocator(kCpuWritable), 
    m_GpuLinearAllocator(kGpuExclusive)
{
    m_OwningManager = nullptr;
    m_CommandList = nullptr;
    m_CurrentAllocator = nullptr;
    ZeroMemory(m_CurrentDescriptorHeaps, sizeof(m_CurrentDescriptorHeaps));

    m_CurGraphicsRootSignature = nullptr;
    m_CurGraphicsPipelineState = nullptr;
    m_CurComputeRootSignature = nullptr;
    m_CurComputePipelineState = nullptr;
    m_NumBarriersToFlush = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandContext::~CommandContext( void )
{
    if (m_CommandList != nullptr)
    {
        m_CommandList->Release();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::Initialize(void)
{
    CommandListManager::Get().CreateNewCommandList(m_Type, &m_CommandList, &m_CurrentAllocator);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::Reset( void )
{
    // We only call Reset() on previously freed contexts.  The command list persists, but we must
    // request a new allocator.
    ASSERT(m_CommandList != nullptr && m_CurrentAllocator == nullptr);
    m_CurrentAllocator = CommandListManager::Get().GetQueue(m_Type).RequestAllocator();
    m_CommandList->Reset(m_CurrentAllocator, nullptr);

    m_CurGraphicsRootSignature = nullptr;
    m_CurGraphicsPipelineState = nullptr;
    m_CurComputeRootSignature = nullptr;
    m_CurComputePipelineState = nullptr;
    m_NumBarriersToFlush = 0;

    BindDescriptorHeaps();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::BindDescriptorHeaps( void )
{
    UINT NonNullHeaps = 0;
    ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        ID3D12DescriptorHeap* HeapIter = m_CurrentDescriptorHeaps[i];
        if (HeapIter != nullptr)
            HeapsToBind[NonNullHeaps++] = HeapIter;
    }

    if (NonNullHeaps > 0)
        m_CommandList->SetDescriptorHeaps(NonNullHeaps, HeapsToBind);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetRenderTargets( UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV )
{
    m_CommandList->OMSetRenderTargets( NumRTVs, RTVs, FALSE, &DSV );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[])
{
    m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
    m_CommandList->BeginQuery(QueryHeap, Type, HeapIndex);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
    m_CommandList->EndQuery(QueryHeap, Type, HeapIndex);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset)
{
    m_CommandList->ResolveQueryData(QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer, DestinationBufferOffset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearUAV( GpuBuffer& Target )
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    const UINT ClearColor[4] = {};
    m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::ClearUAV( GpuBuffer& Target )
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    const UINT ClearColor[4] = {};
    m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearUAV( ColorBuffer& Target )
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

    //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
    const float* ClearColor = Target.GetClearColor().GetPtr();
    m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::ClearUAV( ColorBuffer& Target )
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
    CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

    //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
    const float* ClearColor = Target.GetClearColor().GetPtr();
    m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetRootSignature(const RootSignature& RootSig)
{
    if (RootSig.GetSignature() == m_CurComputeRootSignature)
        return;

    m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature = RootSig.GetSignature());

    m_DynamicViewDescriptorHeap.ParseComputeRootSignature(RootSig);
    m_DynamicSamplerDescriptorHeap.ParseComputeRootSignature(RootSig);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetPipelineState(const ComputePSO& PSO)
{
    ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
    if (PipelineState == m_CurComputePipelineState)
        return;

    m_CommandList->SetPipelineState(PipelineState);
    m_CurComputePipelineState = PipelineState;
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstantArray(UINT RootEntry, UINT NumConstants, const void* pConstants)
{
    m_CommandList->SetComputeRoot32BitConstants(RootEntry, NumConstants, pConstants, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstant(UINT RootEntry, DWParam Val, UINT Offset)
{
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, Val.Uint, Offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstants(UINT RootEntry, DWParam X)
{
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, X.Uint, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstants(UINT RootEntry, DWParam X, DWParam Y)
{
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, X.Uint, 0);
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, Y.Uint, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstants(UINT RootEntry, DWParam X, DWParam Y, DWParam Z)
{
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, X.Uint, 0);
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, Y.Uint, 1);
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, Z.Uint, 2);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstants(UINT RootEntry, DWParam X, DWParam Y, DWParam Z, DWParam W)
{
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, X.Uint, 0);
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, Y.Uint, 1);
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, Z.Uint, 2);
    m_CommandList->SetComputeRoot32BitConstant(RootEntry, W.Uint, 3);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
    m_CommandList->SetComputeRootConstantBufferView(RootIndex, CBV);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
    ASSERT(BufferData != nullptr && IsAligned(BufferData, 16));
    DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
    //SIMDMemCopy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16) >> 4);
    memcpy(cb.DataPtr, BufferData, BufferSize);
    m_CommandList->SetComputeRootConstantBufferView(RootIndex, cb.GpuAddress);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
    ASSERT(BufferData != nullptr && IsAligned(BufferData, 16));
    DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
    SIMDMemCopy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16) >> 4);
    m_CommandList->SetComputeRootShaderResourceView(RootIndex, cb.GpuAddress);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset)
{
    ASSERT((SRV.UsageState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0);
    m_CommandList->SetComputeRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset)
{
    ASSERT((UAV.UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
    m_CommandList->SetComputeRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::Dispatch(size_t GroupCountX, size_t GroupCountY, size_t GroupCountZ)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_CommandList->Dispatch((UINT)GroupCountX, (UINT)GroupCountY, (UINT)GroupCountZ);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::Dispatch1D(size_t ThreadCountX, size_t GroupSizeX)
{
    Dispatch(DivideByMultiple(ThreadCountX, GroupSizeX), 1, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::Dispatch2D(size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX, size_t GroupSizeY)
{
    Dispatch(
        DivideByMultiple(ThreadCountX, GroupSizeX),
        DivideByMultiple(ThreadCountY, GroupSizeY), 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::Dispatch3D(size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ)
{
    Dispatch(
        DivideByMultiple(ThreadCountX, GroupSizeX),
        DivideByMultiple(ThreadCountY, GroupSizeY),
        DivideByMultiple(ThreadCountZ, GroupSizeZ));
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
    SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
    m_DynamicViewDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

void ComputeContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
    SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
}

void ComputeContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
    m_DynamicSamplerDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
{
    m_CommandList->SetComputeRootDescriptorTable(RootIndex, FirstHandle);
}

// ----------------------------------------------------------------------------------------------------------------------------

#if 0
void ComputeContext::ExecuteIndirect(CommandSignature& CommandSig, GpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset, uint32_t MaxCommands, GpuBuffer* CommandCounterBuffer, uint64_t CounterOffset)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
        ArgumentBuffer.GetResource(), ArgumentStartOffset,
        CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::DispatchIndirect(GpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset)
{
    ExecuteIndirect(Graphics::DispatchIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
}

#endif

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearColor( ColorBuffer& Target )
{
    m_CommandList->ClearRenderTargetView(Target.GetRTV(), Target.GetClearColor().GetPtr(), 0, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearDepth( DepthBuffer& Target )
{
    m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearStencil( DepthBuffer& Target )
{
    m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearDepthAndStencil( DepthBuffer& Target )
{
    m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetViewportAndScissor( const D3D12_VIEWPORT& vp, const D3D12_RECT& rect )
{
    ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    m_CommandList->RSSetViewports( 1, &vp );
    m_CommandList->RSSetScissorRects( 1, &rect );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h)
{
    SetViewport((float)x, (float)y, (float)w, (float)h);
    SetScissor(x, y, x + w, y + h);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetViewport( const D3D12_VIEWPORT& vp )
{
    m_CommandList->RSSetViewports( 1, &vp );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetViewport( FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth, FLOAT maxDepth )
{
    D3D12_VIEWPORT vp;
    vp.Width = w;
    vp.Height = h;
    vp.MinDepth = minDepth;
    vp.MaxDepth = maxDepth;
    vp.TopLeftX = x;
    vp.TopLeftY = y;
    m_CommandList->RSSetViewports( 1, &vp );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetScissor( const D3D12_RECT& rect )
{
    ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    m_CommandList->RSSetScissorRects( 1, &rect );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetScissor(UINT left, UINT top, UINT right, UINT bottom)
{
    SetScissor(CD3DX12_RECT(left, top, right, bottom));
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetRootSignature(const RootSignature& RootSig)
{
    if (RootSig.GetSignature() == m_CurGraphicsRootSignature)
        return;

    m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature = RootSig.GetSignature());

    m_DynamicViewDescriptorHeap.ParseGraphicsRootSignature(RootSig);
    m_DynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(RootSig);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetPipelineState(const GraphicsPSO& PSO)
{
    ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
    if (PipelineState == m_CurGraphicsPipelineState)
        return;

    m_CommandList->SetPipelineState(PipelineState);
    m_CurGraphicsPipelineState = PipelineState;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetStencilRef(UINT ref)
{
    m_CommandList->OMSetStencilRef(ref);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetBlendFactor(Color BlendFactor)
{
    m_CommandList->OMSetBlendFactor(BlendFactor.GetPtr());
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
{
    m_CommandList->IASetPrimitiveTopology(Topology);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants)
{
    m_CommandList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstant(UINT RootEntry, DWParam Val, UINT Offset)
{
    m_CommandList->SetGraphicsRoot32BitConstant(RootEntry, Val.Uint, Offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstants(UINT RootIndex, DWParam X)
{
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y)
{
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
{
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
{
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
    m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
    m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
    ASSERT(BufferData != nullptr && IsAligned(BufferData, 16));
    DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
    //SIMDMemCopy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16) >> 4);
    memcpy(cb.DataPtr, BufferData, BufferSize);
    m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VertexData)
{
    ASSERT(VertexData != nullptr && IsAligned(VertexData, 16));

    size_t BufferSize = AlignUp(NumVertices * VertexStride, 16);
    DynAlloc vb = m_CpuLinearAllocator.Allocate(BufferSize);

    SIMDMemCopy(vb.DataPtr, VertexData, BufferSize >> 4);

    D3D12_VERTEX_BUFFER_VIEW VBView;
    VBView.BufferLocation = vb.GpuAddress;
    VBView.SizeInBytes = (UINT)BufferSize;
    VBView.StrideInBytes = (UINT)VertexStride;

    m_CommandList->IASetVertexBuffers(Slot, 1, &VBView);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicIB(size_t IndexCount, const uint16_t* IndexData)
{
    ASSERT(IndexData != nullptr && IsAligned(IndexData, 16));

    size_t BufferSize = AlignUp(IndexCount * sizeof(uint16_t), 16);
    DynAlloc ib = m_CpuLinearAllocator.Allocate(BufferSize);

    SIMDMemCopy(ib.DataPtr, IndexData, BufferSize >> 4);

    D3D12_INDEX_BUFFER_VIEW IBView;
    IBView.BufferLocation = ib.GpuAddress;
    IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
    IBView.Format = DXGI_FORMAT_R16_UINT;

    m_CommandList->IASetIndexBuffer(&IBView);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
    ASSERT(BufferData != nullptr && IsAligned(BufferData, 16));
    DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
    SIMDMemCopy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16) >> 4);
    m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuAddress);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset)
{
    ASSERT((SRV.UsageState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
    m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset)
{
    ASSERT((UAV.UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
    m_CommandList->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
    SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
}

void GraphicsContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
    m_DynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
    SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
    m_DynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
{
    m_CommandList->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
{
    m_CommandList->IASetIndexBuffer(&IBView);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
{
    SetVertexBuffers(Slot, 1, &VBView);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
{
    m_CommandList->IASetVertexBuffers(StartSlot, Count, VBViews);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::Draw(UINT VertexCount, UINT VertexStartOffset)
{
    DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

// ----------------------------------------------------------------------------------------------------------------------------

#if 0
void GraphicsContext::ExecuteIndirect(CommandSignature& CommandSig, GpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset, uint32_t MaxCommands, GpuBuffer* CommandCounterBuffer, uint64_t CounterOffset)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
        ArgumentBuffer.GetResource(), ArgumentStartOffset,
        CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::DrawIndirect(GpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset)
{
    ExecuteIndirect(Graphics::DrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
}
#endif

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::TransitionResource(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
    D3D12_RESOURCE_STATES OldState = Resource.UsageState;

    if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        ASSERT((OldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == OldState);
        ASSERT((NewState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == NewState);
    }

    if (OldState != NewState)
    {
        ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = Resource.GetResource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = OldState;
        BarrierDesc.Transition.StateAfter = NewState;

        // Check to see if we already started the transition
        if (NewState == Resource.TransitioningState)
        {
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
            Resource.TransitioningState = (D3D12_RESOURCE_STATES)-1;
        }
        else
            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

        Resource.UsageState = NewState;
    }
    else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
        InsertUAVBarrier(Resource, FlushImmediate);

    if (FlushImmediate || m_NumBarriersToFlush == 16)
        FlushResourceBarriers();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::BeginResourceTransition(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
    // If it's already transitioning, finish that transition
    if (Resource.TransitioningState != (D3D12_RESOURCE_STATES)-1)
        TransitionResource(Resource, Resource.TransitioningState);

    D3D12_RESOURCE_STATES OldState = Resource.UsageState;

    if (OldState != NewState)
    {
        ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        BarrierDesc.Transition.pResource = Resource.GetResource();
        BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        BarrierDesc.Transition.StateBefore = OldState;
        BarrierDesc.Transition.StateAfter = NewState;

        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

        Resource.TransitioningState = NewState;
    }

    if (FlushImmediate || m_NumBarriersToFlush == 16)
        FlushResourceBarriers();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InsertUAVBarrier(GpuResource& Resource, bool FlushImmediate)
{
    ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.UAV.pResource = Resource.GetResource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InsertAliasBarrier(GpuResource& Before, GpuResource& After, bool FlushImmediate)
{
    ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

    BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    BarrierDesc.Aliasing.pResourceBefore = Before.GetResource();
    BarrierDesc.Aliasing.pResourceAfter = After.GetResource();

    if (FlushImmediate)
        FlushResourceBarriers();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::WriteBuffer( GpuResource& Dest, size_t DestOffset, const void* BufferData, size_t NumBytes )
{
    ASSERT(BufferData != nullptr && IsAligned(BufferData, 16));
    DynAlloc TempSpace = m_CpuLinearAllocator.Allocate( NumBytes, 512 );
    SIMDMemCopy(TempSpace.DataPtr, BufferData, DivideByMultiple(NumBytes, 16));
    CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes );
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::FillBuffer( GpuResource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes )
{
    DynAlloc TempSpace = m_CpuLinearAllocator.Allocate( NumBytes, 512 );
    __m128 VectorValue = _mm_set1_ps(Value.Float);
    SIMDMemFill(TempSpace.DataPtr, VectorValue, DivideByMultiple(NumBytes, 16));
    CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes );
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InitializeTexture( GpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[] )
{
    UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.GetResource(), 0, NumSubresources);

    CommandContext& InitContext = CommandContext::Begin();

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    DynAlloc mem = InitContext.ReserveUploadMemory(uploadBufferSize);
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    InitContext.FlushResourceBarriers();
    UpdateSubresources(InitContext.m_CommandList, Dest.GetResource(), mem.Buffer.GetResource(), 0, 0, NumSubresources, SubData);
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    InitContext.Finish(true);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::CopySubresource(GpuResource& Dest, UINT DestSubIndex, GpuResource& Src, UINT SrcSubIndex)
{
    FlushResourceBarriers();

    D3D12_TEXTURE_COPY_LOCATION DestLocation =
    {
        Dest.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        DestSubIndex
    };

    D3D12_TEXTURE_COPY_LOCATION SrcLocation =
    {
        Src.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        SrcSubIndex
    };

    m_CommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InitializeTextureArraySlice(GpuResource& Dest, UINT SliceIndex, GpuResource& Src)
{
    CommandContext& Context = CommandContext::Begin();

    Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    Context.FlushResourceBarriers();

    const D3D12_RESOURCE_DESC& DestDesc = Dest.GetResource()->GetDesc();
    const D3D12_RESOURCE_DESC& SrcDesc = Src.GetResource()->GetDesc();

    ASSERT(SliceIndex < DestDesc.DepthOrArraySize &&
        SrcDesc.DepthOrArraySize == 1 &&
        DestDesc.Width == SrcDesc.Width &&
        DestDesc.Height == SrcDesc.Height &&
        DestDesc.MipLevels <= SrcDesc.MipLevels
        );

    UINT SubResourceIndex = SliceIndex * DestDesc.MipLevels;

    for (UINT i = 0; i < DestDesc.MipLevels; ++i)
    {
        D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
        {
            Dest.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            SubResourceIndex + i
        };

        D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
        {
            Src.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            i
        };

        Context.m_CommandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
    }

    Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);
    Context.Finish(true);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InitializeBuffer( GpuResource& Dest, const void* BufferData, size_t NumBytes, size_t Offset)
{
    CommandContext& InitContext = CommandContext::Begin();

    DynAlloc mem = InitContext.ReserveUploadMemory(NumBytes);
    SIMDMemCopy(mem.DataPtr, BufferData, DivideByMultiple(NumBytes, 16));

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
    InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), Offset, mem.Buffer.GetResource(), 0, NumBytes);
    InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    InitContext.Finish(true);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::PIXBeginEvent(const wchar_t* label)
{
#ifdef RELEASE
    (label);
#else
    //::PIXBeginEvent(m_CommandList, 0, label);
#endif
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::PIXEndEvent(void)
{
#ifndef RELEASE
    //::PIXEndEvent(m_CommandList);
#endif
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::PIXSetMarker(const wchar_t* label)
{
#ifdef RELEASE
    (label);
#else
    //::PIXSetMarker(m_CommandList, 0, label);
#endif
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::FlushResourceBarriers(void)
{
    if (m_NumBarriersToFlush > 0)
    {
        m_CommandList->ResourceBarrier(m_NumBarriersToFlush, m_ResourceBarrierBuffer);
        m_NumBarriersToFlush = 0;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr)
{
    if (m_CurrentDescriptorHeaps[Type] != HeapPtr)
    {
        m_CurrentDescriptorHeaps[Type] = HeapPtr;
        BindDescriptorHeaps();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[])
{
    bool AnyChanged = false;

    for (UINT i = 0; i < HeapCount; ++i)
    {
        if (m_CurrentDescriptorHeaps[Type[i]] != HeapPtrs[i])
        {
            m_CurrentDescriptorHeaps[Type[i]] = HeapPtrs[i];
            AnyChanged = true;
        }
    }

    if (AnyChanged)
        BindDescriptorHeaps();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
{
    m_CommandList->SetPredication(Buffer, BufferOffset, Op);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::CopyBuffer(GpuResource& Dest, GpuResource& Src)
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    m_CommandList->CopyResource(Dest.GetResource(), Src.GetResource());
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::CopyBufferRegion(GpuResource& Dest, size_t DestOffset, GpuResource& Src, size_t SrcOffset, size_t NumBytes)
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    //TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetResource(), SrcOffset, NumBytes);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::CopyCounter(GpuResource& Dest, size_t DestOffset, StructuredBuffer& Src)
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(Src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetCounterBuffer().GetResource(), 0, 4);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::ResetCounter(StructuredBuffer& Buf, uint32_t Value)
{
    FillBuffer(Buf.GetCounterBuffer(), 0, Value, sizeof(uint32_t));
    TransitionResource(Buf.GetCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx)
{
    m_CommandList->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, QueryIdx);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries)
{
    m_CommandList->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, NumQueries, pReadbackHeap, 0);
}
