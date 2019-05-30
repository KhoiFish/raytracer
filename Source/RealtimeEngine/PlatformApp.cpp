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

#include "PlatformApp.h"
#include "windowsx.h"
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_win32.h>
#include <shellscalingapi.h>

// ----------------------------------------------------------------------------------------------------------------------------

using Microsoft::WRL::ComPtr;
using namespace RealtimeEngine;

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ----------------------------------------------------------------------------------------------------------------------------

HWND                                        PlatformApp::Hwnd               = nullptr;
double                                      PlatformApp::PCFreq             = 0;
__int64                                     PlatformApp::PrevCounter        = 0;
bool                                        PlatformApp::FullscreenMode     = false;
std::thread*                                PlatformApp::RenderThread       = nullptr;
bool                                        PlatformApp::ExitRenderThread   = false;
RenderInterface*                            PlatformApp::Renderer           = nullptr;
RECT                                        PlatformApp::WindowRect;
Core::ThreadEvent                           PlatformApp::RenderThreadEvent;
std::atomic<PlatformApp::MouseCoord>        PlatformApp::CurrentMouseCoord;
PlatformApp::MouseCoord                     PlatformApp::LastMouseCoord;
Core::SafeQueue<PlatformApp::RenderMessage> PlatformApp::MessageQueue;

// ----------------------------------------------------------------------------------------------------------------------------

static void centerWindow(HWND hWnd)
{
    RECT rc;

    GetWindowRect(hWnd, &rc);

    int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
    int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

    SetWindowPos(hWnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

// ----------------------------------------------------------------------------------------------------------------------------

int PlatformApp::Run(RenderInterface* pRenderInterface, HINSTANCE hInstance, int nCmdShow)
{
    int retValue = 0;

    try
    {
        // Set renderer pointer
        Renderer = pRenderInterface;

        // Tell windows we're DPI aware so it doesn't do crappy scaling on our window
        SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_SYSTEM_DPI_AWARE);

        // Windows init
        WNDCLASSEX windowClass = { 0 };
        windowClass.cbSize          = sizeof(WNDCLASSEX);
        windowClass.style           = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc     = WindowProc;
        windowClass.hInstance       = hInstance;
        windowClass.hCursor         = LoadCursor(NULL, IDC_ARROW);
        windowClass.lpszClassName   = L"PlatformApp";
        RegisterClassEx(&windowClass);

        RECT windowRect = { 0, 0, static_cast<LONG>(pRenderInterface->GetWidth()), static_cast<LONG>(pRenderInterface->GetHeight()) };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        // Create window
        Hwnd = CreateWindow(
            windowClass.lpszClassName,
            MakeWStr(pRenderInterface->GetName()).c_str(),
            WindowStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,
            nullptr,
            hInstance,
            pRenderInterface);

        ShowWindow(Hwnd, nCmdShow);
        centerWindow(Hwnd);

        // Create the render thread and start it
        RenderThreadEvent.Reset();
        ExitRenderThread = false;
        RenderThread     = new std::thread(RenderThreadMain);

        // Main windows message loop
        MSG msg = {};
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        retValue = (int)msg.wParam;
    }
    catch (std::exception& e)
    {
        RenderDebugPrintf("Application hit a problem: %s", e.what());
        retValue = EXIT_FAILURE;
    }

    // Exit render thread
    if (RenderThread != nullptr)
    {
        // Signal thread to exit and wait
        ExitRenderThread = true;
        RenderThreadEvent.WaitOne(-1);

        // Kill the thread
        RenderThread->join();
        delete RenderThread;
        RenderThread = nullptr;
    }

    return retValue;
}

// ----------------------------------------------------------------------------------------------------------------------------

void PlatformApp::ToggleFullscreenWindow(IDXGISwapChain* pSwapChain)
{
    if (FullscreenMode)
    {
        // Restore the window's attributes and size.
        SetWindowLong(Hwnd, GWL_STYLE, WindowStyle);

        SetWindowPos(
            Hwnd,
            HWND_NOTOPMOST,
            WindowRect.left,
            WindowRect.top,
            WindowRect.right - WindowRect.left,
            WindowRect.bottom - WindowRect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ShowWindow(Hwnd, SW_NORMAL);
    }
    else
    {
        // Save the old window rect so we can restore it when exiting fullscreen mode.
        GetWindowRect(Hwnd, &WindowRect);

        // Make the window borderless so that the client area can fill the screen.
        SetWindowLong(Hwnd, GWL_STYLE, WindowStyle & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME));

        RECT fullscreenWindowRect;
        try
        {
            if (pSwapChain)
            {
                // Get the settings of the display on which the app's window is currently displayed
                ComPtr<IDXGIOutput> pOutput;
                ThrowIfFailed(pSwapChain->GetContainingOutput(&pOutput));
                DXGI_OUTPUT_DESC Desc;
                ThrowIfFailed(pOutput->GetDesc(&Desc));
                fullscreenWindowRect = Desc.DesktopCoordinates;
            }
            else
            {
                // Fallback to EnumDisplaySettings implementation
                throw HrException(S_FALSE);
            }
        }
        catch (HrException & e)
        {
            UNREFERENCED_PARAMETER(e);

            // Get the settings of the primary display
            DEVMODE devMode = {};
            devMode.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode);

            fullscreenWindowRect = {
                devMode.dmPosition.x,
                devMode.dmPosition.y,
                devMode.dmPosition.x + static_cast<LONG>(devMode.dmPelsWidth),
                devMode.dmPosition.y + static_cast<LONG>(devMode.dmPelsHeight)
            };
        }

        SetWindowPos(
            Hwnd,
            HWND_TOPMOST,
            fullscreenWindowRect.left,
            fullscreenWindowRect.top,
            fullscreenWindowRect.right,
            fullscreenWindowRect.bottom,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);


        ShowWindow(Hwnd, SW_MAXIMIZE);
    }

    FullscreenMode = !FullscreenMode;
}

