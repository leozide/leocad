#include "lc_global.h"
#include "lc_synth.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_file.h"
#include "pieceinf.h"
#include <locale.h>

void lcSynthInit()
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	struct lcHoseInfo
	{
		const char* PartID;
		lcSynthType Type;
		float Length;
		int NumSections;
	};

	lcHoseInfo HoseInfo[] =
	{
		{ "72504",  LC_SYNTH_PIECE_RIBBED_HOSE,     31.25f,   4 }, // Technic Ribbed Hose  2L
		{ "72706",  LC_SYNTH_PIECE_RIBBED_HOSE,     50.00f,   7 }, // Technic Ribbed Hose  3L
		{ "71952",  LC_SYNTH_PIECE_RIBBED_HOSE,     75.00f,  11 }, // Technic Ribbed Hose  4L
		{ "71944",  LC_SYNTH_PIECE_RIBBED_HOSE,    112.50f,  17 }, // Technic Ribbed Hose  6L
		{ "71951",  LC_SYNTH_PIECE_RIBBED_HOSE,    143.75f,  22 }, // Technic Ribbed Hose  8L
		{ "71986",  LC_SYNTH_PIECE_RIBBED_HOSE,    212.50f,  33 }, // Technic Ribbed Hose 11L
		{ "43675",  LC_SYNTH_PIECE_RIBBED_HOSE,    375.00f,  58 }, // Technic Ribbed Hose 19L
		{ "32580",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  120.00f,  15 }, // Technic Axle Flexible  7
		{ "32199",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  200.00f,  35 }, // Technic Axle Flexible 11
		{ "55709",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  200.00f,  35 }, // Technic Axle Flexible 11
		{ "32200",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  220.00f,  40 }, // Technic Axle Flexible 12
		{ "32201",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  260.00f,  50 }, // Technic Axle Flexible 14
		{ "32202",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  300.00f,  60 }, // Technic Axle Flexible 16
		{ "32235",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  360.00f,  75 }, // Technic Axle Flexible 19
		{ "76384",  LC_SYNTH_PIECE_STRING_BRAIDED, 200.00f,  46 }, // String Braided 11L with End Studs
		{ "75924",  LC_SYNTH_PIECE_STRING_BRAIDED, 400.00f,  96 }, // String Braided 21L with End Studs
		{ "572C02", LC_SYNTH_PIECE_STRING_BRAIDED, 800.00f, 196 }, // String Braided 41L with End Studs
		{ "73129",  LC_SYNTH_PIECE_SHOCK_ABSORBER, 110.00f,   1 }, // Technic Shock Absorber 6.5L 
		{ "41838",  LC_SYNTH_PIECE_SHOCK_ABSORBER, 110.00f,   1 }, // Technic Shock Absorber 6.5L Soft
		{ "76138",  LC_SYNTH_PIECE_SHOCK_ABSORBER, 110.00f,   1 }, // Technic Shock Absorber 6.5L Stiff
		{ "76537",  LC_SYNTH_PIECE_SHOCK_ABSORBER, 110.00f,   1 }  // Technic Shock Absorber 6.5L Extra Stiff
	};

	for (unsigned int InfoIdx = 0; InfoIdx < sizeof(HoseInfo) / sizeof(HoseInfo[0]); InfoIdx++)
	{
		PieceInfo* Info = Library->FindPiece(HoseInfo[InfoIdx].PartID, NULL, false);

		if (Info)
			Info->SetSynthInfo(new lcSynthInfo(HoseInfo[InfoIdx].Type, HoseInfo[InfoIdx].Length, HoseInfo[InfoIdx].NumSections, Info));
	}

//	"758C01" // Hose Flexible  12L
//	"73590A" // Hose Flexible 8.5L without Tabs
//	"73590B" // Hose Flexible 8.5L with Tabs
}

