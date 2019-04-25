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
#include "Texture.h"
#include "Color.h"
//#include "PixelBuffer.h"
#include "DynamicDescriptorHeap.h"
#include "LinearAllocator.h"
//#include "CommandSignature.h"
//#include "GraphicsCore.h"

#include <vector>
#include <mutex>

// ----------------------------------------------------------------------------------------------------------------------------

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{
    // ----------------------------------------------------------------------------------------------------------------------------

    class ColorBuffer;
    class DepthBuffer;
    class Texture;
    class GraphicsContext;
    class ComputeContext;

    // ----------------------------------------------------------------------------------------------------------------------------

    struct DWParam
    {
        DWParam(FLOAT f) : Float(f) {}
        DWParam(UINT u) : Uint(u) {}
        DWParam(INT i) : Int(i) {}

        void operator= (FLOAT f) { Float = f; }
        void operator= (UINT u) { Uint = u; }
        void operator= (INT i) { Int = i; }

        union
        {
            FLOAT Float;
            UINT  Uint;
            INT   Int;
        };
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ContextManager
    {
    public:

        ContextManager() {}

    public:

        ContextManager&                               Get();
        CommandContext*                               AllocateContext(D3D12_COMMAND_LIST_TYPE Type);
        void                                          FreeContext(CommandContext*);
        void                                          DestroyAllContexts();

    private:

        std::vector<std::unique_ptr<CommandContext>>  sm_ContextPool[4];
        std::queue<CommandContext*>                   sm_AvailableContexts[4];
        std::mutex                                    sm_ContextAllocationMutex;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class CommandContext : NonCopyable
    {
    public:

        ~CommandContext(void);

        static void                 DestroyAllContexts();
        static CommandContext&      Begin(const std::wstring ID = L"");
        void                        Initialize();
        uint64_t                    Flush(bool WaitForCompletion = false);
        uint64_t                    Finish(bool WaitForCompletion = false);

        void                        CopyBuffer(GpuResource& Dest, GpuResource& Src);
        void                        CopyBufferRegion(GpuResource& Dest, size_t DestOffset, GpuResource& Src, size_t SrcOffset, size_t NumBytes);
        void                        CopySubresource(GpuResource& Dest, UINT DestSubIndex, GpuResource& Src, UINT SrcSubIndex);
        void                        CopyCounter(GpuResource& Dest, size_t DestOffset, StructuredBuffer& Src);
        void                        ResetCounter(StructuredBuffer& Buf, uint32_t Value = 0);

        static void                 InitializeTexture(GpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
        static void                 InitializeBuffer(GpuResource& Dest, const void* Data, size_t NumBytes, size_t Offset = 0);
        static void                 InitializeTextureArraySlice(GpuResource& Dest, UINT SliceIndex, GpuResource& Src);
//        static void                 ReadbackTexture2D(GpuResource& ReadbackBuffer, PixelBuffer& SrcBuffer);

        void                        WriteBuffer(GpuResource& Dest, size_t DestOffset, const void* Data, size_t NumBytes);
        void                        FillBuffer(GpuResource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes);

        void                        TransitionResource(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
        void                        BeginResourceTransition(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);

        void                        InsertUAVBarrier(GpuResource& Resource, bool FlushImmediate = false);
        void                        InsertAliasBarrier(GpuResource& Before, GpuResource& After, bool FlushImmediate = false);
        void                        FlushResourceBarriers(void);

        void                        InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx);
        void                        ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries);

        void                        PIXBeginEvent(const wchar_t* label);
        void                        PIXEndEvent(void);
        void                        PIXSetMarker(const wchar_t* label);

        void                        SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr);
        void                        SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[]);
        void                        SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

        GraphicsContext&            GetGraphicsContext() { return reinterpret_cast<GraphicsContext&>(*this); }
        ComputeContext&             GetComputeContext() { return reinterpret_cast<ComputeContext&>(*this); }
        ID3D12GraphicsCommandList*  GetCommandList() { return m_CommandList; }
        DynAlloc                    ReserveUploadMemory(size_t SizeInBytes) { return m_CpuLinearAllocator.Allocate(SizeInBytes); }

    protected:

        void                        BindDescriptorHeaps(void);
        void                        SetID(const std::wstring& ID) { m_ID = ID; }

    protected:

        CommandListManager*         m_OwningManager;
        ID3D12GraphicsCommandList*  m_CommandList;
        ID3D12CommandAllocator*     m_CurrentAllocator;

        ID3D12RootSignature*        m_CurGraphicsRootSignature;
        ID3D12PipelineState*        m_CurGraphicsPipelineState;
        ID3D12RootSignature*        m_CurComputeRootSignature;
        ID3D12PipelineState*        m_CurComputePipelineState;

        DynamicDescriptorHeap       m_DynamicViewDescriptorHeap;        // HEAP_TYPE_CBV_SRV_UAV
        DynamicDescriptorHeap       m_DynamicSamplerDescriptorHeap;    // HEAP_TYPE_SAMPLER

        D3D12_RESOURCE_BARRIER      m_ResourceBarrierBuffer[16];
        UINT                        m_NumBarriersToFlush;

        ID3D12DescriptorHeap*       m_CurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

        LinearAllocator             m_CpuLinearAllocator;
        LinearAllocator             m_GpuLinearAllocator;

        std::wstring                m_ID;
        D3D12_COMMAND_LIST_TYPE     m_Type;

    private:

        void                        Reset();

        friend ContextManager;
        CommandContext(D3D12_COMMAND_LIST_TYPE Type);
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class GraphicsContext : public CommandContext
    {
    public:

        static GraphicsContext& Begin(const std::wstring& ID = L"")
        {
            return CommandContext::Begin(ID).GetGraphicsContext();
        }

        void ClearUAV(GpuBuffer& Target);
        void ClearUAV(ColorBuffer& Target);
        void ClearColor(ColorBuffer& Target);
        void ClearDepth(DepthBuffer& Target);
        void ClearStencil(DepthBuffer& Target);
        void ClearDepthAndStencil(DepthBuffer& Target);

        void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
        void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
        void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset);

        void SetRootSignature(const RootSignature& RootSig);

        void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
        void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
        void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV) { SetRenderTargets(1, &RTV); }
        void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(1, &RTV, DSV); }
        void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(0, nullptr, DSV); }

        void SetViewport(const D3D12_VIEWPORT& vp);
        void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
        void SetScissor(const D3D12_RECT& rect);
        void SetScissor(UINT left, UINT top, UINT right, UINT bottom);
        void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
        void SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h);
        void SetStencilRef(UINT StencilRef);
        void SetBlendFactor(Color BlendFactor);
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

        void SetPipelineState(const GraphicsPSO& PSO);
        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
        void SetConstant(UINT RootIndex, DWParam Val, UINT Offset = 0);
        void SetConstants(UINT RootIndex, DWParam X);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
        void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
        void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData);
        void SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset = 0);
        void SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset = 0);
        void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
        void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

        void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView);
        void SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView);
        void SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[]);
        void SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData);
        void SetDynamicIB(size_t IndexCount, const uint16_t* IBData);
        void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);

        void Draw(UINT VertexCount, UINT VertexStartOffset = 0);
        void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
        void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0);
        void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
        void DrawIndirect(GpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0);

