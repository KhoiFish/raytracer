#include "Util.h"
#include "Raytracer.h"
#include "ImageIO.h"

#include <windows.h>
#include <stdlib.h>
#include <varargs.h>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>
#include <iostream>
#include <fstream>

// ----------------------------------------------------------------------------------------------------------------------------

void PrintCompletion(const char* otherInfo, double percentage)
{
    static const char* PBSTR = "||||||||||||||||||||";
    const int PBWIDTH = (int)strlen(PBSTR);

    int val = (int)(percentage * 100);
    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;

    printf("\r%s %3d%% [%.*s%*s]", otherInfo, val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

// ----------------------------------------------------------------------------------------------------------------------------

const char* ProgressPrint(Raytracer* tracer)
{
    static char buf[256];

    Raytracer::Stats stats = tracer->GetStats();
    float percentage = float(stats.NumPixelsTraced) / float(stats.TotalNumPixels);
    int   numMinutes = stats.TotalTimeInSeconds / 60;
    int   numSeconds = stats.TotalTimeInSeconds % 60;

    snprintf(buf, 256, "#time:%dm:%2ds  #rays:%lld  #pixels:%d  #pdfQueryRetries:%d ",
        numMinutes, numSeconds, stats.TotalRaysFired, stats.NumPixelsTraced, stats.NumPdfQueryRetries);

    PrintCompletion(buf, percentage);

    return buf;
}

// ----------------------------------------------------------------------------------------------------------------------------

std::string GetTimeAndDateString()
{
    auto n = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(n);
    std::tm timeBuf;
    localtime_s(&timeBuf, &in_time_t);

    std::stringstream ss;
    ss << std::put_time(&timeBuf, "%Y-%m-%d-%H.%M.%S");

    return ss.str();
}

// ----------------------------------------------------------------------------------------------------------------------------

void WriteImageAndLog(Raytracer* raytracer, std::string name)
{
    std::string baseFilename = std::string(RT_OUTPUT_IMAGE_DIR) + name + std::string(".") + std::string(GetTimeAndDateString());
    ImageIO::WriteToPNGFile(raytracer->GetOutputBuffer(), raytracer->GetOutputWidth(), raytracer->GetOutputHeight(), (baseFilename + std::string(".png")).c_str());

    std::ofstream out((baseFilename + std::string(".log")).c_str());
    if (out.is_open())
    {
        out << ProgressPrint(raytracer);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

void RenderDebugPrintf(const char *fmt, ...)
{
    char buf[1024];
    va_list arg;

    va_start(arg, fmt);
    vsprintf_s(buf, fmt, arg);
    va_end(arg);

    printf(buf);

    #if defined(PLATFORM_WINDOWS)
        OutputDebugStringA(buf);
    #endif
}

// ----------------------------------------------------------------------------------------------------------------------------

void _RtlAssert(const char* message, const char* file, int line)
{
    DEBUG_PRINTF("Assert!\n %s --- %s:%d", message, file, line);

    #if defined(PLATFORM_WINDOWS)
        DebugBreak();
    #endif
}
