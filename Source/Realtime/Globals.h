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


// ----------------------------------------------------------------------------------------------------------------------------
// Windows includes
// ----------------------------------------------------------------------------------------------------------------------------
#include <windows.h>
#include <wrl.h>
#include <shellapi.h>
#include <atlbase.h>

#include <dxgi.h>
#include <dxgi1_6.h>
#include "d3d12.h"
#include "d3dx12.h"

#ifdef _DEBUG
#include <dxgidebug.h>
#endif


// ----------------------------------------------------------------------------------------------------------------------------
// Standard includes
// ----------------------------------------------------------------------------------------------------------------------------
#include <stdint.h>
#include <string>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <cassert>


// ----------------------------------------------------------------------------------------------------------------------------

typedef Microsoft::WRL::ComPtr<ID3D12PipelineState>    GfxPipeState;
typedef Microsoft::WRL::ComPtr<ID3D12RootSignature>    GfxRootSignature;
typedef Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>   GfxDescHeap;
typedef Microsoft::WRL::ComPtr<ID3D12Resource>         GfxResource;
typedef D3D12_VIEWPORT                                 GfxViewport;
typedef D3D12_RECT                                     GfxRect;
typedef IDXGISwapChain                                 GfxSwapChain;

typedef std::string                                    string_t;

// ----------------------------------------------------------------------------------------------------------------------------

class HrException : public std::runtime_error
{
    inline std::string HrToString(HRESULT hr)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
        return std::string(s_str);
    }
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

inline void ThrowIfFailed(HRESULT hr, const wchar_t* msg)
{
    if (FAILED(hr))
    {
        OutputDebugString(msg);
        throw HrException(hr);
    }
}

inline void ThrowIfFalse(bool value)
{
    ThrowIfFailed(value ? S_OK : E_FAIL);
}

inline void ThrowIfFalse(bool value, const wchar_t* msg)
{
    ThrowIfFailed(value ? S_OK : E_FAIL, msg);
}


#define ASSERT( isTrue, ... ) (void)(isTrue)
#define WARN_ONCE_IF( isTrue, ... ) (void)(isTrue)
#define WARN_ONCE_IF_NOT( isTrue, ... ) (void)(isTrue)
#define DEBUGPRINT( msg, ... ) do {} while(0)
#define ASSERT_SUCCEEDED( hr, ... ) (void)(hr)


inline size_t HashRange(const uint32_t* const Begin, const uint32_t* const End, size_t Hash)
{
    // An inexpensive hash for CPUs lacking SSE4.2
    for (const uint32_t* Iter = Begin; Iter < End; ++Iter)
        Hash = 16777619U * Hash ^ *Iter;

    return Hash;
}

template <typename T> inline size_t HashState(const T * StateDesc, size_t Count = 1, size_t Hash = 2166136261U)
{
    static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
    return HashRange((uint32_t*)StateDesc, (uint32_t*)(StateDesc + Count), Hash);
}

inline std::wstring MakeWStr(const std::string& str)
{
    return std::wstring(str.begin(), str.end());
}


struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};


template <typename T> __forceinline T AlignUpWithMask(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T> __forceinline T AlignDownWithMask(T value, size_t mask)
{
    return (T)((size_t)value & ~mask);
}

template <typename T> __forceinline T AlignUp(T value, size_t alignment)
{
    return AlignUpWithMask(value, alignment - 1);
}

template <typename T> __forceinline T AlignDown(T value, size_t alignment)
{
    return AlignDownWithMask(value, alignment - 1);
}

template <typename T> __forceinline bool IsAligned(T value, size_t alignment)
{
    return 0 == ((size_t)value & (alignment - 1));
}

template <typename T> __forceinline T DivideByMultiple(T value, size_t alignment)
{
    return (T)((value + alignment - 1) / alignment);
}

template <typename T> __forceinline bool IsPowerOfTwo(T value)
{
    return 0 == (value & (value - 1));
}

template <typename T> __forceinline bool IsDivisible(T value, T divisor)
{
    return (value / divisor) * divisor == value;
}