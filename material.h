#pragma once
#include "Ray.h"
#include "Hitable.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Material
{
public:
	virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const = 0;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MLambertian : public Material
{
public:

	MLambertian(const Vec3& albedo) : Albedo(albedo) {}

	virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const
	{
		Vec3 target = rec.P + rec.Normal + RandomInUnitSphere();
		scattered = Ray(rec.P, target - rec.P);
		attenuation = Albedo;
		return true;
	}

private:

	Vec3 Albedo;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MMetal : public Material
{
public:

	MMetal(const Vec3& albedo, float fuzz) : Albedo(albedo)
	{
		if (fuzz < 1)
		{
			Fuzz = fuzz;
		}
		else
		{
			Fuzz = 1;
		}
	}

	virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const
	{
		Vec3 reflected = Reflect(UnitVector(rayIn.Direction()), rec.Normal);
		scattered = Ray(rec.P, reflected + Fuzz*RandomInUnitSphere());
		attenuation = Albedo;
		return (Dot(scattered.Direction(), rec.Normal) > 0);
	}

private:

	Vec3 Albedo;
	float Fuzz;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MDielectric : public Material
{
public:

	MDielectric(float ri) : RefId(ri) {}

	virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const
	{
		Vec3 outwardNormal;
		Vec3 reflected = Reflect(rayIn.Direction(), rec.Normal);
		attenuation = Vec3(1.0f, 1.0f, 1.0f);

		float niOverNt;
		Vec3 refracted;
		float reflectProb;
		float cosine;
		if (Dot(rayIn.Direction(), rec.Normal) > 0)
		{
			outwardNormal = -rec.Normal;
			niOverNt = RefId;
			cosine = RefId * Dot(rayIn.Direction(), rec.Normal) / rayIn.Direction().Length();
		}
		else
		{
			outwardNormal = rec.Normal;
			niOverNt = 1.0f / RefId;
			cosine = -Dot(rayIn.Direction(), rec.Normal) / rayIn.Direction().Length();
		}

		if (Refract(rayIn.Direction(), outwardNormal, niOverNt, refracted))
		{
			reflectProb = Schlick(cosine, RefId);
		}
		else
		{
			scattered = Ray(rec.P, reflected);
			reflectProb = 1.0f;
		}

		if (RandomFloat() < reflectProb)
		{
			scattered = Ray(rec.P, reflected);
		}
		else
		{
			scattered = Ray(rec.P, refracted);
		}

		return true;
	}

private:
	
	float RefId;
};