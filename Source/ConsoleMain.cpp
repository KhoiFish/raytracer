#include <iostream>
#include <fstream>
#include <cfloat>

#include "Core/Vec3.h"
#include "Core/Camera.h"
#include "Core/Raytracer.h"
#include "Core/ImageIO.h"
#include "SampleScenes.h"

// ----------------------------------------------------------------------------------------------------------------------------

#define  OUTPUT_IMAGE_DIR  "OutputImages/"

static int    sOutputWidth      = 512;
static int    sOutputHeight     = 512;
static float  sAspect           = float(sOutputWidth) / float(sOutputHeight);
static int    sNumSamplesPerRay = 100;
static int    sMaxScatterDepth  = 50;
static int    sNumThreads       = 8;

static bool sSceneEnabled[MaxScene] =
{
    true,  // Random
    true,  // Cornell
    true,  // Cornell smoke
    true,  // Final
};

// ----------------------------------------------------------------------------------------------------------------------------

static void RaytraceAndPrintProgress(Raytracer& tracer, Camera& cam, IHitable* world)
{
    // Start the trace
    printf("\nRendering frame...\n");
    tracer.BeginRaytrace(cam, world);

    // Wait for trace to finish
    char buf[256];
    while (!tracer.WaitForTraceToFinish(1000 * 500))
    {
        // Get stats
        Raytracer::Stats stats = tracer.GetStats();
        float percentage = float(stats.NumPixelsTraced) / float(stats.TotalNumPixels);

        snprintf(buf, 256, "#rays:%lld #pixels:%d", stats.TotalRaysFired, stats.NumPixelsTraced);
        PrintProgress(buf, percentage);
    }

    printf("\nRendering done!");
}

// ----------------------------------------------------------------------------------------------------------------------------

int main()
{
    // Create ray tracer
    Raytracer tracer(sOutputWidth, sOutputHeight, sNumSamplesPerRay, sMaxScatterDepth, sNumThreads);

    // Random scene
    if (sSceneEnabled[SceneRandom])
    {
        Camera cam = GetCameraForSample(SceneRandom, sAspect);
        tracer.SetDefaultAmbient(Vec3(.7f, .7f, .7f));
        RaytraceAndPrintProgress(tracer, cam, SampleSceneRandom(cam));
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(), 
            OUTPUT_IMAGE_DIR "randomworld.ppm");
    }

    // Cornell box
    if (sSceneEnabled[SceneCornell] || sSceneEnabled[SceneCornellSmoke])
    {
        if (sSceneEnabled[SceneCornell])
        {
            Camera cam = GetCameraForSample(SceneCornell, sAspect);
            tracer.SetDefaultAmbient(Vec3(0, 0, 0));
            RaytraceAndPrintProgress(tracer, cam, SampleSceneCornellBox(false));
            ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(),
                OUTPUT_IMAGE_DIR "cornell.ppm");
        }

        if (sSceneEnabled[SceneCornellSmoke])
        {
            Camera cam = GetCameraForSample(SceneCornellSmoke, sAspect);
            tracer.SetDefaultAmbient(Vec3(0, 0, 0));
            RaytraceAndPrintProgress(tracer, cam, SampleSceneCornellBox(true));
            ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(),
                OUTPUT_IMAGE_DIR "cornell_smoke.ppm");
        }
    }

    // Final
    if (sSceneEnabled[SceneFinal])
    {
        Camera cam = GetCameraForSample(SceneFinal, sAspect);
        tracer.SetDefaultAmbient(Vec3(0, 0, 0));
        RaytraceAndPrintProgress(tracer, cam, SampleSceneFinal());
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(),
            OUTPUT_IMAGE_DIR "final.ppm");
    }

    // Done
    return 0;
}