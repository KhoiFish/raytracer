#pragma once

#include "Vec3.h"
#include <stdint.h>

// ----------------------------------------------------------------------------------------------------------------------------

class ImageIO
{
public:

    static void WriteToPPMFile(const Vec3* buffer, int width, int height, const char* pOutFilename);
    static void WriteToPNGFile(const Vec3* buffer, int width, int height, const char* pOutFilename);
};
