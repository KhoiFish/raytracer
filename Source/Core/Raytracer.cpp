#include "Raytracer.h"
#include <iostream>
#include <fstream>
#include <cfloat>
#include <iomanip>

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
    ThreadPtrs = new std::thread*[NumThreads];
    DefaultAmbient = Vec3(0, 0, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

Raytracer::~Raytracer()
{
    cleanupRaytrace();

    delete[] OutputBuffer;
    OutputBuffer = nullptr;

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

void Raytracer::WriteOutputToPPMFile(std::ofstream outFile)
{
    printf("\nWriting ppm file...\n");

    outFile << "P3\n" << OutputWidth << " " << OutputHeight << "\n255\n";

    int numPixels = OutputWidth * OutputHeight;
    for (int i = 0; i < numPixels; i++)
    {
        // Get the pixel
        Vec3 col = OutputBuffer[i];

        // Gamma correct
        col = Vec3(sqrt(col.R()), sqrt(col.G()), sqrt(col.B()));

        // Write pixel to file
        int ir = int(255.99*col.R());
        int ig = int(255.99*col.G());
        int ib = int(255.99*col.B());

        ir = (ir > 255) ? 255 : ir;
        ig = (ig > 255) ? 255 : ig;
        ib = (ib > 255) ? 255 : ib;

        outFile << ir << " " << ig << " " << ib << "\n";

        // Print progress
        if ((i % OutputWidth) == 0)
        {
            PrintProgress("", float(i) / float(numPixels));
        }
    }

    printf("\nFinished writing ppm file!\n");
}

// ----------------------------------------------------------------------------------------------------------------------------

void Raytracer::SetDefaultAmbient(const Vec3& ambient)
{
    DefaultAmbient = ambient;
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
                col += tracer->trace(r, world, 0);
            }
            col /= float(tracer->NumRaySamples);

            // Write color to output buffer
            tracer->OutputBuffer[offset] = col;
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

Vec3 Raytracer::trace(const Ray& r, IHitable *world, int depth)
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
            return emitted + attenuation * trace(scattered, world, depth + 1);
        }
        else
        {
            return emitted;
        }
    }
    else
    {
        return DefaultAmbient;
    }
}
