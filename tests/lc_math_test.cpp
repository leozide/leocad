#include "lc_global.h"
#include "lc_tests.h"
#include "lc_math.h"

void lcTests::BoundingBoxVolumeIntersection()
{
	const lcVector4 Planes[6] =
	{
		lcVector4(1, 0, 0, -0.25f), lcVector4(-1, 0, 0, -0.25f),
		lcVector4(0, 1, 0, -2), lcVector4(0, -1, 0, -2),
		lcVector4(0, 0, 1, -2), lcVector4(0, 0, -1, -2)
	};

	// Every corner is outside, so this must reach the face-clipping path.
	QVERIFY(lcBoundingBoxIntersectsVolume(lcVector3(-1, -1, -1), lcVector3(1, 1, 1), Planes));
	QVERIFY(lcBoundingBoxIntersectsVolume(lcVector3(-0.1f, -1, -1), lcVector3(0.1f, 1, 1), Planes));
	QVERIFY(lcBoundingBoxIntersectsVolume(lcVector3(0.25f, -1, -1), lcVector3(1, 1, 1), Planes));
	QVERIFY(!lcBoundingBoxIntersectsVolume(lcVector3(1, -1, -1), lcVector3(2, 1, 1), Planes));

	const lcVector4 FacePlanes[6] =
	{
		lcVector4(1, 0, 0, -0.25f), lcVector4(-1, 0, 0, -0.25f),
		lcVector4(0, 1, 0, -1.25f), lcVector4(0, -1, 0, 0.75f),
		lcVector4(0, 0, 1, -0.25f), lcVector4(0, 0, -1, -0.25f)
	};
	// A small volume crosses the y=1 face without containing any box corners.
	QVERIFY(lcBoundingBoxIntersectsVolume(lcVector3(-1, -1, -1), lcVector3(1, 1, 1), FacePlanes));

	const lcVector4 UnitPlanes[6] =
	{
		lcVector4(1, 0, 0, -1), lcVector4(-1, 0, 0, -1),
		lcVector4(0, 1, 0, -1), lcVector4(0, -1, 0, -1),
		lcVector4(0, 0, 1, -1), lcVector4(0, 0, -1, -1)
	};
	QVERIFY(lcBoundingBoxIntersectsVolume(lcVector3(-2, -2, -2), lcVector3(2, 2, 2), UnitPlanes));

	const float Diagonal = sqrtf(0.5f);
	lcVector4 RotatedPlanes[6] =
	{
		lcVector4(Diagonal, Diagonal, 0, -0.25f), lcVector4(-Diagonal, -Diagonal, 0, -0.25f),
		lcVector4(-Diagonal, Diagonal, 0, -0.25f), lcVector4(Diagonal, -Diagonal, 0, -0.25f),
		lcVector4(0, 0, 1, -0.25f), lcVector4(0, 0, -1, -0.25f)
	};
	QVERIFY(lcBoundingBoxIntersectsVolume(lcVector3(-2, -2, -2), lcVector3(2, 2, 2), RotatedPlanes));
	for (lcVector4& Plane : RotatedPlanes)
	{
		Plane.w = -1.0f - Plane.x * 2.5f;
		Plane *= 3.0f;
	}
	// These boxes are disjoint, but no single volume plane rejects the box.
	QVERIFY(!lcBoundingBoxIntersectsVolume(lcVector3(-1, -1, -1), lcVector3(1, 1, 1), RotatedPlanes));

	// Compare against the independent interval-overlap rule for axis-aligned boxes.
	for (int x = -3; x <= 3; x++)
		for (int y = -3; y <= 3; y++)
			for (int z = -3; z <= 3; z++)
				for (float Size : { 0.25f, 1.0f, 3.0f })
				{
					const lcVector3 Min(x - Size, y - Size, z - Size);
					const lcVector3 Max(x + Size, y + Size, z + Size);
					const bool Expected = Min.x <= 1 && Max.x >= -1 && Min.y <= 1 && Max.y >= -1 && Min.z <= 1 && Max.z >= -1;
					QCOMPARE(lcBoundingBoxIntersectsVolume(Min, Max, UnitPlanes), Expected);
				}
}

