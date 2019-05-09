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
    // Negate Z
    return XMVectorSet(vec.X(), vec.Y(), -vec.Z(), vec.W());
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline XMFLOAT4 ConvertToXMFloat4(const Core::Vec4& vec, bool negateZ = true)
{
    return DirectX::XMFLOAT4(vec[0], vec[1], negateZ ? -vec[2] : vec[2], 1.f);
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline Core::Vec4 ConvertToVec4(const XMVECTOR& vec)
{
    // Negate Z
    XMFLOAT4 v4;
    XMStoreFloat4(&v4, vec);
    return Core::Vec4(v4.x, v4.y, -v4.z, v4.w);
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline void CreateSphere(RenderSceneNode* renderNode, float radius)
{
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline void CreateCube(RenderSceneNode* renderNode, Core::Vec4 sideLengths)
{
}

// ----------------------------------------------------------------------------------------------------------------------------

static inline void CreatePlaneFromPoints(const Core::Vec4* planePoints, const Core::Vec4& normal)
{
}

// ----------------------------------------------------------------------------------------------------------------------------

void GenerateRenderListFromWorld(const Core::IHitable* currentHead, const RealtimeEngine::Texture* defaultTexture, std::vector<RenderSceneNode*>& outSceneList, std::vector<SpotLight>& spotLightsList, std::vector<DirectX::XMMATRIX>& matrixStack, std::vector<bool>& flipNormalStack)
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
        XMMATRIX                translation      = XMMatrixTranslation(offset.X(), offset.Y(), -offset.Z());

        matrixStack.push_back(translation);
        GenerateRenderListFromWorld(translateHitable->GetHitObject(), defaultTexture, outSceneList, spotLightsList, matrixStack, flipNormalStack);
        matrixStack.pop_back();
    }
    else if (tid == typeid(Core::HitableRotateY))
    {
        Core::HitableRotateY*   rotateYHitable = (Core::HitableRotateY*)currentHead;
        XMMATRIX                rotation       = XMMatrixRotationY(XMConvertToRadians(-rotateYHitable->GetAngleDegrees()));

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
        float           radius;
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

        XMMATRIX         translation = XMMatrixTranslation(offset.X(), offset.Y(), -offset.Z());
        XMMATRIX         newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RenderSceneNode* newNode     = new RenderSceneNode();

        Core::Vec4 color = material->AlbedoValue(0.5f, 0.5f, Core::Vec4(0, 0, 0));
        const RenderMaterial newMaterial =
        {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { color.X(), color.Y(), color.Z(), 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            128.0f
        };

        CreateSphere(newNode, radius * 2.f);
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = newMaterial;
        newNode->DiffuseTexture = defaultTexture;
        newNode->Hitable        = currentHead;
        outSceneList.push_back(newNode);
    }
    else if (tid == typeid(Core::HitableBox))
    {
        Core::HitableBox* box = (Core::HitableBox*)currentHead;

        Core::Vec4 minP, maxP;
        box->GetPoints(minP, maxP);

        Core::Vec4             diff        = maxP - minP;
        Core::Vec4             offset      = minP + (diff * 0.5f);
        XMMATRIX               translation = XMMatrixTranslation(offset.X(), offset.Y(), -offset.Z());
        XMMATRIX               newMatrix   = ComputeFinalMatrix(matrixStack, translation);
        RenderSceneNode*       newNode     = new RenderSceneNode();
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
        outSceneList.push_back(newNode);
    }
    else if (tid == typeid(Core::TriMesh))
    {
        Core::TriMesh*    triMesh   = (Core::TriMesh*)currentHead;
        Core::IHitable**  triArray  = nullptr;
        int               numTris   = 0;
        VertexBuffer      vertices;
        IndexBuffer       indices;
        
        // Walk through all the triangles
        triMesh->GetTriArray(triArray, numTris);
        for (int t = 0; t < numTris; t++)
        {
            const Core::Triangle* tri = (const Core::Triangle*)triArray[t];

            const Core::Triangle::Vertex* triVertices = tri->GetVertices();
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

        const RealtimeEngine::Texture* newTexture = defaultTexture;
        if (typeid(*triMesh->GetMaterial()) == typeid(Core::MWavefrontObj))
        {
            std::string  fileName = ((Core::MWavefrontObj*)triMesh->GetMaterial())->GetDiffuseMap()->GetSourceFilename().c_str();
            newTexture = RealtimeEngine::TextureManager::LoadFromFile(fileName);
        }

        newNode->VertexData     = vertices;
        newNode->IndexData      = indices;
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = MaterialWhite;
        newNode->DiffuseTexture = newTexture;
        newNode->Hitable        = currentHead;
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
        RenderSceneNode*        newNode     = new RenderSceneNode();

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

        CreatePlaneFromPoints(planePoints, normal);
        newNode->WorldMatrix    = newMatrix;
        newNode->Material       = newMaterial;
        newNode->DiffuseTexture = defaultTexture;
        newNode->Hitable        = currentHead;
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
                light.Color                = ConvertToXMFloat4(color, false);

                spotLightsList.push_back(light);
            }
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static void UpdateCameras(float newVertFov, float forwardAmount, float strafeAmount, float upDownAmount, int mouseDx, int mouseDy, Core::Camera& raytracerCamera) //, CameraDX12& renderCamera)
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
    Core::Vec4   lookFrom, lookAt, up;
    float        vertFov, aspect, aperture, focusDist, t0, t1;
    Core::Vec4   clearColor;
    raytracerCamera.GetCameraParams(lookFrom, lookAt, up, vertFov, aspect, aperture, focusDist, t0, t1, clearColor);

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
    raytracerCamera.Setup(lookFrom, lookAt, up, newVertFov, aspect, aperture, focusDist, t0, t1, clearColor);
    raytracerCamera.SetFocusDistanceToLookAt();

    // Set the render camera
    XMVECTOR cameraPos    = ConvertToXMVector(lookFrom);
    XMVECTOR cameraTarget = ConvertToXMVector(lookAt);
    XMVECTOR cameraUp     = ConvertToXMVector(up);
    //renderCamera.set_LookAt(cameraPos, cameraTarget, cameraUp);
    //renderCamera.set_Projection(newVertFov, aspect, 0.1f, 10000.0f);

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
}
