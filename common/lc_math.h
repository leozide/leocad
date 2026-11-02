#pragma once

#include <math.h>
#include <float.h>

#define LC_DTOR (static_cast<float>(M_PI / 180))
#define LC_RTOD (static_cast<float>(180 / M_PI))
#define LC_PI (static_cast<float>(M_PI))
#define LC_2PI (static_cast<float>(2 * M_PI))
#define LC_RGB_EPSILON (static_cast<float>(0.5f / 255.0f))

inline constexpr float LC_EPSILON = 0.0001f;

#define LC_RGB(r,g,b) LC_RGBA(r,g,b,255)
#define LC_RGBA(r,g,b,a) ((quint32)(((quint8) (r) | ((quint16) (g) << 8)) | (((quint32) (quint8) (b)) << 16) | (((quint32) (quint8) (a)) << 24)))
#define LC_RGBA_RED(rgba)   ((quint8)(((rgba) >>  0) & 0xff))
#define LC_RGBA_GREEN(rgba) ((quint8)(((rgba) >>  8) & 0xff))
#define LC_RGBA_BLUE(rgba)  ((quint8)(((rgba) >> 16) & 0xff))
#define LC_RGBA_ALPHA(rgba) ((quint8)(((rgba) >> 24) & 0xff))
#define LC_SRGB_TO_LINEAR(v) (powf(v, 2.2f))
#define LC_LINEAR_TO_SRGB(v) (powf(v, 1.0f / 2.2f))

inline quint32 lcRGBAFromQColor(const QColor& Color)
{
	return LC_RGBA(Color.red(), Color.green(), Color.blue(), Color.alpha());
}

inline QColor lcQColorFromRGBA(quint32 RGBA)
{
	return QColor::fromRgb(LC_RGBA_RED(RGBA), LC_RGBA_GREEN(RGBA), LC_RGBA_BLUE(RGBA), LC_RGBA_ALPHA(RGBA));
}

template<typename T>
inline T lcMin(const T& a, const T& b)
{
	return a < b ? a : b;
}

template<typename T>
inline T lcMax(const T& a, const T& b)
{
	return a > b ? a : b;
}

template<typename T>
inline T lcClamp(const T& Value, const T& Min, const T& Max)
{
	if (Value > Max)
		return Max;
	else if (Value < Min)
		return Min;
	else
		return Value;
}

inline bool lcIsZero(float Value)
{
	return fabsf(Value) < LC_EPSILON;
}

class lcVector2i
{
public:
	lcVector2i()
	{
	}

	constexpr lcVector2i(const int _x, const int _y)
		: x(_x), y(_y)
	{
	}

	int x, y;
};

inline bool operator==(const lcVector2i& a, const lcVector2i& b)
{
	return a.x == b.x && a.y == b.y;
}

inline bool operator!=(const lcVector2i& a, const lcVector2i& b)
{
	return a.x != b.x || a.y != b.y;
}

class lcVector2
{
public:
	lcVector2()
	{
	}

	constexpr lcVector2(const float _x, const float _y)
		: x(_x), y(_y)
	{
	}

	const float& operator[](int i) const
	{
		return ((float*)this)[i];
	}

	float& operator[](int i)
	{
		return ((float*)this)[i];
	}

	const float* GetFloats() const
	{
		return (const float*)this;
	}

	float* GetFloats()
	{
		return (float*)this;
	}

	bool IsNan() const
	{
		return std::isnan(x) || std::isnan(y);
	}

	float x, y;
};

class lcVector3
{
public:
	lcVector3()
	{
	}

	constexpr lcVector3(const float _x, const float _y, const float _z)
		: x(_x), y(_y), z(_z)
	{
	}

	explicit lcVector3(const lcVector4& v);

	const float& operator[](int i) const
	{
		return ((float*)this)[i];
	}

	float& operator[](int i)
	{
		return ((float*)this)[i];
	}

	const float* GetFloats() const
	{
		return (const float*)this;
	}

	float* GetFloats()
	{
		return (float*)this;
	}

	bool IsNan() const
	{
		return std::isnan(x) || std::isnan(y) || std::isnan(z);
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

	constexpr lcVector4(const float _x, const float _y, const float _z, const float _w)
		: x(_x), y(_y), z(_z), w(_w)
	{
	}

	constexpr lcVector4(const lcVector3& _xyz, const float _w)
		: x(_xyz.x), y(_xyz.y), z(_xyz.z), w(_w)
	{
	}

	const float& operator[](int i) const
	{
		return ((float*)this)[i];
	}

	float& operator[](int i)
	{
		return ((float*)this)[i];
	}

	const float* GetFloats() const
	{
		return (const float*)this;
	}

	float* GetFloats()
	{
		return (float*)this;
	}

	bool IsNan() const
	{
		return std::isnan(x) || std::isnan(y) || std::isnan(z) || std::isnan(w);
	}

	float x, y, z, w;
};

class lcMatrix33
{
public:
	lcMatrix33()
	{
	}

	lcMatrix33(const lcVector3& _x, const lcVector3& _y, const lcVector3& _z)
	{
		r[0] = _x;
		r[1] = _y;
		r[2] = _z;
	}

	explicit lcMatrix33(const lcMatrix44& Matrix);

	const lcVector3& operator[](int i) const
	{
		return r[i];
	}

	lcVector3& operator[](int i)
	{
		return r[i];
	}

	const float* GetFloats() const
	{
		return (const float*)this;
	}

	float* GetFloats()
	{
		return (float*)this;
	}

	void Orthonormalize();
	void SnapToAxisAligned();

	lcVector3 r[3];
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