void lcTests::MathMatrixRoundTrips()
{
	QVERIFY(12.0f / lcVector3(2, 3, 4) == lcVector3(6, 4, 3));
	const lcVector3 Point(2, -3, 4);
	const lcVector3 Translation(10, -20, 30);
	const lcMatrix44 Transform(lcMatrix33FromAxisAngle(lcVector3(1, 2, 3), 0.7f), Translation);
	QVERIFY(lcLength(lcMul31(lcMul31(Point, Transform), lcMatrix44AffineInverse(Transform)) - Point) < 0.0001f);
	QVERIFY(lcLength(lcMul31(lcMul31(Point, Transform), lcMatrix44Inverse(Transform)) - Point) < 0.0001f);
	for (float Angle : { 0.0f, 0.7f, LC_PI / 2, LC_PI })
	{
		const lcMatrix44 Rotation = lcMatrix44FromAxisAngle(lcVector3(1, 2, 3), Angle);
		const lcVector4 AxisAngle = lcMatrix44ToAxisAngle(Rotation);
		const lcMatrix44 Reconstructed = lcMatrix44FromAxisAngle(lcVector3(AxisAngle), AxisAngle.w);
		for (int Row = 0; Row < 4; Row++)
			for (int Column = 0; Column < 4; Column++)
				QVERIFY(fabsf(Reconstructed[Row][Column] - Rotation[Row][Column]) < 0.001f);
	}

	const lcMatrix44 View = lcMatrix44LookAt(lcVector3(100, -200, 300), lcVector3(0, 0, 0), lcVector3(0, 0, 1));
	const int Viewport[4] = { 20, 30, 800, 600 };
	for (const lcMatrix44& Projection : { lcMatrix44Perspective(45, 4.0f / 3.0f, 10, 1000), lcMatrix44Ortho(-100, 100, -75, 75, 10, 1000) })
	{
		const lcVector3 Screen = lcProjectPoint(Point, View, Projection, Viewport);
		QVERIFY(lcLength(lcUnprojectPoint(Screen, View, Projection, Viewport) - Point) < 0.01f);
		lcVector3 Points[2] = { Screen, Screen };
		lcUnprojectPoints(Points, 2, View, Projection, Viewport);
		for (const lcVector3& Result : Points)
			QVERIFY(lcLength(Result - Point) < 0.01f);
	}
}

void lcTests::MathTriangleIntersections()
{
	const lcVector3 a(0, 0, 0), b(1, 0, 0), c(0, 1, 0);
	for (const lcVector3& Point : { a, b, c, lcVector3(0.5f, 0, 0), lcVector3(0.5f, 0.5f, 0), lcVector3(0.25f, 0.25f, 0) })
		for (bool Reverse : { false, true })
		{
			float Distance = 10;
			lcVector3 Intersection;
			QVERIFY(lcLineTriangleMinIntersection(a, Reverse ? c : b, Reverse ? b : c, Point + lcVector3(0, 0, 1), Point - lcVector3(0, 0, 1), &Distance, &Intersection));
			QCOMPARE(Distance, 1.0f);
			QVERIFY(lcLength(Intersection - Point) < 0.00001f);
		}

	float Distance = 10;
	lcVector3 Intersection;
	QVERIFY(!lcLineTriangleMinIntersection(a, b, c, lcVector3(1, 1, 1), lcVector3(1, 1, -1), &Distance, &Intersection));
	QVERIFY(!lcLineTriangleMinIntersection(a, b, c, lcVector3(0.2f, 0.2f, 1), lcVector3(0.2f, 0.2f, 2), &Distance, &Intersection));
	QVERIFY(!lcLineTriangleMinIntersection(a, a, a, lcVector3(0, 0, 1), lcVector3(0, 0, -1), &Distance, &Intersection));
	Distance = 0.5f;
	QVERIFY(!lcLineTriangleMinIntersection(a, b, c, lcVector3(0.2f, 0.2f, 1), lcVector3(0.2f, 0.2f, -1), &Distance, &Intersection));
	// End defines the ray direction; MinDist provides its distance limit.
	Distance = 10;
	QVERIFY(lcLineTriangleMinIntersection(a, b, c, lcVector3(0.2f, 0.2f, 2), lcVector3(0.2f, 0.2f, 1), &Distance, &Intersection));
	QCOMPARE(Distance, 2.0f);
}

