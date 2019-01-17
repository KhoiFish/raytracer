#pragma once

/*
 *  Copyright(c) 2018 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

/**
 *  @file Material.h
 *  @date October 24, 2018
 *  @author Jeremiah van Oosten
 *
 *  @brief Material structure that uses HLSL constant buffer padding rules.
 */


#include <DirectXMath.h>

struct MaterialDX12
{
    MaterialDX12(
        DirectX::XMFLOAT4 emissive = { 0.0f, 0.0f, 0.0f, 1.0f },
        DirectX::XMFLOAT4 ambient = { 0.1f, 0.1f, 0.1f, 1.0f },
        DirectX::XMFLOAT4 diffuse = { 1.0f, 1.0f, 1.0f, 1.0f },
        DirectX::XMFLOAT4 specular = { 1.0f, 1.0f, 1.0f, 1.0f },
        float specularPower = 128.0f
    )
        : Emissive( emissive )
        , Ambient( ambient )
        , Diffuse( diffuse )
        , Specular( specular )
        , SpecularPower( specularPower ) 
    {}

    DirectX::XMFLOAT4 Emissive;
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 Ambient;
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 Diffuse;
    //----------------------------------- (16 byte boundary)
    DirectX::XMFLOAT4 Specular;
    //----------------------------------- (16 byte boundary)
    float             SpecularPower;
    uint32_t          Padding[3];
    //----------------------------------- (16 byte boundary)
    // Total:                              16 * 5 = 80 bytes
    
    // Define some interesting materials.
    static const MaterialDX12 Red;
    static const MaterialDX12 Green;
    static const MaterialDX12 Blue;
    static const MaterialDX12 Cyan;
    static const MaterialDX12 Magenta;
    static const MaterialDX12 Yellow;
    static const MaterialDX12 White;
    static const MaterialDX12 Black;
    static const MaterialDX12 Emerald;
    static const MaterialDX12 Jade;
    static const MaterialDX12 Obsidian;
    static const MaterialDX12 Pearl;
    static const MaterialDX12 Ruby;
    static const MaterialDX12 Turquoise;
    static const MaterialDX12 Brass;
    static const MaterialDX12 Bronze;
    static const MaterialDX12 Chrome;
    static const MaterialDX12 Copper;
    static const MaterialDX12 Gold;
    static const MaterialDX12 Silver;
    static const MaterialDX12 BlackPlastic;
    static const MaterialDX12 CyanPlastic;
    static const MaterialDX12 GreenPlastic;
    static const MaterialDX12 RedPlastic;
    static const MaterialDX12 WhitePlastic;
    static const MaterialDX12 YellowPlastic;
    static const MaterialDX12 BlackRubber;
    static const MaterialDX12 CyanRubber;
    static const MaterialDX12 GreenRubber;
    static const MaterialDX12 RedRubber;
    static const MaterialDX12 WhiteRubber;
    static const MaterialDX12 YellowRubber;
};