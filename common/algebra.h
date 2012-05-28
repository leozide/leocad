#ifndef _ALGEBRA_H_
#define _ALGEBRA_H_

#include <math.h>

//
// Simple math library and linear algebra functions.
//
// Everything is based on the Vector4 class, so changing that class should be enough
// to add support for compiler specific math intrinsics.
//
// Functions that end with 34 mean that they don't care what happens to the 4th
// component, it can either be affected or not.
//
// Matrices are represented as row-major, so we pre-multiply instead of post-multiplying
// like you would in a column major notation.
//
// OpenGL only expects a matrix to be an array of 16 floats so it doesn't matter what
// notation we use.
//
// v[0]  v[1]  v[2]  v[3]   <- x, y, z, w
//
// m[0]  m[1]  m[2]  m[3]   <- x axis
// m[4]  m[5]  m[6]  m[7]   <- y axis
// m[8]  m[9]  m[10] m[11]  <- z axis
// m[12] m[13] m[14] m[15]  <- translation
// 

// TODO: Move this define to config.h
#define LC_MATH_FLOAT
//#define LC_MATH_SSE

// Classes defined in this file:
class Vector3;
class Vector4;
class Quaternion;
class Matrix44;

// ============================================================================
// Vector4 class (float version).

#ifdef LC_MATH_FLOAT

class Vector4
{
public:
	// Constructors.
	inline Vector4() { }
	inline explicit Vector4(const float _x, const float _y, const float _z)
		: x(_x), y(_y), z(_z) { }
	inline explicit Vector4(const float _x, const float _y, const float _z, const float _w)
		: x(_x), y(_y), z(_z), w(_w) { }

	inline operator const float*() const { return (const float*)this; }
	inline float& operator[](int i) const { return ((float*)this)[i]; }

