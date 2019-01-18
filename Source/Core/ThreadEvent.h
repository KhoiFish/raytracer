#pragma once

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <functional>

// ----------------------------------------------------------------------------------------------------------------------------

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

                if (timedOut)
                {
                    return Triggered;
                }
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



