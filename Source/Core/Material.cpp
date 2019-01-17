#include "Material.h"

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 Material::Emitted(float u, float v, Vec3& p) const
{
    return Vec3(0, 0, 0);
}

// ----------------------------------------------------------------------------------------------------------------------------

MLambertian::MLambertian(Texture* albedo) : Albedo(albedo)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

bool MLambertian::Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const
{
    Vec3 target = rec.P + rec.Normal + RandomInUnitSphere();
    scattered = Ray(rec.P, target - rec.P, rayIn.Time());
    attenuation = Albedo->Value(rec.U, rec.V, rec.P);
    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MMetal::MMetal(const Vec3& albedo, float fuzz) : Albedo(albedo)
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

// ----------------------------------------------------------------------------------------------------------------------------

bool MMetal::Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const
{
    Vec3 reflected = Reflect(UnitVector(rayIn.Direction()), rec.Normal);
    scattered = Ray(rec.P, reflected + Fuzz * RandomInUnitSphere(), rayIn.Time());
    attenuation = Albedo;
    return (Dot(scattered.Direction(), rec.Normal) > 0);
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MDielectric::MDielectric(float ri) : RefId(ri)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

bool MDielectric::Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const
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

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MDiffuseLight::MDiffuseLight(Texture* tex) : EmitTex(tex)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

bool MDiffuseLight::Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const
{
    return false;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 MDiffuseLight::Emitted(float u, float v, Vec3& p) const
{
    return EmitTex->Value(u, v, p);
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MIsotropic::MIsotropic(Texture* albedo) : Albedo(albedo)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

bool MIsotropic::Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const
{
    scattered = Ray(rec.P, RandomInUnitSphere());
    attenuation = Albedo->Value(rec.U, rec.V, rec.P);

    return true;
}
