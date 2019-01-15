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
#include "Core/Texture.h"
#include "Core/XYZRect.h"
#include "Core/FlipNormals.h"
#include "Core/HitableBox.h"
#include "Core/HitableTransform.h"
#include "Core/ConstantMedium.h"

#define STB_IMAGE_IMPLEMENTATION
#include "StbImage/stb_image.h"

// ----------------------------------------------------------------------------------------------------------------------------

static IHitable* randomScene(float time0, float time1)
{
	const int numObjects = 500;
	IHitable **list = new IHitable*[numObjects + 1];

	Texture* checker = new CheckerTexture(
		new ConstantTexture(Vec3(.2f, .3f, .1f)),
		new ConstantTexture(Vec3(.9f, .9f, .9f))
	);
	list[0] = new Sphere(Vec3(0, -1000, 0), 1000, new MLambertian(checker));

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
						new MLambertian(new ConstantTexture(Vec3(RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat(), RandomFloat()*RandomFloat()))
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
	list[i++] = new Sphere(Vec3(-4, 1, 0), 1.f, new MLambertian(new ConstantTexture(Vec3(0.4f, 0.2f, 0.1f))));
	list[i++] = new Sphere(Vec3(4, 1, 0), 1.f, new MMetal(Vec3(0.7f, 0.6f, 0.5f), 0.f));

	// Generate BVH tree
	BVHNode* bvhHead = new BVHNode(list, i, time0, time1);

	return bvhHead;
}

// ----------------------------------------------------------------------------------------------------------------------------

static IHitable* createTwoPerlinSpheres()
{
	Texture* perTex = new NoiseTexture(4.f);
	Texture* imageTex = nullptr;
	{
		int width, height, numChannels;
		unsigned char* texData = stbi_load("guitar.jpg", &width, &height, &numChannels, 0);
		imageTex = new ImageTexture(texData, width, height);
	}

	IHitable **list = new IHitable*[2];
	list[0] = new Sphere(Vec3(0, -1000, 0), 1000, new MLambertian(perTex));
	list[1] = new Sphere(Vec3(0, 2, 0), 2, new MLambertian(imageTex));

	return new HitableList(list, 2);
}

// ----------------------------------------------------------------------------------------------------------------------------

static IHitable* SimpleLightScene()
{
	Texture* perlinTex = new NoiseTexture(4);
	IHitable** list = new IHitable*[4];
	list[0] = new Sphere(Vec3(0, -1000, 0), 1000, new MLambertian(perlinTex));
	list[1] = new Sphere(Vec3(0, 2, 0), 2, new MLambertian(perlinTex));
	list[2] = new Sphere(Vec3(0, 7, 0), 2, new MDiffuseLight(new ConstantTexture(Vec3(4, 4, 4))));
	list[3] = new XYZRect(XYZRect::AxisPlane::XY, 3, 5, 1, 3, -2, new MDiffuseLight(new ConstantTexture(Vec3(4, 4, 4))));

	return new HitableList(list, 4);
}

// ----------------------------------------------------------------------------------------------------------------------------

static IHitable* CornellBox(bool smoke)
{
	IHitable** list = new IHitable*[8];
	int i = 0;

	Material* red = new MLambertian(new ConstantTexture(Vec3(.65f, .05f, .05f)));
	Material* white = new MLambertian(new ConstantTexture(Vec3(.73f, .73f, .73f)));
	Material* green = new MLambertian(new ConstantTexture(Vec3(.12f, .45f, .15f)));
	Material* light = new MDiffuseLight(new ConstantTexture(Vec3(15, 15, 15)));

	list[i++] = new FlipNormals(new XYZRect(XYZRect::YZ, 0, 555, 0, 555, 555, green));
	list[i++] = new XYZRect(XYZRect::YZ, 0, 555, 0, 555, 0, red);

	// The commented line creates a light that causes the output to be black.
	// This might be due to values passing 1.0, we should investigate
	//list[i++] = new XYZRect(XYZRect::XZ, 113, 443, 127, 432, 554, light);
	list[i++] = new XYZRect(XYZRect::XZ, 213, 343, 227, 332, 554, light);

	list[i++] = new FlipNormals(new XYZRect(XYZRect::XZ, 0, 555, 0, 555, 555, white));
	list[i++] = new XYZRect(XYZRect::XZ, 0, 555, 0, 555, 0, white);
	list[i++] = new FlipNormals(new XYZRect(XYZRect::XY, 0, 555, 0, 555, 555, white));

	IHitable* box1 = new HitableTranslate(
		new HitableRotateY(
			new HitableBox(Vec3(0, 0, 0), Vec3(165, 165, 165), white),
			-18),
		Vec3(130, 0, 65));

	IHitable* box2 = new HitableTranslate(
		new HitableRotateY(
			new HitableBox(Vec3(0, 0, 0), Vec3(165, 330, 165), white),
			15),
		Vec3(265, 0, 295));

	if (smoke)
	{
		list[i++] = new ConstantMedium(box1, 0.01f, new ConstantTexture(Vec3(1.f, 1.f, 1.f)));
		list[i++] = new ConstantMedium(box2, 0.01f, new ConstantTexture(Vec3(0.f, 0.f, 0.f)));
	}
	else
	{
		list[i++] = box1;
		list[i++] = box2;
	}

	return new HitableList(list, i);
}

// ----------------------------------------------------------------------------------------------------------------------------

int main()
{
	// Raytracer params
	const int    outputWidth  = 1000;
	const int    outputHeight = 512;
	const int    numSamples   = 200;
	const int    maxDepth     = 50;
	const int    numThreads   = 8;

	// Create ray tracer
	Raytracer tracer(outputWidth, outputHeight, numSamples, maxDepth, numThreads);

	// Random scene
	if (false)
	{
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
		Camera cam(
			lookFrom, lookAt, upVec,
			vertFov, aspect, aperture, distToFocus,
			shutterTime0, shutterTime1);

		// Render and write out image
		tracer.Render(cam, randomScene(shutterTime0, shutterTime1));
		tracer.WriteOutputToPPMFile(std::ofstream("randomworld.ppm"));
	}

	// Cornell box
	{
		// Camera options
		const Vec3   lookFrom = Vec3(278, 278, -800);
		const Vec3   lookAt = Vec3(278, 278, 0);
		const Vec3   upVec = Vec3(0, 1, 0);
		const float  vertFov = 40.f;
		const float  aspect = float(outputWidth) / float(outputHeight);
		const float  aperture = 0.0f;
		const float  distToFocus = 10.f;
		const float  shutterTime0 = 0.f;
		const float  shutterTime1 = 1.f;
		Camera cam(
			lookFrom, lookAt, upVec,
			vertFov, aspect, aperture, distToFocus,
			shutterTime0, shutterTime1);

		// Render and write out image
		tracer.Render(cam, CornellBox(false));
		tracer.WriteOutputToPPMFile(std::ofstream("cornell.ppm"));

		tracer.Render(cam, CornellBox(true));
		tracer.WriteOutputToPPMFile(std::ofstream("cornell_smoke.ppm"));
	}

	// Done
	return 0;
}