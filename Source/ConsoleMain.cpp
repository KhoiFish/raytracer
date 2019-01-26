#include "Core/Camera.h"
#include "Core/Raytracer.h"
#include "SampleScenes.h"

// ----------------------------------------------------------------------------------------------------------------------------

static int    sOutputWidth      = 256;
static int    sOutputHeight     = 256;
static float  sAspect           = float(sOutputWidth) / float(sOutputHeight);
static int    sNumSamplesPerRay = 1;
static int    sMaxScatterDepth  = 50;
static int    sNumThreads       = 6;

static bool   sSceneEnabled[MaxScene] =
{
    true,  // Random
    true,  // Cornell
    true,  // Cornell smoke
    true,  // SceneMesh
    true,  // Final
};

// ----------------------------------------------------------------------------------------------------------------------------

static void raytraceAndPrintProgress(Raytracer& tracer, Camera& cam, WorldScene* scene)
{
    // Start the trace
    printf("\nRendering frame...\n");
    tracer.BeginRaytrace(cam, scene);

    // Wait for trace to finish    
    while (!tracer.WaitForTraceToFinish(1000 * 500))
    {
        ProgressPrint(&tracer);
    }
    ProgressPrint(&tracer);

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
        raytraceAndPrintProgress(tracer, cam, SampleSceneRandom(cam));
        WriteImageAndLog(&tracer, "random");
    }

    // Cornell box
    if (sSceneEnabled[SceneCornell])
    {
        Camera cam = GetCameraForSample(SceneCornell, sAspect);
        raytraceAndPrintProgress(tracer, cam, SampleSceneCornellBox(false));
        WriteImageAndLog(&tracer, "cornell");
    }

    // Cornell smoke
    if (sSceneEnabled[SceneCornellSmoke])
    {
        Camera cam = GetCameraForSample(SceneCornellSmoke, sAspect);
        raytraceAndPrintProgress(tracer, cam, SampleSceneCornellBox(true));
        WriteImageAndLog(&tracer, "cornell_smoke");
    }

    // Mesh
    if (sSceneEnabled[SceneMesh])
    {
        Camera cam = GetCameraForSample(SceneMesh, sAspect);
        raytraceAndPrintProgress(tracer, cam, SampleSceneMesh());   
        WriteImageAndLog(&tracer, "mesh");
    }

    // Final
    if (sSceneEnabled[SceneFinal])
    {
        Camera cam = GetCameraForSample(SceneFinal, sAspect);
        raytraceAndPrintProgress(tracer, cam, SampleSceneFinal());
        WriteImageAndLog(&tracer, "final");
    }

    // Done
    return 0;
}