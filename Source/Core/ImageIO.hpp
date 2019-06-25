// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

#include "ImageIO.h"
#include "Util.h"
#include "StbImage/stb_image_write.h"

#include <iostream>
#include <fstream>
#include <iomanip>

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

void ImageIO::WriteToPPMFile(const Vec4* buffer, int width, int height, const char* pOutFilename)
{
    std::ofstream outFile(pOutFilename);

    printf("\nWriting ppm file %s...\n", pOutFilename);

    outFile << "P3\n" << width << " " << height << "\n255\n";

    const int numPixels = width * height;
    for (int i = 0; i < numPixels; i++)
    {
        // Get the pixel
        Vec4 col = buffer[i];

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

void ImageIO::WriteToPNGFile(const Vec4* buffer, int width, int height, const char* pOutFilename)
{
    const int numPixels = width * height;
    uint8_t* convertedBuffer = new uint8_t[width * height * 4];
    for (int i = 0; i < numPixels; i++)
    {
        // Get the pixel
        Vec4 col = buffer[i];

        // Convert to rgba and gamma correct
        int ir, ig, ib, ia;
        GetRGBA8888(col, false, ir, ig, ib, ia);

        int offset = (i * 4);
        convertedBuffer[offset + 0] = ir;
        convertedBuffer[offset + 1] = ig;
        convertedBuffer[offset + 2] = ib;
        convertedBuffer[offset + 3] = ia;
    }

    stbi_write_png(pOutFilename, width, height, 4, convertedBuffer, width * 4);
}
