#pragma once

#include "Ray.h"
#include "Vec3.h"
#include "IHitable.h"
#include "HitableList.h"
#include "Material.h"
#include "Camera.h"
#include "Util.h"
#include <thread>
#include <atomic>

// ----------------------------------------------------------------------------------------------------------------------------

class Raytracer
{
public:

	Raytracer(int width, int height, int numSamples, int maxDepth, int numThreads);
	~Raytracer();

	void Render(const Camera& cam, IHitable* world);
	void WriteOutputToPPMFile(std::ofstream outFile);
	void SetDefaultAmbient(const Vec3& ambient);

private:

	static void  threadTraceNextPixel(int id, Raytracer* tracer, const Camera& cam, IHitable* world);
	static void  printProgress(double percentage);
	Vec3         trace(const Ray& r, IHitable *world, int depth);

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
