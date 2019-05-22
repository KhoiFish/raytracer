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

#include "SampleScenes.h"

#include "Core/Vec4.h"
#include "Core/Ray.h"
#include "Core/Sphere.h"
#include "Core/MovingSphere.h"
#include "Core/HitableList.h"
#include "Core/Util.h"
#include "Core/Material.h"
#include "Core/BVHNode.h"
#include "Core/CoreTexture.h"
#include "Core/XYZRect.h"
#include "Core/FlipNormals.h"
#include "Core/HitableBox.h"
#include "Core/HitableTransform.h"
#include "Core/ConstantMedium.h"
#include "Core/CoreTriangle.h"
#include "Core/TriMesh.h"

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

static Camera getCameraForSample(SampleScene scene)
{
    float aspect = 1.f;

    switch (scene)
    {
        case SceneRandom:
        {
            const Vec4   lookFrom = Vec4(13, 2, 3);
            const Vec4   lookAt = Vec4(0, 0, 0);
            const Vec4   upVec = Vec4(0, 1, 0);
            const float  vertFov = 20.f;
            const float  aperture = 0.0f;
            const float  distToFocus = 10.f;
            const float  shutterTime0 = 0.f;
            const float  shutterTime1 = 1.f;
            const Vec4   clearColor(.7f, .7f, .7f);

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
            const Vec4   lookFrom = Vec4(278, 278, -800);
            const Vec4   lookAt = Vec4(278, 278, 0);
            const Vec4   upVec = Vec4(0, 1, 0);
            const float  vertFov = 40.f;
            const float  aperture = 0.0f;
            const float  distToFocus = 10.f;
            const float  shutterTime0 = 0.f;
            const float  shutterTime1 = 1.f;
            const Vec4   clearColor(0, 0, 0);

            return Camera(
                lookFrom, lookAt, upVec,
                vertFov, aspect, aperture, distToFocus,
                shutterTime0, shutterTime1, clearColor);
        }
        break;

        case SceneFinal:
        {
            // Camera options
            const Vec4   lookFrom = Vec4(478, 278, -600);
            const Vec4   lookAt = Vec4(278, 278, 0);
            const Vec4   upVec = Vec4(0, 1, 0);
            const float  vertFov = 40.f;
            const float  aperture = 0.0f;
            const float  distToFocus = 10.f;
            const float  shutterTime0 = 0.f;
            const float  shutterTime1 = 1.f;
            const Vec4   clearColor(0, 0, 0);

            return Camera (
                lookFrom, lookAt, upVec,
                vertFov, aspect, aperture, distToFocus,
                shutterTime0, shutterTime1, clearColor);
        }
        break;

        // lookFrom:(-495.333893, 303.848877, -828.657288)  lookAt:(-494.744324, 303.853485, -827.849609)  up:(0.000000, 1.000000, 0.000000)  vertFov:40.000000  aspect:1.777778  aperture:0.000000  focusDist:90.000000
        case SceneMesh:
        {
            // Camera options
            const Vec4   lookFrom = Vec4(-495.333893f, 303.848877f, -828.657288f);
            const Vec4   lookAt   = Vec4(-494.744324f, 303.853485f, -827.849609f);
            const Vec4   upVec = Vec4(0, 1, 0);
            const float  vertFov = 40.f;
            const float  aperture = 0.0f;
            const float  distToFocus = 90.f;
            const float  shutterTime0 = 0.f;
            const float  shutterTime1 = 1.f;
            const Vec4   clearColor(0, 0, 0);

            return Camera(
                lookFrom, lookAt, upVec,
                vertFov, aspect, aperture, distToFocus,
                shutterTime0, shutterTime1, clearColor);
        }
        break;

        default:
        {
            const Vec4   lookFrom = Vec4(13, 2, 3);
            const Vec4   lookAt = Vec4(0, 0, 0);
            const Vec4   upVec = Vec4(0, 1, 0);
            const float  vertFov = 20.f;
            const float  aperture = 0.0f;
            const float  distToFocus = 10.f;
            const float  shutterTime0 = 0.f;
            const float  shutterTime1 = 1.f;
            const Vec4   clearColor(.7f, .7f, .7f);

            return Camera(
                lookFrom, lookAt, upVec,
                vertFov, aspect, aperture, distToFocus,
                shutterTime0, shutterTime1, clearColor);
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

static WorldScene* sampleSceneRandom()
{
    Camera cam = getCameraForSample(SceneRandom);

    float time0, time1;
    cam.GetShutterTime(time0, time1);

    const int numObjects = 500;
    IHitable **list = new IHitable*[numObjects + 1];

    BaseTexture* checker = new CheckerTexture(
        new ConstantTexture(Vec4(.2f, .3f, .1f)),
        new ConstantTexture(Vec4(.9f, .9f, .9f))
    );
    list[0] = new Sphere(Vec4(0, -1000, 0), 1000, new MLambertian(checker));

    int i = 1;
    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            float chooseMat = RandomFloat();
            Vec4 center(a + .9f * RandomFloat(), .2f, b + .9f * RandomFloat());
            if ((center - Vec4(4, .2f, 0)).Length() > .9f)
            {
                if (chooseMat < .8f)
                {
                    list[i++] = new MovingSphere(
                        center, center + Vec4(0, .5f*RandomFloat(), 0),
                        .0f, 1.f,
                        .2f,
                        new MLambertian(new ConstantTexture(Vec4(RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat()))
                        )
                    );
                }
                else if (chooseMat < .95f)
                {
                    list[i++] = new Sphere(center, .2f, new MMetal(new ConstantTexture(Vec4(.5f * (1 + RandomFloat()), .5f * (1 + RandomFloat()), .5f * (1 + RandomFloat()))), .5f * RandomFloat()));
                }
                else
                {
                    list[i++] = new Sphere(center, .2f, new MDielectric(1.5f));
                }
            }
        }
    }

    list[i++] = new Sphere(Vec4(0, 1, 0), 1.f, new MDielectric(1.5f));
    list[i++] = new Sphere(Vec4(-4, 1, 0), 1.f, new MLambertian(new ConstantTexture(Vec4(0.4f, 0.2f, 0.1f))));
    list[i++] = new Sphere(Vec4(4, 1, 0), 1.f, new MMetal(new ConstantTexture(Vec4(0.7f, 0.6f, 0.5f)), 0.f));

    // Generate BVH tree
    BVHNode* bvhHead = new BVHNode(list, i, time0, time1);

    return WorldScene::Create(cam, bvhHead);
}

// ----------------------------------------------------------------------------------------------------------------------------

static WorldScene* sampleSceneCornellBox(bool smoke)
{
    IHitable** list = new IHitable*[8];
    int i = 0;

    IHitable** lsList = new IHitable*[2];
    int numLs = 0;

    Material* red   = new MLambertian(new ConstantTexture(Vec4(.65f, .05f, .05f)));
    Material* white = new MLambertian(new ConstantTexture(Vec4(.73f, .73f, .73f)));
    Material* green = new MLambertian(new ConstantTexture(Vec4(.12f, .45f, .15f)));
    Material* light = new MDiffuseLight(new ConstantTexture(Vec4(50, 50, 50)));
    Material* glass = new MDielectric(1.5f);

    list[i++] = new FlipNormals(new XYZRect(XYZRect::YZ, 0, 555, 0, 555, 555, green));
    list[i++] = new XYZRect(XYZRect::YZ, 0, 555, 0, 555, 0, red);

    IHitable* lightShape = new XYZRect(XYZRect::XZ, 200, 350, 200, 350, 555, light, true);
    list[i++] = new FlipNormals(lightShape);
    lsList[numLs++] = lightShape;

    // Take out the ceiling, this blocks the lighting samples in the realtime raytracer
    // TODO: fix this
    //list[i++] = new FlipNormals(new XYZRect(XYZRect::XZ, 0, 555, 0, 555, 555, white));

    list[i++] = new XYZRect(XYZRect::XZ, 0, 555, 0, 555, 0, white);
    list[i++] = new FlipNormals(new XYZRect(XYZRect::XY, 0, 555, 0, 555, 555, white));

    IHitable* glassSphere = new Sphere(Vec4(190, 90, 190), 90, glass, true);
    list[i++] = glassSphere;
    lsList[numLs++] = glassSphere;

    IHitable* box2 = new HitableTranslate(
        new HitableRotateY(
            new HitableBox(Vec4(0, 0, 0), Vec4(165, 330, 165), white),
            15),
        Vec4(265, 0, 295));

    SampleScene sceneType;
    if (smoke)
    {
        sceneType = SceneCornellSmoke;
        list[i++] = new ConstantMedium(box2, 0.01f, new ConstantTexture(Vec4(0.f, 0.f, 0.f)));
    }
    else
    {
        sceneType = SceneCornell;
        list[i++] = box2;
    }

    return WorldScene::Create(getCameraForSample(sceneType), list, i, lsList, numLs);
}

// ----------------------------------------------------------------------------------------------------------------------------

static WorldScene* sampleSceneMesh()
{
    const int numBoxes = 5;
    int total = 0;

    const Vec4 colorSapphire    = Vec4(0.06f, 0.3f, 0.7f);
    const Vec4 colorAlmostWhite = Vec4(0.9f, 0.9f, 0.9f);
    const Vec4 colorGreenish    = Vec4(0.2f, 0.9f, 0.4f);
    const Vec4 colorYellow      = Vec4(1.0f, 1.0f, 0.0f);
    const Vec4 colorPurple      = Vec4(0.621f, 0.351f, 0.988f);

    IHitable** list     = new IHitable*[30];
    Material*  ground   = new MLambertian(new ConstantTexture(colorPurple));

    IHitable** lsList = new IHitable*[2];
    int numLs = 0;

    // Create random hitable boxes, in BVH tree
    list[total++] = new HitableBox(Vec4(-2000, -100, -2000), Vec4(2000, 100, 2000), ground);

    // Create light
    {
        Material *lightMat   = new MDiffuseLight(new ConstantTexture(Vec4(30, 30, 30)));
        IHitable *lightShape = new XYZRect(XYZRect::XZ, -200, 200, -200, 200, 1000, lightMat, true);
        list[total++]        = new FlipNormals(lightShape);;
        lsList[numLs++]      = lightShape;
    }

    // Meshes
    {

        IHitable *r8 =
            new HitableTranslate(
                new HitableRotateY(
                    TriMesh::CreateFromOBJFile(GetAbsolutePath(RUNTIMEDATA_DIR "/r8.obj").c_str(), 25.f, false), 20.f),
                Vec4(220, 105, 145)
            );
        list[total++] = r8;

#if 1
        IHitable *totoro =
            new HitableTranslate(
                new HitableRotateY(
                    TriMesh::CreateFromOBJFile(GetAbsolutePath(RUNTIMEDATA_DIR "/totoro.obj").c_str(), 10.f, false, new MMetal(new ConstantTexture(colorSapphire), 0.5f)), 180.f),
                Vec4(-60, 105, 145)
            );
        list[total++] = totoro;

        IHitable *luigi =
            new HitableTranslate(
                new HitableRotateY(
                    TriMesh::CreateFromOBJFile(GetAbsolutePath(RUNTIMEDATA_DIR "/luigi.obj").c_str(), 2.f), 180.f),
                Vec4(-320, 105, -100)
            );
        list[total++] = luigi;
#endif
    }

    // Dielectric and metal spheres
    {
        IHitable* newDielectricSphere = new Sphere(Vec4(359, 300, -300), 150, new MDielectric(1.5f));
        list[total++] = newDielectricSphere;
        lsList[numLs++] = newDielectricSphere;
    }

    // Volumes
    {
        list[total++] = new Sphere(Vec4(500, 250, 100), 125, new MLambertian(new ConstantTexture(colorYellow)));

        //IHitable *boundary = new Sphere(Vec4(500, 250, 100), 125, new MDielectric(1.5f));
        //list[total++] = boundary;
        //list[total++] = new ConstantMedium(boundary, 0.2f, new ConstantTexture(colorYellow));

        //boundary = new Sphere(Vec4(0, 0, 0), 5000, new MDielectric(1.5f));
        //list[total++] = new ConstantMedium(boundary, 0.0001f, new ConstantTexture(Vec4(1.0f, 1.0f, 1.0f)));
    }

    return WorldScene::Create(getCameraForSample(SceneMesh), list, total, lsList, numLs);
}

// ----------------------------------------------------------------------------------------------------------------------------

static WorldScene* sampleSceneFinal()
{
    const int numBoxes = 20;
    int total = 0;

    IHitable** list = new IHitable*[30];
    IHitable** boxlist = new IHitable*[10000];
    IHitable** boxlist2 = new IHitable*[10000];
    Material*  white = new MLambertian(new ConstantTexture(Vec4(0.73f, 0.73f, 0.73f)));
    Material*  ground = new MLambertian(new ConstantTexture(Vec4(0.48f, 0.83f, 0.53f)));
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

                boxlist[b++] = new HitableBox(Vec4(x0, y0, z0), Vec4(x1, y1, z1), ground);
            }
        }
        list[total++] = new BVHNode(boxlist, b, 0, 1);
    }

    // Create light
    {
        Material* lightMaterial = new MDiffuseLight(new ConstantTexture(Vec4(7, 7, 7)));
        IHitable* lightShape = new XYZRect(XYZRect::XZ, 123, 423, 147, 412, 554, lightMaterial, true);
        list[total++] = new FlipNormals(lightShape);
        lsList[numLs++] = lightShape;
    }

    // Moving sphere
    {
        Vec4 center(400, 400, 200);
        list[total++] = new MovingSphere(center, center + Vec4(30, 0, 0), 0, 1, 50, new MLambertian(new ConstantTexture(Vec4(0.7f, 0.3f, 0.1f))));
    }

    // Dielectric and metal spheres
    {
        IHitable* newDielectricSphere = new Sphere(Vec4(260, 150, 45), 50, new MDielectric(1.5f));
        list[total++]   = newDielectricSphere;
        lsList[numLs++] = newDielectricSphere;

        list[total++] = new Sphere(Vec4(0, 150, 145), 50, new MMetal(new ConstantTexture(Vec4(0.8f, 0.8f, 0.9f)), 10.0f));
    }

    // Volumes
    {
        IHitable *boundary = new Sphere(Vec4(360, 150, 145), 70, new MDielectric(1.5f));
        list[total++] = boundary;
        list[total++] = new ConstantMedium(boundary, 0.2f, new ConstantTexture(Vec4(0.2f, 0.4f, 0.9f)));

        boundary = new Sphere(Vec4(0, 0, 0), 5000, new MDielectric(1.5f));
        list[total++] = new ConstantMedium(boundary, 0.0001f, new ConstantTexture(Vec4(1.0f, 1.0f, 1.0f)));
    }

    // Image texture sphere
    {
        Material *emat = new MLambertian(new ImageTexture(GetAbsolutePath(RUNTIMEDATA_DIR "/guitar.jpg").c_str()));
        list[total++] = new Sphere(Vec4(400, 200, 400), 100, emat);
    }

    // Perlin noise sphere
    {
        BaseTexture *perTex = new NoiseTexture(0.1f);
        list[total++] = new Sphere(Vec4(220, 280, 300), 80, new MLambertian(perTex));
    }

    // Translated, rotated spheres in BVH tree
    {
        int ns = 1000;
        for (int j = 0; j < ns; j++)
        {
            boxlist2[j] = new Sphere(Vec4(165 * RandomFloat(), 165 * RandomFloat(), 165 * RandomFloat()), 10, white);
        }
        list[total++] = new HitableTranslate(new HitableRotateY(new BVHNode(boxlist2, ns, 0.0, 1.0), 15), Vec4(-100, 270, 395));
    }

    return WorldScene::Create(getCameraForSample(SceneFinal), list, total, lsList, numLs);
}

// ----------------------------------------------------------------------------------------------------------------------------

WorldScene* GetSampleScene(SampleScene sceneType)
{
    WorldScene* ret = nullptr;
    switch (sceneType)
    {
        case SceneRandom:
        {
            ret = sampleSceneRandom();
        }
        break;

        case SceneCornell:
        case SceneCornellSmoke:
        {
            ret = sampleSceneCornellBox(sceneType == SceneCornellSmoke);
        }
        break;

        case SceneMesh:
        {
            ret = sampleSceneMesh();
        }
        break;

        case SceneFinal:
        {
            ret = sampleSceneFinal();
        }
        break;

        default:
        break;
    }

    return ret;
}
