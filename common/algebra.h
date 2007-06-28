#ifndef _ALGEBRA_H_
#define _ALGEBRA_H_

#include <math.h>
#include <float.h>

//
// Simple math library and linear algebra functions.
//
// Everything is based on macros and the Float4 type, so changing them should be enough
// to add support for compiler specific math intrinsics and avoid slowing things down
// when inlining is disabled.
//
// Matrices are represented as row-major, so we pre-multiply instead of post-multiplying
// (like you would in a column major notation).
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
class Matrix33;
class Matrix44;

// ============================================================================
// Float version.

#ifdef LC_MATH_FLOAT

// Vector Initialization.
#define lcBaseVector_Set3(res, a, b, c) { res.x = a; res.y = b; res.z = c; }
#define lcBaseVector_Set4(res, a, b, c, d) { res.x = a; res.y = b; res.z = c; res.w = d; }
#define lcBaseVector_Copy3(res, a) { res.x = a.x; res.y = a.y; res.z = a.z; }
#define lcBaseVector_Copy4(res, a) { res.x = a.x; res.y = a.y; res.z = a.z; res.w = a.w; }

#define lcBaseVector_SetX(res, a, b) { res.x = b; }
#define lcBaseVector_SetY(res, a, b) { res.y = b; }
#define lcBaseVector_SetZ(res, a, b) { res.z = b; }
#define lcBaseVector_SetW(res, a, b) { res.w = b; }

// Vector Comparisons.
#define lcBaseVector_Equal3(res, a, b) { res = ((a.x == b.x) && (a.y == b.y) && (a.z == b.z)); }
#define lcBaseVector_Equal4(res, a, b) { res = ((a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w)); }

// Vector Arithmetic operations.
#define lcBaseVector_Neg3(res, a) { res.x = -a.x; res.y = -a.y; res.z = -a.z; }
#define lcBaseVector_Neg4(res, a) { res.x = -a.x; res.y = -a.y; res.z = -a.z; res.w = -a.w; }

#define lcBaseVector_Add3(res, a, b) { res.x = a.x + b.x; res.y = a.y + b.y; res.z = a.z + b.z; }
#define lcBaseVector_Add4(res, a, b) { res.x = a.x + b.x; res.y = a.y + b.y; res.z = a.z + b.z; res.w = a.w + b.w; }
#define lcBaseVector_Sub3(res, a, b) { res.x = a.x - b.x; res.y = a.y - b.y; res.z = a.z - b.z; }
#define lcBaseVector_Sub4(res, a, b) { res.x = a.x - b.x; res.y = a.y - b.y; res.z = a.z - b.z; res.w = a.w - b.w; }

#define lcBaseVector_Mul3(res, a, b) { res.x = a.x * b.x; res.y = a.y * b.y; res.z = a.z * b.z; }
#define lcBaseVector_Mul4(res, a, b) { res.x = a.x * b.x; res.y = a.y * b.y; res.z = a.z * b.z; res.w = a.w * b.w; }
#define lcBaseVector_Div3(res, a, b) { res.x = a.x / b.x; res.y = a.y / b.y; res.z = a.z / b.z; }
#define lcBaseVector_Div4(res, a, b) { res.x = a.x / b.x; res.y = a.y / b.y; res.z = a.z / b.z; res.w = a.w / b.w; }

#define lcBaseVector_Mul3f(res, a, b) { res.x = a.x * b; res.y = a.y * b; res.z = a.z * b; }
#define lcBaseVector_Mul4f(res, a, b) { res.x = a.x * b; res.y = a.y * b; res.z = a.z * b; res.w = a.w * b; }
#define lcBaseVector_Div3f(res, a, b) { res.x = a.x / b; res.y = a.y / b; res.z = a.z / b; }
#define lcBaseVector_Div4f(res, a, b) { res.x = a.x / b; res.y = a.y / b; res.z = a.z / b; res.w = a.w / b; }

#define lcBaseVector_Dot3(res, a, b) { res = a.x * b.x + a.y * b.y + a.z * b.z; }
#define lcBaseVector_Dot4(res, a, b) { res = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
#define lcBaseVector_Cross3(res, a, b) { res.x = a.y * b.z - a.z * b.y; res.y = a.z * b.x - a.x * b.z; res.z = a.x * b.y - a.y * b.x; }

#define lcBaseVector_Abs3(res, a) { res.x = fabsf(a.x); res.y = fabsf(a.y); res.z = fabsf(a.z); }
#define lcBaseVector_Abs4(res, a) { res.x = fabsf(a.x); res.y = fabsf(a.y); res.z = fabsf(a.z); res.w = fabsf(a.w); }

#define lcBaseVector_Min3(res, a, b) { res.x = a.x < b.x ? a.x : b.x; res.y = a.y < b.y ? a.y : b.y; res.z = a.z < b.z ? a.z : b.z; }
#define lcBaseVector_Min4(res, a, b) { res.x = a.x < b.x ? a.x : b.x; res.y = a.y < b.y ? a.y : b.y; res.z = a.z < b.z ? a.z : b.z; res.w = a.w < b.w ? a.w : b.w; }