	// Comparison.
	friend inline bool operator==(const Vector4& a, const Vector4& b)
	{ return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w); }

	friend inline bool Compare3(const Vector4& a, const Vector4& b)
	{ return (a.x == b.x) && (a.y == b.y) && (a.z == b.z); }

	// Math operations for 4 components.
	friend inline Vector4 operator+(const Vector4& a, const Vector4& b)
	{ return Vector4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }

	friend inline Vector4 operator-(const Vector4& a, const Vector4& b)
	{ return Vector4(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w); }

	friend inline Vector4 operator*(const Vector4& a, float f)
	{ return Vector4(a.x*f, a.y*f, a.z*f, a.w*f); }

	friend inline Vector4 operator*(const Vector4& a, const Vector4& b)
	{ return Vector4(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w); }

	friend inline Vector4 operator/(const Vector4& a, float f)
	{ return Vector4(a.x/f, a.y/f, a.z/f, a.w/f); }

	friend inline Vector4 operator/=(Vector4& a, float f)
	{ a = Vector4(a.x/f, a.y/f, a.z/f, a.w/f); return a; }

	friend inline Vector4 operator-(const Vector4& a)
	{ return Vector4(-a.x, -a.y, -a.z, -a.w); }

	// Math operations ignoring the 4th component.
	friend inline Vector4 Add34(const Vector4& a, const Vector4& b)
	{ return Vector4(a.x+b.x, a.y+b.y, a.z+b.z); }

	friend inline Vector4 Subtract34(const Vector4& a, const Vector4& b)
	{ return Vector4(a.x-b.x, a.y-b.y, a.z-b.z); }

	friend inline Vector4 Multiply34(const Vector4& a, float f)
	{ return Vector4(a.x*f, a.y*f, a.z*f); }

	friend inline Vector4 Multiply34(const Vector4& a, const Vector4& b)
	{ return Vector4(a.x*b.x, a.y*b.y, a.z*b.z); }

	friend inline Vector4 Divide34(const Vector4& a, float f)
	{ return Vector4(a.x/f, a.y/f, a.z/f); }

	friend inline Vector4 Negate34(const Vector4& a)
	{ return Vector4(-a.x, -a.y, -a.z, -a.w); }

	// Dot product.
	friend inline float Dot3(const Vector4& a, const Vector4& b)
	{ return a.x*b.x + a.y*b.y + a.z*b.z; }

	friend inline float Dot4(const Vector4& a, const Vector4& b)
	{ return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

	// Cross product.
	friend inline Vector4 Cross3(const Vector4& a, const Vector4& b)
	{ return Vector4(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }

	// Other functions.
	inline float Length3() const
	{ return sqrtf(x*x + y*y + z*z); }

	inline void Normalize34()
	{
		float len = 1.0f / sqrtf(x*x + y*y + z*z);

		x *= len;
		y *= len;
		z *= len;
	}

	inline void Abs34()
	{
		if (x < 0.0f) x = -x;
		if (y < 0.0f) y = -y;
		if (z < 0.0f) z = -z;
	}

	inline void Abs()
	{
		if (x < 0.0f) x = -x;
		if (y < 0.0f) y = -y;
		if (z < 0.0f) z = -z;
		if (w < 0.0f) w = -w;
	}

protected:
	float x, y, z, w;
};

#endif

// ============================================================================
// Vector4 class (SSE version).

#ifdef LC_MATH_SSE

// If you can't find this file you need to install the VS6 Processor Pack.
#include <xmmintrin.h>

class __declspec(align(16)) Vector4
{
public:
	// Constructors.
	inline Vector4() { }
	inline explicit Vector4(const __m128& _xyzw)
		: xyzw(_xyzw) { }
	inline explicit Vector4(const float _x, const float _y, const float _z)
		: xyzw(_mm_setr_ps(_x, _y, _z, _z)) { }
	inline explicit Vector4(const float _x, const float _y, const float _z, const float _w)
		: xyzw(_mm_setr_ps(_x, _y, _z, _w)) { }

	inline float& operator[](int i) const { return ((const float*)this)[i]; }

	// Comparison.
	friend inline bool operator==(const Vector4& a, const Vector4& b)
	{ return !_mm_movemask_ps(_mm_cmpneq_ps(a.xyzw, b.xyzw)); }

	friend inline bool Compare3(const Vector4& a, const Vector4& b)
	{ return (_mm_movemask_ps(_mm_cmpeq_ps(a.xyzw, b.xyzw)) & 0x7) == 0x7; }

	// Math operations for 4 components.
	friend inline Vector4 operator+(const Vector4& a, const Vector4& b)
	{ return Vector4(_mm_add_ps(a.xyzw, b.xyzw)); }

	friend inline Vector4 operator-(const Vector4& a, const Vector4& b)
	{ return Vector4(_mm_sub_ps(a.xyzw, b.xyzw)); }

	friend inline Vector4 operator*(const Vector4& a, float f)
	{ return Vector4(_mm_mul_ps(a.xyzw, _mm_load_ps1(&f))); }

	friend inline Vector4 operator*(const Vector4& a, const Vector4& b)
	{ return Vector4(_mm_mul_ps(a.xyzw, b.xyzw)); }

	friend inline Vector4 operator/(const Vector4& a, float f)
	{ return Vector4(_mm_div_ps(a.xyzw, _mm_load_ps1(&f))); }

	friend inline Vector4 operator-(const Vector4& a)
	{
		static const __declspec(align(16)) unsigned int Mask[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 }
		return Vector4(_mm_xor_ps(xyzw, *(__m128*)&Mask));
	}

	// Math operations ignoring the 4th component.
	friend inline Vector4 Add34(const Vector4& a, const Vector4& b)
	{ return a*b }

	friend inline Vector4 Subtract34(const Vector4& a, const Vector4& b)
	{ return a-b; }

	friend inline Vector4 Multiply34(const Vector4& a, float f)
	{ return a*f; }

	friend inline Vector4 Multiply34(const Vector4& a, const Vector4& b)
	{ return a*b; }

	friend inline Vector4 Divide34(const Vector4& a, float f)
	{ return a/f; }

	friend inline Vector4 Negate34(const Vector4& a)
	{ return -a; }

	// Dot product.
	friend inline float Dot3(const Vector4& a, const Vector4& b)
	{
		__m128 tmp = _mm_mul_ps(a.xyzw, b.xyzw);
		__m128 yz = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 2, 2, 2)));
		tmp = _mm_add_ss(tmp, yz);

		return *(const float*)&tmp;
	}

	// Cross product.
	friend inline Vector4 Cross3(const Vector4& a, const Vector4& b)
	{
		// a(yzx)*b(zxy)-a(zxy)*b(yzx)
		__m128 r1 = _mm_mul_ps(_mm_shuffle_ps(a.xyzw, a.xyzw, _MM_SHUFFLE(0, 0, 2, 1)), _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(0, 1, 0, 2)));
		__m128 r2 = _mm_mul_ps(_mm_shuffle_ps(a.xyzw, a.xyzw, _MM_SHUFFLE(0, 1, 0, 2)), _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(0, 0, 2, 1)));

		return Vector4(_mm_sub_ps(r1, r2));
	}

	// Other functions.
	inline float Length3() const
	{
		__m128 tmp = _mm_mul_ps(xyzw, xyzw);
		__m128 yz = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 2, 2, 2)));
		tmp = _mm_add_ss(tmp, yz);
		tmp = _mm_sqrt_ss(tmp);

		return *(const float*)&tmp;
	}

	inline void Normalize34()
	{
		__m128 tmp = _mm_mul_ps(xyzw, xyzw);
		__m128 yz = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 2, 2, 2)));
		tmp = _mm_add_ss(tmp, yz);
		tmp = _mm_rsqrt_ss(tmp);
		tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(0, 0, 0, 0));
		xyzw = _mm_mul_ps(xyzw, tmp);
	}

	inline void Abs()
	{
		static const __declspec(align(16)) unsigned int Mask[4] = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff }
		xyzw = _mm_and_ps(xyzw, *(__m128*)&Mask);
	}

