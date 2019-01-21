#pragma once

#include "Vec3.h"
#include "OrthoNormalBasis.h"
#include "Util.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Pdf
{
public:

    virtual float Value(const Vec3& direction) const = 0;
    virtual Vec3  Generate() const = 0;
};

// ----------------------------------------------------------------------------------------------------------------------------

class CosinePdf : public Pdf
{
public:

    CosinePdf(const Vec3& w)
    {
        UVW.BuildFromW(w);
    }

    virtual float Value(const Vec3& direction) const
    {
        float cosine = Dot(UnitVector(direction), UVW.W());
        if (cosine > 0)
        {
            return (cosine / RT_PI);
        }
        else
        {
            return 0;
        }
    }

    virtual Vec3 Generate() const
    {
        return UVW.Local(RandomCosineDirection());
    }

private:

    OrthoNormalBasis UVW;
};

// ----------------------------------------------------------------------------------------------------------------------------

class HitablePdf : public Pdf
{
public:

    HitablePdf(IHitable* hitable, const Vec3& origin) : Hitable(hitable), Origin(origin) {}

    virtual float Value(const Vec3& direction) const
    {
        return Hitable->PdfValue(Origin, direction);
    }

    virtual Vec3 Generate() const
    {
        return Hitable->Random(Origin);
    }

private:

    IHitable*   Hitable;
    Vec3        Origin;
};

// ----------------------------------------------------------------------------------------------------------------------------

class MixturePdf : public Pdf
{
public:

    MixturePdf(Pdf* p0, Pdf* p1)
    {
        pdfs[0] = p0;
        pdfs[1] = p1;
    }

    virtual float Value(const Vec3& direction) const
    {
        return (0.5f * pdfs[0]->Value(direction)) + (0.5f * pdfs[1]->Value(direction));
    }

    virtual Vec3 Generate() const
    {
        if (RandomFloat() < 0.5f)
        {
            return pdfs[0]->Generate();
        }
        else
        {
            return pdfs[1]->Generate();
        }
    }

private:

    Pdf* pdfs[2];
};
