#include <iostream>
#include <fstream>
#include <cfloat>
#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include "hitablelist.h"
#include "camera.h"
#include "util.h"
#include "material.h"
#include "raytracer.h"

// ----------------------------------------------------------------------------------------------------------------------------

static hitable* random_scene()
{
	int n = 500;
	hitable **list = new hitable*[n + 1];
	list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(vec3(.5f, .5f, .5f)));

	int i = 1;
	for (int a = -11; a < 11; a++)
	{
		for (int b = -11; b < 11; b++)
		{
			float choose_mat = drand48();
			vec3 center(a + .9f * drand48(), .2f, b + .9f * drand48());
			if ((center - vec3(4, .2f, 0)).length() > .9f)
			{
				if (choose_mat < .8f)
				{
					list[i++] = new sphere(center, .2f, new lambertian(vec3(drand48()*drand48(), drand48()*drand48(), drand48()*drand48())));
				}
				else if (choose_mat < .95f)
				{
					list[i++] = new sphere(center, .2f, new metal(vec3(.5f * (1 + drand48()), .5f * (1 + drand48()), .5f * (1 + drand48())), .5f * drand48()));
				}
				else
				{
					list[i++] = new sphere(center, .2f, new dielectric(1.5f));
				}
			}
		}
	}

	list[i++] = new sphere(vec3(0, 1, 0), 1.f, new dielectric(1.5f));
	list[i++] = new sphere(vec3(-4, 1, 0), 1.f, new lambertian(vec3(0.4f, 0.2f, 0.1f)));
	list[i++] = new sphere(vec3(4, 1, 0), 1.f, new metal(vec3(0.7f, 0.6f, 0.5f), 0.f));

	return new hitable_list(list, i);
}

// ----------------------------------------------------------------------------------------------------------------------------

int main()
{
	// Raytracer params
	const int outputWidth = 1200;
	const int outputHeight = 800;
	const int numSamples = 100;
	const int maxDepth = 50;
	const int numThreads = 8;
	const float aspect = float(outputWidth) / float(outputHeight);

	// Create world
	hitable* world = random_scene();

	// Setup camera
	camera cam(
		vec3(13, 2, 3), // Look from
		vec3(0, 0, 0),  // Look at
		vec3(0, 1, 0),  // Up vec
		20.f,           // Vertical FOV
		aspect,         // Aspect
		.1f,            // Aperture
		10.f);          // Distance to focus

	// Ray trace world
	raytracer tracer(outputWidth, outputHeight, numSamples, maxDepth, numThreads);
	tracer.render(cam, world);

	// Write to file
	tracer.writeOutputToPPMFile(std::ofstream("test.ppm"));

	// Done
	return 0;
}