#define lcBaseVector_Max3(res, a, b) { res.x = a.x > b.x ? a.x : b.x; res.y = a.y > b.y ? a.y : b.y; res.z = a.z > b.z ? a.z : b.z; }
#define lcBaseVector_Max4(res, a, b) { res.x = a.x > b.x ? a.x : b.x; res.y = a.y > b.y ? a.y : b.y; res.z = a.z > b.z ? a.z : b.z; res.w = a.w > b.w ? a.w : b.w; }

#define lcBaseVector_Length3(res, a) { res = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }
#define lcBaseVector_Length4(res, a) { res = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w); }

// Base class.
struct lcBaseVector
{
	float x, y, z, w;
};

#endif

// ============================================================================
// SSE version.

#ifdef LC_MATH_SSE

// If you can't find this file you need to install the VS6 Processor Pack.
#include <xmmintrin.h>

// Vector Initialization.
#define lcBaseVector_Set3(res, a, b, c) { res.xyzw = _mm_setr_ps(a, b, c, 0.0f); }
#define lcBaseVector_Set4(res, a, b, c, d) { res.xyzw = _mm_setr_ps(a, b, c, d); }
#define lcBaseVector_Copy3(res, a) { res = a; }
#define lcBaseVector_Copy4(res, a) { res = a; }

#define lcBaseVector_SetX(res, a, b) \
{ \
	__m128 xxyy = _mm_shuffle_ps(_mm_load_ps1(&b), a.xyzw, _MM_SHUFFLE(1, 1, 0, 0)); \
	res.xyzw = _mm_shuffle_ps(xxyy, a.xyzw, _MM_SHUFFLE(3, 2, 2, 0)); \
}

#define lcBaseVector_SetY(res, a, b) \
{ \
	__m128 xxyy = _mm_shuffle_ps(a.xyzw, _mm_load_ps1(&b), _MM_SHUFFLE(1, 1, 0, 0)); \
	res.xyzw = _mm_shuffle_ps(xxyy, a.xyzw, _MM_SHUFFLE(3, 2, 2, 0)); \
}

#define lcBaseVector_SetZ(res, a, b) \
{ \
	__m128 zzww = _mm_shuffle_ps(_mm_load_ps1(&b), a.xyzw, _MM_SHUFFLE(3, 3, 2, 2)); \
	res.xyzw = _mm_shuffle_ps(a.xyzw, zzww, _MM_SHUFFLE(3, 0, 1, 0)); \
}

#define lcBaseVector_SetW(res, a, b) \
{ \
	__m128 zzww = _mm_shuffle_ps(a.xyzw, _mm_load_ps1(&b), _MM_SHUFFLE(3, 3, 2, 2)); \
	res.xyzw = _mm_shuffle_ps(a.xyzw, zzww, _MM_SHUFFLE(3, 0, 1, 0)); \
}

// Vector Comparisons.
#define lcBaseVector_Equal3(res, a, b) { res = (_mm_movemask_ps(_mm_cmpeq_ps(a.xyzw, b.xyzw)) & 0x7) == 0x7; }
#define lcBaseVector_Equal4(res, a, b) { res = !_mm_movemask_ps(_mm_cmpneq_ps(a.xyzw, b.xyzw)); }

// Vector Arithmetic operations.
#define lcBaseVector_Neg3(res, a) \
{ \
	const __declspec(align(16)) unsigned int Mask[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 }; \
	res.xyzw = _mm_xor_ps(a.xyzw, *(__m128*)&Mask); \
}

#define lcBaseVector_Neg4(res, a) \
{ \
	const __declspec(align(16)) unsigned int Mask[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 }; \
	res.xyzw = _mm_xor_ps(a.xyzw, *(__m128*)&Mask); \
}

#define lcBaseVector_Add3(res, a, b) { res.xyzw = _mm_add_ps(a.xyzw, b.xyzw); }
#define lcBaseVector_Add4(res, a, b) { res.xyzw = _mm_add_ps(a.xyzw, b.xyzw); }
#define lcBaseVector_Sub3(res, a, b) { res.xyzw = _mm_sub_ps(a.xyzw, b.xyzw); }
#define lcBaseVector_Sub4(res, a, b) { res.xyzw = _mm_sub_ps(a.xyzw, b.xyzw); }

#define lcBaseVector_Mul3(res, a, b) { res.xyzw = _mm_mul_ps(a.xyzw, b.xyzw); }
#define lcBaseVector_Mul4(res, a, b) { res.xyzw = _mm_mul_ps(a.xyzw, b.xyzw); }
#define lcBaseVector_Div3(res, a, b) { res.xyzw = _mm_div_ps(a.xyzw, b.xyzw); }
#define lcBaseVector_Div4(res, a, b) { res.xyzw = _mm_div_ps(a.xyzw, b.xyzw); }

