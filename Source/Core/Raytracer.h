#pragma once

#include <thread>
#include <atomic>
#include <cstdint>

#include "Ray.h"
#include "Vec3.h"
#include "IHitable.h"
#include "HitableList.h"
#include "Material.h"
#include "Camera.h"
#include "Util.h"
#include "ThreadEvent.h"
#include "Pdf.h"
#include "WorldScene.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Raytracer
{
public:

    typedef std::chrono::time_point<std::chrono::system_clock> StdTime;
    typedef void(*OnTraceComplete)(Raytracer* tracer, bool actuallyFinished);

    struct Stats
    {
        int64_t     TotalRaysFired;
        int         NumPixelSamples;
        int         TotalNumPixelSamples;
        int         NumPdfQueryRetries;
        int         TotalTimeInSeconds;
    };

public:

    Raytracer(int width, int height, int numSamples, int maxDepth, int numThreads, bool pdfEnabled);
    ~Raytracer();

    void             BeginRaytrace(const Camera& cam, WorldScene* scene, OnTraceComplete onComplete = nullptr);
    bool             WaitForTraceToFinish(int timeoutMicroSeconds);
    Stats            GetStats() const;

    inline Vec3*     GetOutputBuffer() const { return OutputBuffer; }
    inline uint8_t*  GetOutputBufferRGBA() const { return OutputBufferRGBA; }
    inline int       GetOutputWidth() const { return OutputWidth; }
    inline int       GetOutputHeight() const { return OutputHeight; }

private:

    static void      threadTraceNextPixel(int id, Raytracer* tracer, const Camera& cam, WorldScene* scene);
    Vec3             trace(const Ray& r, WorldScene* scene, int depth, const Vec3& clearColor);
    void             cleanupRaytrace();

private:

    // Output options
    int                   OutputWidth;
    int                   OutputHeight;
    Vec3*                 OutputBuffer;
    uint8_t*              OutputBufferRGBA;

    // Tracing options
    int                   NumRaySamples;
    int                   MaxDepth;
    int                   NumThreads;
    bool                  PdfEnabled;

    // Thread tracking
    std::atomic<int>      CurrentPixelSampleOffset;
    std::atomic<int64_t>  TotalRaysFired;
    std::atomic<int>      NumThreadsDone;
    std::atomic<int>      NumPdfQueryRetries;
    std::atomic<bool>     ThreadExitRequested;
    std::atomic<StdTime>  StartTime;
    std::atomic<StdTime>  EndTime;
    std::thread**         ThreadPtrs;
    ThreadEvent           RaytraceEvent;
    bool                  IsRaytracing;
    OnTraceComplete       OnComplete;
};
