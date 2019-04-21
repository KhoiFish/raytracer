/*
  Copyright 2019 Khoi Nguyen

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
  files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

	 The above copyright notice and this permission notice shall be included in all copies or substantial
	 portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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

typedef std::string string_t;

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