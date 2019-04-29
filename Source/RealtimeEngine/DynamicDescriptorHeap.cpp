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

#include "DynamicDescriptorHeap.h"
#include "RenderDevice.h"
#include "CommandContext.h"
#include "CommandList.h"
#include "RootSignature.h"

using namespace  yart;

// ----------------------------------------------------------------------------------------------------------------------------

std::mutex                                                  DynamicDescriptorHeap::Mutex;
std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>>   DynamicDescriptorHeap::DescriptorHeapPool[2];
std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>>      DynamicDescriptorHeap::RetiredDescriptorHeaps[2];
std::queue<ID3D12DescriptorHeap*>                           DynamicDescriptorHeap::AvailableDescriptorHeaps[2];

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::DestroyAll()
{
    DescriptorHeapPool[0].clear();
    DescriptorHeapPool[1].clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::SetGraphicsDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
{
    GraphicsHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, handles);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::SetComputeDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
{
    ComputeHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, handles);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::ParseGraphicsRootSignature(const RootSignature& rootSig)
{
    GraphicsHandleCache.ParseRootSignature(DescriptorType, rootSig);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::ParseComputeRootSignature(const RootSignature& rootSig)
{
    ComputeHandleCache.ParseRootSignature(DescriptorType, rootSig);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* cmdList)
{
    if (GraphicsHandleCache.StaleRootParamsBitMap != 0)
        CopyAndBindStagedTables(GraphicsHandleCache, cmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* cmdList)
{
    if (ComputeHandleCache.StaleRootParamsBitMap != 0)
        CopyAndBindStagedTables(ComputeHandleCache, cmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

// ----------------------------------------------------------------------------------------------------------------------------

bool DynamicDescriptorHeap::HasSpace(uint32_t count)
{
    return (CurrentHeapPtr != nullptr && CurrentOffset + count <= kNumDescriptorsPerHeap);
}

// ----------------------------------------------------------------------------------------------------------------------------

DescriptorHandle DynamicDescriptorHeap::Allocate(uint32_t count)
{
    DescriptorHandle ret = FirstDescriptor + CurrentOffset * DescriptorSize;
    CurrentOffset += count;
    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
    std::lock_guard<std::mutex> lockGuard(Mutex);

    uint32_t idx = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

    while (!RetiredDescriptorHeaps[idx].empty() && CommandListManager::Get().IsFenceComplete(RetiredDescriptorHeaps[idx].front().first))
    {
        AvailableDescriptorHeaps[idx].push(RetiredDescriptorHeaps[idx].front().second);
        RetiredDescriptorHeaps[idx].pop();
    }

    if (!AvailableDescriptorHeaps[idx].empty())
    {
        ID3D12DescriptorHeap* heapPtr = AvailableDescriptorHeaps[idx].front();
        AvailableDescriptorHeaps[idx].pop();
        return heapPtr;
    }
    else
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type           = heapType;
        heapDesc.NumDescriptors = kNumDescriptorsPerHeap;
        heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        heapDesc.NodeMask       = 1;

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heapPtr;
        ASSERT_SUCCEEDED(RenderDevice::Get().GetD3DDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heapPtr)));
        DescriptorHeapPool[idx].emplace_back(heapPtr);

        return heapPtr.Get();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint64_t fenceValue, const std::vector<ID3D12DescriptorHeap*>& usedHeaps)
{
    std::lock_guard<std::mutex> lockGuard(Mutex);

    uint32_t idx = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;
    for (auto iter = usedHeaps.begin(); iter != usedHeaps.end(); ++iter)
    {
        RetiredDescriptorHeaps[idx].push(std::make_pair(fenceValue, *iter));
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::RetireCurrentHeap()
{
    // Don't retire unused heaps.
    if (CurrentOffset == 0)
    {
        ASSERT(CurrentHeapPtr == nullptr);
        return;
    }

    ASSERT(CurrentHeapPtr != nullptr);
    RetiredHeaps.push_back(CurrentHeapPtr);
    CurrentHeapPtr = nullptr;
    CurrentOffset = 0;
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::RetireUsedHeaps( uint64_t fenceValue )
{
    DiscardDescriptorHeaps(DescriptorType, fenceValue, RetiredHeaps);
    RetiredHeaps.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

DynamicDescriptorHeap::DynamicDescriptorHeap(CommandContext& OwningContext, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
    : OwningContext(OwningContext), DescriptorType(heapType)
{
    CurrentHeapPtr = nullptr;
    CurrentOffset  = 0;
    DescriptorSize = RenderDevice::Get().GetD3DDevice()->GetDescriptorHandleIncrementSize(heapType);
}

// ----------------------------------------------------------------------------------------------------------------------------

DynamicDescriptorHeap::~DynamicDescriptorHeap()
{
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::CleanupUsedHeaps(uint64_t fenceValue)
{
    RetireCurrentHeap();
    RetireUsedHeaps(fenceValue);
    GraphicsHandleCache.ClearCache();
    ComputeHandleCache.ClearCache();
}

// ----------------------------------------------------------------------------------------------------------------------------

inline ID3D12DescriptorHeap* DynamicDescriptorHeap::GetHeapPointer()
{
    if (CurrentHeapPtr == nullptr)
    {
        ASSERT(CurrentOffset == 0);
        CurrentHeapPtr  = RequestDescriptorHeap(DescriptorType);
        FirstDescriptor = DescriptorHandle(CurrentHeapPtr->GetCPUDescriptorHandleForHeapStart(), CurrentHeapPtr->GetGPUDescriptorHandleForHeapStart());
    }

    return CurrentHeapPtr;
}

// ----------------------------------------------------------------------------------------------------------------------------

uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeStagedSize()
{
    // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
    uint32_t neededSpace = 0;
    uint32_t staleParams = StaleRootParamsBitMap;
    uint32_t rootIndex;
    while (_BitScanForward((unsigned long*)&rootIndex, staleParams))
    {
        staleParams ^= (1 << rootIndex);

        uint32_t MaxSetHandle;
        ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, RootDescriptorTable[rootIndex].AssignedHandlesBitMap),
            "Root entry marked as stale but has no stale descriptors");

        neededSpace += MaxSetHandle + 1;
    }
    return neededSpace;
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables(
    D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorSize,
    DescriptorHandle destHandleStart, ID3D12GraphicsCommandList* cmdList,
    void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*setFunc)(uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE))
{
    uint32_t staleParamCount = 0;
    uint32_t tableSize[DescriptorHandleCache::kMaxNumDescriptorTables];
    uint32_t rootIndices[DescriptorHandleCache::kMaxNumDescriptorTables];
    uint32_t neededSpace = 0;
    uint32_t rootIndex;

    // Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
    uint32_t staleParams = StaleRootParamsBitMap;
    while (_BitScanForward((unsigned long*)&rootIndex, staleParams))
    {
        rootIndices[staleParamCount] = rootIndex;
        staleParams ^= (1 << rootIndex);

        uint32_t MaxSetHandle;
        ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, RootDescriptorTable[rootIndex].AssignedHandlesBitMap), "Root entry marked as stale but has no stale descriptors");

        neededSpace += MaxSetHandle + 1;
        tableSize[staleParamCount] = MaxSetHandle + 1;

        ++staleParamCount;
    }

    ASSERT(staleParamCount <= DescriptorHandleCache::kMaxNumDescriptorTables, "We're only equipped to handle so many descriptor tables");

    StaleRootParamsBitMap = 0;

    static const uint32_t           kMaxDescriptorsPerCopy = 16;
    uint32_t                        numDestDescriptorRanges = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE     pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy];
    uint32_t                        pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy];

    uint32_t                        numSrcDescriptorRanges = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE     pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy];
    uint32_t                        pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy];

    for (uint32_t i = 0; i < staleParamCount; ++i)
    {
        rootIndex = rootIndices[i];
        (cmdList->*setFunc)(rootIndex, destHandleStart.GetGpuHandle());

        DescriptorTableCache&            rootDescTable = RootDescriptorTable[rootIndex];
        D3D12_CPU_DESCRIPTOR_HANDLE*     srcHandles = rootDescTable.TableStart;
        uint64_t                         setHandles = (uint64_t)rootDescTable.AssignedHandlesBitMap;
        D3D12_CPU_DESCRIPTOR_HANDLE      curDest = destHandleStart.GetCpuHandle();

        destHandleStart += tableSize[i] * descriptorSize;

        unsigned long skipCount;
        while (_BitScanForward64(&skipCount, setHandles))
        {
            // Skip over unset descriptor handles
            setHandles >>= skipCount;
            srcHandles += skipCount;
            curDest.ptr += skipCount * descriptorSize;

            unsigned long descriptorCount;
            _BitScanForward64(&descriptorCount, ~setHandles);
            setHandles >>= descriptorCount;

            // If we run out of temp room, copy what we've got so far
            if (numSrcDescriptorRanges + descriptorCount > kMaxDescriptorsPerCopy)
            {
                RenderDevice::Get().GetD3DDevice()->CopyDescriptors(
                    numDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
                    numSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
                    type);

                numSrcDescriptorRanges = 0;
                numDestDescriptorRanges = 0;
            }

            // Setup destination range
            pDestDescriptorRangeStarts[numDestDescriptorRanges] = curDest;
            pDestDescriptorRangeSizes[numDestDescriptorRanges] = descriptorCount;
            ++numDestDescriptorRanges;

            // Setup source ranges (one descriptor each because we don't assume they are contiguous)
            for (uint32_t j = 0; j < descriptorCount; ++j)
            {
                pSrcDescriptorRangeStarts[numSrcDescriptorRanges] = srcHandles[j];
                pSrcDescriptorRangeSizes[numSrcDescriptorRanges] = 1;
                ++numSrcDescriptorRanges;
            }

            // Move the destination pointer forward by the number of descriptors we will copy
            srcHandles += descriptorCount;
            curDest.ptr += descriptorCount * descriptorSize;
        }
    }

    RenderDevice::Get().GetD3DDevice()->CopyDescriptors(
        numDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
        numSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
        type);
}

    // ----------------------------------------------------------------------------------------------------------------------------
    
void DynamicDescriptorHeap::CopyAndBindStagedTables(DescriptorHandleCache& handleCache, ID3D12GraphicsCommandList* cmdList,
    void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*setFunc)(uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE))
{
    uint32_t neededSize = handleCache.ComputeStagedSize();
    if (!HasSpace(neededSize))
    {
        RetireCurrentHeap();
        UnbindAllValid();
        neededSize = handleCache.ComputeStagedSize();
    }

    // This can trigger the creation of a new heap
    OwningContext.SetDescriptorHeap(DescriptorType, GetHeapPointer());
    handleCache.CopyAndBindStaleTables(DescriptorType, DescriptorSize, Allocate(neededSize), cmdList, setFunc);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::UnbindAllValid(void)
{
    GraphicsHandleCache.UnbindAllValid();
    ComputeHandleCache.UnbindAllValid();
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    if (!HasSpace(1))
    {
        RetireCurrentHeap();
        UnbindAllValid();
    }

    OwningContext.SetDescriptorHeap(DescriptorType, GetHeapPointer());

    DescriptorHandle destHandle = FirstDescriptor + CurrentOffset * DescriptorSize;
    CurrentOffset += 1;

    RenderDevice::Get().GetD3DDevice()->CopyDescriptorsSimple(1, destHandle.GetCpuHandle(), handle, DescriptorType);

    return destHandle.GetGpuHandle();
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
{
    StaleRootParamsBitMap = 0;

    unsigned long tableParams = RootDescriptorTablesBitMap;
    unsigned long rootIndex;
    while (_BitScanForward(&rootIndex, tableParams))
    {
        tableParams ^= (1 << rootIndex);
        if (RootDescriptorTable[rootIndex].AssignedHandlesBitMap != 0)
        {
            StaleRootParamsBitMap |= (1 << rootIndex);
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
{
    ASSERT(((1 << rootIndex) & RootDescriptorTablesBitMap) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table");
    ASSERT(offset + numHandles <= RootDescriptorTable[rootIndex].TableSize);

    DescriptorTableCache& tableCache = RootDescriptorTable[rootIndex];
    D3D12_CPU_DESCRIPTOR_HANDLE* copyDest = tableCache.TableStart + offset;
    for (uint32_t i = 0; i < numHandles; ++i)
    {
        copyDest[i] = handles[i];
    }
    tableCache.AssignedHandlesBitMap |= ((1 << numHandles) - 1) << offset;
    StaleRootParamsBitMap |= (1 << rootIndex);
}

// ----------------------------------------------------------------------------------------------------------------------------

void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig)
{
    uint32_t currentOffset = 0;

    ASSERT(rootSig.NumParameters <= 16, "Maybe we need to support something greater");

    StaleRootParamsBitMap = 0;
    RootDescriptorTablesBitMap = (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
        rootSig.SamplerTableBitMap : rootSig.DescriptorTableBitMap);

    unsigned long tableParams = RootDescriptorTablesBitMap;
    unsigned long rootIndex;
    while (_BitScanForward(&rootIndex, tableParams))
    {
        tableParams ^= (1 << rootIndex);

        uint32_t tableSize = rootSig.DescriptorTableSize[rootIndex];
        ASSERT(tableSize > 0);

        DescriptorTableCache& rootDescriptorTable = RootDescriptorTable[rootIndex];
        rootDescriptorTable.AssignedHandlesBitMap = 0;
        rootDescriptorTable.TableStart = HandleCache + currentOffset;
        rootDescriptorTable.TableSize = tableSize;

        currentOffset += tableSize;
    }

    MaxCachedDescriptors = currentOffset;

    ASSERT(MaxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
}
