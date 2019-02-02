#include "Util.h"
#include "Raytracer.h"
#include "ImageIO.h"

#if defined(PLATFORM_WINDOWS)
    #include <windows.h>
#else
    #define vsprintf_s  vsprintf
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>
#include <iostream>
#include <fstream>
#include <inttypes.h>

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
    double percentage = double(double(stats.NumPixelSamples) / double(stats.TotalNumPixelSamples));
    int    numMinutes = stats.TotalTimeInSeconds / 60;
    int    numSeconds = stats.TotalTimeInSeconds % 60;

    snprintf(buf, 256, "#time:%dm:%2ds  #rays:%" PRId64 "  #pixels:%" PRId64 "  #pdfQueryRetries:%d ",
        numMinutes, numSeconds, stats.TotalRaysFired, stats.NumPixelSamples, stats.NumPdfQueryRetries);

    PrintCompletion(buf, percentage);

    return buf;
}

// ----------------------------------------------------------------------------------------------------------------------------

// Disable annoying windows warning about localtime being unsecure
#pragma warning(push)
#pragma warning(disable : 4996)

std::string GetTimeAndDateString()
{
    std::stringstream ss;

    std::time_t t = std::time(nullptr);
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d-%H.%M.%S");

    return ss.str();
}

#pragma warning(pop) 

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

    printf("%s", buf);

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

// ----------------------------------------------------------------------------------------------------------------------------

std::vector<std::string> GetStringTokens(std::string sourceStr, std::string delim)
{
    std::vector<std::string> tokenList;
    size_t last = 0, next = 0;
    while ((next = sourceStr.find(delim, last)) != std::string::npos)
    {
        tokenList.push_back(sourceStr.substr(last, next - last));
        last = next + delim.length();
    }
    tokenList.push_back(sourceStr.substr(last, next - last));

    return tokenList;
}
