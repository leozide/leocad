#include "lc_global.h"
#include "lc_synth.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"

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
		{ "32580",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,   60.00f,  15 }, // Technic Axle Flexible  7
		{ "32199",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  140.00f,  35 }, // Technic Axle Flexible 11
		{ "55709",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  140.00f,  35 }, // Technic Axle Flexible 11
		{ "32200",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  160.00f,  40 }, // Technic Axle Flexible 12
		{ "32201",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  200.00f,  50 }, // Technic Axle Flexible 14
		{ "32202",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  240.00f,  60 }, // Technic Axle Flexible 16
		{ "32235",  LC_SYNTH_PIECE_FLEXIBLE_AXLE,  300.00f,  75 }, // Technic Axle Flexible 19
		{ "76384",  LC_SYNTH_PIECE_STRING_BRAIDED, 200.00f,  46 }, // String Braided 11L with End Studs
		{ "75924",  LC_SYNTH_PIECE_STRING_BRAIDED, 400.00f,  96 }, // String Braided 21L with End Studs
		{ "572C02", LC_SYNTH_PIECE_STRING_BRAIDED, 800.00f, 196 }  // String Braided 41L with End Studs
	};

	for (unsigned int InfoIdx = 0; InfoIdx < sizeof(HoseInfo) / sizeof(HoseInfo[0]); InfoIdx++)
	{
		PieceInfo* Info = Library->FindPiece(HoseInfo[InfoIdx].PartID, NULL, false);

		if (Info)
			Info->SetSynthInfo(new lcSynthInfo(HoseInfo[InfoIdx].Type, HoseInfo[InfoIdx].Length, HoseInfo[InfoIdx].NumSections));
	}

//	"758C01" // Hose Flexible  12L
//	"73590A" // Hose Flexible 8.5L without Tabs
//	"73590B" // Hose Flexible 8.5L with Tabs
}

lcSynthInfo::lcSynthInfo(lcSynthType Type, float Length, int NumSections)
	: mType(Type), mLength(Length), mNumSections(NumSections)
{
	float EdgeSectionLength;
	float MidSectionLength;

	mRigidEdges = false;

	switch (mType)
	{
	case LC_SYNTH_PIECE_RIBBED_HOSE:
		EdgeSectionLength = 6.25f;
		MidSectionLength = 6.25f;
		break;

	case LC_SYNTH_PIECE_FLEXIBLE_AXLE:
		EdgeSectionLength = 0.0f;
		MidSectionLength = 4.0f;
		break;

	case LC_SYNTH_PIECE_STRING_BRAIDED:
		EdgeSectionLength = 8.0f;
		MidSectionLength = 4.0f;
		mRigidEdges = true;
		break;
	}

	mStart.Transform = lcMatrix44Identity();
	mStart.Transform[1][1] = -1.0f;
	mStart.Length = EdgeSectionLength;
	mMiddle.Transform = lcMatrix44Identity();
	mMiddle.Length = MidSectionLength;
	mEnd.Transform = lcMatrix44Identity();
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
	}

	lcMatrix33 RotationY = lcMatrix33RotationY(LC_PI / 2.0f);
	float HalfLength = mLength / 2.0f;
	Scale = lcMin(Scale, HalfLength);

	ControlPoints[0].Transform = lcMatrix44(RotationY, lcVector3(-HalfLength, 0.0f, 0.0f));
	ControlPoints[0].Scale = Scale;
	ControlPoints[1].Transform = lcMatrix44(RotationY, lcVector3( HalfLength, 0.0f, 0.0f));
	ControlPoints[1].Scale = Scale;
}

#include "lc_file.h"

