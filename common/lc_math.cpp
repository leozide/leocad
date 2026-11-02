#include "lc_global.h"
#include "lc_math.h"

#ifndef QT_NO_DEBUG

QDebug operator<<(QDebug Debug, const lcVector2& v)
{
	QDebugStateSaver Saver(Debug);
	Debug.nospace() << '(' << v.x << ", " << v.y << ')';
	return Debug;
}

QDebug operator<<(QDebug Debug, const lcVector3& v)
{
	QDebugStateSaver Saver(Debug);
	Debug.nospace() << '(' << v.x << ", " << v.y << ", " << v.z << ')';
	return Debug;
}

QDebug operator<<(QDebug Debug, const lcVector4& v)
{
	QDebugStateSaver Saver(Debug);
	Debug.nospace() << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')';
	return Debug;
}

QDebug operator<<(QDebug Debug, const lcMatrix33& m)
{
	QDebugStateSaver Saver(Debug);
	Debug.nospace() << '[' << m[0] << ", " << m[1] << ", " << m[2] << ']';
	return Debug;
}

QDebug operator<<(QDebug Debug, const lcMatrix44& m)
{
	QDebugStateSaver Saver(Debug);
	Debug.nospace() << '[' << m[0] << ", " << m[1] << ", " << m[2] << ", " << m[3] << ']';
	return Debug;
}

#endif

QDataStream& operator<<(QDataStream& Stream, const lcVector3& v)
{
	Stream << v.x << v.y << v.z;
	return Stream;
}

QDataStream& operator>>(QDataStream& Stream, lcVector3& v)
{
	Stream >> v.x >> v.y >> v.z;
	return Stream;
}

QDataStream& operator<<(QDataStream& Stream, const lcVector4& v)
{
	Stream << v.x << v.y << v.z << v.w;
	return Stream;
}

QDataStream& operator>>(QDataStream& Stream, lcVector4& v)
{
	Stream >> v.x >> v.y >> v.z >> v.w;
	return Stream;
}

QDataStream& operator<<(QDataStream& Stream, const lcMatrix33& m)
{
	Stream << m[0] << m[1] << m[2];
	return Stream;
}

QDataStream& operator>>(QDataStream& Stream, lcMatrix33& m)
{
	Stream >> m[0] >> m[1] >> m[2];
	return Stream;
}

QDataStream& operator<<(QDataStream& Stream, const lcMatrix44& m)
{
	Stream << m[0] << m[1] << m[2] << m[3];
	return Stream;
}

QDataStream& operator>>(QDataStream& Stream, lcMatrix44& m)
{
	Stream >> m[0] >> m[1] >> m[2] >> m[3];
	return Stream;
}

lcVector4 lcMatrix44ToAxisAngle(const lcMatrix44& m)
{
	lcVector3 Rows[3];
	Rows[0] = lcNormalize(lcVector3(m.r[0][0], m.r[0][1], m.r[0][2]));
	Rows[1] = lcNormalize(lcVector3(m.r[1][0], m.r[1][1], m.r[1][2]));
	Rows[2] = lcNormalize(lcVector3(m.r[2][0], m.r[2][1], m.r[2][2]));

	if (m.Determinant() < 0.0f)
		Rows[0] *= -1.0f;

	const float Trace = Rows[0][0] + Rows[1][1] + Rows[2][2];
	const float Cos = 0.5f * (Trace - 1.0f);
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

// Inverse code from the GLU library.
lcMatrix44 lcMatrix44Inverse(const lcMatrix44& m)
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

	const lcVector4 Row0(r0[4], r1[4], r2[4], r3[4]);
	const lcVector4 Row1(r0[5], r1[5], r2[5], r3[5]);
	const lcVector4 Row2(r0[6], r1[6], r2[6], r3[6]);
	const lcVector4 Row3(r0[7], r1[7], r2[7], r3[7]);

	lcMatrix44 out(Row0, Row1, Row2, Row3);

	return out;

#undef MAT
#undef SWAP_ROWS
}

lcVector3 lcProjectPoint(const lcVector3& Point, const lcMatrix44& ModelView, const lcMatrix44& Projection, const int (&Viewport)[4])
{
	lcVector4 Tmp;

	Tmp = lcMul4(lcVector4(Point[0], Point[1], Point[2], 1.0f), ModelView);
	Tmp = lcMul4(Tmp, Projection);

	// Normalize.
	Tmp /= Tmp[3];

	// Screen coordinates.
	return lcVector3(Viewport[0] + (1 + Tmp[0]) * Viewport[2] / 2, Viewport[1] + (1 + Tmp[1]) * Viewport[3] / 2, (1 + Tmp[2]) / 2);
}

lcVector3 lcUnprojectPoint(const lcVector3& Point, const lcMatrix44& ModelView, const lcMatrix44& Projection, const int (&Viewport)[4])
{
	// Calculate the screen to model transform.
	const lcMatrix44 Transform = lcMatrix44Inverse(lcMul(ModelView, Projection));

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

void lcUnprojectPoints(lcVector3* Points, int NumPoints, const lcMatrix44& ModelView, const lcMatrix44& Projection, const int (&Viewport)[4])
{
	// Calculate the screen to model transform.
	const lcMatrix44 Transform = lcMatrix44Inverse(lcMul(ModelView, Projection));

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

void lcGetFrustumPlanes(const lcMatrix44& WorldView, const lcMatrix44& Projection, lcVector4 (&Planes)[6])
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
		const lcVector3 Normal(Planes[i][0], Planes[i][1], Planes[i][2]);
		const float Length = Normal.Length();
		Planes[i] /= -Length;
	}
}