#define lcBaseVector_Mul3f(res, a, b) { res.xyzw = _mm_mul_ps(a.xyzw, _mm_load_ps1(&b)); }
#define lcBaseVector_Mul4f(res, a, b) { res.xyzw = _mm_mul_ps(a.xyzw, _mm_load_ps1(&b)); }
#define lcBaseVector_Div3f(res, a, b) { res.xyzw = _mm_div_ps(a.xyzw, _mm_load_ps1(&b)); }
#define lcBaseVector_Div4f(res, a, b) { res.xyzw = _mm_div_ps(a.xyzw, _mm_load_ps1(&b)); }

#define lcBaseVector_Dot3(res, a, b) \
{ \
	__m128 tmp = _mm_mul_ps(a.xyzw, b.xyzw); \
	__m128 yz = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 2, 2, 2))); \
	tmp = _mm_add_ss(tmp, yz); \
	res = *(const float*)&tmp; \
}

#define lcBaseVector_Dot4(res, a, b) \
{ \
	__m128 tmp = _mm_mul_ps(a.xyzw, b.xyzw); \
	__m128 xy = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(0, 0, 0, 0)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 1, 1, 1))); \
	__m128 zw = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(3, 3, 3, 3))); \
	tmp = _mm_add_ss(xy, zw); \
	res = *(const float*)&tmp; \
}

// a(yzx)*b(zxy)-a(zxy)*b(yzx)
#define lcBaseVector_Cross3(res, a, b) \
{ \
	__m128 r1 = _mm_mul_ps(_mm_shuffle_ps(a.xyzw, a.xyzw, _MM_SHUFFLE(0, 0, 2, 1)), _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(0, 1, 0, 2))); \
	__m128 r2 = _mm_mul_ps(_mm_shuffle_ps(a.xyzw, a.xyzw, _MM_SHUFFLE(0, 1, 0, 2)), _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(0, 0, 2, 1))); \
	res.xyzw = _mm_sub_ps(r1, r2); \
}

#define lcBaseVector_Abs3(res, a) \
{ \
	const __declspec(align(16)) unsigned int Mask[4] = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff }; \
	res.xyzw = _mm_and_ps(a.xyzw, *(__m128*)&Mask); \
}

#define lcBaseVector_Abs4(res, a) \
{ \
	const __declspec(align(16)) unsigned int Mask[4] = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff }; \
	res.xyzw = _mm_and_ps(a.xyzw, *(__m128*)&Mask); \
}

#define lcBaseVector_Length3(res, a) \
{ \
	__m128 tmp = _mm_mul_ps(a.xyzw, a.xyzw); \
	__m128 yz = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 2, 2, 2))); \
	tmp = _mm_add_ss(tmp, yz); \
	tmp = _mm_sqrt_ss(tmp); \
	res = *(const float*)&tmp; \
}

#define lcBaseVector_Length4(res, a) \
{ \
	__m128 tmp = _mm_mul_ps(a.xyzw, a.xyzw); \
	__m128 xy = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(0, 0, 0, 0)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 1, 1, 1))); \
	__m128 zw = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(3, 3, 3, 3))); \
	tmp = _mm_add_ss(xy, zw); \
	tmp = _mm_sqrt_ss(tmp); \
	res = *(const float*)&tmp; \
}

// Base class.
struct __declspec(align(16)) lcBaseVector
{
	__m128 xyzw;
};

/*
inline void Normalize34()
{
	__m128 tmp = _mm_mul_ps(xyzw, xyzw);
	__m128 yz = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 2, 2, 2)));
	tmp = _mm_add_ss(tmp, yz);
	tmp = _mm_rsqrt_ss(tmp);
	tmp = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(0, 0, 0, 0));
	xyzw = _mm_mul_ps(xyzw, tmp);
}
*/
#endif

// ============================================================================
// Vector classes.

class Vector4
{
public:
	inline Vector4();
	inline explicit Vector4(const float x, const float y, const float z);
	inline explicit Vector4(const float x, const float y, const float z, const float w);
	inline explicit Vector4(const Vector3& a);
	inline explicit Vector4(const Vector3& a, const float w);

	inline operator const float*() const { return (const float*)this; }
	inline operator float*() const { return (float*)this; }
	inline float& operator[](int i) const { return ((float*)this)[i]; }

	inline float Length() const;
	inline float LengthSquared() const;
	inline const Vector4& Normalize();

	lcBaseVector v;
};

class Vector3
{
public:
	inline Vector3();
	inline explicit Vector3(const float x, const float y, const float z);
	inline explicit Vector3(const Vector4& a);

	inline operator const float*() const { return (const float*)this; }
	inline operator float*() { return (float*)this; }
	inline float& operator[](int i) const { return ((float*)this)[i]; }

