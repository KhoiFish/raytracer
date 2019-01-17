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

// ----------------------------------------------------------------------------------------------------------------------------

class Raytracer
{
public:

    struct Stats
    {
        int64_t  TotalRaysFired;
        int      NumPixelsTraced;
        int      TotalNumPixels;
    };

public:

    Raytracer(int width, int height, int numSamples, int maxDepth, int numThreads);
    ~Raytracer();

    void          BeginRaytrace(const Camera& cam, IHitable* world);
    bool          WaitForTraceToFinish(int timeoutMicroSeconds);
    Stats         GetStats() const;
    void          SetDefaultAmbient(const Vec3& ambient);

    inline Vec3*  GetOutputBuffer() const { return OutputBuffer; }
    inline int    GetOutputWidth() const  { return OutputWidth; }
    inline int    GetOutputHeight() const { return OutputHeight; }

private:

    static void   threadTraceNextPixel(int id, Raytracer* tracer, const Camera& cam, IHitable* world);
    Vec3          trace(const Ray& r, IHitable *world, int depth);
    void          cleanupRaytrace();

private:

    // Output options
    int                   OutputWidth;
    int                   OutputHeight;
    Vec3*                 OutputBuffer;

    // Tracing options
    int                   NumRaySamples;
    int                   MaxDepth;
    int                   NumThreads;
    Vec3                  DefaultAmbient;

    // Thread tracking
    std::atomic<int>      CurrentOutputOffset;
    std::atomic<int64_t>  TotalRaysFired;
    std::atomic<int>      NumThreadsDone;
    std::thread**         ThreadPtrs;
    ThreadEvent           RaytraceEvent;
    bool                  IsRaytracing;
};