std::tuple<lcVector3, float> lcZoomExtents(const lcVector3& Position, const lcMatrix44& WorldView, const lcMatrix44& Projection, const lcVector3* Points, size_t NumPoints)
{
	if (!NumPoints)
		return std::make_tuple(Position, 2500.0f);

	lcVector4 Planes[6];
	lcGetFrustumPlanes(WorldView, Projection, Planes);

	const lcVector3 Front(WorldView[0][2], WorldView[1][2], WorldView[2][2]);

	float SmallestDistance = FLT_MAX;

	for (int PlaneIdx = 0; PlaneIdx < 4; PlaneIdx++)
	{
		const lcVector3 Plane(Planes[PlaneIdx][0], Planes[PlaneIdx][1], Planes[PlaneIdx][2]);
		const float ep = lcDot(Position, Plane);
		const float fp = lcDot(Front, Plane);

		for (size_t PointIdx = 0; PointIdx < NumPoints; PointIdx++)
		{
			const float u = (ep - lcDot(Points[PointIdx], Plane)) / fp;

			if (u < SmallestDistance)
				SmallestDistance = u;
		}
	}

	lcVector3 NewPosition = Position - (Front * SmallestDistance);

	float FarDistance = 2500.0f;

	for (size_t PointIdx = 0; PointIdx < NumPoints; PointIdx++)
	{
		const float Distance = lcDot(NewPosition - Points[PointIdx], Front);

		if (Distance > FarDistance)
			FarDistance = Distance;
	}

	return std::make_tuple(NewPosition, FarDistance);
}

lcVector3 lcRGBToHSL(const lcVector3& rgb)
{
	int Mi;
	float M, m, C, h, S, L; // h is H/60

	Mi = (rgb[0] >= rgb[1]) ? 0 : 1;
	Mi = (rgb[Mi] >= rgb[2]) ? Mi : 2;
	M = rgb[Mi];

	m = (rgb[0] < rgb[1]) ? rgb[0] : rgb[1];
	m = (m < rgb[2]) ? m : rgb[2];

	C = M - m;
	L = (M + m) / 2.0f;

	if (C < LC_RGB_EPSILON) // C == 0.0
		h = 0.0f;
	else if (Mi == 0)       // M == R
		h = 0.0f + (rgb[1] - rgb[2]) / C;
	else if (Mi == 1)       // M == G
		h = 2.0f + (rgb[2] - rgb[0]) / C;
	else                    // M = B
		h = 4.0f + (rgb[0] - rgb[1]) / C;

	h = (h <  0.0) ? h + 6.0f : h;
	h = (h >= 6.0) ? h - 6.0f : h;

	S = ((L < (LC_RGB_EPSILON / 2.0f)) || (L > (1.0f -(LC_RGB_EPSILON / 2.0f))))
		? 0.0f : (2.0f * (M - L)) / (1.0f - fabs((2.0f * L) - 1.0f)) ;

	return lcVector3(h, S, L);
}

lcVector3 lcHSLToRGB(const lcVector3& hSL)
{
	lcVector3 rgb;
	float h, S, L, C, X, m;

	h = hSL[0];
	S = hSL[1];
	L = hSL[2];

	C = (1.0f - fabs(2.0f * L - 1.0f)) * S;
	X = C * (1.0f - fabs(fmodf(h, 2.0f) - 1.0f));

	if (h < 1.0f)
		rgb = lcVector3(C, X, 0.0f);
	else if (h < 2.0f)
		rgb = lcVector3(X, C, 0.0f);
	else if (h < 3.0f)
		rgb = lcVector3(0.0f, C, X);
	else if (h < 4.0f)
		rgb = lcVector3(0.0f, X, C);
	else if (h < 5.0f)
		rgb = lcVector3(X, 0.0f, C);
	else
		rgb = lcVector3(C, 0.0f, X);

	m = L - C / 2.0f;
	rgb += m;

	return rgb;
}

lcVector4 lcAlgorithmicEdgeColor(const lcVector3& Value, const float ValueLum, const float EdgeLum, const float Contrast, const float Saturation)
{
	float y1, yt;
	lcVector3 hSL, rgb1, rgbf;

	// Determine luma target
	if (EdgeLum < ValueLum)
	{
		// Light base color
		yt = ValueLum - Contrast * ValueLum;
	}
	else
	{
		// Dark base color
		yt = ValueLum + Contrast * (1.0f - ValueLum);
	}

	// Get base color in hSL
	hSL = lcRGBToHSL(Value);

	// Adjust saturation
//	sat = 4.0f * sat - 2.0f;
//	if (sat < 0.0f)
//	{
//		sat = -sat;
//		hSL[0] = (hSL[0] < 3.0f) ? hSL[0] + 3.0f : hSL[0] - 3.0f;
//	}
//	sat = (sat > 2.0f) ? 2.0f : sat;
//	if (sat > 1.0f)
//	{
//		// Supersaturate
//		sat -= 1.0f;
//		hSL[1] += sat * (1.0f - hSL[1]);
//	}
//	else
//	{
		// Desaturate
		hSL[1] *= Saturation;
//	}

	// Adjusted color to RGB
	rgb1 = lcHSLToRGB(lcVector3(hSL[0], hSL[1], 0.5f));

	// Fix adjusted color luma to target value
	y1 = lcLuminescence(rgb1);

	if (yt < y1)
	{
		// Make darker via scaling
		rgbf = (yt/y1) * rgb1;
	}
	else
	{
		// Make lighter via scaling anti-color
		rgbf = lcVector3(1.0f, 1.0f, 1.0f) - rgb1;
		rgbf *= (1.0f - yt) / (1.0f - y1);
		rgbf = lcVector3(1.0f, 1.0f, 1.0f) - rgbf;
	}

	return  lcVector4(lcLinearToSRGB(rgbf), 1.0f);
}
