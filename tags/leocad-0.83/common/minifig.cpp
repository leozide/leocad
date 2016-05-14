#include "lc_global.h"
#include "lc_colors.h"
#include "lc_math.h"
#include <string.h>
#include <stdio.h>
#include "minifig.h"
#include "pieceinf.h"
#include "project.h"
#include "lc_model.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_context.h"
#include "lc_file.h"

MinifigWizard::MinifigWizard(lcMinifig* Minifig)
{
	char Filename[LC_MAXPATH];
	strcpy(Filename, lcGetPiecesLibrary()->mLibraryPath);
	strcat(Filename, "mlcad.ini");

	lcDiskFile DiskSettings;
	if (DiskSettings.Open(Filename, "rt"))
	{
		ParseSettings(DiskSettings);
	}
	else
	{
		QResource Resource(":/resources/minifig.ini");

		if (Resource.isValid())
		{
			QByteArray Data;

			if (Resource.isCompressed())
				Data = qUncompress(Resource.data(), Resource.size());
			else
				Data = QByteArray::fromRawData((const char*)Resource.data(), Resource.size());

			lcMemFile MemSettings;
			MemSettings.WriteBuffer(Data.constData(), Data.size());
			ParseSettings(MemSettings);
		}
	}

	mMinifig = Minifig;

	m_RotateX = 75.0f;
	m_RotateZ = 180.0f;
	m_Distance = 10.0f;
	m_AutoZoom = true;
	m_Tracking = LC_TRACK_NONE;
}

void MinifigWizard::OnInitialUpdate()
{
	MakeCurrent();
	mContext->SetDefaultState();

	memset(mMinifig, 0, sizeof(lcMinifig));

	const int ColorCodes[LC_MFW_NUMITEMS] = { 4, 7, 14, 7, 1, 0, 7, 4, 4, 14, 14, 7, 7, 0, 0, 7, 7 };
	const char* Pieces[LC_MFW_NUMITEMS] = { "3624", "None", "3626BP01", "None", "973", "3815", "None", "3819", "3818", "3820", "3820", "None", "None", "3817", "3816", "None", "None" };

	for (int i = 0; i < LC_MFW_NUMITEMS; i++)
	{
		mMinifig->Colors[i] = lcGetColorIndex(ColorCodes[i]);

		PieceInfo* Info = lcGetPiecesLibrary()->FindPiece(Pieces[i], NULL, false);
		if (Info)
		{
			mMinifig->Parts[i] = Info;
			Info->AddRef();
		}
	}

	Calculate();
}

MinifigWizard::~MinifigWizard()
{
	for (int i = 0; i < LC_MFW_NUMITEMS; i++)
		if (mMinifig->Parts[i])
			mMinifig->Parts[i]->Release(true);
}

