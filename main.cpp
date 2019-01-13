#include <iostream>
#include <fstream>
#include <cfloat>
#include "Vec3.h"
#include "Ray.h"
#include "Sphere.h"
#include "HitableList.h"
#include "Camera.h"
#include "Util.h"
#include "Material.h"
#include "Raytracer.h"

// ----------------------------------------------------------------------------------------------------------------------------

static Hitable* randomScene()
{
	const int numObjects = 500;
	Hitable **list = new Hitable*[numObjects + 1];
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
					list[i++] = new Sphere(center, .2f, new MLambertian(Vec3(RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat())));
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

	return new HitableList(list, i);
}

// ----------------------------------------------------------------------------------------------------------------------------

int main()
{
	// Raytracer params
	const int    outputWidth  = 3840;
	const int    outputHeight = 2160;
	const int    numSamples   = 1;
	const int    maxDepth     = 50;
	const int    numThreads   = 8;
	const float  aspect       = float(outputWidth) / float(outputHeight);

	// Create world
	Hitable* world = randomScene();

	// Setup camera
	Camera cam(
		Vec3(13, 2, 3), // Look from
		Vec3(0, 0, 0),  // Look at
		Vec3(0, 1, 0),  // Up vec
		20.f,           // Vertical FOV
		aspect,         // Aspect
		.1f,            // Aperture
		10.f);          // Distance to focus

	// Ray trace world
	Raytracer tracer(outputWidth, outputHeight, numSamples, maxDepth, numThreads);
	tracer.Render(cam, world);

	// Write to file
	tracer.WriteOutputToPPMFile(std::ofstream("test.ppm"));

	// Done
	return 0;
}