	inline float Length() const;
	inline float LengthSquared() const;
	inline const Vector3& Normalize();

	lcBaseVector v;
};

// ============================================================================
// Quaternion class.

class Quaternion
{
public:
	inline Quaternion();
	inline explicit Quaternion(const float x, const float y, const float z, const float w);

	inline operator const float*() const { return (const float*)this; }
	inline operator float*() { return (float*)this; }
	inline float& operator[](int i) const { return ((float*)this)[i]; }

	lcBaseVector v;
};

// ============================================================================
// 3x3 Matrix class.

class Matrix33
{
public:
	// Constructors.
	inline Matrix33();
	inline Matrix33(const Vector3& Row0, const Vector3& Row1, const Vector3& Row2);
	inline Matrix33(const Matrix44& a);

	inline operator const float*() const { return (const float*)this; }
	inline operator float*() { return (float*)this; }
	inline const Vector3& operator[](int i) const { return m_Rows[i]; }
	inline Vector3& operator[](int i) { return m_Rows[i]; }

	inline Matrix33& operator=(const Matrix33& a)
	{
		m_Rows[0] = Vector3(a.m_Rows[0][0], a.m_Rows[0][1], a.m_Rows[0][2]);
		m_Rows[1] = Vector3(a.m_Rows[1][0], a.m_Rows[1][1], a.m_Rows[1][2]);
		m_Rows[2] = Vector3(a.m_Rows[2][0], a.m_Rows[2][1], a.m_Rows[2][2]);
		return *this;
	}

	inline float Determinant() const;

	Vector3 m_Rows[3];
};

// ============================================================================
// 4x4 Matrix class.

class Matrix44
{
public:
	inline Matrix44();
	inline Matrix44(const Vector4& Row0, const Vector4& Row1, const Vector4& Row2, const Vector4& Row3);
	inline Matrix44(const Matrix33& a);

	inline operator const float*() const { return (const float*)this; }
	inline operator float*() const { return (float*)this; }
	inline const Vector4& operator[](int i) const { return m_Rows[i]; }
	inline Vector4& operator[](int i) { return m_Rows[i]; }

	inline Matrix44& operator=(const Matrix33& a)
	{
		m_Rows[0] = Vector4(a.m_Rows[0][0], a.m_Rows[0][1], a.m_Rows[0][2], 0.0f);
		m_Rows[1] = Vector4(a.m_Rows[1][0], a.m_Rows[1][1], a.m_Rows[1][2], 0.0f);
		m_Rows[2] = Vector4(a.m_Rows[2][0], a.m_Rows[2][1], a.m_Rows[2][2], 0.0f);
		m_Rows[3] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		return *this;
	}

	inline void SetTranslation(const Vector3& a)
	{ m_Rows[3] = Vector4(a[0], a[1], a[2], 1.0f); }

	inline Vector3 GetTranslation() const
	{ return Vector3(m_Rows[3]); }

	Vector4 m_Rows[4];
};

// ============================================================================
// Vector Functions

// Vector constuctors.
inline Vector4::Vector4()
{ }

inline Vector4::Vector4(const float x, const float y, const float z)
{ lcBaseVector_Set3(v, x, y, z); }

inline Vector4::Vector4(const float x, const float y, const float z, const float w)
{ lcBaseVector_Set4(v, x, y, z, w); }

inline Vector4::Vector4(const Vector3& a)
{ lcBaseVector_Copy3(v, a.v); }

inline Vector4::Vector4(const Vector3& a, const float w)
{ lcBaseVector_Copy3(v, a.v); lcBaseVector_SetW(v, v, w); }

inline Vector3::Vector3()
{ }

inline Vector3::Vector3(const float x, const float y, const float z)
{ lcBaseVector_Set3(v, x, y, z); }

inline Vector3::Vector3(const Vector4& a)
{ lcBaseVector_Copy3(v, a.v); }

// Vector comparisons.
inline bool operator==(const Vector4& a, const Vector4& b)
{ bool res; lcBaseVector_Equal4(res, a.v, b.v); return res; }

inline bool operator==(const Vector3& a, const Vector3& b)
{ bool res; lcBaseVector_Equal3(res, a.v, b.v); return res; }

// Vector-Vector operations.
inline Vector4 operator+(const Vector4& a, const Vector4& b)
{ Vector4 res; lcBaseVector_Add4(res.v, a.v, b.v); return res; }

inline const Vector4& operator+=(Vector4& a, const Vector4& b)
{ lcBaseVector_Add4(a.v, a.v, b.v); return a; }

inline Vector4 operator-(const Vector4& a, const Vector4& b)
{ Vector4 res; lcBaseVector_Sub4(res.v, a.v, b.v); return res; }

inline const Vector4& operator-=(Vector4& a, const Vector4& b)
{ lcBaseVector_Sub4(a.v, a.v, b.v); return a; }