	lcMatrix44(const lcMatrix33& Rotation, const lcVector3& Translation)
	{
		r[0] = lcVector4(Rotation[0][0], Rotation[0][1], Rotation[0][2], 0.0f);
		r[1] = lcVector4(Rotation[1][0], Rotation[1][1], Rotation[1][2], 0.0f);
		r[2] = lcVector4(Rotation[2][0], Rotation[2][1], Rotation[2][2], 0.0f);
		r[3] = lcVector4(Translation, 1.0f);
	}

	lcVector3 GetTranslation() const
	{
		return lcVector3(r[3][0], r[3][1], r[3][2]);
	}

	void SetTranslation(const lcVector3& Translation)
	{
		r[3] = lcVector4(Translation[0], Translation[1], Translation[2], 1.0f);
	}

	const lcVector4& operator[](int i) const
	{
		return r[i];
	}

	lcVector4& operator[](int i)
	{
		return r[i];
	}

	const float* GetFloats() const
	{
		return (const float*)this;
	}

	float* GetFloats()
	{
		return (float*)this;
	}

	float Determinant() const;

	lcVector4 r[4];
};

inline bool operator==(const lcVector2& a, const lcVector2& b)
{
	return a.x == b.x && a.y == b.y;
}

inline lcVector3::lcVector3(const lcVector4& v)
	: x(v.x), y(v.y), z(v.z)
{
}

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

inline lcVector3& operator+=(lcVector3& a, float b)
{
	a.x += b;
	a.y += b;
	a.z += b;

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

inline bool operator==(const lcVector3& a, const lcVector3& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline bool operator!=(const lcVector3& a, const lcVector3& b)
{
	return a.x != b.x || a.y != b.y || a.z != b.z;
}

inline bool operator<(const lcVector3& a, const lcVector3& b)
{
	if (a.x < b.x)
		return true;
	else if (a.x > b.x)
		return false;

	if (a.y < b.y)
		return true;
	else if (a.y > b.y)
		return false;

	if (a.z < b.z)
		return true;
	else if (a.z > b.z)
		return false;

	return false;
}

inline bool operator==(const lcVector4& a, const lcVector4& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

inline bool operator==(const lcMatrix33& a, const lcMatrix33& b)
{
	return a.r[0] == b.r[0] && a.r[1] == b.r[1] && a.r[2] == b.r[2];
}

inline bool operator==(const lcMatrix44& a, const lcMatrix44& b)
{
	return a.r[0] == b.r[0] && a.r[1] == b.r[1] && a.r[2] == b.r[2] && a.r[3] == b.r[3];
}

#ifndef QT_NO_DEBUG

QDebug operator<<(QDebug Debug, const lcVector2& v);
QDebug operator<<(QDebug Debug, const lcVector3& v);
QDebug operator<<(QDebug Debug, const lcVector4& v);
QDebug operator<<(QDebug Debug, const lcMatrix33& m);
QDebug operator<<(QDebug Debug, const lcMatrix44& m);

#endif

QDataStream& operator<<(QDataStream& Stream, const lcVector3& v);
QDataStream& operator>>(QDataStream& Stream, lcVector3& v);
QDataStream& operator<<(QDataStream& Stream, const lcVector4& v);
QDataStream& operator>>(QDataStream& Stream, lcVector4& v);
QDataStream& operator<<(QDataStream& Stream, const lcMatrix33& m);
QDataStream& operator>>(QDataStream& Stream, lcMatrix33& m);
QDataStream& operator<<(QDataStream& Stream, const lcMatrix44& m);
QDataStream& operator>>(QDataStream& Stream, lcMatrix44& m);

inline void lcVector3::Normalize()
{
	const float InvLength = 1.0f / Length();

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

inline float lcLength(const lcVector3& a)
{
	return a.Length();
}

inline float lcLengthSquared(const lcVector3& a)
{
	return a.LengthSquared();
}

inline bool lcIsZero(const lcVector3& Value)
{
	return Value.LengthSquared() < LC_EPSILON * LC_EPSILON;
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

inline float lcDot3(const lcVector4& a, const lcVector4& b)
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

template<>
inline lcVector3 lcMin<lcVector3>(const lcVector3& a, const lcVector3& b)
{
	return lcVector3(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z);
}

template<>
inline lcVector3 lcMax<lcVector3>(const lcVector3& a, const lcVector3& b)
{
	return lcVector3(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z);
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

inline lcVector4& operator+=(lcVector4& a, const lcVector4& b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	a.w += b.w;

	return a;
}

inline lcVector4& operator-=(lcVector4& a, const lcVector4& b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	a.w -= b.w;

	return a;
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

inline quint32 lcPackNormal(const lcVector3& Normal)
{
	quint32 Packed = 0;

	Packed |= (((qint8)(Normal.x * 127.0f)) & 0xff) << 0;
	Packed |= (((qint8)(Normal.y * 127.0f)) & 0xff) << 8;
	Packed |= (((qint8)(Normal.z * 127.0f)) & 0xff) << 16;

	return Packed;
}

inline lcVector3 lcUnpackNormal(quint32 Packed)
{
	lcVector3 Normal;

	Normal.x = (float)(qint8)((Packed >>  0) & 0xff) / 127.0f;
	Normal.y = (float)(qint8)((Packed >>  8) & 0xff) / 127.0f;
	Normal.z = (float)(qint8)((Packed >> 16) & 0xff) / 127.0f;

	return Normal;
}

inline lcVector3 lcVector3LDrawToLeoCAD(const lcVector3& Vector)
{
	return lcVector3(Vector[0], Vector[2], -Vector[1]);
}

inline lcVector3 lcVector3FromColor(quint32 Color)
{
	lcVector3 v(LC_RGBA_RED(Color), LC_RGBA_GREEN(Color), LC_RGBA_BLUE(Color));
	v /= 255.0f;
	return v;
}

inline lcVector3 lcVector3FromQColor(QColor Color)
{
	return lcVector3(Color.redF(), Color.greenF(), Color.blueF());
}

inline lcVector4 lcVector4FromColor(quint32 Color)
{
	lcVector4 v(LC_RGBA_RED(Color), LC_RGBA_GREEN(Color), LC_RGBA_BLUE(Color), LC_RGBA_ALPHA(Color));
	v /= 255.0f;
	return v;
}

inline quint32 lcColorFromVector3(const lcVector3& Color)
{
	return LC_RGB(roundf(Color[0] * 255), roundf(Color[1] * 255), roundf(Color[2] * 255));
}

inline QColor lcQColorFromVector3(const lcVector3& Color)
{
	return QColor::fromRgb(roundf(Color[0] * 255), roundf(Color[1] * 255), roundf(Color[2] * 255));
}

inline float lcLuminescence(const lcVector3& Color)
{
	return 0.2126f * Color[0] + 0.7152f * Color[1] + 0.0722f * Color[2];
}

inline lcVector3 lcSRGBToLinear(const lcVector3& Color)
{
	const float r = LC_SRGB_TO_LINEAR(Color[0]);
	const float g = LC_SRGB_TO_LINEAR(Color[1]);
	const float b = LC_SRGB_TO_LINEAR(Color[2]);

	return lcVector3(r, g, b);
}

inline lcVector3 lcLinearToSRGB(const lcVector3& Color)
{
	const float r = LC_LINEAR_TO_SRGB(Color[0]);
	const float g = LC_LINEAR_TO_SRGB(Color[1]);
	const float b = LC_LINEAR_TO_SRGB(Color[2]);

	return lcVector3(r, g, b);
}

inline lcVector3 lcMul(const lcVector3& a, const lcMatrix33& b)
{
	return b.r[0] * a[0] + b.r[1] * a[1] + b.r[2] * a[2];
}

inline lcVector3 lcMul31(const lcVector3& a, const lcMatrix44& b)
{
	lcVector4 v = b.r[0] * a[0] + b.r[1] * a[1] + b.r[2] * a[2] + b.r[3];

	return lcVector3(v[0], v[1], v[2]);
}

inline lcVector3 lcMul31(const lcVector4& a, const lcMatrix44& b)
{
	lcVector4 v = b.r[0] * a[0] + b.r[1] * a[1] + b.r[2] * a[2] + b.r[3];

	return lcVector3(v[0], v[1], v[2]);
}

inline lcVector3 lcMul30(const lcVector3& a, const lcMatrix44& b)
{
	lcVector4 v = b.r[0] * a[0] + b.r[1] * a[1] + b.r[2] * a[2];

	return lcVector3(v[0], v[1], v[2]);
}

inline lcVector3 lcMul30(const lcVector4& a, const lcMatrix44& b)
{
	lcVector4 v = b.r[0] * a[0] + b.r[1] * a[1] + b.r[2] * a[2];

	return lcVector3(v[0], v[1], v[2]);
}

inline lcVector4 lcMul4(const lcVector4& a, const lcMatrix44& b)
{
	return b.r[0] * a[0] + b.r[1] * a[1] + b.r[2] * a[2] + b.r[3] * a[3];
}

inline lcMatrix33 lcMul(const lcMatrix33& a, const lcMatrix33& b)
{
	const lcVector3 Col0(b.r[0][0], b.r[1][0], b.r[2][0]);
	const lcVector3 Col1(b.r[0][1], b.r[1][1], b.r[2][1]);
	const lcVector3 Col2(b.r[0][2], b.r[1][2], b.r[2][2]);

	const lcVector3 Ret0(lcDot(a.r[0], Col0), lcDot(a.r[0], Col1), lcDot(a.r[0], Col2));
	const lcVector3 Ret1(lcDot(a.r[1], Col0), lcDot(a.r[1], Col1), lcDot(a.r[1], Col2));
	const lcVector3 Ret2(lcDot(a.r[2], Col0), lcDot(a.r[2], Col1), lcDot(a.r[2], Col2));

	return lcMatrix33(Ret0, Ret1, Ret2);
}

inline lcMatrix44 lcMul(const lcMatrix44& a, const lcMatrix44& b)
{
	lcMatrix44 Result;

	Result.r[0] = b.r[0] * a[0].x + b.r[1] * a[0].y + b.r[2] * a[0].z + b.r[3] * a[0].w;
	Result.r[1] = b.r[0] * a[1].x + b.r[1] * a[1].y + b.r[2] * a[1].z + b.r[3] * a[1].w;
	Result.r[2] = b.r[0] * a[2].x + b.r[1] * a[2].y + b.r[2] * a[2].z + b.r[3] * a[2].w;
	Result.r[3] = b.r[0] * a[3].x + b.r[1] * a[3].y + b.r[2] * a[3].z + b.r[3] * a[3].w;

	return Result;
}

inline lcMatrix33::lcMatrix33(const lcMatrix44& Matrix)
{
	r[0] = lcVector3(Matrix.r[0].x, Matrix.r[0].y, Matrix.r[0].z);
	r[1] = lcVector3(Matrix.r[1].x, Matrix.r[1].y, Matrix.r[1].z);
	r[2] = lcVector3(Matrix.r[2].x, Matrix.r[2].y, Matrix.r[2].z);
}

inline void lcMatrix33::Orthonormalize()
{
	r[0] = lcNormalize(r[0]);
	r[1] = lcNormalize(r[1] - lcDot(r[1], r[0]) * r[0]);
	r[2] = r[2] - lcDot(r[2], r[0]) * r[0];
	r[2] -= lcDot(r[2], r[1]) * r[1];
	r[2] = lcNormalize(r[2]);
}

inline void lcMatrix33::SnapToAxisAligned()
{
	// Only replace a complete orientation close to a signed permutation matrix.
	// Rounding individual elements would distort intentionally angled parts.
	lcMatrix33 Snapped;
	unsigned int Columns = 0;

	for (int Row = 0; Row < 3; Row++)
	{
		unsigned int RowColumns = 0;

		for (int Column = 0; Column < 3; Column++)
		{
			const float Value = roundf(r[Row][Column]);

			if (!(fabsf(r[Row][Column] - Value) <= 0.000001f) || fabsf(Value) > 1.0f)
				return;

			Snapped[Row][Column] = Value == 0.0f ? 0.0f : Value;

			if (Value != 0.0f)
				RowColumns |= 1U << Column;
		}

		if (RowColumns == 0 || (RowColumns & (RowColumns - 1)) != 0 || (Columns & RowColumns) != 0)
			return;

		Columns |= RowColumns;
	}

	*this = Snapped;
}

inline lcMatrix33 lcMatrix33Identity()
{
	lcMatrix33 m;

	m.r[0] = lcVector3(1.0f, 0.0f, 0.0f);
	m.r[1] = lcVector3(0.0f, 1.0f, 0.0f);
	m.r[2] = lcVector3(0.0f, 0.0f, 1.0f);

	return m;
}

inline lcMatrix33 lcMatrix33Scale(const lcVector3& Scale)
{
	lcMatrix33 m;

	m.r[0] = lcVector3(Scale.x, 0.0f, 0.0f);
	m.r[1] = lcVector3(0.0f, Scale.y, 0.0f);
	m.r[2] = lcVector3(0.0f, 0.0f, Scale.z);

	return m;
}

inline lcMatrix33 lcMatrix33RotationX(const float Radians)
{
	float s, c;

	s = sinf(Radians);
	c = cosf(Radians);

	lcMatrix33 m;

	m.r[0] = lcVector3(1.0f, 0.0f, 0.0f);
	m.r[1] = lcVector3(0.0f,    c,    s);
	m.r[2] = lcVector3(0.0f,   -s,    c);

	return m;
}

inline lcMatrix33 lcMatrix33RotationY(const float Radians)
{
	float s, c;

	s = sinf(Radians);
	c = cosf(Radians);

	lcMatrix33 m;

	m.r[0] = lcVector3(   c, 0.0f,   -s);
	m.r[1] = lcVector3(0.0f, 1.0f, 0.0f);
	m.r[2] = lcVector3(   s, 0.0f,    c);

	return m;
}

inline lcMatrix33 lcMatrix33RotationZ(const float Radians)
{
	float s = sinf(Radians);
	float c = cosf(Radians);

	lcMatrix33 m;

	m.r[0] = lcVector3(   c,    s, 0.0f);
	m.r[1] = lcVector3(  -s,    c, 0.0f);
	m.r[2] = lcVector3(0.0f, 0.0f, 1.0f);

	return m;
}

inline lcMatrix33 lcMatrix33FromAxisAngle(const lcVector3& Axis, const float Radians)
{
	float s = sinf(Radians);
	float c = cosf(Radians);
	float mag = Axis.Length();

	if (mag == 0.0f)
		return lcMatrix33Identity();

	lcVector3 Normal = Axis * (1.0f / mag);

	float xx = Normal[0] * Normal[0];
	float yy = Normal[1] * Normal[1];
	float zz = Normal[2] * Normal[2];
	float xy = Normal[0] * Normal[1];
	float yz = Normal[1] * Normal[2];
	float zx = Normal[2] * Normal[0];
	float xs = Normal[0] * s;
	float ys = Normal[1] * s;
	float zs = Normal[2] * s;
	float one_c = 1.0f - c;

	lcMatrix33 m;

	m.r[0] = lcVector3((one_c * xx) + c, (one_c * xy) + zs, (one_c * zx) - ys);
	m.r[1] = lcVector3((one_c * xy) - zs, (one_c * yy) + c, (one_c * yz) + xs);
	m.r[2] = lcVector3((one_c * zx) + ys, (one_c * yz) - xs, (one_c * zz) + c);

	return m;
}

inline lcMatrix33 lcMatrix33Transpose(const lcMatrix33& m)
{
	lcMatrix33 t;

	t.r[0] = lcVector3(m[0][0], m[1][0], m[2][0]);
	t.r[1] = lcVector3(m[0][1], m[1][1], m[2][1]);
	t.r[2] = lcVector3(m[0][2], m[1][2], m[2][2]);

	return t;
}

inline lcMatrix33 lcMatrix33AffineInverse(const lcMatrix33& m)
{
	lcMatrix33 Inv;

	Inv.r[0] = lcVector3(m.r[0][0], m.r[1][0], m.r[2][0]);
	Inv.r[1] = lcVector3(m.r[0][1], m.r[1][1], m.r[2][1]);
	Inv.r[2] = lcVector3(m.r[0][2], m.r[1][2], m.r[2][2]);

	return Inv;
}

inline bool lcMatrix33Similar(const lcMatrix33& a, const lcMatrix33& b)
{
	for (int Row = 0; Row < 3; Row++)
	{
		float Dot = lcDot(a.r[Row], b.r[Row]);

		if (Dot < 0.9999f || Dot > 1.0001f)
			return false;
	}

	return true;
}

inline float lcMatrix44::Determinant() const
{
	return r[0][0] * r[1][1] * r[2][2] + r[0][1] * r[1][2] * r[2][0] +
	       r[0][2] * r[1][0] * r[2][1] - r[0][0] * r[1][2] * r[2][1] -
	       r[0][1] * r[1][0] * r[2][2] - r[0][2] * r[1][1] * r[2][0];
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

inline lcMatrix44 lcMatrix44Scale(const lcVector3& Scale)
{
	lcMatrix44 m;

	m.r[0] = lcVector4(Scale.x, 0.0f, 0.0f, 0.0f);
	m.r[1] = lcVector4(0.0f, Scale.y, 0.0f, 0.0f);
	m.r[2] = lcVector4(0.0f, 0.0f, Scale.z, 0.0f);
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

inline lcMatrix44 lcMatrix44Frustum(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
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

	m.r[0] = lcVector4(x, 0, 0, 0);
	m.r[1] = lcVector4(0, y, 0, 0);
	m.r[2] = lcVector4(a, b, c, -1);
	m.r[3] = lcVector4(0, 0, d, 0);

	return m;
}

inline lcMatrix44 lcMatrix44Perspective(float FoVy, float Aspect, float Near, float Far)
{
	float Left, Right, Bottom, Top;

	Top = Near * (float)tan(FoVy * LC_PI / 360.0f);
	Bottom = -Top;

	Left = Bottom * Aspect;
	Right = Top * Aspect;

	return lcMatrix44Frustum(Left, Right, Bottom, Top, Near, Far);
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

lcVector4 lcMatrix44ToAxisAngle(const lcMatrix44& m);

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

	lcVector3 Trans = -lcMul30(m.r[3], Inv);
	Inv.r[3] = lcVector4(Trans[0], Trans[1], Trans[2], 1.0f);

	return Inv;
}

lcMatrix44 lcMatrix44Inverse(const lcMatrix44& m);

inline lcMatrix44 lcMatrix44LeoCADToLDraw(const lcMatrix44& Matrix)
{
	lcMatrix44 m;

	m.r[0] = lcVector4(Matrix[0][0], -Matrix[2][0], Matrix[1][0], 0.0f);
	m.r[1] = lcVector4(-Matrix[0][2], Matrix[2][2], -Matrix[1][2], 0.0f);
	m.r[2] = lcVector4(Matrix[0][1], -Matrix[2][1], Matrix[1][1], 0.0f);
	m.r[3] = lcVector4(Matrix[3][0], -Matrix[3][2], Matrix[3][1], 1.0f);

	return m;
}

inline lcMatrix44 lcMatrix44LDrawToLeoCAD(const lcMatrix44& Matrix)
{
	lcMatrix44 m;

	m.r[0] = lcVector4(Matrix[0][0], Matrix[0][2], -Matrix[0][1], 0.0f);
	m.r[1] = lcVector4(Matrix[2][0], Matrix[2][2], -Matrix[2][1], 0.0f);
	m.r[2] = lcVector4(-Matrix[1][0], -Matrix[1][2], Matrix[1][1], 0.0f);
	m.r[3] = lcVector4(Matrix[3][0], Matrix[3][2], -Matrix[3][1], 1.0f);

	return m;
}

// Convert world coordinates to screen coordinates.
lcVector3 lcProjectPoint(const lcVector3& Point, const lcMatrix44& ModelView, const lcMatrix44& Projection, const int (&Viewport)[4]);

lcVector3 lcUnprojectPoint(const lcVector3& Point, const lcMatrix44& ModelView, const lcMatrix44& Projection, const int (&Viewport)[4]);

void lcUnprojectPoints(lcVector3* Points, int NumPoints, const lcMatrix44& ModelView, const lcMatrix44& Projection, const int (&Viewport)[4]);

void lcGetFrustumPlanes(const lcMatrix44& WorldView, const lcMatrix44& Projection, lcVector4 (&Planes)[6]);

std::tuple<lcVector3, float> lcZoomExtents(const lcVector3& Position, const lcMatrix44& WorldView, const lcMatrix44& Projection, const lcVector3* Points, size_t NumPoints);

inline void lcClosestPointsBetweenLines(const lcVector3& Line1a, const lcVector3& Line1b, const lcVector3& Line2a, const lcVector3& Line2b, lcVector3* Intersection1, lcVector3* Intersection2)
{
	const lcVector3 u1 = Line1b - Line1a;
	const lcVector3 u2 = Line2b - Line2a;
	const lcVector3 p21 = Line2a - Line1a;
	const lcVector3 m = lcCross(u2, u1);
	const float m2 = lcDot(m, m);

	if (m2 < 0.00001f)
	{
		if (Intersection1)
			*Intersection1 = Line1a;
		if (Intersection2)
			*Intersection2 = Line2a;
		return;
	}

	const lcVector3 r = lcCross(p21, m / m2);

	if (Intersection1)
	{
		const float t1 = lcDot(r, u2);
		*Intersection1 = Line1a + t1 * u1;
	}

	if (Intersection2)
	{
		const float t2 = lcDot(r, u1);
		*Intersection2 = Line2a + t2 * u2;
	}
}

inline bool lcLineSegmentPlaneIntersection(lcVector3* Intersection, const lcVector3& Start, const lcVector3& End, const lcVector4& Plane)
{
	const lcVector3 Dir = End - Start;
	const lcVector3 PlaneNormal(Plane[0], Plane[1], Plane[2]);

	const float t1 = lcDot(PlaneNormal, Start) + Plane[3];
	const float t2 = lcDot(PlaneNormal, Dir);

	if (t2 == 0.0f)
		return false;

	const float t = -t1 / t2;

	*Intersection = Start + t * Dir;

	if ((t < 0.0f) || (t > 1.0f))
		return false;

	return true;
}

inline bool lcLineTriangleMinIntersection(const lcVector3& p1, const lcVector3& p2, const lcVector3& p3, const lcVector3& Start, const lcVector3& End, float* MinDist, lcVector3* Intersection)
{
	// Calculate the polygon plane.
	const lcVector3 PlaneNormal = lcCross(p1 - p2, p3 - p2);
	const float PlaneD = -lcDot(PlaneNormal, p1);

	// Check if the line is parallel to the plane.
	const lcVector3 Dir = End - Start;

	const float t1 = lcDot(PlaneNormal, Start) + PlaneD;
	const float t2 = lcDot(PlaneNormal, Dir);

	if (t2 == 0)
		return false;

	const float t = -(t1 / t2);

	if (t < 0)
		return false;

	// Intersection of the plane and line segment.
	*Intersection = Start - (t1 / t2) * Dir;

	float Dist = lcLength(Start - *Intersection);

	if (Dist > *MinDist)
		return false;

	// Check if we're inside the triangle.
	lcVector3 pa1, pa2, pa3;
	pa1 = lcNormalize(p1 - *Intersection);
	pa2 = lcNormalize(p2 - *Intersection);
	pa3 = lcNormalize(p3 - *Intersection);

	float a1, a2, a3;
	a1 = lcDot(pa1, pa2);
	a2 = lcDot(pa2, pa3);
	a3 = lcDot(pa3, pa1);

	const float total = (acosf(a1) + acosf(a2) + acosf(a3)) * LC_RTOD;

	if (fabs(total - 360) <= 0.001f)
	{
		*MinDist = Dist;
		return true;
	}

	return false;
}

// Sutherland-Hodgman method of clipping a polygon to a plane.
inline void lcPolygonPlaneClip(lcVector3* InPoints, int NumInPoints, lcVector3* OutPoints, int* NumOutPoints, const lcVector4& Plane)
{
	lcVector3 *s, *p, i;

	*NumOutPoints = 0;
	s = &InPoints[NumInPoints-1];

	for (int j = 0; j < NumInPoints; j++)
	{
		p = &InPoints[j];

		if (lcDot3(*p, Plane) + Plane[3] <= 0)
		{
			if (lcDot3(*s, Plane) + Plane[3] <= 0)
			{
				// Both points inside.
				OutPoints[*NumOutPoints] = *p;
				*NumOutPoints = *NumOutPoints + 1;
			}
			else
			{
				// Outside, inside.
				lcLineSegmentPlaneIntersection(&i, *s, *p, Plane);

				OutPoints[*NumOutPoints] = i;
				*NumOutPoints = *NumOutPoints + 1;
				OutPoints[*NumOutPoints] = *p;
				*NumOutPoints = *NumOutPoints + 1;
			}
		}
		else
		{
			if (lcDot3(*s, Plane) + Plane[3] <= 0)
			{
				// Inside, outside.
				lcLineSegmentPlaneIntersection(&i, *s, *p, Plane);

				OutPoints[*NumOutPoints] = i;
				*NumOutPoints = *NumOutPoints + 1;
			}
		}

		s = p;
	}
}

// Return true if a polygon intersects a set of planes.
inline bool lcTriangleIntersectsPlanes(const lcVector3& p1, const lcVector3& p2, const lcVector3& p3, const lcVector4 (&Planes)[6])
{
	constexpr int NumPlanes = 6;
	const lcVector3 Points[3] = { p1, p2, p3 };
	int Outcodes[3] = { 0, 0, 0 }, i;
	constexpr int NumPoints = 3;

	// First do the Cohen-Sutherland out code test for trivial rejects/accepts.
	for (i = 0; i < NumPoints; i++)
	{
		const lcVector3 Pt(Points[i][0], Points[i][1], Points[i][2]);

		for (int j = 0; j < NumPlanes; j++)
		{
			if (lcDot3(Pt, Planes[j]) + Planes[j][3] > 0)
				Outcodes[i] |= 1 << j;
		}
	}

	// Polygon completely outside a plane.
	if ((Outcodes[0] & Outcodes[1] & Outcodes[2]) != 0)
		return false;

	// If any vertex has an out code of all zeros then we intersect the volume.
	if (!Outcodes[0] || !Outcodes[1] || !Outcodes[2])
		return true;

	// Buffers for clipping the polygon.
	lcVector3 ClipPoints[2][8];
	int NumClipPoints[2];
	int ClipBuffer = 0;

	NumClipPoints[0] = NumPoints;
	ClipPoints[0][0] = lcVector3(p1[0], p1[1], p1[2]);
	ClipPoints[0][1] = lcVector3(p2[0], p2[1], p2[2]);
	ClipPoints[0][2] = lcVector3(p3[0], p3[1], p3[2]);

	// Now clip the polygon against the planes.
	for (i = 0; i < NumPlanes; i++)
	{
		lcPolygonPlaneClip(ClipPoints[ClipBuffer], NumClipPoints[ClipBuffer], ClipPoints[ClipBuffer^1], &NumClipPoints[ClipBuffer^1], Planes[i]);
		ClipBuffer ^= 1;

		if (!NumClipPoints[ClipBuffer])
			return false;
	}

	return true;
}

// Return true if a ray intersects a bounding box, and calculates the distance from the start of the ray (adapted from Graphics Gems).
inline bool lcBoundingBoxRayIntersectDistance(const lcVector3& Min, const lcVector3& Max, const lcVector3& Start, const lcVector3& End, float* Dist, lcVector3* Intersection, lcVector3* Plane)
{
	bool MiddleQuadrant[3];
	bool Inside = true;
	float CandidatePlane[3];
	float MaxT[3];
	int i;

	// Find candidate planes.
	for (i = 0; i < 3; i++)
	{
		if (Start[i] < Min[i])
		{
			MiddleQuadrant[i] = false;
			CandidatePlane[i] = Min[i];
			Inside = false;
		}
		else if (Start[i] > Max[i])
		{
			MiddleQuadrant[i] = false;
			CandidatePlane[i] = Max[i];
			Inside = false;
		}
		else
		{
			MiddleQuadrant[i] = true;
			CandidatePlane[i] = 0.0f;
		}
	}

	// Ray origin inside box.
	if (Inside)
	{
		*Dist = 0;

		if (Intersection)
			*Intersection = Start;

		if (Plane)
			*Plane = Start;

		return true;
	}

	// Calculate T distances to candidate planes.
	lcVector3 Dir = End - Start;

	for (i = 0; i < 3; i++)
	{
		if (!MiddleQuadrant[i] && Dir[i] != 0.0f)
			MaxT[i] = (CandidatePlane[i] - Start[i]) / Dir[i];
		else
			MaxT[i] = -1.0f;
	}

	// Get largest of the MaxT's for final choice of intersection.
	int WhichPlane = 0;
	for (i = 1; i < 3; i++)
		if (MaxT[WhichPlane] < MaxT[i])
			WhichPlane = i;

	// Check final candidate actually inside box.
	if (MaxT[WhichPlane] < 0.0f)
		return false;

	lcVector3 Point;

	for (i = 0; i < 3; i++)
	{
		if (WhichPlane != i)
		{
			Point[i] = Start[i] + MaxT[WhichPlane] * Dir[i];
			if (Point[i] < Min[i] || Point[i] > Max[i])
				return false;
		}
		else
			Point[i] = CandidatePlane[i];
	}

	*Dist = lcLength(Point - Start);

	if (Intersection)
		*Intersection = Point;

	if (Plane)
	{
		*Plane = lcVector3(0.0f, 0.0f, 0.0f);
		(*Plane)[WhichPlane] = CandidatePlane[WhichPlane];
	}

	return true;
}

inline bool lcSphereRayIntersection(const lcVector3& Center, float Radius, const lcVector3& Start, const lcVector3& End, lcVector3& Intersection)
{
	const lcVector3 RayDirection = lcNormalize(End - Start);
	const lcVector3 RayCenter = Center - Start;
	const float RayCenterSquared = lcDot(RayCenter, RayCenter);
	const float ClosestApproach = lcDot(RayCenter, RayDirection);

	if (ClosestApproach < 0)
		return false;

	const float HalfCordSquared = (Radius * Radius) - RayCenterSquared + (ClosestApproach * ClosestApproach);

	if (HalfCordSquared < 0)
		return false;

	Intersection = Start + RayDirection * (ClosestApproach - sqrtf(HalfCordSquared));

	return true;
}

inline bool lcSphereRayMinIntersectDistance(const lcVector3& Center, float Radius, const lcVector3& Start, const lcVector3& End, float* Dist)
{
	const lcVector3 Dir = Center - Start;
	const float LengthSquaredDir = lcLengthSquared(Dir);
	const float RadiusSquared = Radius * Radius;

	if (LengthSquaredDir < RadiusSquared)
	{
		// Ray origin inside sphere.
		*Dist = 0;
		return true;
	}
	else
	{
		const lcVector3 RayDir = End - Start;
		float t = lcDot(Dir, RayDir) / lcLengthSquared(RayDir);

		// Ray points away from sphere.
		if (t < 0)
			return false;

		const float c = (RadiusSquared - LengthSquaredDir) / lcLengthSquared(RayDir) + (t * t);
		if (c > 0)
		{
			*Dist = t - sqrtf(c);
			return true;
		}

		return false;
	}
}

inline bool lcConeRayMinIntersectDistance(const lcVector3& Tip, const lcVector3& Direction, float Radius, float Height, const lcVector3& Start, const lcVector3& End, float* Dist)
{
	const lcVector3 v = End - Start;
	const lcVector3 h = Direction;
	const lcVector3 w = Start - Tip;
	const float vh = lcDot(v, h);
	const float wh = lcDot(w, h);
	const float m = (Radius * Radius) / (Height * Height);

	const float a = lcDot(v, v) - m * vh * vh - vh * vh;
	const float b = 2 * (lcDot(v, w) - m * vh * wh - vh * wh);
	const float c = lcDot(w, w) - m * wh * wh - wh * wh;

	const float delta = b * b - 4 * a * c;

	if (delta < 0.0f)
		return false;

	float ts[2] = { (-b - sqrtf(delta)) / (2 * a), (-b + sqrtf(delta)) / (2 * a) };

	for (int ti = 0; ti < 2; ti++)
	{
		float t = ts[ti];
		lcVector3 Intersection = Start + v * t;

		float ConeD = lcDot(Intersection - Tip, Direction);

		if (ConeD < 0.0f)
			continue;

		if (ConeD < Height)
		{
			*Dist = lcLength(Intersection - Start);

			return true;
		}

		lcVector3 Center(Tip + Direction * Height);
		lcVector4 Plane(Direction, -lcDot(Direction, Center));

		if (lcLineSegmentPlaneIntersection(&Intersection, Start, End, Plane))
		{
			if (lcLengthSquared(Center - Intersection) < Radius * Radius)
			{
				*Dist = lcLength(Intersection - Start);

				return true;
			}
		}
	}

	return false;
}

inline bool lcCylinderRayMinIntersectDistance(float Radius, float Height, const lcVector3& Start, const lcVector3& End, float* Dist)
{
	lcVector4 BottomPlane(0.0f, 0.0f, 1.0f, 0.0f);
	lcVector3 Intersection;
	float MinDistance = FLT_MAX;

	if (lcLineSegmentPlaneIntersection(&Intersection, Start, End, BottomPlane))
	{
		if (Intersection.x * Intersection.x + Intersection.y * Intersection.y < Radius * Radius)
		{
			float Distance = lcLength(Intersection - Start);

			if (Distance < MinDistance)
				MinDistance = Distance;
		}
	}

	lcVector4 TopPlane(0.0f, 0.0f, 1.0f, -Height);

	if (lcLineSegmentPlaneIntersection(&Intersection, Start, End, TopPlane))
	{
		if (Intersection.x * Intersection.x + Intersection.y * Intersection.y < Radius * Radius)
		{
			float Distance = lcLength(Intersection - Start);

			if (Distance < MinDistance)
				MinDistance = Distance;
		}
	}

	lcVector3 Direction = End - Start;

	float a = (Direction.x * Direction.x) + (Direction.y * Direction.y);
	float b = 2 * (Direction.x * Start.x + Direction.y * Start.y);
	float c = (Start.x * Start.x) + (Start.y * Start.y) - (Radius * Radius);

	float delta = b * b - 4 * (a * c);

	if (delta > 0.0f)
	{
		float ts[2] = { (-b - sqrtf(delta)) / (2 * a), (-b + sqrtf(delta)) / (2 * a) };

		for (int ti = 0; ti < 2; ti++)
		{
			float t = ts[ti];
			Intersection = Start + Direction * t;

			if (Intersection.z < 0.0f || Intersection.z > Height)
				continue;

			float Distance = lcLength(Intersection - Start);

			if (Distance < MinDistance)
				MinDistance = Distance;
		}
	}

	if (MinDistance == FLT_MAX)
		return false;

	*Dist = MinDistance;

	return true;
}

inline lcVector3 lcRayPointClosestPoint(const lcVector3& Point, const lcVector3& Start, const lcVector3& End)
{
	const lcVector3 Dir = Point - Start;
	const lcVector3 RayDir = End - Start;

	float t = lcDot(Dir, RayDir) / lcLengthSquared(RayDir);
	t = lcClamp(t, 0.0f, 1.0f);

	return Start + t * RayDir;
}

inline float lcRayPointDistance(const lcVector3& Point, const lcVector3& Start, const lcVector3& End)
{
	const lcVector3 Closest = lcRayPointClosestPoint(Point, Start, End);

	return lcLength(Closest - Point);
}

// Returns true if the axis aligned box intersects the volume defined by planes.
inline bool lcBoundingBoxIntersectsVolume(const lcVector3& Min, const lcVector3& Max, const lcVector4 (&Planes)[6])
{
	constexpr int NumPlanes = 6;
	lcVector3 Points[8] =
	{
		Points[0] = lcVector3(Min[0], Min[1], Min[2]),
		Points[1] = lcVector3(Min[0], Max[1], Min[2]),
		Points[2] = lcVector3(Max[0], Max[1], Min[2]),
		Points[3] = lcVector3(Max[0], Min[1], Min[2]),
		Points[4] = lcVector3(Min[0], Min[1], Max[2]),
		Points[5] = lcVector3(Min[0], Max[1], Max[2]),
		Points[6] = lcVector3(Max[0], Max[1], Max[2]),
		Points[7] = lcVector3(Max[0], Min[1], Max[2])
	};

	// Start by testing trivial reject/accept cases.
	int Outcodes[8];
	int i;

	for (i = 0; i < 8; i++)
	{
		Outcodes[i] = 0;

		for (int j = 0; j < NumPlanes; j++)
		{
			if (lcDot3(Points[i], Planes[j]) + Planes[j][3] > 0)
				Outcodes[i] |= 1 << j;
		}
	}

	int OutcodesOR = 0, OutcodesAND = 0x3f;

	for (i = 0; i < 8; i++)
	{
		OutcodesAND &= Outcodes[i];
		OutcodesOR |= Outcodes[i];
	}

	// All corners outside the same plane.
	if (OutcodesAND != 0)
		return false;

	// All corners inside the volume.
	if (OutcodesOR == 0)
		return true;

	int Indices[36] =
	{
		0, 1, 2,
		0, 2, 3,
		7, 6, 5,
		7, 5, 4,
		0, 1, 5,
		0, 5, 4,
		2, 3, 7,
		2, 7, 6,
		0, 3, 7,
		0, 7, 4,
		1, 2, 6,
		1, 6, 5
	};

	for (int Idx = 0; Idx < 36; Idx += 3)
		if (lcTriangleIntersectsPlanes(Points[Indices[Idx]*3], Points[Indices[Idx+1]*3], Points[Indices[Idx+2]*3], Planes))
			return true;

	return false;
}

struct lcBoundingBox
{
	lcVector3 Min;
	lcVector3 Max;
};

inline void lcGetBoxCorners(const lcVector3& Min, const lcVector3& Max, lcVector3 (&Points)[8])
{
	Points[0] = lcVector3(Max.x, Max.y, Min.z);
	Points[1] = lcVector3(Min.x, Max.y, Min.z);
	Points[2] = lcVector3(Max.x, Max.y, Max.z);
	Points[3] = lcVector3(Min.x, Min.y, Min.z);
	Points[4] = lcVector3(Min.x, Min.y, Max.z);
	Points[5] = lcVector3(Max.x, Min.y, Max.z);
	Points[6] = lcVector3(Max.x, Min.y, Min.z);
	Points[7] = lcVector3(Min.x, Max.y, Max.z);
}

inline void lcGetBoxCorners(const lcBoundingBox& BoundingBox, lcVector3 (&Points)[8])
{
	lcGetBoxCorners(BoundingBox.Min, BoundingBox.Max, Points);
}

lcVector3 lcRGBToHSL(const lcVector3& rgb);

lcVector3 lcHSLToRGB(const lcVector3& hSL);

lcVector4 lcAlgorithmicEdgeColor(const lcVector3& Value, const float ValueLum, const float EdgeLum, const float Contrast, const float Saturation);

Q_DECLARE_METATYPE(lcVector3);
