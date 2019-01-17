#pragma once

#include <iostream>
#include <fstream>
#include <cfloat>
#include <thread>
#include <atomic>
#include <iomanip>
#include "Ray.h"
#include "Vec3.h"
#include "IHitable.h"
#include "HitableList.h"
#include "Material.h"
#include "Camera.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Raytracer
{
public:

	typedef std::thread* ThreadPtr;

	Raytracer(int width, int height, int numSamples, int maxDepth, int numThreads) 
		: OutputWidth(width)
		, OutputHeight(height)
		, NumRaySamples(numSamples)
		, MaxDepth(maxDepth)
		, NumThreads(numThreads)
	{
		OutputBuffer = new Vec3[OutputWidth * OutputHeight];
		ThreadPtrs = new ThreadPtr[NumThreads];
		DefaultAmbient = Vec3(0, 0, 0);
	}

	~Raytracer()
	{
		delete[] OutputBuffer;
		OutputBuffer = nullptr;

		delete[] ThreadPtrs;
		ThreadPtrs = nullptr;
	}

	void Render(const Camera& cam, IHitable* world)
	{
		// Trace each pixel
		CurrentOutputOffset = 0;

		// Create the threads and run them
		printf("Rendering frame...\n");
		for (int i = 0; i < NumThreads; i++)
		{
			ThreadPtrs[i] = new std::thread(threadTraceNextPixel, i, this, cam, world);
		}

		// Join all the threads
		for (int i = 0; i < NumThreads; i++)
		{
			ThreadPtrs[i]->join();
			delete ThreadPtrs[i];
			ThreadPtrs[i] = nullptr;
		}
		printf("\nRendering done!\n\n");
	}

	void WriteOutputToPPMFile(std::ofstream outFile)
	{
		printf("Writing ppm file...\n");

		outFile << "P3\n" << OutputWidth << " " << OutputHeight << "\n255\n";

		int numPixels = OutputWidth * OutputHeight;
		for (int i = 0; i < numPixels; i++)
		{
			// Get the pixel
			Vec3 col = OutputBuffer[i];

			// Write pixel to file
			int ir = int(255.99*col.R());
			int ig = int(255.99*col.G());
			int ib = int(255.99*col.B());

			ir = (ir > 255) ? 255 : ir;
			ig = (ig > 255) ? 255 : ig;
			ib = (ib > 255) ? 255 : ib;

			outFile << ir << " " << ig << " " << ib << "\n";

			// Print progress
			if ((i % OutputWidth) == 0)
			{
				printProgress(float(i) / float(numPixels));
			}
		}

		printf("\nFinished writing ppm file!\n");
	}

	void SetDefaultAmbient(const Vec3& ambient)
	{
		DefaultAmbient = ambient;
	}

private:

	static void threadTraceNextPixel(int id, Raytracer* tracer, const Camera& cam, IHitable* world)
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
				const int x = offset % tracer->OutputWidth;
				const int y = tracer->OutputHeight - (offset / tracer->OutputWidth);

				// Send random rays to pixel, i.e. multi-sample
				Vec3 col(0.f, 0.f, 0.f);
				for (int s = 0; s < tracer->NumRaySamples; s++)
				{
					const float u = float(x + RandomFloat()) / float(tracer->OutputWidth);
					const float v = float(y + RandomFloat()) / float(tracer->OutputHeight);

					Ray r = cam.GetRay(u, v);
					col += tracer->trace(r, world, 0);
				}
				col /= float(tracer->NumRaySamples);

				// Gamma correct
				col = Vec3(sqrt(col.R()), sqrt(col.G()), sqrt(col.B()));

				// Write color to output buffer
				tracer->OutputBuffer[offset] = col;

				// Print progress
				int latestOffset = tracer->CurrentOutputOffset.load();
				if (id == 0 && offset > 0 && (latestOffset % tracer->OutputWidth) == 0)
				{
					float percentDone = (float(latestOffset) / float(numPixels));
					printProgress(percentDone);
				}
			}
		}
	}

	static inline void printProgress(double percentage)
	{
		static const char* PBSTR = "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||";
		static const int PBWIDTH = 60;
		int val = (int)(percentage * 100);
		int lpad = (int)(percentage * PBWIDTH);
		int rpad = PBWIDTH - lpad;
		printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
		fflush(stdout);
	}

	inline Vec3 trace(const Ray& r, IHitable *world, int depth)
	{
		HitRecord rec;
		if (world->Hit(r, 0.001f, FLT_MAX, rec))
		{
			Ray  scattered;
			Vec3 attenuation;
			Vec3 emitted = rec.MatPtr->Emitted(rec.U, rec.V, rec.P);
			if (depth < MaxDepth && rec.MatPtr->Scatter(r, rec, attenuation, scattered))
			{
				return emitted + attenuation * trace(scattered, world, depth + 1);
			}
			else
			{
				return emitted;
			}
		}
		else
		{
			return DefaultAmbient;
		}
	}

private:

	// Output options
	int               OutputWidth;
	int               OutputHeight;
	Vec3*             OutputBuffer;

	// Tracing options
	int               NumRaySamples;
	int               MaxDepth;
	int               NumThreads;
	Vec3              DefaultAmbient;

	// Thread tracking
	std::atomic<int>  CurrentOutputOffset;
	std::thread**     ThreadPtrs;
};