inline Vector4 operator*(const Vector4& a, const Vector4& b)
{ Vector4 res; lcBaseVector_Mul4(res.v, a.v, b.v); return res; }

inline Vector4 operator/(const Vector4& a, const Vector4& b)
{ Vector4 res; lcBaseVector_Div4(res.v, a.v, b.v); return res; }

inline Vector4 operator-(const Vector4& a)
{ Vector4 res; lcBaseVector_Neg4(res.v, a.v); return res; }

inline Vector3 operator+(const Vector3& a, const Vector3& b)
{ Vector3 res; lcBaseVector_Add3(res.v, a.v, b.v); return res; }

inline const Vector3& operator+=(Vector3& a, const Vector3& b)
{ lcBaseVector_Add3(a.v, a.v, b.v); return a; }

inline Vector3 operator-(const Vector3& a, const Vector3& b)
{ Vector3 res; lcBaseVector_Sub3(res.v, a.v, b.v); return res; }

inline const Vector3& operator-=(Vector3& a, const Vector3& b)
{ lcBaseVector_Sub3(a.v, a.v, b.v); return a; }

inline Vector3 operator*(const Vector3& a, const Vector3& b)
{ Vector3 res; lcBaseVector_Mul3(res.v, a.v, b.v); return res; }

inline Vector3 operator/(const Vector3& a, const Vector3& b)
{ Vector3 res; lcBaseVector_Div3(res.v, a.v, b.v); return res; }

inline Vector3 operator-(const Vector3& a)
{ Vector3 res; lcBaseVector_Neg3(res.v, a.v); return res; }

// Vector-Scalar operations.
inline Vector4 operator*(const Vector4& a, float b)
{ Vector4 res; lcBaseVector_Mul4f(res.v, a.v, b); return res; }

inline Vector4 operator*(float a, const Vector4& b)
{ Vector4 res; lcBaseVector_Mul4f(res.v, b.v, a); return res; }

inline Vector4 operator*=(Vector4& a, float b)
{ lcBaseVector_Mul4f(a.v, a.v, b); return a; }

inline Vector4 operator/(const Vector4& a, float b)
{ Vector4 res; lcBaseVector_Div4f(res.v, a.v, b); return res; }

inline Vector4 operator/(float a, const Vector4& b)
{ Vector4 res; lcBaseVector_Div4f(res.v, b.v, a); return res; }

inline Vector4 operator/=(Vector4& a, float b)
{ lcBaseVector_Div4f(a.v, a.v, b); return a; }

inline Vector3 operator*(const Vector3& a, float b)
{ Vector3 res; lcBaseVector_Mul3f(res.v, a.v, b); return res; }

inline Vector3 operator*(float a, const Vector3& b)
{ Vector3 res; lcBaseVector_Mul3f(res.v, b.v, a); return res; }

inline Vector3 operator*=(Vector3& a, float b)
{ lcBaseVector_Mul3f(a.v, a.v, b); return a; }

inline Vector3 operator/(const Vector3& a, float b)
{ Vector3 res; lcBaseVector_Div3f(res.v, a.v, b); return res; }

inline Vector3 operator/(float a, const Vector3& b)
{ Vector3 res; lcBaseVector_Div3f(res.v, b.v, a); return res; }

inline Vector3 operator/=(Vector3& a, float b)
{ lcBaseVector_Div3f(a.v, a.v, b); return a; }

// Dot product.
inline float Dot4(const Vector4& a, const Vector4& b)
{ float res; lcBaseVector_Dot4(res, a.v, b.v); return res; }

inline float Dot3(const Vector4& a, const Vector4& b)
{ float res; lcBaseVector_Dot3(res, a.v, b.v); return res; }

inline float Dot3(const Vector3& a, const Vector4& b)
{ float res; lcBaseVector_Dot3(res, a.v, b.v); return res; }

inline float Dot3(const Vector4& a, const Vector3& b)
{ float res; lcBaseVector_Dot3(res, a.v, b.v); return res; }

inline float Dot3(const Vector3& a, const Vector3& b)
{ float res; lcBaseVector_Dot3(res, a.v, b.v); return res; }

// Cross product.
inline Vector3 Cross3(const Vector4& a, const Vector4& b)
{ Vector3 res; lcBaseVector_Cross3(res.v, a.v, b.v); return res; }

inline Vector3 Cross3(const Vector3& a, const Vector4& b)
{ Vector3 res; lcBaseVector_Cross3(res.v, a.v, b.v); return res; }

inline Vector3 Cross3(const Vector4& a, const Vector3& b)
{ Vector3 res; lcBaseVector_Cross3(res.v, a.v, b.v); return res; }

inline Vector3 Cross3(const Vector3& a, const Vector3& b)
{ Vector3 res; lcBaseVector_Cross3(res.v, a.v, b.v); return res; }

// Other functions.
inline Vector4 Normalize(const Vector4& a)
{ float len; lcBaseVector_Length4(len, a.v); return a / len; }

