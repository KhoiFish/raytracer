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
#define NOMINMAX
#include <windows.h>
#include <wrl.h>
#include <shellapi.h>
#include <atlbase.h>

#include <dxgi.h>
#include <dxgi1_6.h>
#include <DX12/d3d12.h>
#include <DX12/d3dx12.h>
#include <dxgidebug.h>


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


#include <Core/Systems.h>

// ----------------------------------------------------------------------------------------------------------------------------

typedef std::string                                    string_t;

// ----------------------------------------------------------------------------------------------------------------------------

class HrException : public std::runtime_error
{
    inline std::string HrToString(HRESULT hr)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<uint32_t>(hr));
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


inline void Print(const char* msg) { printf("%s", msg); }
inline void Print(const wchar_t* msg) { wprintf(L"%ws", msg); }

inline void GlobalPrint(const char* format, ...)
{
    char buffer[256];
    va_list ap;
    va_start(ap, format);
    vsprintf_s(buffer, 256, format, ap);
    Print(buffer);
}

inline void GlobalPrint(const wchar_t* format, ...)
{
    wchar_t buffer[256];
    va_list ap;
    va_start(ap, format);
    vswprintf(buffer, 256, format, ap);
    Print(buffer);
}

inline void GlobalPrintSubMessage(const char* format, ...)
{
    Print("--> ");
    char buffer[256];
    va_list ap;
    va_start(ap, format);
    vsprintf_s(buffer, 256, format, ap);
    Print(buffer);
    Print("\n");
}
inline void GlobalPrintSubMessage(const wchar_t* format, ...)
{
    Print("--> ");
    wchar_t buffer[256];
    va_list ap;
    va_start(ap, format);
    vswprintf(buffer, 256, format, ap);
    Print(buffer);
    Print("\n");
}



#define WARN_ONCE_IF( isTrue, ... ) (void)(isTrue)
#define WARN_ONCE_IF_NOT( isTrue, ... ) (void)(isTrue)
#define DEBUGPRINT( msg, ... ) do {} while(0)

#define STRINGIFY(x) #x
#define STRINGIFY_BUILTIN(x) STRINGIFY(x)
#define ASSERT( isFalse, ... ) \
        if (!(bool)(isFalse)) { \
            __debugbreak(); \
        }

#define ASSERT_SUCCEEDED( hr, ... ) \
        if (FAILED(hr)) { \
            __debugbreak(); \
        }


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

void SIMDMemCopy(void* __restrict Dest, const void* __restrict Source, size_t NumQuadwords);
void SIMDMemFill(void* __restrict Dest, __m128 FillVector, size_t NumQuadwords);