void lcTests::MathTriangleClipping()
{
	lcVector4 Planes[6];
	const float Angles[6] = { 70, 110, 190, 230, 310, 350 };
	for (int Plane = 0; Plane < 6; Plane++)
		Planes[Plane] = lcVector4(cosf(Angles[Plane] * LC_DTOR), sinf(Angles[Plane] * LC_DTOR), Plane % 2 ? 1.0f : -1.0f, -5);

	const lcVector3 a(0, 10, 0), b(-8.660254f, -5, 0), c(8.660254f, -5, 0);
	lcVector3 Points[2][9];
	Points[0][0] = a;
	Points[0][1] = b;
	Points[0][2] = c;
	int Count = 3;
	for (int Plane = 0; Plane < 6; Plane++)
	{
		int NextCount;
		lcPolygonPlaneClip(Points[Plane % 2], Count, Points[(Plane + 1) % 2], &NextCount, Planes[Plane]);
		Count = NextCount;
	}
	QCOMPARE(Count, 9);
	QVERIFY(lcTriangleIntersectsPlanes(a, b, c, Planes));
	QVERIFY(!lcTriangleIntersectsPlanes(a + lcVector3(100, 0, 0), b + lcVector3(100, 0, 0), c + lcVector3(100, 0, 0), Planes));
	lcPolygonPlaneClip(nullptr, 0, nullptr, &Count, Planes[0]);
	QCOMPARE(Count, 0);
}

void lcTests::MathRayIntersections()
{
	float Distance;
	QVERIFY(!lcCylinderRayMinIntersectDistance(1, 2, lcVector3(3, 0, 1), lcVector3(4, 0, 1), &Distance));
	QVERIFY(lcCylinderRayMinIntersectDistance(1, 2, lcVector3(3, 0, 1), lcVector3(2, 0, 1), &Distance));
	QCOMPARE(Distance, 2.0f);
	QVERIFY(lcCylinderRayMinIntersectDistance(1, 2, lcVector3(0, 0, 3), lcVector3(0, 0, 2.5f), &Distance));
	QCOMPARE(Distance, 1.0f);
	QVERIFY(lcCylinderRayMinIntersectDistance(1, 2, lcVector3(-3, 1, 1), lcVector3(-2, 1, 1), &Distance));
	QCOMPARE(Distance, 3.0f);
	QVERIFY(lcCylinderRayMinIntersectDistance(1, 2, lcVector3(0, 0, 1), lcVector3(1, 0, 1), &Distance));
	QCOMPARE(Distance, 0.0f);
	QVERIFY(!lcCylinderRayMinIntersectDistance(1, 2, lcVector3(0, 0, 1), lcVector3(0, 0, 1), &Distance));

	const lcVector3 Tip(0, 0, 0), Direction(0, 0, 1);
	QVERIFY(!lcConeRayMinIntersectDistance(Tip, Direction, 1, 2, lcVector3(3, 0, 1), lcVector3(4, 0, 1), &Distance));
	QVERIFY(lcConeRayMinIntersectDistance(Tip, Direction, 1, 2, lcVector3(3, 0, 1), lcVector3(2, 0, 1), &Distance));
	QCOMPARE(Distance, 2.5f);
	QVERIFY(lcConeRayMinIntersectDistance(Tip, Direction, 1, 2, lcVector3(0, 0, 3), lcVector3(0, 0, 2.5f), &Distance));
	QCOMPARE(Distance, 1.0f);
	QVERIFY(lcConeRayMinIntersectDistance(Tip, Direction, 1, 2, lcVector3(0, 0, -1), lcVector3(0, 0, -0.5f), &Distance));
	QCOMPARE(Distance, 1.0f);
	QVERIFY(lcConeRayMinIntersectDistance(Tip, Direction, 1, 2, lcVector3(1, 0, 0), lcVector3(0, 0, 2), &Distance));
	QVERIFY(fabsf(Distance - sqrtf(5.0f) / 2) < 0.00001f);
	QVERIFY(lcConeRayMinIntersectDistance(Tip, Direction, 1, 2, lcVector3(0, 0, 1), lcVector3(1, 0, 1), &Distance));
	QCOMPARE(Distance, 0.0f);
	QVERIFY(!lcConeRayMinIntersectDistance(Tip, Direction, 1, 2, lcVector3(0, 0, 1), lcVector3(0, 0, 1), &Distance));
	for (float Step : { 0.5f, 1.0f, 10.0f })
	{
		const lcVector3 MovedTip(10, 20, 30);
		const lcVector3 Start = MovedTip + lcVector3(1, 3, 0);
		QVERIFY(lcConeRayMinIntersectDistance(MovedTip, lcVector3(1, 0, 0), 1, 2, Start, Start - lcVector3(0, Step, 0), &Distance));
		QCOMPARE(Distance, 2.5f);
		QVERIFY(lcCylinderRayMinIntersectDistance(1, 2, lcVector3(3, 0, 1), lcVector3(3 - Step, 0, 1), &Distance));
		QCOMPARE(Distance, 2.0f);
	}
}

