#pragma once

#include <iostream>
#include <fstream>
#include <cfloat>
#include "ray.h"
#include "vec3.h"
#include "hitable.h"
#include "hitablelist.h"
#include "material.h"
#include "camera.h"
#include "util.h"

class raytracer
{
public:

	raytracer(int width, int height, int numSamples, int maxDepth) 
		: OutputWidth(width)
		, OutputHeight(height)
		, NumRaySamples(numSamples)
		, MaxDepth(maxDepth)
	{
		OutputBuffer = new vec3[OutputWidth * OutputHeight];
	}

	void render(camera cam, hitable* world)
	{
		// Trace each pixel
		int outputOffset = 0;
		for (int j = OutputHeight - 1; j >= 0; j--)
		{
			for (int i = 0; i < OutputWidth; i++)
			{
				// Send random rays to pixel, i.e. multi-sample
				vec3 col(0.f, 0.f, 0.f);
				for (int s = 0; s < NumRaySamples; s++)
				{
					float u = float(i + drand48()) / float(OutputWidth);
					float v = float(j + drand48()) / float(OutputHeight);
					ray r = cam.get_ray(u, v);
					col += trace(r, world, 0);
				}
				col /= float(NumRaySamples);
				
				// Gamma correct
				col = vec3(sqrt(col.r()), sqrt(col.g()), sqrt(col.b()));

				// Write color to output buffer
				OutputBuffer[outputOffset++] = col;
			}
		}
	}

	void writeOutputToPPMFile(std::ofstream outFile)
	{
		outFile << "P3\n" << OutputWidth << " " << OutputHeight << "\n255\n";

		int numPixels = OutputWidth * OutputHeight;
		for (int i = 0; i < numPixels; i++)
		{
			// Get the pixel
			vec3 col = OutputBuffer[i];

			// Write pixel to file
			int ir = int(255.99*col.r());
			int ig = int(255.99*col.g());
			int ib = int(255.99*col.b());
			outFile << ir << " " << ig << " " << ib << "\n";
		}	
	}

private:

	inline vec3 trace(const ray& r, hitable *world, int depth)
	{
		hit_record rec;
		if (world->hit(r, 0.001f, FLT_MAX, rec))
		{
			ray scattered;
			vec3 attenuation;
			if (depth < MaxDepth && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
			{
				return attenuation * trace(scattered, world, depth + 1);
			}
			else
			{
				return vec3(0, 0, 0);
			}
		}
		else
		{
			vec3 unitDirection = unit_vector(r.direction());
			float t = 0.5f * (unitDirection.y() + 1.0f);
			return ((1.0f - t) * vec3(1.0f, 1.0f, 1.0f)) + (t * vec3(0.5f, 0.7f, 1.0f));
		}
	}

private:

	// Output options
	int            OutputWidth;
	int            OutputHeight;
	vec3*          OutputBuffer;

	// Tracing options
	int            NumRaySamples;
	int            MaxDepth;
};