protected:
	__m128 xyzw;
};

#endif

// ============================================================================
// 3D Vector class.

class Vector3
{
public:
	// Constructors.
	inline Vector3()
		{ }
	inline explicit Vector3(const Vector4& _v)
		: m_Value(_v) { }
	inline explicit Vector3(const float _x, const float _y, const float _z)
		: m_Value(_x, _y, _z) { }
	inline explicit Vector3(const float* xyz)
		: m_Value(xyz[0], xyz[1], xyz[2]) { }

	inline operator const float*() const { return (const float*)this; }
	inline operator float*() { return (float*)this; }
	inline const Vector4& GetValue() const { return m_Value; }
	inline operator const Vector4() const
	{ return Vector4(m_Value[0], m_Value[1], m_Value[2], 0.0f); }

	inline float& operator[](int i) const { return m_Value[i]; }

	// Math operations.
	friend inline Vector3 operator+=(Vector3& a, const Vector3& b)
	{ a.m_Value = a.m_Value + b.m_Value; return a; }

	friend inline Vector3 operator*=(Vector3& a, float b)
	{ a.m_Value = a.m_Value * b; return a; }

	friend inline Vector3 operator/=(Vector3& a, float b)
	{ a.m_Value = a.m_Value / b; return a; }

	// Other functions.
	inline float Length() const
	{ return m_Value.Length3(); }

	inline float LengthSquared() const
	{ return Dot3(m_Value, m_Value); }

	inline const Vector3& Normalize()
	{ m_Value.Normalize34(); return *this; }

	inline void Abs()
	{ m_Value.Abs34(); }

protected:
	Vector4 m_Value;
};


// ============================================================================
// Operators.

// Comparison.
inline bool operator==(const Vector3& a, const Vector3& b)
{ return Compare3(a.GetValue(), b.GetValue()); }

// Multiply by a scalar.
inline Vector3 operator*(const Vector3& a, float f)
{ return Vector3(Multiply34(a.GetValue(), f)); }

inline Vector3 operator*(float f, const Vector3& a)
{ return Vector3(Multiply34(a.GetValue(), f)); }

// Divide by a scalar.
inline Vector3 operator/(const Vector3& a, float f)
{ return Vector3(Divide34(a.GetValue(), f)); }

inline Vector3 operator/(float f, const Vector3& a)
{ return Vector3(Divide34(a.GetValue(), f)); }

// Add vectors.
inline Vector3 operator+(const Vector3& a, const Vector3& b)
{ return Vector3(Add34(a.GetValue(), b.GetValue())); }

