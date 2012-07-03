#ifndef _LC_MATH_H_
#define _LC_MATH_H_

#include <math.h>
#include <float.h>

#define LC_DTOR 0.017453f
#define LC_RTOD 57.29578f
#define LC_PI   3.141592f
#define LC_2PI  6.283185f

template <typename T, typename U>
inline T lcMin(const T& a, const U& b)
{
	return a < b ? a : b;
}

template <typename T, typename U>
inline T lcMax(const T& a, const U& b)
{
	return a > b ? a : b;
}

template <typename T, typename U, typename V>
inline T lcClamp(const T& Value, const U& Min, const V& Max)
{
	if (Value > Max)
		return Max;
	else if (Value < Min)
		return Min;
	else
		return Value;
}

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
	float Length() const;
	float LengthSquared() const;

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

	lcVector4(const lcVector3& _xyz, const float _w)
		: x(_xyz.x), y(_xyz.y), z(_xyz.z), w(_w)
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

inline lcVector3 operator*(float a, const lcVector3& b)
{
	return lcVector3(b.x * a, b.y * a, b.z * a);
}

inline lcVector3 operator/(float a, const lcVector3& b)
{
	return lcVector3(b.x / a, b.y / a, b.z / a);
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

inline float lcVector3::LengthSquared() const
{
	return x * x + y * y + z * z;
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

inline float lcDot3(const lcVector4& a, const lcVector3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float lcDot3(const lcVector3& a, const lcVector4& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float lcDot(const lcVector4& a, const lcVector4& b)
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

inline lcVector4& operator*=(lcVector4& a, float b)
{
	a.x *= b;
	a.y *= b;
	a.z *= b;
	a.w *= b;

	return a;
}

inline lcVector4& operator/=(lcVector4& a, float b)
{
	a.x /= b;
	a.y /= b;
	a.z /= b;
	a.w /= b;

	return a;
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

	lcVector4 Ret0(lcDot(a.r[0], Col0), lcDot(a.r[0], Col1), lcDot(a.r[0], Col2), lcDot(a.r[0], Col3));
	lcVector4 Ret1(lcDot(a.r[1], Col0), lcDot(a.r[1], Col1), lcDot(a.r[1], Col2), lcDot(a.r[1], Col3));
	lcVector4 Ret2(lcDot(a.r[2], Col0), lcDot(a.r[2], Col1), lcDot(a.r[2], Col2), lcDot(a.r[2], Col3));
	lcVector4 Ret3(lcDot(a.r[3], Col0), lcDot(a.r[3], Col1), lcDot(a.r[3], Col2), lcDot(a.r[3], Col3));

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

inline lcMatrix44 lcMatrix44LookAt(const lcVector3& Eye, const lcVector3& Target, const lcVector3& Up)
{
	lcVector3 x, y, z;

	z = lcNormalize(Eye - Target);
	x = lcNormalize(lcCross(Up, z));
	y = lcNormalize(lcCross(z, x));

	lcMatrix44 m;

	m.r[0] = lcVector4(x[0], y[0], z[0], 0.0f);
	m.r[1] = lcVector4(x[1], y[1], z[1], 0.0f);
	m.r[2] = lcVector4(x[2], y[2], z[2], 0.0f);
	m.r[3] = m.r[0] * -Eye[0] + m.r[1] * -Eye[1] + m.r[2] * -Eye[2];
	m.r[3][3] = 1.0f;

	return m;
}

inline lcMatrix44 lcMatrix44Perspective(float FoVy, float Aspect, float Near, float Far)
{
	float Left, Right, Bottom, Top;

	Top = Near * (float)tan(FoVy * LC_PI / 360.0f);
	Bottom = -Top;

	Left = Bottom * Aspect;
	Right = Top * Aspect;

	if ((Near <= 0.0f) || (Far <= 0.0f) || (Near == Far) || (Left == Right) || (Top == Bottom))
		return lcMatrix44Identity();

	float x, y, a, b, c, d;

	x = (2.0f * Near) / (Right - Left);
	y = (2.0f * Near) / (Top - Bottom);
	a = (Right + Left) / (Right - Left);
	b = (Top + Bottom) / (Top - Bottom);
	c = -(Far + Near) / (Far - Near);
	d = -(2.0f * Far * Near) / (Far - Near);

	lcMatrix44 m;

	m.r[0] = lcVector4(x, 0, 0,  0);
	m.r[1] = lcVector4(0, y, 0,  0);
	m.r[2] = lcVector4(a, b, c, -1);
	m.r[3] = lcVector4(0, 0, d,  0);

	return m;
}

inline lcMatrix44 lcMatrix44Ortho(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
	lcMatrix44 m;

	m.r[0] = lcVector4(2.0f / (Right-Left), 0.0f, 0.0f, 0.0f),
	m.r[1] = lcVector4(0.0f, 2.0f / (Top-Bottom), 0.0f, 0.0f),
	m.r[2] = lcVector4(0.0f, 0.0f, -2.0f / (Far-Near), 0.0f),
	m.r[3] = lcVector4(-(Right+Left) / (Right-Left), -(Top+Bottom) / (Top-Bottom), -(Far+Near) / (Far-Near), 1.0f);

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

	rot[3] = acosf(lcClamp(Cos, -1.0f, 1.0f));  // in [0,PI]

	if (rot[3] > 0.01f)
	{
		if (fabsf(LC_PI - rot[3]) > 0.01f)
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

inline lcMatrix44 lcMatrix44FromEulerAngles(const lcVector3& Radians)
{
	float CosYaw, SinYaw, CosPitch, SinPitch, CosRoll, SinRoll;

	CosRoll = cosf(Radians[0]);
	SinRoll = sinf(Radians[0]);
	CosPitch = cosf(Radians[1]);
	SinPitch = sinf(Radians[1]);
	CosYaw = cosf(Radians[2]);
	SinYaw = sinf(Radians[2]);

	lcMatrix44 m;

	m.r[0] = lcVector4(CosYaw * CosPitch, SinYaw * CosPitch, -SinPitch, 0.0f);
	m.r[1] = lcVector4(CosYaw * SinPitch * SinRoll - SinYaw * CosRoll, CosYaw * CosRoll + SinYaw * SinPitch * SinRoll, CosPitch * SinRoll, 0.0f);
	m.r[2] = lcVector4(CosYaw * SinPitch * CosRoll + SinYaw * SinRoll, SinYaw * SinPitch * CosRoll - CosYaw * SinRoll, CosPitch * CosRoll, 0.0f);
	m.r[3] = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);

	return m;
}

inline lcVector3 lcMatrix44ToEulerAngles(const lcMatrix44& RotMat)
{
	float SinPitch, CosPitch, SinRoll, CosRoll, SinYaw, CosYaw;

	SinPitch = -RotMat.r[0][2];
	CosPitch = sqrtf(1 - SinPitch*SinPitch);

	if (fabsf(CosPitch) > 0.0005f)
	{
		SinRoll = RotMat.r[1][2] / CosPitch;
		CosRoll = RotMat.r[2][2] / CosPitch;
		SinYaw = RotMat.r[0][1] / CosPitch;
		CosYaw = RotMat.r[0][0] / CosPitch;
	} 
	else
	{
		SinRoll = -RotMat.r[2][1];
		CosRoll = RotMat.r[1][1];
		SinYaw = 0.0f;
		CosYaw = 1.0f;
	}

	lcVector3 Rot(atan2f(SinRoll, CosRoll), atan2f(SinPitch, CosPitch), atan2f(SinYaw, CosYaw));

	if (Rot[0] < 0) Rot[0] += LC_2PI;
	if (Rot[1] < 0) Rot[1] += LC_2PI;
	if (Rot[2] < 0) Rot[2] += LC_2PI;

	return Rot;
}

inline lcMatrix44 lcMatrix44Transpose(const lcMatrix44& m)
{
	lcMatrix44 t;

	t.r[0] = lcVector4(m[0][0], m[1][0], m[2][0], m[3][0]);
	t.r[1] = lcVector4(m[0][1], m[1][1], m[2][1], m[3][1]);
	t.r[2] = lcVector4(m[0][2], m[1][2], m[2][2], m[3][2]);
	t.r[3] = lcVector4(m[0][3], m[1][3], m[2][3], m[3][3]);

	return t;
}

inline lcMatrix44 lcMatrix44AffineInverse(const lcMatrix44& m)
{
	lcMatrix44 Inv;

	Inv.r[0] = lcVector4(m.r[0][0], m.r[1][0], m.r[2][0], m.r[0][3]);
	Inv.r[1] = lcVector4(m.r[0][1], m.r[1][1], m.r[2][1], m.r[1][3]);
	Inv.r[2] = lcVector4(m.r[0][2], m.r[1][2], m.r[2][2], m.r[2][3]);

	lcVector3 Trans = -lcMul30(lcVector3(m[3][0], m[3][1], m[3][2]), Inv);
	Inv.r[3] = lcVector4(Trans[0], Trans[1], Trans[2], 1.0f);

	return Inv;
}

// Inverse code from the GLU library.
inline lcMatrix44 lcMatrix44Inverse(const lcMatrix44& m)
{
#define SWAP_ROWS(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,col,row) m.r[row][col]

	float wtmp[4][8];
	float m0, m1, m2, m3, s;
	float *r0, *r1, *r2, *r3;

	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

	r0[0] = MAT(m,0,0), r0[1] = MAT(m,0,1),
	r0[2] = MAT(m,0,2), r0[3] = MAT(m,0,3),
	r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

	r1[0] = MAT(m,1,0), r1[1] = MAT(m,1,1),
	r1[2] = MAT(m,1,2), r1[3] = MAT(m,1,3),
	r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

	r2[0] = MAT(m,2,0), r2[1] = MAT(m,2,1),
	r2[2] = MAT(m,2,2), r2[3] = MAT(m,2,3),
	r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

	r3[0] = MAT(m,3,0), r3[1] = MAT(m,3,1),
	r3[2] = MAT(m,3,2), r3[3] = MAT(m,3,3),
	r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

	// choose pivot - or die
	if (fabs(r3[0])>fabs(r2[0])) SWAP_ROWS(r3, r2);
	if (fabs(r2[0])>fabs(r1[0])) SWAP_ROWS(r2, r1);
	if (fabs(r1[0])>fabs(r0[0])) SWAP_ROWS(r1, r0);
//	if (0.0 == r0[0])  return GL_FALSE;

	// eliminate first variable
	m1 = r1[0]/r0[0]; m2 = r2[0]/r0[0]; m3 = r3[0]/r0[0];
	s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
	s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
	s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
	s = r0[5];
	if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
	s = r0[6];
	if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
	s = r0[7];
	if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

	// choose pivot - or die
	if (fabs(r3[1])>fabs(r2[1])) SWAP_ROWS(r3, r2);
	if (fabs(r2[1])>fabs(r1[1])) SWAP_ROWS(r2, r1);
//	if (0.0 == r1[1])  return GL_FALSE;

	// eliminate second variable
	m2 = r2[1]/r1[1]; m3 = r3[1]/r1[1];
	r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
	s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
	s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
	s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
	s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

	// choose pivot - or die
	if (fabs(r3[2])>fabs(r2[2])) SWAP_ROWS(r3, r2);
//	if (0.0 == r2[2])  return GL_FALSE;

	// eliminate third variable
	m3 = r3[2]/r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
	r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
	r3[7] -= m3 * r2[7];

	// last check
//	if (0.0 == r3[3]) return GL_FALSE;

	s = 1.0f/r3[3];              // now back substitute row 3
	r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

	m2 = r2[3];                 // now back substitute row 2
	s  = 1.0f/r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
	r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
	r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
	r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

	m1 = r1[2];                 // now back substitute row 1
	s  = 1.0f/r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
	r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
	r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

	m0 = r0[1];                 // now back substitute row 0
	s  = 1.0f/r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
	r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

	lcVector4 Row0(r0[4], r1[4], r2[4], r3[4]);
	lcVector4 Row1(r0[5], r1[5], r2[5], r3[5]);
	lcVector4 Row2(r0[6], r1[6], r2[6], r3[6]);
	lcVector4 Row3(r0[7], r1[7], r2[7], r3[7]);
	
	lcMatrix44 out(Row0, Row1, Row2, Row3);
	
	return out;

#undef MAT
#undef SWAP_ROWS
}

// Convert world coordinates to screen coordinates.
inline lcVector3 lcProjectPoint(const lcVector3& Point, const lcMatrix44& ModelView, const lcMatrix44& Projection, const int Viewport[4])
{
	lcVector4 Tmp;

	Tmp = lcMul4(lcVector4(Point[0], Point[1], Point[2], 1.0f), ModelView);
	Tmp = lcMul4(Tmp, Projection);

	// Normalize.
	Tmp /= Tmp[3];

	// Screen coordinates.
	return lcVector3(Viewport[0] + (1 + Tmp[0]) * Viewport[2] / 2, Viewport[1] + (1 + Tmp[1]) * Viewport[3] / 2, (1 + Tmp[2]) / 2);
}

inline lcVector3 lcUnprojectPoint(const lcVector3& Point, const lcMatrix44& ModelView, const lcMatrix44& Projection, const int Viewport[4])
{
	// Calculate the screen to model transform.
	lcMatrix44 Transform = lcMatrix44Inverse(lcMul(ModelView, Projection));

	lcVector4 Tmp;

	// Convert the point to homogeneous coordinates.
	Tmp[0] = (Point[0] - Viewport[0]) * 2.0f / Viewport[2] - 1.0f;
	Tmp[1] = (Point[1] - Viewport[1]) * 2.0f / Viewport[3] - 1.0f;
	Tmp[2] = Point[2] * 2.0f - 1.0f;
	Tmp[3] = 1.0f;

	Tmp = lcMul4(Tmp, Transform);

	if (Tmp[3] != 0.0f)
		Tmp /= Tmp[3];

	return lcVector3(Tmp[0], Tmp[1], Tmp[2]);
}

inline void lcUnprojectPoints(lcVector3* Points, int NumPoints, const lcMatrix44& ModelView, const lcMatrix44& Projection, const int Viewport[4])
{
	// Calculate the screen to model transform.
	lcMatrix44 Transform = lcMatrix44Inverse(lcMul(ModelView, Projection));

	for (int i = 0; i < NumPoints; i++)
	{
		lcVector4 Tmp;

		// Convert the point to homogeneous coordinates.
		Tmp[0] = (Points[i][0] - Viewport[0]) * 2.0f / Viewport[2] - 1.0f;
		Tmp[1] = (Points[i][1] - Viewport[1]) * 2.0f / Viewport[3] - 1.0f;
		Tmp[2] = Points[i][2] * 2.0f - 1.0f;
		Tmp[3] = 1.0f;

		Tmp = lcMul4(Tmp, Transform);

		if (Tmp[3] != 0.0f)
			Tmp /= Tmp[3];

		Points[i] = lcVector3(Tmp[0], Tmp[1], Tmp[2]);
	}
}

inline void lcGetFrustumPlanes(const lcMatrix44& WorldView, const lcMatrix44& Projection, lcVector4 Planes[6])
{
	lcMatrix44 WorldProj = lcMul(WorldView, Projection);

	Planes[0][0] = (WorldProj[0][0] - WorldProj[0][3]) * -1;
	Planes[0][1] = (WorldProj[1][0] - WorldProj[1][3]) * -1;
	Planes[0][2] = (WorldProj[2][0] - WorldProj[2][3]) * -1;
	Planes[0][3] = (WorldProj[3][0] - WorldProj[3][3]) * -1;
	Planes[1][0] =  WorldProj[0][0] + WorldProj[0][3];
	Planes[1][1] =  WorldProj[1][0] + WorldProj[1][3];
	Planes[1][2] =  WorldProj[2][0] + WorldProj[2][3];
	Planes[1][3] =  WorldProj[3][0] + WorldProj[3][3];
	Planes[2][0] = (WorldProj[0][1] - WorldProj[0][3]) * -1;
	Planes[2][1] = (WorldProj[1][1] - WorldProj[1][3]) * -1;
	Planes[2][2] = (WorldProj[2][1] - WorldProj[2][3]) * -1;
	Planes[2][3] = (WorldProj[3][1] - WorldProj[3][3]) * -1;
	Planes[3][0] =  WorldProj[0][1] + WorldProj[0][3];
	Planes[3][1] =  WorldProj[1][1] + WorldProj[1][3];
	Planes[3][2] =  WorldProj[2][1] + WorldProj[2][3];
	Planes[3][3] =  WorldProj[3][1] + WorldProj[3][3];
	Planes[4][0] = (WorldProj[0][2] - WorldProj[0][3]) * -1;
	Planes[4][1] = (WorldProj[1][2] - WorldProj[1][3]) * -1;
	Planes[4][2] = (WorldProj[2][2] - WorldProj[2][3]) * -1;
	Planes[4][3] = (WorldProj[3][2] - WorldProj[3][3]) * -1;
	Planes[5][0] =  WorldProj[0][2] + WorldProj[0][3];
	Planes[5][1] =  WorldProj[1][2] + WorldProj[1][3];
	Planes[5][2] =  WorldProj[2][2] + WorldProj[2][3];
	Planes[5][3] =  WorldProj[3][2] + WorldProj[3][3];

	for (int i = 0; i < 6; i++)
	{
		lcVector3 Normal(Planes[i][0], Planes[i][1], Planes[i][2]);
		float Length = Normal.Length();
		Planes[i] /= -Length;
	}
}

inline lcVector3 lcZoomExtents(const lcVector3& Position, const lcMatrix44& WorldView, const lcMatrix44& Projection, const lcVector3* Points, int NumPoints)
{
	if (!NumPoints)
		return Position;

	lcVector4 Planes[6];
	lcGetFrustumPlanes(WorldView, Projection, Planes);

	lcVector3 Front(WorldView[0][2], WorldView[1][2], WorldView[2][2]);

	// Calculate the position that is as close as possible to the model and has all pieces visible.
	float SmallestDistance = FLT_MAX;

	for (int p = 0; p < 4; p++)
	{
		lcVector3 Plane(Planes[p][0], Planes[p][1], Planes[p][2]);
		float ep = lcDot(Position, Plane);
		float fp = lcDot(Front, Plane);

		for (int j = 0; j < NumPoints; j++)
		{
			// Intersect the camera line with the plane that contains this point, NewEye = Eye + u * (Target - Eye)
			float u = (ep - lcDot(Points[j], Plane)) / fp;

			if (u < SmallestDistance)
				SmallestDistance = u;
		}
	}

	return Position - (Front * SmallestDistance);
}

// Calculate the intersection of a line segment and a plane and returns false
// if they are parallel or the intersection is outside the line segment.
inline bool lcLinePlaneIntersection(lcVector3& Intersection, const lcVector3& Start, const lcVector3& End, const lcVector4& Plane)
{
	lcVector3 Dir = End - Start;
	lcVector3 PlaneNormal(Plane[0], Plane[1], Plane[2]);

	float t1 = lcDot(PlaneNormal, Start) + Plane[3];
	float t2 = lcDot(PlaneNormal, Dir);

	if (t2 == 0.0f)
		return false;

	float t = -t1 / t2;

	Intersection = Start + t * Dir;

	if ((t < 0.0f) || (t > 1.0f))
		return false;

	return true;
}

inline bool lcLineTriangleMinIntersection(const lcVector3& p1, const lcVector3& p2, const lcVector3& p3, const lcVector3& Start, const lcVector3& End, float& MinDist, lcVector3& Intersection)
{
	// Calculate the polygon plane.
	lcVector3 PlaneNormal = lcCross(p1 - p2, p3 - p2);
	float PlaneD = -lcDot(PlaneNormal, p1);

	// Check if the line is parallel to the plane.
	lcVector3 Dir = End - Start;

	float t1 = lcDot(PlaneNormal, Start) + PlaneD;
	float t2 = lcDot(PlaneNormal, Dir);

	if (t2 == 0)
		return false;

	float t = -(t1 / t2);

	if (t < 0)
		return false;

	// Intersection of the plane and line segment.
	Intersection = Start - (t1 / t2) * Dir;

	float Dist = (Start - Intersection).Length();

	if (Dist > MinDist)
		return false;

	// Check if we're inside the triangle.
	lcVector3 pa1, pa2, pa3;
	pa1 = lcNormalize(p1 - Intersection);
	pa2 = lcNormalize(p2 - Intersection);
	pa3 = lcNormalize(p3 - Intersection);

	float a1, a2, a3;
	a1 = lcDot(pa1, pa2);
	a2 = lcDot(pa2, pa3);
	a3 = lcDot(pa3, pa1);

	float total = (acosf(a1) + acosf(a2) + acosf(a3)) * LC_RTOD;

	if (fabs(total - 360) <= 0.001f)
	{
		MinDist = Dist;
		return true;
	}

	return false;
}

#endif // _LC_MATH_H_
