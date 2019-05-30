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

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

// ----------------------------------------------------------------------------------------------------------------------------

namespace Core
{
    template <typename T>
    class SafeQueue
    {
    public:

        // Disable copying of the queue
        SafeQueue()                             = default;
        SafeQueue(const SafeQueue&)             = delete;
        SafeQueue& operator=(const SafeQueue&)  = delete;

        bool Pop(T& item, int timeOutMicroseconds)
        {
            std::unique_lock<std::mutex> lck(Mutex);
            
            while (!NotEmpty)
            {
                if (timeOutMicroseconds >= 0)
                {
                    bool timedOut = CondVar.wait_for(
                        lck,
                        std::chrono::microseconds(timeOutMicroseconds),
                        std::bind(&SafeQueue::NotEmpty, this)
                    );

                    if (!NotEmpty || timedOut)
                    {
                        return false;
                    }
                }
                else
                {
                    CondVar.wait(
                        lck,
                        std::bind(&SafeQueue::NotEmpty, this)
                    );
                }
            }

            item = Queue.front();
            Queue.pop();
            NotEmpty = (Queue.size() > 0);

            return true;
        }

        void Push(const T& item)
        {
            std::unique_lock<std::mutex> guardLock(Mutex);
            Queue.push(item);
            NotEmpty = true;
            CondVar.notify_one();
        }

    private:

        bool                    NotEmpty = false;
        std::condition_variable CondVar;
        std::mutex              Mutex;
        std::queue<T>           Queue;
    };
}