lcSynthInfo::lcSynthInfo(lcSynthType Type, float Length, int NumSections, PieceInfo* Info)
	: mPieceInfo(Info), mType(Type), mLength(Length), mNumSections(NumSections)
{
	float EdgeSectionLength;
	float MidSectionLength;

	switch (mType)
	{
	case LC_SYNTH_PIECE_RIBBED_HOSE:
		EdgeSectionLength = 6.25f;
		MidSectionLength = 6.25f;
		mRigidEdges = false;
		mCurve = true;
		break;

	case LC_SYNTH_PIECE_FLEXIBLE_AXLE:
		EdgeSectionLength = 30.0f;
		MidSectionLength = 4.0f;
		mRigidEdges = true;
		mCurve = true;
		break;

	case LC_SYNTH_PIECE_STRING_BRAIDED:
		EdgeSectionLength = 8.0f;
		MidSectionLength = 4.0f;
		mRigidEdges = true;
		mCurve = true;
		break;

	case LC_SYNTH_PIECE_SHOCK_ABSORBER:
		EdgeSectionLength = 0.0f;
		MidSectionLength = 0.0f;
		mRigidEdges = false;
		mCurve = false;
		break;
	}

	if (mType != LC_SYNTH_PIECE_SHOCK_ABSORBER)
	{
		mStart.Transform = lcMatrix44(lcMatrix33(lcVector3(0.0f, 0.0f, 1.0f), lcVector3(1.0f, 0.0f, 0.0f), lcVector3(0.0f, 1.0f, 0.0f)), lcVector3(0.0f, 0.0f, 0.0f));
		mMiddle.Transform = lcMatrix44Identity();
		mEnd.Transform = lcMatrix44(lcMatrix33(lcVector3(0.0f, 0.0f, 1.0f), lcVector3(1.0f, 0.0f, 0.0f), lcVector3(0.0f, 1.0f, 0.0f)), lcVector3(0.0f, 0.0f, 0.0f));
	}
	else
	{
		mStart.Transform = lcMatrix44Identity();
		mMiddle.Transform = lcMatrix44Identity();
		mEnd.Transform = lcMatrix44Identity();
	}

	mStart.Length = EdgeSectionLength;
	mMiddle.Length = MidSectionLength;
	mEnd.Length = EdgeSectionLength;
}

void lcSynthInfo::GetDefaultControlPoints(lcArray<lcPieceControlPoint>& ControlPoints) const
{
	ControlPoints.SetSize(2);

	float Scale;

	switch (mType)
	{
	case LC_SYNTH_PIECE_RIBBED_HOSE:
		Scale = 80.0f;
		break;

	case LC_SYNTH_PIECE_FLEXIBLE_AXLE:
		Scale = 12.0f;
		break;

	case LC_SYNTH_PIECE_STRING_BRAIDED:
		Scale = 12.0f;
		break;

	case LC_SYNTH_PIECE_SHOCK_ABSORBER:
		Scale = 1.0f;
		break;
	}

	float HalfLength = mLength / 2.0f;
	Scale = lcMin(Scale, HalfLength);

	if (mType != LC_SYNTH_PIECE_SHOCK_ABSORBER)
	{
		ControlPoints[0].Transform = lcMatrix44Translation(lcVector3(-HalfLength, 0.0f, 0.0f));
		ControlPoints[1].Transform = lcMatrix44Translation(lcVector3( HalfLength, 0.0f, 0.0f));
	}
	else
	{
		ControlPoints[0].Transform = lcMatrix44Translation(lcVector3(0.0f, 0.0f, -mLength));
		ControlPoints[1].Transform = lcMatrix44Translation(lcVector3(0.0f, 0.0f, 0.0f));
	}

	ControlPoints[0].Scale = Scale;
	ControlPoints[1].Scale = Scale;
}

float lcSynthInfo::GetSectionTwist(const lcMatrix44& StartTransform, const lcMatrix44& EndTransform) const
{
	lcVector3 StartTangent(StartTransform[1].x, StartTransform[1].y, StartTransform[1].z);
	lcVector3 EndTangent(EndTransform[1].x, EndTransform[1].y, EndTransform[1].z);
	lcVector3 StartUp(StartTransform[2].x, StartTransform[2].y, StartTransform[2].z);
	lcVector3 EndUp(EndTransform[2].x, EndTransform[2].y, EndTransform[2].z);

	float TangentDot = lcDot(StartTangent, EndTangent);
	float UpDot = lcDot(StartUp, EndUp);

	if (TangentDot > 0.99f && UpDot > 0.99f)
		return 0.0f;

	if (fabs(TangentDot) > 0.99f)
	{
		return acosf(lcClamp(lcDot(EndUp, StartUp), -1.0f, 1.0f));
	}
	else if (TangentDot > -0.99f)
	{
		lcVector3 Axis = lcCross(StartTangent, EndTangent);
		float Angle = acosf(lcClamp(TangentDot, -1.0f, 1.0f));

		lcMatrix33 Rotation = lcMatrix33FromAxisAngle(Axis, Angle);
		lcVector3 AdjustedStartUp = lcMul(StartUp, Rotation);
		return acosf(lcClamp(lcDot(EndUp, AdjustedStartUp), -1.0f, 1.0f));
	}

	lcVector3 StartSide(StartTransform[0].x, StartTransform[0].y, StartTransform[0].z);
	lcVector3 EndSide(EndTransform[0].x, EndTransform[0].y, EndTransform[0].z);

	float SideDot = lcDot(StartSide, EndSide);

	if (fabs(SideDot) < 0.99f)
	{
		lcVector3 Axis = lcCross(StartSide, EndSide);
		float Angle = acosf(SideDot);

		lcMatrix33 Rotation = lcMatrix33FromAxisAngle(Axis, Angle);
		lcVector3 AdjustedStartUp = lcMul(StartUp, Rotation);
		return acosf(lcClamp(lcDot(EndUp, AdjustedStartUp), -1.0f, 1.0f));
	}

	return 0.0f;
}

