#pragma once

#include <iostream>
#include <fstream>
#include <cfloat>
#include <thread>
#include <atomic>
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

	typedef std::thread* ThreadPtr;

	raytracer(int width, int height, int numSamples, int maxDepth, int numThreads) 
		: OutputWidth(width)
		, OutputHeight(height)
		, NumRaySamples(numSamples)
		, MaxDepth(maxDepth)
		, NumThreads(numThreads)
	{
		OutputBuffer = new vec3[OutputWidth * OutputHeight];
		ThreadPtrs = new ThreadPtr[NumThreads];
	}

	~raytracer()
	{
		delete[] OutputBuffer;
		OutputBuffer = nullptr;

		delete[] ThreadPtrs;
		ThreadPtrs = nullptr;
	}

	void render(camera cam, hitable* world)
	{
		// Trace each pixel
		CurrentOutputOffset = 0;

		// Create the threads and run them
		for (int i = 0; i < NumThreads; i++)
		{
			ThreadPtrs[i] = new std::thread(threadTraceNextPixel, this, cam, world);
		}

		// Join all the threads
		for (int i = 0; i < NumThreads; i++)
		{
			ThreadPtrs[i]->join();
			delete ThreadPtrs[i];
			ThreadPtrs[i] = nullptr;
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

	static void threadTraceNextPixel(raytracer* tracer, camera cam, hitable* world)
	{
		const int numPixels = (tracer->OutputWidth * tracer->OutputHeight);
		int offset = tracer->CurrentOutputOffset.load();
		while (offset < numPixels)
		{
			// Find the next offset to the pixel to trace
			while (offset < numPixels)
			{
				if (tracer->CurrentOutputOffset.compare_exchange_strong(offset, offset + 1))
				{
					break;
				}
				offset = tracer->CurrentOutputOffset.load();
			}

			// Do we have an pixel to trace?
			if (offset < numPixels)
			{
				// Get the offsets
				int x = offset % tracer->OutputWidth;
				int y = tracer->OutputHeight - (offset / tracer->OutputWidth);

				// Send random rays to pixel, i.e. multi-sample
				vec3 col(0.f, 0.f, 0.f);
				for (int s = 0; s < tracer->NumRaySamples; s++)
				{
					float u = float(x + drand48()) / float(tracer->OutputWidth);
					float v = float(y + drand48()) / float(tracer->OutputHeight);
					ray r = cam.get_ray(u, v);
					col += tracer->trace(r, world, 0);
				}
				col /= float(tracer->NumRaySamples);

				// Gamma correct
				col = vec3(sqrt(col.r()), sqrt(col.g()), sqrt(col.b()));

				// Write color to output buffer
				tracer->OutputBuffer[offset] = col;
			}
		}
	}

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
	int               OutputWidth;
	int               OutputHeight;
	vec3*             OutputBuffer;

	// Tracing options
	int               NumRaySamples;
	int               MaxDepth;
	int               NumThreads;

	// Thread tracking
	std::atomic<int>  CurrentOutputOffset;
	std::thread**     ThreadPtrs;
};