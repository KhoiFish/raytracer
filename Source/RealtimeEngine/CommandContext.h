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

#pragma once

#include "Globals.h"
#include "CommandList.h"
#include "PipelineStateObject.h"
#include "RootSignature.h"
#include "GpuBuffer.h"
#include "RTTexture.h"
#include "RenderTarget.h"
#include "LinearAllocator.h"

#include <vector>
#include <mutex>

// ----------------------------------------------------------------------------------------------------------------------------

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    // ----------------------------------------------------------------------------------------------------------------------------

    class ColorTarget;
    class DepthTarget;
    class Texture;
    class GraphicsContext;
    class ComputeContext;

    // ----------------------------------------------------------------------------------------------------------------------------

    struct DWParam
    {
        DWParam(FLOAT f) : Float(f) {}
        DWParam(uint32_t u) : Uint(u) {}
        DWParam(INT i) : Int(i) {}

        void operator= (FLOAT f)    { Float = f; }
        void operator= (uint32_t u) { Uint = u; }
        void operator= (INT i)      { Int = i; }

        union
        {
            FLOAT     Float;
            uint32_t  Uint;
            INT       Int;
        };
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ContextManager
    {
    public:

        ContextManager() {}

    public:

        CommandContext*                               AllocateContext(D3D12_COMMAND_LIST_TYPE type);
        void                                          FreeContext(CommandContext*);
        void                                          DestroyAllContexts();

    private:

        std::vector<std::unique_ptr<CommandContext>>  ContextPool[4];
        std::queue<CommandContext*>                   AvailableContexts[4];
        std::mutex                                    ContextAllocationMutex;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class CommandContext : NonCopyable
    {
    public:

        ~CommandContext(void);

        static void                 DestroyAllContexts();
        static CommandContext&      Begin(const string_t ID = "");

        void                        Initialize();
        uint64_t                    Flush(bool waitForCompletion = false);
        uint64_t                    Finish(bool waitForCompletion = false);

        void                        CopyBuffer(GpuResource& dest, GpuResource& src);
        void                        CopyBufferRegion(GpuResource& dest, size_t destOffset, GpuResource& src, size_t srcOffset, size_t numBytes);
        void                        CopySubresource(GpuResource& dest, uint32_t destSubIndex, GpuResource& src, uint32_t srcSubIndex);

        static void                 InitializeTexture(GpuResource& dest, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA subData[]);
        static void                 InitializeBuffer(GpuResource& dest, const void* Data, size_t numBytes, size_t offset = 0);
        static void                 InitializeTextureArraySlice(GpuResource& dest, uint32_t sliceIndex, GpuResource& src);
        static void                 ReadbackTexture2D(GpuResource& readbackBuffer, ColorTarget& srcBuffer);

        void                        WriteBuffer(GpuResource& dest, size_t destOffset, const void* data, size_t numBytes);
        void                        FillBuffer(GpuResource& dest, size_t destOffset, DWParam value, size_t numBytes);

        void                        TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
        void                        BeginResourceTransition(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);

        void                        InsertUAVBarrier(GpuResource& resource, bool flushImmediate = false);
        void                        InsertAliasBarrier(GpuResource& before, GpuResource& after, bool flushImmediate = false);
        void                        FlushResourceBarriers();

        void                        InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t queryIdx);
        void                        ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t numQueries);

        void                        SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heapPtr);
        void                        SetDescriptorHeaps(uint32_t heapCount, D3D12_DESCRIPTOR_HEAP_TYPE type[], ID3D12DescriptorHeap* heapPtrs[]);
        void                        SetPredication(ID3D12Resource* buffer, UINT64 bufferOffset, D3D12_PREDICATION_OP op);

        GraphicsContext&            GetGraphicsContext()                    { return reinterpret_cast<GraphicsContext&>(*this); }
        ComputeContext&             GetComputeContext()                     { return reinterpret_cast<ComputeContext&>(*this); }
        ID3D12GraphicsCommandList4* GetCommandList()                        { return TheCommandList; }
        DynAlloc                    ReserveUploadMemory(size_t SizeInBytes) { return CpuLinearAllocator.Allocate(SizeInBytes); }

    protected:

        void                        BindDescriptorHeaps();
        void                        SetID(const string_t& id) { ID = id; }

    protected:

        CommandListManager*         OwningManager;
        ID3D12GraphicsCommandList4* TheCommandList;
        ID3D12CommandAllocator*     CurrentAllocator;

        ID3D12RootSignature*        CurGraphicsRootSignature;
        ID3D12PipelineState*        CurGraphicsPipelineState;
        ID3D12RootSignature*        CurComputeRootSignature;
        ID3D12PipelineState*        CurComputePipelineState;

        D3D12_RESOURCE_BARRIER      ResourceBarrierBuffer[16];
        uint32_t                    NumBarriersToFlush;

        ID3D12DescriptorHeap*       CurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        LinearAllocator             CpuLinearAllocator;
        LinearAllocator             GpuLinearAllocator;

        string_t                    ID;
        D3D12_COMMAND_LIST_TYPE     Type;

    private:

        friend ContextManager;
        CommandContext(D3D12_COMMAND_LIST_TYPE type);

        void                        Reset();
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class GraphicsContext : public CommandContext
    {
    public:

        static GraphicsContext& Begin(const string_t& id = "")
        {
            return CommandContext::Begin(id).GetGraphicsContext();
        }

        void ClearColor(ColorTarget& target);
        void ClearDepth(DepthTarget& target);
        void ClearStencil(DepthTarget& target);
        void ClearDepthAndStencil(DepthTarget& target);

        void BeginQuery(ID3D12QueryHeap* queryHeap, D3D12_QUERY_TYPE type, uint32_t heapIndex);
        void EndQuery(ID3D12QueryHeap* queryHeap, D3D12_QUERY_TYPE type, uint32_t heapIndex);
        void ResolveQueryData(ID3D12QueryHeap* queryHeap, D3D12_QUERY_TYPE type, uint32_t startIndex, uint32_t numQueries, ID3D12Resource* destinationBuffer, UINT64 destinationBufferOffset);

        void SetRootSignature(const RootSignature& rootSig);
        void SetRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE rtvArray[]);
        void SetRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE rtvArray[], D3D12_CPU_DESCRIPTOR_HANDLE dsv);
        void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv);
        void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv);
        void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE dsv);

        void SetViewport(const D3D12_VIEWPORT& vp);
        void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
        void SetScissor(const D3D12_RECT& rect);
        void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
        void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
        void SetViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
        void SetStencilRef(uint32_t stencilRef);
        void SetBlendFactor(float blendFactor[4]);
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);

        void SetPipelineState(const GraphicsPSO& pso);
        void SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* pConstants);
        void SetConstant(uint32_t rootIndex, DWParam val, uint32_t offset = 0);
        void SetConstants(uint32_t rootIndex, DWParam x);
        void SetConstants(uint32_t rootIndex, DWParam x, DWParam y);
        void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z);
        void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w);
        void SetConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv);
        void SetDynamicConstantBufferView(uint32_t rootIndex, size_t bufferSize, const void* bufferData);
        void SetBufferSRV(uint32_t rootIndex, const GpuBuffer& srv, UINT64 offset = 0);
        void SetBufferUAV(uint32_t rootIndex, const GpuBuffer& uav, UINT64 offset = 0);
        void SetDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle);

        void SetDynamicDescriptor(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
        void SetDynamicSampler(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);

        void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& ibView);
        void SetVertexBuffer(uint32_t Slot, const D3D12_VERTEX_BUFFER_VIEW& vbView);
        void SetNullIndexBuffer(); 
        void SetNullVertexBuffer(uint32_t slot);
        void SetVertexBuffers(uint32_t startSlot, uint32_t count, const D3D12_VERTEX_BUFFER_VIEW vbViews[]);
        void SetDynamicVB(uint32_t slot, size_t numVertices, size_t vertexStride, const void* vbData);
        void SetDynamicIB(size_t indexCount, const uint16_t* ibData);
        void SetDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* bufferData);

        void Draw(uint32_t vertexCount, uint32_t vertexStartOffset = 0);
        void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, INT baseVertexLocation = 0);
        void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0);
        void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, INT baseVertexLocation, uint32_t startInstanceLocation);
        void DrawIndirect(GpuBuffer& argumentBuffer, uint64_t argumentBufferOffset = 0);
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ComputeContext : public CommandContext
    {
    public:

        static ComputeContext& Begin(const string_t& id = "", bool async = false);

        void ClearUAV(GpuBuffer& target);
        void ClearUAV(ColorTarget& target);
        void SetRootSignature(const RootSignature& rootSig);
        void SetPipelineState(const ComputePSO& pso);
        void SetPipelineState(RaytracingPSO& pso);
        void SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* pConstants);
        void SetConstant(uint32_t rootIndex, DWParam val, uint32_t offset = 0);
        void SetConstants(uint32_t rootIndex, DWParam x);
        void SetConstants(uint32_t rootIndex, DWParam x, DWParam y);
        void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z);
        void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w);
        void SetConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv);
        void SetDynamicConstantBufferView(uint32_t rootIndex, size_t bufferSize, const void* bufferData);
        void SetDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* bufferData);
        void SetBufferSRV(uint32_t rootIndex, const GpuBuffer& SRV, UINT64 offset = 0);
        void SetBufferUAV(uint32_t rootIndex, const GpuBuffer& UAV, UINT64 offset = 0);
        void SetBufferSRV(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddr, UINT64 offset = 0);
        void SetBufferUAV(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddr, UINT64 offset = 0);
        void SetDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle);

        void SetDynamicDescriptor(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
        void SetDynamicSampler(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);

        void Dispatch(size_t groupCountX = 1, size_t groupCountY = 1, size_t groupCountZ = 1);
        void Dispatch1D(size_t threadCountX, size_t groupSizeX = 64);
        void Dispatch2D(size_t threadCountX, size_t threadCountY, size_t groupSizeX = 8, size_t groupSizeY = 8);
        void Dispatch3D(size_t threadCountX, size_t threadCountY, size_t threadCountZ, size_t groupSizeX, size_t groupSizeY, size_t groupSizeZ);
        void DispatchIndirect(GpuBuffer& argumentBuffer, uint64_t argumentBufferOffset = 0);
        void DispatchRays(RaytracingPSO& pso, uint32_t width, uint32_t height);
    };
}
