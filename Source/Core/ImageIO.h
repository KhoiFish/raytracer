#pragma once

#include "Vec3.h"

// ----------------------------------------------------------------------------------------------------------------------------

class ImageIO
{
public:

    static void WriteToPPMFile(const Vec3* buffer, int width, int height, const char* pOutFilename);
};
