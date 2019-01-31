#include "Core/Camera.h"
#include "Core/Raytracer.h"
#include "SampleScenes.h"

// ----------------------------------------------------------------------------------------------------------------------------

static int    sOutputWidth      = 1024;
static int    sOutputHeight     = 1024;
static float  sAspect           = float(sOutputWidth) / float(sOutputHeight);
static int    sNumSamplesPerRay = 5000;
static int    sMaxScatterDepth  = 50;
static int    sNumThreads       = 16;

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

static void parseCommandline(int argc, const char* argv[])
{
    for (int i = 0; i < argc; i++)
    {
        if (strstr(argv[i], "width") != nullptr && (i + 1) < argc)
        {
            sOutputWidth = atoi(argv[++i]);
        }
        else if (strstr(argv[i], "height") != nullptr && (i + 1) < argc)
        {
            sOutputHeight = atoi(argv[++i]);
        }
        else if (strstr(argv[i], "samples") != nullptr && (i + 1) < argc)
        {
            sNumSamplesPerRay = atoi(argv[++i]);
        }
        else if (strstr(argv[i], "depth") != nullptr && (i + 1) < argc)
        {
            sMaxScatterDepth = atoi(argv[++i]);
        }
        else if (strstr(argv[i], "threads") != nullptr && (i + 1) < argc)
        {
            sNumThreads = atoi(argv[++i]);
        }
        else if (strstr(argv[i], "disableScene") != nullptr && (i + 1) < argc)
        {
            const int sceneNum = atoi(argv[++i]);
            if (sceneNum < MaxScene)
            {
                sSceneEnabled[sceneNum] = false;
            }
        }
    }

    if (argc <= 1)
    {
        printf("Commandline usage:\n\twidth [num]  height [num]  samples [num]  depth [num]  threads [num]  disableScene [sceneNum]\n");
    }

    printf("Current tracing parameters:\n\tresolution:%dx%d numSamples:%d scatterDepth:%d numThreads:%d\n",
        sOutputWidth, sOutputHeight, sNumSamplesPerRay, sMaxScatterDepth, sNumThreads);
}

// ----------------------------------------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    // Create ray tracer
    parseCommandline(argc, argv);
    Raytracer tracer(sOutputWidth, sOutputHeight, sNumSamplesPerRay, sMaxScatterDepth, sNumThreads, true);

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