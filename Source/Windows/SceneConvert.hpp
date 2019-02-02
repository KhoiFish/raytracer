#pragma once

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

#include "ShaderStructs.h"

#include <DirectXMath.h>
using namespace DirectX;
#include <Mesh.h>

// ----------------------------------------------------------------------------------------------------------------------------

struct RenderSceneNode
{
    const IHitable*         Hitable;
    std::shared_ptr<Mesh>   MeshData;
    XMMATRIX                WorldMatrix;
    RenderMaterial          Material;
    Texture*                DiffuseTexture;
};

// ----------------------------------------------------------------------------------------------------------------------------

static const RenderMaterial MaterialWhite =
{
    { 0.00f, 0.00f, 0.00f, 1.00f },
    { .1f, .1f, .1f, .1f },
    { 1.00f, 1.00f, 1.00f, 1.00f },
    { 1.00f, 1.00f, 1.00f, 1.00f },
    128.0f
};

// ----------------------------------------------------------------------------------------------------------------------------

static bool FlipFromNormalStack(Vec3& normal, const std::vector<bool>& flipNormalStack)
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

static inline XMVECTOR ConvertToXMVector(const Vec3& vec)
{
    // Negate Z
    return XMVectorSet(vec.X(), vec.Y(), -vec.Z(), vec.W());
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline XMFLOAT4 ConvertToXMFloat4(const Vec3& vec, bool negateZ = true)
{
    return DirectX::XMFLOAT4(vec[0], vec[1], negateZ ? -vec[2] : vec[2], 1.f);
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline Vec3 ConvertToVec3(const XMVECTOR& vec)
{
    // Negate Z
    XMFLOAT4 v4;
    XMStoreFloat4(&v4, vec);
    return Vec3(v4.x, v4.y, -v4.z, v4.w);
}

// ----------------------------------------------------------------------------------------------------------------------------

void GenerateRenderListFromWorld(std::shared_ptr<CommandList> commandList, const IHitable* currentHead, Texture& defaultTexture,
    std::vector<RenderSceneNode*>& outSceneList, std::vector<SpotLight>& spotLightsList, std::vector<DirectX::XMMATRIX>& matrixStack, std::vector<bool>& flipNormalStack)
{
    const std::type_info& tid = typeid(*currentHead);

    if (tid == typeid(HitableList))
    {
        HitableList* hitList = (HitableList*)currentHead;

        IHitable** list = hitList->GetList();
        const int  listSize = hitList->GetListSize();
        for (int i = 0; i < listSize; i++)
        {
            GenerateRenderListFromWorld(commandList, list[i], defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        }
    }
    else if (tid == typeid(BVHNode))
    {
        BVHNode* bvhNode = (BVHNode*)currentHead;

        GenerateRenderListFromWorld(commandList, bvhNode->GetLeft(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        if (bvhNode->GetLeft() != bvhNode->GetRight())
        {
            GenerateRenderListFromWorld(commandList, bvhNode->GetRight(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        }
    }
    else if (tid == typeid(HitableTranslate))
    {
        HitableTranslate* translateHitable = (HitableTranslate*)currentHead;
        Vec3              offset           = translateHitable->GetOffset();
        XMMATRIX          translation      = XMMatrixTranslation(offset.X(), offset.Y(), -offset.Z());

        matrixStack.push_back(translation);
        GenerateRenderListFromWorld(commandList, translateHitable->GetHitObject(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        matrixStack.pop_back();
    }
    else if (tid == typeid(HitableRotateY))
    {
        HitableRotateY*   rotateYHitable = (HitableRotateY*)currentHead;
        XMMATRIX          rotation       = XMMatrixRotationY(XMConvertToRadians(-rotateYHitable->GetAngleDegrees()));

        matrixStack.push_back(rotation);
        GenerateRenderListFromWorld(commandList, rotateYHitable->GetHitObject(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        matrixStack.pop_back();
    }
    else if (tid == typeid(FlipNormals))
    {
        FlipNormals* flipNormals = (FlipNormals*)currentHead;
        flipNormalStack.push_back(true);
        GenerateRenderListFromWorld(commandList, flipNormals->GetHitObject(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        flipNormalStack.pop_back();
    }
    else if (tid == typeid(Sphere) || tid == typeid(MovingSphere))
    {
        Vec3  offset;
        float radius;
        Material* material = nullptr;
        if (tid == typeid(Sphere))
        {
            Sphere* sphere = (Sphere*)currentHead;
            offset   = sphere->GetCenter();
            radius   = sphere->GetRadius();
            material = sphere->GetMaterial();
        }
        else if (tid == typeid(MovingSphere))
        {
            MovingSphere* sphere = (MovingSphere*)currentHead;
            offset   = sphere->Center(1.f);
            radius   = sphere->GetRadius();
            material = sphere->GetMaterial();
        }

        XMMATRIX         translation = XMMatrixTranslation(offset.X(), offset.Y(), -offset.Z());
        XMMATRIX         newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RenderSceneNode* newNode     = new RenderSceneNode();

        Vec3 color = material->AlbedoValue(0.5f, 0.5f, Vec3(0, 0, 0));
        const RenderMaterial newMaterial =
        {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { color.X(), color.Y(), color.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            128.0f
        };

        newNode->MeshData       = Mesh::CreateSphere(*commandList, radius * 2.f);
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = newMaterial;
        newNode->DiffuseTexture = &defaultTexture;
        newNode->Hitable        = currentHead;
        outSceneList.push_back(newNode);
    }
    else if (tid == typeid(HitableBox))
    {
        HitableBox* box = (HitableBox*)currentHead;

        Vec3 minP, maxP;
        box->GetPoints(minP, maxP);

        Vec3             diff        = maxP - minP;
        Vec3             offset      = minP + (diff * 0.5f);
        XMMATRIX         translation = XMMatrixTranslation(offset.X(), offset.Y(), -offset.Z());
        XMMATRIX         newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        XMFLOAT3         sideLengths = XMFLOAT3(fabs(diff.X()), fabs(diff.Y()), fabs(diff.Z()));
        RenderSceneNode* newNode     = new RenderSceneNode();
        const Material*  material    = box->GetMaterial();

        Vec3 color = material->AlbedoValue(0.5f, 0.5f, Vec3(0, 0, 0));
        const RenderMaterial newMaterial =
        {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { color.X(), color.Y(), color.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            128.0f
        };

        newNode->MeshData       = Mesh::CreateCube(*commandList, sideLengths);
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = newMaterial;
        newNode->DiffuseTexture = &defaultTexture;
        newNode->Hitable        = currentHead;
        outSceneList.push_back(newNode);
    }
    else if (tid == typeid(TriMesh))
    {
        TriMesh*         triMesh   = (TriMesh*)currentHead;
        IHitable**       triArray  = nullptr;
        int              numTris   = 0;
        VertexCollection vertices;
        IndexCollection  indices;
        
        // Walk through all the triangles
        triMesh->GetTriArray(triArray, numTris);
        for (int t = 0; t < numTris; t++)
        {
            const Triangle* tri = (const Triangle*)triArray[t];

            const Triangle::Vertex* triVertices = tri->GetVertices();
            for (int v = 0; v < 3; v++)
            {
                float s = 1.f - triVertices[v].UV[0];
                float t = 1.f - triVertices[v].UV[1];

                XMVECTOR position = ConvertToXMVector(triVertices[v].Vert);
                XMVECTOR normal   = ConvertToXMVector(triVertices[v].Normal);
                XMVECTOR texCoord = XMVectorSet(s, t, 0, 0);

                vertices.push_back(VertexPositionNormalTexture(position, normal, texCoord));
                indices.push_back(uint32_t(vertices.size() - 1));
            }
        }

        XMMATRIX         translation = XMMatrixIdentity();
        XMMATRIX         newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RenderSceneNode* newNode     = new RenderSceneNode();

        Texture* newTexture = &defaultTexture;
        if (typeid(*triMesh->GetMaterial()) == typeid(MWavefrontObj))
        {
            std::string  fileName = ((MWavefrontObj*)triMesh->GetMaterial())->GetDiffuseMap()->GetSourceFilename().c_str();
            std::wstring widestr = std::wstring(fileName.begin(), fileName.end());
            newTexture = new Texture();
            commandList->LoadTextureFromFile(*newTexture, widestr.c_str());
        }

        newNode->MeshData       = Mesh::CreateFromCollection(*commandList, vertices, indices);
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = MaterialWhite;
        newNode->DiffuseTexture = newTexture;
        newNode->Hitable        = currentHead;
        outSceneList.push_back(newNode);
    }
    else if (tid == typeid(XYZRect))
    {
        XYZRect*  xyzRect = (XYZRect*)currentHead;
        Vec3      planePoints[4];
        Vec3      normal;
        XMFLOAT3  xmPlanePoints[4];
        XMFLOAT3  xmNormal;
        xyzRect->GetPlaneData(planePoints, normal);
        bool normalFlipped = FlipFromNormalStack(normal, flipNormalStack);
        for (int i = 0; i < 4; i++)
        {
            int dst = normalFlipped ? i : (4 - i - 1);
            xmPlanePoints[dst] = XMFLOAT3(planePoints[i].X(), planePoints[i].Y(), -planePoints[i].Z());
        }
        xmNormal = XMFLOAT3(normal.X(), normal.Y(), -normal.Z());
        
        const Material*  material    = xyzRect->GetMaterial();
        XMMATRIX         translation = XMMatrixIdentity();
        XMMATRIX         newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RenderSceneNode* newNode     = new RenderSceneNode();

        // Clamp colors for lights, since they can overblow the color buffer
        Vec3 color = material->AlbedoValue(0.5f, 0.5f, Vec3(0, 0, 0));
        color.Clamp(0.f, 1.f);

        // Light shapes should not get lit, and always show up fully shaded
        Vec3 em = xyzRect->IsALightShape() ? Vec3(1.f, 1.f, 1.f, 1.f) : Vec3(0.0f, 0.0f, 0.0f, 1.0f);

        const RenderMaterial newMaterial =
        {
            { em.X(), em.Y(), em.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { color.X(), color.Y(), color.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            128.0f
        };

        newNode->MeshData       = Mesh::CreatePlaneFromPoints(*commandList, xmPlanePoints, xmNormal);
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = newMaterial;
        newNode->DiffuseTexture = &defaultTexture;
        newNode->Hitable        = currentHead;
        outSceneList.push_back(newNode);
        
        // These can be light shapes too
        if (xyzRect->IsALightShape())
        {
            if (xyzRect->GetAxisPlane() == XYZRect::AxisPlane::XZ)
            {
                float xValues[2], yValue, zValues[2];
                xyzRect->GetParams(
                    xValues[0], xValues[1],
                    zValues[0], zValues[1],
                    yValue);

                Vec3 pos(
                    (xValues[0] + xValues[1]) * 0.5f,
                    yValue,
                    (zValues[0] + zValues[1]) * 0.5f);
                
                Vec3  upDir   = Vec3(1.f, 0.f, 1.f);
                Vec3  lookAt  = Vec3(pos[0], -yValue, pos[2]);
                Vec3  dir     = UnitVector(lookAt - pos);

                // Pullback the camera for shadowmap rendering
                float smapDist = sShadowmapFar - fabs(yValue);
                Vec3  smapPos  = pos - (dir * smapDist);

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
                light.Color                = ConvertToXMFloat4(color, false);

                spotLightsList.push_back(light);
            }
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static void UpdateCameras(float forwardAmount, float strafeAmount, float upDownAmount, int mouseDx, int mouseDy, Camera& raytracerCamera, CameraDX12& renderCamera)
{
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

    // Get ray tracer camera params
    Vec3   lookFrom, lookAt, up;
    float  vertFov, aspect, aperture, focusDist, t0, t1;
    Vec3   clearColor;
    raytracerCamera.GetCameraParams(lookFrom, lookAt, up, vertFov, aspect, aperture, focusDist, t0, t1, clearColor);

    // Update render camera look at
    Vec3  diffVec = (lookAt - lookFrom);
    Vec3  viewDir = diffVec.MakeUnitVector();
    Vec3  rightDir = Cross(up, viewDir).MakeUnitVector();
    float upAngle = mouseDx * -.1f;
    float rightAngle = mouseDy * -.1f;

    // Rotate around up axis
    viewDir = Quat::RotateVector(viewDir, up, upAngle);
    rightDir = Quat::RotateVector(rightDir, up, upAngle);

    // Rotate around right axis
    viewDir = Quat::RotateVector(viewDir, rightDir, rightAngle);

    Vec3 cameraTranslate = (viewDir * forwardAmount) + (rightDir * strafeAmount) + (up * upDownAmount);
    lookFrom += cameraTranslate;
    lookAt = lookFrom + (viewDir * diffVec.Length());

    // Update raytracer camera
    raytracerCamera.Setup(lookFrom, lookAt, up, vertFov, aspect, aperture, focusDist, t0, t1, clearColor);
    raytracerCamera.SetFocusDistanceToLookAt();

    // Set the render camera
    XMVECTOR cameraPos = ConvertToXMVector(lookFrom);
    XMVECTOR cameraTarget = ConvertToXMVector(lookAt);
    XMVECTOR cameraUp = ConvertToXMVector(up);
    renderCamera.set_LookAt(cameraPos, cameraTarget, cameraUp);
    renderCamera.set_Projection(vertFov, aspect, 0.1f, 10000.0f);

    // Print out camera changes
    if (mouseMoved)
    {
        DEBUG_PRINTF("lookFrom:(%f, %f, %f)  lookAt:(%f, %f, %f)  up:(%f, %f, %f)  vertFov:%f  aspect:%f  aperture:%f  focusDist:%f\n",
            lookFrom[0], lookFrom[1], lookFrom[2],
            lookAt[0], lookAt[1], lookAt[2],
            up[0], up[1], up[2],
            vertFov, aspect, aperture, focusDist
        );
    }
}
