#include "Raytracer.h"
#include "XYZRect.h"
#include "BVHNode.h"
#include <cfloat>
#include <vector>
#include <math.h>

// ----------------------------------------------------------------------------------------------------------------------------

static inline Vec3 DeNaN(const Vec3& c)
{
    Vec3 temp = c;
    for (int i = 0; i < 3; i++)
    {
        if (isnan(temp[i]))
        {
            temp[i] = 0;
        }
    }

    return temp;
}

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
    , ThreadExitRequested(false)
    , IsRaytracing(false)
{
    OutputBuffer      = new Vec3[OutputWidth * OutputHeight];
    OutputBufferRGBA  = new uint8_t[OutputWidth * OutputHeight * 4];
    ThreadPtrs        = new std::thread*[NumThreads];
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

void Raytracer::BeginRaytrace(const Camera& cam, WorldScene* scene, OnTraceComplete onComplete)
{
    // Clean up last trace, if any
    cleanupRaytrace();

    IsRaytracing = true;
    TotalRaysFired = 0;
    CurrentOutputOffset = 0;
    NumThreadsDone = 0;
    NumPdfQueryRetries = 0;
    OnComplete = onComplete;
    StartTime = std::chrono::system_clock::now();

    // Memset out buffers
    memset(OutputBufferRGBA, 0, sizeof(uint8_t) * OutputWidth * OutputHeight * 4);

    // Create the threads and run them
    for (int i = 0; i < NumThreads; i++)
    {
        ThreadPtrs[i] = new std::thread(threadTraceNextPixel, i, this, cam, scene);
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
    stats.TotalRaysFired     = TotalRaysFired.load();
    stats.NumPixelsTraced    = CurrentOutputOffset.load();
    stats.TotalNumPixels     = (OutputWidth * OutputHeight);
    stats.NumPdfQueryRetries = NumPdfQueryRetries.load();
    
    StdTime endTime = IsRaytracing ? std::chrono::system_clock::now() : EndTime.load();
    stats.TotalTimeInSeconds = (int)std::chrono::duration<double>(endTime - StartTime.load()).count();
    
    return stats;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Raytracer::cleanupRaytrace()
{
    if (IsRaytracing)
    {
        // Signal threads to shutdown and wait for shutdown
        ThreadExitRequested = true;
        RaytraceEvent.WaitOne(-1);

        // Join all the threads
        for (int i = 0; i < NumThreads; i++)
        {
            ThreadPtrs[i]->join();
            delete ThreadPtrs[i];
            ThreadPtrs[i] = nullptr;
        }

        // Reset event
        ThreadExitRequested = false;
        IsRaytracing = false;
        RaytraceEvent.Reset();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Raytracer::threadTraceNextPixel(int id, Raytracer* tracer, const Camera& cam, WorldScene* scene)
{
    const int numPixels = (tracer->OutputWidth * tracer->OutputHeight);

    int offset = tracer->CurrentOutputOffset.load();
    while (offset < numPixels && (tracer->ThreadExitRequested.load() == false))
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
                col += DeNaN(tracer->trace(r, scene, 0, cam.GetClearColor()));

                // Bail if we're requested to exit
                if (tracer->ThreadExitRequested.load() == true)
                {
                    break;
                }
            }
            col /= float(tracer->NumRaySamples);

            // Write color to output buffer
            tracer->OutputBuffer[offset] = col;

            // Write RGBA
            {
                int rgbaOffset = offset * 4;
                int ir, ig, ib, ia;
                GetRGBA8888(col, false, ir, ig, ib, ia);

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
        tracer->EndTime = std::chrono::system_clock::now();
        if (tracer->OnComplete != nullptr)
        {
            bool actuallyFinished = tracer->CurrentOutputOffset.load() == numPixels;
            tracer->OnComplete(tracer, actuallyFinished);
        }

        // Last thread, signal trace is done
        tracer->RaytraceEvent.Signal();
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 Raytracer::trace(const Ray& r, WorldScene* scene, int depth, const Vec3& clearColor)
{
    // Bail if we're requested to exit
    if (ThreadExitRequested.load() == true)
    {
        return clearColor;
    }

    // Increment num rays fired
    TotalRaysFired++;

    HitRecord hitRec;
    if (scene->GetWorld()->Hit(r, 0.001f, FLT_MAX, hitRec))
    {
        Material::ScatterRecord scatterRec;
        Vec3 emitted = hitRec.MatPtr->Emitted(r, hitRec, hitRec.U, hitRec.V, hitRec.P);
        if (depth < MaxDepth && hitRec.MatPtr->Scatter(r, hitRec, scatterRec))
        {
            if (scatterRec.IsSpecular)
            {
                return scatterRec.Attenuation * trace(scatterRec.SpecularRay, scene, depth + 1, clearColor);
            }
            else
            {
                // Prepare the pdf query
                HitablePdf  hitablePdf(scene->GetLightShapes(), hitRec.P);
                MixturePdf  mixPdf(&hitablePdf, scatterRec.Pdf);
                Pdf*        pdf = scatterRec.Pdf;
                if (scene->GetLightShapes() != nullptr)
                {
                    pdf = &mixPdf;
                }

                // PDF values may be zero or close to it, or close to infinity.
                // We retry in these cases
                Ray   scattered;
                float pdfValue;
                int   numPdfQueries = 0;
                do
                {
                   scattered = Ray(hitRec.P, pdf->Generate(), r.Time());
                   pdfValue  = pdf->Value(scattered.Direction());
                   numPdfQueries++;
                } while (pdfValue <= 0.0000001f);

                if (numPdfQueries > 1)
                {
                    NumPdfQueryRetries += (numPdfQueries - 1);
                }
                
                // Compute the aggregate color
                const float scatterPdf = hitRec.MatPtr->ScatteringPdf(r, hitRec, scattered);
                const Vec3  tracedVec  = trace(scattered, scene, depth + 1, clearColor);
                const Vec3  ret        = emitted + (scatterRec.Attenuation * scatterPdf * tracedVec / pdfValue);

                // When enabled, this will test for valid floats
                VEC3_SANITY_CHECK(emitted);
                VEC3_SANITY_CHECK(scatterRec.Attenuation);
                SANITY_CHECK_FLOAT(scatterPdf);
                VEC3_SANITY_CHECK(tracedVec);
                SANITY_CHECK_FLOAT(pdfValue);
                VEC3_SANITY_CHECK(ret);

                return ret;
            }
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
