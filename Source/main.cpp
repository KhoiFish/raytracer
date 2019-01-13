#include <iostream>
#include <fstream>
#include <cfloat>
#include "Core/Vec3.h"
#include "Core/Ray.h"
#include "Core/Sphere.h"
#include "Core/MovingSphere.h"
#include "Core/HitableList.h"
#include "Core/Camera.h"
#include "Core/Util.h"
#include "Core/Material.h"
#include "Core/Raytracer.h"
#include "Core/BVHNode.h"

// ----------------------------------------------------------------------------------------------------------------------------

static IHitable* randomScene(float time0, float time1)
{
	const int numObjects = 500;
	IHitable **list = new IHitable*[numObjects + 1];
	list[0] = new Sphere(Vec3(0, -1000, 0), 1000, new MLambertian(Vec3(.5f, .5f, .5f)));

	int i = 1;
	for (int a = -11; a < 11; a++)
	{
		for (int b = -11; b < 11; b++)
		{
			float chooseMat = RandomFloat();
			Vec3 center(a + .9f * RandomFloat(), .2f, b + .9f * RandomFloat());
			if ((center - Vec3(4, .2f, 0)).Length() > .9f)
			{
				if (chooseMat < .8f)
				{
					list[i++] = new MovingSphere(
						center, center + Vec3(0, .5f*RandomFloat(), 0), 
						.0f, 1.f,
						.2f, 
						new MLambertian(Vec3(RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat())
						)
					);
				}
				else if (chooseMat < .95f)
				{
					list[i++] = new Sphere(center, .2f, new MMetal(Vec3(.5f * (1 + RandomFloat()), .5f * (1 + RandomFloat()), .5f * (1 + RandomFloat())), .5f * RandomFloat()));
				}
				else
				{
					list[i++] = new Sphere(center, .2f, new MDielectric(1.5f));
				}
			}
		}
	}

	list[i++] = new Sphere(Vec3(0, 1, 0), 1.f, new MDielectric(1.5f));
	list[i++] = new Sphere(Vec3(-4, 1, 0), 1.f, new MLambertian(Vec3(0.4f, 0.2f, 0.1f)));
	list[i++] = new Sphere(Vec3(4, 1, 0), 1.f, new MMetal(Vec3(0.7f, 0.6f, 0.5f), 0.f));

	// Generate BVH tree
	BVHNode* bvhHead = new BVHNode(list, i, time0, time1);

	return bvhHead;
}

// ----------------------------------------------------------------------------------------------------------------------------

int main()
{
	// Raytracer params
	const int    outputWidth  = 3840;
	const int    outputHeight = 2160;
	const int    numSamples   = 100;
	const int    maxDepth     = 50;
	const int    numThreads   = 8;

	// Camera options
	const Vec3   lookFrom     = Vec3(13, 2, 3);
	const Vec3   lookAt       = Vec3(0, 0, 0);
	const Vec3   upVec        = Vec3(0, 1, 0);
	const float  vertFov      = 20.f;
	const float  aspect       = float(outputWidth) / float(outputHeight);
	const float  aperture     = 0.0f;
	const float  distToFocus  = 10.f;
	const float  shutterTime0 = 0.f;
	const float  shutterTime1 = 1.f;

	// Create world
	IHitable* world = randomScene(shutterTime0, shutterTime1);

	// Setup camera
	Camera cam(
		lookFrom, lookAt, upVec,
		vertFov, aspect, aperture, distToFocus,
		shutterTime0, shutterTime1);

	// Ray trace world
	Raytracer tracer(outputWidth, outputHeight, numSamples, maxDepth, numThreads);
	tracer.Render(cam, world);

	// Write to file
	tracer.WriteOutputToPPMFile(std::ofstream("test.ppm"));

	// Done
	return 0;
}