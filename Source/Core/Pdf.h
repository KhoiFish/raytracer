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

#include "Vec4.h"
#include "OrthoNormalBasis.h"
#include "Util.h"

namespace Core
{

    // ----------------------------------------------------------------------------------------------------------------------------

    class Pdf
    {
    public:
        virtual ~Pdf() {}

        virtual float Value(const Vec4& direction) const = 0;
        virtual Vec4  Generate() const = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class CosinePdf : public Pdf
    {
    public:

        CosinePdf(const Vec4& w)
        {
            UVW.BuildFromW(w);
        }

        virtual float Value(const Vec4& direction) const
        {
            float cosine = Dot(UnitVector(direction), UVW.W());
            if (cosine > 0)
            {
                float ret = cosine / RT_PI;

                // HACK. Sometimes pdfs come back zero.
                // Clamp the value so we get some contribution from the pdf.
                // This gets rid of "rogue" pixels that are brightly colored.
                ret = GetMax(ret, 0.05f);

                return ret;
            }
            else
            {
                return 0.05f;
            }
        }

        virtual Vec4 Generate() const
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

        HitablePdf(IHitable* hitable, const Vec4& origin) : Hitable(hitable), Origin(origin) {}

        virtual float Value(const Vec4& direction) const
        {
            return Hitable->PdfValue(Origin, direction);
        }

        virtual Vec4 Generate() const
        {
            return Hitable->Random(Origin);
        }

    private:

        IHitable* Hitable;
        Vec4        Origin;
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

        virtual float Value(const Vec4& direction) const
        {
            return (0.5f * pdfs[0]->Value(direction)) + (0.5f * pdfs[1]->Value(direction));
        }

        virtual Vec4 Generate() const
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
}