#include "Util.h"
#include "Raytracer.h"
#include "ImageIO.h"

#include <stdlib.h>
#include <stdio.h>
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

#if defined(PLATFORM_WINDOWS)
    #include <windows.h>
    #include <filesystem>
    #define STD_FILESYSTEM_SUPPORTED
#else
    #define vsprintf_s  vsprintf
    #include <limits.h>
#endif

// ----------------------------------------------------------------------------------------------------------------------------

void PrintCompletion(const char* otherInfo, double percentage)
{
    static const char* PBSTR = "||||||||||||||||||||";
    const int PBWIDTH = (int)strlen(PBSTR);

    int lpad = (int)(percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;

    printf("\r%s [%.*s%*s]", otherInfo, lpad, PBSTR, rpad, "");
    
    fflush(stdout);
}

// ----------------------------------------------------------------------------------------------------------------------------

const char* ProgressPrint(Raytracer* tracer, bool enablePercentBar)
{
    static char buf[256];

    Raytracer::Stats stats = tracer->GetStats();
    double percentage = double(double(stats.NumPixelSamples) / double(stats.TotalNumPixelSamples));
    int    percentInt = (int)(percentage * 100);
    int    numMinutes = stats.TotalTimeInSeconds / 60;
    int    numSeconds = stats.TotalTimeInSeconds % 60;

    snprintf(buf, 256, "#done:%3d%% #time:%dm:%2ds  #rays:%" PRId64 "  #pixels:%" PRId64 "  #pdfQueryRetries:%d",
        percentInt, numMinutes, numSeconds, stats.TotalRaysFired, stats.NumPixelSamples, stats.NumPdfQueryRetries);

    if (enablePercentBar)
    {
        PrintCompletion(buf, percentage);
    }
    else
    {
        printf("\r%s", buf);
        fflush(stdout);
    }

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
    #else
        assert(false);
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

// ----------------------------------------------------------------------------------------------------------------------------

std::string GetParentDir(std::string filePath)
{
#if defined(STD_FILESYSTEM_SUPPORTED)
    return std::filesystem::u8path(filePath.c_str()).parent_path().string();
#else
    // Kind of crap that std::filesystem is not supported widely yet (i.e. MacOS/Xcode)
    // Search from the end until we find a slash
    int i;
    for (i = (int)filePath.size() - 1; i >= 0; i--)
    {
        if (filePath[i] == '\\' || filePath[i] == '/')
        {
            break;
        }
    }

    return filePath.substr(0, i);
#endif
}

// ----------------------------------------------------------------------------------------------------------------------------

std::string GetAbsolutePath(std::string relativePath)
{
#if defined(STD_FILESYSTEM_SUPPORTED)
    return std::filesystem::absolute(std::filesystem::u8path(relativePath.c_str())).string();
#else
    char resolvedPath[1024];
    realpath(relativePath.c_str(), resolvedPath);
    return std::string(resolvedPath);
#endif
}
