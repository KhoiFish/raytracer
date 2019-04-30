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
#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    // ----------------------------------------------------------------------------------------------------------------------------

    class CommandAllocatorPool
    {
    public:

        CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type);
        ~CommandAllocatorPool();\

    public:

        void                                                       Create(ID3D12Device* pDevice);
        void                                                       Shutdown();
        ID3D12CommandAllocator*                                    RequestAllocator(uint64_t completedFenceValue);
        void                                                       DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);
        inline size_t                                              Size() { return AllocatorPool.size(); }

    private:

        const D3D12_COMMAND_LIST_TYPE                              CommandListType;
        ID3D12Device*                                              Device;
        std::vector<ID3D12CommandAllocator*>                       AllocatorPool;
        std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>>   ReadyAllocators;
        std::mutex                                                 AllocatorMutex;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class CommandQueue
    {
    public:

        CommandQueue(D3D12_COMMAND_LIST_TYPE type);
        ~CommandQueue();

    public:

        void                            Create(ID3D12Device* pDevice);
        void                            Shutdown();
        uint64_t                        IncrementFence();
        bool                            IsFenceComplete(uint64_t fenceValue);
        void                            StallForFence(uint64_t fenceValue);
        void                            StallForProducer(CommandQueue& producer);
        void                            WaitForFence(uint64_t fenceValue);

        void                            WaitForIdle()        { WaitForFence(IncrementFence()); }
        ID3D12CommandQueue*             GetCommandQueue()    { return TheCommandQueue; }
        uint64_t                        GetNextFenceValue()  { return NextFenceValue; }
        inline bool                     IsReady()            { return TheCommandQueue != nullptr; }

    private:

        uint64_t                        ExecuteCommandList(ID3D12CommandList* list);
        ID3D12CommandAllocator*         RequestAllocator();
        void                            DiscardAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* allocator);

    private:

        friend class CommandListManager;
        friend class CommandContext;

        ID3D12CommandQueue*             TheCommandQueue;
        const D3D12_COMMAND_LIST_TYPE   Type;
        CommandAllocatorPool            AllocatorPool;
        std::mutex                      FenceMutex;
        std::mutex                      EventMutex;

        // Lifetime of these objects is managed by the descriptor cache
        ID3D12Fence*                    FencePtr;
        uint64_t                        NextFenceValue;
        uint64_t                        LastCompletedFenceValue;
        HANDLE                          FenceEventHandle;

    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class CommandListManager
    {
    public:

        CommandListManager();
        ~CommandListManager();

    public:

        static CommandListManager&  Get();
        void                        Create(ID3D12Device* pDevice);
        void                        Shutdown();

        CommandQueue&               GetGraphicsQueue();
        CommandQueue&               GetComputeQueue();
        CommandQueue&               GetCopyQueue();
        CommandQueue&               GetQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
        ID3D12CommandQueue*         GetCommandQueue();

        void                        CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** list, ID3D12CommandAllocator** allocator);
        bool                        IsFenceComplete(uint64_t fenceValue);
        void                        WaitForFence(uint64_t fenceValue);
        void                        IdleGPU();

    private:

        friend class CommandContext;

        ID3D12Device*               Device;
        CommandQueue                GraphicsQueue;
        CommandQueue                ComputeQueue;
        CommandQueue                CopyQueue;
    };
}
