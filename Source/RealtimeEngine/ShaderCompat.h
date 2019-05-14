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

#ifndef SHADERCOMPAT_H
#define SHADERCOMPAT_H

// ----------------------------------------------------------------------------------------------------------------------------

#ifdef HLSL
    typedef float2      XMFLOAT2;
    typedef float3      XMFLOAT3;
    typedef float4      XMFLOAT4;
    typedef float4      XMVECTOR;
    typedef float4x4    XMMATRIX;
    typedef float4x4    XMFLOAT4X4;
    typedef uint        UINT;

    #define ALIGN_BEGIN(alignValue)
    #define ALIGN_END
#else
    #include <wrl.h>
    #include <d3dcompiler.h>
    #include <DirectXColors.h>
    #include <stdint.h>

    #define ALIGN_BEGIN(alignValue) __pragma(pack(push, alignValue))
    #define ALIGN_END               __pragma(pack(pop))

    using namespace DirectX;
#endif

#endif