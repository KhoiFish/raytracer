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

// ----------------------------------------------------------------------------------------------------------------------------

// Output options
static const char*  gOutputFilename = "test.ppm";
static int          gOutputWidth = 1200;
static int          gOutputHeight = 800;

// Tracing options
static int          gNumRaySamples = 10;
static int          gMaxDepth = 50;

// Camera options
static vec3         gLookFrom(13, 2, 3);
static vec3         gLookAt(0, 0, 0);
static vec3         gUpVec(0, 1, 0);
static float        gVerticalFOV = 20.f;
static float        gAperture = .1f;
static float        gDistToFocus = 10.f; //(gLookFrom - gLookAt).length();

// ----------------------------------------------------------------------------------------------------------------------------

inline vec3 color(const ray& r, hitable *world, int depth)
{
	hit_record rec;
	if (world->hit(r, 0.001f, FLT_MAX, rec))
	{
		ray scattered;
		vec3 attenuation;
		if (depth < gMaxDepth && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
		{
			return attenuation * color(scattered, world, depth + 1);
		}
		else
		{
			return vec3(0, 0, 0);
		}
	}
	else
	{
		vec3 unit_direction = unit_vector(r.direction());
		float t = 0.5f * (unit_direction.y() + 1.0f);
		return ((1.0f - t) * vec3(1.0f, 1.0f, 1.0f)) + (t * vec3(0.5f, 0.7f, 1.0f));
	}
}

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
	// Open file for writing
	std::ofstream outFile(gOutputFilename);
	outFile << "P3\n" << gOutputWidth << " " << gOutputHeight << "\n255\n";

	// Setup world and camera
	hitable* world = random_scene();
	camera cam(gLookFrom, gLookAt, gUpVec, gVerticalFOV, float(gOutputWidth)/float(gOutputHeight), gAperture, gDistToFocus);

	// Trace each pixel
	for (int j = gOutputHeight - 1; j >= 0; j--)
	{
		for (int i = 0; i < gOutputWidth; i++)
		{
			vec3 col(0.f, 0.f, 0.f);
			for (int s = 0; s < gNumRaySamples; s++)
			{
				float u = float(i + drand48()) / float(gOutputWidth);
				float v = float(j + drand48()) / float(gOutputHeight);
				ray r = cam.get_ray(u, v);
				col += color(r, world, 0);
			}
			col /= float(gNumRaySamples);
			col = vec3(sqrt(col.r()), sqrt(col.g()), sqrt(col.b()));

			// Write pixel to file
			int ir = int(255.99*col.r());
			int ig = int(255.99*col.g());
			int ib = int(255.99*col.b());
			outFile << ir << " " << ig << " " << ib << "\n";
		}
	}

	// Done
	return 0;
}