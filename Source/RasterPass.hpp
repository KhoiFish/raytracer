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

#include "RaytracingShaderInclude.h"

// ----------------------------------------------------------------------------------------------------------------------------

namespace RasterRenderRootSig
{
    enum EnumTypes
    {
        SceneCB = 0,
        InstanceCB,
        MaterialCB,
        CompositeCB,

        DirectLightResult,
        DirectLightAlbedo,
        IndirectLightResult,
        IndirectLightAlbedo,
        CpuResultsTex,
        PrevResultsTex,
        PositionsTex,
        NormalsTex,
        TexCoordsTex,
        DiffuseTex,
        CurrSVGFLinearZTex,
        PrevSVGFLinearZTex,
        SVGFMoVecTex,
        SVGFCompactTex,

        Num,
    };

    enum IndexTypes
    {
        Register = 0,
        Count,
        Space,
        RangeType,
    };

    // [register, count, space, D3D12_DESCRIPTOR_RANGE_TYPE]
    static UINT Range[RasterRenderRootSig::Num][4] =
    {
        { 0,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // SceneCB
        { 1,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // InstanceCB
        { 2,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // MaterialCB
        { 3,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_CBV },   // CompositeCB

        { 0,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // DirectLightResult
        { 1,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // DirectLightAlbedo
        { 2,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // IndirectLightResult
        { 3,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // IndirectLightAlbedo
        { 4,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // CpuResultsTex   
        { 5,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // PrevResultsTex
        { 6,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // PositionsTex
        { 7,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // NormalsTex  
        { 8,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // TexCoordsTex
        { 9,  1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // DiffuseTex  
        { 10, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // CurrSVGFLinearZTex
        { 11, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // PrevSVGFLinearZTex
        { 12, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // SVGFMoVecTex
        { 13, 1, 0, D3D12_DESCRIPTOR_RANGE_TYPE_SRV },   // SVGFCompactTex
    };
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_SAMPLER_DESC Renderer::GetLinearSamplerDesc()
{
    D3D12_SAMPLER_DESC linearSamplerDesc;
    linearSamplerDesc.Filter            = D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    linearSamplerDesc.AddressU          = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearSamplerDesc.AddressV          = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearSamplerDesc.AddressW          = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearSamplerDesc.MipLODBias        = 0.0f;
    linearSamplerDesc.MaxAnisotropy     = 8;
    linearSamplerDesc.ComparisonFunc    = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    linearSamplerDesc.BorderColor[0]    = 1.0f;
    linearSamplerDesc.BorderColor[1]    = 1.0f;
    linearSamplerDesc.BorderColor[2]    = 1.0f;
    linearSamplerDesc.BorderColor[3]    = 1.0f;
    linearSamplerDesc.MinLOD            = 0.0f;
    linearSamplerDesc.MaxLOD            = D3D12_FLOAT32_MAX;

    return linearSamplerDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

 D3D12_SAMPLER_DESC Renderer::GetAnisoSamplerDesc()
{
    D3D12_SAMPLER_DESC anisoSamplerDesc;
    anisoSamplerDesc.Filter             = D3D12_FILTER_ANISOTROPIC;
    anisoSamplerDesc.AddressU           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    anisoSamplerDesc.AddressV           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    anisoSamplerDesc.AddressW           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    anisoSamplerDesc.MipLODBias         = 0.0f;
    anisoSamplerDesc.MaxAnisotropy      = 8;
    anisoSamplerDesc.ComparisonFunc     = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    anisoSamplerDesc.BorderColor[0]     = 1.0f;
    anisoSamplerDesc.BorderColor[1]     = 1.0f;
    anisoSamplerDesc.BorderColor[2]     = 1.0f;
    anisoSamplerDesc.BorderColor[3]     = 1.0f;
    anisoSamplerDesc.MinLOD             = 0.0f;
    anisoSamplerDesc.MaxLOD             = D3D12_FLOAT32_MAX;

    return anisoSamplerDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_RASTERIZER_DESC Renderer::GetDefaultRasterState(bool frontCounterClockwise)
{
    D3D12_RASTERIZER_DESC rasterizerDesc;
    rasterizerDesc.FillMode                 = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode                 = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise    = frontCounterClockwise ? TRUE : FALSE;
    rasterizerDesc.DepthBias                = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp           = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias     = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable          = TRUE;
    rasterizerDesc.MultisampleEnable        = FALSE;
    rasterizerDesc.AntialiasedLineEnable    = FALSE;
    rasterizerDesc.ForcedSampleCount        = 0;
    rasterizerDesc.ConservativeRaster       = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    return rasterizerDesc;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_BLEND_DESC Renderer::GetBlendDisabledState()
{
    D3D12_BLEND_DESC blendDisable = {};
    blendDisable.IndependentBlendEnable                 = FALSE;
    blendDisable.RenderTarget[0].BlendEnable            = FALSE;
    blendDisable.RenderTarget[0].SrcBlend               = D3D12_BLEND_SRC_ALPHA;
    blendDisable.RenderTarget[0].DestBlend              = D3D12_BLEND_INV_SRC_ALPHA;
    blendDisable.RenderTarget[0].BlendOp                = D3D12_BLEND_OP_ADD;
    blendDisable.RenderTarget[0].SrcBlendAlpha          = D3D12_BLEND_ONE;
    blendDisable.RenderTarget[0].DestBlendAlpha         = D3D12_BLEND_INV_SRC_ALPHA;
    blendDisable.RenderTarget[0].BlendOpAlpha           = D3D12_BLEND_OP_ADD;
    blendDisable.RenderTarget[0].RenderTargetWriteMask  = 0;
    blendDisable.RenderTarget[0].RenderTargetWriteMask  = D3D12_COLOR_WRITE_ENABLE_ALL;

    return blendDisable;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_DEPTH_STENCIL_DESC Renderer::GetDepthEnabledState()
{
    D3D12_DEPTH_STENCIL_DESC depthStateReadWrite;
    depthStateReadWrite.DepthEnable                     = TRUE;
    depthStateReadWrite.DepthWriteMask                  = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStateReadWrite.DepthFunc                       = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthStateReadWrite.StencilEnable                   = FALSE;
    depthStateReadWrite.StencilReadMask                 = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStateReadWrite.StencilWriteMask                = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStateReadWrite.FrontFace.StencilFunc           = D3D12_COMPARISON_FUNC_ALWAYS;
    depthStateReadWrite.FrontFace.StencilPassOp         = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.FrontFace.StencilFailOp         = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.FrontFace.StencilDepthFailOp    = D3D12_STENCIL_OP_KEEP;
    depthStateReadWrite.BackFace                        = depthStateReadWrite.FrontFace;

    return depthStateReadWrite;
}

// ----------------------------------------------------------------------------------------------------------------------------

D3D12_DEPTH_STENCIL_DESC Renderer::GetDepthDisabledState()
{
    D3D12_DEPTH_STENCIL_DESC depthStateDisabled = GetDepthEnabledState();
    depthStateDisabled.DepthEnable     = FALSE;
    depthStateDisabled.DepthWriteMask  = D3D12_DEPTH_WRITE_MASK_ZERO;

    return depthStateDisabled;
}

// ----------------------------------------------------------------------------------------------------------------------------

const char* Renderer::GetSelectedBufferName()
{
    static const char* bufferNames[] =
    {
        "Composite Output",
        "Direct Lighting",
        "Indirect Lighting",
        "Cpu Raytrace",
        "Positions",
        "Normals",
        "TexCoords And Depth",
        "Diffuse",
    };

    if (SelectedBufferIndex >= ARRAY_SIZE(bufferNames))
    {
        return "Unknown";
    }
    else
    {
        return bufferNames[SelectedBufferIndex];
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::CleanupRasterPass()
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRasterRootSignatures()
{
    // Refer to composite shader and match this to the TextureMultipliers[] index
    CpuResultsBufferIndex = 3;

    // Constant buffers
    RasterSceneConstantBuffer.Create(L"Raster Scene Constant Buffer", 1, (uint32_t)AlignUp(sizeof(SceneConstantBuffer), 256));
    RasterCompositeConstantBuffer.Create(L"Raster Composite Constant Buffer", 1, (uint32_t)AlignUp(sizeof(CompositeConstantBuffer), 256));

    // Root sig setup
    {
        const int numSamplers = 2;
        RasterRootSignature.Reset(RasterRenderRootSig::Num, numSamplers);
        {
            for (int i = 0; i < RasterRenderRootSig::Num; i++)
            {
                RasterRootSignature[i].InitAsDescriptorRange(
                    (D3D12_DESCRIPTOR_RANGE_TYPE)RasterRenderRootSig::Range[i][RasterRenderRootSig::RangeType],
                    RasterRenderRootSig::Range[i][RasterRenderRootSig::Register],
                    RasterRenderRootSig::Range[i][RasterRenderRootSig::Count],
                    RasterRenderRootSig::Range[i][RasterRenderRootSig::Space],
                    D3D12_SHADER_VISIBILITY_ALL);
            }

            RasterRootSignature.InitStaticSampler(0, GetLinearSamplerDesc(), D3D12_SHADER_VISIBILITY_PIXEL);
            RasterRootSignature.InitStaticSampler(1, GetAnisoSamplerDesc(), D3D12_SHADER_VISIBILITY_PIXEL);
        }
        RasterRootSignature.Finalize("RasterRootSignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    // Geometry pass PSO setup
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\GeometryPass_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\GeometryPass_PS.cso", &pixelShaderBlob));

        RasterGeometryPassPSO.SetRootSignature(RasterRootSignature);
        RasterGeometryPassPSO.SetRasterizerState(GetDefaultRasterState());
        RasterGeometryPassPSO.SetBlendState(GetBlendDisabledState());
        RasterGeometryPassPSO.SetDepthStencilState(GetDepthEnabledState());
        RasterGeometryPassPSO.SetInputLayout(RealtimeSceneVertexEx::InputElementCount, RealtimeSceneVertexEx::InputElements);
        RasterGeometryPassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RasterGeometryPassPSO.SetRenderTargetFormats(_countof(GBufferRTTypes), GBufferRTTypes, RenderDevice::Get().GetDepthBufferFormat());
        RasterGeometryPassPSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RasterGeometryPassPSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RasterGeometryPassPSO.Finalize();
    }

    // Composite pass PSO setup
    {
        Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\CompositePass_VS.cso", &vertexShaderBlob));
        ThrowIfFailed(D3DReadFileToBlob(SHADERBUILD_DIR L"\\CompositePass_PS.cso", &pixelShaderBlob));

        DXGI_FORMAT rtFormats[] { BackbufferFormat, CompositeOutputBufferRTType };

        RasterCompositePassPSO.SetRootSignature(RasterRootSignature);
        RasterCompositePassPSO.SetRasterizerState(GetDefaultRasterState(true));
        RasterCompositePassPSO.SetBlendState(GetBlendDisabledState());
        RasterCompositePassPSO.SetDepthStencilState(GetDepthDisabledState());
        RasterCompositePassPSO.SetInputLayout(RealtimeSceneVertexEx::InputElementCount, RealtimeSceneVertexEx::InputElements);
        RasterCompositePassPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        RasterCompositePassPSO.SetRenderTargetFormats(_countof(rtFormats), rtFormats, RenderDevice::Get().GetDepthBufferFormat());
        RasterCompositePassPSO.SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
        RasterCompositePassPSO.SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
        RasterCompositePassPSO.Finalize();
    }    
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupRasterDescriptors()
{
    RasterDescriptorIndexStart = RendererDescriptorHeap->GetCount();

    RendererDescriptorHeap->AllocateBufferCbv(
        RasterSceneConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RasterSceneConstantBuffer.GetBufferSize());

    // Hack, this a placeholder for instance CB
    RendererDescriptorHeap->AllocateBufferCbv(
        RasterSceneConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RasterSceneConstantBuffer.GetBufferSize());

    // Hack, this a placeholder for material CB
    RendererDescriptorHeap->AllocateBufferCbv(
        RasterSceneConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RasterSceneConstantBuffer.GetBufferSize());

    RendererDescriptorHeap->AllocateBufferCbv(
        RasterCompositeConstantBuffer.GetGpuVirtualAddress(),
        (UINT)RasterCompositeConstantBuffer.GetBufferSize());

    RendererDescriptorHeap->AllocateTexture2DSrv(
        DirectLightingBuffer[DirectIndirectBufferType_Results].GetResource(),
        DirectIndirectRTBufferType);

    RendererDescriptorHeap->AllocateTexture2DSrv(
        DirectLightingBuffer[DirectIndirectBufferType_Albedo].GetResource(),
        DirectIndirectRTBufferType);

    RendererDescriptorHeap->AllocateTexture2DSrv(
        IndirectLightingBuffer[DirectIndirectBufferType_Results].GetResource(),
        DirectIndirectRTBufferType);

    RendererDescriptorHeap->AllocateTexture2DSrv(
        IndirectLightingBuffer[DirectIndirectBufferType_Albedo].GetResource(),
        DirectIndirectRTBufferType);

    RendererDescriptorHeap->AllocateTexture2DSrv(
        CPURaytracerTex.GetResource(),
        CPURaytracerTexType);

    RendererDescriptorHeap->AllocateTexture2DSrv(
        CompositeOutputBuffers[0].GetResource(),
        CompositeOutputBufferRTType);

    for (int i = 0; i < GBufferType_Num; i++)
    {
        RendererDescriptorHeap->AllocateTexture2DSrv(
            GBuffers[i].GetResource(),
            GBufferRTTypes[i]);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::SetupSceneConstantBuffer(SceneConstantBuffer& sceneCB)
{
    RealtimeCamera& camera             = TheRenderScene->GetCamera();
    XMMATRIX        currViewProjection = XMMatrixTranspose(camera.GetViewMatrix() * camera.GetProjectionMatrix());
    static XMMATRIX prevViewProjection = currViewProjection;

    sceneCB.FarClipDist  = camera.GetZFar();
    sceneCB.OutputSize   = XMFLOAT4(float(Width), float(Height), 1.0f / float(Width), 1.0f / float(Height));
    sceneCB.CameraJitter = camera.GetJitter();
    XMStoreFloat4(&sceneCB.CameraPosition, camera.GetEye());
    XMStoreFloat4x4(&sceneCB.ViewMatrix, XMMatrixTranspose(camera.GetViewMatrix()));
    XMStoreFloat4x4(&sceneCB.ProjectionMatrix, XMMatrixTranspose(camera.GetProjectionMatrix()));
    XMStoreFloat4x4(&sceneCB.ViewProjectionMatrix, currViewProjection);
    XMStoreFloat4x4(&sceneCB.PrevViewProjectionMatrix, prevViewProjection);
    XMStoreFloat4x4(&sceneCB.InverseViewProjectionMatrix, XMMatrixTranspose(XMMatrixInverse(nullptr, camera.GetViewMatrix() * camera.GetProjectionMatrix())));
    XMStoreFloat4x4(&sceneCB.InverseTransposeViewProjectionMatrix, XMMatrixInverse(nullptr, camera.GetViewMatrix() * camera.GetProjectionMatrix()));

    // Update previous
    prevViewProjection = currViewProjection;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderSceneList(GraphicsContext& renderContext)
{
    const UINT diffuseTextureHeapIndexStart = TheRenderScene->GetDiffuseTextureList().DescriptorHeapStartIndex;
    for (int i = 0; i < TheRenderScene->GetRenderSceneList().size(); i++)
    {
        RealtimeSceneNode*  pNode                   = TheRenderScene->GetRenderSceneList()[i];
        UINT                diffuseTextureHeapIndex = diffuseTextureHeapIndexStart + TheRenderScene->GetRenderSceneList()[i]->DiffuseTextureIndex;

        renderContext.SetDescriptorTable(RasterRenderRootSig::InstanceCB, RendererDescriptorHeap->GetGpuHandle(pNode->InstanceDataHeapIndex));
        renderContext.SetDescriptorTable(RasterRenderRootSig::MaterialCB, RendererDescriptorHeap->GetGpuHandle(pNode->MaterialHeapIndex));
        renderContext.SetDescriptorTable(RasterRenderRootSig::DiffuseTex, RendererDescriptorHeap->GetGpuHandle(diffuseTextureHeapIndex));
        renderContext.SetVertexBuffer(0, pNode->VertexBuffer.VertexBufferView());
        renderContext.SetIndexBuffer(pNode->IndexBuffer.IndexBufferView());
        renderContext.DrawIndexed((uint32_t)pNode->Indices.size());
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderGeometryPass()
{
    // Jitter camera
    {
        if (TheUserInputData.GpuCameraJitter)
        {
            // Half pixel offset
            float xOff = (float(RandDist(RandGen)) - 0.5f);
            float yOff = (float(RandDist(RandGen)) - 0.5f);
            TheRenderScene->GetCamera().SetJitter(xOff / float(Width), yOff / float(Height));
        }
        else
        {
            TheRenderScene->GetCamera().SetJitter(0, 0);
        }
    }

    GraphicsContext& renderContext = GraphicsContext::Begin("Geometry Pass");
    {
        renderContext.SetRootSignature(RasterRootSignature);
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());

        // Geometry pass
        {
            // Udpate all the gpu buffers in the render scene
            TheRenderScene->UpdateAllGpuBuffers(renderContext);

            // Update scene constant buffer
            SceneConstantBuffer sceneCB;
            SetupSceneConstantBuffer(sceneCB);
            RasterSceneConstantBuffer.Upload(&sceneCB, sizeof(sceneCB));

            // Transition resources
            for (int i = 0; i < GBufferType_Num; i++)
            {
                renderContext.TransitionResource(GBuffers[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
            }
            renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
            
            // Bind gbuffer
            D3D12_CPU_DESCRIPTOR_HANDLE rtvs[GBufferType_Num];
            for (int i = 0; i < GBufferType_Num; i++)
            {
                rtvs[i] = GBuffers[i].GetRTV();
            }
            renderContext.SetRenderTargets(ARRAYSIZE(rtvs), rtvs, RenderDevice::Get().GetDepthStencil().GetDSV());

            renderContext.ClearColor(GBuffers[GBufferType_Position]);
            renderContext.ClearColor(GBuffers[GBufferType_Normal]);
            renderContext.ClearColor(GBuffers[GBufferType_TexCoordAndDepth]);
            renderContext.ClearColor(GBuffers[GBufferType_Albedo]);

            // Set descriptor heaps and tables
            renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RendererDescriptorHeap->GetDescriptorHeap());
            for (uint32_t i = 0; i < RasterRenderRootSig::Num; i++)
            {
                uint32_t heapIndex = (RasterDescriptorIndexStart + i);
                renderContext.SetDescriptorTable(i, RendererDescriptorHeap->GetGpuHandle(heapIndex));
            }

            renderContext.ClearDepth(RenderDevice::Get().GetDepthStencil());
            renderContext.SetPipelineState(RasterGeometryPassPSO);
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            renderContext.FlushResourceBarriers();
            RenderSceneList(renderContext);
        }
    }
    renderContext.Finish();
}

// ----------------------------------------------------------------------------------------------------------------------------

void Renderer::RenderCompositePass()
{
    GraphicsContext& renderContext = GraphicsContext::Begin("Composite Pass");
    {
        renderContext.SetRootSignature(RasterRootSignature);
        renderContext.SetViewport(RenderDevice::Get().GetScreenViewport());
        renderContext.SetScissor(RenderDevice::Get().GetScissorRect());

        // Composite pass
        {
            // Update GPU texture with the cpu traced results
            if (TheRaytracer != nullptr)
            {
                D3D12_SUBRESOURCE_DATA subresource;
                subresource.RowPitch    = (sizeof(float) * 4) * TheRaytracer->GetOutputWidth();
                subresource.SlicePitch  = subresource.RowPitch * TheRaytracer->GetOutputHeight();
                subresource.pData       =  TheRaytracer->GetOutputBuffer();
                renderContext.InitializeTexture(CPURaytracerTex, 1, &subresource);
            }

            // Setup constants
            CompositeConstantBuffer compositeCB;
            {
                const XMFLOAT4 bufferOn  = XMFLOAT4(1, 1, 1, 1);
                const XMFLOAT4 bufferOff = XMFLOAT4(0, 0, 0, 0);

                // The following multipliers let us select which buffer to display
                // See the CompositePass shader for more details.
                bool enableToneMapping = TheUserInputData.GpuEnableToneMapping;
                for (int i = 0; i < ARRAY_SIZE(compositeCB.TextureMultipliers); i++)
                {
                    if (SelectedBufferIndex == i || SelectedBufferIndex == 0)
                    {
                        compositeCB.TextureMultipliers[i] = bufferOn;
                    }
                    else
                    {
                        compositeCB.TextureMultipliers[i] = bufferOff;
                    }
                }

                if (SelectedBufferIndex == 0)
                {
                    compositeCB.CompositeMultipliers[0] = bufferOn;
                    compositeCB.CompositeMultipliers[1] = bufferOff;
                }
                else
                {
                    compositeCB.CompositeMultipliers[0] = bufferOff;
                    compositeCB.CompositeMultipliers[1] = bufferOn;
                    enableToneMapping                   = (SelectedBufferIndex == CpuResultsBufferIndex) ? true : false;
                }

                // Setup direct/indirect multipliers
                compositeCB.DirectIndirectLightMult = XMFLOAT2(TheUserInputData.GpuDirectLightMult, TheUserInputData.GpuIndirectLightMult);

                // Cpu completed samples
                int numSamples = (TheRaytracer != nullptr) ? (TheRaytracer->GetStats().CompletedSampleCount + 1) : 1;
                compositeCB.CpuNormalizeFactor = 1.0f / float(numSamples);

                // Tone mapping
                compositeCB.CompositeMultipliers[2] = enableToneMapping ? bufferOn : bufferOff;

                // Accum count
                compositeCB.AccumCount = float(AccumCount);
            }
            RasterCompositeConstantBuffer.Upload(&compositeCB, sizeof(compositeCB));

            // Transition resources
            renderContext.TransitionResource(CPURaytracerTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            renderContext.TransitionResource(CompositeOutputBuffers[0], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            renderContext.TransitionResource(DirectLightingBuffer[DirectIndirectBufferType_Results], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            renderContext.TransitionResource(DirectLightingBuffer[DirectIndirectBufferType_Albedo], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            renderContext.TransitionResource(IndirectLightingBuffer[DirectIndirectBufferType_Results], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            renderContext.TransitionResource(IndirectLightingBuffer[DirectIndirectBufferType_Albedo], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            for (int i = 0; i < GBufferType_Num; i++)
            {
                renderContext.TransitionResource(GBuffers[i], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }
            renderContext.TransitionResource(RenderDevice::Get().GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET);
            renderContext.TransitionResource(CompositeOutputBuffers[1], D3D12_RESOURCE_STATE_RENDER_TARGET);
            renderContext.TransitionResource(RenderDevice::Get().GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_READ);

            // Set descriptor heaps and tables
            renderContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RendererDescriptorHeap->GetDescriptorHeap());
            for (uint32_t i = 0; i < RasterRenderRootSig::Num; i++)
            {
                uint32_t heapIndex = (RasterDescriptorIndexStart + i);
                renderContext.SetDescriptorTable(i, RendererDescriptorHeap->GetGpuHandle(heapIndex));
            }

            // Set direct and indirect textures to the denoised output
            renderContext.SetDescriptorTable(RasterRenderRootSig::DirectLightResult, DenoiseDirectOutputDescriptor.GePreviousGpuDescriptor());
            renderContext.SetDescriptorTable(RasterRenderRootSig::IndirectLightResult, DenoiseIndirectOutputDescriptor.GePreviousGpuDescriptor());

            // Bind render targets
            D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2] =
            {
                RenderDevice::Get().GetRenderTarget().GetRTV(),
                CompositeOutputBuffers[1].GetRTV()
            };
            renderContext.SetRenderTargets(ARRAYSIZE(rtvs), rtvs, RenderDevice::Get().GetDepthStencil().GetDSV());

            // Setup rest of the pipeline
            renderContext.SetPipelineState(RasterCompositePassPSO);
            renderContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            renderContext.FlushResourceBarriers();

            // Draw
            renderContext.SetNullVertexBuffer(0);
            renderContext.SetNullIndexBuffer();
            renderContext.Draw(3);

            // Update previous
            renderContext.CopyBuffer(CompositeOutputBuffers[0], CompositeOutputBuffers[1]);
        }
    }
    renderContext.Finish();
}