void lcSynthInfo::CalculateSections(const lcArray<lcPieceControlPoint>& ControlPoints, lcArray<lcMatrix44>& Sections, void (*SectionCallback)(const lcVector3& CurvePoint, int SegmentIndex, float t, void* Param), void* CallbackParam) const
{
	float SectionLength = 0.0f;

	for (int ControlPointIdx = 0; ControlPointIdx < ControlPoints.GetSize() - 1 && Sections.GetSize() < mNumSections + 2; ControlPointIdx++)
	{
		lcVector3 SegmentControlPoints[4];

		lcMatrix44 StartTransform = lcMatrix44LeoCADToLDraw(ControlPoints[ControlPointIdx].Transform);
		lcMatrix44 EndTransform = lcMatrix44LeoCADToLDraw(ControlPoints[ControlPointIdx + 1].Transform);
		StartTransform = lcMatrix44(lcMul(lcMatrix33(StartTransform), lcMatrix33(mStart.Transform)), StartTransform.GetTranslation());

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
		else
			StartTransform = lcMatrix44(lcMul(lcMatrix33(StartTransform), lcMatrix33Scale(lcVector3(1.0f, -1.0f, 1.0f))), StartTransform.GetTranslation());

		EndTransform = lcMatrix44(lcMul(lcMatrix33(EndTransform), lcMatrix33Scale(lcVector3(1.0f, -1.0f, 1.0f))), EndTransform.GetTranslation());

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

		int CurrentPointIndex = 0;

		lcVector3 StartUp(lcMul30(lcVector3(1.0f, 0.0f, 0.0f), StartTransform));
		lcVector3 EndUp(lcMul30(lcVector3(1.0f, 0.0f, 0.0f), EndTransform));
		lcVector4 UpRotation;
		float UpDot = lcDot(StartUp, EndUp);

		if (UpDot < 0.99f)
			UpRotation = lcVector4(lcCross(StartUp, EndUp), acosf(UpDot));
		else
			UpRotation = lcVector4(0.0f, 0.0f, 0.0f, 0.0f);

		float CurrentSegmentLength = 0.0f;
		float TotalSegmentLength = 0.0f;

		for (int PointIdx = 0; PointIdx < CurvePoints.GetSize() - 1; PointIdx++)
			TotalSegmentLength += lcLength(CurvePoints[PointIdx] - CurvePoints[PointIdx + 1]);

		while (CurrentPointIndex < CurvePoints.GetSize() - 1)
		{
			float Length = lcLength(CurvePoints[CurrentPointIndex] - CurvePoints[CurrentPointIndex + 1]);
			CurrentSegmentLength += Length;
			SectionLength -= Length;
			CurrentPointIndex++;

			if (SectionLength > 0.0f)
				continue;

			float t = (float)CurrentPointIndex / (float)(NumCurvePoints - 1);
			float it = 1.0f - t;

			lcVector3 Tangent = -3.0f * it * it * SegmentControlPoints[0] + (3.0f * it * it - 6.0f * t * it) * SegmentControlPoints[1] + (-3.0f * t * t + 6.0f * t * it) * SegmentControlPoints[2] + 3.0f * t * t * SegmentControlPoints[3];
			lcVector3 Up = lcMul(StartUp, lcMatrix33FromAxisAngle(lcVector3(UpRotation[0], UpRotation[1], UpRotation[2]), UpRotation[3] * (CurrentSegmentLength / TotalSegmentLength)));
			lcVector3 Side = lcCross(Tangent, Up);
			Up = lcCross(Side, Tangent);

			Sections.Add(lcMatrix44(lcMatrix33(lcNormalize(Up), lcNormalize(Tangent), lcNormalize(Side)), CurvePoints[CurrentPointIndex]));

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
		EndTransform = lcMatrix44(lcMul(lcMatrix33(EndTransform), lcMatrix33Scale(lcVector3(1.0f, -1.0f, 1.0f))), EndTransform.GetTranslation());
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

void lcSynthInfo::AddFlexibleAxleParts(lcMemFile& File, const lcArray<lcMatrix44>& Sections) const
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
		"STUD3A.DAT",
		"S/FAXLE1.DAT",
		"S/FAXLE2.DAT",
		"S/FAXLE3.DAT",
		"S/FAXLE4.DAT",
		"S/FAXLE5.DAT",
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

	lcVector3 MidLineVertices[12] =
	{
		lcVector3(-5.602f, 0.0f,  2.000f), lcVector3(-2.000f, 0.0f,  2.000f), lcVector3(-2.000f, 0.0f,  5.602f), lcVector3( 5.602f, 0.0f,  2.000f), 
		lcVector3( 2.000f, 0.0f,  2.000f), lcVector3( 2.000f, 0.0f,  5.602f), lcVector3(-5.602f, 0.0f, -2.000f), lcVector3(-2.000f, 0.0f, -2.000f), 
		lcVector3(-2.000f, 0.0f, -5.602f), lcVector3( 5.602f, 0.0f, -2.000f), lcVector3( 2.000f, 0.0f, -2.000f), lcVector3( 2.000f, 0.0f, -5.602f)
	};

	for (int SectionIdx = 1; SectionIdx < Sections.GetSize() - 1; SectionIdx++)
	{
		for (int VertexIdx = 0; VertexIdx < 12; VertexIdx++)
		{
			lcVector3 Vertex1 = lcMul31(MidLineVertices[VertexIdx], Sections[SectionIdx]);
			lcVector3 Vertex2 = lcMul31(MidLineVertices[VertexIdx], Sections[SectionIdx + 1]);

			sprintf(Line, "2 24 %f %f %f %f %f %f\n", Vertex1[0], Vertex1[1], Vertex1[2], Vertex2[0], Vertex2[1], Vertex2[2]);
			File.WriteBuffer(Line, strlen(Line));
		}
	}

	lcVector3 MidQuadVertices[32] =
	{
		lcVector3(-6.000f, 0.0f,  0.000f), lcVector3(-5.602f, 0.0f,  2.000f), lcVector3(-5.602f, 0.0f,  2.000f), lcVector3(-2.000f, 0.0f,  2.000f),
		lcVector3(-2.000f, 0.0f,  2.000f), lcVector3(-2.000f, 0.0f,  5.602f), lcVector3(-2.000f, 0.0f,  5.602f), lcVector3( 0.000f, 0.0f,  6.000f),
		lcVector3( 5.602f, 0.0f,  2.000f), lcVector3( 6.000f, 0.0f,  0.000f), lcVector3( 2.000f, 0.0f,  2.000f), lcVector3( 5.602f, 0.0f,  2.000f),
		lcVector3( 2.000f, 0.0f,  5.602f), lcVector3( 2.000f, 0.0f,  2.000f), lcVector3( 0.000f, 0.0f,  6.000f), lcVector3( 2.000f, 0.0f,  5.602f),
		lcVector3(-5.602f, 0.0f, -2.000f), lcVector3(-6.000f, 0.0f,  0.000f), lcVector3(-2.000f, 0.0f, -2.000f), lcVector3(-5.602f, 0.0f, -2.000f),
		lcVector3(-2.000f, 0.0f, -5.602f), lcVector3(-2.000f, 0.0f, -2.000f), lcVector3( 0.000f, 0.0f, -6.000f), lcVector3(-2.000f, 0.0f, -5.602f),
		lcVector3( 6.000f, 0.0f,  0.000f), lcVector3( 5.602f, 0.0f, -2.000f), lcVector3( 5.602f, 0.0f, -2.000f), lcVector3( 2.000f, 0.0f, -2.000f),
		lcVector3( 2.000f, 0.0f, -2.000f), lcVector3( 2.000f, 0.0f, -5.602f), lcVector3( 2.000f, 0.0f, -5.602f), lcVector3( 0.000f, 0.0f, -6.000f),
	};

	for (int SectionIdx = 1; SectionIdx < Sections.GetSize() - 1; SectionIdx++)
	{
		for (int VertexIdx = 0; VertexIdx < 32; VertexIdx += 2)
		{
			lcVector3 Vertex1 = lcMul31(MidQuadVertices[VertexIdx], Sections[SectionIdx]);
			lcVector3 Vertex2 = lcMul31(MidQuadVertices[VertexIdx + 1], Sections[SectionIdx]);
			lcVector3 Vertex3 = lcMul31(MidQuadVertices[VertexIdx + 1], Sections[SectionIdx + 1]);
			lcVector3 Vertex4 = lcMul31(MidQuadVertices[VertexIdx], Sections[SectionIdx + 1]);

			sprintf(Line, "4 16 %f %f %f %f %f %f %f %f %f %f %f %f\n", Vertex1[0], Vertex1[1], Vertex1[2], Vertex2[0], Vertex2[1], Vertex2[2], Vertex3[0], Vertex3[1], Vertex3[2], Vertex4[0], Vertex4[1], Vertex4[2]);
			File.WriteBuffer(Line, strlen(Line));
		}
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
}

void lcSynthInfo::AddStringBraidedParts(lcMemFile& File, const lcArray<lcMatrix44>& Sections) const
{
	char Line[256];

	{
		const int SectionIdx = 0;
		lcMatrix33 Transform(lcMul(lcMatrix33(lcVector3(0.0f, 1.0f, 0.0f), lcVector3(1.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 1.0f)), lcMatrix33(Sections[SectionIdx])));
		lcVector3 Offset = lcMul31(lcVector3(0.0f, -8.0f, 0.0f), Sections[SectionIdx]);

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f 572A.DAT\n", Offset[0], Offset[1], Offset[2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2]);

		File.WriteBuffer(Line, strlen(Line));
	}

	const int NumSegments = 16;
	const float PositionTable[16] =
	{
		-1.5f, -1.386f, -1.061f, -0.574f, 0.0f, 0.574f, 1.061f, 1.386f, 1.5f, 1.386f, 1.061f, 0.574f, 0.0f, -0.574f, -1.061f, -1.386f
	};

	for (int SectionIdx = 1; SectionIdx < Sections.GetSize() - 1; SectionIdx++)
	{
		lcMatrix33 Transform1 = lcMul(lcMatrix33(lcVector3(0.0f, 1.0f, 0.0f), lcVector3(1.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 1.0f)), lcMatrix33(Sections[SectionIdx]));
		lcMatrix33 Transform2 = lcMul(lcMatrix33(lcVector3(0.0f, 1.0f, 0.0f), lcVector3(1.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 1.0f)), lcMatrix33(Sections[SectionIdx + 1]));
		lcVector3 Offset1 = Sections[SectionIdx].GetTranslation();
		lcVector3 Offset2 = Sections[SectionIdx + 1].GetTranslation();

		for (int BraidIdx = 0; BraidIdx < 4; BraidIdx++)
		{
			int BaseX = (BraidIdx == 0 || BraidIdx == 2) ? 0 : 8;
			int BaseY = (BraidIdx == 0 || BraidIdx == 3) ? 12 : 4;

			for (int SegmentIdx = 0; SegmentIdx < NumSegments; SegmentIdx++)
			{
				float t1 = (float)SegmentIdx / (float)NumSegments;
				float t2 = (float)(SegmentIdx + 1) / (float)NumSegments;

//				lcVector3 Vertex1 = lcMul(lcVector3((float)(SegmentIdx * 4) / (float)NumSegments, PositionTable[(BaseX + SegmentIdx) % NumSegments], PositionTable[(BaseY + SegmentIdx) % NumSegments]) + lcVector3(0.0f, 1.5f, 0.0f), Transform1) + Offset1;
//				lcVector3 Vertex2 = lcMul(lcVector3((float)((SegmentIdx + 1) * 4) / (float)NumSegments, PositionTable[(BaseX + SegmentIdx + 1) % NumSegments], PositionTable[(BaseY + SegmentIdx + 1) % NumSegments]) + lcVector3(0.0f, 1.5f, 0.0f), Transform1) + Offset1;

				lcVector3 Vertex11 = lcVector3(t1 * 4.0f, PositionTable[(BaseX + SegmentIdx) % NumSegments], PositionTable[(BaseY + SegmentIdx) % NumSegments]) + lcVector3(0.0f, 1.5f, 0.0f);
				lcVector3 Vertex12 = lcVector3((1.0f - t1) * -4.0f, PositionTable[(BaseX + SegmentIdx) % NumSegments], PositionTable[(BaseY + SegmentIdx) % NumSegments]) + lcVector3(0.0f, 1.5f, 0.0f);
				lcVector3 Vertex21 = lcVector3(t2 * 4.0f, PositionTable[(BaseX + SegmentIdx + 1) % NumSegments], PositionTable[(BaseY + SegmentIdx + 1) % NumSegments]) + lcVector3(0.0f, 1.5f, 0.0f);
				lcVector3 Vertex22 = lcVector3((1.0f - t2) * -4.0f, PositionTable[(BaseX + SegmentIdx + 1) % NumSegments], PositionTable[(BaseY + SegmentIdx + 1) % NumSegments]) + lcVector3(0.0f, 1.5f, 0.0f);

				lcVector3 Vertex1 = (lcMul(Vertex11, Transform1) + Offset1) * (1.0f - t1) + (lcMul(Vertex12, Transform2) + Offset2) * t1;
				lcVector3 Vertex2 = (lcMul(Vertex21, Transform1) + Offset1) * (1.0f - t2) + (lcMul(Vertex22, Transform2) + Offset2) * t2;
				
				sprintf(Line, "2 24 %f %f %f %f %f %f\n", Vertex1[0], Vertex1[1], Vertex1[2], Vertex2[0], Vertex2[1], Vertex2[2]);
				File.WriteBuffer(Line, strlen(Line));
			}
		}

		lcMatrix33 Transform = lcMul(lcMatrix33(lcVector3(1.5f, 0.0f, 0.0f), lcVector3(0.0f, 4.0f, 0.0f), lcVector3(0.0f, 0.0f, 1.5f)), lcMatrix33(Sections[SectionIdx]));
		lcVector3 Offset = lcMul31(lcVector3(1.5f, 0.0f, 0.0f), Sections[SectionIdx]);

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f 4-4CYLI.DAT\n", Offset[0], Offset[1], Offset[2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2]);
		File.WriteBuffer(Line, strlen(Line));
	}

	{
		const int SectionIdx = Sections.GetSize() - 1;
		lcMatrix33 Transform(lcMul(lcMatrix33(lcVector3(0.0f, 1.0f, 0.0f), lcVector3(1.0f, 0.0f, 0.0f), lcVector3(0.0f, 0.0f, 1.0f)), lcMatrix33(Sections[SectionIdx])));
		lcVector3 Offset = lcMul31(lcVector3(0.0f, 8.0f, 0.0f), Sections[SectionIdx]);

		sprintf(Line, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f 572A.DAT\n", Offset[0], Offset[1], Offset[2], Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1], Transform[0][2], Transform[1][2], Transform[2][2]);

		File.WriteBuffer(Line, strlen(Line));
	}
}

lcMesh* lcSynthInfo::CreateMesh(const lcArray<lcPieceControlPoint>& ControlPoints) const
{
	lcArray<lcMatrix44> Sections;
	CalculateSections(ControlPoints, Sections, NULL, NULL);

	lcMemFile File; // todo: rewrite this to pass the parts directly

	switch (mType)
	{
	case LC_SYNTH_PIECE_RIBBED_HOSE:
		AddRibbedHoseParts(File, Sections);
		break;

	case LC_SYNTH_PIECE_FLEXIBLE_AXLE:
		AddFlexibleAxleParts(File, Sections);
		break;

	case LC_SYNTH_PIECE_STRING_BRAIDED:
		AddStringBraidedParts(File, Sections);
		break;
	}

	File.WriteU8(0);

	lcLibraryMeshData MeshData;
	lcArray<lcLibraryTextureMap> TextureStack;
	File.Seek(0, SEEK_SET);

	const char* OldLocale = setlocale(LC_NUMERIC, "C");
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

	CalculateSections(ControlPoints, Sections, lcSynthInsertCallback, &SynthInsertParam);

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
