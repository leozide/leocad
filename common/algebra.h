#ifndef _ALGEBRA_H_
#define _ALGEBRA_H_

#include <math.h>

//
// Simple math library and algebra functions.
//
// Everything is based on the Float4 class, so changing that class should be enough
// to add support for compiler specific math intrinsics. For now only the reference
// implementation is supported.
//
// TODO: Add SSE support.
//

// Classes defined in this file:
class Float4;
class Point3;
class Vector3;
class Matrix33;
class Matrix44;

// ============================================================================
// Float4 class.

class Float4
{
public:
	// Constructors.
	inline Float4() { };
	inline explicit Float4(const float _x, const float _y, const float _z)
		: x(_x), y(_y), z(_z) { };
	inline explicit Float4(const float _x, const float _y, const float _z, const float _w)
		: x(_x), y(_y), z(_z), w(_w) { };

	// Get/Set functions.
	inline float GetX() const { return x; };
	inline float GetY() const { return y; };
	inline float GetZ() const { return z; };
	inline float GetW() const { return w; };
	inline void SetX(const float _x) { x = _x; };
	inline void SetY(const float _y) { y = _y; };
	inline void SetZ(const float _z) { z = _z; };
	inline void SetW(const float _w) { w = _w; };

	// Comparison.
	friend inline bool operator==(const Float4& a, const Float4& b)
	{ return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w); };

	friend inline bool Compare3(const Float4& a, const Float4& b)
	{ return (a.x == b.x) && (a.y == b.y) && (a.z == b.z); };

	// Math operations.
	friend inline Float4 operator+(const Float4& a, const Float4& b)
	{ return Float4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w); };

	friend inline Float4 operator-(const Float4& a, const Float4& b)
	{ return Float4(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w); };

	friend inline Float4 operator*(const Float4& a, float f)
	{ return Float4(a.x*f, a.y*f, a.z*f, a.w*f); };

	friend inline Float4 operator*(const Float4& a, const Float4& b)
	{ return Float4(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w); };

	friend inline Float4 operator/(const Float4& a, float f)
	{ return Float4(a.x/f, a.y/f, a.z/f, a.w/f); };

	// Dot product.
	friend inline float Dot3(const Float4& a, const Float4& b)
	{ return a.x*b.x + a.y*b.y + a.z*b.z; };

	// Cross product.
	friend inline Float4 Cross3(const Float4& a, const Float4& b)
	{ return Float4(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); };

	// Other functions.
	inline float Length3() const
	{ return sqrtf(Dot3(*this, *this)); };

protected:
	float x, y, z, w;
};


// ============================================================================
// 3D Point class.

class Point3
{
public:
	// Constructors.
	inline Point3() { };
	inline explicit Point3(const Float4& _v)
		: m_Value(_v) { };
	inline explicit Point3(const float _x, const float _y, const float _z)
		: m_Value(_x, _y, _z) { };

	// Get/Set functions.
	inline const Float4& GetValue() const { return m_Value; };
	inline float GetX() const { return m_Value.GetX(); };
	inline float GetY() const { return m_Value.GetY(); };
	inline float GetZ() const { return m_Value.GetZ(); };
	inline void SetX(const float _x) { m_Value.SetX(_x); };
	inline void SetY(const float _y) { m_Value.SetY(_y); };
	inline void SetZ(const float _z) { m_Value.SetZ(_z); };

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
	inline Vector3()
		{ };
	inline explicit Vector3(const Float4& _v)
		: m_Value(_v) { };
	inline explicit Vector3(const float _x, const float _y, const float _z)
		: m_Value(_x, _y, _z) { };

	// Get/Set functions.
	inline const Float4& GetValue() const { return m_Value; };
	inline float GetX() const { return m_Value.GetX(); };
	inline float GetY() const { return m_Value.GetY(); };
	inline float GetZ() const { return m_Value.GetZ(); };
	inline void SetX(const float _x) { m_Value.SetX(_x); };
	inline void SetY(const float _y) { m_Value.SetY(_y); };
	inline void SetZ(const float _z) { m_Value.SetZ(_z); };

	// Math operations.
	template<typename T> inline Vector3& operator+=(const T& a) { return *this = *this + a; }
	template<typename T> inline Vector3& operator-=(const T& a) { return *this = *this - a; }
	template<typename T> inline Vector3& operator/=(const T& a) { return *this = *this / a; }
	template<typename T> inline Vector3& operator*=(const T& a) { return *this = *this * a; }

	// Other functions.
	inline float Length() const
	{ return m_Value.Length3(); };

	inline float LengthSquared() const
	{ return Dot3(m_Value, m_Value); };

protected:
	Float4 m_Value;
};


// ============================================================================
// 3x3 Matrix class.

class Matrix33
{
public:

protected:
};


// ============================================================================
// 4x4 Matrix class.

class Matrix44
{
public:

protected:
};


// ============================================================================
// Operators.

// Comparison.
inline bool operator==(const Point3& a, const Point3& b)
{ return Compare3(a.GetValue(),  b.GetValue()); };

inline bool operator==(const Vector3& a, const Vector3& b)
{ return Compare3(a.GetValue(),  b.GetValue()); };

// Multiply by a scalar.
inline Vector3 operator*(const Vector3& a, float f)
{ return Vector3(a.GetValue()*f); };

inline Vector3 operator*(float f, const Vector3& a)
{ return Vector3(a.GetValue()*f); };

inline Point3 operator*(const Point3& a, float f)
{ return Point3(a.GetValue()*f); };

inline Point3 operator*(float f, const Point3& a)
{ return Point3(a.GetValue()*f); };

// Divide by a scalar.
inline Vector3 operator/(const Vector3& a, float f)
{ return Vector3(a.GetValue()/f); };

inline Vector3 operator/(float f, const Vector3& a)
{ return Vector3(a.GetValue()/f); };

inline Point3 operator/(const Point3& a, float f)
{ return Point3(a.GetValue()/f); };

inline Point3 operator/(float f, const Point3& a)
{ return Point3(a.GetValue()/f); };

// Add vectors/points (return a point if either is a point).
inline Point3 operator+(const Point3& a, const Point3& b)
{ return Point3(a.GetValue()+b.GetValue()); };

inline Point3 operator+(const Point3& a, const Vector3& b)
{ return Point3(a.GetValue()+b.GetValue()); };

inline Point3 operator+(const Vector3& a, const Point3& b)
{ return Point3(a.GetValue()+b.GetValue()); };

inline Vector3 operator+(const Vector3& a, const Vector3& b)
{ return Vector3(a.GetValue()+b.GetValue()); };

// Subtract vectors/points (return a vector if both arguments are the same type).
inline Vector3 operator-(const Point3& a, const Point3& b)
{ return Vector3(a.GetValue()-b.GetValue()); };

inline Point3 operator-(const Point3& a, const Vector3& b)
{ return Point3(a.GetValue()-b.GetValue()); };

inline Point3 operator-(const Vector3& a, const Point3& b)
{ return Point3(a.GetValue()-b.GetValue()); };

inline Vector3 operator-(const Vector3& a, const Vector3& b)
{ return Vector3(a.GetValue()-b.GetValue()); };

// Dot product.
inline float Dot3(const Vector3& a, const Vector3& b)
{ return Dot3(a.GetValue(), b.GetValue()); };


// ============================================================================
// Linear Algebra functions.

float DistancePointSegmentSquared(const Point3& Pt, const Point3& SegStart, const Point3& SegEnd);

#endif
