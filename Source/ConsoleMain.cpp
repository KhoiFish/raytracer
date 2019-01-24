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

static int    sOutputWidth      = 256;
static int    sOutputHeight     = 256;
static float  sAspect           = float(sOutputWidth) / float(sOutputHeight);
static int    sNumSamplesPerRay = 500;
static int    sMaxScatterDepth  = 50;
static int    sNumThreads       = 7;

static bool   sSceneEnabled[MaxScene] =
{
    false,  // Random
    false,  // Cornell
    false,  // Cornell smoke
    true,   // Final
};

// ----------------------------------------------------------------------------------------------------------------------------

static void RaytraceAndPrintProgress(Raytracer& tracer, Camera& cam, WorldScene* scene)
{
    // Start the trace
    printf("\nRendering frame...\n");
    tracer.BeginRaytrace(cam, scene);

    // Wait for trace to finish
    char buf[256];
    while (!tracer.WaitForTraceToFinish(1000 * 500))
    {
        // Get stats
        Raytracer::Stats stats = tracer.GetStats();
        float percentage = float(stats.NumPixelsTraced) / float(stats.TotalNumPixels);

        snprintf(buf, 256, "#rays:%lld #pixels:%d #pdfQueryRetries:%d", stats.TotalRaysFired, stats.NumPixelsTraced, stats.NumPdfQueryRetries);
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
        RaytraceAndPrintProgress(tracer, cam, SampleSceneRandom(cam));
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(), 
            OUTPUT_IMAGE_DIR "randomworld.ppm");
    }

    // Cornell box
    if (sSceneEnabled[SceneCornell])
    {
        Camera cam = GetCameraForSample(SceneCornell, sAspect);
        RaytraceAndPrintProgress(tracer, cam, SampleSceneCornellBox(false));
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(),
            OUTPUT_IMAGE_DIR "cornell.ppm");
    }

    // Cornell smoke
    if (sSceneEnabled[SceneCornellSmoke])
    {
        Camera cam = GetCameraForSample(SceneCornellSmoke, sAspect);
        RaytraceAndPrintProgress(tracer, cam, SampleSceneCornellBox(true));
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(),
            OUTPUT_IMAGE_DIR "cornell_smoke.ppm");
    }

    // Final
    if (sSceneEnabled[SceneFinal])
    {
        Camera cam = GetCameraForSample(SceneFinal, sAspect);
        RaytraceAndPrintProgress(tracer, cam, SampleSceneFinal());
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(),
            OUTPUT_IMAGE_DIR "final.ppm");
    }

    // Done
    return 0;
}