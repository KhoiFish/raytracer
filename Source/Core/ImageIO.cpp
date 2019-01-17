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

        // Gamma correct
        col = Vec3(sqrt(col.R()), sqrt(col.G()), sqrt(col.B()));

        // Write pixel to file
        int ir = int(255.99*col.R());
        int ig = int(255.99*col.G());
        int ib = int(255.99*col.B());

        ir = (ir > 255) ? 255 : ir;
        ig = (ig > 255) ? 255 : ig;
        ib = (ib > 255) ? 255 : ib;

        outFile << ir << " " << ig << " " << ib << "\n";

        // Print progress
        if ((i % width) == 0)
        {
            PrintProgress("", float(i) / float(numPixels));
        }
    }

    printf("\nFinished writing ppm file!\n");
}
