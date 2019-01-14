#pragma once
#include "Ray.h"
#include "IHitable.h"
#include "Util.h"
#include "Texture.h"

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

	MLambertian(Texture* albedo) : Albedo(albedo) {}

	virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const
	{
		Vec3 target = rec.P + rec.Normal + RandomInUnitSphere();
		scattered = Ray(rec.P, target - rec.P, rayIn.Time());
		attenuation = Albedo->Value(0, 0, rec.P);
		return true;
	}

private:

	Texture* Albedo;
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
		scattered = Ray(rec.P, reflected + Fuzz*RandomInUnitSphere(), rayIn.Time());
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
			scattered = Ray(rec.P, reflected, rayIn.Time());
			reflectProb = 1.0f;
		}

		if (RandomFloat() < reflectProb)
		{
			scattered = Ray(rec.P, reflected, rayIn.Time());
		}
		else
		{
			scattered = Ray(rec.P, refracted, rayIn.Time());
		}

		return true;
	}

private:
	
	float RefId;
};