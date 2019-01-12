#pragma once
#include <math.h>
#include <stdlib.h>
#include <iostream>

class vec3
{
public:

	vec3() {}

	vec3(float e0, float e1, float e2)
	{
		e[0] = e0; e[1] = e1; e[2] = e2;
	}

	inline float x() const { return e[0]; }
	inline float y() const { return e[1]; }
	inline float z() const { return e[2]; }
	inline float r() const { return e[0]; }
	inline float g() const { return e[1]; }
	inline float b() const { return e[2]; }

	inline float& x() { return e[0]; }
	inline float& y() { return e[1]; }
	inline float& z() { return e[2]; }
	inline float& r() { return e[0]; }
	inline float& g() { return e[1]; }
	inline float& b() { return e[2]; }

	inline const vec3& operator+() const { return *this; }
	inline vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
	inline float operator[](int i) const { return e[i]; }
	inline float& operator[](int i) { return e[i]; }

	inline vec3& operator+=(const vec3 &v2);
	inline vec3& operator-=(const vec3 &v2);
	inline vec3& operator*=(const vec3 &v2);
	inline vec3& operator/=(const vec3 &v2);
	inline vec3& operator*=(const float t);
	inline vec3& operator/=(const float t);

	inline float length() const
	{
		return sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
	}

	inline float squared_length() const
	{
		return (e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
	}

	inline void make_unit_vector();

private:

	float e[3];
};

inline std::istream& operator>>(std::istream &is, vec3 &t)
{
	is >> t.x() >> t.y() >> t.z();
}

inline std::ostream& operator<<(std::ostream &os, const vec3 &t)
{
	os << t.x() << " " << t.y() << " " << t.z();
	return os;
}

inline void vec3::make_unit_vector()
{
	float k = 1.0f / length();
	x() *= k;
	y() *= k;
	z() *= k;
}

inline vec3 operator+(const vec3 &v1, const vec3 &v2)
{
	return vec3(
		v1.x() + v2.x(),
		v1.y() + v2.y(),
		v1.z() + v2.z());
}

inline vec3 operator-(const vec3 &v1, const vec3 &v2)
{
	return vec3(
		v1.x() - v2.x(),
		v1.y() - v2.y(),
		v1.z() - v2.z());
}

inline vec3 operator*(const vec3 &v1, const vec3 &v2)
{
	return vec3(
		v1.x() * v2.x(),
		v1.y() * v2.y(),
		v1.z() * v2.z());
}

inline vec3 operator/(const vec3 &v1, const vec3 &v2)
{
	return vec3(
		v1.x() / v2.x(),
		v1.y() / v2.y(),
		v1.z() / v2.z());
}

inline vec3 operator*(float t, const vec3 &v)
{
	return vec3(
		t * v.x(),
		t * v.y(),
		t * v.z());
}

inline vec3 operator*(const vec3 &v, float t)
{
	return (t * v);
}

inline vec3 operator/(const vec3 &v, float t)
{
	return vec3(
		v.x() / t,
		v.y() / t,
		v.z() / t);
}

inline float dot(const vec3 &v1, const vec3 &v2)
{
	return
		(v1.x() * v2.x()) +
		(v1.y() * v2.y()) +
		(v1.z() * v2.z());
}

inline vec3 cross(const vec3& v1, const vec3& v2)
{
	return vec3(
		 (v1.y()*v2.z() - v1.z()*v2.y()),
		-(v1.x()*v2.z() - v1.z()*v2.x()),
		 (v1.x()*v2.y() - v1.y()*v2.x())
	);
}

inline vec3& vec3::operator+=(const vec3& v)
{
	x() += v.x();
	y() += v.y();
	z() += v.z();

	return *this;
}

inline vec3& vec3::operator-=(const vec3& v)
{
	x() -= v.x();
	y() -= v.y();
	z() -= v.z();

	return *this;
}

inline vec3& vec3::operator*=(const vec3& v)
{
	x() *= v.x();
	y() *= v.y();
	z() *= v.z();

	return *this;
}

inline vec3& vec3::operator/=(const vec3& v)
{
	x() /= v.x();
	y() /= v.y();
	z() /= v.z();

	return *this;
}

inline vec3& vec3::operator*=(const float t)
{
	float k = 1.0f / t;
	x() *= k;
	y() *= k;
	z() *= k;

	return *this;
}

inline vec3& vec3::operator/=(const float t)
{
	float k = 1.0f / t;
	x() *= k;
	y() *= k;
	z() *= k;

	return *this;
}

inline vec3 unit_vector(const vec3& v)
{
	return v / v.length();
}

inline vec3 reflect(const vec3& v, const vec3& n)
{
	return v - (2 * dot(v, n) * n);
}

inline bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted)
{
	vec3 uv = unit_vector(v);
	float dt = dot(uv, n);
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