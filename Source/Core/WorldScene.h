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

#pragma once

#include "IHitable.h"
#include "HitableList.h"
#include "Camera.h"
#include <vector>

// ----------------------------------------------------------------------------------------------------------------------------

class WorldScene
{
public:

    virtual ~WorldScene()
    {
        if (World != nullptr)
        {
            delete World;
            World = nullptr;
        }

        if (LightShapes != nullptr)
        {
            delete LightShapes;
            LightShapes = nullptr;
        }
    }

    static inline WorldScene* Create(const Camera& camera, std::vector <IHitable*> hitables, std::vector<IHitable*> lightShapes)
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

        // Lightshapes are shared hitables, don't delete them
        newScene->LightShapes = new HitableList(lightShapesList, lightShapesSize, false);

        newScene->TheCamera = camera;

        return newScene;
    }

    static inline WorldScene* Create(const Camera& camera, IHitable** hitables, int numHitables, IHitable** lightShapes = nullptr, int numLightShapes = 0)
    {
        WorldScene* newScene = new WorldScene();
        newScene->World      = new HitableList(hitables, numHitables);

        if (lightShapes != nullptr && numLightShapes > 0)
        {
            // Lightshapes are shared hitables, don't delete them
            newScene->LightShapes = new HitableList(lightShapes, numLightShapes, false);
        }

        newScene->TheCamera = camera;

        return newScene;
    }

    static inline WorldScene* Create(const Camera& camera, IHitable* oneHitable, IHitable* oneLightShape = nullptr)
    {
        IHitable** hitablesList = new IHitable*[1];
        hitablesList[0] = oneHitable;

        IHitable** lightShapesList = nullptr;
        if (oneLightShape != nullptr)
        {
            lightShapesList = new IHitable*[1];
            lightShapesList[0] = oneLightShape;
        }

        return Create(camera, hitablesList, 1, lightShapesList, 1);
    }

    inline HitableList* GetWorld()       { return World; }
    inline HitableList* GetLightShapes() { return LightShapes; }
    inline Camera&      GetCamera()      { return TheCamera; }

private:

    WorldScene() : World(nullptr), LightShapes(nullptr) {}

private:

    HitableList* World;
    HitableList* LightShapes;
    Camera       TheCamera;
};
