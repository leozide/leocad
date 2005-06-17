//
// Math and Linear Algebra stuff.
//

#include "defines.h"
#include "algebra.h"

// ============================================================================
// 4x4 Matrix class.

void Matrix44::CreateLookAt(const Point3& Eye, const Point3& Target, const Vector3& Up)
{
	Vector3 x, y, z;

	// Z = Eye - Target
	z = Eye - Target;

  // X = Y Cross Z
	x = Cross3(Up, z);

  // Y = Z Cross X
	y = Cross3(z, x);

	// Normalize everything.
	x.Normalize();
	y.Normalize();
	z.Normalize();

	m_Rows[0] = Float4(x.GetX(), y.GetX(), z.GetX(), 0.0f);
	m_Rows[1] = Float4(x.GetY(), y.GetY(), z.GetY(), 0.0f);
	m_Rows[2] = Float4(x.GetZ(), y.GetZ(), z.GetZ(), 0.0f);
	m_Rows[3] = m_Rows[0]*-Eye.GetX() + m_Rows[1]*-Eye.GetY() + m_Rows[2]*-Eye.GetZ();
}

void Matrix44::CreatePerspective(float FoVy, float Aspect, float Near, float Far)
{
	float Left, Right, Bottom, Top;

	Top = Near * (float)tan(FoVy * LC_PI / 360.0f);
	Bottom = -Top;

	Left = Bottom * Aspect;
	Right = Top * Aspect;

	if ((Near <= 0.0f) || (Far <= 0.0f) || (Near == Far) || (Left == Right) || (Top == Bottom))
		return;

	float x, y, a, b, c, d;

	x = (2.0f * Near) / (Right - Left);
	y = (2.0f * Near) / (Top - Bottom);
	a = (Right + Left) / (Right - Left);
	b = (Top + Bottom) / (Top - Bottom);
	c = -(Far + Near) / (Far - Near);
	d = -(2.0f * Far * Near) / (Far - Near);

	m_Rows[0] = Float4(x, 0,  a, 0);
	m_Rows[1] = Float4(0, y,  b, 0);
	m_Rows[2] = Float4(0, 0,  c, d);
	m_Rows[3] = Float4(0, 0, -1, 0);
}

// ============================================================================
// Other Functions.

// Convert object coordinates to screen coordinates.
Point3 ProjectPoint(const Point3& Pt, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4])
{
	Point3 Tmp;

	Tmp = Pt * ModelView;
	Tmp = Tmp * Projection;

	// Normalize.
	Tmp /= Tmp[3];

	// Screen coordinates.
	return Point3(Viewport[0]+(1+Tmp[0])*Viewport[2]/2, Viewport[1]+(1+Tmp[1])*Viewport[3]/2, (1+Tmp[2])/2);
}
