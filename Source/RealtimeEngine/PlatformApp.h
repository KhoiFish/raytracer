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
#include "RenderInterface.h"
#include "../Core/ThreadEvent.h"
#include "../Core/SafeQueue.h"
#include <thread>
#include <atomic>

// ----------------------------------------------------------------------------------------------------------------------------

namespace RealtimeEngine
{
    class PlatformApp
    {
    public:

        static int      Run(RenderInterface* pRenderInterface, HINSTANCE hInstance, int nCmdShow);
        static void     SetWindowZorderToTopMost(bool setToTopMost);
        static void     SetWindowTitle(const string_t& title);
        static HWND     GetHwnd();
        static bool     IsFullscreen();

    private:

        enum MessageType
        {
            MessageType_None,
            MessageType_Size,
            MessageType_KeyDown,
            MessageType_KeyUp,
            MessageType_AltEnter,
            MessageType_MouseMove,
            MessageType_LButtonDown,
            MessageType_LButtonUp,
        };

        struct RenderMessage
        {
            RenderMessage() {}

            RenderMessage(MessageType type, int32_t param0 = 0, int32_t param1 = 0, int32_t param2 = 0)
                : Type(type)
            {
                Params[0] = param0;
                Params[1] = param1;
                Params[2] = param2;
            }

            MessageType  Type;
            int32_t      Params[3];
        };

        struct MouseCoord
        {
            MouseCoord() {}
            MouseCoord(int32_t x, int32_t y) : X(x), Y(y) {}

            bool operator !=(const MouseCoord& other)
            {
                return (X != other.X || Y != other.Y);
            }

            int32_t X;
            int32_t Y;
        };

    private:

        static LRESULT CALLBACK                 WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam);
        static bool                             HandleRenderMessage(const RenderMessage& msg);
        static void                             RenderThreadMain();
        static void                             ToggleFullscreenWindow(IDXGISwapChain* pOutput = nullptr);

    private:

        static const uint32_t                   WindowStyle = WS_OVERLAPPEDWINDOW;
        static HWND                             Hwnd;
        static bool                             FullscreenMode;
        static RECT                             WindowRect;
        static double                           PCFreq;
        static __int64                          PrevCounter;
        static RenderInterface*                 Renderer;
        static std::thread*                     RenderThread;
        static Core::ThreadEvent                RenderThreadEvent;
        static Core::SafeQueue<RenderMessage>   MessageQueue;
        static std::atomic<MouseCoord>          CurrentMouseCoord;
        static MouseCoord                       LastMouseCoord;
        static bool                             ExitRenderThread;
    };

}