void MinifigWizard::ParseSettings(lcFile& Settings)
{
	const char* SectionNames[LC_MFW_NUMITEMS] =
	{
		"[HATS]",   // LC_MFW_HATS
		"[HATS2]",  // LC_MFW_HATS2
		"[HEAD]",   // LC_MFW_HEAD
		"[NECK]",   // LC_MFW_NECK
		"[BODY]",   // LC_MFW_BODY
		"[BODY2]",  // LC_MFW_BODY2
		"[BODY3]",  // LC_MFW_BODY3
		"[RARM]",   // LC_MFW_RARM
		"[LARM]",   // LC_MFW_LARM
		"[RHAND]",  // LC_MFW_RHAND
		"[LHAND]",  // LC_MFW_LHAND
		"[RHANDA]", // LC_MFW_RHANDA
		"[LHANDA]", // LC_MFW_LHANDA
		"[RLEG]",   // LC_MFW_RLEG
		"[LLEG]",   // LC_MFW_LLEG
		"[RLEGA]",  // LC_MFW_RLEGA
		"[LLEGA]",  // LC_MFW_LLEGA
	};

	for (int SectionIndex = 0; SectionIndex < LC_MFW_NUMITEMS; SectionIndex++)
	{
		lcArray<lcMinifigPieceInfo>& InfoArray = mSettings[SectionIndex];

		InfoArray.RemoveAll();
		Settings.Seek(0, SEEK_SET);

		char Line[1024];
		bool FoundSection = false;
		const char* SectionName = SectionNames[SectionIndex];
		size_t SectionNameLength = strlen(SectionName);

		// Find start of section
		while (Settings.ReadLine(Line, sizeof(Line)))
		{
			if (!strncmp(Line, SectionName, SectionNameLength))
			{
				FoundSection = true;
				break;
			}
		}

		if (!FoundSection)
		{

			lcMinifigPieceInfo MinifigInfo;
			strncpy(MinifigInfo.Description, "None", sizeof(MinifigInfo.Description));
			MinifigInfo.Description[sizeof(MinifigInfo.Description)-1] = 0;
			MinifigInfo.Offset = lcMatrix44Identity();
			MinifigInfo.Info = NULL;

			InfoArray.Add(MinifigInfo);
			continue;
		}

		// Parse section.
		while (Settings.ReadLine(Line, sizeof(Line)))
		{
			if (Line[0] == '[')
				break;

			char* DescriptionStart = strchr(Line, '"');
			if (!DescriptionStart)
				continue;
			DescriptionStart++;
			char* DescriptionEnd = strchr(DescriptionStart, '"');
			if (!DescriptionEnd)
				continue;
			*DescriptionEnd = 0;
			DescriptionEnd++;

			char* NameStart = strchr(DescriptionEnd, '"');
			if (!NameStart)
				continue;
			NameStart++;
			char* NameEnd = strchr(NameStart, '"');
			if (!NameEnd)
				continue;
			*NameEnd = 0;
			NameEnd++;

			strupr(NameStart);
			char* Ext = strrchr(NameStart, '.');
			if (Ext != NULL)
			{
				if (!strcmp(Ext, ".DAT"))
					*Ext = 0;
			}

			PieceInfo* Info = lcGetPiecesLibrary()->FindPiece(NameStart, NULL, false);
			if (!Info && *NameStart)
				continue;

			float Mat[12];
			int Flags;

			if (sscanf(NameEnd, "%d %g %g %g %g %g %g %g %g %g %g %g %g",
					   &Flags, &Mat[0], &Mat[1], &Mat[2], &Mat[3], &Mat[4], &Mat[5], &Mat[6], 
					   &Mat[7], &Mat[8], &Mat[9], &Mat[10], &Mat[11]) != 13)
				continue;

			lcMatrix44 Offset = lcMatrix44Identity();
			float* OffsetMatrix = &Offset[0][0];

			OffsetMatrix[0] =  Mat[0];
			OffsetMatrix[8] = -Mat[1];
			OffsetMatrix[4] =  Mat[2];
			OffsetMatrix[2] = -Mat[3];
			OffsetMatrix[10] = Mat[4];
			OffsetMatrix[6] = -Mat[5];
			OffsetMatrix[1] =  Mat[6];
			OffsetMatrix[9] = -Mat[7];
			OffsetMatrix[5] =  Mat[8];
			OffsetMatrix[12] =  Mat[9];
			OffsetMatrix[14] = -Mat[10];
			OffsetMatrix[13] =  Mat[11];

			lcMinifigPieceInfo MinifigInfo;
			strncpy(MinifigInfo.Description, DescriptionStart, sizeof(MinifigInfo.Description));
			MinifigInfo.Description[sizeof(MinifigInfo.Description)-1] = 0;
			MinifigInfo.Offset = Offset;
			MinifigInfo.Info = Info;

			InfoArray.Add(MinifigInfo);
		}
	}
}

