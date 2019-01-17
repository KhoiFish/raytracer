#pragma once

#include "Core/IHitable.h"
#include "Core/Camera.h"

// ----------------------------------------------------------------------------------------------------------------------------

enum SampleScene
{
    SceneRandom = 0,
    SceneCornell,
    SceneCornellSmoke,
    SceneFinal,

    MaxScene
};

// ----------------------------------------------------------------------------------------------------------------------------

Camera      GetCameraForSample(SampleScene scene, float aspect);
IHitable*   SampleSceneRandom(const Camera& cam);
IHitable*   SampleSceneCreateTwoPerlinSpheres();
IHitable*   SampleSceneSimpleLight();
IHitable*   SampleSceneCornellBox(bool smoke);
IHitable*   SampleSceneFinal();