// Subtract vectors.
inline Vector3 operator-(const Vector3& a, const Vector3& b)
{ return Vector3(Subtract34(a.GetValue(), b.GetValue())); }

// Negate.
inline Vector3 operator-(const Vector3& a)
{ return Vector3(Negate34(a.GetValue())); }

// Dot product.
inline float Dot3(const Vector3& a, const Vector3& b)
{ return Dot3(a.GetValue(), b.GetValue()); }

// Cross product.
inline Vector3 Cross3(const Vector3& a, const Vector3& b)
{ return Vector3(Cross3(a.GetValue(), b.GetValue())); }


// ============================================================================
// Quaternion class.

class Quaternion
{
public:
	// Constructors.
	inline Quaternion()
		{ }
	inline explicit Quaternion(const Vector4& _v)
		: m_Value(_v) { }
	inline explicit Quaternion(const float _x, const float _y, const float _z, const float _w)
		: m_Value(_x, _y, _z, _w) { }

	// Get/Set functions.
	inline const float operator[](int i) const { return m_Value[i]; }

	// Conversions.
	inline void FromAxisAngle(const Vector4& AxisAngle)
	{
		float s = sinf(AxisAngle[3] / 2.0f);
		m_Value = Vector4(AxisAngle[0] * s, AxisAngle[1] * s, AxisAngle[2] * s, cosf(AxisAngle[3] / 2.0f));
	}

	inline void CreateRotationX(float Radians)
	{
		m_Value = Vector4(sinf(Radians / 2.0f), 0, 0, cosf(Radians / 2.0f));
	}

	inline void CreateRotationY(float Radians)
	{
		m_Value = Vector4(0, sinf(Radians / 2.0f), 0, cosf(Radians / 2.0f));
	}

	inline void CreateRotationZ(float Radians)
	{
		m_Value = Vector4(0, 0, sinf(Radians / 2.0f), cosf(Radians / 2.0f));
	}

	inline void ToAxisAngle(Vector4& AxisAngle) const
	{
		float Len = m_Value[0]*m_Value[0] + m_Value[1]*m_Value[1] + m_Value[2]*m_Value[2];

		if (Len > 0.0001f)
		{
			float f = 1.0f / sqrtf(Len);
			AxisAngle = Vector4(m_Value[0] * f, m_Value[1] * f, m_Value[2] * f, acosf(m_Value[3]) * 2.0f);
		}
		else
		{
			AxisAngle = Vector4(0, 0, 1, 0);
		}
	}

	// Operators.
	friend inline Quaternion Mul(const Quaternion& a, const Quaternion& b)
	{
    float x =  a.m_Value[0] * b.m_Value[3] + a.m_Value[1] * b.m_Value[2] - a.m_Value[2] * b.m_Value[1] + a.m_Value[3] * b.m_Value[0];
    float y = -a.m_Value[0] * b.m_Value[2] + a.m_Value[1] * b.m_Value[3] + a.m_Value[2] * b.m_Value[0] + a.m_Value[3] * b.m_Value[1];
    float z =  a.m_Value[0] * b.m_Value[1] - a.m_Value[1] * b.m_Value[0] + a.m_Value[2] * b.m_Value[3] + a.m_Value[3] * b.m_Value[2];
    float w = -a.m_Value[0] * b.m_Value[0] - a.m_Value[1] * b.m_Value[1] - a.m_Value[2] * b.m_Value[2] + a.m_Value[3] * b.m_Value[3];

		return Quaternion(x, y, z, w);
	}

	friend inline Vector3 Mul(const Vector3& a, const Quaternion& b)
	{
		// Faster to transform to a matrix and multiply.
		float Tx  = 2.0f*b[0];
		float Ty  = 2.0f*b[1];
		float Tz  = 2.0f*b[2];
		float Twx = Tx*b[3];
		float Twy = Ty*b[3];
		float Twz = Tz*b[3];
		float Txx = Tx*b[0];
		float Txy = Ty*b[0];
		float Txz = Tz*b[0];
		float Tyy = Ty*b[1];
		float Tyz = Tz*b[1];
		float Tzz = Tz*b[2];

		Vector3 Rows[3];
		Rows[0] = Vector3(1.0f-(Tyy+Tzz), Txy+Twz, Txz-Twy);
		Rows[1] = Vector3(Txy-Twz, 1.0f-(Txx+Tzz), Tyz+Twx);
		Rows[2] = Vector3(Txz+Twy, Tyz-Twx, 1.0f-(Txx+Tyy));

		return Vector3(Rows[0].GetValue()*a[0] + Rows[1].GetValue()*a[1] + Rows[2].GetValue()*a[2]);
	}

protected:
	Vector4 m_Value;
};