void lcSynthInfo::CalculateCurveSections(const lcArray<lcPieceControlPoint>& ControlPoints, lcArray<lcMatrix44>& Sections, void (*SectionCallback)(const lcVector3& CurvePoint, int SegmentIndex, float t, void* Param), void* CallbackParam) const
{
	float SectionLength = 0.0f;

	for (int ControlPointIdx = 0; ControlPointIdx < ControlPoints.GetSize() - 1 && Sections.GetSize() < mNumSections + 2; ControlPointIdx++)
	{
		lcVector3 SegmentControlPoints[4];

		lcMatrix44 StartTransform = lcMatrix44LeoCADToLDraw(ControlPoints[ControlPointIdx].Transform);
		lcMatrix44 EndTransform = lcMatrix44LeoCADToLDraw(ControlPoints[ControlPointIdx + 1].Transform);
		StartTransform = lcMatrix44(lcMul(lcMul(lcMatrix33(mStart.Transform), lcMatrix33(StartTransform)), lcMatrix33Scale(lcVector3(1.0f, -1.0f, 1.0f))), StartTransform.GetTranslation());

		if (ControlPointIdx == 0)
		{
			if (mRigidEdges)
			{
				StartTransform.SetTranslation(lcMul30(lcVector3(0.0f, mStart.Length, 0.0f), StartTransform) + StartTransform.GetTranslation());
				SectionLength = 0.0f;
			}
			else
				SectionLength = mStart.Length;

			Sections.Add(StartTransform);
		}

		EndTransform = lcMatrix44(lcMul(lcMul(lcMatrix33(mEnd.Transform), lcMatrix33(EndTransform)), lcMatrix33Scale(lcVector3(1.0f, -1.0f, 1.0f))), EndTransform.GetTranslation());

		SegmentControlPoints[0] = StartTransform.GetTranslation();
		SegmentControlPoints[1] = lcMul31(lcVector3(0.0f, ControlPoints[ControlPointIdx].Scale, 0.0f), StartTransform);
		SegmentControlPoints[2] = lcMul31(lcVector3(0.0f, -ControlPoints[ControlPointIdx + 1].Scale, 0.0f), EndTransform);
		SegmentControlPoints[3] = EndTransform.GetTranslation();

		const int NumCurvePoints = 8192;
		lcArray<lcVector3> CurvePoints;
		CurvePoints.AllocGrow(NumCurvePoints);

		for (int PointIdx = 0; PointIdx < NumCurvePoints; PointIdx++)
		{
			float t = (float)PointIdx / (float)(NumCurvePoints - 1);
			float it = 1.0f - t;

			lcVector3 Position = it * it * it * SegmentControlPoints[0] + it * it * 3.0f * t * SegmentControlPoints[1] + it * 3.0 * t * t * SegmentControlPoints[2] + t * t * t * SegmentControlPoints[3];
			CurvePoints.Add(Position);
		}

		float CurrentSegmentLength = 0.0f;
		float TotalSegmentLength = 0.0f;

		for (int PointIdx = 0; PointIdx < CurvePoints.GetSize() - 1; PointIdx++)
			TotalSegmentLength += lcLength(CurvePoints[PointIdx] - CurvePoints[PointIdx + 1]);

		lcVector3 StartUp(lcMul30(lcVector3(1.0f, 0.0f, 0.0f), StartTransform));
		float Twist = GetSectionTwist(StartTransform, EndTransform);
		int CurrentPointIndex = 0;

		while (CurrentPointIndex < CurvePoints.GetSize() - 1)
		{
			float Length = lcLength(CurvePoints[CurrentPointIndex + 1] - CurvePoints[CurrentPointIndex]);
			CurrentSegmentLength += Length;
			SectionLength -= Length;
			CurrentPointIndex++;

			if (SectionLength > 0.0f)
				continue;

			float t = (float)CurrentPointIndex / (float)(NumCurvePoints - 1);
			float it = 1.0f - t;

			lcVector3 Tangent = lcNormalize(-3.0f * it * it * SegmentControlPoints[0] + (3.0f * it * it - 6.0f * t * it) * SegmentControlPoints[1] + (-3.0f * t * t + 6.0f * t * it) * SegmentControlPoints[2] + 3.0f * t * t * SegmentControlPoints[3]);
			lcVector3 Up;

			if (Twist)
			{
				Up = lcMul(StartUp, lcMatrix33FromAxisAngle(Tangent, Twist * (CurrentSegmentLength / TotalSegmentLength)));
				CurrentSegmentLength = 0.0f;
			}
			else
				Up = StartUp;

			lcVector3 Side = lcNormalize(lcCross(Tangent, Up));
			Up = lcNormalize(lcCross(Side, Tangent));
			StartUp = Up;

			Sections.Add(lcMatrix44(lcMatrix33(Up, Tangent, Side), CurvePoints[CurrentPointIndex]));

			if (SectionCallback)
				SectionCallback(CurvePoints[CurrentPointIndex], ControlPointIdx, t, CallbackParam);

			if (Sections.GetSize() == mNumSections + 2)
				break;

			SectionLength += mMiddle.Length;
			if (Sections.GetSize() == mNumSections + 1 && !mRigidEdges)
				SectionLength += mEnd.Length;
		}
	}

	while (Sections.GetSize() < mNumSections + 2)
	{
		lcMatrix44 EndTransform = lcMatrix44LeoCADToLDraw(ControlPoints[ControlPoints.GetSize() - 1].Transform);
		EndTransform = lcMatrix44(lcMul(lcMul(lcMatrix33(mEnd.Transform), lcMatrix33(EndTransform)), lcMatrix33Scale(lcVector3(1.0f, -1.0f, 1.0f))), EndTransform.GetTranslation());
		lcVector3 Position = lcMul31(lcVector3(0.0f, SectionLength, 0.0f), EndTransform);
		EndTransform.SetTranslation(Position);
		Sections.Add(EndTransform);

		if (SectionCallback)
			SectionCallback(Position, ControlPoints.GetSize() - 1, 1.0f, CallbackParam);

		SectionLength += mMiddle.Length;
		if (Sections.GetSize() == mNumSections + 1 && !mRigidEdges)
			SectionLength += mEnd.Length;
	}
}

