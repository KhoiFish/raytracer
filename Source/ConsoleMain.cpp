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

static inline const char* progressPrint(Raytracer& tracer)
{
    static char buf[256];

    Raytracer::Stats stats = tracer.GetStats();
    float percentage = float(stats.NumPixelsTraced) / float(stats.TotalNumPixels);
    int   numMinutes = stats.TotalTimeInSeconds / 60;
    int   numSeconds = stats.TotalTimeInSeconds % 60;

    snprintf(buf, 256, "#time:%dm:%2ds  #rays:%lld  #pixels:%d  #pdfQueryRetries:%d ", 
        numMinutes, numSeconds, stats.TotalRaysFired, stats.NumPixelsTraced, stats.NumPdfQueryRetries);

    PrintProgress(buf, percentage);

    return buf;
}

// ----------------------------------------------------------------------------------------------------------------------------

static void raytraceAndPrintProgress(Raytracer& tracer, Camera& cam, WorldScene* scene)
{
    // Start the trace
    printf("\nRendering frame...\n");
    tracer.BeginRaytrace(cam, scene);

    // Wait for trace to finish    
    while (!tracer.WaitForTraceToFinish(1000 * 500))
    {
        progressPrint(tracer);
    }
    progressPrint(tracer);

    printf("\nRendering done!");
}

// ----------------------------------------------------------------------------------------------------------------------------

static void writeLog(Raytracer& tracer, const std::string& logFilename)
{
    std::ofstream out(logFilename.c_str());
    if (out.is_open())
    {
        out << progressPrint(tracer);
    }
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

        std::string baseName = std::string(OUTPUT_IMAGE_DIR "random.") + std::string(GetTimeAndDateString());
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(), (baseName + std::string(".ppm")).c_str());
        writeLog(tracer, baseName + std::string(".log"));
    }

    // Cornell box
    if (sSceneEnabled[SceneCornell])
    {
        Camera cam = GetCameraForSample(SceneCornell, sAspect);
        raytraceAndPrintProgress(tracer, cam, SampleSceneCornellBox(false));

        std::string baseName = std::string(OUTPUT_IMAGE_DIR "cornell.") + std::string(GetTimeAndDateString());
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(), (baseName + std::string(".ppm")).c_str());
        writeLog(tracer, baseName + std::string(".log"));
    }

    // Cornell smoke
    if (sSceneEnabled[SceneCornellSmoke])
    {
        Camera cam = GetCameraForSample(SceneCornellSmoke, sAspect);
        raytraceAndPrintProgress(tracer, cam, SampleSceneCornellBox(true));

        std::string baseName = std::string(OUTPUT_IMAGE_DIR "cornell_smoke.") + std::string(GetTimeAndDateString());
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(), (baseName + std::string(".ppm")).c_str());
        writeLog(tracer, baseName + std::string(".log"));
    }

    // Mesh
    if (sSceneEnabled[SceneMesh])
    {
        Camera cam = GetCameraForSample(SceneMesh, sAspect);
        raytraceAndPrintProgress(tracer, cam, SampleSceneMesh());   

        std::string baseName = std::string(OUTPUT_IMAGE_DIR "mesh.") + std::string(GetTimeAndDateString());
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(), (baseName + std::string(".ppm")).c_str());
        writeLog(tracer, baseName + std::string(".log"));
    }

    // Final
    if (sSceneEnabled[SceneFinal])
    {
        Camera cam = GetCameraForSample(SceneFinal, sAspect);
        raytraceAndPrintProgress(tracer, cam, SampleSceneFinal());

        std::string baseName = std::string(OUTPUT_IMAGE_DIR "final.") + std::string(GetTimeAndDateString());
        ImageIO::WriteToPPMFile(tracer.GetOutputBuffer(), tracer.GetOutputWidth(), tracer.GetOutputHeight(), (baseName + std::string(".ppm")).c_str());
        writeLog(tracer, baseName + std::string(".log"));
    }

    // Done
    return 0;
}