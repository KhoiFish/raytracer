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
            UserInput.InputDirty = true;
            break;
        }

        case VK_ESCAPE:
        {
            PostQuitMessage(0);
            break;
        }

        case VK_SHIFT:
        {
            UserInput.ShiftKeyPressed = true;
            break;
        }

        case VK_UP:
        case 'W':
        case 'w':
        {
            UserInput.Forward = 1.0f;
            UserInput.InputDirty = true;
            break;
        }

        case VK_DOWN:
        case 'S':
        case 's':
        {
            UserInput.Backward = 1.0f;
            UserInput.InputDirty = true;
            break;
        }

        case VK_LEFT:
        case 'A':
        case 'a':
        {
            UserInput.Left = 1.0f;
            UserInput.InputDirty = true;
            break;
        }

        case VK_RIGHT:
        case 'D':
        case 'd':
        {
            UserInput.Right = 1.0f;
            UserInput.InputDirty = true;
            break;
        }

        case 'Q':
        case 'q':
        {
            UserInput.Down = 1.0f;
            UserInput.InputDirty = true;
            break;
        }
        

        case 'E':
        case 'e':
        {
            UserInput.Up = 1.0f;
            UserInput.InputDirty = true;
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
            UserInput.ShiftKeyPressed = false;
            break;
        }

        case VK_UP:
        case 'W':
        case 'w':
        {
            UserInput.Forward = 0.0f;
            UserInput.InputDirty = true;
            break;
        }

        case VK_DOWN:
        case 'S':
        case 's':
        {
            UserInput.Backward = 0.0f;
            UserInput.InputDirty = true;
            break;
        }

        case VK_LEFT:
        case 'A':
        case 'a':
        {
            UserInput.Left = 0.0f;
            UserInput.InputDirty = true;
            break;
        }

        case VK_RIGHT:
        case 'D':
        case 'd':
        {
            UserInput.Right = 0.0f;
            UserInput.InputDirty = true;
            break;
        }

        case 'Q':
        case 'q':
        {
            UserInput.Down = 0.0f;
            UserInput.InputDirty = true;
            break;
        }

        case 'E':
        case 'e':
        {
            UserInput.Up = 0.0f;
            UserInput.InputDirty = true;
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
            break;
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool Renderer::OverrideImguiInput()
{
    return false;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnMouseMove(uint32_t x, uint32_t y)
{
    if (UserInput.LeftMouseButtonPressed)
    {
        UserInput.MouseDx    = int(x) - UserInput.PrevMouseX;
        UserInput.MouseDy    = int(y) - UserInput.PrevMouseY;
        UserInput.PrevMouseX = x;
        UserInput.PrevMouseY = y;
        UserInput.InputDirty = true;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnLeftButtonDown(uint32_t x, uint32_t y)
{
    UserInput.LeftMouseButtonPressed = true;
    UserInput.PrevMouseX             = x;
    UserInput.PrevMouseY             = y;
    UserInput.InputDirty             = true;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::OnLeftButtonUp(uint32_t x, uint32_t y)
{
    UserInput.LeftMouseButtonPressed = false;
    UserInput.PrevMouseX             = x;
    UserInput.PrevMouseY             = y;
    UserInput.InputDirty             = true;
}
