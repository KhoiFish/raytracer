#include "Material.h"
#include "OrthoNormalBasis.h"

// ----------------------------------------------------------------------------------------------------------------------------

MLambertian::MLambertian(BaseTexture* albedo) : Albedo(albedo)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool MLambertian::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    scatterRec.IsSpecular  = false;
    scatterRec.Attenuation = Albedo->Value(hitRec.U, hitRec.V, hitRec.P);
    scatterRec.Pdf         = new CosinePdf(hitRec.Normal);
   
    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

float MLambertian::ScatteringPdf(const Ray& rayIn, const HitRecord& rec, Ray& scattered) const
{
    float cosine = Dot(rec.Normal, UnitVector(scattered.Direction()));
    if (cosine < 0)
    {
        cosine = 0;
    }

    return cosine / RT_PI;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 MLambertian::AlbedoValue(float u, float v, const Vec3& p) const
{
    return Albedo->Value(u, v, p);
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

bool MMetal::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    Vec3 reflected = Reflect(UnitVector(rayIn.Direction()), hitRec.Normal);
    scatterRec.SpecularRay = Ray(hitRec.P, reflected + Fuzz * RandomInUnitSphere());
    scatterRec.Attenuation = Albedo;
    scatterRec.IsSpecular  = true;
    scatterRec.Pdf         = nullptr;

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 MMetal::AlbedoValue(float u, float v, const Vec3& p) const
{
    return Albedo;
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MDielectric::MDielectric(float ri) : RefId(ri)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

bool MDielectric::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    Vec3 outwardNormal;
    Vec3 reflected = Reflect(rayIn.Direction(), hitRec.Normal);

    scatterRec.IsSpecular  = true;
    scatterRec.Attenuation = Vec3(1.0f, 1.0f, 1.0f);
    scatterRec.Pdf         = nullptr;

    float niOverNt;
    Vec3 refracted;
    float reflectProb;
    float cosine;
    if (Dot(rayIn.Direction(), hitRec.Normal) > 0)
    {
        outwardNormal = -hitRec.Normal;
        niOverNt = RefId;
        cosine = RefId * Dot(rayIn.Direction(), hitRec.Normal) / rayIn.Direction().Length();
    }
    else
    {
        outwardNormal = hitRec.Normal;
        niOverNt = 1.0f / RefId;
        cosine = -Dot(rayIn.Direction(), hitRec.Normal) / rayIn.Direction().Length();
    }

    if (Refract(rayIn.Direction(), outwardNormal, niOverNt, refracted))
    {
        reflectProb = Schlick(cosine, RefId);
    }
    else
    {
        scatterRec.SpecularRay = Ray(hitRec.P, reflected, rayIn.Time());
        reflectProb = 1.0f;
    }

    if (RandomFloat() < reflectProb)
    {
        scatterRec.SpecularRay = Ray(hitRec.P, reflected, rayIn.Time());
    }
    else
    {
        scatterRec.SpecularRay = Ray(hitRec.P, refracted, rayIn.Time());
    }

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MDiffuseLight::MDiffuseLight(BaseTexture* tex) : EmitTex(tex)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 MDiffuseLight::Emitted(const Ray& rayIn, const HitRecord& rec, float u, float v, Vec3& p) const
{
    if (Dot(rec.Normal, rayIn.Direction()) < 0.f)
    {
        return EmitTex->Value(u, v, p);
    }
    else
    {
        return Vec3(0, 0, 0);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MIsotropic::MIsotropic(BaseTexture* albedo) : Albedo(albedo)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

bool MIsotropic::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    scatterRec.SpecularRay = Ray(hitRec.P, RandomInUnitSphere());
    scatterRec.Attenuation = Albedo->Value(hitRec.U, hitRec.V, hitRec.P);
    scatterRec.Pdf         = nullptr;
    scatterRec.IsSpecular  = true;

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 MIsotropic::AlbedoValue(float u, float v, const Vec3& p) const
{
    return Albedo->Value(u, v, p);
}
