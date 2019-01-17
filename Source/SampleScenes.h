#pragma once

#include "Core/IHitable.h"

// ----------------------------------------------------------------------------------------------------------------------------

enum SampleScene
{
    Random = 0,
    Cornell,
    CornellSmoke,
    Final,

    MaxScene
};

// ----------------------------------------------------------------------------------------------------------------------------

IHitable* SampleSceneRandom(float time0, float time1);
IHitable* SampleSceneCreateTwoPerlinSpheres();
IHitable* SampleSceneSimpleLight();
IHitable* SampleSceneCornellBox(bool smoke);
IHitable* SampleSceneFinal();
