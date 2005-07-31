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

	m_Rows[0] = Vector4(x.GetX(), y.GetX(), z.GetX(), 0.0f);
	m_Rows[1] = Vector4(x.GetY(), y.GetY(), z.GetY(), 0.0f);
	m_Rows[2] = Vector4(x.GetZ(), y.GetZ(), z.GetZ(), 0.0f);
	m_Rows[3] = m_Rows[0]*-Eye.GetX() + m_Rows[1]*-Eye.GetY() + m_Rows[2]*-Eye.GetZ();
	m_Rows[3][3] = 1.0f;
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

	m_Rows[0] = Vector4(x, 0, 0,  0);
	m_Rows[1] = Vector4(0, y, 0,  0);
	m_Rows[2] = Vector4(a, b, c, -1);
	m_Rows[3] = Vector4(0, 0, d,  0);
}

// Inverse code from the GLU library.
Matrix44 Inverse(const Matrix44& m)
{
#define SWAP_ROWS(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,c,r) m.m_Rows[r][c]

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

	Matrix44 out(Vector4(r0[4], r1[4], r2[4], r3[4]), 
	             Vector4(r0[5], r1[5], r2[5], r3[5]), 
	             Vector4(r0[6], r1[6], r2[6], r3[6]), 
	             Vector4(r0[7], r1[7], r2[7], r3[7]));

	return out;

#undef MAT
#undef SWAP_ROWS
}

// ============================================================================
// Project/Unproject a point.

// Convert world coordinates to screen coordinates.
Point3 ProjectPoint(const Point3& Pt, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4])
{
	Vector4 Tmp;

	Tmp = Vector4(Pt) * ModelView;
	Tmp = Tmp * Projection;

	// Normalize.
	Tmp /= Tmp[3];

	// Screen coordinates.
	return Point3(Viewport[0]+(1+Tmp[0])*Viewport[2]/2, Viewport[1]+(1+Tmp[1])*Viewport[3]/2, (1+Tmp[2])/2);
}

// Convert screen coordinates to world coordinates.
Point3 UnprojectPoint(const Point3& Point, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4])
{
	Point3 Tmp = Point;
	UnprojectPoints(&Tmp, 1, ModelView, Projection, Viewport);
	return Tmp;
}

void UnprojectPoints(Point3* Points, int NumPoints, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4])
{
	// Calculate the screen to model transform.
	Matrix44 Transform = Inverse(ModelView * Projection);

	for (int i = 0; i < NumPoints; i++)
	{
		Vector4 Tmp;

		// Convert the point to homogeneous coordinates.
		Tmp[0] = (Points[i][0] - Viewport[0]) * 2.0f / Viewport[2] - 1.0f;
		Tmp[1] = (Points[i][1] - Viewport[1]) * 2.0f / Viewport[3] - 1.0f;
		Tmp[2] = Points[i][2] * 2.0f - 1.0f;
		Tmp[3] = 1.0f;

		Tmp = Tmp * Transform;

		if (Tmp[3] != 0.0f)
			Tmp /= Tmp[3];

		Points[i] = Point3(Tmp[0], Tmp[1], Tmp[2]);
	}
}

// ============================================================================
// Geometry functions.

// Sutherland-Hodgman method of clipping a polygon to a plane.
void PolygonPlaneClip(Point3* InPoints, int NumInPoints, Point3* OutPoints, int* NumOutPoints, const Vector4& Plane)
{
	Point3 *s, *p, i;

	*NumOutPoints = 0;
	s = &InPoints[NumInPoints-1];

	for (int j = 0; j < NumInPoints; j++)
	{
		p = &InPoints[j];

		if (Dot3(*p, Plane) + Plane[3] <= 0)
		{
			if (Dot3(*s, Plane) + Plane[3] <= 0)
			{
				// Both points inside.
				OutPoints[*NumOutPoints] = *p;
				*NumOutPoints = *NumOutPoints + 1;
			}
			else
			{
				// Outside, inside.
				LinePlaneIntersection(i, *s, *p, Plane);

				OutPoints[*NumOutPoints] = i;
				*NumOutPoints = *NumOutPoints + 1;
				OutPoints[*NumOutPoints] = *p;
				*NumOutPoints = *NumOutPoints + 1;
			}
		}
		else
		{
			if (Dot3(*s, Plane) + Plane[3] <= 0)
			{
				// Inside, outside.
				LinePlaneIntersection(i, *s, *p, Plane);

				OutPoints[*NumOutPoints] = i;
				*NumOutPoints = *NumOutPoints + 1;
			}
		}

		s = p;
	}
}

// Return the intersction point of a line and a plane, or false if they are parallel.
bool LinePlaneIntersection(Point3& Intersection, const Point3& Start, const Point3& End, const Vector4& Plane)
{
	Vector3 Dir = End - Start;

	float t1 = Dot3(Plane, Start) + Plane[3];
	float t2 = Dot3(Plane, Dir);

	if (t2 == 0.0f)
		return false;

	Intersection = Start - (t1 / t2) * Dir;

	return true;
}

