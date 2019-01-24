#pragma once

#include <cfloat>
#include <assert.h>

// ----------------------------------------------------------------------------------------------------------------------------

#define DEBUG_PRINTF(fmt, ...) RenderDebugPrintf(fmt, ##__VA_ARGS__)
void RenderDebugPrintf(const char *fmt, ...);

// ----------------------------------------------------------------------------------------------------------------------------

#if _DEBUG
    #define RTL_ASSERT(expression)  assert(expression)
#else
    // Enable asserts in release builds
    void _RtlAssert(const char* message, const char* file, int line);
    #define RTL_ASSERT(expression)  (void)((!!(expression)) || (_RtlAssert(#expression, __FILE__, (int)__LINE__), 0))
#endif

// ----------------------------------------------------------------------------------------------------------------------------

#if 1 // _DEBUG
    #define SANITY_CHECK_FLOAT(f)       RTL_ASSERT(!isnan(f) && !isinf(f))
    #define ENABLE_VEC3_SANITY_CHECK
#else
    #define SANITY_CHECK_FLOAT(f)       
#endif
