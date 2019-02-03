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
        ScatterRecord() : IsSpecular(false), Attenuation(0, 0, 0), PdfPtr(nullptr) {}
        ~ScatterRecord()
        {
            // I don't like the asymmetrical allocation/deallocation strategy for pdfs.
            // But we'll do this for now.
            if (PdfPtr)
            {
                delete PdfPtr;
                PdfPtr = nullptr;
            }
        }

        Ray   SpecularRay;
        bool  IsSpecular;
        Vec3  Attenuation;
        Ray   ScatteredClassic;
        Pdf*  PdfPtr;
    };

    Material() : Owner(nullptr), Albedo(nullptr), EmitTex(nullptr) {}
    Material(BaseTexture* albedo, BaseTexture* emitTex) : Owner(nullptr), Albedo(albedo), EmitTex(emitTex) {}

    virtual ~Material()
    {
        if (Albedo != nullptr)
        {
            delete Albedo;
            Albedo = nullptr;
        }

        if (EmitTex != nullptr)
        {
            delete EmitTex;
            EmitTex = nullptr;
        }
    }

    virtual bool  Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const { return false; }
    virtual float ScatteringPdf(const Ray& rayIn, const HitRecord& rec, Ray& scattered) const { return 1.f; }
    virtual Vec3  Emitted(const Ray& rayIn, const HitRecord& rec, float u, float v, Vec3& p) const { return Vec3(0, 0, 0); }
    virtual Vec3  AlbedoValue(float u, float v, const Vec3& p) const { return Vec3(0, 0, 0); }

public:

    IHitable*   Owner;

protected:

    BaseTexture* Albedo;
    BaseTexture* EmitTex;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MLambertian : public Material
{
public:

    MLambertian(BaseTexture* albedo);

    virtual bool  Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
    virtual float ScatteringPdf(const Ray& rayIn, const HitRecord& rec, Ray& scattered) const;
    virtual Vec3  AlbedoValue(float u, float v, const Vec3& p) const;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MMetal : public Material
{
public:

    MMetal(BaseTexture* albedo, float fuzz);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
    virtual Vec3 AlbedoValue(float u, float v, const Vec3& p) const;

private:

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
    virtual Vec3 AlbedoValue(float u, float v, const Vec3& p) const;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MIsotropic : public Material
{
public:

    MIsotropic(BaseTexture* albedo);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
    virtual Vec3 AlbedoValue(float u, float v, const Vec3& p) const;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MWavefrontObj : public MLambertian
{
public:

    MWavefrontObj(const char* materialFilePath, bool makeMetal = false, float fuzz = 0.5f);
    virtual ~MWavefrontObj();

    virtual bool  Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
    virtual Vec3  AlbedoValue(float u, float v, const Vec3& p) const;

    const ImageTexture* GetDiffuseMap() const { return DiffuseMap; }

private:

    bool          MakeMetal;
    float         Fuzz;
    ImageTexture* DiffuseMap;
};
