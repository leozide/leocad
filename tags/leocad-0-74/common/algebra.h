#ifndef _ALGEBRA_H_
#define _ALGEBRA_H_

#include <math.h>

//
// Simple math library and linear algebra functions.
//
// Everything is based on the Float4 class, so changing that class should be enough
// to add support for compiler specific math intrinsics.
//
// Functions that end with 34 mean that they don't care what happens to the 4th
// component, it can either be affected or not.
//

// TODO: Move this define to config.h
#define LC_MATH_FLOAT
//#define LC_MATH_SSE

// Classes defined in this file:
class Float4;
class Point3;
class Vector3;
class Quaternion;
class Matrix33;
class Matrix44;

// ============================================================================
// Float4 class (float version).

#ifdef LC_MATH_FLOAT

class Float4
{
public:
	// Constructors.
	inline Float4() { }
	inline explicit Float4(const float _x, const float _y, const float _z)
		: x(_x), y(_y), z(_z) { }
	inline explicit Float4(const float _x, const float _y, const float _z, const float _w)
		: x(_x), y(_y), z(_z), w(_w) { }

	// Get/Set functions.
	inline float GetX() const { return x; }
	inline float GetY() const { return y; }
	inline float GetZ() const { return z; }
	inline float GetW() const { return w; }
	inline void SetX(const float _x) { x = _x; }
	inline void SetY(const float _y) { y = _y; }
	inline void SetZ(const float _z) { z = _z; }
	inline void SetW(const float _w) { w = _w; }

	template<typename T>
	inline const float operator[](T i) const { return ((const float*)this)[i]; }

