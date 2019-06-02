// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

#include "Raytracer.h"
#include "XYZRect.h"
#include "BVHNode.h"
#include <memory.h>
#include <cfloat>
#include <vector>
#include <math.h>

using namespace Core;

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
    OutputBuffer       = new Vec4[OutputWidth * OutputHeight];
    ZeroedOutputBuffer = new Vec4[OutputWidth * OutputHeight];
    OutputBufferRGBA8888   = new uint8_t[OutputWidth * OutputHeight * 4];
    ThreadPtrs         = new std::thread*[NumThreads];

    for (int i = 0; i < (OutputWidth * OutputHeight); i++)
    {
        ZeroedOutputBuffer[i] = Vec4(0, 0, 0);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

Raytracer::~Raytracer()
{
    cleanupRaytrace();

    delete[] OutputBuffer;
    OutputBuffer = nullptr;

    delete[] OutputBufferRGBA8888;
    OutputBufferRGBA8888 = nullptr;

    delete[] ZeroedOutputBuffer;
    ZeroedOutputBuffer = nullptr;

    delete[] ThreadPtrs;
    ThreadPtrs = nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------

void Raytracer::BeginRaytrace(WorldScene* scene, OnTraceComplete onComplete)
{
    // Clean up last trace, if any
    cleanupRaytrace();
    resetRaytrace();
    OnComplete = onComplete;

    // Create the threads and run them
    LocalThreadData.resize(NumThreads);
    for (int i = 0; i < NumThreads; i++)
    {
        LocalThreadData[i].RestartTrace = false;
        ThreadPtrs[i] = new std::thread(threadTraceNextPixel, i, this, scene);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void Raytracer::resetRaytrace()
{
    IsRaytracing                = true;
    TotalRaysFired              = 0;
    CurrentPixelSampleOffset    = 0;
    NumThreadsDone              = 0;
    NumPdfQueryRetries          = 0;
    StartTime                   = std::chrono::system_clock::now();

    // Clear out buffers
    const int area = OutputWidth * OutputHeight;
    memset(OutputBufferRGBA8888, 0, sizeof(uint8_t) * area * 4);
    memcpy(OutputBuffer, ZeroedOutputBuffer, sizeof(float) * area * 4);
}

// ----------------------------------------------------------------------------------------------------------------------------

void Raytracer::RestartCurrentRaytrace()
{
    // This is not really thread-safe or consistent, but whatevs.
    if (IsRaytracing)
    {
        CurrentPixelSampleOffset.store(0);
        for (int i = 0; i < NumThreads; i++)
        {
            LocalThreadData[i].RestartTrace = true;
        }
        resetRaytrace();
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
    const StdTime endTime   = IsRaytracing ? std::chrono::system_clock::now() : EndTime;
    const int64_t numPixels = int64_t(OutputWidth * OutputHeight);

    Stats stats;
    stats.TotalRaysFired        = TotalRaysFired.load();
    stats.NumPixelSamples       = CurrentPixelSampleOffset.load();
    stats.TotalNumPixelSamples  = numPixels * int64_t(NumRaySamples);
    stats.CompletedSampleCount  = int(CurrentPixelSampleOffset.load() / numPixels);
    stats.NumPdfQueryRetries    = NumPdfQueryRetries.load();
    stats.TotalTimeInSeconds    = (int)std::chrono::duration<double>(endTime - StartTime).count();
    stats.CurrentPixelOffset    = int(CurrentPixelSampleOffset.load() % numPixels);
    
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
        LocalThreadData.clear();

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
    const bool    tileEnabled       = ((tracer->OutputWidth % tileLength) == 0) && ((tracer->OutputHeight % tileLength) == 0);

    // Thread starts here
    int64_t pixelSampleOffset = tracer->CurrentPixelSampleOffset.load();
    while ((tracer->ThreadExitRequested.load() == false) && pixelSampleOffset < totalPixelSamples)
    {
        // Find the next offset to the pixel to trace
        while (pixelSampleOffset < totalPixelSamples)
        {
            // See if we're restarting
            if (tracer->LocalThreadData[id].RestartTrace)
            {
                pixelSampleOffset = tracer->CurrentPixelSampleOffset.load();
                tracer->LocalThreadData[id].RestartTrace = false;
            }

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

            // Trace and accumulate color to output buffer
            tracer->OutputBuffer[outIdx] += tracer->trace(scene, r, 0);

            // Write RGBA (for previewing)
            {
                const int64_t numSamples = (pixelSampleOffset / int64_t(numPixels)) + 1;
                Vec4          curCol     = tracer->OutputBuffer[outIdx] * float(1.0 / double(numSamples));

                int rgbaOffset = outIdx * 4;
                int ir, ig, ib, ia;
                GetRGBA8888(curCol, false, ir, ig, ib, ia);

                tracer->OutputBufferRGBA8888[rgbaOffset + 0] = (uint8_t)ir;
                tracer->OutputBufferRGBA8888[rgbaOffset + 1] = (uint8_t)ig;
                tracer->OutputBufferRGBA8888[rgbaOffset + 2] = (uint8_t)ib;
                tracer->OutputBufferRGBA8888[rgbaOffset + 3] = (uint8_t)ia;
            }
        }
    }

    // This thread is done
    tracer->NumThreadsDone++;

    // Are we the last thread?
    if (tracer->NumThreadsDone.load() >= tracer->NumThreads)
    {
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

Vec4 Raytracer::trace(WorldScene* scene, const Ray& r, int depth)
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
        const Vec4 emitted = hitRec.MatPtr->Emitted(r, hitRec, hitRec.U, hitRec.V, hitRec.P);

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
                    MixturePdf  mixPdf(&hitablePdf, scatterRec.PdfPtr);
                    Pdf*        pdf = scatterRec.PdfPtr;
                    if (scene->GetLightShapes() != nullptr)
                    {
                        pdf = &mixPdf;
                    }

                    scattered  = Ray(hitRec.P, pdf->Generate(), r.Time());
                    pdfValue   = pdf->Value(scattered.Direction());
                    scatterPdf = hitRec.MatPtr->ScatteringPdf(r, hitRec, scattered);
                }

                // Clamp bad pdf values
                if (isnan(pdfValue) || isinf(pdfValue))
                {
                    pdfValue = 1.0f;
                }

                // Compute the aggregate color
                const Vec4 color = trace(scene, scattered, depth + 1);
                const Vec4 ret   = emitted + (scatterRec.Attenuation * scatterPdf * color / pdfValue);

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
