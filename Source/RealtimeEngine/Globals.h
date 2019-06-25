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

// Windows includes
#define NOMINMAX
#include <windows.h>
#include <wrl.h>
#include <shellapi.h>
#include <atlbase.h>
#include <comdef.h>

// DirectX includes
#include <DX12/dxgi.h>
#include <DX12/dxgi1_6.h>
#include <DX12/d3d12.h>
#include <DX12/d3dx12.h>
#include <DX12/dxgidebug.h>

// Standard includes
#include <stdint.h>
#include <string>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <cassert>

// Systems include
#include <Core/Systems.h>

// ----------------------------------------------------------------------------------------------------------------------------

typedef std::string  string_t;

// ----------------------------------------------------------------------------------------------------------------------------

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#define RTL_HRESULT_SUCCEEDED(hr, ...) \
    if (FAILED(hr)) \
    { \
        Core::RenderDebugPrintf("HRESULT: %#016x\n", hr); \
        __debugbreak(); \
    }

// ----------------------------------------------------------------------------------------------------------------------------

#define MAKE_SMART_COM_PTR(_a) _COM_SMARTPTR_TYPEDEF(_a, __uuidof(_a))
MAKE_SMART_COM_PTR(ID3D12Device5);
MAKE_SMART_COM_PTR(ID3D12GraphicsCommandList4);
MAKE_SMART_COM_PTR(ID3D12CommandQueue);
MAKE_SMART_COM_PTR(IDXGISwapChain3);
MAKE_SMART_COM_PTR(IDXGIFactory4);
MAKE_SMART_COM_PTR(IDXGIAdapter1);
MAKE_SMART_COM_PTR(ID3D12Fence);
MAKE_SMART_COM_PTR(ID3D12CommandAllocator);
MAKE_SMART_COM_PTR(ID3D12Resource);
MAKE_SMART_COM_PTR(ID3D12DescriptorHeap);
MAKE_SMART_COM_PTR(ID3D12Debug);
MAKE_SMART_COM_PTR(ID3D12StateObject);
MAKE_SMART_COM_PTR(ID3D12RootSignature);
MAKE_SMART_COM_PTR(ID3DBlob);

// ----------------------------------------------------------------------------------------------------------------------------

inline std::wstring MakeWStr(const std::string& str)
{
    return std::wstring(str.begin(), str.end());
}

// ----------------------------------------------------------------------------------------------------------------------------

class HrException : public std::runtime_error
{
public:

    HrException(HRESULT hr) 
        : std::runtime_error(HrToString(hr)), HResult(hr) {}

private:

    std::string HrToString(HRESULT hr)
    {
        char buf[64];
        sprintf_s(buf, "HRESULT of 0x%08X", static_cast<uint32_t>(hr));

        return std::string(buf);
    }

    const HRESULT HResult;
};

// ----------------------------------------------------------------------------------------------------------------------------

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

inline size_t HashRange(const uint32_t* const begin, const uint32_t* const end, size_t hash)
{
    for (const uint32_t* iter = begin; iter < end; ++iter)
    {
        hash = 16777619U * hash ^ *iter;
    }

    return hash;
}

template <typename T> inline size_t HashState(const T * stateDesc, size_t count = 1, size_t hash = 2166136261U)
{
    RTL_ASSERT_MSG((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");

    return HashRange((uint32_t*)stateDesc, (uint32_t*)(stateDesc + count), hash);
}

// ----------------------------------------------------------------------------------------------------------------------------

template <typename T> inline T     AlignUpWithMask(T value, size_t mask)       { return (T)(((size_t)value + mask) & ~mask); }
template <typename T> inline T     AlignDownWithMask(T value, size_t mask)     { return (T)((size_t)value & ~mask); }
template <typename T> inline T     AlignUp(T value, size_t alignment)          { return AlignUpWithMask(value, alignment - 1); }
template <typename T> inline T     AlignDown(T value, size_t alignment)        { return AlignDownWithMask(value, alignment - 1); }
template <typename T> inline bool  IsAligned(T value, size_t alignment)        { return 0 == ((size_t)value & (alignment - 1)); }
template <typename T> inline T     DivideByMultiple(T value, size_t alignment) { return (T)((value + alignment - 1) / alignment); }
template <typename T> inline bool  IsPowerOfTwo(T value)                       { return 0 == (value & (value - 1)); }
template <typename T> inline bool  IsDivisible(T value, T divisor)             { return (value / divisor) * divisor == value; }

// ----------------------------------------------------------------------------------------------------------------------------
