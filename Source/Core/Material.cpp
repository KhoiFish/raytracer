#include "Material.h"
#include "OrthoNormalBasis.h"
#include <iostream>
#include <fstream>

// ----------------------------------------------------------------------------------------------------------------------------

MLambertian::MLambertian(BaseTexture* albedo) : Albedo(albedo)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool MLambertian::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    Vec3 target = hitRec.P + hitRec.Normal + RandomInUnitSphere();

    scatterRec.IsSpecular       = false;
    scatterRec.Attenuation      = Albedo->Value(hitRec.U, hitRec.V, hitRec.P);
    scatterRec.Pdf              = new CosinePdf(hitRec.Normal);
    scatterRec.ScatteredClassic = Ray(hitRec.P, target - hitRec.P, rayIn.Time());
   
    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

float MLambertian::ScatteringPdf(const Ray& rayIn, const HitRecord& rec, Ray& scattered) const
{
    float cosine = Dot(rec.Normal, UnitVector(scattered.Direction()));

    float ret;
    if (cosine < 0)
    {
        ret = cosine = 0;
    }
    else
    {
        // HACK. Sometimes pdfs come back nearly zero.
        // Clamp the value so we get some contribution from the pdf.
        // This gets rid of "rogue" pixels that are brightly colored.
        ret = cosine / RT_PI;
        ret = GetMax(ret, 0.05f);
    }

    return ret;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 MLambertian::AlbedoValue(float u, float v, const Vec3& p) const
{
    return Albedo->Value(u, v, p);
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MMetal::MMetal(BaseTexture* albedo, float fuzz) : Albedo(albedo)
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
    scatterRec.Attenuation = Albedo->Value(hitRec.U, hitRec.V, hitRec.P);
    scatterRec.IsSpecular  = true;
    scatterRec.Pdf         = nullptr;

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 MMetal::AlbedoValue(float u, float v, const Vec3& p) const
{
    return Albedo->Value(u, v, p);
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

Vec3 MDiffuseLight::AlbedoValue(float u, float v, const Vec3& p) const
{
    return EmitTex->Value(u, v, p);
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
    scatterRec.Pdf = nullptr;
    scatterRec.IsSpecular = true;

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 MIsotropic::AlbedoValue(float u, float v, const Vec3& p) const
{
    return Albedo->Value(u, v, p);
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MWavefrontObj::MWavefrontObj(const char* materialFilePath, bool makeMetal, float fuzz)
    : MLambertian(new ConstantTexture(Vec3(0, 0, 0)))
    , MakeMetal(makeMetal)
    , Fuzz(fuzz)
    , DiffuseMap(nullptr)
{
    std::ifstream inputFile(materialFilePath);
    if (inputFile.is_open())
    {
        std::string strLine;
        while (!inputFile.eof())
        {
            std::getline(inputFile, strLine);

            if ((strLine.find("map_Kd")) == 0)
            {
                std::string textureFilename = strLine.substr(strlen("map_Kd") + 1);
                DiffuseMap = new ImageTexture(textureFilename.c_str());
            }
        }

        if (DiffuseMap == nullptr)
        {
            DiffuseMap = new ImageTexture("white.png");
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool MWavefrontObj::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    if (MakeMetal)
    {
        Vec3 reflected = Reflect(UnitVector(rayIn.Direction()), hitRec.Normal);
        scatterRec.SpecularRay = Ray(hitRec.P, reflected + Fuzz * RandomInUnitSphere());
        scatterRec.IsSpecular  = true;
        scatterRec.Pdf         = nullptr;
    }
    else
    {
        MLambertian::Scatter(rayIn, hitRec, scatterRec);
    }
    
    scatterRec.Attenuation = DiffuseMap->Value(hitRec.U, hitRec.V, hitRec.P);

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec3 MWavefrontObj::AlbedoValue(float u, float v, const Vec3& p) const
{
    return DiffuseMap->Value(u, v, p);
}
