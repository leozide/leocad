//
// Math and Linear Algebra stuff.
//

#include <float.h>
#include "defines.h"
#include "algebra.h"

// ============================================================================
// 4x4 Matrix class.

void Matrix44::CreateLookAt(const Vector3& Eye, const Vector3& Target, const Vector3& Up)
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

	m_Rows[0] = Vector4(x[0], y[0], z[0], 0.0f);
	m_Rows[1] = Vector4(x[1], y[1], z[1], 0.0f);
	m_Rows[2] = Vector4(x[2], y[2], z[2], 0.0f);
	m_Rows[3] = m_Rows[0]*-Eye[0] + m_Rows[1]*-Eye[1] + m_Rows[2]*-Eye[2];
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

void Matrix44::CreateOrtho(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
	m_Rows[0] = Vector4(2.0f / (Right-Left), 0.0f, 0.0f, 0.0f);
	m_Rows[1] = Vector4(0.0f, 2.0f / (Top-Bottom), 0.0f, 0.0f);
	m_Rows[2] = Vector4(0.0f, 0.0f, -2.0f / (Far-Near), 0.0f);
	m_Rows[3] = Vector4(-(Right+Left) / (Right-Left), -(Top+Bottom) / (Top-Bottom), -(Far+Near) / (Far-Near), 1.0f);
}

void GetFrustumPlanes(const Matrix44& WorldView, const Matrix44& Projection, Vector4 Planes[6])
{
	// TODO: Use vectors.
	Matrix44 WorldProj = Mul(WorldView, Projection);

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
		float Len = Vector3(Planes[i]).Length();
		Planes[i] /= -Len;
	}
}

Vector3 ZoomExtents(const Vector3& Position, const Matrix44& WorldView, const Matrix44& Projection, const Vector3* Points, int NumPoints)
{
	if (!NumPoints)
		return Position;

	Vector4 Planes[6];
	GetFrustumPlanes(WorldView, Projection, Planes);

	Vector3 Front = Vector3(WorldView[0][2], WorldView[1][2], WorldView[2][2]);

	// Calculate the position that is as close as possible to the model and has all pieces visible.
	float SmallestDistance = FLT_MAX;

	for (int p = 0; p < 4; p++)
	{
		float ep = Dot3(Position, Planes[p]);
		float fp = Dot3(Front, Planes[p]);

		for (int j = 0; j < NumPoints; j++)
		{
			// Intersect the camera line with the plane that contains this point, NewEye = Eye + u * (Target - Eye)
			float u = (ep - Dot3(Points[j], Planes[p])) / fp;

			if (u < SmallestDistance)
				SmallestDistance = u;
		}
	}

	return Position - (Front * SmallestDistance);
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

	Vector4 Row0(r0[4], r1[4], r2[4], r3[4]);
	Vector4 Row1(r0[5], r1[5], r2[5], r3[5]);
	Vector4 Row2(r0[6], r1[6], r2[6], r3[6]);
	Vector4 Row3(r0[7], r1[7], r2[7], r3[7]);
	
	Matrix44 out(Row0, Row1, Row2, Row3);
	
	return out;

#undef MAT
#undef SWAP_ROWS
}

// ============================================================================
// Project/Unproject a point.

// Convert world coordinates to screen coordinates.
Vector3 ProjectPoint(const Vector3& Pt, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4])
{
	Vector4 Tmp;

	Tmp = Mul4(Vector4(Pt[0], Pt[1], Pt[2], 1.0f), ModelView);
	Tmp = Mul4(Tmp, Projection);

	// Normalize.
	Tmp /= Tmp[3];

	// Screen coordinates.
	return Vector3(Viewport[0]+(1+Tmp[0])*Viewport[2]/2, Viewport[1]+(1+Tmp[1])*Viewport[3]/2, (1+Tmp[2])/2);
}

void ProjectPoints(Vector3* Points, int NumPoints, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4])
{
	for (int i = 0; i < NumPoints; i++)
	{
		Vector4 Tmp;

		Tmp = Mul4(Vector4(Points[i][0], Points[i][1], Points[i][2], 1.0f), ModelView);
		Tmp = Mul4(Tmp, Projection);

		// Normalize.
		Tmp /= Tmp[3];

		// Screen coordinates.
		Points[i] = Vector3(Viewport[0]+(1+Tmp[0])*Viewport[2]/2, Viewport[1]+(1+Tmp[1])*Viewport[3]/2, (1+Tmp[2])/2);
	}
}

// Convert screen coordinates to world coordinates.
Vector3 UnprojectPoint(const Vector3& Point, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4])
{
	Vector3 Tmp = Point;
	UnprojectPoints(&Tmp, 1, ModelView, Projection, Viewport);
	return Tmp;
}

