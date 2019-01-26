#include "ImageIO.h"
#include "Util.h"
#include "StbImage/stb_image_write.h"

#include <iostream>
#include <fstream>
#include <iomanip>

// ----------------------------------------------------------------------------------------------------------------------------

void ImageIO::WriteToPPMFile(const Vec3* buffer, int width, int height, const char* pOutFilename)
{
    std::ofstream outFile(pOutFilename);

    printf("\nWriting ppm file %s...\n", pOutFilename);

    outFile << "P3\n" << width << " " << height << "\n255\n";

    const int numPixels = width * height;
    for (int i = 0; i < numPixels; i++)
    {
        // Get the pixel
        Vec3 col = buffer[i];

        int ir, ig, ib, ia;
        GetRGBA8888(col, true, ir, ig, ib, ia);

        outFile << ir << " " << ig << " " << ib << "\n";

        // Print progress
        if ((i % width) == 0)
        {
            PrintCompletion("", float(i) / float(numPixels));
        }
    }

    printf("\nFinished writing ppm file!\n");
}

// ----------------------------------------------------------------------------------------------------------------------------

void ImageIO::WriteToPNGFile(const Vec3* buffer, int width, int height, const char* pOutFilename)
{
    const int numPixels = width * height;
    uint8_t* convertedBuffer = new uint8_t[width * height * 4];
    for (int i = 0; i < numPixels; i++)
    {
        // Get the pixel
        Vec3 col = buffer[i];

        // Convert to rgba and gamma correct
        int ir, ig, ib, ia;
        GetRGBA8888(col, true, ir, ig, ib, ia);

        int offset = (i * 4);
        convertedBuffer[offset + 0] = ir;
        convertedBuffer[offset + 1] = ig;
        convertedBuffer[offset + 2] = ib;
        convertedBuffer[offset + 3] = ia;
    }

    stbi_write_png(pOutFilename, width, height, 4, convertedBuffer, width * 4);
}
