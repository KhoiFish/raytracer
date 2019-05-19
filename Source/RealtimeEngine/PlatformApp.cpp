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
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_win32.h>

// ----------------------------------------------------------------------------------------------------------------------------

using Microsoft::WRL::ComPtr;
using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

HWND    PlatformApp::Hwnd = nullptr;
bool    PlatformApp::FullscreenMode = false;
RECT    PlatformApp::WindowRect;
double  PlatformApp::PCFreq = 0;
__int64 PlatformApp::PrevCounter = 0;

// ----------------------------------------------------------------------------------------------------------------------------

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ----------------------------------------------------------------------------------------------------------------------------

int PlatformApp::Run(RenderInterface* pRenderInterface, HINSTANCE hInstance, int nCmdShow)
{
    try
    {
        // Init performance counter
        {
            LARGE_INTEGER li;

            QueryPerformanceFrequency(&li);
            PCFreq = double(li.QuadPart);

            QueryPerformanceCounter(&li);
            PrevCounter = li.QuadPart;
        }

        // Parse the command line parameters
        /*int argc;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        pSample->ParseCommandLineArgs(argv, argc);
        LocalFree(argv);*/

        // Initialize the window class.
        WNDCLASSEX windowClass = { 0 };
        windowClass.cbSize = sizeof(WNDCLASSEX);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = hInstance;
        windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        windowClass.lpszClassName = L"PlatformApp";
        RegisterClassEx(&windowClass);

        RECT windowRect = { 0, 0, static_cast<LONG>(pRenderInterface->GetWidth()), static_cast<LONG>(pRenderInterface->GetHeight()) };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        // Create the window and store a handle to it.
        Hwnd = CreateWindow(
            windowClass.lpszClassName,
            L"yart",
            WindowStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,        // We have no parent window.
            nullptr,        // We aren't using menus.
            hInstance,
            pRenderInterface);

        // Initialize the sample. OnInit is defined in each child-implementation of DXSample.
        pRenderInterface->OnInit();

        ShowWindow(Hwnd, nCmdShow);

        // Main sample loop.
        MSG msg = {};
        while (msg.message != WM_QUIT)
        {
            // Process any messages in the queue.
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        pRenderInterface->OnDestroy();

        // Return this part of the WM_QUIT message to Windows.
        return static_cast<char>(msg.wParam);
    }
    catch (std::exception & e)
    {
        OutputDebugString(L"Application hit a problem: ");
        OutputDebugStringA(e.what());
        OutputDebugString(L"\nTerminating.\n");

        pRenderInterface->OnDestroy();
        return EXIT_FAILURE;
    }
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
    RenderInterface* pRenderInterface = reinterpret_cast<RenderInterface*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    bool keyboardCaptured = false, mouseCaptured = false;
    if (ImGui::GetCurrentContext() != nullptr && !pRenderInterface->OverrideImguiInput())
    {
        // Pass message to imgui
        ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);

        // See if mouse wants to be captured
        ImGuiIO& imguiIo = ImGui::GetIO();
        keyboardCaptured = imguiIo.WantCaptureKeyboard;
        mouseCaptured    = imguiIo.WantCaptureMouse;
    }
    
    switch (message)
    {
    case WM_CREATE:
    {
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
    return 0;

    case WM_KEYDOWN:
        if (pRenderInterface && !keyboardCaptured)
        {
            pRenderInterface->OnKeyDown(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_KEYUP:
        if (pRenderInterface && !keyboardCaptured)
        {
            pRenderInterface->OnKeyUp(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_SYSKEYDOWN:
        // Handle ALT+ENTER:
        if ((wParam == VK_RETURN) && (lParam & (1 << 29)))
        {
            ;
        }
        break;

    case WM_PAINT:
        if (pRenderInterface)
        {
            // Get delta time
            float dtSeconds;
            {
                LARGE_INTEGER li;
                QueryPerformanceCounter(&li);
                dtSeconds   = float(double(li.QuadPart - PrevCounter) / PCFreq);
                PrevCounter = li.QuadPart;
            }

            pRenderInterface->OnUpdate(dtSeconds);
            pRenderInterface->OnRender();
        }
        return 0;

    case WM_SIZE:
        if (pRenderInterface)
        {
            RECT windowRect = {};
            GetWindowRect(hWnd, &windowRect);
            pRenderInterface->SetWindowBounds(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

            RECT clientRect = {};
            GetClientRect(hWnd, &clientRect);
            pRenderInterface->OnSizeChanged(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, wParam == SIZE_MINIMIZED);
        }
        return 0;

    case WM_MOVE:
        if (pRenderInterface)
        {
            RECT windowRect = {};
            GetWindowRect(hWnd, &windowRect);
            pRenderInterface->SetWindowBounds(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

            int xPos = (int)(short)LOWORD(lParam);
            int yPos = (int)(short)HIWORD(lParam);
            pRenderInterface->OnWindowMoved(xPos, yPos);
        }
        return 0;

    case WM_DISPLAYCHANGE:
        if (pRenderInterface)
        {
            pRenderInterface->OnDisplayChanged();
        }
        return 0;

    case WM_MOUSEMOVE:
        if (pRenderInterface && static_cast<UINT8>(wParam) == MK_LBUTTON && !mouseCaptured)
        {
            uint32_t x = LOWORD(lParam);
            uint32_t y = HIWORD(lParam);
            pRenderInterface->OnMouseMove(x, y);
        }
        return 0;

    case WM_LBUTTONDOWN:
    if (!mouseCaptured)
    {
        uint32_t x = LOWORD(lParam);
        uint32_t y = HIWORD(lParam);
        pRenderInterface->OnLeftButtonDown(x, y);
    }
    return 0;

    case WM_LBUTTONUP:
    if  (!mouseCaptured)
    {
        uint32_t x = LOWORD(lParam);
        uint32_t y = HIWORD(lParam);
        pRenderInterface->OnLeftButtonUp(x, y);
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}