// ----------------------------------------------------------------------------------------------------------------------------

void PlatformApp::SetWindowZorderToTopMost(bool setToTopMost)
{
    RECT windowRect;
    GetWindowRect(Hwnd, &windowRect);

    SetWindowPos(
        Hwnd,
        (setToTopMost) ? HWND_TOPMOST : HWND_NOTOPMOST,
        windowRect.left,
        windowRect.top,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        SWP_FRAMECHANGED | SWP_NOACTIVATE);
}

// ----------------------------------------------------------------------------------------------------------------------------

LRESULT CALLBACK PlatformApp::WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() != nullptr)
    {
        // Pass message to imgui
        ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);
    }

    switch (message)
    {
        case WM_CREATE:
        {
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
            return 0;
        }

        case WM_PAINT:
        case WM_DISPLAYCHANGE:
        {
            // Throttle paint messages
            Sleep(16);
            return 0;
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }

        case WM_SIZE:
        {
            RECT clientRect = {};
            GetClientRect(Hwnd, &clientRect);
            int32_t newWidth  = clientRect.right - clientRect.left;
            int32_t newHeight = clientRect.bottom - clientRect.top;
            MessageQueue.Push(RenderMessage(MessageType_Size, newWidth, newHeight, int32_t(wParam)));

            // Throttle size messages
            Sleep(16);

            return 0;
        }

        case WM_SYSKEYDOWN:
        {
            // Handle ALT+ENTER:
            if ((wParam == VK_RETURN) && (lParam & (1 << 29)))
            {
                MessageQueue.Push(RenderMessage(MessageType_AltEnter));
            }
        }
        break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            if (message == WM_KEYDOWN && wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
                return 0;
            }

            MessageType type = (message == WM_KEYDOWN) ? MessageType_KeyDown : MessageType_KeyUp;

            MessageQueue.Push(RenderMessage(type, int32_t(wParam), int32_t(lParam)));
            return 0;
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        {
            int32_t         x    = GET_X_LPARAM(lParam);
            int32_t         y    = GET_Y_LPARAM(lParam);
            MessageType     type = (message == WM_LBUTTONDOWN) ? MessageType_LButtonDown : MessageType_LButtonUp;
            RenderMessage   msg(type, x, y);
            MessageQueue.Push(msg);

            return 0;
        }

        case WM_MOUSEMOVE:
        {
            if (wParam == MK_LBUTTON)
            {
                MouseCoord coord(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                CurrentMouseCoord.store(coord);
            }
        }
        break;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// ----------------------------------------------------------------------------------------------------------------------------

void PlatformApp::RenderThreadMain()
{
    // Set to high priority thread
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Init performance counter
    {
        LARGE_INTEGER li;

        QueryPerformanceFrequency(&li);
        PCFreq = double(li.QuadPart);

        QueryPerformanceCounter(&li);
        PrevCounter = li.QuadPart;
    }

    // Init rendering
    Renderer->OnInit();

    std::queue<RenderMessage> culledMessageQueue;
    while (!ExitRenderThread)
    {
        // Process render messages
        {
            // Handle all messages in the queue
            RenderMessage msg;
            RenderMessage sizeMessage(MessageType_None);
            while (MessageQueue.Pop(msg, 0))
            {
                if (msg.Type == MessageType_Size)
                {
                    // Ignore until we get the last one
                    sizeMessage = msg;
                    continue;
                }

                HandleRenderMessage(msg);
            }

            // Process any (and last) size message
            if (sizeMessage.Type == MessageType_Size)
            {
                HandleRenderMessage(sizeMessage);
            }

            // Process mouse move
            if (LastMouseCoord != CurrentMouseCoord.load())
            {
                LastMouseCoord = CurrentMouseCoord.load();
                HandleRenderMessage(RenderMessage(PlatformApp::MessageType_MouseMove, LastMouseCoord.X, LastMouseCoord.Y));
            }
        }

        // Pump app rendering and update
        {
            LARGE_INTEGER li;
            QueryPerformanceCounter(&li);
            float dtSeconds = float(double(li.QuadPart - PrevCounter) / PCFreq);
            PrevCounter = li.QuadPart;

            Renderer->OnUpdate(dtSeconds);
            Renderer->OnRender();
        }
    }

    // Shutdown render
    Renderer->OnDestroy();

    // Signal we're done
    RenderThreadEvent.Signal();
}

// ----------------------------------------------------------------------------------------------------------------------------

void PlatformApp::HandleRenderMessage(const RenderMessage& msg)
{
    // See if IMGUI has captured keyboard/mouse events
    if (ImGui::GetCurrentContext() != nullptr)
    {
        // See if mouse wants to be captured
        ImGuiIO& imguiIo = ImGui::GetIO();
        bool keyboardCaptured = imguiIo.WantCaptureKeyboard;
        bool mouseCaptured    = imguiIo.WantCaptureMouse;

        // Bail out of keyboard/mouse messages if captured
        if (keyboardCaptured)
        {
            switch (msg.Type)
            {
                case MessageType_KeyDown:
                case MessageType_KeyUp:
                    return;
            }
        }
        if(mouseCaptured)
        {
            switch (msg.Type)
            {
            case MessageType_LButtonDown:
            case MessageType_LButtonUp:
            case MessageType_MouseMove:
                return;
            }
        }
    }

    // Handle render message
    switch (msg.Type)
    {
        case MessageType_Size:
        {
            Renderer->UpdateForSizeChange(msg.Params[0], msg.Params[1]);
            Renderer->OnSizeChanged(msg.Params[0], msg.Params[1], msg.Params[2] == SIZE_MINIMIZED);
        }
        break;

        case MessageType_KeyDown:
        {
            Renderer->OnKeyDown(static_cast<UINT8>(msg.Params[0]));
        }
        break;
        

        case MessageType_KeyUp:
        {
            Renderer->OnKeyUp(static_cast<UINT8>(msg.Params[0]));
        }
        break;

        case MessageType_AltEnter:
        {
            ToggleFullscreenWindow(RenderDevice::Get().GetSwapChain());
        }
        break;

        case MessageType_LButtonDown:
        {
            Renderer->OnLeftButtonDown(msg.Params[0], msg.Params[1]);
        }
        break;

        case MessageType_LButtonUp:
        {
            Renderer->OnLeftButtonUp(msg.Params[0], msg.Params[1]);
        }
        break;

        case MessageType_MouseMove:
        {
            Renderer->OnMouseMove(msg.Params[0], msg.Params[1]);
        }
        break;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void PlatformApp::SetWindowTitle(const string_t& title)
{
    SetWindowTextA(Hwnd, title.c_str());
}

// ----------------------------------------------------------------------------------------------------------------------------

HWND PlatformApp::GetHwnd()
{
    return Hwnd;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool PlatformApp::IsFullscreen()
{
    return FullscreenMode;
}