// ============================================================================
// 3x3 Matrix class.

class Matrix33
{
public:
	// Constructors.
	inline Matrix33()
		{ }
	inline Matrix33(const Vector3& Row0, const Vector3& Row1, const Vector3& Row2)
		{ m_Rows[0] = Row0; m_Rows[1] = Row1; m_Rows[2] = Row2; }

	inline void LoadIdentity()
	{
		m_Rows[0] = Vector3(1.0f, 0.0f, 0.0f);
		m_Rows[1] = Vector3(0.0f, 1.0f, 0.0f);
		m_Rows[2] = Vector3(0.0f, 0.0f, 1.0f);
	}

	inline void CreateFromAxisAngle(const Vector3& Axis, const float Radians)
	{
		float s, c, mag, xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

		s = sinf(Radians);
		c = cosf(Radians);
		mag = Axis.Length();

		if (mag == 0.0f)
		{
			LoadIdentity();
			return;
		}

		Vector3 Normal = Axis * (1.0f / mag);

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

		m_Rows[0] = Vector3((one_c * xx) + c, (one_c * xy) + zs, (one_c * zx) - ys);
		m_Rows[1] = Vector3((one_c * xy) - zs, (one_c * yy) + c, (one_c * yz) + xs);
		m_Rows[2] = Vector3((one_c * zx) + ys, (one_c * yz) - xs, (one_c * zz) + c);
	}

	friend inline Vector3 Mul(const Vector3& a, const Matrix33& b)
	{ return Vector3(b.m_Rows[0]*a[0] + b.m_Rows[1]*a[1] + b.m_Rows[2]*a[2]); }

protected:
	Vector3 m_Rows[3];

	friend class Matrix44;
};

// ============================================================================
// 4x4 Matrix class.

class Matrix44
{
public:
	inline Matrix44()
	{ }
	inline Matrix44(const Vector4& Row0, const Vector4& Row1, const Vector4& Row2, const Vector4& Row3)
	{ m_Rows[0] = Row0; m_Rows[1] = Row1; m_Rows[2] = Row2; m_Rows[3] = Row3; }

	inline operator const float*() const { return (const float*)this; }
	inline const Vector4& operator[](int i) const { return m_Rows[i]; }
	inline Vector4& operator[](int i) { return m_Rows[i]; }

