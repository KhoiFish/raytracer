#pragma once

#include "Vec4.h"
#include <stdint.h>

// ----------------------------------------------------------------------------------------------------------------------------

class ImageIO
{
public:

    static void WriteToPPMFile(const Vec4* buffer, int width, int height, const char* pOutFilename);
    static void WriteToPNGFile(const Vec4* buffer, int width, int height, const char* pOutFilename);
};