void lcSynthInfo::CalculateLineSections(const lcArray<lcPieceControlPoint>& ControlPoints, lcArray<lcMatrix44>& Sections, void(*SectionCallback)(const lcVector3& CurvePoint, int SegmentIndex, float t, void* Param), void* CallbackParam) const
{
	for (int ControlPointIdx = 0; ControlPointIdx < ControlPoints.GetSize(); ControlPointIdx++)
	{
		lcMatrix44 Transform = lcMatrix44LeoCADToLDraw(ControlPoints[ControlPointIdx].Transform);
		Sections.Add(Transform);

		if (SectionCallback)
			SectionCallback(Transform.GetTranslation(), ControlPointIdx, 1.0f, CallbackParam);
	}
}

void lcSynthInfo::AddRibbedHoseParts(lcMemFile& File, const lcArray<lcMatrix44>& Sections) const
{
	char Line[256];

	{
		const int SectionIdx = 0;
		lcMatrix33 Transform(lcMul(lcMatrix33Scale(lcVector3(1.0f, -1.0f, 1.0f)), lcMatrix33(Sections[SectionIdx])));
		lcVector3 Offset = Sections[SectionIdx].GetTranslation();

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f 79.DAT\n", Offset[0], Offset[1], Offset[2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2]);

		File.WriteBuffer(Line, strlen(Line));
	}

	for (int SectionIdx = 1; SectionIdx < Sections.GetSize() - 1; SectionIdx++)
	{
		const lcMatrix44& Transform = Sections[SectionIdx];

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f 80.DAT\n", Transform[3][0], Transform[3][1], Transform[3][2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2]);

		File.WriteBuffer(Line, strlen(Line));
	}

	{
		const int SectionIdx = Sections.GetSize() - 1;
		lcMatrix33 Transform(Sections[SectionIdx]);
		lcVector3 Offset = lcMul31(lcVector3(0.0f, -6.25f, 0.0f), Sections[SectionIdx]);

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f 79.DAT\n", Offset[0], Offset[1], Offset[2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2]);

		File.WriteBuffer(Line, strlen(Line));
	}
}

