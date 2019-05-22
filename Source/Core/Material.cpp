// ----------------------------------------------------------------------------------------------------------------------------
// 
// Copyright 2019 Khoi Nguyen
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
//    The above copyright notice and this permission notice shall be included in all copies or substantial
//    portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// ----------------------------------------------------------------------------------------------------------------------------

#include "Material.h"
#include "OrthoNormalBasis.h"
#include <cstring>
#include <iostream>
#include <fstream>

using namespace Core;

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 Material::GetAverageAlbedo() const
{
    // Run through the entire texture map and get an average color
    const float step = 0.01f;
    Vec4        color(0, 0, 0, 9);
    Vec4        p(0, 0, 0, 0);
    float       u = 0.0f;
    int         count = 0;
    while (u <= 1.0f)
    {
        float v = 0.0f;
        while (v <= 1.0f)
        {
            color += AlbedoValue(u, v, p);
            v += step;
            count++;
        }

        u += step;
    }

    return (color / float(count));
}

// ----------------------------------------------------------------------------------------------------------------------------

MLambertian::MLambertian(BaseTexture* albedo) : Material(albedo, nullptr)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

bool MLambertian::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    Vec4 target = hitRec.P + hitRec.Normal + RandomInUnitSphere();

    scatterRec.IsSpecular       = false;
    scatterRec.Attenuation      = AlbedoTexture->Value(hitRec.U, hitRec.V, hitRec.P);
    scatterRec.PdfPtr           = new CosinePdf(hitRec.Normal);
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

Vec4 MLambertian::AlbedoValue(float u, float v, const Vec4& p) const
{
    return AlbedoTexture->Value(u, v, p);
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MMetal::MMetal(BaseTexture* albedo, float fuzz) : Material(albedo, nullptr)
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
    Vec4 reflected = Reflect(UnitVector(rayIn.Direction()), hitRec.Normal);
    scatterRec.SpecularRay = Ray(hitRec.P, reflected + Fuzz * RandomInUnitSphere());
    scatterRec.Attenuation = AlbedoTexture->Value(hitRec.U, hitRec.V, hitRec.P);
    scatterRec.IsSpecular  = true;
    scatterRec.PdfPtr      = nullptr;

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 MMetal::AlbedoValue(float u, float v, const Vec4& p) const
{
    return AlbedoTexture->Value(u, v, p);
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MDielectric::MDielectric(float ri) : RefId(ri)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

bool MDielectric::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    Vec4 outwardNormal;
    Vec4 reflected = Reflect(rayIn.Direction(), hitRec.Normal);

    scatterRec.IsSpecular  = true;
    scatterRec.Attenuation = Vec4(1.0f, 1.0f, 1.0f);
    scatterRec.PdfPtr      = nullptr;

    float niOverNt;
    Vec4 refracted;
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

MDiffuseLight::MDiffuseLight(BaseTexture* tex) : Material(nullptr, tex)
{
    ;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 MDiffuseLight::Emitted(const Ray& rayIn, const HitRecord& rec, float u, float v, Vec4& p) const
{
    if (Dot(rec.Normal, rayIn.Direction()) < 0.f)
    {
        return EmitTex->Value(u, v, p);
    }
    else
    {
        return Vec4(0, 0, 0);
    }
}

Vec4 MDiffuseLight::AlbedoValue(float u, float v, const Vec4& p) const
{
    return EmitTex->Value(u, v, p);
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MIsotropic::MIsotropic(BaseTexture* albedo)  : Material(albedo, nullptr)
{

}

// ----------------------------------------------------------------------------------------------------------------------------

bool MIsotropic::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    scatterRec.SpecularRay = Ray(hitRec.P, RandomInUnitSphere());
    scatterRec.Attenuation = AlbedoTexture->Value(hitRec.U, hitRec.V, hitRec.P);
    scatterRec.PdfPtr      = nullptr;
    scatterRec.IsSpecular  = true;

    return true;
}

// ----------------------------------------------------------------------------------------------------------------------------

Vec4 MIsotropic::AlbedoValue(float u, float v, const Vec4& p) const
{
    return AlbedoTexture->Value(u, v, p);
}

// ----------------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------------------

MWavefrontObj::MWavefrontObj(const char* materialFilePath, bool makeMetal, float fuzz)
    : MLambertian(new ConstantTexture(Vec4(0, 0, 0)))
    , MakeMetal(makeMetal)
    , Fuzz(fuzz)
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
                std::string texFilePath = GetAbsolutePath(GetParentDir(materialFilePath) + std::string("/") + textureFilename);
                AlbedoTexture = new ImageTexture(texFilePath.c_str());
            }
        }

        if (AlbedoTexture == nullptr)
        {
            DEBUG_PRINTF("No diffuse found, trying default\n");
            AlbedoTexture = new ImageTexture(GetAbsolutePath(RUNTIMEDATA_DIR "/white.png").c_str());
        }
    }
    else
    {
        DEBUG_PRINTF("Could not open material file %s!", materialFilePath);
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

MWavefrontObj::~MWavefrontObj()
{
    if (AlbedoTexture != nullptr)
    {
        delete AlbedoTexture;
        AlbedoTexture = nullptr;
    }
}

// ----------------------------------------------------------------------------------------------------------------------------

bool MWavefrontObj::Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const
{
    if (MakeMetal)
    {
        Vec4 reflected = Reflect(UnitVector(rayIn.Direction()), hitRec.Normal);
        scatterRec.SpecularRay = Ray(hitRec.P, reflected + Fuzz * RandomInUnitSphere());
        scatterRec.IsSpecular  = true;
        scatterRec.PdfPtr      = nullptr;
    }
    else
    {
        MLambertian::Scatter(rayIn, hitRec, scatterRec);
    }
    
    scatterRec.Attenuation = AlbedoTexture->Value(hitRec.U, hitRec.V, hitRec.P);

    return true;
}
