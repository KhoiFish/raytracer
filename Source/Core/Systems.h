#pragma once

#include <cfloat>
#include <assert.h>

// ----------------------------------------------------------------------------------------------------------------------------

#define RUNTIMEDATA_DIR     "RuntimeData"
#define SHADERBUILD_DIR     "ShaderBuild"

// ----------------------------------------------------------------------------------------------------------------------------

#define DEBUG_PRINTF(fmt, ...) RenderDebugPrintf(fmt, ##__VA_ARGS__)
void RenderDebugPrintf(const char *fmt, ...);

// ----------------------------------------------------------------------------------------------------------------------------

// Enable asserts in release builds
void _RtlAssert(const char* message, const char* file, int line);
#define RTL_ASSERT(expression)  (void)((!!(expression)) || (_RtlAssert(#expression, __FILE__, (int)__LINE__), 0))

// ----------------------------------------------------------------------------------------------------------------------------

#if _DEBUG
    #define SANITY_CHECK_FLOAT(f)       RTL_ASSERT(!isnan(f) && !isinf(f))
    #define ENABLE_VEC3_SANITY_CHECK
#else
    #define SANITY_CHECK_FLOAT(f)       
#endif