#if 0
        void ExecuteIndirect(CommandSignature& CommandSig, GpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0, uint32_t MaxCommands = 1, GpuBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);
#endif
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class ComputeContext : public CommandContext
    {
    public:

        static ComputeContext& Begin(const std::wstring& ID = L"", bool Async = false);

        void ClearUAV(GpuBuffer& Target);
        void ClearUAV(ColorBuffer& Target);

        void SetRootSignature(const RootSignature& RootSig);

        void SetPipelineState(const ComputePSO& PSO);
        void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
        void SetConstant(UINT RootIndex, DWParam Val, UINT Offset = 0);
        void SetConstants(UINT RootIndex, DWParam X);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
        void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
        void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
        void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData);
        void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);
        void SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset = 0);
        void SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset = 0);
        void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

        void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
        void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
        void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

        void Dispatch(size_t GroupCountX = 1, size_t GroupCountY = 1, size_t GroupCountZ = 1);
        void Dispatch1D(size_t ThreadCountX, size_t GroupSizeX = 64);
        void Dispatch2D(size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX = 8, size_t GroupSizeY = 8);
        void Dispatch3D(size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ);
        void DispatchIndirect(GpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0);
#if 0
        void ExecuteIndirect(CommandSignature& CommandSig, GpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0, uint32_t MaxCommands = 1, GpuBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);
#endif

    private:
    };
}
