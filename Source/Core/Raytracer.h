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

#pragma once

#include <thread>
#include <atomic>
#include <cstdint>

#include "Ray.h"
#include "Vec4.h"
#include "IHitable.h"
#include "HitableList.h"
#include "Material.h"
#include "Camera.h"
#include "Util.h"
#include "ThreadEvent.h"
#include "Pdf.h"
#include "WorldScene.h"

// ----------------------------------------------------------------------------------------------------------------------------
namespace Core
{
    class Raytracer
    {
    public:

        typedef std::chrono::time_point<std::chrono::system_clock> StdTime;
        typedef void(*OnTraceComplete)(Raytracer* tracer, bool actuallyFinished);

        struct Stats
        {
            int64_t     TotalRaysFired;
            int64_t     NumPixelSamples;
            int64_t     TotalNumPixelSamples;
            int         CompletedSampleCount;
            int         NumPdfQueryRetries;
            int         TotalTimeInSeconds;
        };

    public:

        Raytracer(int width, int height, int numSamples, int maxDepth, int numThreads, bool pdfEnabled);
        ~Raytracer();

        void             BeginRaytrace(WorldScene* scene, OnTraceComplete onComplete = nullptr);
        bool             WaitForTraceToFinish(int timeoutMicroSeconds);
        Stats            GetStats() const;

        inline Vec4*     GetOutputBuffer() const     { return OutputBuffer; }
        inline uint8_t*  GetOutputBufferRGBA() const { return OutputBufferRGBA; }
        inline int       GetOutputWidth() const      { return OutputWidth; }
        inline int       GetOutputHeight() const     { return OutputHeight; }
        inline bool      IsTracing() const           { return IsRaytracing; }

    private:

        static void      threadTraceNextPixel(int id, Raytracer* tracer, WorldScene* scene);
        Vec4             trace(WorldScene* scene, const Ray& r, int depth);
        void             cleanupRaytrace();

    private:

        // Output options
        int                   OutputWidth;
        int                   OutputHeight;
        Vec4*                 OutputBuffer;
        uint8_t*              OutputBufferRGBA;

        // Tracing options
        int                   NumRaySamples;
        int                   MaxDepth;
        int                   NumThreads;
        bool                  PdfEnabled;

        // Thread tracking
        std::atomic<int64_t>  CurrentPixelSampleOffset;
        std::atomic<int64_t>  TotalRaysFired;
        std::atomic<int>      NumThreadsDone;
        std::atomic<int>      NumPdfQueryRetries;
        std::atomic<bool>     ThreadExitRequested;
        StdTime               StartTime;
        StdTime               EndTime;
        std::thread**         ThreadPtrs;
        ThreadEvent           RaytraceEvent;
        bool                  IsRaytracing;
        OnTraceComplete       OnComplete;
    };
}