	// Comparison.
	friend inline bool operator==(const Float4& a, const Float4& b)
	{ return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w); }

	friend inline bool Compare3(const Float4& a, const Float4& b)
	{ return (a.x == b.x) && (a.y == b.y) && (a.z == b.z); }

	// Math operations for 4 components.
	friend inline Float4 operator+(const Float4& a, const Float4& b)
	{ return Float4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); }

	friend inline Float4 operator-(const Float4& a, const Float4& b)
	{ return Float4(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w); }

	friend inline Float4 operator*(const Float4& a, float f)
	{ return Float4(a.x*f, a.y*f, a.z*f, a.w*f); }

	friend inline Float4 operator*(const Float4& a, const Float4& b)
	{ return Float4(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w); }

	friend inline Float4 operator/(const Float4& a, float f)
	{ return Float4(a.x/f, a.y/f, a.z/f, a.w/f); }

	friend inline Float4 operator-(const Float4& a)
	{ return Float4(-a.x, -a.y, -a.z, -a.w); }

	// Math operations ignoring the 4th component.
	friend inline Float4 Add34(const Float4& a, const Float4& b)
	{ return Float4(a.x+b.x, a.y+b.y, a.z+b.z); }

	friend inline Float4 Subtract34(const Float4& a, const Float4& b)
	{ return Float4(a.x-b.x, a.y-b.y, a.z-b.z); }

	friend inline Float4 Multiply34(const Float4& a, float f)
	{ return Float4(a.x*f, a.y*f, a.z*f); }

	friend inline Float4 Multiply34(const Float4& a, const Float4& b)
	{ return Float4(a.x*b.x, a.y*b.y, a.z*b.z); }

	friend inline Float4 Divide34(const Float4& a, float f)
	{ return Float4(a.x/f, a.y/f, a.z/f); }

	friend inline Float4 Negate34(const Float4& a)
	{ return Float4(-a.x, -a.y, -a.z, -a.w); }

	// Dot product.
	friend inline float Dot3(const Float4& a, const Float4& b)
	{ return a.x*b.x + a.y*b.y + a.z*b.z; }

	// Cross product.
	friend inline Float4 Cross3(const Float4& a, const Float4& b)
	{ return Float4(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }

	// Other functions.
	inline float Length3() const
	{ return sqrtf(Dot3(*this, *this)); }

	inline void Normalize3()
	{ *this = *this / Length3(); }

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
// Float4 class (SSE version).

#ifdef LC_MATH_SSE

// If you can't find this file you need to install the VS6 Processor Pack.
#include <xmmintrin.h>

class __declspec(align(16)) Float4
{
public:
	// Constructors.
	inline Float4() { }
	inline explicit Float4(const __m128& _xyzw)
		: xyzw(_xyzw) { }
	inline explicit Float4(const float _x, const float _y, const float _z)
		: xyzw(_mm_setr_ps(_x, _y, _z, _z)) { }
	inline explicit Float4(const float _x, const float _y, const float _z, const float _w)
		: xyzw(_mm_setr_ps(_x, _y, _z, _w)) { }

	// Get/Set functions.
	inline float GetX() const { return ((const float*)this)[0]; }
	inline float GetY() const { return ((const float*)this)[1]; }
	inline float GetZ() const { return ((const float*)this)[2]; }
	inline float GetW() const { return ((const float*)this)[3]; }
	inline void SetX(const float _x)
	{
		__m128 xxyy = _mm_shuffle_ps(_mm_load_ps1(&_x), xyzw, _MM_SHUFFLE(1, 1, 0, 0));
		xyzw = _mm_shuffle_ps(xxyy, xyzw, _MM_SHUFFLE(3, 2, 2, 0));
	}
	inline void SetY(const float _y)
	{
		__m128 xxyy = _mm_shuffle_ps(xyzw, _mm_load_ps1(&_y), _MM_SHUFFLE(1, 1, 0, 0));
		xyzw = _mm_shuffle_ps(xxyy, xyzw, _MM_SHUFFLE(3, 2, 2, 0));
	}
	inline void SetZ(const float _z)
	{
		__m128 zzww = _mm_shuffle_ps(_mm_load_ps1(&_z), xyzw, _MM_SHUFFLE(3, 3, 2, 2));
		xyzw = _mm_shuffle_ps(xyzw, zzww, _MM_SHUFFLE(2, 0, 1, 0));
	}
	inline void SetW(const float _w)
	{
		__m128 zzww = _mm_shuffle_ps(xyzw, _mm_load_ps1(&_w), _MM_SHUFFLE(3, 3, 2, 2));
		xyzw = _mm_shuffle_ps(xyzw, zzww, _MM_SHUFFLE(2, 0, 1, 0));
	}

	template<typename T>
	inline const float operator[](T i) const { return ((const float*)this)[i]; }

	// Comparison.
	friend inline bool operator==(const Float4& a, const Float4& b)
	{ return !_mm_movemask_ps(_mm_cmpneq_ps(a.xyzw, b.xyzw)); }

	friend inline bool Compare3(const Float4& a, const Float4& b)
	{ return (_mm_movemask_ps(_mm_cmpeq_ps(a.xyzw, b.xyzw)) & 0x7) == 0x7; }

	// Math operations for 4 components.
	friend inline Float4 operator+(const Float4& a, const Float4& b)
	{ return Float4(_mm_add_ps(a.xyzw, b.xyzw)); }

	friend inline Float4 operator-(const Float4& a, const Float4& b)
	{ return Float4(_mm_sub_ps(a.xyzw, b.xyzw)); }

	friend inline Float4 operator*(const Float4& a, float f)
	{ return Float4(_mm_mul_ps(a.xyzw, _mm_load_ps1(&f))); }

	friend inline Float4 operator*(const Float4& a, const Float4& b)
	{ return Float4(_mm_mul_ps(a.xyzw, b.xyzw)); }

	friend inline Float4 operator/(const Float4& a, float f)
	{ return Float4(_mm_div_ps(a.xyzw, _mm_load_ps1(&f))); }

	friend inline Float4 operator-(const Float4& a)
	{
		static const __declspec(align(16)) unsigned int Mask[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 }
		return Float4(_mm_xor_ps(xyzw, *(__m128*)&Mask));
	}

	// Math operations ignoring the 4th component.
	friend inline Float4 Add34(const Float4& a, const Float4& b)
	{ return a*b }

	friend inline Float4 Subtract34(const Float4& a, const Float4& b)
	{ return a-b; }

	friend inline Float4 Multiply34(const Float4& a, float f)
	{ return a*f; }

	friend inline Float4 Multiply34(const Float4& a, const Float4& b)
	{ return a*b; }

	friend inline Float4 Divide34(const Float4& a, float f)
	{ return a/f; }

	friend inline Float4 Negate34(const Float4& a)
	{ return -a; }

	// Dot product.
	friend inline float Dot3(const Float4& a, const Float4& b)
	{
		__m128 tmp = _mm_mul_ps(a.xyzw, b.xyzw);
		__m128 yz = _mm_add_ss(_mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(2, 2, 2, 2)));
		tmp = _mm_add_ss(tmp, yz);

		return *(const float*)&tmp;
	}

	// Cross product.
	friend inline Float4 Cross3(const Float4& a, const Float4& b)
	{
		// a(yzx)*b(zxy)-a(zxy)*b(yzx)
		__m128 r1 = _mm_mul_ps(_mm_shuffle_ps(a.xyzw, a.xyzw, _MM_SHUFFLE(0, 0, 2, 1)), _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(0, 1, 0, 2)));
		__m128 r2 = _mm_mul_ps(_mm_shuffle_ps(a.xyzw, a.xyzw, _MM_SHUFFLE(0, 1, 0, 2)), _mm_shuffle_ps(b.xyzw, b.xyzw, _MM_SHUFFLE(0, 0, 2, 1)));

		return Float4(_mm_sub_ps(r1, r2));
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

	inline void Normalize3()
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
// 3D Point class.

class Point3
{
public:
	// Constructors.
	inline Point3() { }
	inline explicit Point3(const Float4& _v)
		: m_Value(_v) { }
	inline explicit Point3(const float _x, const float _y, const float _z)
		: m_Value(_x, _y, _z) { }

	// Get/Set functions.
	inline const Float4& GetValue() const { return m_Value; }
	inline float GetX() const { return m_Value.GetX(); }
	inline float GetY() const { return m_Value.GetY(); }
	inline float GetZ() const { return m_Value.GetZ(); }
	inline void SetX(const float _x) { m_Value.SetX(_x); }
	inline void SetY(const float _y) { m_Value.SetY(_y); }
	inline void SetZ(const float _z) { m_Value.SetZ(_z); }

	template<typename T>
	inline const float operator[](T i) const { return m_Value[i]; }

	// Math operations.
	template<typename T> inline Point3& operator+=(const T& a) { return *this = *this + a; }
	template<typename T> inline Point3& operator-=(const T& a) { return *this = *this - a; }
	template<typename T> inline Point3& operator/=(const T& a) { return *this = *this / a; }
	template<typename T> inline Point3& operator*=(const T& a) { return *this = *this * a; }

protected:
	Float4 m_Value;
};


// ============================================================================
// 3D Vector class.

class Vector3
{
public:
	// Constructors.
	inline Vector3()
		{ }
	inline explicit Vector3(const Float4& _v)
		: m_Value(_v) { }
	inline explicit Vector3(const float _x, const float _y, const float _z)
		: m_Value(_x, _y, _z) { }

	// Get/Set functions.
	inline const Float4& GetValue() const { return m_Value; }
	inline float GetX() const { return m_Value.GetX(); }
	inline float GetY() const { return m_Value.GetY(); }
	inline float GetZ() const { return m_Value.GetZ(); }
	inline void SetX(const float _x) { m_Value.SetX(_x); }
	inline void SetY(const float _y) { m_Value.SetY(_y); }
	inline void SetZ(const float _z) { m_Value.SetZ(_z); }

	template<typename T>
	inline const float operator[](T i) const { return m_Value[i]; }

	// Math operations.
	template<typename T> inline Vector3& operator+=(const T& a) { return *this = *this + a; }
	template<typename T> inline Vector3& operator-=(const T& a) { return *this = *this - a; }
	template<typename T> inline Vector3& operator/=(const T& a) { return *this = *this / a; }
	template<typename T> inline Vector3& operator*=(const T& a) { return *this = *this * a; }

	// Other functions.
	inline float Length() const
	{ return m_Value.Length3(); }

	inline float LengthSquared() const
	{ return Dot3(m_Value, m_Value); }

	inline void Normalize()
	{ m_Value.Normalize3(); }

	inline void Abs()
	{ m_Value.Abs(); }

protected:
	Float4 m_Value;
};


// ============================================================================
// Point3 and Vector3 Operators.

// Comparison.
inline bool operator==(const Point3& a, const Point3& b)
{ return Compare3(a.GetValue(), b.GetValue()); }

inline bool operator==(const Vector3& a, const Vector3& b)
{ return Compare3(a.GetValue(), b.GetValue()); }

// Multiply by a scalar.
inline Vector3 operator*(const Vector3& a, float f)
{ return Vector3(Multiply34(a.GetValue(), f)); }

inline Vector3 operator*(float f, const Vector3& a)
{ return Vector3(Multiply34(a.GetValue(), f)); }

inline Point3 operator*(const Point3& a, float f)
{ return Point3(Multiply34(a.GetValue(), f)); }

inline Point3 operator*(float f, const Point3& a)
{ return Point3(Multiply34(a.GetValue(), f)); }

// Divide by a scalar.
inline Vector3 operator/(const Vector3& a, float f)
{ return Vector3(Divide34(a.GetValue(), f)); }

inline Vector3 operator/(float f, const Vector3& a)
{ return Vector3(Divide34(a.GetValue(), f)); }

inline Point3 operator/(const Point3& a, float f)
{ return Point3(Divide34(a.GetValue(), f)); }

inline Point3 operator/(float f, const Point3& a)
{ return Point3(Divide34(a.GetValue(), f)); }

// Add vectors/points (return a point if either is a point).
inline Point3 operator+(const Point3& a, const Point3& b)
{ return Point3(Add34(a.GetValue(), b.GetValue())); }

inline Point3 operator+(const Point3& a, const Vector3& b)
{ return Point3(Add34(a.GetValue(), b.GetValue())); }

inline Point3 operator+(const Vector3& a, const Point3& b)
{ return Point3(Add34(a.GetValue(), b.GetValue())); }

inline Vector3 operator+(const Vector3& a, const Vector3& b)
{ return Vector3(Add34(a.GetValue(), b.GetValue())); }

// Subtract vectors/points (return a vector if both arguments are the same type).
inline Vector3 operator-(const Point3& a, const Point3& b)
{ return Vector3(Subtract34(a.GetValue(), b.GetValue())); }

inline Point3 operator-(const Point3& a, const Vector3& b)
{ return Point3(Subtract34(a.GetValue(), b.GetValue())); }

inline Point3 operator-(const Vector3& a, const Point3& b)
{ return Point3(Subtract34(a.GetValue(), b.GetValue())); }

inline Vector3 operator-(const Vector3& a, const Vector3& b)
{ return Vector3(Subtract34(a.GetValue(), b.GetValue())); }

// Negate.
inline Vector3 operator-(const Vector3& a)
{ return Vector3(Negate34(a.GetValue())); }

inline Point3 operator-(const Point3& a)
{ return Point3(Negate34(a.GetValue())); }

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
	inline explicit Quaternion(const Float4& _v)
		: m_Value(_v) { }
	inline explicit Quaternion(const float _x, const float _y, const float _z, const float _w)
		: m_Value(_x, _y, _z, _w) { }

	// Get/Set functions.
	inline float GetX() const { return m_Value.GetX(); }
	inline float GetY() const { return m_Value.GetY(); }
	inline float GetZ() const { return m_Value.GetZ(); }
	inline float GetW() const { return m_Value.GetW(); }
	inline void SetX(const float _x) { m_Value.SetX(_x); }
	inline void SetY(const float _y) { m_Value.SetY(_y); }
	inline void SetZ(const float _z) { m_Value.SetZ(_z); }
	inline void SetW(const float _w) { m_Value.SetW(_w); }

	template<typename T>
	inline const float operator[](T i) const { return m_Value[i]; }

	// Conversions.
	inline void FromAxisAngle(const Float4& AxisAngle)
	{
		float s = sinf(AxisAngle[3] / 2.0f);
		m_Value = Float4(AxisAngle[0] * s, AxisAngle[1] * s, AxisAngle[2] * s, cosf(AxisAngle[3] / 2.0f));
	}

	inline void ToAxisAngle(Float4& AxisAngle) const
	{
		float Len = m_Value[0]*m_Value[0] + m_Value[1]*m_Value[1] + m_Value[2]*m_Value[2];

		if (Len > 0.001f)
		{
			float f = 1.0f / sqrtf(Len);
			AxisAngle = Float4(m_Value[0] * f, m_Value[1] * f, m_Value[2] * f, acosf(m_Value[3]) * 2.0f);
		}
		else
		{
			AxisAngle = Float4(0, 0, 1, 0);
		}
	}

	// Operators.
	friend inline Quaternion operator*(const Quaternion& a, const Quaternion& b)
	{
    float x =  a.m_Value[0] * b.m_Value[3] + a.m_Value[1] * b.m_Value[2] - a.m_Value[2] * b.m_Value[1] + a.m_Value[3] * b.m_Value[0];
    float y = -a.m_Value[0] * b.m_Value[2] + a.m_Value[1] * b.m_Value[3] + a.m_Value[2] * b.m_Value[0] + a.m_Value[3] * b.m_Value[1];
    float z =  a.m_Value[0] * b.m_Value[1] - a.m_Value[1] * b.m_Value[0] + a.m_Value[2] * b.m_Value[3] + a.m_Value[3] * b.m_Value[2];
    float w = -a.m_Value[0] * b.m_Value[0] - a.m_Value[1] * b.m_Value[1] - a.m_Value[2] * b.m_Value[2] + a.m_Value[3] * b.m_Value[3];

		return Quaternion(x, y, z, w);
	}

	friend inline Vector3 operator*(const Vector3& a, const Quaternion& b)
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

		return Vector3(Rows[0].GetValue()*a.GetX() + Rows[1].GetValue()*a.GetY() + Rows[2].GetValue()*a.GetZ());
	}

