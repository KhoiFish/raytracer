#include "SampleScenes.h"

#include "Core/Vec3.h"
#include "Core/Ray.h"
#include "Core/Sphere.h"
#include "Core/MovingSphere.h"
#include "Core/HitableList.h"
#include "Core/Util.h"
#include "Core/Material.h"
#include "Core/BVHNode.h"
#include "Core/Texture.h"
#include "Core/XYZRect.h"
#include "Core/FlipNormals.h"
#include "Core/HitableBox.h"
#include "Core/HitableTransform.h"
#include "Core/ConstantMedium.h"
#include "Core/Triangle.h"
#include "Core/TriMesh.h"

#include "StbImage/stb_image.h"

// ----------------------------------------------------------------------------------------------------------------------------

Camera GetCameraForSample(SampleScene scene, float aspect)
{
    switch (scene)
    {
        case SceneRandom:
        {
            const Vec3   lookFrom = Vec3(13, 2, 3);
            const Vec3   lookAt = Vec3(0, 0, 0);
            const Vec3   upVec = Vec3(0, 1, 0);
            const float  vertFov = 20.f;
            const float  aperture = 0.0f;
            const float  distToFocus = 10.f;
            const float  shutterTime0 = 0.f;
            const float  shutterTime1 = 1.f;
            const Vec3   clearColor(.7f, .7f, .7f);

            return Camera(
                lookFrom, lookAt, upVec,
                vertFov, aspect, aperture, distToFocus,
                shutterTime0, shutterTime1, clearColor);
        }
        break;

        case SceneCornell:
        case SceneCornellSmoke:
        {
            // Camera options
            const Vec3   lookFrom = Vec3(278, 278, -800);
            const Vec3   lookAt = Vec3(278, 278, 0);
            const Vec3   upVec = Vec3(0, 1, 0);
            const float  vertFov = 40.f;
            const float  aperture = 0.0f;
            const float  distToFocus = 10.f;
            const float  shutterTime0 = 0.f;
            const float  shutterTime1 = 1.f;
            const Vec3   clearColor(0, 0, 0);

            return Camera(
                lookFrom, lookAt, upVec,
                vertFov, aspect, aperture, distToFocus,
                shutterTime0, shutterTime1, clearColor);
        }
        break;

        case SceneFinal:
        {
            // Camera options
            const Vec3   lookFrom = Vec3(478, 278, -600);
            const Vec3   lookAt = Vec3(278, 278, 0);
            const Vec3   upVec = Vec3(0, 1, 0);
            const float  vertFov = 40.f;
            const float  aperture = 0.0f;
            const float  distToFocus = 10.f;
            const float  shutterTime0 = 0.f;
            const float  shutterTime1 = 1.f;
            const Vec3   clearColor(0, 0, 0);

            return Camera (
                lookFrom, lookAt, upVec,
                vertFov, aspect, aperture, distToFocus,
                shutterTime0, shutterTime1, clearColor);
        }
        break;

        default:
        {
            const Vec3   lookFrom = Vec3(13, 2, 3);
            const Vec3   lookAt = Vec3(0, 0, 0);
            const Vec3   upVec = Vec3(0, 1, 0);
            const float  vertFov = 20.f;
            const float  aperture = 0.0f;
            const float  distToFocus = 10.f;
            const float  shutterTime0 = 0.f;
            const float  shutterTime1 = 1.f;
            const Vec3   clearColor(.7f, .7f, .7f);

            return Camera(
                lookFrom, lookAt, upVec,
                vertFov, aspect, aperture, distToFocus,
                shutterTime0, shutterTime1, clearColor);
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

WorldScene* SampleSceneRandom(const Camera& cam)
{
    float time0, time1;
    cam.GetShutterTime(time0, time1);

    const int numObjects = 500;
    IHitable **list = new IHitable*[numObjects + 1];

    BaseTexture* checker = new CheckerTexture(
        new ConstantTexture(Vec3(.2f, .3f, .1f)),
        new ConstantTexture(Vec3(.9f, .9f, .9f))
    );
    list[0] = new Sphere(Vec3(0, -1000, 0), 1000, new MLambertian(checker));

    int i = 1;
    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            float chooseMat = RandomFloat();
            Vec3 center(a + .9f * RandomFloat(), .2f, b + .9f * RandomFloat());
            if ((center - Vec3(4, .2f, 0)).Length() > .9f)
            {
                if (chooseMat < .8f)
                {
                    list[i++] = new MovingSphere(
                        center, center + Vec3(0, .5f*RandomFloat(), 0),
                        .0f, 1.f,
                        .2f,
                        new MLambertian(new ConstantTexture(Vec3(RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat()))
                        )
                    );
                }
                else if (chooseMat < .95f)
                {
                    list[i++] = new Sphere(center, .2f, new MMetal(Vec3(.5f * (1 + RandomFloat()), .5f * (1 + RandomFloat()), .5f * (1 + RandomFloat())), .5f * RandomFloat()));
                }
                else
                {
                    list[i++] = new Sphere(center, .2f, new MDielectric(1.5f));
                }
            }
        }
    }

    list[i++] = new Sphere(Vec3(0, 1, 0), 1.f, new MDielectric(1.5f));
    list[i++] = new Sphere(Vec3(-4, 1, 0), 1.f, new MLambertian(new ConstantTexture(Vec3(0.4f, 0.2f, 0.1f))));
    list[i++] = new Sphere(Vec3(4, 1, 0), 1.f, new MMetal(Vec3(0.7f, 0.6f, 0.5f), 0.f));

    // Generate BVH tree
    BVHNode* bvhHead = new BVHNode(list, i, time0, time1);

    return WorldScene::Create(bvhHead);
}

// ----------------------------------------------------------------------------------------------------------------------------

WorldScene* SampleSceneCreateTwoPerlinSpheres()
{
    BaseTexture* perTex = new NoiseTexture(4.f);
    BaseTexture* imageTex = nullptr;
    {
        int width, height, numChannels;
        unsigned char* texData = stbi_load("RuntimeData/guitar.jpg", &width, &height, &numChannels, 0);
        imageTex = new ImageTexture(texData, width, height);
    }

    IHitable **list = new IHitable*[2];
    list[0] = new Sphere(Vec3(0, -1000, 0), 1000, new MLambertian(perTex));
    list[1] = new Sphere(Vec3(0, 2, 0), 2, new MLambertian(imageTex));

    return WorldScene::Create(list, 2);
}

// ----------------------------------------------------------------------------------------------------------------------------

WorldScene* SampleSceneSimpleLight()
{
    BaseTexture* perlinTex = new NoiseTexture(4);
    IHitable** list = new IHitable*[4];
    list[0] = new Sphere(Vec3(0, -1000, 0), 1000, new MLambertian(perlinTex));
    list[1] = new Sphere(Vec3(0, 2, 0), 2, new MLambertian(perlinTex));
    list[2] = new Sphere(Vec3(0, 7, 0), 2, new MDiffuseLight(new ConstantTexture(Vec3(4, 4, 4))));
    list[3] = new XYZRect(XYZRect::AxisPlane::XY, 3, 5, 1, 3, -2, new MDiffuseLight(new ConstantTexture(Vec3(4, 4, 4))));

    return WorldScene::Create(list, 4);
}

// ----------------------------------------------------------------------------------------------------------------------------

WorldScene* SampleSceneCornellBox(bool smoke)
{
    IHitable** list = new IHitable*[8];
    int i = 0;

    IHitable** lsList = new IHitable*[2];
    int numLs = 0;

    Material* red   = new MLambertian(new ConstantTexture(Vec3(.65f, .05f, .05f)));
    Material* white = new MLambertian(new ConstantTexture(Vec3(.73f, .73f, .73f)));
    Material* green = new MLambertian(new ConstantTexture(Vec3(.12f, .45f, .15f)));
    Material* light = new MDiffuseLight(new ConstantTexture(Vec3(15, 15, 15)));
    Material* glass = new MDielectric(1.5f);

    list[i++] = new FlipNormals(new XYZRect(XYZRect::YZ, 0, 555, 0, 555, 555, green));
    list[i++] = new XYZRect(XYZRect::YZ, 0, 555, 0, 555, 0, red);

    IHitable* lightShape = new XYZRect(XYZRect::XZ, 213, 343, 227, 332, 554, light, true);
    list[i++] = new FlipNormals(lightShape);
    lsList[numLs++] = lightShape;

    list[i++] = new FlipNormals(new XYZRect(XYZRect::XZ, 0, 555, 0, 555, 555, white));
    list[i++] = new XYZRect(XYZRect::XZ, 0, 555, 0, 555, 0, white);
    list[i++] = new FlipNormals(new XYZRect(XYZRect::XY, 0, 555, 0, 555, 555, white));

    IHitable* glassSphere = new Sphere(Vec3(190, 90, 190), 90, glass, true);
    list[i++] = glassSphere;
    lsList[numLs++] = glassSphere;

    IHitable* box2 = new HitableTranslate(
        new HitableRotateY(
            new HitableBox(Vec3(0, 0, 0), Vec3(165, 330, 165), white),
            15),
        Vec3(265, 0, 295));

    if (smoke)
    {
        list[i++] = new ConstantMedium(box2, 0.01f, new ConstantTexture(Vec3(0.f, 0.f, 0.f)));
    }
    else
    {
        list[i++] = box2;
    }

    return WorldScene::Create(list, i, lsList, numLs);
}

// ----------------------------------------------------------------------------------------------------------------------------
#if 0
WorldScene* SampleSceneMesh()
{
    IHitable** list = new IHitable*[16];
    int i = 0;

    IHitable** lsList = new IHitable*[2];
    int numLs = 0;

    Material* red   = new MLambertian(new ConstantTexture(Vec3(.65f, .05f, .05f)));
    Material* white = new MLambertian(new ConstantTexture(Vec3(.73f, .73f, .73f)));
    Material* green = new MLambertian(new ConstantTexture(Vec3(.12f, .45f, .15f)));
    Material* light = new MDiffuseLight(new ConstantTexture(Vec3(15, 15, 15)));
    Material* glass = new MDielectric(1.5f);
    Material* metal = new MMetal(Vec3(0.8f, 0.8f, 0.9f), 10.0f);

    list[i++] = new FlipNormals(new XYZRect(XYZRect::YZ, 0, 555, 0, 555, 555, green));
    list[i++] = new XYZRect(XYZRect::YZ, 0, 555, 0, 555, 0, red);

    IHitable* lightShape = new XYZRect(XYZRect::XZ, 213, 343, 227, 332, 554, light, true);
    list[i++] = new FlipNormals(lightShape);
    lsList[numLs++] = lightShape;

    list[i++] = new FlipNormals(new XYZRect(XYZRect::XZ, 0, 555, 0, 555, 555, white));
    list[i++] = new XYZRect(XYZRect::XZ, 0, 555, 0, 555, 0, white);
    list[i++] = new FlipNormals(new XYZRect(XYZRect::XY, 0, 555, 0, 555, 555, white));
    
    IHitable *triMesh =
        new HitableTranslate(
            new HitableRotateY(
                TriMesh::CreateFromOBJFile("RuntimeData/r8.obj", white, 20.f),
                35.f
            ),
            Vec3(280, 100, 100)
        );

    list[i++] = triMesh;
    //lsList[numLs++] = triMesh;

    return WorldScene::Create(list, i, lsList, numLs);
}
#else

WorldScene* SampleSceneMesh()
{
    const int numBoxes = 5;
    int total = 0;

    IHitable** list = new IHitable*[30];
    IHitable** boxlist = new IHitable*[10000];
    IHitable** boxlist2 = new IHitable*[10000];
    Material*  white = new MLambertian(new ConstantTexture(Vec3(0.73f, 0.73f, 0.73f)));
    Material*  ground = new MLambertian(new ConstantTexture(Vec3(0.48f, 0.83f, 0.53f)));

    IHitable** lsList = new IHitable*[2];
    int numLs = 0;

    // Create random hitable boxes, in BVH tree
    list[total++] = new HitableBox(Vec3(-1000, -100, -1000), Vec3(1000, 100, 1000), ground);

    // Create light
    {
        Material *lightMat = new MDiffuseLight(new ConstantTexture(Vec3(7, 7, 7)));
        IHitable *lightShape = new FlipNormals(new XYZRect(XYZRect::XZ, 123, 423, 147, 412, 554, lightMat, true));
        list[total++] = lightShape;
        lsList[numLs++] = lightShape;
    }

    // Volumes
    {
        IHitable *boundary =
            new HitableTranslate(
                new HitableRotateY(
                    TriMesh::CreateFromOBJFile("RuntimeData/r8.obj", new MDielectric(1.5f), 20.f),
                    35.f
                ),
                Vec3(130, 150, 145)
            );
        list[total++] = boundary;
        list[total++] = new ConstantMedium(boundary, 0.2f, new ConstantTexture(Vec3(0.2f, 0.4f, 0.9f)));


        //boundary = new Sphere(Vec3(0, 0, 0), 5000, new MDielectric(1.5f));
        //list[total++] = new ConstantMedium(boundary, 0.0001f, new ConstantTexture(Vec3(1.0f, 1.0f, 1.0f)));
    }

    return WorldScene::Create(list, total, lsList, numLs);
}


#endif

// ----------------------------------------------------------------------------------------------------------------------------

WorldScene* SampleSceneFinal()
{
    const int numBoxes = 20;
    int total = 0;

    IHitable** list = new IHitable*[30];
    IHitable** boxlist = new IHitable*[10000];
    IHitable** boxlist2 = new IHitable*[10000];
    Material*  white = new MLambertian(new ConstantTexture(Vec3(0.73f, 0.73f, 0.73f)));
    Material*  ground = new MLambertian(new ConstantTexture(Vec3(0.48f, 0.83f, 0.53f)));
    IHitable** lsList = new IHitable*[2];
    int numLs = 0;

    // Create random hitable boxes, in BVH tree
    {
        int b = 0;
        for (int i = 0; i < numBoxes; i++)
        {
            for (int j = 0; j < numBoxes; j++)
            {
                float w = 100;
                float x0 = -1000 + i * w;
                float z0 = -1000 + j * w;
                float y0 = 0;
                float x1 = x0 + w;
                float y1 = 100 * (RandomFloat() + 0.01f);
                float z1 = z0 + w;

                boxlist[b++] = new HitableBox(Vec3(x0, y0, z0), Vec3(x1, y1, z1), ground);
            }
        }
        list[total++] = new BVHNode(boxlist, b, 0, 1);
    }

    // Create light
    {
        Material* lightMaterial = new MDiffuseLight(new ConstantTexture(Vec3(7, 7, 7)));
        IHitable* lightShape = new XYZRect(XYZRect::XZ, 123, 423, 147, 412, 554, lightMaterial, true);
        list[total++] = new FlipNormals(lightShape);
        lsList[numLs++] = lightShape;
    }

    // Moving sphere
    {
        Vec3 center(400, 400, 200);
        list[total++] = new MovingSphere(center, center + Vec3(30, 0, 0), 0, 1, 50, new MLambertian(new ConstantTexture(Vec3(0.7f, 0.3f, 0.1f))));
    }

    // Dielectric and metal spheres
    {
        IHitable* newDielectricSphere = new Sphere(Vec3(260, 150, 45), 50, new MDielectric(1.5f));
        list[total++]   = newDielectricSphere;
        lsList[numLs++] = newDielectricSphere;

        list[total++] = new Sphere(Vec3(0, 150, 145), 50, new MMetal(Vec3(0.8f, 0.8f, 0.9f), 10.0f));
    }

    // Volumes
    {
        IHitable *boundary = new Sphere(Vec3(360, 150, 145), 70, new MDielectric(1.5f));
        list[total++] = boundary;
        list[total++] = new ConstantMedium(boundary, 0.2f, new ConstantTexture(Vec3(0.2f, 0.4f, 0.9f)));

        boundary = new Sphere(Vec3(0, 0, 0), 5000, new MDielectric(1.5f));
        list[total++] = new ConstantMedium(boundary, 0.0001f, new ConstantTexture(Vec3(1.0f, 1.0f, 1.0f)));
    }

    // Image texture sphere
    {
        int nx, ny, nn;
        unsigned char *texData = stbi_load("RuntimeData/guitar.jpg", &nx, &ny, &nn, 0);
        Material *emat = new MLambertian(new ImageTexture(texData, nx, ny));
        list[total++] = new Sphere(Vec3(400, 200, 400), 100, emat);
    }

    // Perlin noise sphere
    {
        BaseTexture *perTex = new NoiseTexture(0.1f);
        list[total++] = new Sphere(Vec3(220, 280, 300), 80, new MLambertian(perTex));
    }

    // Translated, rotated spheres in BVH tree
    {
        int ns = 1000;
        for (int j = 0; j < ns; j++)
        {
            boxlist2[j] = new Sphere(Vec3(165 * RandomFloat(), 165 * RandomFloat(), 165 * RandomFloat()), 10, white);
        }
        list[total++] = new HitableTranslate(new HitableRotateY(new BVHNode(boxlist2, ns, 0.0, 1.0), 15), Vec3(-100, 270, 395));
    }

    return WorldScene::Create(list, total, lsList, numLs);
}
