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

#include "CommandList.h"
#include <algorithm>

using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

static CommandListManager gCommandManager;

// ----------------------------------------------------------------------------------------------------------------------------

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type) :
    CommandListType(type),
    Device(nullptr)
{
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandAllocatorPool::~CommandAllocatorPool()
{
    Shutdown();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandAllocatorPool::Create(ID3D12Device* pDevice)
{
    Device = pDevice;
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandAllocatorPool::Shutdown()
{
    for (size_t i = 0; i < AllocatorPool.size(); ++i)
    {
        AllocatorPool[i]->Release();
    }

    AllocatorPool.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t completedFenceValue)
{
    std::lock_guard<std::mutex> lockGuard(AllocatorMutex);

    ID3D12CommandAllocator* pAllocator = nullptr;
    if (!ReadyAllocators.empty())
    {
        std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = ReadyAllocators.front();

        if (AllocatorPair.first <= completedFenceValue)
        {
            pAllocator = AllocatorPair.second;
            RTL_HRESULT_SUCCEEDED(pAllocator->Reset());
            ReadyAllocators.pop();
        }
    }

    // If no allocator's were ready to be reused, create a new one
    if (pAllocator == nullptr)
    {
        RTL_HRESULT_SUCCEEDED(Device->CreateCommandAllocator(CommandListType, IID_PPV_ARGS(&pAllocator)));
        wchar_t AllocatorName[32];
        swprintf(AllocatorName, 32, L"CommandAllocator %zu", AllocatorPool.size());
        pAllocator->SetName(AllocatorName);
        AllocatorPool.push_back(pAllocator);
    }

    return pAllocator;
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator * allocator)
{
    std::lock_guard<std::mutex> lockGuard(AllocatorMutex);

    // That fence value indicates we are free to reset the allocator
    ReadyAllocators.push(std::make_pair(fenceValue, allocator));
}


// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------


CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type) :
    Type(type),
    TheCommandQueue(nullptr),
    FencePtr(nullptr),
    NextFenceValue((uint64_t)Type << 56 | 1),
    LastCompletedFenceValue((uint64_t)Type << 56),
    AllocatorPool(Type)
{
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandQueue::~CommandQueue()
{
    Shutdown();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandQueue::Shutdown()
{
    if (TheCommandQueue == nullptr)
    {
        return;
    }

    AllocatorPool.Shutdown();

    CloseHandle(FenceEventHandle);

    FencePtr->Release();
    FencePtr = nullptr;

    TheCommandQueue->Release();
    TheCommandQueue = nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandQueue::Create(ID3D12Device* pDevice)
{
    RTL_ASSERT(pDevice != nullptr);
    RTL_ASSERT(!IsReady());
    RTL_ASSERT(AllocatorPool.Size() == 0);

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = Type;
    queueDesc.NodeMask = 1;
    pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&TheCommandQueue));
    TheCommandQueue->SetName(L"CommandList::TheCommandQueue");

    RTL_HRESULT_SUCCEEDED(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&FencePtr)));
    FencePtr->SetName(L"CommandListManager::FencePtr");
    FencePtr->Signal((uint64_t)Type << 56);

    FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
    RTL_ASSERT(FenceEventHandle != INVALID_HANDLE_VALUE);

    AllocatorPool.Create(pDevice);

    RTL_ASSERT(IsReady());
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandListManager::CommandListManager() :
    Device(nullptr),
    GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
    ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
    CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
{
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandListManager::~CommandListManager()
{
    Shutdown();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandListManager::Shutdown()
{
    GraphicsQueue.Shutdown();
    ComputeQueue.Shutdown();
    CopyQueue.Shutdown();
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandQueue& CommandListManager::GetGraphicsQueue()
{
    return GraphicsQueue;
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandQueue& CommandListManager::GetComputeQueue()
{
    return ComputeQueue;
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandQueue& CommandListManager::GetCopyQueue()
{
    return CopyQueue;
}

// ----------------------------------------------------------------------------------------------------------------------------

uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* list)
{
    std::lock_guard<std::mutex> lockGuard(FenceMutex);

    RTL_HRESULT_SUCCEEDED(((ID3D12GraphicsCommandList4*)list)->Close());

    // Kickoff the command list
    TheCommandQueue->ExecuteCommandLists(1, &list);

    // Signal the next fence value (with the GPU)
    TheCommandQueue->Signal(FencePtr, NextFenceValue);

    // And increment the fence value.  
    return NextFenceValue++;
}

// ----------------------------------------------------------------------------------------------------------------------------

uint64_t CommandQueue::IncrementFence()
{
    std::lock_guard<std::mutex> lockGuard(FenceMutex);
    TheCommandQueue->Signal(FencePtr, NextFenceValue);
    return NextFenceValue++;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
    // Avoid querying the fence value by testing against the last one seen.
    // The max() is to protect against an unlikely race condition that could cause the last
    // completed fence value to regress.
    if (fenceValue > LastCompletedFenceValue)
    {
        LastCompletedFenceValue = std::max(LastCompletedFenceValue, FencePtr->GetCompletedValue());
    }

    return fenceValue <= LastCompletedFenceValue;
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandQueue::StallForFence(uint64_t fenceValue)
{
    CommandQueue& producer = gCommandManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
    TheCommandQueue->Wait(producer.FencePtr, fenceValue);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandQueue::StallForProducer(CommandQueue& producer)
{
    RTL_ASSERT(producer.NextFenceValue > 0);
    TheCommandQueue->Wait(producer.FencePtr, producer.NextFenceValue - 1);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandQueue::WaitForFence(uint64_t FenceValue)
{
    if (IsFenceComplete(FenceValue))
    {
        return;
    }

    // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
    // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
    // the fence can only have one event set on completion, then thread B has to wait for 
    // 100 before it knows 99 is ready.  Maybe insert sequential events?
    {
        std::lock_guard<std::mutex> lockGuard(EventMutex);

        FencePtr->SetEventOnCompletion(FenceValue, FenceEventHandle);
        WaitForSingleObject(FenceEventHandle, INFINITE);
        LastCompletedFenceValue = FenceValue;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

ID3D12CommandAllocator* CommandQueue::RequestAllocator()
{
    uint64_t completedFence = FencePtr->GetCompletedValue();

    return AllocatorPool.RequestAllocator(completedFence);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandQueue::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
{
    AllocatorPool.DiscardAllocator(fenceValue, allocator);
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandListManager& CommandListManager::Get()
{
    return gCommandManager;
}


// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------


void CommandListManager::Create(ID3D12Device* pDevice)
{
    RTL_ASSERT(pDevice != nullptr);

    Device = pDevice;

    GraphicsQueue.Create(pDevice);
    ComputeQueue.Create(pDevice);
    CopyQueue.Create(pDevice);
}

// ----------------------------------------------------------------------------------------------------------------------------

CommandQueue& CommandListManager::GetQueue(D3D12_COMMAND_LIST_TYPE type)
{
    switch (type)
    {
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return ComputeQueue;
        case D3D12_COMMAND_LIST_TYPE_COPY:    return CopyQueue;
        default:                              return GraphicsQueue;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

ID3D12CommandQueue* CommandListManager::GetCommandQueue()
{
    return GraphicsQueue.GetCommandQueue();
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandListManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** list, ID3D12CommandAllocator** allocator)
{
    RTL_ASSERT_MSG(type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
    switch (type)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:   *allocator = GraphicsQueue.RequestAllocator(); break;
        case D3D12_COMMAND_LIST_TYPE_BUNDLE:   break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:  *allocator = ComputeQueue.RequestAllocator(); break;
        case D3D12_COMMAND_LIST_TYPE_COPY:     *allocator = CopyQueue.RequestAllocator(); break;
    }
    
    RTL_HRESULT_SUCCEEDED( Device->CreateCommandList(1, type, *allocator, nullptr, IID_PPV_ARGS(list)) );
    (*list)->SetName(L"CommandList");
}

// ----------------------------------------------------------------------------------------------------------------------------

bool CommandListManager::IsFenceComplete(uint64_t fenceValue)
{
    return GetQueue(D3D12_COMMAND_LIST_TYPE(fenceValue >> 56)).IsFenceComplete(fenceValue);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandListManager::WaitForFence(uint64_t fenceValue)
{
    CommandQueue& producer = gCommandManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
    producer.WaitForFence(fenceValue);
}

// ----------------------------------------------------------------------------------------------------------------------------

void CommandListManager::IdleGPU()
{
    GraphicsQueue.WaitForIdle();
    ComputeQueue.WaitForIdle();
    CopyQueue.WaitForIdle();
}