void lcSynthInfo::AddFlexibleAxleParts(lcMemFile& File, lcLibraryMeshData& MeshData, const lcArray<lcMatrix44>& Sections) const
{
	char Line[256];
	const int NumEdgeParts = 6;

	lcMatrix33 EdgeTransforms[6] = 
	{
		lcMatrix33(lcVector3(-1.0f, 0.0f, 0.0f), lcVector3(0.0f, -5.0f, 0.0f), lcVector3(0.0f, 0.0f,  1.0f)),
		lcMatrix33(lcVector3( 0.0f, 1.0f, 0.0f), lcVector3(1.0f,  0.0f, 0.0f), lcVector3(0.0f, 0.0f, -1.0f)),
		lcMatrix33(lcVector3( 0.0f, 1.0f, 0.0f), lcVector3(1.0f,  0.0f, 0.0f), lcVector3(0.0f, 0.0f, -1.0f)),
		lcMatrix33(lcVector3( 0.0f, 1.0f, 0.0f), lcVector3(1.0f,  0.0f, 0.0f), lcVector3(0.0f, 0.0f, -1.0f)),
		lcMatrix33(lcVector3( 0.0f, 1.0f, 0.0f), lcVector3(1.0f,  0.0f, 0.0f), lcVector3(0.0f, 0.0f, -1.0f)),
		lcMatrix33(lcVector3( 0.0f, 1.0f, 0.0f), lcVector3(1.0f,  0.0f, 0.0f), lcVector3(0.0f, 0.0f, -1.0f)),
	};

	const char* EdgeParts[6] =
	{
		"STUD3A.DAT", "S/FAXLE1.DAT", "S/FAXLE2.DAT", "S/FAXLE3.DAT", "S/FAXLE4.DAT", "S/FAXLE5.DAT"
	};

	for (int PartIdx = 0; PartIdx < NumEdgeParts; PartIdx++)
	{
		const int SectionIdx = 0;
		lcMatrix33 Transform(lcMul(lcMul(EdgeTransforms[PartIdx], lcMatrix33Scale(lcVector3(1.0f, -1.0f, 1.0f))), lcMatrix33(Sections[SectionIdx])));
		lcVector3 Offset = lcMul31(lcVector3(0.0f, -4.0f * (5 - PartIdx), 0.0f), Sections[SectionIdx]);

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f %s\n", Offset[0], Offset[1], Offset[2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2], EdgeParts[PartIdx]);

		File.WriteBuffer(Line, strlen(Line));
	}

	for (int PartIdx = 0; PartIdx < NumEdgeParts; PartIdx++)
	{
		const int SectionIdx = Sections.GetSize() - 1;
		lcMatrix33 Transform(lcMul(EdgeTransforms[PartIdx], lcMatrix33(Sections[SectionIdx])));
		lcVector3 Offset = lcMul31(lcVector3(0.0f, 4.0f * (5 - PartIdx), 0.0f), Sections[SectionIdx]);

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f %s\n", Offset[0], Offset[1], Offset[2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2], EdgeParts[PartIdx]);

		File.WriteBuffer(Line, strlen(Line));
	}

	lcVector3 SectionVertices[16] =
	{
		lcVector3(-6.000f, 0.0f,  0.000f), lcVector3(-5.602f, 0.0f,  2.000f), lcVector3(-2.000f, 0.0f,  2.000f), lcVector3(-2.000f, 0.0f,  5.602f),
		lcVector3( 0.000f, 0.0f,  6.000f), lcVector3( 2.000f, 0.0f,  5.602f), lcVector3( 2.000f, 0.0f,  2.000f), lcVector3( 5.602f, 0.0f,  2.000f),
		lcVector3( 6.000f, 0.0f,  0.000f), lcVector3( 5.602f, 0.0f, -2.000f), lcVector3( 2.000f, 0.0f, -2.000f), lcVector3( 2.000f, 0.0f, -5.602f),
		lcVector3( 0.000f, 0.0f, -6.000f), lcVector3(-2.000f, 0.0f, -5.602f), lcVector3(-2.000f, 0.0f, -2.000f), lcVector3(-5.602f, 0.0f, -2.000f)
	};

	int BaseVertex;
	lcVertex* VertexBuffer;
	lcuint32* IndexBuffer;
	MeshData.AddVertices(LC_MESHDATA_SHARED, 16 * (Sections.GetSize() - 1), &BaseVertex, &VertexBuffer);
	MeshData.AddIndices(LC_MESHDATA_SHARED, LC_MESH_LINES, 24, 2 * 12 * (Sections.GetSize() - 2), &IndexBuffer);

	for (int SectionIdx = 1; SectionIdx < Sections.GetSize(); SectionIdx++)
	{
		for (int VertexIdx = 0; VertexIdx < 16; VertexIdx++)
		{
			VertexBuffer->Position = lcMul31(SectionVertices[VertexIdx], Sections[SectionIdx]);
			VertexBuffer++;
		}
	}

	int BaseLinesVertex = BaseVertex;

	for (int SectionIdx = 1; SectionIdx < Sections.GetSize() - 1; SectionIdx++)
	{
		for (int VertexIdx = 0; VertexIdx < 16; VertexIdx++)
		{
			if (VertexIdx % 4)
			{
				*IndexBuffer++ = BaseLinesVertex;
				*IndexBuffer++ = BaseLinesVertex + 16;
			}
			BaseLinesVertex++;
		}
	}

	MeshData.AddIndices(LC_MESHDATA_SHARED, LC_MESH_TRIANGLES, 16, 6 * 16 * (Sections.GetSize() - 2), &IndexBuffer);

	for (int SectionIdx = 1; SectionIdx < Sections.GetSize() - 1; SectionIdx++)
	{
		for (int VertexIdx = 0; VertexIdx < 16; VertexIdx++)
		{
			int Vertex1 = BaseVertex + VertexIdx;
			int Vertex2 = BaseVertex + (VertexIdx + 1) % 16;

			*IndexBuffer++ = Vertex1;
			*IndexBuffer++ = Vertex2;
			*IndexBuffer++ = Vertex1 + 16;
			*IndexBuffer++ = Vertex2;
			*IndexBuffer++ = Vertex2 + 16;
			*IndexBuffer++ = Vertex1 + 16;
		}
		BaseVertex += 16;
	}
}

