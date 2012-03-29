#ifndef _LC_MATH_H_
#define _LC_MATH_H_

#include <math.h>

class lcVector3;
class lcVector4;
class lcMatrix33;
class lcMatrix44;

class lcVector3
{
public:
	lcVector3()
	{
	}

	lcVector3(const float _x, const float _y, const float _z)
		: x(_x), y(_y), z(_z)
	{
	}

	lcVector3(const lcVector3& a)
		: x(a.x), y(a.y), z(a.z)
	{
	}

	operator const float*() const
	{
		return (const float*)this;
	}
	
	float& operator[](int i) const
	{
		return ((float*)this)[i];
	}

	void Normalize();
	void Dot();
	float Length() const;

	float x, y, z;
};

class lcVector4
{
public:
	lcVector4()
	{
	}

	lcVector4(const float _x, const float _y, const float _z, const float _w)
		: x(_x), y(_y), z(_z), w(_w)
	{
	}

	operator const float*() const
	{
		return (const float*)this;
	}
	
	float& operator[](int i) const
	{
		return ((float*)this)[i];
	}

	float x, y, z, w;
};

inline lcVector3 operator+(const lcVector3& a, const lcVector3& b)
{
	return lcVector3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline lcVector3 operator-(const lcVector3& a, const lcVector3& b)
{
	return lcVector3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline lcVector3 operator*(const lcVector3& a, const lcVector3& b)
{
	return lcVector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

inline lcVector3 operator/(const lcVector3& a, const lcVector3& b)
{
	return lcVector3(a.x / b.x, a.y / b.y, a.z / b.z);
}

inline lcVector3 operator*(const lcVector3& a, float b)
{
	return lcVector3(a.x * b, a.y * b, a.z * b);
}

inline lcVector3 operator/(const lcVector3& a, float b)
{
	return lcVector3(a.x / b, a.y / b, a.z / b);
}

inline lcVector3 operator-(const lcVector3& a)
{
	return lcVector3(-a.x, -a.y, -a.z);
}

inline lcVector3& operator+=(lcVector3& a, const lcVector3& b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;

	return a;
}

inline lcVector3& operator-=(lcVector3& a, const lcVector3& b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;

	return a;
}

inline lcVector3& operator*=(lcVector3& a, const lcVector3& b)
{
	a.x *= b.x;
	a.y *= b.y;
	a.z *= b.z;

	return a;
}

inline lcVector3& operator/=(lcVector3& a, const lcVector3& b)
{
	a.x /= b.x;
	a.y /= b.y;
	a.z /= b.z;

	return a;
}

inline lcVector3& operator*=(lcVector3& a, float b)
{
	a.x *= b;
	a.y *= b;
	a.z *= b;

	return a;
}

inline lcVector3& operator/=(lcVector3& a, float b)
{
	a.x /= b;
	a.y /= b;
	a.z /= b;

	return a;
}

inline void lcVector3::Normalize()
{
	float InvLength = 1.0f / Length();

	x *= InvLength;
	y *= InvLength;
	z *= InvLength;
}

inline float lcVector3::Length() const
{
	return sqrtf(x * x + y * y + z * z);
}

inline lcVector3 lcNormalize(const lcVector3& a)
{
	lcVector3 Ret(a);
	Ret.Normalize();
	return Ret;
}

inline float lcDot(const lcVector3& a, const lcVector3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline lcVector3 lcCross(const lcVector3& a, const lcVector3& b)
{
	return lcVector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

#endif // _LC_MATH_H_
