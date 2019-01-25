#pragma once
#include "Ray.h"
#include "IHitable.h"
#include "Util.h"
#include "Texture.h"
#include "Pdf.h"
#include <vector>

// ----------------------------------------------------------------------------------------------------------------------------

class Material
{
public:

    struct ScatterRecord
    {
        ScatterRecord() : IsSpecular(false), Attenuation(0, 0, 0), Pdf(nullptr) {}
        ~ScatterRecord()
        {
            // I don't like the asymmetrical allocation/deallocation strategy for pdfs.
            // But we'll do this for now.
            if (Pdf)
            {
                delete Pdf;
                Pdf = nullptr;
            }
        }

        Ray   SpecularRay;
        bool  IsSpecular;
        Vec3  Attenuation;
        Pdf*  Pdf;
    };

    virtual bool  Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const { return false; }
    virtual float ScatteringPdf(const Ray& rayIn, const HitRecord& rec, Ray& scattered) const { return 1.f; }
    virtual Vec3  Emitted(const Ray& rayIn, const HitRecord& rec, float u, float v, Vec3& p) const { return Vec3(0, 0, 0); }
    virtual Vec3  AlbedoValue(float u, float v, const Vec3& p) const { return Vec3(0, 0, 0); }
};

// ----------------------------------------------------------------------------------------------------------------------------

class MLambertian : public Material
{
public:

    MLambertian(BaseTexture* albedo);

    virtual bool  Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
    virtual float ScatteringPdf(const Ray& rayIn, const HitRecord& rec, Ray& scattered) const;
    virtual Vec3  AlbedoValue(float u, float v, const Vec3& p) const;

private:

    BaseTexture* Albedo;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MMetal : public Material
{
public:

    MMetal(const Vec3& albedo, float fuzz);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
    virtual Vec3 AlbedoValue(float u, float v, const Vec3& p) const;

private:

    Vec3 Albedo;
    float Fuzz;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MDielectric : public Material
{
public:

    MDielectric(float ri);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;

private:
    
    float RefId;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MDiffuseLight : public Material
{
public:

    MDiffuseLight(BaseTexture* tex);

    virtual Vec3 Emitted(const Ray& rayIn, const HitRecord& rec, float u, float v, Vec3& p) const;

private:

    BaseTexture* EmitTex;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MIsotropic : public Material
{
public:

    MIsotropic(BaseTexture* albedo);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
    virtual Vec3 AlbedoValue(float u, float v, const Vec3& p) const;

private:

    BaseTexture* Albedo;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MWavefrontObj : public Material
{
public:

    MWavefrontObj(const char* materialFilePath);

    virtual bool  Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
    virtual float ScatteringPdf(const Ray& rayIn, const HitRecord& rec, Ray& scattered) const;
    virtual Vec3  AlbedoValue(float u, float v, const Vec3& p) const;

    const ImageTexture* GetDiffuseMap() const { return DiffuseMap; }

private:

    ImageTexture* DiffuseMap;
};
