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
	
	operator float*()
	{
		return (float*)this;
	}
	
	const float& operator[](int i) const
	{
		return ((float*)this)[i];
	}

	float& operator[](int i)
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
	
	operator float*()
	{
		return (float*)this;
	}
	
	const float& operator[](int i) const
	{
		return ((float*)this)[i];
	}

	float& operator[](int i)
	{
		return ((float*)this)[i];
	}

	float x, y, z, w;
};

class lcMatrix44
{
public:
	lcMatrix44()
	{
	}

	lcMatrix44(const lcVector4& _x, const lcVector4& _y, const lcVector4& _z, const lcVector4& _w)
	{
		r[0] = _x;
		r[1] = _y;
		r[2] = _z;
		r[3] = _w;
	}

	void SetTranslation(const lcVector3& Translation)
	{
		r[3] = lcVector4(Translation[0], Translation[1], Translation[2], 1.0f);
	}

	operator const float*() const
	{
		return (const float*)this;
	}
	
	operator float*()
	{
		return (float*)this;
	}
	
	const lcVector4& operator[](int i) const
	{
		return r[i];
	}

	lcVector4& operator[](int i)
	{
		return r[i];
	}

	lcVector4 r[4];
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

inline float lcDot4(const lcVector4& a, const lcVector4& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline lcVector3 lcCross(const lcVector3& a, const lcVector3& b)
{
	return lcVector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

inline lcVector4 operator+(const lcVector4& a, const lcVector4& b)
{
	return lcVector4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline lcVector4 operator-(const lcVector4& a, const lcVector4& b)
{
	return lcVector4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

inline lcVector4 operator*(const lcVector4& a, float f)
{
	return lcVector4(a.x * f, a.y * f, a.z * f, a.w * f);
}

inline lcVector4 operator*(const lcVector4& a, const lcVector4& b)
{
	return lcVector4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

inline lcVector4 operator/(const lcVector4& a, float f)
{
	return lcVector4(a.x / f, a.y / f, a.z / f, a.w / f);
}

inline lcVector4 operator/(const lcVector4& a, const lcVector4& b)
{
	return lcVector4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

inline lcVector3 lcMul31(const lcVector3& a, const lcMatrix44& b)
{
	lcVector4 v = b.r[0] * a[0] + b.r[1] * a[1] + b.r[2] * a[2] + b.r[3];

	return lcVector3(v[0], v[1], v[2]);
}

inline lcVector3 lcMul30(const lcVector3& a, const lcMatrix44& b)
{
	lcVector4 v = b.r[0] * a[0] + b.r[1] * a[1] + b.r[2] * a[2];

	return lcVector3(v[0], v[1], v[2]);
}

inline lcVector4 lcMul4(const lcVector4& a, const lcMatrix44& b)
{
	return b.r[0] * a[0] + b.r[1] * a[1] + b.r[2] * a[2] + b.r[3] * a[3];
}

inline lcMatrix44 lcMul(const lcMatrix44& a, const lcMatrix44& b)
{
	lcVector4 Col0(b.r[0][0], b.r[1][0], b.r[2][0], b.r[3][0]);
	lcVector4 Col1(b.r[0][1], b.r[1][1], b.r[2][1], b.r[3][1]);
	lcVector4 Col2(b.r[0][2], b.r[1][2], b.r[2][2], b.r[3][2]);
	lcVector4 Col3(b.r[0][3], b.r[1][3], b.r[2][3], b.r[3][3]);

	lcVector4 Ret0(lcDot4(a.r[0], Col0), lcDot4(a.r[0], Col1), lcDot4(a.r[0], Col2), lcDot4(a.r[0], Col3));
	lcVector4 Ret1(lcDot4(a.r[1], Col0), lcDot4(a.r[1], Col1), lcDot4(a.r[1], Col2), lcDot4(a.r[1], Col3));
	lcVector4 Ret2(lcDot4(a.r[2], Col0), lcDot4(a.r[2], Col1), lcDot4(a.r[2], Col2), lcDot4(a.r[2], Col3));
	lcVector4 Ret3(lcDot4(a.r[3], Col0), lcDot4(a.r[3], Col1), lcDot4(a.r[3], Col2), lcDot4(a.r[3], Col3));

	return lcMatrix44(Ret0, Ret1, Ret2, Ret3);
}

inline lcMatrix44 lcMatrix44Identity()
{
	lcMatrix44 m;

	m.r[0] = lcVector4(1.0f, 0.0f, 0.0f, 0.0f);
	m.r[1] = lcVector4(0.0f, 1.0f, 0.0f, 0.0f);
	m.r[2] = lcVector4(0.0f, 0.0f, 1.0f, 0.0f);
	m.r[3] = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);

	return m;
}

inline lcMatrix44 lcMatrix44Translation(const lcVector3& Translation)
{
	lcMatrix44 m;

	m.r[0] = lcVector4(1.0f, 0.0f, 0.0f, 0.0f);
	m.r[1] = lcVector4(0.0f, 1.0f, 0.0f, 0.0f);
	m.r[2] = lcVector4(0.0f, 0.0f, 1.0f, 0.0f);
	m.r[3] = lcVector4(Translation[0], Translation[1], Translation[2], 1.0f);

	return m;
}

inline lcMatrix44 lcMatrix44RotationX(const float Radians)
{
	float s, c;

	s = sinf(Radians);
	c = cosf(Radians);

	lcMatrix44 m;

	m.r[0] = lcVector4(1.0f, 0.0f, 0.0f, 0.0f);
	m.r[1] = lcVector4(0.0f,    c,    s, 0.0f);
	m.r[2] = lcVector4(0.0f,   -s,    c, 0.0f);
	m.r[3] = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);

	return m;
}

inline lcMatrix44 lcMatrix44RotationY(const float Radians)
{
	float s, c;

	s = sinf(Radians);
	c = cosf(Radians);

	lcMatrix44 m;

	m.r[0] = lcVector4(   c, 0.0f,   -s, 0.0f);
	m.r[1] = lcVector4(0.0f, 1.0f, 0.0f, 0.0f);
	m.r[2] = lcVector4(   s, 0.0f,    c, 0.0f);
	m.r[3] = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);

	return m;
}

inline lcMatrix44 lcMatrix44RotationZ(const float Radians)
{
	float s, c;

	s = sinf(Radians);
	c = cosf(Radians);

	lcMatrix44 m;

	m.r[0] = lcVector4(   c,    s, 0.0f, 0.0f);
	m.r[1] = lcVector4(  -s,    c, 0.0f, 0.0f);
	m.r[2] = lcVector4(0.0f, 0.0f, 1.0f, 0.0f);
	m.r[3] = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);

	return m;
}

inline lcMatrix44 lcMatrix44FromAxisAngle(const lcVector3& Axis, const float Radians)
{
	float s, c, mag, xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	s = sinf(Radians);
	c = cosf(Radians);
	mag = Axis.Length();

	if (mag == 0.0f)
		return lcMatrix44Identity();

	lcVector3 Normal = Axis * (1.0f / mag);

	xx = Normal[0] * Normal[0];
	yy = Normal[1] * Normal[1];
	zz = Normal[2] * Normal[2];
	xy = Normal[0] * Normal[1];
	yz = Normal[1] * Normal[2];
	zx = Normal[2] * Normal[0];
	xs = Normal[0] * s;
	ys = Normal[1] * s;
	zs = Normal[2] * s;
	one_c = 1.0f - c;

	lcMatrix44 m;

	m.r[0] = lcVector4((one_c * xx) + c, (one_c * xy) + zs, (one_c * zx) - ys, 0.0f);
	m.r[1] = lcVector4((one_c * xy) - zs, (one_c * yy) + c, (one_c * yz) + xs, 0.0f);
	m.r[2] = lcVector4((one_c * zx) + ys, (one_c * yz) - xs, (one_c * zz) + c, 0.0f);
	m.r[3] = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);

	return m;
}

inline lcVector4 lcMatrix44ToAxisAngle(const lcMatrix44& m)
{
	lcVector3 Rows[3];
	Rows[0] = lcNormalize(lcVector3(m.r[0][0], m.r[0][1], m.r[0][2]));
	Rows[1] = lcNormalize(lcVector3(m.r[1][0], m.r[1][1], m.r[1][2]));
	Rows[2] = lcNormalize(lcVector3(m.r[2][0], m.r[2][1], m.r[2][2]));

	// Determinant should be 1 for rotation matrices.
	float Determinant = Rows[0][0] * Rows[1][1] * Rows[2][2] + Rows[0][1] * Rows[1][2] * Rows[2][0] +
	                    Rows[0][2] * Rows[1][0] * Rows[2][1] - Rows[0][0] * Rows[1][2] * Rows[2][1] - 
	                    Rows[0][1] * Rows[1][0] * Rows[2][2] - Rows[0][2] * Rows[1][1] * Rows[2][0];

	if (Determinant < 0.0f)
		Rows[0] *= -1.0f;

	float Trace = Rows[0][0] + Rows[1][1] + Rows[2][2];
	float Cos = 0.5f * (Trace - 1.0f);
	lcVector4 rot;

	if (Cos < -1.0f)
		Cos = -1.0f;
	else if (Cos > 1.0f)
		Cos = 1.0f;
	rot[3] = acosf(Cos);  // in [0,PI]

	if (rot[3] > 0.01f)
	{
		if (fabsf(3.141592f - rot[3]) > 0.01f)
		{
			rot[0] = Rows[1][2] - Rows[2][1];
			rot[1] = Rows[2][0] - Rows[0][2];
			rot[2] = Rows[0][1] - Rows[1][0];

			float inv = 1.0f / sqrtf(rot[0]*rot[0] + rot[1]*rot[1] + rot[2]*rot[2]);

			rot[0] *= inv;
			rot[1] *= inv;
			rot[2] *= inv;
		}
		else
		{
			// angle is PI
			float HalfInverse;
			if (Rows[0][0] >= Rows[1][1])
			{
				// r00 >= r11
				if (Rows[0][0] >= Rows[2][2])
				{
					// r00 is maximum diagonal term
					rot[0] = 0.5f * sqrtf(Rows[0][0] - Rows[1][1] - Rows[2][2] + 1.0f);
					HalfInverse = 0.5f / rot[0];
					rot[1] = HalfInverse * Rows[1][0];
					rot[2] = HalfInverse * Rows[2][0];
				}
				else
				{
					// r22 is maximum diagonal term
					rot[2] = 0.5f * sqrtf(Rows[2][2] - Rows[0][0] - Rows[1][1] + 1.0f);
					HalfInverse = 0.5f / rot[2];
					rot[0] = HalfInverse * Rows[2][0];
					rot[1] = HalfInverse * Rows[2][1];
				}
			}
			else
			{
				// r11 > r00
				if (Rows[1][1] >= Rows[2][2])
				{
					// r11 is maximum diagonal term
					rot[1] = 0.5f * sqrtf(Rows[1][1] - Rows[0][0] - Rows[2][2] + 1.0f);
					HalfInverse  = 0.5f / rot[1];
					rot[0] = HalfInverse * Rows[1][0];
					rot[2] = HalfInverse * Rows[2][1];
				}
				else
				{
					// r22 is maximum diagonal term
					rot[2] = 0.5f * sqrtf(Rows[2][2] - Rows[0][0] - Rows[1][1] + 1.0f);
					HalfInverse = 0.5f / rot[2];
					rot[0] = HalfInverse * Rows[2][0];
					rot[1] = HalfInverse * Rows[2][1];
				}
			}
		}
	}
	else
	{
		// The angle is 0 and the matrix is the identity.
		rot[0] = 0.0f;
		rot[1] = 0.0f;
		rot[2] = 1.0f;
	}

	return rot;
}

#endif // _LC_MATH_H_
