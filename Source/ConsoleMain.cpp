#include "Core/Camera.h"
#include "Core/Raytracer.h"
#include "SampleScenes.h"

// ----------------------------------------------------------------------------------------------------------------------------

struct SceneConfig
{
    SampleScene   SceneType;
    const char*   OutputName;
    bool          Enabled;
};

// ----------------------------------------------------------------------------------------------------------------------------

static int    sOutputWidth      = 512;
static int    sOutputHeight     = 512;
static float  sAspect           = float(sOutputWidth) / float(sOutputHeight);
static int    sNumSamplesPerRay = 500;
static int    sMaxScatterDepth  = 50;
static int    sNumThreads       = 4;

static SceneConfig sSceneConfigs[] =
{
    { SceneRandom,       "random",   true },
    { SceneCornell,      "cornell1", true },
    { SceneCornellSmoke, "cornell2", true },
    { SceneMesh,         "mesh",     true },
    { SceneFinal,        "final",    true },
};

static const int sNumSceneConfigs = sizeof(sSceneConfigs) / sizeof(SceneConfig);

// ----------------------------------------------------------------------------------------------------------------------------

static void raytraceAndPrintProgress(Raytracer& tracer, WorldScene* scene)
{
    // Start the trace
    printf("\nRendering frame...\n");
    tracer.BeginRaytrace(scene);

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
        else if (strstr(argv[i], "noscene") != nullptr && (i + 1) < argc)
        {
            const int sceneNum = atoi(argv[++i]);
            if (sceneNum < MaxScene)
            {
                sSceneConfigs[sceneNum].Enabled = false;
            }
        }
    }

    if (argc <= 1)
    {
        printf("Commandline usage:\n\twidth [num]  height [num]  samples [num]  depth [num]  threads [num]  noscene [sceneNum]\n");
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

    for (int i = 0; i < sNumSceneConfigs; i++)
    {
        if (sSceneConfigs[i].Enabled)
        {
            WorldScene* worldScene = GetSampleScene(sSceneConfigs[i].SceneType);
            worldScene->GetCamera().SetFocusDistanceToLookAt();
            worldScene->GetCamera().SetAspect(float(sOutputWidth) / float(sOutputHeight));
            raytraceAndPrintProgress(tracer, worldScene);
            WriteImageAndLog(&tracer, sSceneConfigs[i].OutputName);
        }
    }

    // Done
    return 0;
}
