#pragma once

#include <math.h>
#include <stdlib.h>
#include <iostream>

// ----------------------------------------------------------------------------------------------------------------------------

class Vec3
{
public:

	inline Vec3() {}

	inline Vec3(float e0, float e1, float e2)
	{
		e[0] = e0; e[1] = e1; e[2] = e2;
	}

	inline float X() const { return e[0]; }
	inline float Y() const { return e[1]; }
	inline float Z() const { return e[2]; }
	inline float R() const { return e[0]; }
	inline float G() const { return e[1]; }
	inline float B() const { return e[2]; }

	inline float& X() { return e[0]; }
	inline float& Y() { return e[1]; }
	inline float& Z() { return e[2]; }
	inline float& R() { return e[0]; }
	inline float& G() { return e[1]; }
	inline float& B() { return e[2]; }

	inline const Vec3& operator+() const       { return *this; }
	inline Vec3        operator-() const       { return Vec3(-e[0], -e[1], -e[2]); }
	inline float       operator[](int i) const { return e[i]; }
	inline float&      operator[](int i)       { return e[i]; }

	inline Vec3& operator+=(const Vec3 &v2);
	inline Vec3& operator-=(const Vec3 &v2);
	inline Vec3& operator*=(const Vec3 &v2);
	inline Vec3& operator/=(const Vec3 &v2);
	inline Vec3& operator*=(const float t);
	inline Vec3& operator/=(const float t);

	inline float Length() const
	{
		return sqrt(SquaredLength());
	}

	inline float SquaredLength() const
	{
		return (e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
	}

	inline void MakeUnitVector();

private:

	float e[3];
};

// ----------------------------------------------------------------------------------------------------------------------------

inline std::istream& operator>>(std::istream &is, Vec3 &t)
{
	is >> t.X() >> t.Y() >> t.Z();
}

// ----------------------------------------------------------------------------------------------------------------------------

inline std::ostream& operator<<(std::ostream &os, const Vec3 &t)
{
	os << t.X() << " " << t.Y() << " " << t.Z();
	return os;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline void Vec3::MakeUnitVector()
{
	float k = 1.0f / Length();
	X() *= k;
	Y() *= k;
	Z() *= k;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 operator+(const Vec3 &v1, const Vec3 &v2)
{
	return Vec3(
		v1.X() + v2.X(),
		v1.Y() + v2.Y(),
		v1.Z() + v2.Z());
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 operator-(const Vec3 &v1, const Vec3 &v2)
{
	return Vec3(
		v1.X() - v2.X(),
		v1.Y() - v2.Y(),
		v1.Z() - v2.Z());
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 operator*(const Vec3 &v1, const Vec3 &v2)
{
	return Vec3(
		v1.X() * v2.X(),
		v1.Y() * v2.Y(),
		v1.Z() * v2.Z());
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 operator/(const Vec3 &v1, const Vec3 &v2)
{
	return Vec3(
		v1.X() / v2.X(),
		v1.Y() / v2.Y(),
		v1.Z() / v2.Z());
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 operator*(float t, const Vec3 &v)
{
	return Vec3(
		t * v.X(),
		t * v.Y(),
		t * v.Z());
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 operator*(const Vec3 &v, float t)
{
	return (t * v);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 operator/(const Vec3 &v, float t)
{
	return Vec3(
		v.X() / t,
		v.Y() / t,
		v.Z() / t);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline float Dot(const Vec3 &v1, const Vec3 &v2)
{
	return
		(v1.X() * v2.X()) +
		(v1.Y() * v2.Y()) +
		(v1.Z() * v2.Z());
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 Cross(const Vec3& v1, const Vec3& v2)
{
	return Vec3(
		 (v1.Y()*v2.Z() - v1.Z()*v2.Y()),
		-(v1.X()*v2.Z() - v1.Z()*v2.X()),
		 (v1.X()*v2.Y() - v1.Y()*v2.X())
	);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3& Vec3::operator+=(const Vec3& v)
{
	X() += v.X();
	Y() += v.Y();
	Z() += v.Z();

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3& Vec3::operator-=(const Vec3& v)
{
	X() -= v.X();
	Y() -= v.Y();
	Z() -= v.Z();

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3& Vec3::operator*=(const Vec3& v)
{
	X() *= v.X();
	Y() *= v.Y();
	Z() *= v.Z();

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3& Vec3::operator/=(const Vec3& v)
{
	X() /= v.X();
	Y() /= v.Y();
	Z() /= v.Z();

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3& Vec3::operator*=(const float t)
{
	float k = 1.0f / t;
	X() *= k;
	Y() *= k;
	Z() *= k;

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3& Vec3::operator/=(const float t)
{
	float k = 1.0f / t;
	X() *= k;
	Y() *= k;
	Z() *= k;

	return *this;
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 UnitVector(const Vec3& v)
{
	return v / v.Length();
}

// ----------------------------------------------------------------------------------------------------------------------------

inline Vec3 Reflect(const Vec3& v, const Vec3& n)
{
	return v - (2 * Dot(v, n) * n);
}

// ----------------------------------------------------------------------------------------------------------------------------

inline bool Refract(const Vec3& v, const Vec3& n, float ni_over_nt, Vec3& refracted)
{
	Vec3 uv = UnitVector(v);
	float dt = Dot(uv, n);
	float discriminant = 1.0f - ni_over_nt * ni_over_nt*(1 - dt * dt);
	if (discriminant > 0)
	{
		refracted = ni_over_nt * (uv - n * dt) - n * sqrt(discriminant);
		return true;
	}
	else
	{
		return false;
	}
}