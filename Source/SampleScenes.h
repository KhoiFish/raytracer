#pragma once

#include "Core/IHitable.h"
#include "Core/Camera.h"
#include "Core/Raytracer.h"
#include "Core/WorldScene.h"

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
WorldScene* SampleSceneRandom(const Camera& cam);
WorldScene* SampleSceneCreateTwoPerlinSpheres();
WorldScene* SampleSceneSimpleLight();
WorldScene* SampleSceneCornellBox(bool smoke);
WorldScene* SampleSceneMesh();
WorldScene* SampleSceneFinal();
