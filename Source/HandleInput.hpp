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

void Renderer::OnKeyDown(UINT8 key)
{
    switch (key)
    {
        case VK_SPACE:
        {
            ToggleCpuRaytracer();
            SetCameraDirty();
            break;
        }

        case VK_SHIFT:
        {
            TheUserInputData.ShiftKeyPressed = true;
            break;
        }

        case VK_UP:
        case 'W':
        case 'w':
        {
            TheUserInputData.Forward = 1.0f;
            SetCameraDirty();
            break;
        }

        case VK_DOWN:
        case 'S':
        case 's':
        {
            TheUserInputData.Backward = 1.0f;
            SetCameraDirty();
            break;
        }

        case VK_LEFT:
        case 'A':
        case 'a':
        {
            TheUserInputData.Left = 1.0f;
            SetCameraDirty();
            break;
        }

        case VK_RIGHT:
        case 'D':
        case 'd':
        {
            TheUserInputData.Right = 1.0f;
            SetCameraDirty();
            break;
        }

        case 'Q':
        case 'q':
        {
            TheUserInputData.Down = 1.0f;
            SetCameraDirty();
            break;
        }
        

        case 'E':
        case 'e':
        {
            TheUserInputData.Up = 1.0f;
            SetCameraDirty();
            break;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnKeyUp(uint8_t key)
{
    switch (key)
    {
        case VK_SHIFT:
        {
            TheUserInputData.ShiftKeyPressed = false;
            break;
        }

        case VK_UP:
        case 'W':
        case 'w':
        {
            TheUserInputData.Forward = 0.0f;
            SetCameraDirty();
            break;
        }

        case VK_DOWN:
        case 'S':
        case 's':
        {
            TheUserInputData.Backward = 0.0f;
            SetCameraDirty();
            break;
        }

        case VK_LEFT:
        case 'A':
        case 'a':
        {
            TheUserInputData.Left = 0.0f;
            SetCameraDirty();
            break;
        }

        case VK_RIGHT:
        case 'D':
        case 'd':
        {
            TheUserInputData.Right = 0.0f;
            SetCameraDirty();
            break;
        }

        case 'Q':
        case 'q':
        {
            TheUserInputData.Down = 0.0f;
            SetCameraDirty();
            break;
        }

        case 'E':
        case 'e':
        {
            TheUserInputData.Up = 0.0f;
            SetCameraDirty();
            break;
        }

        case 'U':
        case 'u':
        {
            TheUserInputData.GpuUnfilteredComposite = !TheUserInputData.GpuUnfilteredComposite;
            UpdateWindowTitle();
            SetGpuOptionsDirty();
            break;
        }

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            SelectedBufferIndex = atoi((const char*)&key) - 1;
            UpdateWindowTitle();
            SetGpuOptionsDirty();
            break;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnMouseMove(int32_t x, int32_t y)
{
    if (TheUserInputData.LeftMouseButtonPressed)
    {
        TheUserInputData.MouseDx    = int(x) - TheUserInputData.PrevMouseX;
        TheUserInputData.MouseDy    = int(y) - TheUserInputData.PrevMouseY;
        TheUserInputData.PrevMouseX = x;
        TheUserInputData.PrevMouseY = y;
        SetCameraDirty();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnLeftButtonDown(int32_t x, int32_t y)
{
    TheUserInputData.LeftMouseButtonPressed = true;
    TheUserInputData.PrevMouseX             = x;
    TheUserInputData.PrevMouseY             = y;
    SetCameraDirty();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnLeftButtonUp(int32_t x, int32_t y)
{
    TheUserInputData.LeftMouseButtonPressed = false;
    TheUserInputData.PrevMouseX             = x;
    TheUserInputData.PrevMouseY             = y;
    SetCameraDirty();
}
