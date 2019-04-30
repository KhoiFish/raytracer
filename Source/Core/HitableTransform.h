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

#include "IHitable.h"
#include "Ray.h"
#include "AABB.h"
#include "Util.h"

namespace Core
{
    // ----------------------------------------------------------------------------------------------------------------------------

    class HitableTranslate : public IHitable
    {
    public:

        inline HitableTranslate(IHitable* p, const Vec4& displacement) : HitObject(p), Offset(displacement) {}
        virtual ~HitableTranslate();

        virtual bool           Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
        virtual bool           BoundingBox(float t0, float t1, AABB& box) const;
        inline const IHitable* GetHitObject() const { return HitObject; }
        inline const Vec4&     GetOffset() const { return Offset; }

    private:

        IHitable* HitObject;
        Vec4 Offset;
    };

    // ----------------------------------------------------------------------------------------------------------------------------

    class HitableRotateY : public IHitable
    {
    public:

        HitableRotateY(IHitable* obj, float angleDeg);
        virtual ~HitableRotateY();

        virtual bool           Hit(const Ray& r, float tMin, float tMax, HitRecord& rec) const;
        virtual bool           BoundingBox(float t0, float t1, AABB& box) const;
        inline float           GetAngleDegrees() const { return AngleDegrees; }
        inline const IHitable* GetHitObject() const { return HitObject; }

    private:

        IHitable* HitObject;
        float     AngleDegrees;
        float     SinTheta;
        float     CosTheta;
        bool      HasBox;
        AABB      Bbox;
    };

}