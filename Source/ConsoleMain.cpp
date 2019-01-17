#include <iostream>
#include <fstream>
#include <cfloat>

#include "Core/Vec3.h"
#include "Core/Camera.h"
#include "Core/Raytracer.h"
#include "SampleScenes.h"

// ----------------------------------------------------------------------------------------------------------------------------

#define  OUTPUT_IMAGE_DIR  "OutputImages/"

static int  sOutputWidth      = 512;
static int  sOutputHeight     = 512;
static int  sNumSamplesPerRay = 50;
static int  sMaxScatterDepth  = 50;
static int  sNumThreads       = 8;
static bool sSceneEnabled[MaxScene] =
{
    true,  // Random
    true,  // Cornell
    true,  // Cornell smoke
    true,  // Final
};

// ----------------------------------------------------------------------------------------------------------------------------

int main()
{
    // Create ray tracer
    Raytracer tracer(sOutputWidth, sOutputHeight, sNumSamplesPerRay, sMaxScatterDepth, sNumThreads);

    // Random scene
    if (sSceneEnabled[Random])
    {
        // Camera options
        const Vec3   lookFrom     = Vec3(13, 2, 3);
        const Vec3   lookAt       = Vec3(0, 0, 0);
        const Vec3   upVec        = Vec3(0, 1, 0);
        const float  vertFov      = 20.f;
        const float  aspect       = float(sOutputWidth) / float(sOutputHeight);
        const float  aperture     = 0.0f;
        const float  distToFocus  = 10.f;
        const float  shutterTime0 = 0.f;
        const float  shutterTime1 = 1.f;
        Camera cam(
            lookFrom, lookAt, upVec,
            vertFov, aspect, aperture, distToFocus,
            shutterTime0, shutterTime1);

        // Render and write out image
        tracer.SetDefaultAmbient(Vec3(.7f, .7f, .7f));
        tracer.Render(cam, SampleSceneRandom(shutterTime0, shutterTime1));
        tracer.WriteOutputToPPMFile(std::ofstream(OUTPUT_IMAGE_DIR "randomworld.ppm"));
    }

    // Cornell box
    if (sSceneEnabled[Cornell] || sSceneEnabled[CornellSmoke])
    {
        // Camera options
        const Vec3   lookFrom     = Vec3(278, 278, -800);
        const Vec3   lookAt       = Vec3(278, 278, 0);
        const Vec3   upVec        = Vec3(0, 1, 0);
        const float  vertFov      = 40.f;
        const float  aspect       = float(sOutputWidth) / float(sOutputHeight);
        const float  aperture     = 0.0f;
        const float  distToFocus  = 10.f;
        const float  shutterTime0 = 0.f;
        const float  shutterTime1 = 1.f;
        Camera cam(
            lookFrom, lookAt, upVec,
            vertFov, aspect, aperture, distToFocus,
            shutterTime0, shutterTime1);

        // Render and write out image
        if (sSceneEnabled[Cornell])
        {
            tracer.SetDefaultAmbient(Vec3(0, 0, 0));
            tracer.Render(cam, SampleSceneCornellBox(false));
            tracer.WriteOutputToPPMFile(std::ofstream(OUTPUT_IMAGE_DIR "cornell.ppm"));
        }

        if (sSceneEnabled[CornellSmoke])
        {
            tracer.SetDefaultAmbient(Vec3(0, 0, 0));
            tracer.Render(cam, SampleSceneCornellBox(true));
            tracer.WriteOutputToPPMFile(std::ofstream(OUTPUT_IMAGE_DIR "cornell_smoke.ppm"));
        }
    }

    // Final
    if (sSceneEnabled[Final])
    {
        // Camera options
        const Vec3   lookFrom     = Vec3(478, 278, -600);
        const Vec3   lookAt       = Vec3(278, 278, 0);
        const Vec3   upVec        = Vec3(0, 1, 0);
        const float  vertFov      = 40.f;
        const float  aspect       = float(sOutputWidth) / float(sOutputHeight);
        const float  aperture     = 0.0f;
        const float  distToFocus  = 10.f;
        const float  shutterTime0 = 0.f;
        const float  shutterTime1 = 1.f;
        Camera cam(
            lookFrom, lookAt, upVec,
            vertFov, aspect, aperture, distToFocus,
            shutterTime0, shutterTime1);

        // Render and write out image
        tracer.SetDefaultAmbient(Vec3(0, 0, 0));
        tracer.Render(cam, SampleSceneFinal());
        tracer.WriteOutputToPPMFile(std::ofstream(OUTPUT_IMAGE_DIR "final.ppm"));
    }

    // Done
    return 0;
}