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
    SceneMesh,
    SceneFinal,

    MaxScene
};

constexpr const char* SampleSceneNames[MaxScene] =
{
    "SceneRandom",
    "SceneCornell",
    "SceneCornellSmoke",
    "SceneMesh",
    "SceneFinal",
};

// ----------------------------------------------------------------------------------------------------------------------------

WorldScene* GetSampleScene(SampleScene sceneType);
