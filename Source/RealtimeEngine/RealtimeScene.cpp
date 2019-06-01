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

#include "Core/Util.h"
#include "Core/IHitable.h"
#include "Core/Sphere.h"
#include "Core/MovingSphere.h"
#include "Core/HitableList.h"
#include "Core/HitableTransform.h"
#include "Core/BVHNode.h"
#include "Core/HitableBox.h"
#include "Core/XYZRect.h"
#include "Core/Material.h"
#include "Core/Quat.h"
#include "Core/TriMesh.h"

#include "RealtimeSceneShaderInclude.h"
#include "RealtimeCamera.h"
#include "RealtimeScene.h"
#include "RenderDevice.h"
#include "CommandContext.h"

using namespace DirectX;
using namespace RealtimeEngine;

// ----------------------------------------------------------------------------------------------------------------------------

const D3D12_INPUT_ELEMENT_DESC RealtimeSceneVertexEx::InputElements[] =
{
    { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

static const RenderMaterial MaterialWhite =
{
    { 0.00f, 0.00f, 0.00f, 1.00f },
    { .1f, .1f, .1f, .1f },
    { 1.00f, 1.00f, 1.00f, 1.00f },
    { 1.00f, 1.00f, 1.00f, 1.00f },
    128.0f
};

// ----------------------------------------------------------------------------------------------------------------------------

static bool FlipFromNormalStack(Core::Vec4& normal, const std::vector<bool>& flipNormalStack)
{
    bool flipped = false;
    for (int i = 0; i < (int)flipNormalStack.size(); i++)
    {
        flipped = !flipped;
    }

    if (flipped)
    {
        normal = normal * -1.f;
    }

    return flipped;
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline XMMATRIX ComputeFinalMatrix(std::vector<DirectX::XMMATRIX>& matrixStack, XMMATRIX innerMatrix)
{
    XMMATRIX final = innerMatrix;
    for (int i = (int)matrixStack.size() - 1; i >= 0; i--)
    {
        final = final * matrixStack[i];
    }

    return final;
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline XMVECTOR ConvertToXMVector(const Core::Vec4& vec)
{
    return XMVectorSet(vec.X(), vec.Y(), vec.Z(), vec.W());
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline XMFLOAT4 ConvertToXMFloat4(const Core::Vec4& vec)
{
    return DirectX::XMFLOAT4(vec[0], vec[1], vec[2], 1.f);
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline XMFLOAT3 ConvertToXMFloat3(const Core::Vec4& vec)
{
    return DirectX::XMFLOAT3(vec[0], vec[1], vec[2]);
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline Core::Vec4 ConvertToVec4(const XMVECTOR& vec)
{
    XMFLOAT4 v4;
    XMStoreFloat4(&v4, vec);
    return Core::Vec4(v4.x, v4.y, v4.z, v4.w);
}

// ----------------------------------------------------------------------------------------------------------------------------

static void CreateSphere(RealtimeSceneNode* renderNode, float radius, int tessellation)
{
    int verticalSegments   = tessellation;
    int horizontalSegments = tessellation * 2;

    for (int i = 0; i <= verticalSegments; i++)
    {
        float v = 1 - (float)i / verticalSegments;

        float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
        float dy, dxz;

        XMScalarSinCos(&dy, &dxz, latitude);

        for (int j = 0; j <= horizontalSegments; j++)
        {
            float u         = (float)j / horizontalSegments;
            float longitude = j * XM_2PI / horizontalSegments;

            float dx, dz;
            XMScalarSinCos(&dx, &dz, longitude);

            dx *= dxz;
            dz *= dxz;

            XMVECTOR normal   = XMVectorSet(dx, dy, dz, 0);
            XMVECTOR texCoord = XMVectorSet(u, v, 0, 0);

            renderNode->Vertices.push_back(RealtimeSceneVertexEx(normal * radius, normal, texCoord));
        }
    }

    int stride = horizontalSegments + 1;
    for (int i = 0; i < verticalSegments; i++)
    {
        for (int j = 0; j <= horizontalSegments; j++)
        {
            int nextI = i + 1;
            int nextJ = (j + 1) % stride;

            renderNode->Indices.push_back(uint32_t(i * stride + j));
            renderNode->Indices.push_back(uint32_t(nextI * stride + j));
            renderNode->Indices.push_back(uint32_t(i * stride + nextJ));

            renderNode->Indices.push_back(uint32_t(i * stride + nextJ));
            renderNode->Indices.push_back(uint32_t(nextI * stride + j));
            renderNode->Indices.push_back(uint32_t(nextI * stride + nextJ));
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static void CreateCube(RealtimeSceneNode* renderNode, Core::Vec4 sideLengths)
{
    const int FaceCount = 6;

    static const XMVECTORF32 faceNormals[FaceCount] =
    {
        {  0,  0,  1 },
        {  0,  0, -1 },
        {  1,  0,  0 },
        { -1,  0,  0 },
        {  0,  1,  0 },
        {  0, -1,  0 },
    };

    static const XMVECTORF32 textureCoordinates[4] =
    {
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { 0, 0 },
    };

    XMVECTOR halfLengths = XMVectorSet
    (
        sideLengths.X() * 0.5f,
        sideLengths.Y() * 0.5f,
        sideLengths.Z() * 0.5f,
        0.f
    );

    // Create each face in turn.
    for (int i = 0; i < FaceCount; i++)
    {
        XMVECTOR normal = faceNormals[i];
        XMVECTOR basis  = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;
        XMVECTOR side1 = XMVector3Cross(normal, basis);
        XMVECTOR side2 = XMVector3Cross(normal, side1);

        int vbase = (int)renderNode->Vertices.size();
        renderNode->Indices.push_back(uint32_t(vbase + 0));
        renderNode->Indices.push_back(uint32_t(vbase + 1));
        renderNode->Indices.push_back(uint32_t(vbase + 2));
        renderNode->Indices.push_back(uint32_t(vbase + 0));
        renderNode->Indices.push_back(uint32_t(vbase + 2));
        renderNode->Indices.push_back(uint32_t(vbase + 3));

        renderNode->Vertices.push_back(RealtimeSceneVertexEx((normal - side1 - side2) * halfLengths, normal, textureCoordinates[0]));
        renderNode->Vertices.push_back(RealtimeSceneVertexEx((normal - side1 + side2) * halfLengths, normal, textureCoordinates[1]));
        renderNode->Vertices.push_back(RealtimeSceneVertexEx((normal + side1 + side2) * halfLengths, normal, textureCoordinates[2]));
        renderNode->Vertices.push_back(RealtimeSceneVertexEx((normal + side1 - side2) * halfLengths, normal, textureCoordinates[3]));
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static const uint32_t* GetPlaneIndices(bool normalFlipped)
{
    static const uint32_t indices[6]        = { 0, 1, 2, 2, 3, 0 };
    static const uint32_t indicesFlipped[6] = { 0, 3, 1, 1, 3, 2 };

    if (normalFlipped)
    {
        return indicesFlipped;
    }
    else
    {
        return indices;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static void CreatePlaneFromPoints(RealtimeSceneNode* renderNode, const Core::Vec4* points, const Core::Vec4& normal, bool normalFlipped)
{
    renderNode->Vertices.push_back({ ConvertToXMFloat3(points[0]), ConvertToXMFloat3(normal), XMFLOAT2(0.0f, 0.0f) });
    renderNode->Vertices.push_back({ ConvertToXMFloat3(points[1]), ConvertToXMFloat3(normal), XMFLOAT2(1.0f, 0.0f) });
    renderNode->Vertices.push_back({ ConvertToXMFloat3(points[2]), ConvertToXMFloat3(normal), XMFLOAT2(1.0f, 1.0f) });
    renderNode->Vertices.push_back({ ConvertToXMFloat3(points[3]), ConvertToXMFloat3(normal), XMFLOAT2(0.0f, 1.0f) });

    const uint32_t* indices = GetPlaneIndices(normalFlipped);
    for (int i = 0; i < 6; i++)
    {
        renderNode->Indices.push_back(indices[i]);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeAreaLight CreateAreaLight(const Core::Vec4* points, const Core::XYZRect* pXYZRect, const Core::Vec4& normal, const XMMATRIX& worldMatrix, bool normalFlipped)
{
    RealtimeAreaLight areaLight;

    // Get plane parameters
    float a0, a1, b0, b1, k;
    pXYZRect->GetParams(a0, a1, b0, b1, k);

    // "Swizzle" the parameters
    switch (pXYZRect->GetAxisPlane())
    {
        case Core::XYZRect::XY:
        {
            areaLight.PlaneA0 = a0;
            areaLight.PlaneA1 = a1;

            areaLight.PlaneB0 = b0;
            areaLight.PlaneB1 = b1;

            areaLight.PlaneC0 = k;
            areaLight.PlaneC1 = k;
            break;
        }

        case Core::XYZRect::XZ:
        {
            areaLight.PlaneA0 = a0;
            areaLight.PlaneA1 = a1;

            areaLight.PlaneB0 = k;
            areaLight.PlaneB1 = k;

            areaLight.PlaneC0 = b0;
            areaLight.PlaneC1 = b1;
            break;
        }

        case Core::XYZRect::YZ:
        {
            areaLight.PlaneA0 = k;
            areaLight.PlaneA1 = k;

            areaLight.PlaneB0 = a0;
            areaLight.PlaneB1 = a1;

            areaLight.PlaneC0 = b0;
            areaLight.PlaneC1 = b1;
            break;
        }
    }

    // Normal
    XMVECTOR normalWS = XMVector3Transform(ConvertToXMVector(normal), worldMatrix);
    XMStoreFloat4(&areaLight.NormalWS, normalWS);
    
    // Area
    Core::Vec4 cross0 = Cross((points[0] - points[1]), (points[0] - points[3]));
    Core::Vec4 cross1 = Cross((points[2] - points[1]), (points[2] - points[3]));
    areaLight.AreaCoverage = (cross0.Length() * 0.5f) + (cross1.Length() * 0.5f);

    return areaLight;
}

// ----------------------------------------------------------------------------------------------------------------------------

static void CreateVertexIndexBuffers(RealtimeSceneNode* renderNode)
{
    renderNode->VertexBuffer.Create
    (
        L"VertexBuffer", 
        (uint32_t)renderNode->Vertices.size(), 
        sizeof(RealtimeSceneVertexEx), 
        &renderNode->Vertices[0],
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_FLAG_NONE
    );

    renderNode->IndexBuffer.Create
    (
        L"IndexBuffer", 
        (uint32_t)renderNode->Indices.size(), 
        sizeof(uint32_t), 
        &renderNode->Indices[0],
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_FLAG_NONE
    );
}

// ----------------------------------------------------------------------------------------------------------------------------

static RenderMaterial ConvertFromCoreMaterial(const Core::Material* pCoreMaterial)
{
    // HACKS until we get materials working for reals
    Core::Vec4            color = pCoreMaterial->AlbedoValue(0.5f, 0.5f, Core::Vec4(0, 0, 0));
    const std::type_info& tid   = typeid(*pCoreMaterial);
    if (tid == typeid(Core::MWavefrontObj))
    {
        // Get average color
        color = pCoreMaterial->GetAverageAlbedo();
    }
    else if (tid == typeid(Core::MDielectric))
    {
        // These materials don't have color, use white
        color = Core::Vec4(1, 1, 1, 1);
    }

    RenderMaterial newMaterial =
    {
        { 0.0f,      0.0f,      0.0f,      1.0f },
        { 0.0f,      0.0f,      0.0f,      1.0f },
        { color.X(), color.Y(), color.Z(), 1.0f },
        { 0.0f,      0.0f,      0.0f,      1.0f },
        128.0f
    };

    return newMaterial;
}

// ----------------------------------------------------------------------------------------------------------------------------

static RealtimeEngine::Texture* CreateTextureFromMaterial(const RenderMaterial& material)
{
    XMFLOAT4 color = material.Diffuse;

    return &RealtimeEngine::TextureManager::CreateFromColor(color.x, color.y, color.z, color.w);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeScene::GenerateRenderListFromWorld(Core::IHitable* currentHead, std::vector<DirectX::XMMATRIX>& matrixStack, std::vector<bool>& flipNormalStack)
{
    const std::type_info& tid = typeid(*currentHead);

    if (tid == typeid(Core::HitableList))
    {
        Core::HitableList* hitList  = (Core::HitableList*)currentHead;
        Core::IHitable**   list     = hitList->GetList();
        const int          listSize = hitList->GetListSize();
        for (int i = 0; i < listSize; i++)
        {
            GenerateRenderListFromWorld(list[i], matrixStack, flipNormalStack);
        }
    }
    else if (tid == typeid(Core::BVHNode))
    {
        Core::BVHNode* bvhNode = (Core::BVHNode*)currentHead;

        GenerateRenderListFromWorld(bvhNode->GetLeft(), matrixStack, flipNormalStack);
        if (bvhNode->GetLeft() != bvhNode->GetRight())
        {
            GenerateRenderListFromWorld(bvhNode->GetRight(), matrixStack, flipNormalStack);
        }
    }
    else if (tid == typeid(Core::HitableTranslate))
    {
        Core::HitableTranslate* translateHitable = (Core::HitableTranslate*)currentHead;
        Core::Vec4              offset           = translateHitable->GetOffset();
        XMMATRIX                translation      = XMMatrixTranslation(offset.X(), offset.Y(), offset.Z());

        matrixStack.push_back(translation);
        GenerateRenderListFromWorld(translateHitable->GetHitObject(), matrixStack, flipNormalStack);
        matrixStack.pop_back();
    }
    else if (tid == typeid(Core::HitableRotateY))
    {
        Core::HitableRotateY*   rotateYHitable = (Core::HitableRotateY*)currentHead;
        XMMATRIX                rotation       = XMMatrixRotationY(XMConvertToRadians(rotateYHitable->GetAngleDegrees()));

        matrixStack.push_back(rotation);
        GenerateRenderListFromWorld(rotateYHitable->GetHitObject(), matrixStack, flipNormalStack);
        matrixStack.pop_back();
    }
    else if (tid == typeid(Core::FlipNormals))
    {
        Core::FlipNormals* flipNormals = (Core::FlipNormals*)currentHead;

        flipNormalStack.push_back(true);
        GenerateRenderListFromWorld(flipNormals->GetHitObject(), matrixStack, flipNormalStack);
        flipNormalStack.pop_back();
    }
    else if (tid == typeid(Core::Sphere) || tid == typeid(Core::MovingSphere))
    {
        Core::Vec4      offset;
        float           radius  = 0;
        if (tid == typeid(Core::Sphere))
        {
            Core::Sphere* sphere = (Core::Sphere*)currentHead;
            offset   = sphere->GetCenter();
            radius   = sphere->GetRadius();
        }
        else if (tid == typeid(Core::MovingSphere))
        {
            Core::MovingSphere* sphere = (Core::MovingSphere*)currentHead;
            offset   = sphere->Center(1.f);
            radius   = sphere->GetRadius();
        }

        XMMATRIX           translation = XMMatrixTranslation(offset.X(), offset.Y(), offset.Z());
        XMMATRIX           newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RealtimeSceneNode* newNode     = new RealtimeSceneNode();

        CreateSphere(newNode, radius, 32);
        AddNewNode(newNode, newMatrix, currentHead);
    }
    else if (tid == typeid(Core::HitableBox))
    {
        Core::HitableBox* box = (Core::HitableBox*)currentHead;

        Core::Vec4 minP, maxP;
        box->GetPoints(minP, maxP);

        Core::Vec4             diff        = maxP - minP;
        Core::Vec4             offset      = minP + (diff * 0.5f);
        XMMATRIX               translation = XMMatrixTranslation(offset.X(), offset.Y(), offset.Z());
        XMMATRIX               newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RealtimeSceneNode*     newNode     = new RealtimeSceneNode();

        CreateCube(newNode, diff);
        AddNewNode(newNode, newMatrix, currentHead);
    }
    else if (tid == typeid(Core::TriMesh))
    {
        Core::TriMesh*      triMesh   = (Core::TriMesh*)currentHead;
        Core::IHitable**    triArray  = nullptr;
        int                 numTris   = 0;
        RealtimeSceneNode*  newNode   = new RealtimeSceneNode();
        
        // Walk through all the triangles
        triMesh->GetTriArray(triArray, numTris);
        for (int t = 0; t < numTris; t++)
        {
            const Core::Triangle* tri = (const Core::Triangle*)triArray[t];

            const Core::Triangle::Vertex* triVertices = tri->GetVertices();
            for (int v = 2; v >= 0; v--)
            {
                float s = triVertices[v].UV[0];
                float t = 1.f - triVertices[v].UV[1];

                XMVECTOR position = ConvertToXMVector(triVertices[v].Vert);
                XMVECTOR normal   = ConvertToXMVector(triVertices[v].Normal);
                XMVECTOR texCoord = XMVectorSet(s, t, 0, 0);

                newNode->Vertices.push_back(RealtimeSceneVertexEx(position, normal, texCoord));
                newNode->Indices.push_back(uint32_t(newNode->Vertices.size() - 1));
            }
        }

        XMMATRIX translation = XMMatrixIdentity();
        XMMATRIX newMatrix   = ComputeFinalMatrix(matrixStack, translation);

        AddNewNode(newNode, newMatrix, currentHead);
    }
    else if (tid == typeid(Core::XYZRect))
    {
        Core::XYZRect*  xyzRect = (Core::XYZRect*)currentHead;
        Core::Vec4      planePoints[4];
        Core::Vec4      normal;

        xyzRect->GetPlaneData(planePoints, normal);
        bool normalFlipped = FlipFromNormalStack(normal, flipNormalStack);

        const Core::Material*   material    = xyzRect->GetMaterial();
        XMMATRIX                translation = XMMatrixIdentity();
        XMMATRIX                newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RealtimeSceneNode*      newNode     = new RealtimeSceneNode();

        CreatePlaneFromPoints(newNode, planePoints, normal, normalFlipped);
        AddNewNode(newNode, newMatrix, currentHead);

        if (xyzRect->IsALightShape())
        {
            newNode->AreaLight       = CreateAreaLight(planePoints, xyzRect, normal, newMatrix, normalFlipped);
            newNode->AreaLight.Color = newNode->Material.Diffuse;
            newNode->LightIndex      = (int32_t)AreaLightsList.size();

            AreaLightsList.push_back(newNode);
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeEngine::RealtimeScene::AddNewNode(RealtimeSceneNode* newNode, const DirectX::XMMATRIX& worldMatrix, Core::IHitable* pHitable)
{
    // Convert material
    RenderMaterial material = ConvertFromCoreMaterial(pHitable->GetMaterial());

    // Convert texture
    Core::BaseTexture*       pCoreTexture    = pHitable->GetMaterial()->GetAlbedoTexture();
    RealtimeEngine::Texture* pDiffuseTexture = nullptr;
    if (pCoreTexture != nullptr && typeid(*pCoreTexture) == typeid(Core::ImageTexture))
    {
        std::string  fileName = ((Core::ImageTexture*)pCoreTexture)->GetSourceFilename();
        pDiffuseTexture = RealtimeEngine::TextureManager::LoadFromFile(fileName);
    }
    else
    {
        pDiffuseTexture = CreateTextureFromMaterial(material);
    }

    // Fill in the new node
    newNode->InstanceId                 = (uint32_t)RenderSceneList.size();
    newNode->WorldMatrix                = worldMatrix;
    newNode->Material                   = material;;
    newNode->Material.DiffuseTextureId  = DiffuseTextureList.GetCount();
    newNode->DiffuseTextureIndex        = DiffuseTextureList.GetCount();
    newNode->Hitable                    = pHitable;
    DiffuseTextureList.Add(pDiffuseTexture);
    CreateVertexIndexBuffers(newNode);
    RenderSceneList.push_back(newNode);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeScene::UpdateCamera(float newVertFov, float forwardAmount, float strafeAmount, float upDownAmount, int mouseDx, int mouseDy, Core::Camera& worldCamera)
{
    // Get ray tracer camera params
    Core::Vec4   lookFrom, lookAt, up;
    float        vertFov, aspect, aperture, focusDist, t0, t1;
    Core::Vec4   clearColor;
    worldCamera.GetCameraParams(lookFrom, lookAt, up, vertFov, aspect, aperture, focusDist, t0, t1, clearColor);

    // Update render camera look at
    Core::Vec4  diffVec = (lookAt - lookFrom);
    Core::Vec4  viewDir = diffVec.MakeUnitVector();
    Core::Vec4  rightDir = Cross(up, viewDir).MakeUnitVector();
    float upAngle = mouseDx * -.1f;
    float rightAngle = mouseDy * -.1f;

    // Rotate around up axis
    viewDir  = Core::Quat::RotateVector(viewDir, up, upAngle);
    rightDir = Core::Quat::RotateVector(rightDir, up, upAngle);

    // Rotate around right axis
    viewDir = Core::Quat::RotateVector(viewDir, rightDir, rightAngle);

    Core::Vec4 cameraTranslate = (viewDir * forwardAmount) + (rightDir * strafeAmount) + (up * upDownAmount);
    lookFrom += cameraTranslate;
    lookAt = lookFrom + (viewDir * diffVec.Length());

    // Update raytracer camera
    worldCamera.Setup(lookFrom, lookAt, up, newVertFov, aspect, aperture, focusDist, t0, t1, clearColor);
    worldCamera.SetFocusDistanceToLookAt();

    // Set the render camera
    XMVECTOR cameraPos    = ConvertToXMVector(lookFrom);
    XMVECTOR cameraTarget = ConvertToXMVector(lookAt);
    XMVECTOR cameraUp     = ConvertToXMVector(up);
    TheRenderCamera.SetLookAt(cameraPos, cameraTarget, cameraUp);
    TheRenderCamera.SetProjection(newVertFov, aspect, 0.1f, 10000.0f);

    #if 0
        // Has mouse moved?
        static int lastMouseDx = mouseDx;
        static int lastMouseDy = mouseDy;
        bool mouseMoved = false;
        if (mouseDx != lastMouseDx || mouseDy != lastMouseDy)
        {
            mouseMoved = true;
            lastMouseDx = mouseDx;
            lastMouseDy = mouseDy;
        }

        // Print out camera changes
        if (mouseMoved)
        {
            DEBUG_PRINTF("lookFrom:(%f, %f, %f)  lookAt:(%f, %f, %f)  up:(%f, %f, %f)  vertFov:%f  aspect:%f  aperture:%f  focusDist:%f\n",
                lookFrom[0], lookFrom[1], lookFrom[2],
                lookAt[0], lookAt[1], lookAt[2],
                up[0], up[1], up[2],
                newVertFov, aspect, aperture, focusDist
            );
        }
    #endif
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeScene::UpdateAreaLightBuffer(CommandContext& context, RealtimeSceneNode* pNode)
{
    if (pNode->Hitable->IsALightShape())
    {
        memcpy(ScratchCopyBuffer, &pNode->AreaLight, sizeof(RealtimeAreaLight));
        context.WriteBuffer(pNode->AreaLightBuffer, 0, ScratchCopyBuffer, ScratchCopyBufferSize);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeScene::UpdateInstanceBuffer(CommandContext& context, RealtimeSceneNode* pNode)
{
    RenderNodeInstanceData* pInstanceData = (RenderNodeInstanceData*)ScratchCopyBuffer;
    {
        pInstanceData->InstanceId  = pNode->InstanceId;
        pInstanceData->LightIndex  = pNode->LightIndex;
        pInstanceData->WorldMatrix = XMMatrixTranspose(pNode->WorldMatrix);
    }
    context.WriteBuffer(pNode->InstanceDataBuffer, 0, ScratchCopyBuffer, ScratchCopyBufferSize);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeScene::UpdateMaterialBuffer(CommandContext& context, RealtimeSceneNode* pNode)
{
    memcpy(ScratchCopyBuffer, &pNode->Material, sizeof(RenderMaterial));
    context.WriteBuffer(pNode->MaterialBuffer, 0, ScratchCopyBuffer, ScratchCopyBufferSize);
}

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeScene::RealtimeScene(Core::WorldScene* worldScene)
{
    // Generate the scene objects from the world scene
    std::vector<DirectX::XMMATRIX>  matrixStack;
    std::vector<bool>               flipNormalStack;
    GenerateRenderListFromWorld(worldScene->GetWorld(), matrixStack, flipNormalStack);
    RTL_ASSERT(matrixStack.size() == 0);
    RTL_ASSERT(flipNormalStack.size() == 0);

    // Allocate a buffer big enough to do copies
    ScratchCopyBufferSize = (uint32_t)AlignUp(std::max(sizeof(RealtimeAreaLight), std::max(sizeof(RenderNodeInstanceData), sizeof(RenderMaterial))), 256);
    ScratchCopyBuffer     = _aligned_malloc(ScratchCopyBufferSize, 16);

    // Now that scene objects are created, setup for raytracing
    RaytracingGeom = new RaytracingGeometry(RAYTRACING_NUM_HIT_PROGRAMS);
    GraphicsContext& context = GraphicsContext::Begin("RealtimeScene Build");
    {
        for (size_t i = 0; i < RenderSceneList.size(); i++)
        {
            RealtimeSceneNode* pNode = RenderSceneList[i];

            // Update all buffers
            pNode->AreaLightBuffer.Create(L"AreaLight", 1, ScratchCopyBufferSize, ScratchCopyBuffer);
            pNode->InstanceDataBuffer.Create(L"Instance Data", 1, ScratchCopyBufferSize, ScratchCopyBuffer);
            pNode->MaterialBuffer.Create(L"Material Data", 1, ScratchCopyBufferSize, ScratchCopyBuffer);
            UpdateAreaLightBuffer(context, pNode);
            UpdateInstanceBuffer(context, pNode);
            UpdateMaterialBuffer(context, pNode);

            // Get the instance mask
            uint32_t instanceMask =
                pNode->Hitable->IsALightShape() ? RAYTRACING_INSTANCEMASK_AREALIGHT : RAYTRACING_INSTANCEMASK_OPAQUE;

            // Add geometry to the raytracing builder
            RaytracingGeom->AddGeometry
            (
                RaytracingGeometry::GeometryInfo
                (
                    pNode->InstanceId,
                    instanceMask,
                    (uint32_t)pNode->Vertices.size(),
                    (uint32_t)pNode->Indices.size(),
                    &pNode->VertexBuffer,
                    &pNode->IndexBuffer,
                    &pNode->WorldMatrix
                )
            );
        }
    }
    context.Finish(true);
    RaytracingGeom->Build();
}

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeScene::~RealtimeScene()
{
    if (RaytracingGeom != nullptr)
    {
        delete RaytracingGeom;
        RaytracingGeom = nullptr;
    }

    for (int i = 0; i < RenderSceneList.size(); i++)
    {
        delete RenderSceneList[i];
    }
    RenderSceneList.clear();

    if (ScratchCopyBuffer != nullptr)
    {
        _aligned_free(ScratchCopyBuffer);
        ScratchCopyBuffer = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeScene::UpdateAllGpuBuffers(CommandContext& context)
{
    for (size_t i = 0; i < RenderSceneList.size(); i++)
    {
        RealtimeSceneNode* pNode = RenderSceneList[i];
        UpdateAreaLightBuffer(context, pNode);
        UpdateInstanceBuffer(context, pNode);
        UpdateMaterialBuffer(context, pNode);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeEngine::RealtimeScene::SetupResourceViews(DescriptorHeapStack& descriptorHeap)
{
    for (size_t i = 0; i < RenderSceneList.size(); i++)
    {
        RenderSceneList[i]->InstanceDataHeapIndex = descriptorHeap.AllocateBufferCbv(RenderSceneList[i]->InstanceDataBuffer.GetGpuVirtualAddress(), (UINT)RenderSceneList[i]->InstanceDataBuffer.GetBufferSize());
        RenderSceneList[i]->MaterialHeapIndex     = descriptorHeap.AllocateBufferCbv(RenderSceneList[i]->MaterialBuffer.GetGpuVirtualAddress(), (UINT)RenderSceneList[i]->MaterialBuffer.GetBufferSize());
    }

    DiffuseTextureList.DescriptorHeapStartIndex = descriptorHeap.GetCount();
    for (size_t i = 0; i < DiffuseTextureList.Textures.size(); i++)
    {
        RealtimeEngine::Texture* pTexture = DiffuseTextureList.Textures[i];
        descriptorHeap.AllocateTexture2DSrv(pTexture->GetResource(), pTexture->GetFormat());
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeScene::TextureList& RealtimeScene::GetDiffuseTextureList()
{
    return DiffuseTextureList;
}

// ----------------------------------------------------------------------------------------------------------------------------

std::vector<RealtimeSceneNode*>& RealtimeScene::GetRenderSceneList()
{
    return RenderSceneList;
}

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeCamera& RealtimeScene::GetCamera()
{
    return TheRenderCamera;
}

// ----------------------------------------------------------------------------------------------------------------------------

RaytracingGeometry* RealtimeScene::GetRaytracingGeometry()
{
    return RaytracingGeom;
}

// ----------------------------------------------------------------------------------------------------------------------------

std::vector<RealtimeSceneNode*>& RealtimeScene::GetAreaLightsList()
{
    return AreaLightsList;
}

