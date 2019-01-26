#include "Util.h"
#include <windows.h>
#include <stdlib.h>
#include <varargs.h>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>

// ----------------------------------------------------------------------------------------------------------------------------

void PrintProgress(const char* otherInfo, double percentage)
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