inline Vector3 Normalize(const Vector3& a)
{ float len; lcBaseVector_Length3(len, a.v); return a / len; }

inline const Vector4& Vector4::Normalize()
{ *this = ::Normalize(*this); return *this; }

inline const Vector3& Vector3::Normalize()
{ *this = ::Normalize(*this); return *this; }

inline float Length(const Vector4& a)
{ float res; lcBaseVector_Length4(res, a.v); return res; }

inline float Length3(const Vector4& a)
{ float res; lcBaseVector_Length3(res, a.v); return res; }

inline float Length(const Vector3& a)
{ float res; lcBaseVector_Length3(res, a.v); return res; }

inline float Vector4::Length() const
{ return ::Length(*this); }

inline float Vector3::Length() const
{ return ::Length(*this); }

inline float LengthSquared(const Vector4& a)
{ float res; lcBaseVector_Dot4(res, a.v, a.v); return res; }

inline float LengthSquared(const Vector3& a)
{ float res; lcBaseVector_Dot3(res, a.v, a.v); return res; }

inline float Vector4::LengthSquared() const
{ return ::LengthSquared(*this); }

inline float Vector3::LengthSquared() const
{ return ::LengthSquared(*this); }

inline Vector4 Abs(const Vector4& a)
{ Vector4 res; lcBaseVector_Abs4(res.v, a.v); return res; }

inline Vector3 Abs(const Vector3& a)
{ Vector3 res; lcBaseVector_Abs3(res.v, a.v); return res; }

inline Vector4 Min(const Vector4& a, const Vector4& b)
{ Vector4 res; lcBaseVector_Min4(res.v, a.v, b.v); return res; }

inline Vector3 Min(const Vector3& a, const Vector3& b)
{ Vector3 res; lcBaseVector_Min3(res.v, a.v, b.v); return res; }

inline Vector4 Max(const Vector4& a, const Vector4& b)
{ Vector4 res; lcBaseVector_Max4(res.v, a.v, b.v); return res; }

inline Vector3 Max(const Vector3& a, const Vector3& b)
{ Vector3 res; lcBaseVector_Max3(res.v, a.v, b.v); return res; }

// ============================================================================
// Quaternion functions.

// Constructors.
inline Quaternion::Quaternion()
{ }

inline Quaternion::Quaternion(const float x, const float y, const float z, const float w)
{ lcBaseVector_Set4(v, x, y, z, w); }

// Creation.
inline Quaternion CreateRotationXQuaternion(float Radians)
{ return Quaternion(sinf(Radians / 2.0f), 0, 0, cosf(Radians / 2.0f)); }

inline Quaternion CreateRotationYQuaternion(float Radians)
{ return Quaternion(0, sinf(Radians / 2.0f), 0, cosf(Radians / 2.0f)); }

inline Quaternion CreateRotationZQuaternion(float Radians)
{ return Quaternion(0, 0, sinf(Radians / 2.0f), cosf(Radians / 2.0f)); }

// Conversions.
inline Quaternion QuaternionFromAxisAngle(const Vector4& a)
{
	float s = sinf(a[3] / 2.0f);
	return Quaternion(a[0] * s, a[1] * s, a[2] * s, cosf(a[3] / 2.0f));
}

inline Vector4 QuaternionToAxisAngle(const Quaternion& a)
{
	float Len;
	lcBaseVector_Dot3(Len, a.v, a.v);

	if (Len > 0.00001f)
	{
		float f = 1.0f / sqrtf(Len);
		return Vector4(a[0] * f, a[1] * f, a[2] * f, acosf(a[3]) * 2.0f);
	}
	else
	{
		return Vector4(0, 0, 1, 0);
	}
}

// Quaternion operations.
inline Quaternion Mul(const Quaternion& a, const Quaternion& b)
{
  float x =  a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0];
  float y = -a[0] * b[2] + a[1] * b[3] + a[2] * b[0] + a[3] * b[1];
  float z =  a[0] * b[1] - a[1] * b[0] + a[2] * b[3] + a[3] * b[2];
  float w = -a[0] * b[0] - a[1] * b[1] - a[2] * b[2] + a[3] * b[3];

	return Quaternion(x, y, z, w);
}

inline Vector3 Mul(const Vector3& a, const Quaternion& b)
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

	return Vector3(Rows[0]*a[0] + Rows[1]*a[1] + Rows[2]*a[2]);
}

// ============================================================================
// 3x3 Matrix functions.

// Constructors.
inline Matrix33::Matrix33()
{ }

inline Matrix33::Matrix33(const Vector3& Row0, const Vector3& Row1, const Vector3& Row2)
{ m_Rows[0] = Row0; m_Rows[1] = Row1; m_Rows[2] = Row2; }