void lcSynthInfo::AddStringBraidedParts(lcMemFile& File, lcLibraryMeshData& MeshData, lcArray<lcMatrix44>& Sections) const
{
	for (int SectionIdx = 0; SectionIdx < Sections.GetSize(); SectionIdx++)
	{
		lcMatrix33 Transform = lcMul(lcMatrix33(lcVector3(0.0f, 1.0f, 0.0f), lcVector3(1.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 1.0f)), lcMatrix33(Sections[SectionIdx]));
		lcVector3 Offset = Sections[SectionIdx].GetTranslation();
		Sections[SectionIdx] = lcMatrix44(Transform, Offset);
	}

	char Line[256];

	{
		const int SectionIdx = 0;
		lcMatrix33 Transform(Sections[SectionIdx]);
		lcVector3 Offset = lcMul31(lcVector3(-8.0f, 0.0f, 0.0f), Sections[SectionIdx]);

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f 572A.DAT\n", Offset[0], Offset[1], Offset[2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2]);

		File.WriteBuffer(Line, strlen(Line));
	}

	const int NumSegments = 16;
	const int NumBraids = 4;
	const float PositionTable[16] =
	{
		-1.5f, -1.386f, -1.061f, -0.574f, 0.0f, 0.574f, 1.061f, 1.386f, 1.5f, 1.386f, 1.061f, 0.574f, 0.0f, -0.574f, -1.061f, -1.386f
	};

	int BaseVertex;
	lcVertex* VertexBuffer;
	lcuint32* IndexBuffer;
	MeshData.AddVertices(LC_MESHDATA_SHARED, NumBraids * ((Sections.GetSize() - 2) * NumSegments + 1), &BaseVertex, &VertexBuffer);
	MeshData.AddIndices(LC_MESHDATA_SHARED, LC_MESH_LINES, 24, NumBraids * (Sections.GetSize() - 2) * NumSegments * 2, &IndexBuffer);

	for (int BraidIdx = 0; BraidIdx < NumBraids; BraidIdx++)
	{
		int BaseX = (BraidIdx == 0 || BraidIdx == 2) ? 0 : 8;
		int BaseY = (BraidIdx == 0 || BraidIdx == 3) ? 12 : 4;

		for (int SectionIdx = 1; SectionIdx < Sections.GetSize() - 1; SectionIdx++)
		{
			lcMatrix33 Transform1 = lcMatrix33(Sections[SectionIdx]);
			lcMatrix33 Transform2 = lcMatrix33(Sections[SectionIdx + 1]);
			lcVector3 Offset1 = Sections[SectionIdx].GetTranslation();
			lcVector3 Offset2 = Sections[SectionIdx + 1].GetTranslation();

			for (int SegmentIdx = 0; SegmentIdx < ((SectionIdx < Sections.GetSize() - 2) ? NumSegments : NumSegments + 1); SegmentIdx++)
			{
				float t = (float)SegmentIdx / (float)NumSegments;

				lcVector3 Vertex1 = lcVector3(t * 4.0f, PositionTable[(BaseX + SegmentIdx) % NumSegments], PositionTable[(BaseY + SegmentIdx) % NumSegments]) + lcVector3(0.0f, 1.5f, 0.0f);
				lcVector3 Vertex2 = lcVector3((1.0f - t) * -4.0f, PositionTable[(BaseX + SegmentIdx) % NumSegments], PositionTable[(BaseY + SegmentIdx) % NumSegments]) + lcVector3(0.0f, 1.5f, 0.0f);

				lcVector3 Vertex = (lcMul(Vertex1, Transform1) + Offset1) * (1.0f - t) + (lcMul(Vertex2, Transform2) + Offset2) * t;

				VertexBuffer->Position = Vertex;
				VertexBuffer++;

				if (SegmentIdx != NumSegments)
				{
					*IndexBuffer++ = BaseVertex;
					*IndexBuffer++ = BaseVertex + 1;
					BaseVertex++;
				}
			}
		}

		BaseVertex++;
	}

	int NumSlices = 16;
	MeshData.AddVertices(LC_MESHDATA_SHARED, NumSlices * ((Sections.GetSize() - 2) * NumSegments + 1), &BaseVertex, &VertexBuffer);
	MeshData.AddIndices(LC_MESHDATA_SHARED, LC_MESH_TRIANGLES, 16, NumSlices * (Sections.GetSize() - 2) * NumSegments * 6, &IndexBuffer);

	for (int SectionIdx = 1; SectionIdx < Sections.GetSize() - 1; SectionIdx++)
	{
		lcMatrix33 Transform1 = lcMatrix33(Sections[SectionIdx]);
		lcMatrix33 Transform2 = lcMatrix33(Sections[SectionIdx + 1]);
		lcVector3 Offset1 = Sections[SectionIdx].GetTranslation();
		lcVector3 Offset2 = Sections[SectionIdx + 1].GetTranslation();

		for (int SegmentIdx = 0; SegmentIdx < ((SectionIdx < Sections.GetSize() - 2) ? NumSegments : NumSegments + 1); SegmentIdx++)
		{
			float t1 = (float)SegmentIdx / (float)NumSegments;
			int BaseX = 8;
			int BaseY = 4;

			for (int SliceIdx = 0; SliceIdx < NumSlices; SliceIdx++)
			{
				lcVector3 Vertex11 = lcVector3(t1 * 4.0f, PositionTable[(BaseX + SliceIdx) % NumSlices], PositionTable[(BaseY + SliceIdx) % NumSlices]) + lcVector3(0.0f, 1.5f, 0.0f);
				lcVector3 Vertex12 = lcVector3((1.0f - t1) * -4.0f, PositionTable[(BaseX + SliceIdx) % NumSlices], PositionTable[(BaseY + SliceIdx) % NumSlices]) + lcVector3(0.0f, 1.5f, 0.0f);

				lcVector3 Vertex1 = (lcMul(Vertex11, Transform1) + Offset1) * (1.0f - t1) + (lcMul(Vertex12, Transform2) + Offset2) * t1;

				VertexBuffer->Position = Vertex1;
				VertexBuffer++;

				if (SegmentIdx != NumSegments)
				{
					*IndexBuffer++ = BaseVertex + SliceIdx;
					*IndexBuffer++ = BaseVertex + (SliceIdx + 1) % NumSlices;
					*IndexBuffer++ = BaseVertex + (SliceIdx + 1) % NumSlices + NumSlices;

					*IndexBuffer++ = BaseVertex + SliceIdx + NumSlices;
					*IndexBuffer++ = BaseVertex + SliceIdx;
					*IndexBuffer++ = BaseVertex + (SliceIdx + 1) % NumSlices + NumSlices;
				}
			}

			BaseVertex += NumSlices;
		}
	}

	{
		const int SectionIdx = Sections.GetSize() - 1;
		lcMatrix33 Transform(Sections[SectionIdx]);
		lcVector3 Offset = lcMul31(lcVector3(8.0f, 0.0f, 0.0f), Sections[SectionIdx]);

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f 572A.DAT\n", Offset[0], Offset[1], Offset[2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2]);

		File.WriteBuffer(Line, strlen(Line));
	}
}

