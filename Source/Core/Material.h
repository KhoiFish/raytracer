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
    
    virtual Vec3 Emitted(float u, float v, Vec3& p) const;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MLambertian : public Material
{
public:

    MLambertian(Texture* albedo);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const;

private:

    Texture* Albedo;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MMetal : public Material
{
public:

    MMetal(const Vec3& albedo, float fuzz);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const;

private:

    Vec3 Albedo;
    float Fuzz;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MDielectric : public Material
{
public:

    MDielectric(float ri);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const;

private:
    
    float RefId;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MDiffuseLight : public Material
{
public:

    MDiffuseLight(Texture* tex);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const;

    virtual Vec3 Emitted(float u, float v, Vec3& p) const;

private:

    Texture* EmitTex;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MIsotropic : public Material
{
public:

    MIsotropic(Texture* albedo);

    virtual bool Scatter(const Ray& rayIn, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const;

private:

    Texture* Albedo;
};
