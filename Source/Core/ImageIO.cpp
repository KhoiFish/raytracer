#include "ImageIO.h"
#include "Util.h"

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
            PrintProgress("", float(i) / float(numPixels));
        }
    }

    printf("\nFinished writing ppm file!\n");
}