	inline void LoadIdentity()
	{
		m_Rows[0] = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
		m_Rows[1] = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
		m_Rows[2] = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
		m_Rows[3] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	// Math operations.
	friend inline Vector3 Mul31(const Vector3& a, const Matrix44& b)
	{ return Vector3(b.m_Rows[0]*a[0] + b.m_Rows[1]*a[1] + b.m_Rows[2]*a[2] + b.m_Rows[3]); }

	friend inline Vector3 Mul30(const Vector3& a, const Matrix44& b)
	{ return Vector3(b.m_Rows[0]*a[0] + b.m_Rows[1]*a[1] + b.m_Rows[2]*a[2]); }

	friend inline Vector4 Mul4(const Vector4& a, const Matrix44& b)
	{ return Vector4(b.m_Rows[0]*a[0] + b.m_Rows[1]*a[1] + b.m_Rows[2]*a[2] + b.m_Rows[3]*a[3]); }

	friend inline Matrix44 Mul(const Matrix44& a, const Matrix44& b)
	{
		Vector4 Col0(b.m_Rows[0][0], b.m_Rows[1][0], b.m_Rows[2][0], b.m_Rows[3][0]);
		Vector4 Col1(b.m_Rows[0][1], b.m_Rows[1][1], b.m_Rows[2][1], b.m_Rows[3][1]);
		Vector4 Col2(b.m_Rows[0][2], b.m_Rows[1][2], b.m_Rows[2][2], b.m_Rows[3][2]);
		Vector4 Col3(b.m_Rows[0][3], b.m_Rows[1][3], b.m_Rows[2][3], b.m_Rows[3][3]);

		Vector4 Ret0(Dot4(a.m_Rows[0], Col0), Dot4(a.m_Rows[0], Col1), Dot4(a.m_Rows[0], Col2), Dot4(a.m_Rows[0], Col3));
		Vector4 Ret1(Dot4(a.m_Rows[1], Col0), Dot4(a.m_Rows[1], Col1), Dot4(a.m_Rows[1], Col2), Dot4(a.m_Rows[1], Col3));
		Vector4 Ret2(Dot4(a.m_Rows[2], Col0), Dot4(a.m_Rows[2], Col1), Dot4(a.m_Rows[2], Col2), Dot4(a.m_Rows[2], Col3));
		Vector4 Ret3(Dot4(a.m_Rows[3], Col0), Dot4(a.m_Rows[3], Col1), Dot4(a.m_Rows[3], Col2), Dot4(a.m_Rows[3], Col3));

		return Matrix44(Ret0, Ret1, Ret2, Ret3);
	}

	inline Matrix44& operator=(const Matrix33& a)
	{
		m_Rows[0] = Vector4(a.m_Rows[0][0], a.m_Rows[0][1], a.m_Rows[0][2], 0.0f);
		m_Rows[1] = Vector4(a.m_Rows[1][0], a.m_Rows[1][1], a.m_Rows[1][2], 0.0f);
		m_Rows[2] = Vector4(a.m_Rows[2][0], a.m_Rows[2][1], a.m_Rows[2][2], 0.0f);
		m_Rows[3] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		return *this;
	}

	inline void Transpose3()
	{
		Vector4 a = m_Rows[0], b = m_Rows[1], c = m_Rows[2];
		m_Rows[0] = Vector4(a[0], b[0], c[0], a[3]);
		m_Rows[1] = Vector4(a[1], b[1], c[1], b[3]);
		m_Rows[2] = Vector4(a[2], b[2], c[2], c[3]);
	}

	inline void SetTranslation(const Vector3& a)
	{ m_Rows[3] = Vector4(a[0], a[1], a[2], 1.0f); }

	friend Matrix44 Inverse(const Matrix44& m);
	void CreateLookAt(const Vector3& Eye, const Vector3& Target, const Vector3& Up);
	void CreatePerspective(float FoVy, float Aspect, float Near, float Far);
	void CreateOrtho(float Left, float Right, float Bottom, float Top, float Near, float Far);

	void CreateFromAxisAngle(const Vector3& Axis, float Radians)
	{
		Matrix33 Mat;
		Mat.CreateFromAxisAngle(Axis, Radians);
		*this = Mat;
	}

	Vector4 ToAxisAngle()
	{
		Matrix33 tmp(Vector3(m_Rows[0]).Normalize(), Vector3(m_Rows[1]).Normalize(), Vector3(m_Rows[2]).Normalize());

		// Determinant should be 1 for rotation matrices.
		float Determinant = tmp.m_Rows[0][0] * tmp.m_Rows[1][1] * tmp.m_Rows[2][2] + tmp.m_Rows[0][1] * tmp.m_Rows[1][2] * tmp.m_Rows[2][0] +
	                        tmp.m_Rows[0][2] * tmp.m_Rows[1][0] * tmp.m_Rows[2][1] - tmp.m_Rows[0][0] * tmp.m_Rows[1][2] * tmp.m_Rows[2][1] - 
	                        tmp.m_Rows[0][1] * tmp.m_Rows[1][0] * tmp.m_Rows[2][2] - tmp.m_Rows[0][2] * tmp.m_Rows[1][1] * tmp.m_Rows[2][0];

		if (Determinant < 0.0f)
			tmp.m_Rows[0] *= -1.0f;

		float Trace = tmp.m_Rows[0][0] + tmp.m_Rows[1][1] + tmp.m_Rows[2][2];
		float Cos = 0.5f * (Trace - 1.0f);
		Vector4 rot;

		if (Cos < -1.0f)
			Cos = -1.0f;
		else if (Cos > 1.0f)
			Cos = 1.0f;
		rot[3] = acosf(Cos);  // in [0,PI]

		if (rot[3] > 0.01f)
		{
			if (fabsf(3.141592f - rot[3]) > 0.01f)
			{
				rot[0] = tmp.m_Rows[1][2] - tmp.m_Rows[2][1];
				rot[1] = tmp.m_Rows[2][0] - tmp.m_Rows[0][2];
				rot[2] = tmp.m_Rows[0][1] - tmp.m_Rows[1][0];

				float inv = 1.0f / sqrtf(rot[0]*rot[0] + rot[1]*rot[1] + rot[2]*rot[2]);

				rot[0] *= inv;
				rot[1] *= inv;
				rot[2] *= inv;
			}
			else
			{
				// angle is PI
				float HalfInverse;
				if (tmp.m_Rows[0][0] >= tmp.m_Rows[1][1])
				{
					// r00 >= r11
					if (tmp.m_Rows[0][0] >= tmp.m_Rows[2][2])
					{
						// r00 is maximum diagonal term
						rot[0] = 0.5f * sqrtf(tmp.m_Rows[0][0] - tmp.m_Rows[1][1] - tmp.m_Rows[2][2] + 1.0f);
						HalfInverse = 0.5f / rot[0];
						rot[1] = HalfInverse * tmp.m_Rows[1][0];
						rot[2] = HalfInverse * tmp.m_Rows[2][0];
					}
					else
					{
						// r22 is maximum diagonal term
						rot[2] = 0.5f * sqrtf(tmp.m_Rows[2][2] - tmp.m_Rows[0][0] - tmp.m_Rows[1][1] + 1.0f);
						HalfInverse = 0.5f / rot[2];
						rot[0] = HalfInverse * tmp.m_Rows[2][0];
						rot[1] = HalfInverse * tmp.m_Rows[2][1];
					}
				}
				else
				{
					// r11 > r00
					if (tmp.m_Rows[1][1] >= tmp.m_Rows[2][2])
					{
						// r11 is maximum diagonal term
						rot[1] = 0.5f * sqrtf(tmp.m_Rows[1][1] - tmp.m_Rows[0][0] - tmp.m_Rows[2][2] + 1.0f);
						HalfInverse  = 0.5f / rot[1];
						rot[0] = HalfInverse * tmp.m_Rows[1][0];
						rot[2] = HalfInverse * tmp.m_Rows[2][1];
					}
					else
					{
						// r22 is maximum diagonal term
						rot[2] = 0.5f * sqrtf(tmp.m_Rows[2][2] - tmp.m_Rows[0][0] - tmp.m_Rows[1][1] + 1.0f);
						HalfInverse = 0.5f / rot[2];
						rot[0] = HalfInverse * tmp.m_Rows[2][0];
						rot[1] = HalfInverse * tmp.m_Rows[2][1];
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

protected:
	Vector4 m_Rows[4];
};

// ============================================================================
// Linear Algebra Functions.

Vector3 ZoomExtents(const Vector3& Position, const Matrix44& WorldView, const Matrix44& Projection, const Vector3* Points, int NumPoints);

Vector3 ProjectPoint(const Vector3& Point, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4]);
void ProjectPoints(Vector3* Points, int NumPoints, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4]);
Vector3 UnprojectPoint(const Vector3& Point, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4]);
void UnprojectPoints(Vector3* Points, int NumPoints, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4]);

void PolygonPlaneClip(Vector3* InPoints, int NumInPoints, Vector3* OutPoints, int* NumOutPoints, const Vector4& Plane);
bool LinePlaneIntersection(Vector3& Intersection, const Vector3& Start, const Vector3& End, const Vector4& Plane);
bool LineTriangleMinIntersection(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& Start, const Vector3& End, float& MinDist, Vector3& Intersection);
bool LineQuadMinIntersection(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4, const Vector3& Start, const Vector3& End, float& MinDist, Vector3& Intersection);

#endif
