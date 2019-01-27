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
    , CurrentPixelSampleOffset(0)
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
    CurrentPixelSampleOffset = 0;
    NumThreadsDone = 0;
    NumPdfQueryRetries = 0;
    OnComplete = onComplete;
    StartTime = std::chrono::system_clock::now();

    // Clear out buffers
    const int area = OutputWidth * OutputHeight;
    memset(OutputBufferRGBA, 0, sizeof(uint8_t) * area * 4);
    for (int i = 0; i < area; i++)
    {
        OutputBuffer[i] = Vec3(0, 0, 0);
    }

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
    stats.TotalRaysFired        = TotalRaysFired.load();
    stats.NumPixelSamples       = CurrentPixelSampleOffset.load();
    stats.TotalNumPixelSamples  = (OutputWidth * OutputHeight) * NumRaySamples;
    stats.NumPdfQueryRetries    = NumPdfQueryRetries.load();
    
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
    const int numPixels         = tracer->OutputWidth * tracer->OutputHeight;
    const int totalPixelSamples = numPixels * tracer->NumRaySamples;

    int pixelSampleOffset = tracer->CurrentPixelSampleOffset.load();
    while ((tracer->ThreadExitRequested.load() == false) && pixelSampleOffset < totalPixelSamples)
    {
        // Find the next offset to the pixel to trace
        while (pixelSampleOffset < totalPixelSamples)
        {
            // Try and get the next pixel offset
            if (tracer->CurrentPixelSampleOffset.compare_exchange_strong(pixelSampleOffset, pixelSampleOffset + 1))
            {
                break;
            }
            pixelSampleOffset = tracer->CurrentPixelSampleOffset.load();
        }

        // Do we have an pixel to trace?
        if (pixelSampleOffset < totalPixelSamples)
        {
            // Get the effective output offset
            const int outputOffset = (pixelSampleOffset % numPixels);

            // Get a random ray to the pixel
            const int   x = outputOffset % tracer->OutputWidth;
            const int   y = tracer->OutputHeight - (outputOffset / tracer->OutputWidth);
            const float u = float(x + RandomFloat()) / float(tracer->OutputWidth);
            const float v = float(y + RandomFloat()) / float(tracer->OutputHeight);
            const Ray   r = cam.GetRay(u, v);

            // Trace
            const Vec3 outColor = DeNaN(tracer->trace(r, scene, 0, cam.GetClearColor()));

            // Accumulate color to output buffer
            tracer->OutputBuffer[outputOffset] += outColor;

            // Write RGBA (for previewing)
            {
                const int   numSamples = (pixelSampleOffset / numPixels) + 1;
                Vec3        curCol     = tracer->OutputBuffer[outputOffset] * (1.f / float(numSamples));

                int rgbaOffset = outputOffset * 4;
                int ir, ig, ib, ia;
                GetRGBA8888(curCol, false, ir, ig, ib, ia);

                tracer->OutputBufferRGBA[rgbaOffset + 0] = (uint8_t)ir;
                tracer->OutputBufferRGBA[rgbaOffset + 1] = (uint8_t)ig;
                tracer->OutputBufferRGBA[rgbaOffset + 2] = (uint8_t)ib;
                tracer->OutputBufferRGBA[rgbaOffset + 3] = (uint8_t)ia;
            }
        }
    }

    // This thread is done
    tracer->NumThreadsDone++;

    // Are we the last thread?
    if (tracer->NumThreadsDone.load() >= tracer->NumThreads)
    {
        // The last man out normalizes the output buffer
        const float scale = 1.f / float(tracer->NumRaySamples);
        for (int i = 0; i < numPixels; i++)
        {
            tracer->OutputBuffer[i] *= scale;
        }

        // Mark timestamp and call completion callback
        tracer->EndTime = std::chrono::system_clock::now();
        if (tracer->OnComplete != nullptr)
        {
            bool actuallyFinished = tracer->CurrentPixelSampleOffset.load() == totalPixelSamples;
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
