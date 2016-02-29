#include "lc_global.h"
#include "lc_synth.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"

void lcSynthInit()
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	struct lcRibbedHoseInfo
	{
		const char* PartID;
		float Length;
		int NumSections;
	};

	lcRibbedHoseInfo RibbedHoseInfo[] =
	{
		{ "72504",  15.625f, 4 }, // Technic Ribbed Hose  2L
		{ "72706",  25.0f,   7 }, // Technic Ribbed Hose  3L
		{ "71952",  37.5f,  11 }, // Technic Ribbed Hose  4L
		{ "71944",  56.25f, 17 }, // Technic Ribbed Hose  6L
		{ "71951",  71.875, 22 }, // Technic Ribbed Hose  8L
		{ "71986", 106.25f, 33 }, // Technic Ribbed Hose 11L
		{ "43675", 187.5f,  58 }  // Technic Ribbed Hose 19L
	};

	lcMatrix33 RotationY = lcMatrix33RotationY(LC_PI / 2.0f);

	for (int InfoIdx = 0; InfoIdx < sizeof(RibbedHoseInfo) / sizeof(RibbedHoseInfo[0]); InfoIdx++)
	{
		PieceInfo* Info = Library->FindPiece(RibbedHoseInfo[InfoIdx].PartID, NULL, false);
		if (Info)
		{
			lcSynthInfo* SynthInfo = new lcSynthInfo();

			float Length = RibbedHoseInfo[InfoIdx].Length;
			SynthInfo->DefaultControlPoints[0] = lcMatrix44(RotationY, lcVector3(-Length, 0.0f, 0.0f));
			SynthInfo->DefaultControlPoints[1] = lcMatrix44(RotationY, lcVector3( Length, 0.0f, 0.0f));
			SynthInfo->DefaultStiffness = 80.0f;
			SynthInfo->NumSections = RibbedHoseInfo[InfoIdx].NumSections;

			SynthInfo->Components[0].Transform = lcMatrix44Identity();
			SynthInfo->Components[0].Transform[1][1] = -1.0f;
			strcpy(SynthInfo->Components[0].PartID, "79.DAT");
			SynthInfo->Components[0].Length = 6.25f;
			SynthInfo->Components[1].Transform = lcMatrix44Identity();
			strcpy(SynthInfo->Components[1].PartID, "80.DAT");
			SynthInfo->Components[1].Length = 6.25f;
			SynthInfo->Components[2].Transform = lcMatrix44Identity();
			strcpy(SynthInfo->Components[2].PartID, "79.DAT");
			SynthInfo->Components[2].Length = 6.25f;
			Info->SetSynthInfo(SynthInfo);
		}
	}
}

inline lcMatrix44 lcMatrix44LeoCADToLDraw(const lcMatrix44& Matrix)
{
	lcMatrix44 m;

	m.r[0] = lcVector4( Matrix[0][0], -Matrix[2][0],  Matrix[1][0], 0.0f);
	m.r[1] = lcVector4(-Matrix[0][2],  Matrix[2][2], -Matrix[1][2], 0.0f);
	m.r[2] = lcVector4( Matrix[0][1], -Matrix[2][1],  Matrix[1][1], 0.0f);
	m.r[3] = lcVector4( Matrix[3][0], -Matrix[3][2],  Matrix[3][1], 1.0f);

	return m;
}

#include "lc_file.h"