void MinifigWizard::OnDraw()
{
	mContext->SetDefaultState();

	float Aspect = (float)mWidth/(float)mHeight;
	mContext->SetViewport(0, 0, mWidth, mHeight);

	lcGetActiveModel()->DrawBackground(mContext);

	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int InfoIdx = 0; InfoIdx < LC_MFW_NUMITEMS; InfoIdx++)
	{
		PieceInfo* Info = mMinifig->Parts[InfoIdx];

		if (!Info)
			continue;

		lcVector3 Points[8];
		lcGetBoxCorners(Info->GetBoundingBox(), Points);

		for (int PointIdx = 0; PointIdx < 8; PointIdx++)
		{
			lcVector3 Point = lcMul31(Points[PointIdx], mMinifig->Matrices[InfoIdx]);

			Min = lcMin(Point, Min);
			Max = lcMax(Point, Max);
		}
	}

	lcVector3 Center = (Min + Max) / 2.0f;

	lcVector3 Eye(0.0f, 0.0f, 1.0f);

	Eye = lcMul30(Eye, lcMatrix44RotationX(-m_RotateX * LC_DTOR));
	Eye = lcMul30(Eye, lcMatrix44RotationZ(-m_RotateZ * LC_DTOR));

	lcMatrix44 Projection = lcMatrix44Perspective(30.0f, Aspect, 1.0f, 2500.0f);
	mContext->SetProjectionMatrix(Projection);

	lcMatrix44 ViewMatrix;

	if (m_AutoZoom)
	{
		lcVector3 Points[8];
		lcGetBoxCorners(Min, Max, Points);

		Eye += Center;

		lcMatrix44 ModelView = lcMatrix44LookAt(Eye, Center, lcVector3(0, 0, 1));
		Eye = lcZoomExtents(Eye, ModelView, Projection, Points, 8);

		ViewMatrix = lcMatrix44LookAt(Eye, Center, lcVector3(0, 0, 1));

		// Update the new camera distance.
		lcVector3 d = Eye - Center;
		m_Distance = d.Length();
	}
	else
	{
		ViewMatrix = lcMatrix44LookAt(Eye * m_Distance, Center, lcVector3(0, 0, 1));
	}

	mContext->SetViewMatrix(ViewMatrix);

	Calculate();

	lcScene Scene;
	Scene.Begin(ViewMatrix);

	for (int PieceIdx = 0; PieceIdx < LC_MFW_NUMITEMS; PieceIdx++)
		if (mMinifig->Parts[PieceIdx])
			mMinifig->Parts[PieceIdx]->AddRenderMeshes(Scene, mMinifig->Matrices[PieceIdx], mMinifig->Colors[PieceIdx], false, false);

	Scene.End();

	mContext->SetProgram(LC_PROGRAM_SIMPLE);
	mContext->DrawOpaqueMeshes(Scene.mOpaqueMeshes);
	mContext->DrawTranslucentMeshes(Scene.mTranslucentMeshes);

	mContext->UnbindMesh(); // context remove
}

void MinifigWizard::OnLeftButtonDown()
{
	if (m_Tracking == LC_TRACK_NONE)
	{
		m_DownX = mInputState.x;
		m_DownY = mInputState.y;
		m_Tracking = LC_TRACK_LEFT;
	}
}

void MinifigWizard::OnLeftButtonUp()
{
	if (m_Tracking == LC_TRACK_LEFT)
		m_Tracking = LC_TRACK_NONE;
}

void MinifigWizard::OnLeftButtonDoubleClick()
{
	m_AutoZoom = true;
	Redraw();
}

void MinifigWizard::OnRightButtonDown()
{
	if (m_Tracking == LC_TRACK_NONE)
	{
		m_DownX = mInputState.x;
		m_DownY = mInputState.y;
		m_Tracking = LC_TRACK_RIGHT;
	}
}

void MinifigWizard::OnRightButtonUp()
{
	if (m_Tracking == LC_TRACK_RIGHT)
		m_Tracking = LC_TRACK_NONE;
}

