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

#pragma once
#include "Ray.h"
#include "IHitable.h"
#include "Util.h"
#include "CoreTexture.h"
#include "Pdf.h"
#include <vector>

namespace Core
{
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
            Vec4  Attenuation;
            Ray   ScatteredClassic;
            Pdf* PdfPtr;
        };

        Material() : Owner(nullptr), AlbedoTexture(nullptr), EmitTex(nullptr) {}
        Material(BaseTexture* albedo, BaseTexture* emitTex) : Owner(nullptr), AlbedoTexture(albedo), EmitTex(emitTex) {}

        virtual ~Material()
        {
            if (AlbedoTexture != nullptr)
            {
                delete AlbedoTexture;
                AlbedoTexture = nullptr;
            }

            if (EmitTex != nullptr)
            {
                delete EmitTex;
                EmitTex = nullptr;
            }
        }

        virtual bool            Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const { return false; }
        virtual float           ScatteringPdf(const Ray& rayIn, const HitRecord& rec, Ray& scattered) const { return 1.f; }
        virtual Vec4            Emitted(const Ray& rayIn, const HitRecord& rec, float u, float v, Vec4& p) const { return Vec4(0, 0, 0); }
        virtual Vec4            AlbedoValue(float u, float v, const Vec4& p) const { return Vec4(0, 0, 0); }
        virtual Vec4            GetAverageAlbedo() const;
        virtual BaseTexture*    GetAlbedoTexture() { return AlbedoTexture; }
        virtual BaseTexture*    GetEmitTexture() { return EmitTex; }

    public:

        IHitable* Owner;

    protected:

        BaseTexture* AlbedoTexture;
        BaseTexture* EmitTex;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class MLambertian : public Material
    {
    public:

        MLambertian(BaseTexture* albedo);

        virtual bool  Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
        virtual float ScatteringPdf(const Ray& rayIn, const HitRecord& rec, Ray& scattered) const;
        virtual Vec4  AlbedoValue(float u, float v, const Vec4& p) const;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class MMetal : public Material
    {
    public:

        MMetal(BaseTexture* albedo, float fuzz);

        virtual bool Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
        virtual Vec4 AlbedoValue(float u, float v, const Vec4& p) const;
        float        GetFuzz() const { return Fuzz; }

    private:

        float Fuzz;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class MDielectric : public Material
    {
    public:

        MDielectric(float ri);

        virtual bool Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
        float        GetReflectiveIndex() const { return RefId; }

    private:

        float RefId;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class MDiffuseLight : public Material
    {
    public:

        MDiffuseLight(BaseTexture* tex);

        virtual Vec4 Emitted(const Ray& rayIn, const HitRecord& rec, float u, float v, Vec4& p) const;
        virtual Vec4 AlbedoValue(float u, float v, const Vec4& p) const;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class MIsotropic : public Material
    {
    public:

        MIsotropic(BaseTexture* albedo);

        virtual bool Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;
        virtual Vec4 AlbedoValue(float u, float v, const Vec4& p) const;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class MWavefrontObj : public MLambertian
    {
    public:

        MWavefrontObj(const char* materialFilePath, bool makeMetal = false, float fuzz = 0.5f);
        virtual ~MWavefrontObj();

        virtual bool Scatter(const Ray& rayIn, const HitRecord& hitRec, ScatterRecord& scatterRec) const;

    private:

        bool          MakeMetal;
        float         Fuzz;
    };

}