inline Matrix33::Matrix33(const Matrix44& a)
{
	m_Rows[0] = Vector3(a.m_Rows[0][0], a.m_Rows[0][1], a.m_Rows[0][2]);
	m_Rows[1] = Vector3(a.m_Rows[1][0], a.m_Rows[1][1], a.m_Rows[1][2]);
	m_Rows[2] = Vector3(a.m_Rows[2][0], a.m_Rows[2][1], a.m_Rows[2][2]);
}

inline Matrix33 IdentityMatrix33()
{ return Matrix33(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f)); }

// Conversions.
Matrix33 MatrixFromAxisAngle(const Vector4& AxisAngle);
inline Matrix33 MatrixFromAxisAngle(const Vector3& Axis, float Radians)
{
	return MatrixFromAxisAngle(Vector4(Axis, Radians));
}

Vector4 MatrixToAxisAngle(const Matrix33& Mat);
Matrix33 MatrixFromEulerAngles(const Vector3& Angles);
Vector3 MatrixToEulerAngles(const Matrix33& a);

// Math operations.
inline Vector3 Mul(const Vector3& a, const Matrix33& b)
{ return Vector3(b.m_Rows[0]*a[0] + b.m_Rows[1]*a[1] + b.m_Rows[2]*a[2]); }

inline Matrix33 Mul(const Matrix33& a, const Matrix33& b)
{
	Vector3 Col0(b.m_Rows[0][0], b.m_Rows[1][0], b.m_Rows[2][0]);
	Vector3 Col1(b.m_Rows[0][1], b.m_Rows[1][1], b.m_Rows[2][1]);
	Vector3 Col2(b.m_Rows[0][2], b.m_Rows[1][2], b.m_Rows[2][2]);

	Vector3 Ret0(Dot3(a.m_Rows[0], Col0), Dot3(a.m_Rows[0], Col1), Dot3(a.m_Rows[0], Col2));
	Vector3 Ret1(Dot3(a.m_Rows[1], Col0), Dot3(a.m_Rows[1], Col1), Dot3(a.m_Rows[1], Col2));
	Vector3 Ret2(Dot3(a.m_Rows[2], Col0), Dot3(a.m_Rows[2], Col1), Dot3(a.m_Rows[2], Col2));

	return Matrix33(Ret0, Ret1, Ret2);
}

inline float Determinant(const Matrix33& a)
{
	return a.m_Rows[0][0] * a.m_Rows[1][1] * a.m_Rows[2][2] + a.m_Rows[0][1] * a.m_Rows[1][2] * a.m_Rows[2][0] +
	       a.m_Rows[0][2] * a.m_Rows[1][0] * a.m_Rows[2][1] - a.m_Rows[0][0] * a.m_Rows[1][2] * a.m_Rows[2][1] - 
	       a.m_Rows[0][1] * a.m_Rows[1][0] * a.m_Rows[2][2] - a.m_Rows[0][2] * a.m_Rows[1][1] * a.m_Rows[2][0];
}

inline Matrix33 Transpose(const Matrix33& m)
{
	Vector3 a(m.m_Rows[0][0], m.m_Rows[1][0], m.m_Rows[2][0]);
	Vector3 b(m.m_Rows[0][1], m.m_Rows[1][1], m.m_Rows[2][1]);
	Vector3 c(m.m_Rows[0][2], m.m_Rows[1][2], m.m_Rows[2][2]);
	return Matrix33(a, b, c);
}

inline float Matrix33::Determinant() const
{ return ::Determinant(*this); }

// ============================================================================
// 4x4 Matrix functions.

// Constructors.
inline Matrix44::Matrix44()
{ }

inline Matrix44::Matrix44(const Vector4& Row0, const Vector4& Row1, const Vector4& Row2, const Vector4& Row3)
{ m_Rows[0] = Row0; m_Rows[1] = Row1; m_Rows[2] = Row2; m_Rows[3] = Row3; }

inline Matrix44 IdentityMatrix44()
{
	return Matrix44(Vector4(1.0f, 0.0f, 0.0f, 0.0f), Vector4(0.0f, 1.0f, 0.0f, 0.0f),
	                Vector4(0.0f, 0.0f, 1.0f, 0.0f), Vector4(0.0f, 0.0f, 0.0f, 1.0f));
}

inline Matrix44::Matrix44(const Matrix33& a)
{ *this = a; }

// Math operations.
inline Vector3 Mul31(const Vector3& a, const Matrix44& b)
{ return Vector3(b.m_Rows[0]*a[0] + b.m_Rows[1]*a[1] + b.m_Rows[2]*a[2] + b.m_Rows[3]); }

inline Vector3 Mul30(const Vector3& a, const Matrix44& b)
{ return Vector3(b.m_Rows[0]*a[0] + b.m_Rows[1]*a[1] + b.m_Rows[2]*a[2]); }

inline Vector4 Mul30(const Vector4& a, const Matrix44& b)
{ return Vector4(b.m_Rows[0]*a[0] + b.m_Rows[1]*a[1] + b.m_Rows[2]*a[2]); }