void MinifigWizard::OnMouseMove()
{
	if (m_Tracking == LC_TRACK_LEFT)
	{
		// Rotate.
		m_RotateZ += mInputState.x - m_DownX;
		m_RotateX += mInputState.y - m_DownY;

		if (m_RotateX > 179.5f)
			m_RotateX = 179.5f;
		else if (m_RotateX < 0.5f)
			m_RotateX = 0.5f;

		m_DownX = mInputState.x;
		m_DownY = mInputState.y;

		Redraw();
	}
	else if (m_Tracking == LC_TRACK_RIGHT)
	{
		// Zoom.
		m_Distance += (float)(m_DownY - mInputState.y) * 0.2f;
		m_AutoZoom = false;

		if (m_Distance < 0.5f)
			m_Distance = 0.5f;

		m_DownX = mInputState.x;
		m_DownY = mInputState.y;

		Redraw();
	}
}

void MinifigWizard::Calculate()
{
	float HeadOffset = 0.0f;
	lcMatrix44 Root, Mat, Mat2;

	PieceInfo** Parts = mMinifig->Parts;
	float* Angles = mMinifig->Angles;
	lcMatrix44* Matrices = mMinifig->Matrices;

	bool DroidTorso = Parts[LC_MFW_BODY] && !strcmp(Parts[LC_MFW_BODY]->m_strName, "30375");
	bool SkeletonTorso = Parts[LC_MFW_BODY] && !strcmp(Parts[LC_MFW_BODY]->m_strName, "6260");

	if (Parts[LC_MFW_BODY3])
		Root = lcMatrix44Translation(lcVector3(0, 0, 74.0f));
	else
		Root = lcMatrix44Translation(lcVector3(0, 0, 72.0f));
	Matrices[LC_MFW_BODY] = lcMul(mSettings[LC_MFW_BODY][GetSelectionIndex(LC_MFW_BODY)].Offset, Root);

	if (Parts[LC_MFW_NECK])
	{
		Matrices[LC_MFW_NECK] = lcMul(mSettings[LC_MFW_NECK][GetSelectionIndex(LC_MFW_NECK)].Offset, Root);
		HeadOffset = 0.08f;
	}

	if (Parts[LC_MFW_HEAD])
	{
		Mat = lcMatrix44RotationZ(-LC_DTOR * Angles[LC_MFW_HEAD]);
		Mat.SetTranslation(lcVector3(0.0f, 0.0f, 24.0f + HeadOffset));
		Mat = lcMul(mSettings[LC_MFW_HEAD][GetSelectionIndex(LC_MFW_HEAD)].Offset, Mat);
		Matrices[LC_MFW_HEAD] = lcMul(Mat, Root);
	}

	if (Parts[LC_MFW_HATS])
	{
		Mat = lcMatrix44RotationZ(-LC_DTOR * Angles[LC_MFW_HATS]);
		Mat = lcMul(mSettings[LC_MFW_HATS][GetSelectionIndex(LC_MFW_HATS)].Offset, Mat);
		Matrices[LC_MFW_HATS] = lcMul(Mat, Matrices[LC_MFW_HEAD]);
	}

	if (Parts[LC_MFW_HATS2])
	{
		Mat = lcMatrix44RotationX(-LC_DTOR * Angles[LC_MFW_HATS2]);
		Mat = lcMul(mSettings[LC_MFW_HATS2][GetSelectionIndex(LC_MFW_HATS2)].Offset, Mat);
		Matrices[LC_MFW_HATS2] = lcMul(Mat, Matrices[LC_MFW_HATS]);
	}

	if (Parts[LC_MFW_RARM])
	{
		Mat = lcMatrix44RotationX(-LC_DTOR * Angles[LC_MFW_RARM]);

		if (DroidTorso || SkeletonTorso)
			Mat2 = lcMatrix44Identity();
		else
			Mat2 = lcMatrix44RotationY(-LC_DTOR * 9.791f);
		Mat2.SetTranslation(lcVector3(15.5f, 0, -8.0f));

		Mat = lcMul(mSettings[LC_MFW_RARM][GetSelectionIndex(LC_MFW_RARM)].Offset, Mat);
		Mat = lcMul(Mat, Mat2);
		Matrices[LC_MFW_RARM] = lcMul(Mat, Root);
	}

	if (Parts[LC_MFW_RHAND])
	{
		Mat = lcMatrix44RotationY(-LC_DTOR * Angles[LC_MFW_RHAND]);
		Mat2 = lcMatrix44RotationX(LC_DTOR * 45);
		Mat = lcMul(mSettings[LC_MFW_RHAND][GetSelectionIndex(LC_MFW_RHAND)].Offset, Mat);
		Mat = lcMul(Mat, Mat2);
		Mat.SetTranslation(lcVector3(5.0f, -10.0f, -19.0f));
		Matrices[LC_MFW_RHAND] = lcMul(Mat, Matrices[LC_MFW_RARM]);
	}

	if (Parts[LC_MFW_RHANDA])
	{
		Mat = lcMatrix44RotationZ(LC_DTOR * Angles[LC_MFW_RHANDA]);
		Mat.SetTranslation(lcVector3(0, -10.0f, 0));
		Mat = lcMul(mSettings[LC_MFW_RHANDA][GetSelectionIndex(LC_MFW_RHANDA)].Offset, Mat);
		Matrices[LC_MFW_RHANDA] = lcMul(Mat, Matrices[LC_MFW_RHAND]);
	}

	if (Parts[LC_MFW_LARM])
	{
		Mat = lcMatrix44RotationX(-LC_DTOR * Angles[LC_MFW_LARM]);

		if (DroidTorso || SkeletonTorso)
			Mat2 = lcMatrix44Identity();
		else
			Mat2 = lcMatrix44RotationY(LC_DTOR * 9.791f);
		Mat2.SetTranslation(lcVector3(-15.5f, 0.0f, -8.0f));

		Mat = lcMul(mSettings[LC_MFW_LARM][GetSelectionIndex(LC_MFW_LARM)].Offset, Mat);
		Mat = lcMul(Mat, Mat2);
		Matrices[LC_MFW_LARM] = lcMul(Mat, Root);
	}

	if (Parts[LC_MFW_LHAND])
	{
		Mat = lcMatrix44RotationY(-LC_DTOR * Angles[LC_MFW_LHAND]);
		Mat2 = lcMatrix44RotationX(LC_DTOR * 45);
		Mat = lcMul(mSettings[LC_MFW_LHAND][GetSelectionIndex(LC_MFW_LHAND)].Offset, Mat);
		Mat = lcMul(Mat, Mat2);
		Mat.SetTranslation(lcVector3(-5.0f, -10.0f, -19.0f));
		Matrices[LC_MFW_LHAND] = lcMul(Mat, Matrices[LC_MFW_LARM]);
	}

	if (Parts[LC_MFW_LHANDA])
	{
		Mat = lcMatrix44RotationZ(LC_DTOR * Angles[LC_MFW_LHANDA]);
		Mat.SetTranslation(lcVector3(0, -10.0f, 0));
		Mat = lcMul(mSettings[LC_MFW_LHANDA][GetSelectionIndex(LC_MFW_LHANDA)].Offset, Mat);
		Matrices[LC_MFW_LHANDA] = lcMul(Mat, Matrices[LC_MFW_LHAND]);
	}

	if (Parts[LC_MFW_BODY2])
	{
		Mat = lcMatrix44Identity();
		Mat.SetTranslation(lcVector3(0, 0, -32.0f));
		Mat = lcMul(mSettings[LC_MFW_BODY2][GetSelectionIndex(LC_MFW_BODY2)].Offset, Mat);
		Matrices[LC_MFW_BODY2] = lcMul(Mat, Root);
	}

	if (Parts[LC_MFW_BODY3])
	{
		Mat = lcMatrix44Identity();
		Mat.SetTranslation(lcVector3(0, 0, -32.0f));
		Mat = lcMul(mSettings[LC_MFW_BODY3][GetSelectionIndex(LC_MFW_BODY3)].Offset, Mat);
		Matrices[LC_MFW_BODY3] = lcMul(Mat, Root);
	}

	if (Parts[LC_MFW_RLEG])
	{
		Mat = lcMatrix44RotationX(-LC_DTOR * Angles[LC_MFW_RLEG]);
		Mat.SetTranslation(lcVector3(0, 0, -44.0f));
		Mat = lcMul(mSettings[LC_MFW_RLEG][GetSelectionIndex(LC_MFW_RLEG)].Offset, Mat);
		Matrices[LC_MFW_RLEG] = lcMul(Mat, Root);
	}

	if (Parts[LC_MFW_RLEGA])
	{
		lcVector3 Center(-10.0f, -1.0f, -28.0f);
		Mat = lcMatrix44RotationZ(LC_DTOR * Angles[LC_MFW_RLEGA]);
		Mat2 = mSettings[LC_MFW_RLEGA][GetSelectionIndex(LC_MFW_RLEGA)].Offset;
		Mat2.SetTranslation(lcMul31(-Center, Mat2));
		Mat = lcMul(Mat2, Mat);
		Mat.SetTranslation(lcMul31(Center, Mat2));
		Matrices[LC_MFW_RLEGA] = lcMul(Mat, Matrices[LC_MFW_RLEG]);
	}

	if (Parts[LC_MFW_LLEG])
	{
		Mat = lcMatrix44RotationX(-LC_DTOR * Angles[LC_MFW_LLEG]);
		Mat.SetTranslation(lcVector3(0, 0, -44.0f));
		Mat = lcMul(mSettings[LC_MFW_LLEG][GetSelectionIndex(LC_MFW_LLEG)].Offset, Mat);
		Matrices[LC_MFW_LLEG] = lcMul(Mat, Root);
	}

	if (Parts[LC_MFW_LLEGA])
	{
		lcVector3 Center(10.0f, -1.0f, -28.0f);
		Mat = lcMatrix44RotationZ(LC_DTOR * Angles[LC_MFW_LLEGA]);
		Mat2 = mSettings[LC_MFW_LLEGA][GetSelectionIndex(LC_MFW_LLEGA)].Offset;
		Mat2.SetTranslation(lcMul31(-Center, Mat2));
		Mat = lcMul(Mat2, Mat);
		Mat.SetTranslation(lcMul31(Center, Mat2));
		Matrices[LC_MFW_LLEGA] = lcMul(Mat, Matrices[LC_MFW_LLEG]);
	}
}

int MinifigWizard::GetSelectionIndex(int Type) const
{
	const lcArray<lcMinifigPieceInfo>& InfoArray = mSettings[Type];

	for (int Index = 0; Index < InfoArray.GetSize(); Index++)
		if (InfoArray[Index].Info == mMinifig->Parts[Type])
			return Index;

	return 0;
}

void MinifigWizard::SetSelectionIndex(int Type, int Index)
{
	MakeCurrent();

	if (mMinifig->Parts[Type])
		mMinifig->Parts[Type]->Release(true);

	mMinifig->Parts[Type] = mSettings[Type][Index].Info;

	if (mMinifig->Parts[Type])
		mMinifig->Parts[Type]->AddRef();

	Calculate();
}

void MinifigWizard::SetColor(int Type, int Color)
{
	mMinifig->Colors[Type] = Color;
}

void MinifigWizard::SetAngle(int Type, float Angle)
{
	mMinifig->Angles[Type] = Angle;
}
