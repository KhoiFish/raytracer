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
#include "RenderDevice.h"

namespace RealtimeEngine
{
    class RenderInterface : public IDeviceNotify
    {
    public:

		RenderInterface(const string_t& name, uint32_t width, uint32_t height) :
            Name(name),
            Width(width),
            Height(height),
            WindowBounds{ 0,0,0,0 },
            AspectRatio(0.0f)
        {
            UpdateForSizeChange(width, height);
        }

		virtual ~RenderInterface() {}

    public:

        virtual void                   OnInit() = 0;
        virtual void                   OnUpdate(float dtSeconds) = 0;
        virtual void                   OnRender() = 0;
        virtual void                   OnSizeChanged(uint32_t width, uint32_t height, bool minimized) = 0;
        virtual void                   OnDestroy() = 0;

    public:

        virtual void                   OnKeyDown(uint8_t key) {}
        virtual void                   OnKeyUp(uint8_t key) {}
        virtual void                   OnWindowMoved(int x, int y) {}
        virtual void                   OnMouseMove(int32_t x, int32_t y) {}
        virtual void                   OnLeftButtonDown(int32_t x, int32_t y) {}
        virtual void                   OnLeftButtonUp(int32_t x, int32_t y) {}
        virtual void                   OnDisplayChanged() {}
        virtual bool                   OverrideImguiInput() { return true; }

	public:

        const string_t&                GetName() const            { return Name; }
		uint32_t                       GetWidth() const           { return Width; }
		uint32_t                       GetHeight() const          { return Height; }
        RECT                           GetWindowsBounds() const   { return WindowBounds; }

    public:

        inline void UpdateForSizeChange(uint32_t clientWidth, uint32_t clientHeight)
        {
            Width       = clientWidth;
            Height      = clientHeight;
            AspectRatio = static_cast<float>(clientWidth) / static_cast<float>(clientHeight);
        }

        inline void SetWindowBounds(int32_t left, int32_t top, int32_t right, int32_t bottom)
        {
            WindowBounds.left   = static_cast<LONG>(left);
            WindowBounds.top    = static_cast<LONG>(top);
            WindowBounds.right  = static_cast<LONG>(right);
            WindowBounds.bottom = static_cast<LONG>(bottom);
        }

	protected:

        string_t    Name;
        uint32_t    Width;
        uint32_t    Height;
        float       AspectRatio;
        RECT        WindowBounds;
    };

}