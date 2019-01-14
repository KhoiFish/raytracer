#pragma once

#include "Vec3.h"
#include "Perlin.h"

// ----------------------------------------------------------------------------------------------------------------------------

class Texture
{
public:

	virtual Vec3 Value(float u, float v, const Vec3& p) const = 0;
};

// ----------------------------------------------------------------------------------------------------------------------------

class ConstantTexture : public Texture
{
public:

	ConstantTexture() {}

	ConstantTexture(Vec3 color) : Color(color) {}

	virtual Vec3 Value(float u, float v, const Vec3& p) const
	{
		return Color;
	}

private:

	Vec3 Color;
};

// ----------------------------------------------------------------------------------------------------------------------------

class CheckerTexture : public Texture
{
public:

	CheckerTexture() {}
	CheckerTexture(Texture* t0, Texture* t1) : Odd(t0), Even(t1) {}

	virtual Vec3 Value(float u, float v, const Vec3& p) const
	{
		float sines = sin(10.f * p.X()) * sin(10.f * p.Y()) * sin(10.f * p.Z());
		if (sines < 0)
		{
			return Odd->Value(u, v, p);
		}
		else
		{
			return Even->Value(u, v, p);
		}
	}

private:

	Texture* Odd;
	Texture* Even;
};

// ----------------------------------------------------------------------------------------------------------------------------

class NoiseTexture : public Texture
{
public:

	NoiseTexture(float scale) : Scale(scale) {}

	virtual Vec3 Value(float u, float v, const Vec3& p) const
	{
		return Vec3(1, 1, 1) * 0.5f * (1 + sin(Scale * p.Z() + 10 * Perlin::Turb(p)));
	}

private:

	float Scale;
};