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
#include <chrono>
#include <atomic>
#include <functional>

// ----------------------------------------------------------------------------------------------------------------------------

namespace Core
{
    class ThreadEvent
    {
    public:

        inline ThreadEvent() : Triggered(false) {}

        inline void Signal()
        {
            std::unique_lock<std::mutex> lck(Mutex);

            Triggered = true;
            CondVar.notify_one();
        }

        inline void Reset()
        {
            std::unique_lock<std::mutex> lck(Mutex);

            Triggered = false;
        }

        inline bool WaitOne(int timeOutMicroseconds)
        {
            std::unique_lock<std::mutex> lck(Mutex);

            while (!Triggered)
            {
                if (timeOutMicroseconds >= 0)
                {
                    bool timedOut = CondVar.wait_for(
                        lck,
                        std::chrono::microseconds(timeOutMicroseconds),
                        std::bind(&ThreadEvent::Triggered, this)
                    );

                    return Triggered;
                }
                else
                {
                    CondVar.wait(
                        lck,
                        std::bind(&ThreadEvent::Triggered, this)
                    );
                }
            }

            return Triggered;
        }

    private:

        std::mutex               Mutex;
        std::condition_variable  CondVar;
        std::atomic<bool>        Triggered;
    };
}
