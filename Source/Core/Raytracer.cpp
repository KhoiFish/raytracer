#include "Raytracer.h"
#include "XYZRect.h"
#include "BVHNode.h"
#include <cfloat>
#include <vector>
#include <math.h>

// ----------------------------------------------------------------------------------------------------------------------------

Raytracer::Raytracer(int width, int height, int numSamples, int maxDepth, int numThreads, bool pdfEnabled) 
    : OutputWidth(width)
    , OutputHeight(height)
    , NumRaySamples(numSamples)
    , MaxDepth(maxDepth)
    , NumThreads(numThreads)
    , PdfEnabled(pdfEnabled)
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

void Raytracer::BeginRaytrace(WorldScene* scene, OnTraceComplete onComplete)
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
        ThreadPtrs[i] = new std::thread(threadTraceNextPixel, i, this, scene);
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
    const StdTime endTime = IsRaytracing ? std::chrono::system_clock::now() : EndTime.load();

    Stats stats;
    stats.TotalRaysFired       = TotalRaysFired.load();
    stats.NumPixelSamples      = CurrentPixelSampleOffset.load();
    stats.TotalNumPixelSamples = (OutputWidth * OutputHeight) * NumRaySamples;
    stats.NumPdfQueryRetries   = NumPdfQueryRetries.load();
    stats.TotalTimeInSeconds   = (int)std::chrono::duration<double>(endTime - StartTime.load()).count();
    
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

void Raytracer::threadTraceNextPixel(int id, Raytracer* tracer, WorldScene* scene)
{
    const int     numPixels         = tracer->OutputWidth * tracer->OutputHeight;
    const int64_t totalPixelSamples = int64_t(numPixels) * int64_t(tracer->NumRaySamples);
    const int     tileLength        = 32;
    const int     tileArea          = tileLength * tileLength;
    const int     numXTiles         = tracer->OutputWidth  / tileLength;
    const int     numYTiles         = tracer->OutputHeight / tileLength;
    const bool    tileEnabled       = (tracer->OutputWidth == tracer->OutputHeight) && ((tracer->OutputWidth % tileLength) == 0);

    // Thread starts here
    int64_t pixelSampleOffset = tracer->CurrentPixelSampleOffset.load();
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

        // Do we have a pixel to trace?
        if (pixelSampleOffset < totalPixelSamples)
        {
            const int curOffset = pixelSampleOffset % numPixels;

            // Compute effective offsets to the buffer
            int x, y, outIdx;
            if (tileEnabled)
            {
                // Figure out which tile we are on
                const int tileId     = curOffset / tileArea;
                const int tileOffset = curOffset % tileArea;
                const int tileX      = tileId % numXTiles;
                const int tileY      = tileId / numXTiles;

                // Compute offsets
                x      = (tileX * tileLength) + (tileOffset % tileLength);
                y      = (tileY * tileLength) + (tileOffset / tileLength);
                outIdx = (y * tracer->OutputWidth) + x;
            }
            else
            {
                // Trace via scan-lines
                x      = curOffset % tracer->OutputWidth;
                y      = curOffset / tracer->OutputWidth;
                outIdx = curOffset;
            }

            // Get a random ray to the pixel
            const float u = 0.f + float(x + RandomFloat()) / float(tracer->OutputWidth);
            const float v = 1.f - float(y + RandomFloat()) / float(tracer->OutputHeight);
            const Ray   r = scene->GetCamera().GetRay(u, v);

            // Trace
            const Vec3 outColor = tracer->trace(scene, r, 0);

            // Accumulate color to output buffer
            tracer->OutputBuffer[outIdx] += outColor;

            // Write RGBA (for previewing)
            {
                const int64_t numSamples = (pixelSampleOffset / int64_t(numPixels)) + 1;
                Vec3          curCol     = tracer->OutputBuffer[outIdx] * float(1.0 / double(numSamples));

                int rgbaOffset = outIdx * 4;
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

Vec3 Raytracer::trace(WorldScene* scene, const Ray& r, int depth)
{
    // Bail if we're requested to exit
    if (ThreadExitRequested.load() == true)
    {
        return scene->GetCamera().GetBackgroundColor();
    }

    // Increment num rays fired
    TotalRaysFired++;

    // Let's do the path trace
    HitRecord hitRec;
    if (scene->GetWorld()->Hit(r, 0.001f, FLT_MAX, hitRec))
    {
        // We got a hit, get the emitted color
        const Vec3 emitted = hitRec.MatPtr->Emitted(r, hitRec, hitRec.U, hitRec.V, hitRec.P);

        // Test for ray scatter
        Material::ScatterRecord scatterRec;
        if (depth < MaxDepth && hitRec.MatPtr->Scatter(r, hitRec, scatterRec))
        {
            if (scatterRec.IsSpecular)
            {
                return scatterRec.Attenuation * trace(scene, scatterRec.SpecularRay, depth + 1);
            }
            else
            {
                Ray   scattered  = scatterRec.ScatteredClassic;
                float scatterPdf = 1.f;
                float pdfValue   = 1.f;
                if (PdfEnabled)
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
                    int   numPdfQueries = 0;
                    do
                    {
                        scattered = Ray(hitRec.P, pdf->Generate(), r.Time());
                        pdfValue = pdf->Value(scattered.Direction());
                        numPdfQueries++;
                    } while (pdfValue <= 0.0000001f);

                    if (numPdfQueries > 1)
                    {
                        NumPdfQueryRetries += (numPdfQueries - 1);
                    }

                    scatterPdf = hitRec.MatPtr->ScatteringPdf(r, hitRec, scattered);
                }

                // Compute the aggregate color
                const Vec3 color = trace(scene, scattered, depth + 1);
                const Vec3 ret   = emitted + (scatterRec.Attenuation * scatterPdf * color / pdfValue);

                VEC3_SANITY_CHECK(ret);
                return ret;
            }
        }
       
        // No scattering, or reached max depth. Return emitted color
        return emitted;
    }
    
    // No hits, return background color
    return scene->GetCamera().GetBackgroundColor();
}