void lcTests::MathSphereRayDistances()
{
	const lcVector3 Center(10, 20, 30);
	const lcVector3 Start = Center + lcVector3(-3, 0, 0);
	float Distance;
	for (float Step : { 0.5f, 1.0f, 10.0f, 1000.0f })
	{
		const lcVector3 End = Start + lcVector3(Step, 0, 0);
		QVERIFY(lcSphereRayMinIntersectDistance(Center, 1, Start, End, &Distance));
		QCOMPARE(Distance, 2.0f);
		const lcVector3 Intersection = Start + lcNormalize(End - Start) * Distance;
		QVERIFY(lcLength(Intersection - (Center + lcVector3(-1, 0, 0))) < 0.00001f);
	}

	QVERIFY(!lcSphereRayMinIntersectDistance(Center, 1, Start, Start - lcVector3(1, 0, 0), &Distance));
	QVERIFY(!lcSphereRayMinIntersectDistance(Center, 1, Start, Start, &Distance));
	const lcVector3 TangentStart = Center + lcVector3(-3, 1, 0);
	QVERIFY(lcSphereRayMinIntersectDistance(Center, 1, TangentStart, TangentStart + lcVector3(10, 0, 0), &Distance));
	QCOMPARE(Distance, 3.0f);
	QVERIFY(!lcSphereRayMinIntersectDistance(Center, 1, Center + lcVector3(-3, 2, 0), Center + lcVector3(7, 2, 0), &Distance));
	QVERIFY(lcSphereRayMinIntersectDistance(Center, 1, Center, Center + lcVector3(1, 0, 0), &Distance));
	QCOMPARE(Distance, 0.0f);
	QVERIFY(lcSphereRayMinIntersectDistance(Center, 1, Center + lcVector3(1, 0, 0), Center + lcVector3(2, 0, 0), &Distance));
	QCOMPARE(Distance, 0.0f);
}

void lcTests::MathZoomTranslation()
{
	const lcVector3 Eye(0, 0, 10000), Target(0, 0, 0), Up(0, 1, 0);
	const lcVector3 Points[2] = { lcVector3(-1, -1, -5000), lcVector3(1, 1, 5000) };
	const lcMatrix44 Projection = lcMatrix44Perspective(45, 1, 1, 20000);
	const auto [Position, Far] = lcZoomExtents(Eye, lcMatrix44LookAt(Eye, Target, Up), Projection, Points, 2);
	for (const lcVector3& Point : Points)
		QVERIFY(Far >= Position.z - Point.z);

	for (const lcVector3& Translation : { lcVector3(100, -200, -10000), lcVector3(-100, 200, 10000) })
	{
		const lcVector3 Translated[2] = { Points[0] + Translation, Points[1] + Translation };
		const auto [TranslatedPosition, TranslatedFar] = lcZoomExtents(Eye + Translation, lcMatrix44LookAt(Eye + Translation, Target + Translation, Up), Projection, Translated, 2);
		QVERIFY(lcLength(TranslatedPosition - Position - Translation) < 0.01f);
		QVERIFY(fabsf(TranslatedFar - Far) < 0.01f);
		QVERIFY(TranslatedFar > 0.0f);
	}
}