void lcSynthInfo::AddShockAbsorberParts(lcMemFile& File, lcArray<lcMatrix44>& Sections) const
{
	char Line[256];
	lcVector3 Offset;

	Offset = Sections[0].GetTranslation();
	sprintf(Line, "1 0 %f %f %f 1 0 0 0 1 0 0 0 1 4254.DAT\n", Offset[0], Offset[1], Offset[2]);
	File.WriteBuffer(Line, strlen(Line));

	Offset = Sections[1].GetTranslation();
	sprintf(Line, "1 16 %f %f %f 1 0 0 0 1 0 0 0 1 4255.DAT\n", Offset[0], Offset[1], Offset[2]);
	File.WriteBuffer(Line, strlen(Line));

	float Distance = Sections[0].GetTranslation().y - Sections[1].GetTranslation().y;
	float Scale = (Distance - 66.0f) / 44.0f;
	const char* SpringPart;

	if (!strcmp(mPieceInfo->m_strName, "73129"))
		SpringPart = "70038";
	else if (!strcmp(mPieceInfo->m_strName, "41838"))
		SpringPart = "41837";
	else if (!strcmp(mPieceInfo->m_strName, "76138"))
		SpringPart = "71953";
	else if (!strcmp(mPieceInfo->m_strName, "76537"))
		SpringPart = "22977";
	else
		return;

	Offset = Sections[0].GetTranslation();
	sprintf(Line, "1 494 %f %f %f 1 0 0 0 %f 0 0 0 1 %s.DAT\n", Offset[0], Offset[1] - 10 - 44.0f * Scale, Offset[2], Scale, SpringPart);
	File.WriteBuffer(Line, strlen(Line));
}