void UnprojectPoints(Vector3* Points, int NumPoints, const Matrix44& ModelView, const Matrix44& Projection, const int Viewport[4])
{
	// Calculate the screen to model transform.
	Matrix44 Transform = Inverse(Mul(ModelView, Projection));

	for (int i = 0; i < NumPoints; i++)
	{
		Vector4 Tmp;

		// Convert the point to homogeneous coordinates.
		Tmp[0] = (Points[i][0] - Viewport[0]) * 2.0f / Viewport[2] - 1.0f;
		Tmp[1] = (Points[i][1] - Viewport[1]) * 2.0f / Viewport[3] - 1.0f;
		Tmp[2] = Points[i][2] * 2.0f - 1.0f;
		Tmp[3] = 1.0f;

		Tmp = Mul4(Tmp, Transform);

		if (Tmp[3] != 0.0f)
			Tmp /= Tmp[3];

		Points[i] = Vector3(Tmp[0], Tmp[1], Tmp[2]);
	}
}

// ============================================================================
// Geometry functions.

// Sutherland-Hodgman method of clipping a polygon to a plane.
void PolygonPlaneClip(Vector3* InPoints, int NumInPoints, Vector3* OutPoints, int* NumOutPoints, const Vector4& Plane)
{
	Vector3 *s, *p, i;

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

// Calculate the intersection of a line segment and a plane and returns false
// if they are parallel or the intersection is outside the line segment.
bool LinePlaneIntersection(Vector3& Intersection, const Vector3& Start, const Vector3& End, const Vector4& Plane)
{
	Vector3 Dir = End - Start;

	float t1 = Dot3(Plane, Start) + Plane[3];
	float t2 = Dot3(Plane, Dir);

	if (t2 == 0.0f)
		return false;

	float t = -t1 / t2;

	Intersection = Start + t * Dir;

	if ((t < 0.0f) || (t > 1.0f))
		return false;

	return true;
}

bool LineTriangleMinIntersection(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& Start, const Vector3& End, float& MinDist, Vector3& Intersection)
{
	// Calculate the polygon plane.
	Vector4 Plane;
	Plane = Cross3(p1 - p2, p3 - p2);
	Plane[3] = -Dot3(Plane, p1);

	// Check if the line is parallel to the plane.
	Vector3 Dir = End - Start;

	float t1 = Dot3(Plane, Start) + Plane[3];
	float t2 = Dot3(Plane, Dir);

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
	Vector3 pa1, pa2, pa3;
	pa1 = (p1 - Intersection).Normalize();
	pa2 = (p2 - Intersection).Normalize();
	pa3 = (p3 - Intersection).Normalize();

	float a1, a2, a3;
	a1 = Dot3(pa1, pa2);
	a2 = Dot3(pa2, pa3);
	a3 = Dot3(pa3, pa1);

	float total = (acosf(a1) + acosf(a2) + acosf(a3)) * RTOD;

	if (fabs(total - 360) <= 0.001f)
	{
		MinDist = Dist;
		return true;
	}

	return false;
}

bool LineQuadMinIntersection(const Vector3& p1, const Vector3& p2, const Vector3& p3, const Vector3& p4, const Vector3& Start, const Vector3& End, float& MinDist, Vector3& Intersection)
{
	// Calculate the polygon plane.
	Vector4 Plane;
	Plane = Cross3(p1 - p2, p3 - p2);
	Plane[3] = -Dot3(Plane, p1);

	// Check if the line is parallel to the plane.
	Vector3 Dir = End - Start;

	float t1 = Dot3(Plane, Start) + Plane[3];
	float t2 = Dot3(Plane, Dir);

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
	Vector3 pa1, pa2, pa3;
	pa1 = (p1 - Intersection).Normalize();
	pa2 = (p2 - Intersection).Normalize();
	pa3 = (p3 - Intersection).Normalize();

	float a1, a2, a3;
	a1 = Dot3(pa1, pa2);
	a2 = Dot3(pa2, pa3);
	a3 = Dot3(pa3, pa1);

	float total = (acosf(a1) + acosf(a2) + acosf(a3)) * RTOD;

	if (fabs(total - 360) <= 0.001f)
	{
		MinDist = Dist;
		return true;
	}

	// Check if we're inside the second triangle.
	pa2 = (p4 - Intersection).Normalize();

	a1 = Dot3(pa1, pa2);
	a2 = Dot3(pa2, pa3);
	a3 = Dot3(pa3, pa1);

	total = (acosf(a1) + acosf(a2) + acosf(a3)) * RTOD;
			
	if (fabs(total - 360) <= 0.001f)
	{
		MinDist = Dist;
		return true;
	}

	return false;
}
