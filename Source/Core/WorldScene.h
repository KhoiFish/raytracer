#pragma once

#include "IHitable.h"
#include "HitableList.h"
#include <vector>

// ----------------------------------------------------------------------------------------------------------------------------

class WorldScene
{
public:

    static inline WorldScene* Create(std::vector <IHitable*> hitables, std::vector<IHitable*> lightShapes)
    {
        WorldScene* newScene = new WorldScene();

        const int  worldListSize = (int)hitables.size();
        IHitable** worldList     = new IHitable*[worldListSize];
        for (int i = 0; i < worldListSize; i++)
        {
            worldList[i] = hitables[i];
        }
        newScene->World = new HitableList(worldList, worldListSize);

        const int  lightShapesSize = (int)lightShapes.size();
        IHitable** lightShapesList = new IHitable*[lightShapesSize];
        for (int i = 0; i < lightShapesSize; i++)
        {
            lightShapesList[i] = lightShapes[i];
        }
        newScene->LightShapes = new HitableList(lightShapesList, lightShapesSize);
    }

    static inline WorldScene* Create(IHitable** hitables, int numHitables, IHitable** lightShapes = nullptr, int numLightShapes = 0)
    {
        WorldScene* newScene = new WorldScene();
        newScene->World      = new HitableList(hitables, numHitables);

        if (lightShapes != nullptr && numLightShapes > 0)
        {
            newScene->LightShapes = new HitableList(lightShapes, numLightShapes);
        }
        
        return newScene;
    }

    static inline WorldScene* Create(IHitable* oneHitable, IHitable* oneLightShape = nullptr)
    {
        IHitable** hitablesList = new IHitable*[1];
        hitablesList[0] = oneHitable;

        IHitable** lightShapesList = nullptr;
        if (oneLightShape != nullptr)
        {
            lightShapesList = new IHitable*[1];
            lightShapesList[0] = oneLightShape;
        }

        return Create(hitablesList, 1, lightShapesList, 1);
    }

    inline HitableList* GetWorld()       { return World; }
    inline HitableList* GetLightShapes() { return LightShapes; }

private:

    WorldScene() : World(nullptr), LightShapes(nullptr) {}

private:

    HitableList* World;
    HitableList* LightShapes;
};