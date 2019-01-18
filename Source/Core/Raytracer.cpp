#include "Raytracer.h"
#include <cfloat>

// ----------------------------------------------------------------------------------------------------------------------------

Raytracer::Raytracer(int width, int height, int numSamples, int maxDepth, int numThreads)
    : OutputWidth(width)
    , OutputHeight(height)
    , NumRaySamples(numSamples)
    , MaxDepth(maxDepth)
    , NumThreads(numThreads)
    , CurrentOutputOffset(0)
    , TotalRaysFired(0)
    , NumThreadsDone(0)
    , IsRaytracing(false)
{
    OutputBuffer = new Vec3[OutputWidth * OutputHeight];
    OutputBufferRGBA = new uint8_t[OutputWidth * OutputHeight * 4];
    ThreadPtrs = new std::thread*[NumThreads];
}

// ----------------------------------------------------------------------------------------------------------------------------

Raytracer::~Raytracer()
{
    cleanupRaytrace();

    delete[] OutputBuffer;
    OutputBuffer = nullptr;

    delete[] OutputBufferRGBA;
    OutputBufferRGBA = nullptr;

    delete[] ThreadPtrs;
    ThreadPtrs = nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Raytracer::BeginRaytrace(const Camera& cam, IHitable* world)
{
    // Clean up last trace, if any
    cleanupRaytrace();

    IsRaytracing = true;
    TotalRaysFired = 0;
    CurrentOutputOffset = 0;
    NumThreadsDone = 0;

    // Create the threads and run them
    for (int i = 0; i < NumThreads; i++)
    {
        ThreadPtrs[i] = new std::thread(threadTraceNextPixel, i, this, cam, world);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool Raytracer::WaitForTraceToFinish(int timeoutMicroSeconds)
{
    if (IsRaytracing)
    {
        return RaytraceEvent.WaitOne(timeoutMicroSeconds);
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

Raytracer::Stats Raytracer::GetStats() const
{
    Stats stats;
    stats.TotalRaysFired  = TotalRaysFired.load();
    stats.NumPixelsTraced = CurrentOutputOffset.load();
    stats.TotalNumPixels  = (OutputWidth * OutputHeight);

    return stats;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Raytracer::cleanupRaytrace()
{
    if (IsRaytracing)
    {
        // Wait for last thread to rendering
        RaytraceEvent.WaitOne(-1);

        // Join all the threads
        for (int i = 0; i < NumThreads; i++)
        {
            ThreadPtrs[i]->join();
            delete ThreadPtrs[i];
            ThreadPtrs[i] = nullptr;
        }

        // Reset event
        IsRaytracing = false;
        RaytraceEvent.Reset();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Raytracer::threadTraceNextPixel(int id, Raytracer* tracer, const Camera& cam, IHitable* world)
{
    const int numPixels = (tracer->OutputWidth * tracer->OutputHeight);

    int offset = tracer->CurrentOutputOffset.load();
    while (offset < numPixels)
    {
        // Find the next offset to the pixel to trace
        while (offset < numPixels)
        {
            if (tracer->CurrentOutputOffset.compare_exchange_strong(offset, offset + 1))
            {
                break;
            }
            offset = tracer->CurrentOutputOffset.load();
        }

        // Do we have an pixel to trace?
        if (offset < numPixels)
        {
            // Get the offsets
            const int x = offset % tracer->OutputWidth;
            const int y = tracer->OutputHeight - (offset / tracer->OutputWidth);

            // Send random rays to pixel, i.e. multi-sample
            Vec3 col(0.f, 0.f, 0.f);
            for (int s = 0; s < tracer->NumRaySamples; s++)
            {
                const float u = float(x + RandomFloat()) / float(tracer->OutputWidth);
                const float v = float(y + RandomFloat()) / float(tracer->OutputHeight);

                Ray r = cam.GetRay(u, v);
                col += tracer->trace(r, world, 0, cam.GetClearColor());
            }
            col /= float(tracer->NumRaySamples);

            // Write color to output buffer
            tracer->OutputBuffer[offset] = col;

            // Write RGBA
            {
                int rgbaOffset = offset * 4;
                int ir, ig, ib, ia;
                GetRGBA8888(col, true, ir, ig, ib, ia);

                tracer->OutputBufferRGBA[rgbaOffset + 0] = (uint8_t)ir;
                tracer->OutputBufferRGBA[rgbaOffset + 1] = (uint8_t)ig;
                tracer->OutputBufferRGBA[rgbaOffset + 2] = (uint8_t)ib;
                tracer->OutputBufferRGBA[rgbaOffset + 3] = (uint8_t)ia;
            }
        }
    }

    tracer->NumThreadsDone++;
    if (tracer->NumThreadsDone.load() >= tracer->NumThreads)
    {
        // Last thread, signal trace is done
        tracer->RaytraceEvent.Signal();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 Raytracer::trace(const Ray& r, IHitable *world, int depth, const Vec3& clearColor)
{
    // Increment num rays fired
    TotalRaysFired++;

    HitRecord rec;
    if (world->Hit(r, 0.001f, FLT_MAX, rec))
    {
        Ray  scattered;
        Vec3 attenuation;
        Vec3 emitted = rec.MatPtr->Emitted(rec.U, rec.V, rec.P);
        if (depth < MaxDepth && rec.MatPtr->Scatter(r, rec, attenuation, scattered))
        {
            return emitted + attenuation * trace(scattered, world, depth + 1, clearColor);
        }
        else
        {
            return emitted;
        }
    }
    else
    {
        return clearColor;
    }
}
