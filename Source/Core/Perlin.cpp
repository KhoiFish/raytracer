#include "Perlin.h"
#include <ctime>

class SeedRandom
{
public:
    SeedRandom()
    {
        srand((unsigned int)time(NULL));
    }
};

// This should seed random on app startup
static SeedRandom seedRandom;

Vec3*  Perlin::RanVec   = Perlin::perlinGenerate();
int*   Perlin::PermX    = Perlin::PerlinGeneratePerm();
int*   Perlin::PermY    = Perlin::PerlinGeneratePerm();
int*   Perlin::PermZ    = Perlin::PerlinGeneratePerm();