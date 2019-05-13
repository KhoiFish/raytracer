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

#if 0
struct RaytracingDispatchRayInputs
{
    RaytracingDispatchRayInputs() {}
    RaytracingDispatchRayInputs(
        ID3D12RaytracingFallbackDevice& device,
        ID3D12RaytracingFallbackStateObject* pPSO,
        void* pHitGroupShaderTable,
        UINT HitGroupStride,
        UINT HitGroupTableSize,
        LPCWSTR rayGenExportName,
        LPCWSTR missExportName) : m_pPSO(pPSO)
    {
        const UINT shaderTableSize = device.GetShaderIdentifierSize();
        void* pRayGenShaderData = pPSO->GetShaderIdentifier(rayGenExportName);
        void* pMissShaderData = pPSO->GetShaderIdentifier(missExportName);

        m_HitGroupStride = HitGroupStride;

        // MiniEngine requires that all initial data be aligned to 16 bytes
        UINT alignment = 16;
        std::vector<BYTE> alignedShaderTableData(shaderTableSize + alignment - 1);
        BYTE * pAlignedShaderTableData = alignedShaderTableData.data() + ((UINT64)alignedShaderTableData.data() % alignment);
        memcpy(pAlignedShaderTableData, pRayGenShaderData, shaderTableSize);
        m_RayGenShaderTable.Create(L"Ray Gen Shader Table", 1, shaderTableSize, alignedShaderTableData.data());

        memcpy(pAlignedShaderTableData, pMissShaderData, shaderTableSize);
        m_MissShaderTable.Create(L"Miss Shader Table", 1, shaderTableSize, alignedShaderTableData.data());

        m_HitShaderTable.Create(L"Hit Shader Table", 1, HitGroupTableSize, pHitGroupShaderTable);
    }

    D3D12_DISPATCH_RAYS_DESC GetDispatchRayDesc(UINT DispatchWidth, UINT DispatchHeight)
    {
        D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};

        dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = m_RayGenShaderTable.GetGpuVirtualAddress();
        dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = m_RayGenShaderTable.GetBufferSize();
        dispatchRaysDesc.HitGroupTable.StartAddress = m_HitShaderTable.GetGpuVirtualAddress();
        dispatchRaysDesc.HitGroupTable.SizeInBytes = m_HitShaderTable.GetBufferSize();
        dispatchRaysDesc.HitGroupTable.StrideInBytes = m_HitGroupStride;
        dispatchRaysDesc.MissShaderTable.StartAddress = m_MissShaderTable.GetGpuVirtualAddress();
        dispatchRaysDesc.MissShaderTable.SizeInBytes = m_MissShaderTable.GetBufferSize();
        dispatchRaysDesc.MissShaderTable.StrideInBytes = dispatchRaysDesc.MissShaderTable.SizeInBytes; // Only one entry
        dispatchRaysDesc.Width = DispatchWidth;
        dispatchRaysDesc.Height = DispatchHeight;
        dispatchRaysDesc.Depth = 1;
        return dispatchRaysDesc;
    }

    UINT m_HitGroupStride;
    CComPtr<ID3D12RaytracingFallbackStateObject> m_pPSO;
    ByteAddressBuffer   m_RayGenShaderTable;
    ByteAddressBuffer   m_MissShaderTable;
    ByteAddressBuffer   m_HitShaderTable;
};
#endif