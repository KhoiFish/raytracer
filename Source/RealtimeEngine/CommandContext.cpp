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
#include "RenderTarget.h"
#include "DescriptorHeapStack.h"
#include "RenderDevice.h"

using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

static ContextManager sContextManager;

// ----------------------------------------------------------------------------------------------------------------------------

void ContextManager::DestroyAllContexts(void)
{
    for (uint32_t i = 0; i < 4; ++i)
    {
        ContextPool[i].clear();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandContext* ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE type)
{
    std::lock_guard<std::mutex> lockGuard(ContextAllocationMutex);

    auto& availableContexts = AvailableContexts[type];

    CommandContext* ret = nullptr;
    if (availableContexts.empty())
    {
        ret = new CommandContext(type);
        ContextPool[type].emplace_back(ret);
        ret->Initialize();
    }
    else
    {
        ret = availableContexts.front();
        availableContexts.pop();
        ret->Reset();
    }
    RTL_ASSERT(ret != nullptr);

    RTL_ASSERT(ret->Type == type);

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

void ContextManager::FreeContext(CommandContext* usedContext)
{
    RTL_ASSERT(usedContext != nullptr);
    std::lock_guard<std::mutex> lockGuard(ContextAllocationMutex);
    AvailableContexts[usedContext->Type].push(usedContext);
}


// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::DestroyAllContexts()
{
    LinearAllocator::DestroyAll();
    sContextManager.DestroyAllContexts();
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandContext& CommandContext::Begin(const string_t id)
{
    CommandContext* NewContext = sContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
    NewContext->SetID(id);

    return *NewContext;
}

// ----------------------------------------------------------------------------------------------------------------------------

ComputeContext& ComputeContext::Begin(const string_t& id, bool async)
{
    ComputeContext& NewContext = sContextManager.AllocateContext(
        async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT)->GetComputeContext();
    NewContext.SetID(id);

    return NewContext;
}

// ----------------------------------------------------------------------------------------------------------------------------

uint64_t CommandContext::Flush(bool waitForCompletion)
{
    FlushResourceBarriers();

    RTL_ASSERT(CurrentAllocator != nullptr);

    uint64_t fenceValue = CommandListManager::Get().GetQueue(Type).ExecuteCommandList(TheCommandList);
    if (waitForCompletion)
    {
        CommandListManager::Get().WaitForFence(fenceValue);
    }

    // Reset the command list and restore previous state
    TheCommandList->Reset(CurrentAllocator, nullptr);

    if (CurGraphicsRootSignature)
    {
        TheCommandList->SetGraphicsRootSignature(CurGraphicsRootSignature);
        TheCommandList->SetPipelineState(CurGraphicsPipelineState);
    }
    if (CurComputeRootSignature)
    {
        TheCommandList->SetComputeRootSignature(CurComputeRootSignature);
        TheCommandList->SetPipelineState(CurComputePipelineState);
    }

    BindDescriptorHeaps();

    return fenceValue;
}

// ----------------------------------------------------------------------------------------------------------------------------

uint64_t CommandContext::Finish( bool waitForCompletion )
{
    RTL_ASSERT(Type == D3D12_COMMAND_LIST_TYPE_DIRECT || Type == D3D12_COMMAND_LIST_TYPE_COMPUTE);

    FlushResourceBarriers();

    RTL_ASSERT(CurrentAllocator != nullptr);

    CommandQueue& queue = CommandListManager::Get().GetQueue(Type);

    uint64_t fenceValue = queue.ExecuteCommandList(TheCommandList);
    queue.DiscardAllocator(fenceValue, CurrentAllocator);
    CurrentAllocator = nullptr;

    CpuLinearAllocator.CleanupUsedPages(fenceValue);
    GpuLinearAllocator.CleanupUsedPages(fenceValue);

    if (waitForCompletion)
    {
        CommandListManager::Get().WaitForFence(fenceValue);
    }

    sContextManager.FreeContext(this);

    return fenceValue;
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE type) :
    Type(type),
    CpuLinearAllocator(kCpuWritable), 
    GpuLinearAllocator(kGpuExclusive)
{
    OwningManager = nullptr;
    TheCommandList = nullptr;
    CurrentAllocator = nullptr;
    ZeroMemory(CurrentDescriptorHeaps, sizeof(CurrentDescriptorHeaps));

    CurGraphicsRootSignature = nullptr;
    CurGraphicsPipelineState = nullptr;
    CurComputeRootSignature = nullptr;
    CurComputePipelineState = nullptr;
    NumBarriersToFlush = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandContext::~CommandContext()
{
    if (TheCommandList != nullptr)
    {
        TheCommandList->Release();
        TheCommandList = nullptr;
    }

    // These are released by the respective systems
    CurGraphicsRootSignature = nullptr;
    CurComputeRootSignature  = nullptr;
    CurGraphicsRootSignature = nullptr;
    CurComputePipelineState  = nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::Initialize()
{
    CommandListManager::Get().CreateNewCommandList(Type, &TheCommandList, &CurrentAllocator);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::Reset()
{
    // We only call Reset() on previously freed contexts.  The command list persists, but we must
    // request a new allocator.
    RTL_ASSERT(TheCommandList != nullptr && CurrentAllocator == nullptr);
    CurrentAllocator = CommandListManager::Get().GetQueue(Type).RequestAllocator();
    TheCommandList->Reset(CurrentAllocator, nullptr);

    CurGraphicsRootSignature = nullptr;
    CurGraphicsPipelineState = nullptr;
    CurComputeRootSignature  = nullptr;
    CurComputePipelineState  = nullptr;
    NumBarriersToFlush       = 0;

    ZeroMemory(CurrentDescriptorHeaps, sizeof(CurrentDescriptorHeaps));
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::BindDescriptorHeaps()
{
    uint32_t nonNullHeaps = 0;
    ID3D12DescriptorHeap* heapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        ID3D12DescriptorHeap* heapIter = CurrentDescriptorHeaps[i];
        if (heapIter != nullptr)
        {
            heapsToBind[nonNullHeaps++] = heapIter;
        }
    }

    if (nonNullHeaps > 0)
    {
        TheCommandList->SetDescriptorHeaps(nonNullHeaps, heapsToBind);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetRenderTargets( uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE rtvArray[], D3D12_CPU_DESCRIPTOR_HANDLE dsv )
{
    TheCommandList->OMSetRenderTargets( numRTVs, rtvArray, FALSE, &dsv );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE rtvArray[])
{
    TheCommandList->OMSetRenderTargets(numRTVs, rtvArray, FALSE, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::BeginQuery(ID3D12QueryHeap* queryHeap, D3D12_QUERY_TYPE Type, uint32_t heapIndex)
{
    TheCommandList->BeginQuery(queryHeap, Type, heapIndex);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::EndQuery(ID3D12QueryHeap* queryHeap, D3D12_QUERY_TYPE Type, uint32_t heapIndex)
{
    TheCommandList->EndQuery(queryHeap, Type, heapIndex);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ResolveQueryData(ID3D12QueryHeap* queryHeap, D3D12_QUERY_TYPE type, uint32_t startIndex, uint32_t numQueries, ID3D12Resource* destinationBuffer, UINT64 destinationBufferOffset)
{
    TheCommandList->ResolveQueryData(queryHeap, type, startIndex, numQueries, destinationBuffer, destinationBufferOffset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetRootSignature(const RootSignature& rootSig)
{
    if (rootSig.GetSignature() == CurComputeRootSignature)
    {
        return;
    }

    TheCommandList->SetComputeRootSignature(CurComputeRootSignature = rootSig.GetSignature());
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetPipelineState(const ComputePSO& pso)
{
    ID3D12PipelineState* pipelineState = pso.GetPipelineStateObject();
    if (pipelineState == CurComputePipelineState)
    {
        return;
    }

    TheCommandList->SetPipelineState(pipelineState);
    CurComputePipelineState = pipelineState;
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetPipelineState(RaytracingPSO& pso)
{
    GetCommandListDXR()->SetPipelineState1(pso.GetRaytracingPipelineStateObject());
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstantArray(uint32_t rootEntry, uint32_t numConstants, const void* pConstants)
{
    TheCommandList->SetComputeRoot32BitConstants(rootEntry, numConstants, pConstants, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstant(uint32_t rootEntry, DWParam val, uint32_t offset)
{
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, val.Uint, offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstants(uint32_t rootEntry, DWParam x)
{
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, x.Uint, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstants(uint32_t rootEntry, DWParam x, DWParam y)
{
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, x.Uint, 0);
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, y.Uint, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstants(uint32_t rootEntry, DWParam x, DWParam y, DWParam z)
{
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, x.Uint, 0);
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, y.Uint, 1);
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, z.Uint, 2);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstants(uint32_t rootEntry, DWParam x, DWParam y, DWParam z, DWParam w)
{
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, x.Uint, 0);
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, y.Uint, 1);
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, z.Uint, 2);
    TheCommandList->SetComputeRoot32BitConstant(rootEntry, w.Uint, 3);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv)
{
    TheCommandList->SetComputeRootConstantBufferView(rootIndex, cbv);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetDynamicConstantBufferView(uint32_t rootIndex, size_t bufferSize, const void* bufferData)
{
    RTL_ASSERT(bufferData != nullptr && IsAligned(bufferData, 16));
    DynAlloc cb = CpuLinearAllocator.Allocate(bufferSize);
    memcpy(cb.DataPtr, bufferData, bufferSize);
    TheCommandList->SetComputeRootConstantBufferView(rootIndex, cb.GpuAddress);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* bufferData)
{
    RTL_ASSERT(bufferData != nullptr && IsAligned(bufferData, 16));
    DynAlloc cb = CpuLinearAllocator.Allocate(bufferSize);
    memcpy(cb.DataPtr, bufferData, bufferSize);
    TheCommandList->SetComputeRootShaderResourceView(rootIndex, cb.GpuAddress);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetBufferSRV(uint32_t rootIndex, const GpuBuffer& srv, UINT64 offset)
{
    RTL_ASSERT((srv.UsageState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0);
    TheCommandList->SetComputeRootShaderResourceView(rootIndex, srv.GetGpuVirtualAddress() + offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetBufferUAV(uint32_t rootIndex, const GpuBuffer& uav, UINT64 offset)
{
    RTL_ASSERT((uav.UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
    TheCommandList->SetComputeRootUnorderedAccessView(rootIndex, uav.GetGpuVirtualAddress() + offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetBufferSRV(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddr, UINT64 offset)
{
    TheCommandList->SetComputeRootShaderResourceView(rootIndex, gpuAddr + offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetBufferUAV(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddr, UINT64 offset)
{
    TheCommandList->SetComputeRootUnorderedAccessView(rootIndex, gpuAddr + offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::Dispatch(size_t groupCountX, size_t groupCountY, size_t groupCountZ)
{
    FlushResourceBarriers();
    TheCommandList->Dispatch((uint32_t)groupCountX, (uint32_t)groupCountY, (uint32_t)groupCountZ);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::Dispatch1D(size_t threadCountX, size_t groupSizeX)
{
    Dispatch(DivideByMultiple(threadCountX, groupSizeX), 1, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::Dispatch2D(size_t threadCountX, size_t threadCountY, size_t groupSizeX, size_t GroupSizeY)
{
    Dispatch(
        DivideByMultiple(threadCountX, groupSizeX),
        DivideByMultiple(threadCountY, GroupSizeY), 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::Dispatch3D(size_t threadCountX, size_t threadCountY, size_t threadCountZ, size_t groupSizeX, size_t groupSizeY, size_t groupSizeZ)
{
    Dispatch(
        DivideByMultiple(threadCountX, groupSizeX),
        DivideByMultiple(threadCountY, groupSizeY),
        DivideByMultiple(threadCountZ, groupSizeZ));
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::DispatchRays(RaytracingPSO& pso, uint32_t width, uint32_t height)
{
    FlushResourceBarriers();

    D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
    raytraceDesc.Width                                  = width;
    raytraceDesc.Height                                 = height;
    raytraceDesc.Depth                                  = 1;
    raytraceDesc.RayGenerationShaderRecord.StartAddress = pso.GetRaygenShaderTable().ShaderTableBuffer.GetGpuVirtualAddress();
    raytraceDesc.RayGenerationShaderRecord.SizeInBytes  = pso.GetRaygenShaderTable().ShaderTableBuffer.GetBufferSize();
    raytraceDesc.MissShaderTable.StartAddress           = pso.GetMissShaderTable().ShaderTableBuffer.GetGpuVirtualAddress();
    raytraceDesc.MissShaderTable.StrideInBytes          = pso.GetMissShaderTable().ShaderEntryStride;
    raytraceDesc.MissShaderTable.SizeInBytes            = pso.GetMissShaderTable().ShaderTableBuffer.GetBufferSize();
    raytraceDesc.HitGroupTable.StartAddress             = pso.GetHitGroupShaderTable().ShaderTableBuffer.GetGpuVirtualAddress();
    raytraceDesc.HitGroupTable.StrideInBytes            = pso.GetHitGroupShaderTable().ShaderEntryStride;
    raytraceDesc.HitGroupTable.SizeInBytes              = pso.GetHitGroupShaderTable().ShaderTableBuffer.GetBufferSize();

    // Dispatch
    GetCommandListDXR()->DispatchRays(&raytraceDesc);
}

// ----------------------------------------------------------------------------------------------------------------------------

void ComputeContext::SetDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
{
    TheCommandList->SetComputeRootDescriptorTable(rootIndex, firstHandle);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeEngine::ComputeContext::SetDescriptorTable(const PingPongDescriptor& pingPongDescriptor)
{
    TheCommandList->SetComputeRootDescriptorTable(pingPongDescriptor.GetCurrentRootIndex(), pingPongDescriptor.GetCurrentGpuDescriptor());
    TheCommandList->SetComputeRootDescriptorTable(pingPongDescriptor.GetPreviousRootIndex(), pingPongDescriptor.GePreviousGpuDescriptor());
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearColor(ColorTarget& target)
{
    TheCommandList->ClearRenderTargetView(target.GetRTV(), target.GetClearColor(), 0, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearDepth(DepthTarget& target)
{
    TheCommandList->ClearDepthStencilView(target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, target.GetClearDepth(), target.GetClearStencil(), 0, nullptr );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearStencil(DepthTarget& target)
{
    TheCommandList->ClearDepthStencilView(target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL, target.GetClearDepth(), target.GetClearStencil(), 0, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::ClearDepthAndStencil(DepthTarget& target)
{
    TheCommandList->ClearDepthStencilView(target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, target.GetClearDepth(), target.GetClearStencil(), 0, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
    RTL_ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    TheCommandList->RSSetViewports( 1, &vp );
    TheCommandList->RSSetScissorRects( 1, &rect );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    SetViewport((float)x, (float)y, (float)w, (float)h);
    SetScissor(x, y, x + w, y + h);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetViewport(const D3D12_VIEWPORT& vp)
{
    TheCommandList->RSSetViewports( 1, &vp );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth, FLOAT maxDepth)
{
    D3D12_VIEWPORT vp;
    vp.Width    = w;
    vp.Height   = h;
    vp.MinDepth = minDepth;
    vp.MaxDepth = maxDepth;
    vp.TopLeftX = x;
    vp.TopLeftY = y;
    TheCommandList->RSSetViewports( 1, &vp );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetScissor(const D3D12_RECT& rect)
{
    RTL_ASSERT(rect.left < rect.right && rect.top < rect.bottom);
    TheCommandList->RSSetScissorRects( 1, &rect );
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    SetScissor(CD3DX12_RECT(left, top, right, bottom));
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetRootSignature(const RootSignature& rootSig)
{
    if (rootSig.GetSignature() == CurGraphicsRootSignature)
    {
        return;
    }

    TheCommandList->SetGraphicsRootSignature(CurGraphicsRootSignature = rootSig.GetSignature());
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetPipelineState(const GraphicsPSO& pso)
{
    ID3D12PipelineState* pipelineState = pso.GetPipelineStateObject();
    if (pipelineState == CurGraphicsPipelineState)
    {
        return;
    }

    TheCommandList->SetPipelineState(pipelineState);
    CurGraphicsPipelineState = pipelineState;
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetStencilRef(uint32_t ref)
{
    TheCommandList->OMSetStencilRef(ref);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetBlendFactor(float blendFactor[4])
{
    TheCommandList->OMSetBlendFactor(blendFactor);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
    TheCommandList->IASetPrimitiveTopology(topology);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* pConstants)
{
    TheCommandList->SetGraphicsRoot32BitConstants(rootIndex, numConstants, pConstants, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstant(uint32_t rootEntry, DWParam val, uint32_t offset)
{
    TheCommandList->SetGraphicsRoot32BitConstant(rootEntry, val.Uint, offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x)
{
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, x.Uint, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y)
{
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, x.Uint, 0);
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, y.Uint, 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z)
{
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, x.Uint, 0);
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, y.Uint, 1);
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, z.Uint, 2);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w)
{
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, x.Uint, 0);
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, y.Uint, 1);
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, z.Uint, 2);
    TheCommandList->SetGraphicsRoot32BitConstant(rootIndex, w.Uint, 3);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv)
{
    TheCommandList->SetGraphicsRootConstantBufferView(rootIndex, cbv);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicConstantBufferView(uint32_t rootIndex, size_t bufferSize, const void* bufferData)
{
    RTL_ASSERT(bufferData != nullptr && IsAligned(bufferData, 16));
    DynAlloc cb = CpuLinearAllocator.Allocate(bufferSize);
    memcpy(cb.DataPtr, bufferData, bufferSize);
    TheCommandList->SetGraphicsRootConstantBufferView(rootIndex, cb.GpuAddress);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicVB(uint32_t slot, size_t numVertices, size_t vertexStride, const void* vertexData)
{
    RTL_ASSERT(vertexData != nullptr && IsAligned(vertexData, 16));

    size_t   bufferSize = AlignUp(numVertices * vertexStride, 16);
    DynAlloc vb         = CpuLinearAllocator.Allocate(bufferSize);

    memcpy(vb.DataPtr, vertexData, bufferSize);

    D3D12_VERTEX_BUFFER_VIEW vbView;
    vbView.BufferLocation = vb.GpuAddress;
    vbView.SizeInBytes    = (uint32_t)bufferSize;
    vbView.StrideInBytes  = (uint32_t)vertexStride;

    TheCommandList->IASetVertexBuffers(slot, 1, &vbView);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicIB(size_t indexCount, const uint16_t* indexData)
{
    RTL_ASSERT(indexData != nullptr && IsAligned(indexData, 16));

    size_t   bufferSize = AlignUp(indexCount * sizeof(uint16_t), 16);
    DynAlloc ib         = CpuLinearAllocator.Allocate(bufferSize);

    memcpy(ib.DataPtr, indexData, bufferSize);

    D3D12_INDEX_BUFFER_VIEW ibView;
    ibView.BufferLocation   = ib.GpuAddress;
    ibView.SizeInBytes      = (uint32_t)(indexCount * sizeof(uint16_t));
    ibView.Format           = DXGI_FORMAT_R16_UINT;

    TheCommandList->IASetIndexBuffer(&ibView);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* bufferData)
{
    RTL_ASSERT(bufferData != nullptr && IsAligned(bufferData, 16));
    DynAlloc cb = CpuLinearAllocator.Allocate(bufferSize);
    memcpy(cb.DataPtr, bufferData, bufferSize);
    TheCommandList->SetGraphicsRootShaderResourceView(rootIndex, cb.GpuAddress);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetBufferSRV(uint32_t rootIndex, const GpuBuffer& srv, UINT64 offset)
{
    RTL_ASSERT((srv.UsageState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
    TheCommandList->SetGraphicsRootShaderResourceView(rootIndex, srv.GetGpuVirtualAddress() + offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetBufferUAV(uint32_t rootIndex, const GpuBuffer& uav, UINT64 offset)
{
    RTL_ASSERT((uav.UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
    TheCommandList->SetGraphicsRootUnorderedAccessView(rootIndex, uav.GetGpuVirtualAddress() + offset);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
{
    TheCommandList->SetGraphicsRootDescriptorTable(rootIndex, firstHandle);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDescriptorTable(const PingPongDescriptor& pingPongDescriptor)
{
    TheCommandList->SetGraphicsRootDescriptorTable(pingPongDescriptor.GetCurrentRootIndex(), pingPongDescriptor.GetCurrentGpuDescriptor());
    TheCommandList->SetGraphicsRootDescriptorTable(pingPongDescriptor.GetPreviousRootIndex(), pingPongDescriptor.GePreviousGpuDescriptor());
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& ibView)
{
    TheCommandList->IASetIndexBuffer(&ibView);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetVertexBuffer(uint32_t slot, const D3D12_VERTEX_BUFFER_VIEW& vbView)
{
    SetVertexBuffers(slot, 1, &vbView);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetVertexBuffers(uint32_t StartSlot, uint32_t count, const D3D12_VERTEX_BUFFER_VIEW vbViews[])
{
    TheCommandList->IASetVertexBuffers(StartSlot, count, vbViews);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetNullIndexBuffer()
{
    TheCommandList->IASetIndexBuffer(NULL);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetNullVertexBuffer(uint32_t slot)
{
    TheCommandList->IASetVertexBuffers(slot, 0, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::Draw(uint32_t vertexCount, uint32_t vertexStartOffset)
{
    DrawInstanced(vertexCount, 1, vertexStartOffset, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::DrawIndexed(uint32_t IndexCount, uint32_t StartIndexLocation, INT BaseVertexLocation)
{
    DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    FlushResourceBarriers();
    TheCommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, INT baseVertexLocation, uint32_t startInstanceLocation)
{
    FlushResourceBarriers();
    TheCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv)
{
    SetRenderTargets(1, &rtv);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE dsv)
{
    SetRenderTargets(0, nullptr, dsv);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GraphicsContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv)
{
    SetRenderTargets(1, &rtv, dsv);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
    D3D12_RESOURCE_STATES oldState = resource.UsageState;

    if (Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        RTL_ASSERT((oldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == oldState);
        RTL_ASSERT((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState);
    }

    if (oldState != newState)
    {
        RTL_ASSERT_MSG(NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& barrierDesc = ResourceBarrierBuffer[NumBarriersToFlush++];

        barrierDesc.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDesc.Transition.pResource    = resource.GetResource();
        barrierDesc.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrierDesc.Transition.StateBefore  = oldState;
        barrierDesc.Transition.StateAfter   = newState;

        // Check to see if we already started the transition
        if (newState == resource.TransitioningState)
        {
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
            resource.TransitioningState = (D3D12_RESOURCE_STATES)-1;
        }
        else
        {
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        }

        resource.UsageState = newState;
    }
    else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        InsertUAVBarrier(resource, flushImmediate);
    }

    if (flushImmediate || NumBarriersToFlush == 16)
    {
        FlushResourceBarriers();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::BeginResourceTransition(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
    // If it's already transitioning, finish that transition
    if (resource.TransitioningState != (D3D12_RESOURCE_STATES)-1)
    {
        TransitionResource(resource, resource.TransitioningState);
    }

    D3D12_RESOURCE_STATES oldState = resource.UsageState;
    if (oldState != newState)
    {
        RTL_ASSERT_MSG(NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& barrierDesc = ResourceBarrierBuffer[NumBarriersToFlush++];

        barrierDesc.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDesc.Transition.pResource    = resource.GetResource();
        barrierDesc.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrierDesc.Transition.StateBefore  = oldState;
        barrierDesc.Transition.StateAfter   = newState;
        barrierDesc.Flags                   = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

        resource.TransitioningState = newState;
    }

    if (flushImmediate || NumBarriersToFlush == 16)
    {
        FlushResourceBarriers();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InsertUAVBarrier(GpuResource& resource, bool flushImmediate)
{
    RTL_ASSERT_MSG(NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& barrierDesc = ResourceBarrierBuffer[NumBarriersToFlush++];

    barrierDesc.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrierDesc.Flags         = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.UAV.pResource = resource.GetResource();

    if (flushImmediate)
    {
        FlushResourceBarriers();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InsertAliasBarrier(GpuResource& before, GpuResource& after, bool flushImmediate)
{
    RTL_ASSERT_MSG(NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
    D3D12_RESOURCE_BARRIER& barrierDesc = ResourceBarrierBuffer[NumBarriersToFlush++];

    barrierDesc.Type                        = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    barrierDesc.Flags                       = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierDesc.Aliasing.pResourceBefore    = before.GetResource();
    barrierDesc.Aliasing.pResourceAfter     = after.GetResource();

    if (flushImmediate)
    {
        FlushResourceBarriers();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::WriteBuffer(GpuResource& dest, size_t destOffset, const void* bufferData, size_t numBytes)
{
    RTL_ASSERT(bufferData != nullptr && IsAligned(bufferData, 16));
    DynAlloc tempSpace = CpuLinearAllocator.Allocate( numBytes, 512 );
    memcpy(tempSpace.DataPtr, bufferData, numBytes);
    CopyBufferRegion(dest, destOffset, tempSpace.Buffer, tempSpace.Offset, numBytes );
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::FillBuffer( GpuResource& dest, size_t destOffset, DWParam value, size_t numBytes )
{
    DynAlloc tempSpace   = CpuLinearAllocator.Allocate( numBytes, 512 );

    memcpy(tempSpace.DataPtr, &value.Float, numBytes);
    CopyBufferRegion(dest, destOffset, tempSpace.Buffer, tempSpace.Offset, numBytes );
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InitializeTexture( GpuResource& dest, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA subData[] )
{
    UINT64 uploadBufferSize = GetRequiredIntermediateSize(dest.GetResource(), 0, numSubresources);

    CommandContext& initContext = CommandContext::Begin();

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    DynAlloc mem = initContext.ReserveUploadMemory(uploadBufferSize);
    initContext.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
    UpdateSubresources(initContext.TheCommandList, dest.GetResource(), mem.Buffer.GetResource(), 0, 0, numSubresources, subData);
    initContext.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    initContext.Finish(true);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::CopySubresource(GpuResource& dest, uint32_t destSubIndex, GpuResource& src, uint32_t srcSubIndex)
{
    FlushResourceBarriers();

    D3D12_TEXTURE_COPY_LOCATION DestLocation =
    {
        dest.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        destSubIndex
    };

    D3D12_TEXTURE_COPY_LOCATION SrcLocation =
    {
        src.GetResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        srcSubIndex
    };

    TheCommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InitializeTextureArraySlice(GpuResource& dest, uint32_t sliceIndex, GpuResource& src)
{
    CommandContext& context = CommandContext::Begin();

    context.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
    context.FlushResourceBarriers();

    const D3D12_RESOURCE_DESC& destDesc = dest.GetResource()->GetDesc();
    const D3D12_RESOURCE_DESC& srcDesc = src.GetResource()->GetDesc();

    RTL_ASSERT(sliceIndex < destDesc.DepthOrArraySize &&
        srcDesc.DepthOrArraySize == 1 &&
        destDesc.Width == srcDesc.Width &&
        destDesc.Height == srcDesc.Height &&
        destDesc.MipLevels <= srcDesc.MipLevels
        );

    uint32_t subResourceIndex = sliceIndex * destDesc.MipLevels;

    for (uint32_t i = 0; i < destDesc.MipLevels; ++i)
    {
        D3D12_TEXTURE_COPY_LOCATION destCopyLocation =
        {
            dest.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            subResourceIndex + i
        };

        D3D12_TEXTURE_COPY_LOCATION srcCopyLocation =
        {
            src.GetResource(),
            D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            i
        };

        context.TheCommandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
    }

    context.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ);
    context.Finish(true);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InitializeBuffer( GpuResource& dest, const void* bufferData, size_t numBytes, size_t offset)
{
    CommandContext& initContext = CommandContext::Begin();

    DynAlloc mem = initContext.ReserveUploadMemory(numBytes);
    memcpy(mem.DataPtr, bufferData, numBytes);

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    initContext.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
    initContext.TheCommandList->CopyBufferRegion(dest.GetResource(), offset, mem.Buffer.GetResource(), 0, numBytes);
    initContext.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    initContext.Finish(true);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::FlushResourceBarriers()
{
    if (NumBarriersToFlush > 0)
    {
        TheCommandList->ResourceBarrier(NumBarriersToFlush, ResourceBarrierBuffer);
        NumBarriersToFlush = 0;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heapPtr)
{
    if (CurrentDescriptorHeaps[type] != heapPtr)
    {
        CurrentDescriptorHeaps[type] = heapPtr;
        BindDescriptorHeaps();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::SetDescriptorHeaps(uint32_t heapCount, D3D12_DESCRIPTOR_HEAP_TYPE type[], ID3D12DescriptorHeap* heapPtrs[])
{
    bool AnyChanged = false;

    for (uint32_t i = 0; i < heapCount; ++i)
    {
        if (CurrentDescriptorHeaps[type[i]] != heapPtrs[i])
        {
            CurrentDescriptorHeaps[type[i]] = heapPtrs[i];
            AnyChanged = true;
        }
    }

    if (AnyChanged)
    {
        BindDescriptorHeaps();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::SetPredication(ID3D12Resource* buffer, UINT64 bufferOffset, D3D12_PREDICATION_OP op)
{
    TheCommandList->SetPredication(buffer, bufferOffset, op);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::CopyBuffer(GpuResource& dest, GpuResource& src)
{
    TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    TheCommandList->CopyResource(dest.GetResource(), src.GetResource());
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::CopyBufferRegion(GpuResource& dest, size_t destOffset, GpuResource& src, size_t srcOffset, size_t numBytes)
{
    TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
    //TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    TheCommandList->CopyBufferRegion(dest.GetResource(), destOffset, src.GetResource(), srcOffset, numBytes);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t queryIdx)
{
    TheCommandList->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, queryIdx);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandContext::ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t numQueries)
{
    TheCommandList->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, numQueries, pReadbackHeap, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeEngine::CommandContext::ReadbackTexture2D(GpuResource& readbackBuffer, ColorTarget& srcBuffer)
{
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;
    RenderDevice::Get().GetD3DDevice()->GetCopyableFootprints(&srcBuffer.GetResource()->GetDesc(), 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

    // This very short command list only issues one API call and will be synchronized so we can immediately read
    // the buffer contents.
    CommandContext& context = CommandContext::Begin("Copy texture to memory");

    context.TransitionResource(srcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

    context.TheCommandList->CopyTextureRegion(
        &CD3DX12_TEXTURE_COPY_LOCATION(readbackBuffer.GetResource(), PlacedFootprint), 0, 0, 0,
        &CD3DX12_TEXTURE_COPY_LOCATION(srcBuffer.GetResource(), 0), nullptr);

    context.Finish(true);
}