inline Vector4 Mul4(const Vector4& a, const Matrix44& b)
{ return Vector4(b.m_Rows[0]*a[0] + b.m_Rows[1]*a[1] + b.m_Rows[2]*a[2] + b.m_Rows[3]*a[3]); }

inline Matrix44 Mul(const Matrix44& a, const Matrix44& b)
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

inline Matrix44 Transpose3(const Matrix44& m)
{
	Vector4 a(m.m_Rows[0][0], m.m_Rows[1][0], m.m_Rows[2][0], m.m_Rows[0][3]);
	Vector4 b(m.m_Rows[0][1], m.m_Rows[1][1], m.m_Rows[2][1], m.m_Rows[1][3]);
	Vector4 c(m.m_Rows[0][2], m.m_Rows[1][2], m.m_Rows[2][2], m.m_Rows[2][3]);
	return Matrix44(a, b, c, m.m_Rows[3]);
}

Matrix44 Inverse(const Matrix44& m);
Matrix44 RotTranInverse(const Matrix44& m);
Matrix44 CreateLookAtMatrix(const Vector3& Eye, const Vector3& Target, const Vector3& Up);
Matrix44 CreatePerspectiveMatrix(float FoVy, float Aspect, float Near, float Far);
Matrix44 CreateOrthoMatrix44(float Left, float Right, float Bottom, float Top, float Near, float Far);

// ============================================================================
// Axis Aligned Bounding Box.

class BoundingBox
{
public:
	BoundingBox()
	{ }

	BoundingBox(const Vector3& Min, const Vector3& Max)
		: m_Min(Min), m_Max(Max)
	{ }

	void Reset()
	{
		m_Min = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
		m_Max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	}

	Vector3 GetCenter() const
	{
		return (m_Min + m_Max) * 0.5f;
	}

	void GetPoints(Vector3* Points) const
	{
		Points[0] = Vector3(m_Max[0], m_Max[1], m_Min[2]);
		Points[1] = Vector3(m_Min[0], m_Max[1], m_Min[5]);
		Points[2] = Vector3(m_Max[0], m_Max[1], m_Max[2]);
		Points[3] = Vector3(m_Min[0], m_Min[1], m_Min[2]);
		Points[4] = Vector3(m_Min[0], m_Min[1], m_Max[2]);
		Points[5] = Vector3(m_Max[0], m_Min[1], m_Max[2]);
		Points[6] = Vector3(m_Max[0], m_Min[1], m_Min[2]);
		Points[7] = Vector3(m_Min[0], m_Max[1], m_Max[2]);
	}

	void AddPoint(const Vector3& Point)
	{
		m_Min = Min(m_Min, Point);
		m_Max = Max(m_Max, Point);
	}

public:
	Vector3 m_Min;
	Vector3 m_Max;
};

inline BoundingBox operator+(const BoundingBox& a, const BoundingBox& b)
{
	return BoundingBox(Min(a.m_Min, b.m_Min), Max(a.m_Max, b.m_Max));
}

inline const BoundingBox& operator+=(BoundingBox& a, const BoundingBox& b)
{
	a = BoundingBox(Min(a.m_Min, b.m_Min), Max(a.m_Max, b.m_Max));
	return a;
}

// ============================================================================
// Linear Algebra Functions.

Vector3 ZoomExtents(const Vector3& Position, const Matrix44& WorldView, const Matrix44& Projection, const Vector3* Points, int NumPoints);
void GetFrustumPlanes(const Matrix44& WorldView, const Matrix44& Projection, Vector4 Planes[6]);

Vector3 ProjectPoint(const Vector3& Point, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4]);
void ProjectPoints(Vector3* Points, int NumPoints, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4]);
Vector3 UnprojectPoint(const Vector3& Point, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4]);
void UnprojectPoints(Vector3* Points, int NumPoints, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4]);

void PolygonPlaneClip(Vector3* InPoints, int NumInPoints, Vector3* OutPoints, int* NumOutPoints, const Vector4& Plane);
bool PolygonIntersectsPlanes(float* p1, float* p2, float* p3, float* p4, const Vector4* Planes, int NumPlanes);
bool LinePlaneIntersection(Vector3* Intersection, const Vector3& Start, const Vector3& End, const Vector4& Plane);
bool LineTriangleMinIntersection(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& Start, const Vector3& End, float* MinDist, Vector3* Intersection);
bool LineQuadMinIntersection(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4, const Vector3& Start, const Vector3& End, float* MinDist, Vector3* Intersection);
float LinePointMinDistance(const Vector3& Point, const Vector3& Start, const Vector3& End);
bool BoundingBoxRayMinIntersectDistance(const BoundingBox& Box, const Vector3& Start, const Vector3& End, float* Dist);
bool BoundingBoxIntersectsVolume(const BoundingBox& Box, const Vector4* Planes, int NumPlanes);

#endif