lcMesh* lcSynthCreateMesh(lcSynthInfo* SynthInfo, const lcArray<lcPieceControlPoint>& ControlPoints)
{
	lcArray<lcMatrix44> Sections;
	float SectionLength = 0.0f;

	for (int ControlPointIdx = 0; ControlPointIdx < ControlPoints.GetSize() - 1; ControlPointIdx++)
	{
		lcVector3 SegmentControlPoints[4];

		lcMatrix44 StartTransform = lcMatrix44LeoCADToLDraw(ControlPoints[ControlPointIdx].Transform);
		lcMatrix44 EndTransform = lcMatrix44LeoCADToLDraw(ControlPoints[ControlPointIdx + 1].Transform);

		if (ControlPointIdx == 0)
		{
			StartTransform = lcMul(StartTransform, SynthInfo->Components[0].Transform);
			Sections.Add(StartTransform);
			SectionLength = SynthInfo->Components[0].Length;
		}

		if (ControlPointIdx == ControlPoints.GetSize() - 2)
			EndTransform = lcMul(EndTransform, lcMatrix44(lcVector4(1.0f, 0.0f, 0.0f, 0.0f), lcVector4(0.0f, -1.0f, 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f)));

		SegmentControlPoints[0] = StartTransform.GetTranslation();
		SegmentControlPoints[1] = lcMul31(lcVector3(0.0f, ControlPoints[ControlPointIdx].Stiffness, 0.0f), StartTransform);
		SegmentControlPoints[2] = lcMul31(lcVector3(0.0f, -ControlPoints[ControlPointIdx + 1].Stiffness, 0.0f), EndTransform);
		SegmentControlPoints[3] = EndTransform.GetTranslation();

		float PointDistance = lcLength(SegmentControlPoints[3] - SegmentControlPoints[0]);
		float PointDot = lcDot(SegmentControlPoints[1], SegmentControlPoints[2]);

		if ((PointDistance < ControlPoints[ControlPointIdx].Stiffness + ControlPoints[ControlPointIdx + 1].Stiffness) && (PointDot <= 0.001f))
		{
			float Scale = 1.0f / (ControlPoints[ControlPointIdx].Stiffness + ControlPoints[ControlPointIdx + 1].Stiffness);

			SegmentControlPoints[1] = lcMul31(lcVector3(0.0f, ControlPoints[ControlPointIdx].Stiffness * Scale, 0.0f), StartTransform);
			SegmentControlPoints[2] = lcMul31(lcVector3(0.0f, -ControlPoints[ControlPointIdx + 1].Stiffness * Scale, 0.0f), EndTransform);
		}

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
			TotalSegmentLength += lcLength(CurvePoints[CurrentPointIndex] - CurvePoints[CurrentPointIndex + 1]);

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

			if (Sections.GetSize() == SynthInfo->NumSections + 2)
				break;

			if (Sections.GetSize() < SynthInfo->NumSections + 1)
				SectionLength += SynthInfo->Components[1].Length;
			else
				SectionLength += SynthInfo->Components[2].Length;
		}
	}

	while (Sections.GetSize() < SynthInfo->NumSections + 2)
	{
		lcMatrix44 EndTransform = lcMatrix44LeoCADToLDraw(ControlPoints[ControlPoints.GetSize() - 1].Transform);
		EndTransform = lcMul(EndTransform, lcMatrix44(lcVector4(1.0f, 0.0f, 0.0f, 0.0f), lcVector4(0.0f, -1.0f, 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f)));
		lcVector3 Position = lcMul31(lcVector3(0.0f, SectionLength, 0.0f), EndTransform);
		EndTransform.SetTranslation(Position);
		Sections.Add(EndTransform);

		if (Sections.GetSize() < SynthInfo->NumSections + 1)
			SectionLength += SynthInfo->Components[1].Length;
		else
			SectionLength += SynthInfo->Components[2].Length;
	}

	// todo: rewrite this to pass the parts directly

	lcMemFile f;

	for (int i = 0; i < Sections.GetSize(); i++)
	{
		char str[256];

		const lcMatrix44& Transform = Sections[i];
		const char* Name;
		if (i == 0)
			Name = SynthInfo->Components[0].PartID;
		else if (i == Sections.GetSize() - 1)
			Name = SynthInfo->Components[2].PartID;
		else
			Name = SynthInfo->Components[1].PartID;

		sprintf(str, "1 16 %f %f %f %f %f %f %f %f %f %f %f %f %s\n", Transform[3][0], Transform[3][1], Transform[3][2],
				Transform[0][0], Transform[1][0], Transform[2][0],
				Transform[0][1], Transform[1][1], Transform[2][1],
				Transform[0][2], Transform[1][2], Transform[2][2],
				Name);

		f.WriteBuffer(str, strlen(str));
	}

	f.WriteU8(0);

	lcLibraryMeshData MeshData;
	lcArray<lcLibraryTextureMap> TextureStack;
	f.Seek(0, SEEK_SET);

	const char* OldLocale = setlocale(LC_NUMERIC, "C");
	bool Ret = lcGetPiecesLibrary()->ReadMeshData(f, lcMatrix44Identity(), 16, TextureStack, MeshData, LC_MESHDATA_SHARED, false);
	setlocale(LC_NUMERIC, OldLocale);

	if (Ret)
		return lcGetPiecesLibrary()->CreateMesh(NULL, MeshData);

	return NULL;
}
