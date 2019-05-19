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

#include "RenderSceneShader.h"
#include "RenderCamera.h"
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

static void CreatePlaneFromPoints(RealtimeSceneNode* renderNode, const Core::Vec4* points, const Core::Vec4& normal)
{
    renderNode->Vertices.push_back({ ConvertToXMFloat3(points[0]), ConvertToXMFloat3(normal), XMFLOAT2(0.0f, 0.0f) });
    renderNode->Vertices.push_back({ ConvertToXMFloat3(points[1]), ConvertToXMFloat3(normal), XMFLOAT2(1.0f, 0.0f) });
    renderNode->Vertices.push_back({ ConvertToXMFloat3(points[2]), ConvertToXMFloat3(normal), XMFLOAT2(1.0f, 1.0f) });
    renderNode->Vertices.push_back({ ConvertToXMFloat3(points[3]), ConvertToXMFloat3(normal), XMFLOAT2(0.0f, 1.0f) });

    renderNode->Indices.push_back(0);
    renderNode->Indices.push_back(3);
    renderNode->Indices.push_back(1);
    renderNode->Indices.push_back(1);
    renderNode->Indices.push_back(3);
    renderNode->Indices.push_back(2);
}

// ----------------------------------------------------------------------------------------------------------------------------

static void CreateResourceViews(RealtimeSceneNode* renderNode)
{
    renderNode->VertexBuffer.Create(L"VertexBuffer", (uint32_t)renderNode->Vertices.size(), sizeof(RealtimeSceneVertexEx), &renderNode->Vertices[0]);
    renderNode->IndexBuffer.Create(L"IndexBuffer", (uint32_t)renderNode->Indices.size(), sizeof(uint32_t), &renderNode->Indices[0]);
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeScene::GenerateRenderListFromWorld(const Core::IHitable* currentHead, RealtimeEngine::Texture* defaultTexture, std::vector<RealtimeSceneNode*>& outSceneList, std::vector<SpotLight>& spotLightsList, std::vector<DirectX::XMMATRIX>& matrixStack, std::vector<bool>& flipNormalStack)
{
    const std::type_info& tid = typeid(*currentHead);

    if (tid == typeid(Core::HitableList))
    {
        Core::HitableList* hitList  = (Core::HitableList*)currentHead;
        Core::IHitable**   list     = hitList->GetList();
        const int          listSize = hitList->GetListSize();
        for (int i = 0; i < listSize; i++)
        {
            GenerateRenderListFromWorld(list[i], defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        }
    }
    else if (tid == typeid(Core::BVHNode))
    {
        Core::BVHNode* bvhNode = (Core::BVHNode*)currentHead;

        GenerateRenderListFromWorld(bvhNode->GetLeft(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        if (bvhNode->GetLeft() != bvhNode->GetRight())
        {
            GenerateRenderListFromWorld(bvhNode->GetRight(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        }
    }
    else if (tid == typeid(Core::HitableTranslate))
    {
        Core::HitableTranslate* translateHitable = (Core::HitableTranslate*)currentHead;
        Core::Vec4              offset           = translateHitable->GetOffset();
        XMMATRIX                translation      = XMMatrixTranslation(offset.X(), offset.Y(), offset.Z());

        matrixStack.push_back(translation);
        GenerateRenderListFromWorld(translateHitable->GetHitObject(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        matrixStack.pop_back();
    }
    else if (tid == typeid(Core::HitableRotateY))
    {
        Core::HitableRotateY*   rotateYHitable = (Core::HitableRotateY*)currentHead;
        XMMATRIX                rotation       = XMMatrixRotationY(XMConvertToRadians(rotateYHitable->GetAngleDegrees()));

        matrixStack.push_back(rotation);
        GenerateRenderListFromWorld(rotateYHitable->GetHitObject(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        matrixStack.pop_back();
    }
    else if (tid == typeid(Core::FlipNormals))
    {
        Core::FlipNormals* flipNormals = (Core::FlipNormals*)currentHead;

        flipNormalStack.push_back(true);
        GenerateRenderListFromWorld(flipNormals->GetHitObject(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        flipNormalStack.pop_back();
    }
    else if (tid == typeid(Core::Sphere) || tid == typeid(Core::MovingSphere))
    {
        Core::Vec4      offset;
        float           radius  = 0;
        Core::Material* material = nullptr;
        if (tid == typeid(Core::Sphere))
        {
            Core::Sphere* sphere = (Core::Sphere*)currentHead;
            offset   = sphere->GetCenter();
            radius   = sphere->GetRadius();
            material = sphere->GetMaterial();
        }
        else if (tid == typeid(Core::MovingSphere))
        {
            Core::MovingSphere* sphere = (Core::MovingSphere*)currentHead;
            offset   = sphere->Center(1.f);
            radius   = sphere->GetRadius();
            material = sphere->GetMaterial();
        }

        XMMATRIX         translation = XMMatrixTranslation(offset.X(), offset.Y(), offset.Z());
        XMMATRIX         newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RealtimeSceneNode* newNode     = new RealtimeSceneNode();

        Core::Vec4 color = material->AlbedoValue(0.5f, 0.5f, Core::Vec4(0, 0, 0));
        const RenderMaterial newMaterial =
        {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { color.X(), color.Y(), color.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            128.0f
        };

        CreateSphere(newNode, radius, 16);
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = newMaterial;
        newNode->DiffuseTexture = defaultTexture;
        newNode->Hitable        = currentHead;
        CreateResourceViews(newNode);
        outSceneList.push_back(newNode);
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
        RealtimeSceneNode*       newNode     = new RealtimeSceneNode();
        const Core::Material*  material    = box->GetMaterial();

        Core::Vec4 color = material->AlbedoValue(0.5f, 0.5f, Core::Vec4(0, 0, 0));
        const RenderMaterial newMaterial =
        {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { color.X(), color.Y(), color.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            128.0f
        };

        CreateCube(newNode, diff);
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = newMaterial;
        newNode->DiffuseTexture = defaultTexture;
        newNode->Hitable        = currentHead;
        CreateResourceViews(newNode);
        outSceneList.push_back(newNode);
    }
    else if (tid == typeid(Core::TriMesh))
    {
        Core::TriMesh*    triMesh   = (Core::TriMesh*)currentHead;
        Core::IHitable**  triArray  = nullptr;
        int               numTris   = 0;
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
        XMMATRIX newMatrix = ComputeFinalMatrix(matrixStack, translation);

        RealtimeEngine::Texture* newTexture = defaultTexture;
        if (typeid(*triMesh->GetMaterial()) == typeid(Core::MWavefrontObj))
        {
            std::string  fileName = ((Core::MWavefrontObj*)triMesh->GetMaterial())->GetDiffuseMap()->GetSourceFilename().c_str();
            newTexture = RealtimeEngine::TextureManager::LoadFromFile(fileName);
        }

        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = MaterialWhite;
        newNode->DiffuseTexture = newTexture;
        newNode->Hitable        = currentHead;
        CreateResourceViews(newNode);
        outSceneList.push_back(newNode);
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
        RealtimeSceneNode*        newNode     = new RealtimeSceneNode();

        // Clamp colors for lights, since they can overblow the color buffer
        Core::Vec4 color = material->AlbedoValue(0.5f, 0.5f, Core::Vec4(0, 0, 0));
        color.Clamp(0.f, 1.f);

        // Light shapes should not get lit, and always show up fully shaded
        Core::Vec4 em = xyzRect->IsALightShape() ? Core::Vec4(1.f, 1.f, 1.f, 1.f) : Core::Vec4(0.0f, 0.0f, 0.0f, 1.0f);

        const RenderMaterial newMaterial =
        {
            { em.X(), em.Y(), em.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { color.X(), color.Y(), color.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            128.0f
        };

        CreatePlaneFromPoints(newNode, planePoints, normal);
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = newMaterial;
        newNode->DiffuseTexture = defaultTexture;
        newNode->Hitable        = currentHead;
        CreateResourceViews(newNode);
        outSceneList.push_back(newNode);
        
        // These can be light shapes too
        if (xyzRect->IsALightShape())
        {
            if (xyzRect->GetAxisPlane() == Core::XYZRect::AxisPlane::XZ)
            {
                float xValues[2], yValue, zValues[2];
                xyzRect->GetParams(
                    xValues[0], xValues[1],
                    zValues[0], zValues[1],
                    yValue);

                Core::Vec4 pos(
                    (xValues[0] + xValues[1]) * 0.5f,
                    yValue,
                    (zValues[0] + zValues[1]) * 0.5f);
                
                Core::Vec4  upDir   = Core::Vec4(1.f, 0.f, 1.f);
                Core::Vec4  lookAt  = Core::Vec4(pos[0], -yValue, pos[2]);
                Core::Vec4  dir     = UnitVector(lookAt - pos);

                // Pullback the camera for shadowmap rendering
                float smapDist = 1000.f - fabs(yValue);
                Core::Vec4  smapPos  = pos - (dir * smapDist);

                SpotLight light;
                light.PositionWS           = ConvertToXMFloat4(pos);
                light.DirectionWS          = ConvertToXMFloat4(dir);
                light.LookAtWS             = ConvertToXMFloat4(lookAt);
                light.UpWS                 = ConvertToXMFloat4(upDir);
                light.SmapWS               = ConvertToXMFloat4(smapPos);
                light.SpotAngle            = RT_PI / 2.5f;
                light.ConstantAttenuation  = 1.f;
                light.LinearAttenuation    = 0.00f;
                light.QuadraticAttenuation = 0.f;
                light.Color                = ConvertToXMFloat4(color);

                spotLightsList.push_back(light);
            }
        }
    }
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

RealtimeScene::RealtimeScene(Core::WorldScene* worldScene)
{
    // Generate the scene objects from the world scene
    std::vector<DirectX::XMMATRIX>  matrixStack;
    std::vector<bool>               flipNormalStack;
    std::vector<SpotLight>          spotLightsList;
    GenerateRenderListFromWorld(worldScene->GetWorld(), nullptr, RenderSceneList, spotLightsList, matrixStack, flipNormalStack);

    // Now that scene objects are created, setup for raytracing
    SetupForRaytracing();
}

// ----------------------------------------------------------------------------------------------------------------------------

RealtimeScene::~RealtimeScene()
{
    for (int i = 0; i < RenderSceneList.size(); i++)
    {
        delete RenderSceneList[i];
    }
    RenderSceneList.clear();
}

// ----------------------------------------------------------------------------------------------------------------------------

std::vector<RealtimeSceneNode*>& RealtimeScene::GetRenderSceneList()
{
    return RenderSceneList;
}

// ----------------------------------------------------------------------------------------------------------------------------

RenderCamera& RealtimeScene::GetRenderCamera()
{
    return TheRenderCamera;
}

// ----------------------------------------------------------------------------------------------------------------------------

void RealtimeScene::SetupForRaytracing()
{
    // numBottomLevels == number of scene objects
    const int numBottomLevels = (int)RenderSceneList.size();

    // Gather info on TLAS
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuildInfo;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC    tlasDesc = {};
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = tlasDesc.Inputs;
        topLevelInputs.Type             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        topLevelInputs.NumDescs         = numBottomLevels;
        topLevelInputs.Flags            = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        topLevelInputs.pGeometryDescs   = nullptr;
        topLevelInputs.DescsLayout      = D3D12_ELEMENTS_LAYOUT_ARRAY;
        RenderDevice::Get().GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &tlasPrebuildInfo);
    }

    // Gather info on BLAS
    std::vector<UINT64>                                             blasSizes(RenderSceneList.size());
    std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> blasDescs(RenderSceneList.size());
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>                     geometryDescs(RenderSceneList.size());
    UINT64                                                          scratchBufferSizeNeeded = tlasPrebuildInfo.ScratchDataSizeInBytes;
    {
        // Fill in the geometry descriptions
        for (int i = 0; i < (int)RenderSceneList.size(); i++)
        {
            RealtimeSceneNode*    pRenderNode      = RenderSceneList[i];
            const int             offsetToPosition = 0;
            const int             offsetToIndex    = 0;

            D3D12_RAYTRACING_GEOMETRY_DESC& desc = geometryDescs[i];
            desc.Type  = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& trianglesDesc = desc.Triangles;
            trianglesDesc.VertexFormat                  = DXGI_FORMAT_R32G32B32_FLOAT;
            trianglesDesc.VertexCount                   = (UINT)pRenderNode->Vertices.size();
            trianglesDesc.VertexBuffer.StartAddress     = pRenderNode->VertexBuffer->GetGPUVirtualAddress() + offsetToPosition;
            trianglesDesc.VertexBuffer.StrideInBytes    = pRenderNode->VertexBuffer.GetElementSize();
            trianglesDesc.IndexFormat                   = DXGI_FORMAT_R32_UINT;
            trianglesDesc.IndexCount                    = (UINT)pRenderNode->Indices.size();
            trianglesDesc.IndexBuffer                   = pRenderNode->IndexBuffer.GetGpuVirtualAddress() + offsetToIndex;
            trianglesDesc.Transform3x4                  = 0;
        }

        // Gather prebuild info on blas
        for (int i = 0; i < numBottomLevels; i++)
        {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC&   blasDesc   = blasDescs[i];
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& blasInputs = blasDesc.Inputs;
            blasInputs.Type              = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            blasInputs.NumDescs          = 1;
            blasInputs.pGeometryDescs    = &geometryDescs[i];
            blasInputs.Flags             = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            blasInputs.DescsLayout       = D3D12_ELEMENTS_LAYOUT_ARRAY;

            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blasPrebuildInfo;
            RenderDevice::Get().GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&blasInputs, &blasPrebuildInfo);

            blasSizes[i]            = blasPrebuildInfo.ResultDataMaxSizeInBytes;
            scratchBufferSizeNeeded = std::max(blasPrebuildInfo.ScratchDataSizeInBytes, scratchBufferSizeNeeded);
        }
    }

    // Allocate scratch buffer
    ByteAddressBuffer scratchBuffer;
    scratchBuffer.Create(L"Acceleration Structure Scratch Buffer", (UINT)scratchBufferSizeNeeded, 1);

    // Allocate TLAS buffer
    TLASBuffer = new StructuredBuffer();
    TLASBuffer->Create(L"TLAS Buffer", 1, (uint32_t)tlasPrebuildInfo.ResultDataMaxSizeInBytes, nullptr, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

    // Point TLAS description to the allocated buffers
    tlasDesc.DestAccelerationStructureData    = TLASBuffer->GetGpuVirtualAddress();
    tlasDesc.ScratchAccelerationStructureData = scratchBuffer.GetGpuVirtualAddress();

    // Allocate BLAS buffers
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(numBottomLevels);
    BLASBuffers.resize(numBottomLevels);
    for (UINT i = 0; i < blasDescs.size(); i++)
    {
        // Allocate buffer
        BLASBuffers[i] = new StructuredBuffer();
        BLASBuffers[i]->Create(L"BLAS Buffer", 1, (uint32_t)blasSizes[i], nullptr, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

        // Point to buffers
        blasDescs[i].DestAccelerationStructureData    = BLASBuffers[i]->GetGpuVirtualAddress();
        blasDescs[i].ScratchAccelerationStructureData = scratchBuffer.GetGpuVirtualAddress();

        // Fill in the desc
        D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDescs[i];

        // Fill in transform
        XMStoreFloat3x4(reinterpret_cast<XMFLOAT3X4*>(instanceDesc.Transform), RenderSceneList[i]->WorldMatrix);

        // Fill in the rest
        instanceDesc.AccelerationStructure                  = BLASBuffers[i]->GetGpuVirtualAddress();
        instanceDesc.Flags                                  = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instanceDesc.InstanceID                             = i;
        instanceDesc.InstanceMask                           = 0xFF;
        instanceDesc.InstanceContributionToHitGroupIndex    = i;
    }

    // Allocate instance data buffer and update TLAS desc
    InstanceDataBuffer.Create(L"Instance Data Buffer", numBottomLevels, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instanceDescs.data());
    tlasDesc.Inputs.InstanceDescs = InstanceDataBuffer.GetGpuVirtualAddress();
    tlasDesc.Inputs.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY;

    // Finally, build the acceleration structures
    GraphicsContext& context = GraphicsContext::Begin("Create Acceleration Structure");
    {
        ID3D12GraphicsCommandList4* pCommandList = context.GetCommandList();
        auto                        uavBarrier   = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
        for (UINT i = 0; i < blasDescs.size(); i++)
        {
            pCommandList->BuildRaytracingAccelerationStructure(&blasDescs[i], 0, nullptr);
            pCommandList->ResourceBarrier(1, &uavBarrier);
        }
        pCommandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);
    }
    context.Finish(true);
}