protected:
	Float4 m_Value;
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
		m_Rows[0] = Vector3(1, 0, 0);
		m_Rows[1] = Vector3(0, 1, 0);
		m_Rows[2] = Vector3(0, 0, 1);
	}

	inline void FromAxisAngle(const Vector3& Axis, const float Radians)
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

		m_Rows[0] = Vector3((one_c * xx) + c, (one_c * xy) - zs, (one_c * zx) + ys);
		m_Rows[1] = Vector3((one_c * xy) + zs, (one_c * yy) + c, (one_c * yz) - xs);
		m_Rows[2] = Vector3((one_c * zx) - ys, (one_c * yz) + xs, (one_c * zz) + c);
	}

	friend inline Vector3 operator*(const Vector3& a, const Matrix33& b)
	{ return Vector3(b.m_Rows[0]*a.GetX() + b.m_Rows[1]*a.GetY() + b.m_Rows[2]*a.GetZ()); }

protected:
	Vector3 m_Rows[3];
};


// ============================================================================
// 4x4 Matrix class (actually 4x3).

class Matrix44
{
public:
	inline Matrix44()
		{ }
	inline Matrix44(const Float4& Row0, const Float4& Row1, const Float4& Row2, const Float4& Row3)
		{ m_Rows[0] = Row0; m_Rows[1] = Row1; m_Rows[2] = Row2; m_Rows[3] = Row3; }

	inline void Transpose3()
	{
		Float4 a = m_Rows[0], b = m_Rows[1], c = m_Rows[2];
		m_Rows[0] = Float4(a.GetX(), b.GetX(), c.GetX(), a.GetW());
		m_Rows[1] = Float4(a.GetY(), b.GetY(), c.GetY(), b.GetW());
		m_Rows[2] = Float4(a.GetZ(), b.GetZ(), c.GetZ(), c.GetW());
	}

	inline void SetTranslation(const Point3& a)
	{ m_Rows[3] = Float4(a.GetX(), a.GetY(), a.GetZ(), 0.0f); }

	void CreateLookAt(const Point3& Eye, const Point3& Target, const Vector3& Up);
	void CreatePerspective(float FoVy, float Aspect, float Near, float Far);

	friend inline Point3 operator*(const Point3& a, const Matrix44& b)
	{ return Point3(b.m_Rows[0]*a.GetX() + b.m_Rows[1]*a.GetY() + b.m_Rows[2]*a.GetZ() + b.m_Rows[3]); }

protected:
	Float4 m_Rows[4];
};


// ============================================================================
// Other Functions.

Point3 ProjectPoint(const Point3& Pt, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4]);

#endif
