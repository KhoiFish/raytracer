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
#include "DescriptorHeap.h"
#include "RootSignature.h"
#include <vector>
#include <queue>

// ----------------------------------------------------------------------------------------------------------------------------

namespace yart
{
    class CommandContext;

    class DynamicDescriptorHeap
    {
    public:

        DynamicDescriptorHeap(CommandContext& owningContext, D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        ~DynamicDescriptorHeap();

        static void                     DestroyAll();
        void                            CleanupUsedHeaps(uint64_t fenceValue);

        void                            SetGraphicsDescriptorHandles(UINT rootIndex, UINT offset, UINT numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
        void                            SetComputeDescriptorHandles(UINT rootIndex, UINT offset, UINT numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

        void                            ParseGraphicsRootSignature(const RootSignature& rootSig);
        void                            ParseComputeRootSignature(const RootSignature& rootSig);

        void                            CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* cmdList);
        void                            CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList * cmdList);

        D3D12_GPU_DESCRIPTOR_HANDLE     UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handles);

    private:

        // ----------------------------------------------------------------------------------------------------------------------------

        struct DescriptorTableCache
        {
            DescriptorTableCache()
                : AssignedHandlesBitMap(0) {}

            uint32_t                        AssignedHandlesBitMap;
            D3D12_CPU_DESCRIPTOR_HANDLE*    TableStart;
            uint32_t                        TableSize;
        };

        // ----------------------------------------------------------------------------------------------------------------------------

        struct DescriptorHandleCache
        {
            DescriptorHandleCache()
            {
                ClearCache();
            }

            void ClearCache()
            {
                RootDescriptorTablesBitMap = 0;
                MaxCachedDescriptors = 0;
            }

            uint32_t                    ComputeStagedSize();
            void                        CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorSize, DescriptorHandle destHandleStart, ID3D12GraphicsCommandList* cmdList, void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* setFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));
            void                        StageDescriptorHandles(UINT rootIndex, UINT Offset, UINT numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
            void                        ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig);
            void                        UnbindAllValid();

            static const uint32_t       kMaxNumDescriptors = 256;
            static const uint32_t       kMaxNumDescriptorTables = 16;

            uint32_t                    RootDescriptorTablesBitMap;
            uint32_t                    StaleRootParamsBitMap;
            uint32_t                    MaxCachedDescriptors;
            DescriptorTableCache        RootDescriptorTable[kMaxNumDescriptorTables];
            D3D12_CPU_DESCRIPTOR_HANDLE HandleCache[kMaxNumDescriptors];
        };

        // ----------------------------------------------------------------------------------------------------------------------------

    private:

        static ID3D12DescriptorHeap*    RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        static void                     DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint64_t fenceValueForReset, const std::vector<ID3D12DescriptorHeap*>& usedHeaps);
        bool                            HasSpace(uint32_t count);
        void                            RetireCurrentHeap();
        void                            RetireUsedHeaps(uint64_t fenceValue);
        ID3D12DescriptorHeap*           GetHeapPointer();
        DescriptorHandle                Allocate(UINT count);
        void                            CopyAndBindStagedTables(DescriptorHandleCache& handleCache, ID3D12GraphicsCommandList* cmdList, void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* setFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));
        void                            UnbindAllValid();

    private:

        static const uint32_t                                               kNumDescriptorsPerHeap = 1024;
        static std::mutex                                                   Mutex;
        static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>>    DescriptorHeapPool[2];
        static std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>>       RetiredDescriptorHeaps[2];
        static std::queue<ID3D12DescriptorHeap*>                            AvailableDescriptorHeaps[2];

        CommandContext&                                                     OwningContext;
        ID3D12DescriptorHeap*                                               CurrentHeapPtr;
        const D3D12_DESCRIPTOR_HEAP_TYPE                                    DescriptorType;
        uint32_t                                                            DescriptorSize;
        uint32_t                                                            CurrentOffset;
        DescriptorHandle                                                    FirstDescriptor;
        std::vector<ID3D12DescriptorHeap*>                                  RetiredHeaps;
        DescriptorHandleCache                                               GraphicsHandleCache;
        DescriptorHandleCache                                               ComputeHandleCache;
    };
}