lcMesh* lcSynthInfo::CreateMesh(const lcArray<lcPieceControlPoint>& ControlPoints) const
{
	lcArray<lcMatrix44> Sections;

	if (mCurve)
		CalculateCurveSections(ControlPoints, Sections, NULL, NULL);
	else
		CalculateLineSections(ControlPoints, Sections, NULL, NULL);

	lcLibraryMeshData MeshData;
	lcMemFile File; // todo: rewrite this to pass the parts directly
	const char* OldLocale = setlocale(LC_NUMERIC, "C");

	switch (mType)
	{
	case LC_SYNTH_PIECE_RIBBED_HOSE:
		AddRibbedHoseParts(File, Sections);
		break;

	case LC_SYNTH_PIECE_FLEXIBLE_AXLE:
		AddFlexibleAxleParts(File, MeshData, Sections);
		break;

	case LC_SYNTH_PIECE_STRING_BRAIDED:
		AddStringBraidedParts(File, MeshData, Sections);
		break;

	case LC_SYNTH_PIECE_SHOCK_ABSORBER:
		AddShockAbsorberParts(File, Sections);
		break;
	}

	File.WriteU8(0);

	lcArray<lcLibraryTextureMap> TextureStack;
	File.Seek(0, SEEK_SET);

	bool Ret = lcGetPiecesLibrary()->ReadMeshData(File, lcMatrix44Identity(), 16, TextureStack, MeshData, LC_MESHDATA_SHARED, false);
	setlocale(LC_NUMERIC, OldLocale);

	if (Ret)
		return lcGetPiecesLibrary()->CreateMesh(NULL, MeshData);

	return NULL;
}

struct lcSynthInsertParam
{
	lcVector3 Start;
	lcVector3 End;
	int BestSegment;
	float BestTime;
	float BestDistance;
	lcVector3 BestPosition;
};

static void lcSynthInsertCallback(const lcVector3& CurvePoint, int SegmentIndex, float t, void* Param)
{
	lcSynthInsertParam* SynthInsertParam = (lcSynthInsertParam*)Param;

	float Distance = lcRayPointDistance(CurvePoint, SynthInsertParam->Start, SynthInsertParam->End);
	if (Distance < SynthInsertParam->BestDistance)
	{
		SynthInsertParam->BestSegment = SegmentIndex;
		SynthInsertParam->BestTime = t;
		SynthInsertParam->BestDistance = Distance;
		SynthInsertParam->BestPosition = lcVector3LDrawToLeoCAD(CurvePoint);
	}
}

int lcSynthInfo::InsertControlPoint(lcArray<lcPieceControlPoint>& ControlPoints, const lcVector3& Start, const lcVector3& End) const
{
	lcArray<lcMatrix44> Sections;
	lcSynthInsertParam SynthInsertParam;

	SynthInsertParam.Start = Start;
	SynthInsertParam.End = End;
	SynthInsertParam.BestSegment = -1;
	SynthInsertParam.BestDistance = FLT_MAX;

	CalculateCurveSections(ControlPoints, Sections, lcSynthInsertCallback, &SynthInsertParam);

	if (SynthInsertParam.BestSegment != -1)
	{
		lcPieceControlPoint ControlPoint = ControlPoints[SynthInsertParam.BestSegment];
		ControlPoint.Transform.SetTranslation(SynthInsertParam.BestPosition);

		if (SynthInsertParam.BestSegment != ControlPoints.GetSize() - 1)
		{
			lcPieceControlPoint NextControlPoint = ControlPoints[SynthInsertParam.BestSegment + 1];
			float t = SynthInsertParam.BestTime;

			ControlPoint.Scale = ControlPoint.Scale * (1.0f - t) + NextControlPoint.Scale * t;
		}

		ControlPoints.InsertAt(SynthInsertParam.BestSegment + 1, ControlPoint);
	}

	return SynthInsertParam.BestSegment + 1;
}
