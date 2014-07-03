#include "lc_global.h"
#include "lc_math.h"
#include "lc_mesh.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <locale.h>
#include "opengl.h"
#include "pieceinf.h"
#include "lc_texture.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "terrain.h"
#include "project.h"
#include "image.h"
#include "system.h"
#include "minifig.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "lc_library.h"
#include "texfont.h"
#include "debug.h"
#include "lc_application.h"
#include "lc_profile.h"
#include "lc_context.h"
#include "preview.h"

Project::Project()
{
	m_bModified = false;
	mTransformType = LC_TRANSFORM_RELATIVE_TRANSLATION;
	m_pTerrain = new Terrain();
	m_pBackground = new lcTexture();
	mGridTexture = new lcTexture();
	memset(&mSearchOptions, 0, sizeof(mSearchOptions));

	m_pScreenFont = new TexFont();
}

Project::~Project()
{
	DeleteContents(false);

	delete m_pTerrain;
	delete m_pBackground;
	delete mGridTexture;
	delete m_pScreenFont;
}

void Project::UpdateInterface()
{
	// Update all user interface elements.
	gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : NULL, !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : NULL);
	gMainWindow->UpdatePaste(g_App->mClipboard != NULL);
	gMainWindow->UpdateCategories();
	gMainWindow->UpdateTitle(m_strTitle, m_bModified);
	gMainWindow->SetTool(gMainWindow->GetTool());

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateTransformType(mTransformType);
	gMainWindow->UpdateLockSnap(m_nSnap);
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu();
	gMainWindow->UpdatePerspective();

	UpdateSelection();
	gMainWindow->UpdateTime(m_nCurStep, 255);

	const lcArray<View*> Views = gMainWindow->GetViews();
	for (int i = 0; i < Views.GetSize(); i++)
	{
		Views[i]->MakeCurrent();
		RenderInitialize();
	}

	UpdateSelection();
}

void Project::SetTitle(const char* Title)
{
	strcpy(m_strTitle, Title);

	gMainWindow->UpdateTitle(m_strTitle, m_bModified);
}

void Project::DeleteContents(bool bUndo)
{
	mProperties.LoadDefaults();

	if (!bUndo)
	{
		mUndoHistory.DeleteAll();
		mRedoHistory.DeleteAll();

		m_pBackground->Unload();
		m_pTerrain->LoadDefaults(true);
	}

	mPieces.DeleteAll();

	if (!bUndo)
	{
		const lcArray<View*> Views = gMainWindow->GetViews();
		for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
		{
			View* view = Views[ViewIdx];

			if (!view->mCamera->IsSimple())
				view->SetDefaultCamera();
		}
	}

	mCameras.DeleteAll();
	mLights.DeleteAll();
	mGroups.DeleteAll();
}

// Only call after DeleteContents()
void Project::LoadDefaults(bool cameras)
{
	int i;

	mProperties.LoadDefaults();

	// Default values
	gMainWindow->SetColorIndex(lcGetColorIndex(4));
	gMainWindow->SetTool(LC_TOOL_SELECT);
	gMainWindow->SetAddKeys(false);
	gMainWindow->UpdateUndoRedo(NULL, NULL);
	m_nAngleSnap = (unsigned short)lcGetProfileInt(LC_PROFILE_ANGLE_SNAP);
	m_nSnap = lcGetProfileInt(LC_PROFILE_SNAP);
	gMainWindow->UpdateLockSnap(m_nSnap);
	m_nMoveSnap = 0x0304;
	gMainWindow->UpdateSnap();
	m_nCurStep = 1;
	gMainWindow->UpdateTime(1, 255);
	strcpy(m_strHeader, "");
	strcpy(m_strFooter, "Page &P");
	m_pTerrain->LoadDefaults(true);

	const lcArray<View*> Views = gMainWindow->GetViews();
	for (i = 0; i < Views.GetSize (); i++)
	{
		Views[i]->MakeCurrent();
		RenderInitialize();
	}

	if (cameras)
	{
		for (i = 0; i < Views.GetSize(); i++)
			if (!Views[i]->mCamera->IsSimple())
				Views[i]->SetDefaultCamera();

		gMainWindow->UpdateCameraMenu();
	}

	SystemPieceComboAdd(NULL);
	UpdateSelection();
	gMainWindow->UpdateFocusObject(NULL);
}

/////////////////////////////////////////////////////////////////////////////
// Standard file menu commands

// Read a .lcd file
bool Project::FileLoad(lcFile* file, bool bUndo, bool bMerge)
{
	lcint32 i, count;
	char id[32];
	lcuint32 rgb;
	float fv = 0.4f;
	lcuint8 ch;
	lcuint16 sh;

	file->Seek(0, SEEK_SET);
	file->ReadBuffer(id, 32);
	sscanf(&id[7], "%f", &fv);

	// Fix the ugly floating point reading on computers with different decimal points.
	if (fv == 0.0f)
	{
		lconv *loc = localeconv();
		id[8] = loc->decimal_point[0];
		sscanf(&id[7], "%f", &fv);

		if (fv == 0.0f)
			return false;
	}

	if (fv > 0.4f)
		file->ReadFloats(&fv, 1);

	file->ReadU32(&rgb, 1);
	if (!bMerge)
	{
		mProperties.mBackgroundSolidColor[0] = (float)((unsigned char) (rgb))/255;
		mProperties.mBackgroundSolidColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
		mProperties.mBackgroundSolidColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	}

	if (fv < 0.6f) // old view
	{
		Camera* pCam = new Camera(false);
		pCam->CreateName(mCameras);
		mCameras.Add(pCam);

		double eye[3], target[3];
		file->ReadDoubles(eye, 3);
		file->ReadDoubles(target, 3);
		float tmp[3] = { (float)eye[0], (float)eye[1], (float)eye[2] };
		pCam->ChangeKey(1, false, tmp, LC_CK_EYE);
		tmp[0] = (float)target[0]; tmp[1] = (float)target[1]; tmp[2] = (float)target[2];
		pCam->ChangeKey(1, false, tmp, LC_CK_TARGET);

		// Create up vector
		lcVector3 UpVector(0, 0, 1), FrontVector((float)(eye[0] - target[0]), (float)(eye[1] - target[1]), (float)(eye[2] - target[2])), SideVector;
		FrontVector.Normalize();
		if (FrontVector == UpVector)
			SideVector = lcVector3(1, 0, 0);
		else
			SideVector = lcCross(FrontVector, UpVector);
		UpVector = lcNormalize(lcCross(SideVector, FrontVector));
		pCam->ChangeKey(1, false, UpVector, LC_CK_UP);
	}

	if (bMerge)
		file->Seek(32, SEEK_CUR);
	else
	{
		lcuint32 u;
		float f;
		file->ReadS32(&i, 1); m_nAngleSnap = i;
		file->ReadU32(&u, 1); //m_nSnap
		file->ReadFloats(&f, 1); //m_fLineWidth
		file->ReadU32(&u, 1); //m_nDetail
		file->ReadS32(&i, 1); //m_nCurGroup = i;
		file->ReadS32(&i, 1); //m_nCurColor = i;
		file->ReadS32(&i, 1); //action = i;
		file->ReadS32(&i, 1); m_nCurStep = i;
	}

	if (fv > 0.8f)
		file->ReadU32();//m_nScene

	file->ReadS32(&count, 1);
//	SystemStartProgressBar(0, count, 1, "Loading project...");
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	Library->OpenCache();

	int FirstNewPiece = mPieces.GetSize();

	while (count--)
	{
		if (fv > 0.4f)
		{
			Piece* pPiece = new Piece(NULL);
			pPiece->FileLoad(*file);

			if (bMerge)
				for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
					if (strcmp(mPieces[PieceIdx]->GetName(), pPiece->GetName()) == 0)
					{
						pPiece->CreateName(mPieces);
						break;
					}

			if (strlen(pPiece->GetName()) == 0)
				pPiece->CreateName(mPieces);

			mPieces.Add(pPiece);
			if (!bUndo)
				SystemPieceComboAdd(pPiece->mPieceInfo->m_strDescription);
		}
		else
		{
			char name[LC_PIECE_NAME_LEN];
			float pos[3], rot[3];
			lcuint8 color, step, group;

			file->ReadFloats(pos, 3);
			file->ReadFloats(rot, 3);
			file->ReadU8(&color, 1);
			file->ReadBuffer(name, 9);
			file->ReadU8(&step, 1);
			file->ReadU8(&group, 1);

			PieceInfo* pInfo = Library->FindPiece(name, true);
			Piece* pPiece = new Piece(pInfo);

			pPiece->Initialize(pos[0], pos[1], pos[2], step);
			pPiece->SetColorCode(lcGetColorCodeFromOriginalColor(color));
			pPiece->CreateName(mPieces);
			mPieces.Add(pPiece);

			lcMatrix44 ModelWorld = lcMul(lcMatrix44RotationZ(rot[2] * LC_DTOR), lcMul(lcMatrix44RotationY(rot[1] * LC_DTOR), lcMatrix44RotationX(rot[0] * LC_DTOR)));
			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;

			pPiece->ChangeKey(1, false, AxisAngle, LC_PK_ROTATION);
//			pPiece->SetGroup((Group*)group);
			SystemPieceComboAdd(pInfo->m_strDescription);
		}

//		SytemStepProgressBar();
	}

	Library->CloseCache();
//	SytemEndProgressBar();

	if (!bMerge)
	{
		if (fv >= 0.4f)
		{
			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
			{
				file->ReadBuffer(mProperties.mAuthor.GetBuffer(sh), sh);
				mProperties.mAuthor.Buffer()[sh] = 0;
			}

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
			{
				file->ReadBuffer(mProperties.mDescription.GetBuffer(sh), sh);
				mProperties.mDescription.Buffer()[sh] = 0;
			}

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->ReadU16(&sh, 1); else sh = ch;
			if (sh > 255)
				file->Seek(sh, SEEK_CUR);
			else
			{
				file->ReadBuffer(mProperties.mComments.GetBuffer(sh), sh);
				mProperties.mComments.Buffer()[sh] = 0;
			}
		}
	}
	else
	{
		if (fv >= 0.4f)
		{
			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->ReadU16(&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);
		}
	}

	if (fv >= 0.5f)
	{
		int NumGroups = mGroups.GetSize();

		file->ReadS32(&count, 1);
		for (i = 0; i < count; i++)
			mGroups.Add(new lcGroup());

		for (int GroupIdx = NumGroups; GroupIdx < mGroups.GetSize(); GroupIdx++)
		{
			lcGroup* Group = mGroups[GroupIdx];

			if (fv < 1.0f)
			{
				file->ReadBuffer(Group->m_strName, 65);
				file->ReadBuffer(&ch, 1);
				Group->mGroup = (lcGroup*)-1;
			}
			else
				Group->FileLoad(file);

			if (bMerge)
			{
				// Ensure a unique group name

				int max = -1;
				String baseName;

				for (int Existing = 0; Existing < GroupIdx; Existing++)
					max = lcMax(max, InstanceOfName(mGroups[Existing]->m_strName, Group->m_strName, baseName));

				if (max > -1)
				{
					int baseReserve = sizeof(Group->m_strName) - 5; // space, #, 2-digits, and terminating 0
					for (int num = max; (num > 99); num /= 10) { baseReserve--; }
					sprintf(Group->m_strName, "%s #%.2d", (const char*)(baseName.Left(baseReserve)), max+1);
				}
			}
		}

		for (int GroupIdx = NumGroups; GroupIdx < mGroups.GetSize(); GroupIdx++)
		{
			lcGroup* Group = mGroups[GroupIdx];

			i = LC_POINTER_TO_INT(Group->mGroup);
			Group->mGroup = NULL;

			if (i > 0xFFFF || i == -1)
				continue;

			Group->mGroup = mGroups[NumGroups + i];
		}

		for (int PieceIdx = FirstNewPiece; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			Piece* Piece = mPieces[PieceIdx];

			i = LC_POINTER_TO_INT(Piece->GetGroup());
			Piece->SetGroup(NULL);

			if (i > 0xFFFF || i == -1)
				continue;

			Piece->SetGroup(mGroups[NumGroups + i]);
		}

		RemoveEmptyGroups();
	}

	if (!bMerge)
	{
		if (fv >= 0.6f)
		{
			if (fv < 1.0f)
			{
				lcuint32 ViewportMode;
				file->ReadU32(&ViewportMode, 1);
			}
			else
			{
				lcuint8 ViewportMode, ActiveViewport;
				file->ReadU8(&ViewportMode, 1);
				file->ReadU8(&ActiveViewport, 1);
			}

			file->ReadS32(&count, 1);
			for (i = 0; i < count; i++)
				mCameras.Add(new Camera(false));

			if (count < 7)
			{
				Camera* pCam = new Camera(false);
				for (i = 0; i < count; i++)
					pCam->FileLoad(*file);
				delete pCam;
			}
			else
			{
				for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
					mCameras[CameraIdx]->FileLoad(*file);
			}
		}

		if (fv >= 0.7f)
		{
			for (count = 0; count < 4; count++)
			{
				file->ReadS32(&i, 1);

//				Camera* pCam = m_pCameras;
//				while (i--)
//					pCam = pCam->m_pNext;
//				m_pViewCameras[count] = pCam;
			}

			file->ReadU32(&rgb, 1);
			mProperties.mFogColor[0] = (float)((unsigned char) (rgb))/255;
			mProperties.mFogColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			mProperties.mFogColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv < 1.0f)
			{
				file->ReadU32(&rgb, 1);
				mProperties.mFogDensity = (float)rgb/100;
			}
			else
				file->ReadFloats(&mProperties.mFogDensity, 1);

			if (fv < 1.3f)
			{
				file->ReadU8(&ch, 1);
				if (ch == 0xFF)
					file->ReadU16(&sh, 1);
				sh = ch;
			}
			else
				file->ReadU16(&sh, 1);

			if (sh < LC_MAXPATH)
			{
				char Background[LC_MAXPATH];
				file->ReadBuffer(Background, sh);
				mProperties.mBackgroundImage = Background;
			}
			else
				file->Seek (sh, SEEK_CUR);
		}

		if (fv >= 0.8f)
		{
			file->ReadBuffer(&ch, 1);
			file->ReadBuffer(m_strHeader, ch);
			file->ReadBuffer(&ch, 1);
			file->ReadBuffer(m_strFooter, ch);
		}

		if (fv > 0.9f)
		{
			file->ReadU32(&rgb, 1);
			mProperties.mAmbientColor[0] = (float)((unsigned char) (rgb))/255;
			mProperties.mAmbientColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			mProperties.mAmbientColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv < 1.3f)
			{
				file->ReadS32(&i, 1); // m_bAnimation = (i != 0);
				file->ReadS32(&i, 1); // mAddKeys = (i != 0);
				file->ReadU8(&ch, 1); // m_nFPS
				file->ReadS32(&i, 1); // m_nCurFrame = i;
				file->ReadU16(&sh, 1); // m_nTotalFrames
				file->ReadS32(&i, 1); // m_nGridSize = i;
				file->ReadS32(&i, 1); // m_nMoveSnap = i;
			}
			else
			{
				file->ReadU8(&ch, 1); // m_bAnimation = (ch != 0);
				file->ReadU8(&ch, 1); // mAddKeys = (ch != 0);
				file->ReadU8(&ch, 1); // m_nFPS
				file->ReadU16(&sh, 1); // m_nCurFrame
				file->ReadU16(&sh, 1); // m_nTotalFrames
				file->ReadU16(&sh, 1); // m_nGridSize = sh;
				file->ReadU16(&sh, 1);
				if (fv >= 1.4f)
					m_nMoveSnap = sh;
			}
		}

		if (fv > 1.0f)
		{
			file->ReadU32(&rgb, 1);
			mProperties.mBackgroundGradientColor1[0] = (float)((unsigned char) (rgb))/255;
			mProperties.mBackgroundGradientColor1[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			mProperties.mBackgroundGradientColor1[2] = (float)((unsigned char) ((rgb) >> 16))/255;
			file->ReadU32(&rgb, 1);
			mProperties.mBackgroundGradientColor2[0] = (float)((unsigned char) (rgb))/255;
			mProperties.mBackgroundGradientColor2[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			mProperties.mBackgroundGradientColor2[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv > 1.1f)
				m_pTerrain->FileLoad (file);
			else
			{
				file->Seek(4, SEEK_CUR);
				file->ReadBuffer(&ch, 1);
				file->Seek(ch, SEEK_CUR);
			}
		}
	}

	const lcArray<View*> Views = gMainWindow->GetViews();
	for (i = 0; i < Views.GetSize (); i++)
	{
		Views[i]->MakeCurrent();
		RenderInitialize();
	}

	CalculateStep();

	if (!bUndo)
		SelectAndFocusNone(false);

	if (!bMerge)
		gMainWindow->UpdateFocusObject(GetFocusObject());

	if (!bMerge)
	{
		for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
		{
			View* view = Views[ViewIdx];

			if (!view->mCamera->IsSimple())
				view->SetDefaultCamera();
		}

		if (!bUndo)
			ZoomExtents(0, Views.GetSize());
	}

	gMainWindow->UpdateLockSnap(m_nSnap);
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu();
	UpdateSelection();
	gMainWindow->UpdateTime(m_nCurStep, 255);
	gMainWindow->UpdateAllViews();

	return true;
}

void Project::FileSave(lcFile* file, bool bUndo)
{
	float ver_flt = 1.4f; // LeoCAD 0.75 - (and this should have been an integer).
	lcuint32 rgb;
	lcuint8 ch;
	lcuint16 sh;
	int i, j;

	const char LC_STR_VERSION[32] = "LeoCAD 0.7 Project\0\0";

	file->Seek(0, SEEK_SET);
	file->WriteBuffer(LC_STR_VERSION, 32);
	file->WriteFloats(&ver_flt, 1);

	rgb = LC_FLOATRGB(mProperties.mBackgroundSolidColor);
	file->WriteU32(&rgb, 1);

	i = m_nAngleSnap; file->WriteS32(&i, 1);
	file->WriteU32(&m_nSnap, 1);
	file->WriteFloat(1.0f);//m_fLineWidth
	file->WriteU32(0); // m_nDetail
	i = 0;//i = m_nCurGroup;
	file->WriteS32(&i, 1);
	i = 0;//i = m_nCurColor;
	file->WriteS32(&i, 1);
	i = 0; file->WriteS32(&i, 1); // m_nCurAction
	i = m_nCurStep; file->WriteS32(&i, 1);
	file->WriteU32(0);//m_nScene

	file->WriteS32(mPieces.GetSize());
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		mPieces[PieceIdx]->FileSave(*file);

	const char* Author = mProperties.mAuthor.Buffer();
	ch = lcMin(strlen(Author), 100U);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(Author, ch);
	const char* Description = mProperties.mDescription.Buffer();
	ch = lcMin(strlen(Description), 100U);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(Description, ch);
	const char* Comments = mProperties.mComments.Buffer();
	ch = lcMin(strlen(Comments), 255U);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(Comments, ch);

	i = mGroups.GetSize();
	file->WriteS32(&i, 1);

	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
		mGroups[GroupIdx]->FileSave(file, mGroups);

	lcuint8 ViewportMode = 0, ActiveViewport = 0;
	file->WriteU8(&ViewportMode, 1);
	file->WriteU8(&ActiveViewport, 1);

	i = mCameras.GetSize();
	file->WriteS32(&i, 1);

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->FileSave(*file);

	for (j = 0; j < 4; j++)
	{
		i = 0;
//		for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
//			if (pCamera == m_pViewCameras[j])
//				break;
//			else
//				i++;

		file->WriteS32(&i, 1);
	}

	rgb = LC_FLOATRGB(mProperties.mFogColor);
	file->WriteU32(&rgb, 1);
	file->WriteFloats(&mProperties.mFogDensity, 1);
	sh = strlen(mProperties.mBackgroundImage.Buffer());
	file->WriteU16(&sh, 1);
	file->WriteBuffer(mProperties.mBackgroundImage.Buffer(), sh);
	ch = strlen(m_strHeader);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strHeader, ch);
	ch = strlen(m_strFooter);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strFooter, ch);
	// 0.60 (1.0)
	rgb = LC_FLOATRGB(mProperties.mAmbientColor);
	file->WriteU32(&rgb, 1);
	ch = 0;// m_bAnimation;
	file->WriteBuffer(&ch, 1);
	ch = 0;// mAddKeys;
	file->WriteU8(&ch, 1);
	file->WriteU8 (24); // m_nFPS
	file->WriteU16(1); // m_nCurFrame
	file->WriteU16(100); // m_nTotalFrames
	file->WriteU16(0); // m_nGridSize
	file->WriteU16(&m_nMoveSnap, 1);
	// 0.62 (1.1)
	rgb = LC_FLOATRGB(mProperties.mBackgroundGradientColor1);
	file->WriteU32(&rgb, 1);
	rgb = LC_FLOATRGB(mProperties.mBackgroundGradientColor2);
	file->WriteU32(&rgb, 1);
	// 0.64 (1.2)
	m_pTerrain->FileSave(file);

	if (!bUndo)
	{
		lcuint32 pos = 0;
/*
		// TODO: add file preview
		i = lcGetProfileValue("Default", "Save Preview", 0);
		if (i != 0)
		{
			pos = file->GetPosition();

			Image* image = new Image[1];
			LC_IMAGE_OPTS opts;
			opts.interlaced = false;
			opts.transparent = false;
			opts.format = LC_IMAGE_GIF;

			i = m_bAnimation ? m_nCurFrame : m_nCurStep;
			CreateImages(image, 120, 100, i, i, false);
			image[0].FileSave (*file, &opts);
			delete []image;
		}
*/
		file->WriteU32(&pos, 1);
	}
}

void Project::FileReadMPD(lcFile& MPD, lcArray<LC_FILEENTRY*>& FileArray) const
{
	LC_FILEENTRY* CurFile = NULL;
	char Buf[1024];

	while (MPD.ReadLine(Buf, 1024))
	{
		String Line(Buf);

		Line.TrimLeft();

		if (Line[0] != '0')
		{
			// Copy current line.
			if (CurFile != NULL)
				CurFile->File.WriteBuffer(Buf, strlen(Buf));

			continue;
		}

		Line.TrimRight();
		Line = Line.Right(Line.GetLength() - 1);
		Line.TrimLeft();

		// Find where a subfile starts.
		if (Line.CompareNoCase("FILE", 4) == 0)
		{
			Line = Line.Right(Line.GetLength() - 4);
			Line.TrimLeft();

			// Create a new file.
			CurFile = new LC_FILEENTRY();
			strncpy(CurFile->FileName, Line, sizeof(CurFile->FileName));
			FileArray.Add(CurFile);
		}
		else if (Line.CompareNoCase("ENDFILE", 7) == 0)
		{
			// File ends here.
			CurFile = NULL;
		}
		else if (CurFile != NULL)
		{
			// Copy current line.
			CurFile->File.WriteBuffer(Buf, strlen(Buf));
		}
	}
}

void Project::FileReadLDraw(lcFile* file, const lcMatrix44& CurrentTransform, int* nOk, int DefColor, int* nStep, lcArray<LC_FILEENTRY*>& FileArray)
{
	char Line[1024];

	// Save file offset.
	lcuint32 Offset = file->GetPosition();
	file->Seek(0, SEEK_SET);

	while (file->ReadLine(Line, 1024))
	{
		bool read = true;
		char* Ptr = Line;
		char* Tokens[15];

		for (int TokenIdx = 0; TokenIdx < 15; TokenIdx++)
		{
			Tokens[TokenIdx] = 0;

			while (*Ptr && *Ptr <= 32)
			{
				*Ptr = 0;
				Ptr++;
			}

			Tokens[TokenIdx] = Ptr;

			while (*Ptr > 32)
				Ptr++;
		}

		if (!Tokens[0])
			continue;

		int LineType = atoi(Tokens[0]);

		if (LineType == 0)
		{
			if (Tokens[1])
			{
				strupr(Tokens[1]);

				if (!strcmp(Tokens[1], "STEP"))
					(*nStep)++;
			}

			continue;
		}

		if (LineType != 1)
			continue;

		bool Error = false;
		for (int TokenIdx = 1; TokenIdx < 15; TokenIdx++)
		{
			if (!Tokens[TokenIdx])
			{
				Error = true;
				break;
			}
		}

		if (Error)
			continue;

		int ColorCode = atoi(Tokens[1]);

		float Matrix[12];
		for (int TokenIdx = 2; TokenIdx < 14; TokenIdx++)
			Matrix[TokenIdx - 2] = atof(Tokens[TokenIdx]);

		lcMatrix44 IncludeTransform(lcVector4(Matrix[3], Matrix[6], Matrix[9], 0.0f), lcVector4(Matrix[4], Matrix[7], Matrix[10], 0.0f),
		                            lcVector4(Matrix[5], Matrix[8], Matrix[11], 0.0f), lcVector4(Matrix[0], Matrix[1], Matrix[2], 1.0f));

		IncludeTransform = lcMul(IncludeTransform, CurrentTransform);

		if (ColorCode == 16)
			ColorCode = DefColor;

		char* IncludeName = Tokens[14];
		for (Ptr = IncludeName; *Ptr; Ptr++)
			if (*Ptr == '\r' || *Ptr == '\n')
				*Ptr = 0;

		// See if it's a piece in the library
		if (strlen(IncludeName) < LC_PIECE_NAME_LEN)
		{
			char name[LC_PIECE_NAME_LEN];
			strcpy(name, IncludeName);
			strupr(name);

			Ptr = strrchr(name, '.');
			if (Ptr != NULL)
				*Ptr = 0;

			PieceInfo* pInfo = lcGetPiecesLibrary()->FindPiece(name, false);
			if (pInfo != NULL)
			{
				Piece* pPiece = new Piece(pInfo);
				read = false;

				float* Matrix = IncludeTransform;
				lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
				                     lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

				lcVector4 AxisAngle = lcMatrix44ToAxisAngle(Transform);
				AxisAngle[3] *= LC_RTOD;

				pPiece->Initialize(IncludeTransform[3].x / 25.0f, IncludeTransform[3].z / 25.0f, -IncludeTransform[3].y / 25.0f, *nStep);
				pPiece->SetColorCode(ColorCode);
				pPiece->CreateName(mPieces);
				mPieces.Add(pPiece);
				pPiece->ChangeKey(1, false, AxisAngle, LC_PK_ROTATION);
				SystemPieceComboAdd(pInfo->m_strDescription);
				(*nOk)++;
			}
		}

		// Check for MPD files first.
		if (read)
		{
			for (int i = 0; i < FileArray.GetSize(); i++)
			{
				if (stricmp(FileArray[i]->FileName, IncludeName) == 0)
				{
					FileReadLDraw(&FileArray[i]->File, IncludeTransform, nOk, ColorCode, nStep, FileArray);
					read = false;
					break;
				}
			}
		}

		// Try to read the file from disk.
		if (read)
		{
			lcDiskFile tf;

			if (tf.Open(IncludeName, "rt"))
			{
				FileReadLDraw(&tf, IncludeTransform, nOk, ColorCode, nStep, FileArray);
				read = false;
			}
		}

		if (read)
		{
			// Create a placeholder.
			char name[LC_PIECE_NAME_LEN];
			strcpy(name, IncludeName);
			strupr(name);

			Ptr = strrchr(name, '.');
			if (Ptr != NULL)
				*Ptr = 0;

			PieceInfo* Info = lcGetPiecesLibrary()->CreatePlaceholder(name);

			Piece* pPiece = new Piece(Info);
			read = false;

			float* Matrix = IncludeTransform;
			lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
			                     lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(Transform);
			AxisAngle[3] *= LC_RTOD;

			pPiece->Initialize(IncludeTransform[3].x / 25.0f, IncludeTransform[3].z / 25.0f, -IncludeTransform[3].y / 25.0f, *nStep);
			pPiece->SetColorCode(ColorCode);
			pPiece->CreateName(mPieces);
			mPieces.Add(pPiece);
			pPiece->ChangeKey(1, false, AxisAngle, LC_PK_ROTATION);
			SystemPieceComboAdd(Info->m_strDescription);
			(*nOk)++;
		}
	}

	// Restore file offset.
	file->Seek(Offset, SEEK_SET);
}

bool Project::DoSave(const char* FileName)
{
	char SaveFileName[LC_MAXPATH];

	if (FileName && FileName[0])
		strcpy(SaveFileName, FileName);
	else
	{
		if (m_strPathName[0])
			strcpy(SaveFileName, m_strPathName);
	else
	{
			strcpy(SaveFileName, lcGetProfileString(LC_PROFILE_PROJECTS_PATH));

			int Length = strlen(SaveFileName);
			if (Length && (SaveFileName[Length - 1] != '/' && SaveFileName[Length - 1] != '\\'))
				strcat(SaveFileName, "/");

			strcat(SaveFileName, m_strTitle);

			// check for dubious filename
			int iBad = strcspn(SaveFileName, " #%;/\\");
			if (iBad != -1)
				SaveFileName[iBad] = 0;

			strcat(SaveFileName, ".lcd");
		}

		if (!gMainWindow->DoDialog(LC_DIALOG_SAVE_PROJECT, SaveFileName))
			return false;
	}

	lcDiskFile file;
	if (!file.Open(SaveFileName, "wb"))
	{
//		MessageBox("Failed to save.");
		return false;
	}

	char ext[4];
	memset(ext, 0, 4);
	const char* ptr = strrchr(SaveFileName, '.');
	if (ptr != NULL)
	{
		strncpy(ext, ptr+1, 3);
		strlwr(ext);
	}

	if ((strcmp(ext, "dat") == 0) || (strcmp(ext, "ldr") == 0))
	{
		int i, steps = GetLastStep();
		char buf[256];

		ptr = strrchr(m_strPathName, '\\');
		if (ptr == NULL)
			ptr = strrchr(m_strPathName, '/');
		if (ptr == NULL)
			ptr = m_strPathName;
		else
			ptr++;

		sprintf(buf, "0 Model exported from LeoCAD\r\n"
					"0 Original name: %s\r\n", ptr);
		if (!mProperties.mAuthor.IsEmpty())
		{
			strcat(buf, "0 Author: ");
			strcat(buf, mProperties.mAuthor.Buffer());
			strcat(buf, "\r\n");
		}
		strcat(buf, "\r\n");
		file.WriteBuffer(buf, strlen(buf));

		const char* OldLocale = setlocale(LC_NUMERIC, "C");

		for (i = 1; i <= steps; i++)
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if ((Piece->IsVisible(i)) && (Piece->GetStepShow() == i))
				{
					const float* f = Piece->mModelWorld;
					sprintf (buf, "1 %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %s.DAT\r\n",
							 Piece->mColorCode, f[12] * 25.0f, -f[14] * 25.0f, f[13] *25.0f, f[0], -f[8], f[4], -f[2], f[10], -f[6], f[1], -f[9], f[5], Piece->mPieceInfo->m_strName);
					file.WriteBuffer(buf, strlen(buf));
				}
			}

			if (i != steps)
				file.WriteBuffer("0 STEP\r\n", 8);
		}
		file.WriteBuffer("0\r\n", 3);

		setlocale(LC_NUMERIC, OldLocale);
	}
	else
		FileSave(&file, false);     // save me

	file.Close();

	SetModifiedFlag(false);     // back to unmodified

	// reset the title and change the document name
	SetPathName(SaveFileName, true);

	return true; // success
}

// return true if ok to continue
bool Project::SaveModified()
{
	if (!IsModified())
		return true;        // ok to continue

	// get name/title of document
	char name[LC_MAXPATH];
	if (strlen(m_strPathName) == 0)
	{
		// get name based on caption
		strcpy(name, m_strTitle);
		if (strlen(name) == 0)
			strcpy(name, "Untitled");
	}
	else
	{
		// get name based on file title of path name
		char* p;

		p = strrchr(m_strPathName, '\\');
		if (!p)
			p = strrchr(m_strPathName, '/');

		if (p)
			strcpy(name, ++p);
		else
			strcpy(name, m_strPathName);
	}

	char prompt[512];
	sprintf(prompt, "Save changes to %s ?", name);

	switch (gMainWindow->DoMessageBox(prompt, LC_MB_YESNOCANCEL))
	{
	case LC_CANCEL:
		return false;       // don't continue

	case LC_YES:
		// If so, either Save or Update, as appropriate
		if (!DoSave(m_strPathName))
			return false;
		break;

	case LC_NO:
		// If not saving changes, revert the document
		break;
	}

	return true;    // keep going
}

void Project::SetModifiedFlag(bool Modified)
{
	if (m_bModified != Modified)
	{
		m_bModified = Modified;
		gMainWindow->UpdateModified(m_bModified);
	}
}

/////////////////////////////////////////////////////////////////////////////
// File operations

bool Project::OnNewDocument()
{
	SetTitle("Untitled");
	DeleteContents(false);
	memset(m_strPathName, 0, sizeof(m_strPathName)); // no path name yet
	SetModifiedFlag(false); // make clean
	LoadDefaults(true);
	CheckPoint("");

	return true;
}

bool Project::OpenProject(const char* FileName)
{
	if (!SaveModified())
		return false;  // Leave the original one

	bool WasModified = IsModified();
	SetModifiedFlag(false);  // Not dirty for open

	if (!OnOpenDocument(FileName))
	{
		// Check if we corrupted the original document
		if (!IsModified())
			SetModifiedFlag(WasModified);
		else
			OnNewDocument();

		return false;  // Open failed
	}

	SetPathName(FileName, true);

	return true;
}

bool Project::OnOpenDocument (const char* lpszPathName)
{
  lcDiskFile file;
  bool bSuccess = false;

  if (!file.Open(lpszPathName, "rb"))
  {
//    MessageBox("Failed to open file.");
    return false;
  }

  char ext[4];
  const char *ptr;
  memset(ext, 0, 4);
  ptr = strrchr(lpszPathName, '.');
  if (ptr != NULL)
  {
    strncpy(ext, ptr+1, 3);
    strlwr(ext);
  }

  bool datfile = false;
  bool mpdfile = false;

  // Find out what file type we're loading.
  if ((strcmp(ext, "dat") == 0) || (strcmp(ext, "ldr") == 0))
    datfile = true;
  else if (strcmp(ext, "mpd") == 0)
    mpdfile = true;

  // Delete the current project.
  DeleteContents(false);
  LoadDefaults(datfile || mpdfile);
  SetModifiedFlag(true);  // dirty during loading

  if (file.GetLength() != 0)
  {
	lcArray<LC_FILEENTRY*> FileArray;

    // Unpack the MPD file.
    if (mpdfile)
    {
      FileReadMPD(file, FileArray);

      if (FileArray.GetSize() == 0)
      {
        file.Seek(0, SEEK_SET);
        mpdfile = false;
        datfile = true;
      }
    }

    if (datfile || mpdfile)
    {
      int ok = 0, step = 1;
      lcMatrix44 mat = lcMatrix44Identity();

      if (mpdfile)
        FileReadLDraw(&FileArray[0]->File, mat, &ok, 16, &step, FileArray);
      else
        FileReadLDraw(&file, mat, &ok, 16, &step, FileArray);

      m_nCurStep = step;
	  gMainWindow->UpdateTime(m_nCurStep, 255);
	  gMainWindow->UpdateFocusObject(GetFocusObject());
      UpdateSelection();
      CalculateStep();

	  ZoomExtents(0, gMainWindow->GetViews().GetSize());
      gMainWindow->UpdateAllViews();

      bSuccess = true;
    }
    else
    {
      // Load a LeoCAD file.
      bSuccess = FileLoad(&file, false, false);
    }

    // Clean up.
    if (mpdfile)
    {
      for (int i = 0; i < FileArray.GetSize(); i++)
        delete FileArray[i];
    }
  }

  file.Close();

  if (bSuccess == false)
  {
//    MessageBox("Failed to load.");
    DeleteContents(false);   // remove failed contents
    return false;
  }

  CheckPoint("");

  SetModifiedFlag(false);     // start off with unmodified

  return true;
}

void Project::SetPathName(const char* PathName, bool bAddToMRU)
{
	strcpy(m_strPathName, PathName);

	// always capture the complete file name including extension (if present)
	const char* lpszTemp = PathName;
	for (const char* lpsz = PathName; *lpsz != '\0'; lpsz++)
	{
		// remember last directory/drive separator
		if (*lpsz == '\\' || *lpsz == '/' || *lpsz == ':')
			lpszTemp = lpsz + 1;
	}

	// set the document title based on path name
	SetTitle(lpszTemp);

	// add it to the file MRU list
	if (bAddToMRU)
		gMainWindow->AddRecentFile(m_strPathName);
}

/////////////////////////////////////////////////////////////////////////////
// Undo/Redo support

// Save current state.
void Project::CheckPoint(const char* Description)
{
	lcModelHistoryEntry* ModelHistoryEntry = new lcModelHistoryEntry();

	strcpy(ModelHistoryEntry->Description, Description);
	FileSave(&ModelHistoryEntry->File, true);

	mUndoHistory.InsertAt(0, ModelHistoryEntry);
	mRedoHistory.DeleteAll();

	gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : NULL, !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : NULL);
}

/////////////////////////////////////////////////////////////////////////////
// Project rendering

// Only this function should be called.
void Project::Render(View* View, bool ToMemory)
{
	View->mContext->SetDefaultState();
	glViewport(0, 0, View->mWidth, View->mHeight);

	RenderBackground(View);

	RenderScenePieces(View, !ToMemory);

	if (!ToMemory)
	{
		RenderSceneObjects(View);

		RenderViewports(View);
	}
}

void Project::RenderBackground(View* View)
{
	lcContext* Context = View->mContext;

	if (mProperties.mBackgroundType == LC_BACKGROUND_SOLID)
	{
		glClearColor(mProperties.mBackgroundSolidColor[0], mProperties.mBackgroundSolidColor[1], mProperties.mBackgroundSolidColor[2], 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	float ViewWidth = (float)View->mWidth;
	float ViewHeight = (float)View->mHeight;

	Context->SetProjectionMatrix(lcMatrix44Ortho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f));
	Context->SetWorldViewMatrix(lcMatrix44Translation(lcVector3(0.375f, 0.375f, 0.0f)));

	if (mProperties.mBackgroundType == LC_BACKGROUND_GRADIENT)
	{
		glShadeModel(GL_SMOOTH);

		const lcVector3& Color1 = mProperties.mBackgroundGradientColor1;
		const lcVector3& Color2 = mProperties.mBackgroundGradientColor2;

		float Verts[] =
		{
			ViewWidth, ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
			0.0f,      ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
			0.0f,      0.0f,       Color2[0], Color2[1], Color2[2], 1.0f,
			ViewWidth, 0.0f,       Color2[0], Color2[1], Color2[2], 1.0f
		};

		glVertexPointer(2, GL_FLOAT, 6 * sizeof(float), Verts);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 6 * sizeof(float), Verts + 2);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_COLOR_ARRAY);

		glShadeModel(GL_FLAT);
	}

	if (mProperties.mBackgroundType == LC_BACKGROUND_IMAGE)
	{
		glEnable(GL_TEXTURE_2D);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, m_pBackground->mTexture);

		float TileWidth = 1.0f, TileHeight = 1.0f;

		if (mProperties.mBackgroundImageTile)
		{
			TileWidth = ViewWidth / m_pBackground->mWidth;
			TileHeight = ViewHeight / m_pBackground->mHeight;
		}

		float Verts[] =
		{
			0.0f,      ViewHeight, 0.0f,      0.0f,
			ViewWidth, ViewHeight, TileWidth, 0.0f,
			ViewWidth, 0.0f,       TileWidth, TileHeight,
			0.0f,      0.0f,       0.0f,      TileHeight
		};

		glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), Verts);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), Verts + 2);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDisable(GL_TEXTURE_2D);
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}

void Project::RenderScenePieces(View* view, bool DrawInterface)
{
	const lcPreferences& Preferences = lcGetPreferences();
	lcContext* Context = view->mContext;

	Context->SetProjectionMatrix(view->GetProjectionMatrix());

	if (Preferences.mLightingMode != LC_LIGHTING_FLAT)
	{
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
		glEnable(GL_COLOR_MATERIAL);
		glShadeModel(GL_SMOOTH);

		GLfloat mat_translucent[] = { (GLfloat)0.8, (GLfloat)0.8, (GLfloat)0.8, (GLfloat)1.0 };
		GLfloat mat_opaque[] = { (GLfloat)0.8, (GLfloat)0.8, (GLfloat)0.8, (GLfloat)1.0 };
		GLfloat medium_shininess[] = { (GLfloat)64.0 };

		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, medium_shininess);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_opaque);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_translucent);

		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lcVector4(mProperties.mAmbientColor, 1.0f));

		for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
			mLights[LightIdx]->Setup(LightIdx);

		glEnable(GL_LIGHTING);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (mProperties.mFogEnabled)
	{
		glFogi(GL_FOG_MODE, GL_EXP);
		glFogf(GL_FOG_DENSITY, mProperties.mFogDensity);
		glFogfv(GL_FOG_COLOR, lcVector4(mProperties.mFogColor, 1.0f));
		glEnable(GL_FOG);
	}

//	if (m_nScene & LC_SCENE_FLOOR)
//		m_pTerrain->Render(view->mCamera, AspectRatio);

	lcArray<lcRenderMesh> OpaqueMeshes(mPieces.GetSize());
	lcArray<lcRenderMesh> TranslucentMeshes;
	const lcMatrix44& ViewMatrix = view->mCamera->mWorldView;

	Context->SetLineWidth(Preferences.mLineWidth);

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (!Piece->IsVisible(m_nCurStep))
			continue;

		PieceInfo* Info = Piece->mPieceInfo;
		bool Focused, Selected;

		if (DrawInterface)
		{
			Focused = Piece->IsFocused();
			Selected = Piece->IsSelected();
		}
		else
		{
			Focused = false;
			Selected = false;
		}

		Info->AddRenderMeshes(ViewMatrix, &Piece->mModelWorld, Piece->mColorIndex, Focused, Selected, OpaqueMeshes, TranslucentMeshes);
	}

	OpaqueMeshes.Sort(lcOpaqueRenderMeshCompare);
	Context->DrawOpaqueMeshes(ViewMatrix, OpaqueMeshes);

	TranslucentMeshes.Sort(lcTranslucentRenderMeshCompare);
	Context->DrawTranslucentMeshes(ViewMatrix, TranslucentMeshes);

	Context->UnbindMesh(); // context remove

	if (Preferences.mLightingMode != LC_LIGHTING_FLAT)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (mProperties.mFogEnabled)
		glDisable(GL_FOG);

	if (DrawInterface)
	{
		Context->SetLineWidth(2.0f * Preferences.mLineWidth);

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			Piece* Piece = mPieces[PieceIdx];

			if (!Piece->IsVisible(m_nCurStep) || !Piece->IsSelected())
				continue;

			PieceInfo* PieceInfo = Piece->mPieceInfo;

			lcVector3 Min(PieceInfo->m_fDimensions[3], PieceInfo->m_fDimensions[4], PieceInfo->m_fDimensions[5]);
			lcVector3 Max(PieceInfo->m_fDimensions[0], PieceInfo->m_fDimensions[1], PieceInfo->m_fDimensions[2]);
			lcVector3 Edge((Max - Min) * 0.33f);

			float Verts[48][3] =
			{
				{ Max[0], Max[1], Max[2] }, { Max[0] - Edge[0], Max[1], Max[2] },
				{ Max[0], Max[1], Max[2] }, { Max[0], Max[1] - Edge[1], Max[2] },
				{ Max[0], Max[1], Max[2] }, { Max[0], Max[1], Max[2] - Edge[2] },

				{ Min[0], Max[1], Max[2] }, { Min[0] + Edge[0], Max[1], Max[2] },
				{ Min[0], Max[1], Max[2] }, { Min[0], Max[1] - Edge[1], Max[2] },
				{ Min[0], Max[1], Max[2] }, { Min[0], Max[1], Max[2] - Edge[2] },

				{ Max[0], Min[1], Max[2] }, { Max[0] - Edge[0], Min[1], Max[2] },
				{ Max[0], Min[1], Max[2] }, { Max[0], Min[1] + Edge[1], Max[2] },
				{ Max[0], Min[1], Max[2] }, { Max[0], Min[1], Max[2] - Edge[2] },

				{ Min[0], Min[1], Max[2] }, { Min[0] + Edge[0], Min[1], Max[2] },
				{ Min[0], Min[1], Max[2] }, { Min[0], Min[1] + Edge[1], Max[2] },
				{ Min[0], Min[1], Max[2] }, { Min[0], Min[1], Max[2] - Edge[2] },

				{ Max[0], Max[1], Min[2] }, { Max[0] - Edge[0], Max[1], Min[2] },
				{ Max[0], Max[1], Min[2] }, { Max[0], Max[1] - Edge[1], Min[2] },
				{ Max[0], Max[1], Min[2] }, { Max[0], Max[1], Min[2] + Edge[2] },

				{ Min[0], Max[1], Min[2] }, { Min[0] + Edge[0], Max[1], Min[2] },
				{ Min[0], Max[1], Min[2] }, { Min[0], Max[1] - Edge[1], Min[2] },
				{ Min[0], Max[1], Min[2] }, { Min[0], Max[1], Min[2] + Edge[2] },

				{ Max[0], Min[1], Min[2] }, { Max[0] - Edge[0], Min[1], Min[2] },
				{ Max[0], Min[1], Min[2] }, { Max[0], Min[1] + Edge[1], Min[2] },
				{ Max[0], Min[1], Min[2] }, { Max[0], Min[1], Min[2] + Edge[2] },

				{ Min[0], Min[1], Min[2] }, { Min[0] + Edge[0], Min[1], Min[2] },
				{ Min[0], Min[1], Min[2] }, { Min[0], Min[1] + Edge[1], Min[2] },
				{ Min[0], Min[1], Min[2] }, { Min[0], Min[1], Min[2] + Edge[2] },
			};

			Context->SetWorldViewMatrix(lcMul(Piece->mModelWorld, ViewMatrix));

			if (Piece->IsFocused())
				lcSetColorFocused();
			else
				lcSetColorSelected();

			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINES, 0, 48);
		}

		Context->SetLineWidth(Preferences.mLineWidth); // context remove
	}
}

void Project::RenderSceneObjects(View* view)
{
	const lcPreferences& Preferences = lcGetPreferences();
	const lcMatrix44& ViewMatrix = view->mCamera->mWorldView;
	lcContext* Context = view->mContext;

	Context->SetProjectionMatrix(view->GetProjectionMatrix());

#ifdef LC_DEBUG
	RenderDebugPrimitives();
#endif

	if (view->mTrackTool == LC_TRACKTOOL_INSERT)
	{
		lcVector3 Position;
		lcVector4 Rotation;
		GetPieceInsertPosition(view, Position, Rotation);

		lcMatrix44 WorldMatrix = lcMatrix44FromAxisAngle(lcVector3(Rotation[0], Rotation[1], Rotation[2]), Rotation[3] * LC_DTOR);
		WorldMatrix.SetTranslation(Position);

		Context->SetWorldViewMatrix(lcMul(WorldMatrix, ViewMatrix));

		Context->SetLineWidth(2.0f * Preferences.mLineWidth);
		m_pCurPiece->RenderPiece(gMainWindow->mColorIndex);
	}

	if (Preferences.mLightingMode != LC_LIGHTING_FLAT)
		glDisable(GL_LIGHTING);

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if ((pCamera == view->mCamera) || !pCamera->IsVisible())
			continue;

		pCamera->Render(view);
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		if (mLights[LightIdx]->IsVisible())
			mLights[LightIdx]->Render(view);

	Context->SetLineWidth(Preferences.mLineWidth); // context remove

	if (Preferences.mDrawGridStuds || Preferences.mDrawGridLines)
	{
		Context->SetWorldViewMatrix(ViewMatrix);

		const int Spacing = lcMax(Preferences.mGridLineSpacing, 1);
		int MinX = 0, MaxX = 0, MinY = 0, MaxY = 0;

		if (!mPieces.IsEmpty() || view->mTrackTool == LC_TRACKTOOL_INSERT)
		{
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsVisible(m_nCurStep))
					Piece->CompareBoundingBox(bs);
			}

			if (view->mTrackTool == LC_TRACKTOOL_INSERT)
			{
				lcVector3 Position;
				lcVector4 Rotation;
				GetPieceInsertPosition(view, Position, Rotation);

				lcVector3 Points[8] =
				{
					lcVector3(m_pCurPiece->m_fDimensions[0], m_pCurPiece->m_fDimensions[1], m_pCurPiece->m_fDimensions[5]),
					lcVector3(m_pCurPiece->m_fDimensions[3], m_pCurPiece->m_fDimensions[1], m_pCurPiece->m_fDimensions[5]),
					lcVector3(m_pCurPiece->m_fDimensions[0], m_pCurPiece->m_fDimensions[1], m_pCurPiece->m_fDimensions[2]),
					lcVector3(m_pCurPiece->m_fDimensions[3], m_pCurPiece->m_fDimensions[4], m_pCurPiece->m_fDimensions[5]),
					lcVector3(m_pCurPiece->m_fDimensions[3], m_pCurPiece->m_fDimensions[4], m_pCurPiece->m_fDimensions[2]),
					lcVector3(m_pCurPiece->m_fDimensions[0], m_pCurPiece->m_fDimensions[4], m_pCurPiece->m_fDimensions[2]),
					lcVector3(m_pCurPiece->m_fDimensions[0], m_pCurPiece->m_fDimensions[4], m_pCurPiece->m_fDimensions[5]),
					lcVector3(m_pCurPiece->m_fDimensions[3], m_pCurPiece->m_fDimensions[1], m_pCurPiece->m_fDimensions[2])
				};

				lcMatrix44 ModelWorld = lcMatrix44FromAxisAngle(lcVector3(Rotation[0], Rotation[1], Rotation[2]), Rotation[3] * LC_DTOR);
				ModelWorld.SetTranslation(Position);

				for (int i = 0; i < 8; i++)
				{
					lcVector3 Point = lcMul31(Points[i], ModelWorld);

					if (Point[0] < bs[0]) bs[0] = Point[0];
					if (Point[1] < bs[1]) bs[1] = Point[1];
					if (Point[2] < bs[2]) bs[2] = Point[2];
					if (Point[0] > bs[3]) bs[3] = Point[0];
					if (Point[1] > bs[4]) bs[4] = Point[1];
					if (Point[2] > bs[5]) bs[5] = Point[2];
				}
			}

			MinX = (int)(floorf(bs[0] / (0.8f * Spacing))) - 1;
			MinY = (int)(floorf(bs[1] / (0.8f * Spacing))) - 1;
			MaxX = (int)(ceilf(bs[3] / (0.8f * Spacing))) + 1;
			MaxY = (int)(ceilf(bs[4] / (0.8f * Spacing))) + 1;
		}

		MinX = lcMin(MinX, -2);
		MinY = lcMin(MinY, -2);
		MaxX = lcMax(MaxX, 2);
		MaxY = lcMax(MaxY, 2);

		if (Preferences.mDrawGridStuds)
		{
			float Left = MinX * 0.8f * Spacing;
			float Right = MaxX * 0.8f * Spacing;
			float Top = MinY * 0.8f * Spacing;
			float Bottom = MaxY * 0.8f * Spacing;
			float Z = 0;
			float U = (MaxX - MinX) * Spacing;
			float V = (MaxY - MinY) * Spacing;

			float Verts[4 * 5];
			float* CurVert = Verts;

			*CurVert++ = Left;
			*CurVert++ = Top;
			*CurVert++ = Z;
			*CurVert++ = 0.0f;
			*CurVert++ = V;

			*CurVert++ = Left;
			*CurVert++ = Bottom;
			*CurVert++ = Z;
			*CurVert++ = 0.0f;
			*CurVert++ = 0.0f;

			*CurVert++ = Right;
			*CurVert++ = Bottom;
			*CurVert++ = Z;
			*CurVert++ = U;
			*CurVert++ = 0.0f;

			*CurVert++ = Right;
			*CurVert++ = Top;
			*CurVert++ = Z;
			*CurVert++ = U;
			*CurVert++ = V;

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glBindTexture(GL_TEXTURE_2D, mGridTexture->mTexture);
			glEnable(GL_TEXTURE_2D);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);

			glColor4fv(lcVector4FromColor(Preferences.mGridStudColor));

			glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), Verts);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), Verts + 3);

			glDrawArrays(GL_QUADS, 0, 4);

			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
		}

		if (Preferences.mDrawGridLines)
		{
			glColor4fv(lcVector4FromColor(Preferences.mGridLineColor));

			int NumVerts = 2 * (MaxX - MinX + MaxY - MinY + 2);
			float* Verts = (float*)malloc(NumVerts * sizeof(float[3]));
			float* Vert = Verts;
			float LineSpacing = Spacing * 0.8f;

			for (int Step = MinX; Step < MaxX + 1; Step++)
			{
				*Vert++ = Step * LineSpacing;
				*Vert++ = MinY * LineSpacing;
				*Vert++ = 0.0f;
				*Vert++ = Step * LineSpacing;
				*Vert++ = MaxY * LineSpacing;
				*Vert++ = 0.0f;
			}

			for (int Step = MinY; Step < MaxY + 1; Step++)
			{
				*Vert++ = MinX * LineSpacing;
				*Vert++ = Step * LineSpacing;
				*Vert++ = 0.0f;
				*Vert++ = MaxX * LineSpacing;
				*Vert++ = Step * LineSpacing;
				*Vert++ = 0.0f;
			}

			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINES, 0, NumVerts);
			free(Verts);
		}
	}

	if (Preferences.mDrawAxes)
	{
//		glClear(GL_DEPTH_BUFFER_BIT);

		lcMatrix44 Mats[3];
		Mats[0] = view->mCamera->mWorldView;
		Mats[0].SetTranslation(lcVector3(0, 0, 0));
		Mats[1] = lcMul(lcMatrix44(lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 1, 0), lcVector4(0, 0, 0, 1)), Mats[0]);
		Mats[2] = lcMul(lcMatrix44(lcVector4(0, 0, 1, 0), lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 0, 1)), Mats[0]);

		lcVector3 pts[3] =
		{
			lcMul30(lcVector3(20, 0, 0), Mats[0]),
			lcMul30(lcVector3(0, 20, 0), Mats[0]),
			lcMul30(lcVector3(0, 0, 20), Mats[0]),
		};

		Context->SetProjectionMatrix(lcMatrix44Ortho(0, view->mWidth, 0, view->mHeight, -50, 50));
		Context->SetWorldViewMatrix(lcMatrix44Translation(lcVector3(25.375f, 25.375f, 0.0f)));

		// Draw the arrows.
		lcVector3 Verts[11];
		Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);

		glVertexPointer(3, GL_FLOAT, 0, Verts);

		for (int i = 0; i < 3; i++)
		{
			switch (i)
			{
			case 0:
				glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
				break;
			case 1:
				glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
				break;
			case 2:
				glColor4f(0.0f, 0.0f, 0.8f, 1.0f);
				break;
			}

			Verts[1] = pts[i];

			for (int j = 0; j < 9; j++)
				Verts[j+2] = lcMul30(lcVector3(12.0f, cosf(LC_2PI * j / 8) * 3.0f, sinf(LC_2PI * j / 8) * 3.0f), Mats[i]);

			glDrawArrays(GL_LINES, 0, 2);
			glDrawArrays(GL_TRIANGLE_FAN, 1, 10);
		}

		// Draw the text.
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_pScreenFont->MakeCurrent();
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		m_pScreenFont->PrintText(pts[0][0], pts[0][1], 40.0f, "X");
		m_pScreenFont->PrintText(pts[1][0], pts[1][1], 40.0f, "Y");
		m_pScreenFont->PrintText(pts[2][0], pts[2][1], 40.0f, "Z");

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
	}
}

void Project::RenderViewports(View* view)
{
	lcContext* Context = view->mContext;

	Context->SetProjectionMatrix(lcMatrix44Ortho(0.0f, view->mWidth, 0.0f, view->mHeight, -1.0f, 1.0f));
	Context->SetWorldViewMatrix(lcMatrix44Translation(lcVector3(0.375f, 0.375f, 0.0f)));

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	if (gMainWindow->GetActiveView() == view)
	{
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
		float Verts[8] = { 0.0f, 0.0f, view->mWidth - 1, 0.0f, view->mWidth - 1, view->mHeight - 1, 0.0f, view->mHeight - 1 };

		glVertexPointer(2, GL_FLOAT, 0, Verts);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}

	const char* CameraName = view->mCamera->GetName();

	if (CameraName[0])
	{
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_pScreenFont->MakeCurrent();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		m_pScreenFont->PrintText(3.0f, (float)view->mHeight - 1.0f - 6.0f, 0.0f, CameraName);

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
	}

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}

// Initialize OpenGL
void Project::RenderInitialize()
{
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	// Load font
	if (!m_pScreenFont->IsLoaded())
		m_pScreenFont->Initialize();

//	if (m_nScene & LC_SCENE_FLOOR)
//		m_pTerrain->LoadTexture();

	if (mProperties.mBackgroundType == LC_BACKGROUND_IMAGE)
		if (!m_pBackground->Load(mProperties.mBackgroundImage.Buffer(), LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV))
		{
			mProperties.mBackgroundType = LC_BACKGROUND_SOLID;
//			AfxMessageBox ("Could not load background");
		}

	if (!mGridTexture->mTexture)
	{
		const int NumLevels = 9;
		Image GridImages[NumLevels];

		for (int ImageLevel = 0; ImageLevel < NumLevels; ImageLevel++)
		{
			Image& GridImage = GridImages[ImageLevel];

			const int GridSize = 256 >> ImageLevel;
			const float Radius1 = (80 >> ImageLevel) * (80 >> ImageLevel);
			const float Radius2 = (72 >> ImageLevel) * (72 >> ImageLevel);

			GridImage.Allocate(GridSize, GridSize, LC_PIXEL_FORMAT_A8);
			lcuint8* BlurBuffer = new lcuint8[GridSize * GridSize];

			for (int y = 0; y < GridSize; y++)
			{
				lcuint8* Pixel = GridImage.mData + y * GridSize;
				memset(Pixel, 0, GridSize);

				const float y2 = (y - GridSize / 2) * (y - GridSize / 2);

				if (Radius1 <= y2)
					continue;

				if (Radius2 <= y2)
				{
					int x1 = sqrtf(Radius1 - y2);

					for (int x = GridSize / 2 - x1; x < GridSize / 2 + x1; x++)
						Pixel[x] = 255;
				}
				else
				{
					int x1 = sqrtf(Radius1 - y2);
					int x2 = sqrtf(Radius2 - y2);

					for (int x = GridSize / 2 - x1; x < GridSize / 2 - x2; x++)
						Pixel[x] = 255;

					for (int x = GridSize / 2 + x2; x < GridSize / 2 + x1; x++)
						Pixel[x] = 255;
				}
			}

			for (int y = 0; y < GridSize - 1; y++)
			{
				for (int x = 0; x < GridSize - 1; x++)
				{
					lcuint8 a = GridImage.mData[x + y * GridSize];
					lcuint8 b = GridImage.mData[x + 1 + y * GridSize];
					lcuint8 c = GridImage.mData[x + (y + 1) * GridSize];
					lcuint8 d = GridImage.mData[x + 1 + (y + 1) * GridSize];
					BlurBuffer[x + y * GridSize] = (a + b + c + d) / 4;
				}

				int x = GridSize - 1;
				lcuint8 a = GridImage.mData[x + y * GridSize];
				lcuint8 c = GridImage.mData[x + (y + 1) * GridSize];
				BlurBuffer[x + y * GridSize] = (a + c) / 2;
			}

			int y = GridSize - 1;
			for (int x = 0; x < GridSize - 1; x++)
			{
				lcuint8 a = GridImage.mData[x + y * GridSize];
				lcuint8 b = GridImage.mData[x + 1 + y * GridSize];
				BlurBuffer[x + y * GridSize] = (a + b) / 2;
			}

			int x = GridSize - 1;
			BlurBuffer[x + y * GridSize] = GridImage.mData[x + y * GridSize];

			memcpy(GridImage.mData, BlurBuffer, GridSize * GridSize);
			delete[] BlurBuffer;
		}

		mGridTexture->Load(GridImages, NumLevels, LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV | LC_TEXTURE_MIPMAPS | LC_TEXTURE_ANISOTROPIC);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Project functions

void Project::CalculateStep()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];
		Piece->UpdatePosition(m_nCurStep);

		if (Piece->IsSelected())
		{
			if (!Piece->IsVisible(m_nCurStep))
				Piece->SetSelected(false);
			else
				SelectGroup(Piece->GetTopGroup(), true);
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->UpdatePosition(m_nCurStep);

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		mLights[LightIdx]->UpdatePosition(m_nCurStep);
}

bool Project::RemoveSelectedObjects()
{
	bool RemovedPiece = false;
	bool RemovedCamera = false;
	bool RemovedLight = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); )
	{
		Piece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			RemovedPiece = true;
			mPieces.Remove(Piece);
			delete Piece;
		}
		else
			PieceIdx++;
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); )
	{
		Camera* Camera = mCameras[CameraIdx];

		if (Camera->IsSelected())
		{
			const lcArray<View*> Views = gMainWindow->GetViews();
			for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
			{
				View* View = Views[ViewIdx];

				if (Camera == View->mCamera)
					View->SetCamera(Camera, true);
			}

			RemovedCamera = true;
			mCameras.RemoveIndex(CameraIdx);
			delete Camera;
		}
		else
			CameraIdx++;
	}

	if (RemovedCamera)
		gMainWindow->UpdateCameraMenu();

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); )
	{
		Light* Light = mLights[LightIdx];

		if (Light->IsSelected())
		{
			RemovedLight = true;
			mLights.RemoveIndex(LightIdx);
			delete Light;
		}
		else
			LightIdx++;
	}

	RemoveEmptyGroups();

	return RemovedPiece || RemovedCamera || RemovedLight;
}

void Project::UpdateSelection()
{
	unsigned long flags = 0;
	int SelectedCount = 0;
	Object* Focus = NULL;

	if (mPieces.IsEmpty())
		flags |= LC_SEL_NO_PIECES;
	else
	{
		Group* pGroup = NULL;
		bool first = true;

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			Piece* Piece = mPieces[PieceIdx];

			if (Piece->IsSelected())
			{
				SelectedCount++;

				if (Piece->IsFocused())
					Focus = Piece;

				if (flags & LC_SEL_PIECE)
					flags |= LC_SEL_MULTIPLE;
				else
					flags |= LC_SEL_PIECE;

				if (Piece->GetGroup() != NULL)
				{
					flags |= LC_SEL_GROUP;
					if (Piece->IsFocused())
						flags |= LC_SEL_FOCUSGROUP;
				}

				if (first)
				{
					pGroup = Piece->GetGroup();
					first = false;
				}
				else
				{
					if (pGroup != Piece->GetGroup())
						flags |= LC_SEL_CANGROUP;
					else
						if (pGroup == NULL)
							flags |= LC_SEL_CANGROUP;
				}
			}
			else
			{
				flags |= LC_SEL_UNSELECTED;

				if (Piece->IsHidden())
					flags |= LC_SEL_HIDDEN;
			}
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if (pCamera->IsSelected())
		{
			flags |= LC_SEL_CAMERA;
			SelectedCount++;

			if (pCamera->IsFocused())
				Focus = pCamera;
		}
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		Light* pLight = mLights[LightIdx];

		if (pLight->IsSelected())
		{
			flags |= LC_SEL_LIGHT;
			SelectedCount++;

			if (pLight->IsFocused())
				Focus = pLight;
		}
	}

	gMainWindow->UpdateSelectedObjects(flags, SelectedCount, Focus);
}

void Project::CheckAutoSave()
{
	/*
	m_nSaveTimer += 5;
	if (m_nAutosave & LC_AUTOSAVE_FLAG)
	{
//		int nInterval;
//		nInterval = m_nAutosave & ~LC_AUTOSAVE_FLAG;

		if (m_nSaveTimer >= (m_nAutosave*60))
		{
			m_nSaveTimer = 0;

			if (m_strTempFile.IsEmpty())
			{
				char tmpFile[_MAX_PATH], out[_MAX_PATH];
				GetTempPath (_MAX_PATH, out);
				GetTempFileName (out, "~LC", 0, tmpFile);
				DeleteFile (tmpFile);
				if (char *ptr = strchr(tmpFile, '.')) *ptr = 0;
				strcat (tmpFile, ".lcd");
				m_strTempFile = tmpFile;
			}

			CFile file (m_strTempFile, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary);
			m_bUndo = TRUE;
			file.SeekToBegin();
			CArchive ar(&file, CArchive::store);
			Serialize(ar);
			ar.Close();
			m_bUndo = FALSE;
		}
	}
	*/
}

unsigned char Project::GetLastStep()
{
	unsigned char last = 1;
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		last = lcMax(last, mPieces[PieceIdx]->GetStepShow());

	return last;
}

void Project::FindPiece(bool FindFirst, bool SearchForward)
{
	if (mPieces.IsEmpty())
		return;

	int StartIdx = mPieces.GetSize() - 1;
	if (!FindFirst)
	{
		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			Piece* Piece = mPieces[PieceIdx];

			if (Piece->IsFocused() && Piece->IsVisible(m_nCurStep))
			{
				StartIdx = PieceIdx;
				break;
			}
		}
	}

	int CurrentIdx = StartIdx;
	Object* Focus = NULL;

	for (;;)
	{
		if (SearchForward)
			CurrentIdx++;
		else
			CurrentIdx--;

		if (CurrentIdx < 0)
			CurrentIdx = mPieces.GetSize() - 1;
		else if (CurrentIdx >= mPieces.GetSize())
			CurrentIdx = 0;

		if (CurrentIdx == StartIdx)
			break;

		Piece* Current = mPieces[CurrentIdx];

		if (!Current->IsVisible(m_nCurStep))
			continue;

		if ((!mSearchOptions.MatchInfo || Current->mPieceInfo == mSearchOptions.Info) &&
			(!mSearchOptions.MatchColor || Current->mColorIndex == mSearchOptions.ColorIndex) &&
			(!mSearchOptions.MatchName || strcasestr(Current->GetName(), mSearchOptions.Name)))
		{
			Focus = Current;
			break;
		}
	}

	ClearSelectionAndSetFocus(Focus, LC_PIECE_SECTION_POSITION);
}

void Project::ZoomExtents(int FirstView, int LastView)
{
	if (mPieces.IsEmpty())
		return;

	float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(m_nCurStep))
			Piece->CompareBoundingBox(bs);
	}

	lcVector3 Center((bs[0] + bs[3]) / 2, (bs[1] + bs[4]) / 2, (bs[2] + bs[5]) / 2);

	lcVector3 Points[8] =
	{
		lcVector3(bs[0], bs[1], bs[5]),
		lcVector3(bs[3], bs[1], bs[5]),
		lcVector3(bs[0], bs[1], bs[2]),
		lcVector3(bs[3], bs[4], bs[5]),
		lcVector3(bs[3], bs[4], bs[2]),
		lcVector3(bs[0], bs[4], bs[2]),
		lcVector3(bs[0], bs[4], bs[5]),
		lcVector3(bs[3], bs[1], bs[2])
	};

	const lcArray<View*> Views = gMainWindow->GetViews();
	for (int vp = FirstView; vp < LastView; vp++)
	{
		View* view = Views[vp];

		view->mCamera->ZoomExtents(view, Center, Points, 8, m_nCurStep, gMainWindow->GetAddKeys());
	}

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
}

void Project::GetPiecesUsed(lcArray<lcPiecesUsedEntry>& PiecesUsed) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (Piece->mPieceInfo->m_strDescription[0] == '~')
			continue;

		int UsedIdx;

		for (UsedIdx = 0; UsedIdx < PiecesUsed.GetSize(); UsedIdx++)
		{
			if (PiecesUsed[UsedIdx].Info != Piece->mPieceInfo || PiecesUsed[UsedIdx].ColorIndex != Piece->mColorIndex)
				continue;

			PiecesUsed[UsedIdx].Count++;
			break;
		}

		if (UsedIdx == PiecesUsed.GetSize())
		{
			lcPiecesUsedEntry& Entry = PiecesUsed.Add();

			Entry.Info = Piece->mPieceInfo;
			Entry.ColorIndex = Piece->mColorIndex;
			Entry.Count = 1;
		}
	}
}

// Create a series of pictures
void Project::CreateImages(Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite)
{
	if (!GL_BeginRenderToTexture(width, height))
	{
		gMainWindow->DoMessageBox("Error creating images.", LC_MB_ICONERROR | LC_MB_OK);
		return;
	}

	unsigned short oldtime;
	unsigned char* buf = (unsigned char*)malloc (width*height*3);
	oldtime = m_nCurStep;

	View view(this);
	view.SetCamera(gMainWindow->GetActiveView()->mCamera, false);
	view.mWidth = width;
	view.mHeight = height;
	view.SetContext(gMainWindow->mPreviewWidget->mContext);

	if (!hilite)
		SelectAndFocusNone(false);

	RenderInitialize();

	for (int i = from; i <= to; i++)
	{
		m_nCurStep = i;

		if (hilite)
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];
				Piece->SetSelected(Piece->GetStepShow() == i);
			}
		}

		CalculateStep();
		Render(&view, true);
		images[i-from].FromOpenGL(width, height);
	}

	if (hilite)
		SelectAndFocusNone(false);

	m_nCurStep = (unsigned char)oldtime;
	CalculateStep();
	free (buf);

	GL_EndRenderToTexture();
}

void Project::CreateHTMLPieceList(FILE* f, int nStep, bool bImages, const char* ext)
{
	int* ColorsUsed = new int[gColorList.GetSize()];
	memset(ColorsUsed, 0, sizeof(ColorsUsed[0]) * gColorList.GetSize());
	int* PiecesUsed = new int[gColorList.GetSize()];
	int NumColors = 0;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if ((Piece->GetStepShow() == nStep) || (nStep == 0))
			ColorsUsed[Piece->mColorIndex]++;
	}

	fputs("<br><table border=1><tr><td><center>Piece</center></td>\n",f);

	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		if (ColorsUsed[ColorIdx])
		{
			ColorsUsed[ColorIdx] = NumColors;
			NumColors++;
			fprintf(f, "<td><center>%s</center></td>\n", gColorList[ColorIdx].Name);
		}
	}
	NumColors++;
	fputs("</tr>\n",f);

	PieceInfo* pInfo;
	for (int j = 0; j < lcGetPiecesLibrary()->mPieces.GetSize(); j++)
	{
		bool Add = false;
		memset(PiecesUsed, 0, sizeof(PiecesUsed[0]) * gColorList.GetSize());
		pInfo = lcGetPiecesLibrary()->mPieces[j];

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			Piece* Piece = mPieces[PieceIdx];

			if ((Piece->mPieceInfo == pInfo) && ((Piece->GetStepShow() == nStep) || (nStep == 0)))
			{
				PiecesUsed[Piece->mColorIndex]++;
				Add = true;
			}
		}

		if (Add)
		{
			if (bImages)
				fprintf(f, "<tr><td><IMG SRC=\"%s%s\" ALT=\"%s\"></td>\n", pInfo->m_strName, ext, pInfo->m_strDescription);
			else
					fprintf(f, "<tr><td>%s</td>\n", pInfo->m_strDescription);

			int curcol = 1;
			for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
			{
				if (PiecesUsed[ColorIdx])
				{
					while (curcol != ColorsUsed[ColorIdx] + 1)
					{
						fputs("<td><center>-</center></td>\n", f);
						curcol++;
					}

					fprintf(f, "<td><center>%d</center></td>\n", PiecesUsed[ColorIdx]);
					curcol++;
				}
			}

			while (curcol != NumColors)
			{
				fputs("<td><center>-</center></td>\n", f);
				curcol++;
			}

			fputs("</tr>\n", f);
		}
	}
	fputs("</table>\n<br>", f);

	delete[] PiecesUsed;
	delete[] ColorsUsed;
}

void Project::Export3DStudio()
{
	if (mPieces.IsEmpty())
	{
		gMainWindow->DoMessageBox("Nothing to export.", LC_MB_OK | LC_MB_ICONINFORMATION);
		return;
	}

	char FileName[LC_MAXPATH];
	memset(FileName, 0, sizeof(FileName));

	if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_3DSTUDIO, FileName))
		return;

	lcDiskFile File;

	if (!File.Open(FileName, "wb"))
	{
		gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
		return;
	}

	long M3DStart = File.GetPosition();
	File.WriteU16(0x4D4D); // CHK_M3DMAGIC
	File.WriteU32(0);

	File.WriteU16(0x0002); // CHK_M3D_VERSION
	File.WriteU32(10);
	File.WriteU32(3);

	long MDataStart = File.GetPosition();
	File.WriteU16(0x3D3D); // CHK_MDATA
	File.WriteU32(0);

	File.WriteU16(0x3D3E); // CHK_MESH_VERSION
	File.WriteU32(10);
	File.WriteU32(3);

	const int MaterialNameLength = 11;
	char MaterialName[32];

	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];

		sprintf(MaterialName, "Material%03d", ColorIdx);

		long MaterialStart = File.GetPosition();
		File.WriteU16(0xAFFF); // CHK_MAT_ENTRY
		File.WriteU32(0);

		File.WriteU16(0xA000); // CHK_MAT_NAME
		File.WriteU32(6 + MaterialNameLength + 1);
		File.WriteBuffer(MaterialName, MaterialNameLength + 1);

		File.WriteU16(0xA010); // CHK_MAT_AMBIENT
		File.WriteU32(24);

		File.WriteU16(0x0011); // CHK_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0x0012); // CHK_LIN_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0xA020); // CHK_MAT_AMBIENT
		File.WriteU32(24);

		File.WriteU16(0x0011); // CHK_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0x0012); // CHK_LIN_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * Color->Value[0] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[1] + 0.5));
		File.WriteU8(floor(255.0 * Color->Value[2] + 0.5));

		File.WriteU16(0xA030); // CHK_MAT_SPECULAR
		File.WriteU32(24);

		File.WriteU16(0x0011); // CHK_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));

		File.WriteU16(0x0012); // CHK_LIN_COLOR_24
		File.WriteU32(9);

		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));
		File.WriteU8(floor(255.0 * 0.9f + 0.5));

		File.WriteU16(0xA040); // CHK_MAT_SHININESS
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.25 + 0.5));

		File.WriteU16(0xA041); // CHK_MAT_SHIN2PCT
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.05 + 0.5));

		File.WriteU16(0xA050); // CHK_MAT_TRANSPARENCY
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * (1.0f - Color->Value[3]) + 0.5));

		File.WriteU16(0xA052); // CHK_MAT_XPFALL
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.0 + 0.5));

		File.WriteU16(0xA053); // CHK_MAT_REFBLUR
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.2 + 0.5));

		File.WriteU16(0xA100); // CHK_MAT_SHADING
		File.WriteU32(8);

		File.WriteS16(3);

		File.WriteU16(0xA084); // CHK_MAT_SELF_ILPCT
		File.WriteU32(14);

		File.WriteU16(0x0030); // CHK_INT_PERCENTAGE
		File.WriteU32(8);

		File.WriteS16((lcuint8)floor(100.0 * 0.0 + 0.5));

		File.WriteU16(0xA081); // CHK_MAT_TWO_SIDE
		File.WriteU32(6);

		File.WriteU16(0xA087); // CHK_MAT_WIRE_SIZE
		File.WriteU32(10);

		File.WriteFloat(1.0f);

		long MaterialEnd = File.GetPosition();
		File.Seek(MaterialStart + 2, SEEK_SET);
		File.WriteU32(MaterialEnd - MaterialStart);
		File.Seek(MaterialEnd, SEEK_SET);
	}

	File.WriteU16(0x0100); // CHK_MASTER_SCALE
	File.WriteU32(10);

	File.WriteFloat(1.0f);

	File.WriteU16(0x1400); // CHK_LO_SHADOW_BIAS
	File.WriteU32(10);

	File.WriteFloat(1.0f);

	File.WriteU16(0x1420); // CHK_SHADOW_MAP_SIZE
	File.WriteU32(8);

	File.WriteS16(512);

	File.WriteU16(0x1450); // CHK_SHADOW_FILTER
	File.WriteU32(10);

	File.WriteFloat(3.0f);

	File.WriteU16(0x1460); // CHK_RAY_BIAS
	File.WriteU32(10);

	File.WriteFloat(1.0f);

	File.WriteU16(0x1500); // CHK_O_CONSTS
	File.WriteU32(18);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);

	File.WriteU16(0x2100); // CHK_AMBIENT_LIGHT
	File.WriteU32(42);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mAmbientColor, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mAmbientColor, 3);

	File.WriteU16(0x1200); // CHK_SOLID_BGND
	File.WriteU32(42);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mBackgroundSolidColor, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mBackgroundSolidColor, 3);

	File.WriteU16(0x1100); // CHK_BIT_MAP
	File.WriteU32(6 + 1 + strlen(mProperties.mBackgroundImage.Buffer()));
	File.WriteBuffer(mProperties.mBackgroundImage.Buffer(), strlen(mProperties.mBackgroundImage.Buffer()) + 1);

	File.WriteU16(0x1300); // CHK_V_GRADIENT
	File.WriteU32(118);

	File.WriteFloat(1.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mBackgroundGradientColor1, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mBackgroundGradientColor1, 3);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats((mProperties.mBackgroundGradientColor1 + mProperties.mBackgroundGradientColor2) / 2.0f, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats((mProperties.mBackgroundGradientColor1 + mProperties.mBackgroundGradientColor2) / 2.0f, 3);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mBackgroundGradientColor2, 3);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mBackgroundGradientColor2, 3);

	if (mProperties.mBackgroundType == LC_BACKGROUND_GRADIENT)
	{
		File.WriteU16(0x1301); // LIB3DS_USE_V_GRADIENT
		File.WriteU32(6);
	}
	else if (mProperties.mBackgroundType == LC_BACKGROUND_IMAGE)
	{
		File.WriteU16(0x1101); // LIB3DS_USE_BIT_MAP
		File.WriteU32(6);
	}
	else
	{
		File.WriteU16(0x1201); // LIB3DS_USE_SOLID_BGND
		File.WriteU32(6);
	}

	File.WriteU16(0x2200); // CHK_FOG
	File.WriteU32(46);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(1000.0f);
	File.WriteFloat(100.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mFogColor, 3);

	File.WriteU16(0x2210); // CHK_FOG_BGND
	File.WriteU32(6);

	File.WriteU16(0x2302); // CHK_LAYER_FOG
	File.WriteU32(40);

	File.WriteFloat(0.0f);
	File.WriteFloat(100.0f);
	File.WriteFloat(50.0f);
	File.WriteU32(0x00100000);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloats(mProperties.mFogColor, 3);

	File.WriteU16(0x2300); // CHK_DISTANCE_CUE
	File.WriteU32(28);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(1000.0f);
	File.WriteFloat(100.0f);

	File.WriteU16(0x2310); // CHK_DICHK_DCUE_BGNDSTANCE_CUE
	File.WriteU32(6);

	int NumPieces = 0;
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* piece = mPieces[PieceIdx];
		PieceInfo* Info = piece->mPieceInfo;
		lcMesh* Mesh = Info->mMesh;

		if (Mesh->mIndexType == GL_UNSIGNED_INT)
			continue;

		long NamedObjectStart = File.GetPosition();
		File.WriteU16(0x4000); // CHK_NAMED_OBJECT
		File.WriteU32(0);

		char Name[32];
		sprintf(Name, "Piece%.3d", NumPieces);
		NumPieces++;
		File.WriteBuffer(Name, strlen(Name) + 1);

		long TriObjectStart = File.GetPosition();
		File.WriteU16(0x4100); // CHK_N_TRI_OBJECT
		File.WriteU32(0);

		File.WriteU16(0x4110); // CHK_POINT_ARRAY
		File.WriteU32(8 + 12 * Mesh->mNumVertices);

		File.WriteU16(Mesh->mNumVertices);

		float* Verts = (float*)Mesh->mVertexBuffer.mData;
		const lcMatrix44& ModelWorld = piece->mModelWorld;

		for (int VertexIdx = 0; VertexIdx < Mesh->mNumVertices; VertexIdx++)
		{
			lcVector3 Pos(Verts[VertexIdx * 3], Verts[VertexIdx * 3 + 1], Verts[VertexIdx * 3 + 2]);
			Pos = lcMul31(Pos, ModelWorld);
			File.WriteFloat(Pos[0]);
			File.WriteFloat(Pos[1]);
			File.WriteFloat(Pos[2]);
		}

		File.WriteU16(0x4160); // CHK_MESH_MATRIX
		File.WriteU32(54);

		lcMatrix44 Matrix = lcMatrix44Identity();
		File.WriteFloats(Matrix[0], 3);
		File.WriteFloats(Matrix[1], 3);
		File.WriteFloats(Matrix[2], 3);
		File.WriteFloats(Matrix[3], 3);

		File.WriteU16(0x4165); // CHK_MESH_COLOR
		File.WriteU32(7);

		File.WriteU8(0);

		long FaceArrayStart = File.GetPosition();
		File.WriteU16(0x4120); // CHK_FACE_ARRAY
		File.WriteU32(0);

		int NumTriangles = 0;

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			NumTriangles += Section->NumIndices / 3;
		}

		File.WriteU16(NumTriangles);

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			lcuint16* Indices = (lcuint16*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(lcuint16);

			for (int IndexIdx = 0; IndexIdx < Section->NumIndices; IndexIdx += 3)
			{
				File.WriteU16(Indices[IndexIdx + 0]);
				File.WriteU16(Indices[IndexIdx + 1]);
				File.WriteU16(Indices[IndexIdx + 2]);
				File.WriteU16(7);
			}
		}

		NumTriangles = 0;

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			int MaterialIndex = Section->ColorIndex == gDefaultColor ? piece->mColorIndex : Section->ColorIndex;

			File.WriteU16(0x4130); // CHK_MSH_MAT_GROUP
			File.WriteU32(6 + MaterialNameLength + 1 + 2 + 2 * Section->NumIndices / 3);

			sprintf(MaterialName, "Material%03d", MaterialIndex);
			File.WriteBuffer(MaterialName, MaterialNameLength + 1);

			File.WriteU16(Section->NumIndices / 3);

			for (int IndexIdx = 0; IndexIdx < Section->NumIndices; IndexIdx += 3)
				File.WriteU16(NumTriangles++);
		}

		long FaceArrayEnd = File.GetPosition();
		File.Seek(FaceArrayStart + 2, SEEK_SET);
		File.WriteU32(FaceArrayEnd - FaceArrayStart);
		File.Seek(FaceArrayEnd, SEEK_SET);

		long TriObjectEnd = File.GetPosition();
		File.Seek(TriObjectStart + 2, SEEK_SET);
		File.WriteU32(TriObjectEnd - TriObjectStart);
		File.Seek(TriObjectEnd, SEEK_SET);

		long NamedObjectEnd = File.GetPosition();
		File.Seek(NamedObjectStart + 2, SEEK_SET);
		File.WriteU32(NamedObjectEnd - NamedObjectStart);
		File.Seek(NamedObjectEnd, SEEK_SET);
	}

	long MDataEnd = File.GetPosition();
	File.Seek(MDataStart + 2, SEEK_SET);
	File.WriteU32(MDataEnd - MDataStart);
	File.Seek(MDataEnd, SEEK_SET);

	long KFDataStart = File.GetPosition();
	File.WriteU16(0xB000); // CHK_KFDATA
	File.WriteU32(0);

	File.WriteU16(0xB00A); // LIB3DS_KFHDR
	File.WriteU32(6 + 2 + 1 + 4);

	File.WriteS16(5);
	File.WriteU8(0);
	File.WriteS32(100);

	long KFDataEnd = File.GetPosition();
	File.Seek(KFDataStart + 2, SEEK_SET);
	File.WriteU32(KFDataEnd - KFDataStart);
	File.Seek(KFDataEnd, SEEK_SET);

	long M3DEnd = File.GetPosition();
	File.Seek(M3DStart + 2, SEEK_SET);
	File.WriteU32(M3DEnd - M3DStart);
	File.Seek(M3DEnd, SEEK_SET);
}

void Project::ExportPOVRay(lcFile& POVFile)
{
	char Line[1024];

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	char* PieceTable = new char[Library->mPieces.GetSize() * LC_PIECE_NAME_LEN];
	int* PieceFlags = new int[Library->mPieces.GetSize()];
	int NumColors = gColorList.GetSize();
	char* ColorTable = new char[NumColors * LC_MAX_COLOR_NAME];

	memset(PieceTable, 0, Library->mPieces.GetSize() * LC_PIECE_NAME_LEN);
	memset(PieceFlags, 0, Library->mPieces.GetSize() * sizeof(int));
	memset(ColorTable, 0, NumColors * LC_MAX_COLOR_NAME);

	enum
	{
		LGEO_PIECE_LGEO  = 0x01,
		LGEO_PIECE_AR    = 0x02,
		LGEO_PIECE_SLOPE = 0x04
	};

	enum
	{
		LGEO_COLOR_SOLID       = 0x01,
		LGEO_COLOR_TRANSPARENT = 0x02,
		LGEO_COLOR_CHROME      = 0x04,
		LGEO_COLOR_PEARL       = 0x08,
		LGEO_COLOR_METALLIC    = 0x10,
		LGEO_COLOR_RUBBER      = 0x20,
		LGEO_COLOR_GLITTER     = 0x40
	};

	char LGEOPath[LC_MAXPATH];
	strcpy(LGEOPath, lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH));

	// Parse LGEO tables.
	if (LGEOPath[0])
	{
		lcDiskFile TableFile, ColorFile;
		char Filename[LC_MAXPATH];

		int Length = strlen(LGEOPath);

		if ((LGEOPath[Length - 1] != '\\') && (LGEOPath[Length - 1] != '/'))
			strcat(LGEOPath, "/");

		strcpy(Filename, LGEOPath);
		strcat(Filename, "lg_elements.lst");

		if (!TableFile.Open(Filename, "rt"))
		{
			delete[] PieceTable;
			delete[] PieceFlags;
			gMainWindow->DoMessageBox("Could not find LGEO files.", LC_MB_OK | LC_MB_ICONERROR);
			return;
		}

		while (TableFile.ReadLine(Line, sizeof(Line)))
		{
			char Src[1024], Dst[1024], Flags[1024];

			if (*Line == ';')
				continue;

			if (sscanf(Line,"%s%s%s", Src, Dst, Flags) != 3)
				continue;

			strupr(Src);

			PieceInfo* Info = Library->FindPiece(Src, false);
			if (!Info)
				continue;

			int Index = Library->mPieces.FindIndex(Info);

			if (strchr(Flags, 'L'))
			{
				PieceFlags[Index] |= LGEO_PIECE_LGEO;
				sprintf(PieceTable + Index * LC_PIECE_NAME_LEN, "lg_%s", Dst);
			}

			if (strchr(Flags, 'A'))
			{
				PieceFlags[Index] |= LGEO_PIECE_AR;
				sprintf(PieceTable + Index * LC_PIECE_NAME_LEN, "ar_%s", Dst);
			}

			if (strchr(Flags, 'S'))
				PieceFlags[Index] |= LGEO_PIECE_SLOPE;
		}

		strcpy(Filename, LGEOPath);
		strcat(Filename, "lg_colors.lst");

		if (!ColorFile.Open(Filename, "rt"))
		{
			delete[] PieceTable;
			delete[] PieceFlags;
			gMainWindow->DoMessageBox("Could not find LGEO files.", LC_MB_OK | LC_MB_ICONERROR);
			return;
		}

		while (ColorFile.ReadLine(Line, sizeof(Line)))
		{
			char Name[1024], Flags[1024];
			int Code;

			if (*Line == ';')
				continue;

			if (sscanf(Line,"%d%s%s", &Code, Name, Flags) != 3)
				continue;

			int Color = lcGetColorIndex(Code);
			if (Color >= NumColors)
				continue;

			strcpy(&ColorTable[Color * LC_MAX_COLOR_NAME], Name);
		}
	}

	const char* OldLocale = setlocale(LC_NUMERIC, "C");

	// Add includes.
	if (LGEOPath[0])
	{
		POVFile.WriteLine("#include \"lg_defs.inc\"\n#include \"lg_color.inc\"\n\n");

		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		{
			Piece* piece = mPieces[PieceIdx];
			PieceInfo* Info = piece->mPieceInfo;

			for (int CheckIdx = 0; CheckIdx < mPieces.GetSize(); CheckIdx++)
			{
				if (mPieces[CheckIdx]->mPieceInfo != Info)
					continue;

				if (CheckIdx != PieceIdx)
					break;

				int Index = Library->mPieces.FindIndex(Info);

				if (PieceTable[Index * LC_PIECE_NAME_LEN])
				{
					sprintf(Line, "#include \"%s.inc\"\n", PieceTable + Index * LC_PIECE_NAME_LEN);
					POVFile.WriteLine(Line);
				}

				break;
			}
		}

		POVFile.WriteLine("\n");
	}
	else
		POVFile.WriteLine("#include \"colors.inc\"\n\n");

	// Add color definitions.
	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];

		if (lcIsColorTranslucent(ColorIdx))
		{
			sprintf(Line, "#declare lc_%s = texture { pigment { rgb <%f, %f, %f> filter 0.9 } finish { ambient 0.3 diffuse 0.2 reflection 0.25 phong 0.3 phong_size 60 } }\n",
					Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
		}
		else
		{
			sprintf(Line, "#declare lc_%s = texture { pigment { rgb <%f, %f, %f> } finish { ambient 0.1 phong 0.2 phong_size 20 } }\n",
				   Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
		}

		POVFile.WriteLine(Line);

		if (!ColorTable[ColorIdx * LC_MAX_COLOR_NAME])
			sprintf(&ColorTable[ColorIdx * LC_MAX_COLOR_NAME], "lc_%s", Color->SafeName);
	}

	POVFile.WriteLine("\n");

	// Add pieces not included in LGEO.
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];
		PieceInfo* Info = Piece->mPieceInfo;
		int Index = Library->mPieces.FindIndex(Info);

		if (PieceTable[Index * LC_PIECE_NAME_LEN])
			continue;

		char Name[LC_PIECE_NAME_LEN];
		char* Ptr;

		strcpy(Name, Info->m_strName);
		while ((Ptr = strchr(Name, '-')))
			*Ptr = '_';

		sprintf(PieceTable + Index * LC_PIECE_NAME_LEN, "lc_%s", Name);

		Info->mMesh->ExportPOVRay(POVFile, Name, ColorTable);

		POVFile.WriteLine("}\n\n");

		sprintf(Line, "#declare lc_%s_clear = lc_%s\n\n", Name, Name);
		POVFile.WriteLine(Line);
	}

	lcCamera* Camera = gMainWindow->GetActiveView()->mCamera;
	const lcVector3& Position = Camera->mPosition;
	const lcVector3& Target = Camera->mTargetPosition;
	const lcVector3& Up = Camera->mUpVector;

	sprintf(Line, "camera {\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f\n}\n\n",
	        Up[0], Up[1], Up[2], Position[1], Position[0], Position[2], Target[1], Target[0], Target[2], Camera->m_fovy);
	POVFile.WriteLine(Line);
	sprintf(Line, "background { color rgb <%1g, %1g, %1g> }\n\nlight_source { <0, 0, 20> White shadowless }\n\n",
	        mProperties.mBackgroundSolidColor[0], mProperties.mBackgroundSolidColor[1], mProperties.mBackgroundSolidColor[2]);
	POVFile.WriteLine(Line);

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];
		int Index = Library->mPieces.FindIndex(Piece->mPieceInfo);
		int Color;

		Color = Piece->mColorIndex;
		const char* Suffix = lcIsColorTranslucent(Color) ? "_clear" : "";

		const float* f = Piece->mModelWorld;

		if (PieceFlags[Index] & LGEO_PIECE_SLOPE)
		{
			sprintf(Line, "merge {\n object {\n  %s%s\n  texture { %s }\n }\n"
			        " object {\n  %s_slope\n  texture { %s normal { bumps 0.3 scale 0.02 } }\n }\n"
			        " matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
			        PieceTable + Index * LC_PIECE_NAME_LEN, Suffix, &ColorTable[Color * LC_MAX_COLOR_NAME], PieceTable + Index * LC_PIECE_NAME_LEN, &ColorTable[Color * LC_MAX_COLOR_NAME],
			        -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13], f[12], f[14]);
		}
		else
		{
			sprintf(Line, "object {\n %s%s\n texture { %s }\n matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
			        PieceTable + Index * LC_PIECE_NAME_LEN, Suffix, &ColorTable[Color * LC_MAX_COLOR_NAME], -f[5], -f[4], -f[6], -f[1], -f[0], -f[2], f[9], f[8], f[10], f[13], f[12], f[14]);
		}

		POVFile.WriteLine(Line);
	}

	delete[] PieceTable;
	delete[] PieceFlags;
	setlocale(LC_NUMERIC, OldLocale);
	POVFile.Close();
}

void Project::HandleCommand(LC_COMMANDS id)
{
	switch (id)
	{
		case LC_FILE_NEW:
		{
			if (!SaveModified())
				return;  // leave the original one

			OnNewDocument();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_FILE_OPEN:
		{
			char FileName[LC_MAXPATH];

			if (m_strPathName[0])
				strcpy(FileName, m_strPathName);
			else
				strcpy(FileName, lcGetProfileString(LC_PROFILE_PROJECTS_PATH));

			if (gMainWindow->DoDialog(LC_DIALOG_OPEN_PROJECT, FileName))
				OpenProject(FileName);
		} break;

		case LC_FILE_MERGE:
		{
			char FileName[LC_MAXPATH];

			if (m_strPathName[0])
				strcpy(FileName, m_strPathName);
			else
				strcpy(FileName, lcGetProfileString(LC_PROFILE_PROJECTS_PATH));

			if (gMainWindow->DoDialog(LC_DIALOG_MERGE_PROJECT, FileName))
			{
				lcDiskFile file;
				if (file.Open(FileName, "rb"))
				{
					if (file.GetLength() != 0)
					{
						FileLoad(&file, false, true);
						CheckPoint("Merging");
					}
					file.Close();
				}
			}
		} break;

		case LC_FILE_SAVE:
		{
			DoSave(m_strPathName);
		} break;

		case LC_FILE_SAVEAS:
		{
			DoSave(NULL);
		} break;

		case LC_FILE_SAVE_IMAGE:
		{
			lcImageDialogOptions Options;

			int ImageOptions = lcGetProfileInt(LC_PROFILE_IMAGE_OPTIONS);

			Options.Format = (LC_IMAGE_FORMAT)(ImageOptions & ~(LC_IMAGE_MASK));
			Options.Transparent = (ImageOptions & LC_IMAGE_TRANSPARENT) != 0;
			Options.Width = lcGetProfileInt(LC_PROFILE_IMAGE_WIDTH);
			Options.Height = lcGetProfileInt(LC_PROFILE_IMAGE_HEIGHT);

			if (m_strPathName[0])
				strcpy(Options.FileName, m_strPathName);
			else if (m_strTitle[0])
				strcpy(Options.FileName, m_strTitle);
			else
				strcpy(Options.FileName, "Image");

			if (Options.FileName[0])
			{
				char* ext = strrchr(Options.FileName, '.');

				if (ext && (!stricmp(ext, ".lcd") || !stricmp(ext, ".dat") || !stricmp(ext, ".ldr")))
					*ext = 0;

				switch (Options.Format)
				{
				case LC_IMAGE_BMP: strcat(Options.FileName, ".bmp");
					break;
				case LC_IMAGE_JPG: strcat(Options.FileName, ".jpg");
					break;
				default:
				case LC_IMAGE_PNG: strcat(Options.FileName, ".png");
					break;
				}
			}

			Options.Start = m_nCurStep;
			Options.End = m_nCurStep;

			if (!gMainWindow->DoDialog(LC_DIALOG_SAVE_IMAGE, &Options))
				break;

			ImageOptions = Options.Format;

			if (Options.Transparent)
				ImageOptions |= LC_IMAGE_TRANSPARENT;

			lcSetProfileInt(LC_PROFILE_IMAGE_OPTIONS, ImageOptions);
			lcSetProfileInt(LC_PROFILE_IMAGE_WIDTH, Options.Width);
			lcSetProfileInt(LC_PROFILE_IMAGE_HEIGHT, Options.Height);

			if (!Options.FileName[0])
				strcpy(Options.FileName, "Image");

			char* Ext = strrchr(Options.FileName, '.');
			if (Ext)
			{
				if (!strcmp(Ext, ".jpg") || !strcmp(Ext, ".jpeg") || !strcmp(Ext, ".bmp") || !strcmp(Ext, ".png"))
					*Ext = 0;
			}

			const char* ext;
			switch (Options.Format)
			{
			case LC_IMAGE_BMP: ext = ".bmp";
				break;
			case LC_IMAGE_JPG: ext = ".jpg";
				break;
			default:
			case LC_IMAGE_PNG: ext = ".png";
				break;
			}

			Options.End = lcMin(Options.End, 255);
			Options.Start = lcMax(1, Options.Start);

			if (Options.Start > Options.End)
			{
				if (Options.Start > Options.End)
				{
					int Temp = Options.Start;
					Options.Start = Options.End;
					Options.End = Temp;
				}
			}

			Image* images = new Image[Options.End - Options.Start + 1];
			CreateImages(images, Options.Width, Options.Height, Options.Start, Options.End, false);

			for (int i = 0; i <= Options.End - Options.Start; i++)
			{
				char filename[LC_MAXPATH];

				if (Options.Start != Options.End)
				{
					sprintf(filename, "%s%02d%s", Options.FileName, i+1, ext);
				}
				else
				{
					strcat(Options.FileName, ext);
					strcpy(filename, Options.FileName);
				}

				images[i].FileSave(filename, Options.Format, Options.Transparent);
			}
			delete []images;
		} break;

		case LC_FILE_EXPORT_3DS:
			Export3DStudio();
			break;

		case LC_FILE_EXPORT_HTML:
		{
			lcHTMLDialogOptions Options;

			strcpy(Options.PathName, m_strPathName);

			if (Options.PathName[0] != 0)
			{
				char* Slash = strrchr(Options.PathName, '/');

				if (Slash == NULL)
					Slash = strrchr(Options.PathName, '\\');

				if (Slash)
					{
					Slash++;
					*Slash = 0;
				}
			}

			int ImageOptions = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS);
			int HTMLOptions = lcGetProfileInt(LC_PROFILE_HTML_OPTIONS);

			Options.ImageFormat = (LC_IMAGE_FORMAT)(ImageOptions & ~(LC_IMAGE_MASK));
			Options.TransparentImages = (ImageOptions & LC_IMAGE_TRANSPARENT) != 0;
			Options.SinglePage = (HTMLOptions & LC_HTML_SINGLEPAGE) != 0;
			Options.IndexPage = (HTMLOptions & LC_HTML_INDEX) != 0;
			Options.StepImagesWidth = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH);
			Options.StepImagesHeight = lcGetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT);
			Options.HighlightNewParts = (HTMLOptions & LC_HTML_HIGHLIGHT) != 0;
			Options.PartsListStep = (HTMLOptions & LC_HTML_LISTSTEP) != 0;
			Options.PartsListEnd = (HTMLOptions & LC_HTML_LISTEND) != 0;
			Options.PartsListImages = (HTMLOptions & LC_HTML_IMAGES) != 0;
			Options.PartImagesColor = lcGetColorIndex(lcGetProfileInt(LC_PROFILE_HTML_PARTS_COLOR));
			Options.PartImagesWidth = lcGetProfileInt(LC_PROFILE_HTML_PARTS_WIDTH);
			Options.PartImagesHeight = lcGetProfileInt(LC_PROFILE_HTML_PARTS_HEIGHT);

			if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_HTML, &Options))
				break;

			HTMLOptions = 0;

			if (Options.SinglePage)
				HTMLOptions |= LC_HTML_SINGLEPAGE;
			if (Options.IndexPage)
				HTMLOptions |= LC_HTML_INDEX;
			if (Options.HighlightNewParts)
				HTMLOptions |= LC_HTML_HIGHLIGHT;
			if (Options.PartsListStep)
				HTMLOptions |= LC_HTML_LISTSTEP;
			if (Options.PartsListEnd)
				HTMLOptions |= LC_HTML_LISTEND;
			if (Options.PartsListImages)
				HTMLOptions |= LC_HTML_IMAGES;

			ImageOptions = Options.ImageFormat;

			if (Options.TransparentImages)
				ImageOptions |= LC_IMAGE_TRANSPARENT;

			lcSetProfileInt(LC_PROFILE_HTML_IMAGE_OPTIONS, ImageOptions);
			lcSetProfileInt(LC_PROFILE_HTML_OPTIONS, HTMLOptions);
			lcSetProfileInt(LC_PROFILE_HTML_IMAGE_WIDTH, Options.StepImagesWidth);
			lcSetProfileInt(LC_PROFILE_HTML_IMAGE_HEIGHT, Options.StepImagesHeight);
			lcSetProfileInt(LC_PROFILE_HTML_PARTS_COLOR, lcGetColorCode(Options.PartImagesColor));
			lcSetProfileInt(LC_PROFILE_HTML_PARTS_WIDTH, Options.PartImagesWidth);
			lcSetProfileInt(LC_PROFILE_HTML_PARTS_HEIGHT, Options.PartImagesHeight);

			int PathLength = strlen(Options.PathName);
			if (PathLength && Options.PathName[PathLength] != '/' && Options.PathName[PathLength] != '\\')
				strcat(Options.PathName, "/");

			// TODO: create directory

			// TODO: Move to its own function
			{
				FILE* f;
				const char *ext, *htmlext;
				char fn[LC_MAXPATH];
				int i;
				unsigned short last = GetLastStep();

				switch (Options.ImageFormat)
				{
				case LC_IMAGE_BMP: ext = ".bmp";
					break;
				case LC_IMAGE_JPG: ext = ".jpg";
					break;
				default:
				case LC_IMAGE_PNG: ext = ".png";
					break;
				}

				htmlext = ".html";

				if (Options.SinglePage)
				{
					strcpy(fn, Options.PathName);
					strcat(fn, m_strTitle);
					strcat(fn, htmlext);
					f = fopen (fn, "wt");

					if (!f)
					{
						gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
						break;
					}

					fprintf (f, "<HTML>\n<HEAD>\n<TITLE>Instructions for %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

					for (i = 1; i <= last; i++)
					{
						fprintf(f, "<IMG SRC=\"%s-%02d%s\" ALT=\"Step %02d\" WIDTH=%d HEIGHT=%d><BR><BR>\n",
							m_strTitle, i, ext, i, Options.StepImagesWidth, Options.StepImagesHeight);

						if (Options.PartsListStep)
							CreateHTMLPieceList(f, i, Options.PartsListImages, ext);
					}

					if (Options.PartsListEnd)
						CreateHTMLPieceList(f, 0, Options.PartsListImages, ext);

					fputs("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\n", f);
					fclose(f);
				}
				else
				{
					if (Options.IndexPage)
					{
						strcpy(fn, Options.PathName);
						strcat (fn, m_strTitle);
						strcat (fn, "-index");
						strcat (fn, htmlext);
						f = fopen (fn, "wt");

						if (!f)
						{
							gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
							break;
						}

						fprintf(f, "<HTML>\n<HEAD>\n<TITLE>Instructions for %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

						for (i = 1; i <= last; i++)
							fprintf(f, "<A HREF=\"%s-%02d%s\">Step %d<BR>\n</A>", m_strTitle, i, htmlext, i);

						if (Options.PartsListEnd)
							fprintf(f, "<A HREF=\"%s-pieces%s\">Pieces Used</A><BR>\n", m_strTitle, htmlext);

						fputs("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\n", f);
						fclose(f);
					}

					// Create each step
					for (i = 1; i <= last; i++)
					{
						sprintf(fn, "%s%s-%02d%s", Options.PathName, m_strTitle, i, htmlext);
						f = fopen(fn, "wt");

						if (!f)
						{
							gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
							break;
						}

						fprintf(f, "<HTML>\n<HEAD>\n<TITLE>%s - Step %02d</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle, i);
						fprintf(f, "<IMG SRC=\"%s-%02d%s\" ALT=\"Step %02d\" WIDTH=%d HEIGHT=%d><BR><BR>\n",
							m_strTitle, i, ext, i, Options.StepImagesWidth, Options.StepImagesHeight);

						if (Options.PartsListStep)
							CreateHTMLPieceList(f, i, Options.PartsListImages, ext);

						fputs("</CENTER>\n<BR><HR><BR>", f);
						if (i != 1)
							fprintf(f, "<A HREF=\"%s-%02d%s\">Previous</A> ", m_strTitle, i-1, htmlext);

						if (Options.IndexPage)
							fprintf(f, "<A HREF=\"%s-index%s\">Index</A> ", m_strTitle, htmlext);

						if (i != last)
							fprintf(f, "<A HREF=\"%s-%02d%s\">Next</A>", m_strTitle, i+1, htmlext);
						else if (Options.PartsListEnd)
								fprintf(f, "<A HREF=\"%s-pieces%s\">Pieces Used</A>", m_strTitle, htmlext);

						fputs("<BR></HTML>\n",f);
						fclose(f);
					}

					if (Options.PartsListEnd)
					{
						strcpy(fn, Options.PathName);
						strcat (fn, m_strTitle);
						strcat (fn, "-pieces");
						strcat (fn, htmlext);
						f = fopen (fn, "wt");

						if (!f)
						{
							gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
							break;
						}

						fprintf (f, "<HTML>\n<HEAD>\n<TITLE>Pieces used by %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

						CreateHTMLPieceList(f, 0, Options.PartsListImages, ext);

						fputs("</CENTER>\n<BR><HR><BR>", f);
						fprintf(f, "<A HREF=\"%s-%02d%s\">Previous</A> ", m_strTitle, i-1, htmlext);

						if (Options.IndexPage)
							fprintf(f, "<A HREF=\"%s-index%s\">Index</A> ", m_strTitle, htmlext);

						fputs("<BR></HTML>\n",f);
						fclose(f);
					}
				}

				// Save step pictures
				Image* images = new Image[last];
				CreateImages(images, Options.StepImagesWidth, Options.StepImagesHeight, 1, last, Options.HighlightNewParts);

				for (i = 0; i < last; i++)
				{
					sprintf(fn, "%s%s-%02d%s", Options.PathName, m_strTitle, i+1, ext);
					images[i].FileSave(fn, Options.ImageFormat, Options.TransparentImages);
				}
				delete []images;

				if (Options.PartsListImages)
				{
					int cx = Options.PartImagesWidth, cy = Options.PartImagesHeight;
					if (!GL_BeginRenderToTexture(cx, cy))
					{
						gMainWindow->DoMessageBox("Error creating images.", LC_MB_ICONERROR | LC_MB_OK);
						break;
					}

					float aspect = (float)cx/(float)cy;
					glViewport(0, 0, cx, cy);

					PieceInfo* pInfo;
					for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
					{
						Piece* Piece = mPieces[PieceIdx];
						bool bSkip = false;
						pInfo = Piece->mPieceInfo;

						for (int CheckIdx = 0; CheckIdx < PieceIdx; CheckIdx++)
						{
							if (mPieces[CheckIdx]->mPieceInfo == pInfo)
							{
								bSkip = true;
								break;
							}
						}

						if (bSkip)
							continue;

						glDepthFunc(GL_LEQUAL);
						glEnable(GL_DEPTH_TEST);
						glEnable(GL_POLYGON_OFFSET_FILL);
						glPolygonOffset(0.5f, 0.1f);
						glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						pInfo->ZoomExtents(30.0f, aspect);
						pInfo->RenderPiece(Options.PartImagesColor);
						glFinish();

						Image image;
						image.FromOpenGL (cx, cy);

						sprintf(fn, "%s%s%s", Options.PathName, pInfo->m_strName, ext);
						image.FileSave(fn, Options.ImageFormat, Options.TransparentImages);
					}
					GL_EndRenderToTexture();
				}
			}
		} break;

		case LC_FILE_EXPORT_BRICKLINK:
		{
			if (mPieces.IsEmpty())
			{
				gMainWindow->DoMessageBox("Nothing to export.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			char FileName[LC_MAXPATH];
			memset(FileName, 0, sizeof(FileName));

			if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_BRICKLINK, FileName))
				break;

			lcDiskFile BrickLinkFile;
			char Line[1024];

			if (!BrickLinkFile.Open(FileName, "wt"))
			{
				gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
				break;
			}

			lcArray<lcPiecesUsedEntry> PiecesUsed;
			GetPiecesUsed(PiecesUsed);

			const char* OldLocale = setlocale(LC_NUMERIC, "C");
			BrickLinkFile.WriteLine("<INVENTORY>\n");

			for (int PieceIdx = 0; PieceIdx < PiecesUsed.GetSize(); PieceIdx++)
			{
				BrickLinkFile.WriteLine("  <ITEM>\n");
				BrickLinkFile.WriteLine("    <ITEMTYPE>P</ITEMTYPE>\n");

				sprintf(Line, "    <ITEMID>%s</ITEMID>\n", PiecesUsed[PieceIdx].Info->m_strName);
				BrickLinkFile.WriteLine(Line);

				int Count = PiecesUsed[PieceIdx].Count;
				if (Count > 1)
				{
					sprintf(Line, "    <MINQTY>%d</MINQTY>\n", Count);
					BrickLinkFile.WriteLine(Line);
				}

				int Color = lcGetBrickLinkColor(PiecesUsed[PieceIdx].ColorIndex);
				if (Color)
				{
					sprintf(Line, "    <COLOR>%d</COLOR>\n", Color);
					BrickLinkFile.WriteLine(Line);
				}

				BrickLinkFile.WriteLine("  </ITEM>\n");
			}

			BrickLinkFile.WriteLine("</INVENTORY>\n");

			setlocale(LC_NUMERIC, OldLocale);
		} break;

		case LC_FILE_EXPORT_CSV:
		{
			if (mPieces.IsEmpty())
			{
				gMainWindow->DoMessageBox("Nothing to export.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			char FileName[LC_MAXPATH];
			memset(FileName, 0, sizeof(FileName));

			if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_CSV, FileName))
				break;

			lcDiskFile CSVFile;
			char Line[1024];

			if (!CSVFile.Open(FileName, "wt"))
			{
				gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK | LC_MB_ICONERROR);
				break;
			}

			lcArray<lcPiecesUsedEntry> PiecesUsed;
			GetPiecesUsed(PiecesUsed);

			const char* OldLocale = setlocale(LC_NUMERIC, "C");
			CSVFile.WriteLine("Part Name,Color,Quantity,Part ID,Color Code\n");

			for (int PieceIdx = 0; PieceIdx < PiecesUsed.GetSize(); PieceIdx++)
			{
				sprintf(Line, "\"%s\",\"%s\",%d,%s,%d\n", PiecesUsed[PieceIdx].Info->m_strDescription, gColorList[PiecesUsed[PieceIdx].ColorIndex].Name,
						PiecesUsed[PieceIdx].Count, PiecesUsed[PieceIdx].Info->m_strName, gColorList[PiecesUsed[PieceIdx].ColorIndex].Code);
				CSVFile.WriteLine(Line);
			}

			setlocale(LC_NUMERIC, OldLocale);
		} break;

		case LC_FILE_EXPORT_POVRAY:
		{
			lcPOVRayDialogOptions Options;

			memset(Options.FileName, 0, sizeof(Options.FileName));
			strcpy(Options.POVRayPath, lcGetProfileString(LC_PROFILE_POVRAY_PATH));
			strcpy(Options.LGEOPath, lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH));
			Options.Render = lcGetProfileInt(LC_PROFILE_POVRAY_RENDER);

			if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_POVRAY, &Options))
				break;

			lcSetProfileString(LC_PROFILE_POVRAY_PATH, Options.POVRayPath);
			lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Options.LGEOPath);
			lcSetProfileInt(LC_PROFILE_POVRAY_RENDER, Options.Render);

			lcDiskFile POVFile;

			if (!POVFile.Open(Options.FileName, "wt"))
			{
				gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK|LC_MB_ICONERROR);
				break;
			}

			ExportPOVRay(POVFile);

			if (Options.Render)
			{
				lcArray<String> Arguments;
				char Argument[LC_MAXPATH + 32];

				sprintf(Argument, "+I%s", Options.FileName);
				Arguments.Add(Argument);

				if (Options.LGEOPath[0])
				{
					sprintf(Argument, "+L%slg/", Options.LGEOPath);
					Arguments.Add(Argument);
					sprintf(Argument, "+L%sar/", Options.LGEOPath);
					Arguments.Add(Argument);
				}

				sprintf(Argument, "+o%s", Options.FileName);
				char* Slash1 = strrchr(Argument, '\\');
				char* Slash2 = strrchr(Argument, '/');
				if (Slash1 || Slash2)
				{
					if (Slash1 > Slash2)
						*(Slash1 + 1) = 0;
					else
						*(Slash2 + 1) = 0;

					Arguments.Add(Argument);
				}

				g_App->RunProcess(Options.POVRayPath, Arguments);
			}
		} break;

		case LC_FILE_EXPORT_WAVEFRONT:
		{
			char FileName[LC_MAXPATH];
			memset(FileName, 0, sizeof(FileName));

			if (!gMainWindow->DoDialog(LC_DIALOG_EXPORT_WAVEFRONT, FileName))
				break;

			lcDiskFile OBJFile;
			char Line[1024];

			if (!OBJFile.Open(FileName, "wt"))
			{
				gMainWindow->DoMessageBox("Could not open file for writing.", LC_MB_OK|LC_MB_ICONERROR);
				break;
			}

			char buf[LC_MAXPATH], *ptr;
			lcuint32 vert = 1;

			const char* OldLocale = setlocale(LC_NUMERIC, "C");
			strcpy(buf, m_strPathName);
			ptr = strrchr(buf, '\\');
			if (ptr)
				ptr++;
			else
			{
				ptr = strrchr(buf, '/');
				if (ptr)
					ptr++;
				else
					ptr = buf;
			}

			OBJFile.WriteLine("# Model exported from LeoCAD\n");

			if (strlen(buf) != 0)
			{
				sprintf(Line, "# Original name: %s\n", ptr);
				OBJFile.WriteLine(Line);
			}

			if (!mProperties.mAuthor.IsEmpty())
			{
				sprintf(Line, "# Author: %s\n", mProperties.mAuthor.Buffer());
				OBJFile.WriteLine(Line);
			}

			strcpy(buf, FileName);
			ptr = strrchr(buf, '.');
			if (ptr)
				*ptr = 0;

			strcat(buf, ".mtl");
			ptr = strrchr(buf, '\\');
			if (ptr)
				ptr++;
			else
			{
				ptr = strrchr(buf, '/');
				if (ptr)
					ptr++;
				else
					ptr = buf;
			}

			sprintf(Line, "#\n\nmtllib %s\n\n", ptr);
			OBJFile.WriteLine(Line);

			FILE* mat = fopen(buf, "wt");
			fputs("# Colors used by LeoCAD\n# You need to add transparency values\n#\n\n", mat);
			for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
			{
				lcColor* Color = &gColorList[ColorIdx];
				fprintf(mat, "newmtl %s\nKd %.2f %.2f %.2f\n\n", Color->SafeName, Color->Value[0], Color->Value[1], Color->Value[2]);
			}
			fclose(mat);

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];
				const lcMatrix44& ModelWorld = Piece->mModelWorld;
				PieceInfo* pInfo = Piece->mPieceInfo;
				float* Verts = (float*)pInfo->mMesh->mVertexBuffer.mData;

				for (int i = 0; i < pInfo->mMesh->mNumVertices * 3; i += 3)
				{
					lcVector3 Vertex = lcMul31(lcVector3(Verts[i], Verts[i+1], Verts[i+2]), ModelWorld);
					sprintf(Line, "v %.2f %.2f %.2f\n", Vertex[0], Vertex[1], Vertex[2]);
					OBJFile.WriteLine(Line);
				}

				OBJFile.WriteLine("#\n\n");
			}

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];
				PieceInfo* Info = Piece->mPieceInfo;

				strcpy(buf, Piece->GetName());
				for (unsigned int i = 0; i < strlen(buf); i++)
					if ((buf[i] == '#') || (buf[i] == ' '))
						buf[i] = '_';

				sprintf(Line, "g %s\n", buf);
				OBJFile.WriteLine(Line);

				Info->mMesh->ExportWavefrontIndices(OBJFile, Piece->mColorCode, vert);
				vert += Info->mMesh->mNumVertices;
			}

			setlocale(LC_NUMERIC, OldLocale);
		} break;

		case LC_FILE_PROPERTIES:
		{
			lcPropertiesDialogOptions Options;

			Options.Properties = mProperties;
			Options.Title = m_strTitle;
			Options.SetDefault = false;

			GetPiecesUsed(Options.PartsUsed);

			if (!gMainWindow->DoDialog(LC_DIALOG_PROPERTIES, &Options))
				break;

			if (Options.SetDefault)
				Options.Properties.SaveDefaults();

			if (mProperties == Options.Properties)
				break;

			mProperties = Options.Properties;

			const lcArray<View*> Views = gMainWindow->GetViews();
			for (int i = 0; i < Views.GetSize (); i++)
			{
				Views[i]->MakeCurrent();
				RenderInitialize();
			}

			SetModifiedFlag(true);
			CheckPoint("Properties");
		} break;

		case LC_FILE_PRINT_PREVIEW:
			gMainWindow->TogglePrintPreview();
			break;

		case LC_FILE_PRINT:
			gMainWindow->DoDialog(LC_DIALOG_PRINT, NULL);
			break;

		// TODO: printing
		case LC_FILE_PRINT_BOM:
			break;

		case LC_FILE_TERRAIN_EDITOR:
		{
			// TODO: decide what to do with the terrain editor
//			Terrain temp = *m_pTerrain;

//			if (SystemDoDialog(LC_DLG_TERRAIN, temp))
//			{
//				*m_pTerrain = temp;
//				m_pTerrain->LoadTexture();
//			}
		} break;

	case LC_FILE_RECENT1:
	case LC_FILE_RECENT2:
	case LC_FILE_RECENT3:
	case LC_FILE_RECENT4:
		if (!OpenProject(gMainWindow->mRecentFiles[id - LC_FILE_RECENT1]))
			gMainWindow->RemoveRecentFile(id - LC_FILE_RECENT1);
		break;

	case LC_FILE_EXIT:
		gMainWindow->Close();
		break;

	case LC_EDIT_UNDO:
		{
			if (mUndoHistory.GetSize() < 2)
				break;

			lcModelHistoryEntry* Undo = mUndoHistory[0];
			mUndoHistory.RemoveIndex(0);
			mRedoHistory.InsertAt(0, Undo);

			DeleteContents(true);
			FileLoad(&mUndoHistory[0]->File, true, false);

			if (mUndoHistory.GetSize() == 1)
				SetModifiedFlag(false);

			gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : NULL, !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : NULL);
		}
		break;

	case LC_EDIT_REDO:
		{
			if (mRedoHistory.IsEmpty())
				break;

			lcModelHistoryEntry* Redo = mRedoHistory[0];
			mRedoHistory.RemoveIndex(0);
			mUndoHistory.InsertAt(0, Redo);

			DeleteContents(true);
			FileLoad(&Redo->File, true, false);

			SetModifiedFlag(true);

			gMainWindow->UpdateUndoRedo(mUndoHistory.GetSize() > 1 ? mUndoHistory[0]->Description : NULL, !mRedoHistory.IsEmpty() ? mRedoHistory[0]->Description : NULL);
		}
		break;

	case LC_EDIT_CUT:
	case LC_EDIT_COPY:
		{
			lcMemFile* Clipboard = new lcMemFile();

			int i = 0;
//			Light* pLight;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				if (mPieces[PieceIdx]->IsSelected())
					i++;
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
					Piece->FileSave(*Clipboard);
			}

			i = mGroups.GetSize();
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
				mGroups[GroupIdx]->FileSave(Clipboard, mGroups);

			i = 0;
			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				if (mCameras[CameraIdx]->IsSelected())
					i++;
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			{
				Camera* pCamera = mCameras[CameraIdx];

				if (pCamera->IsSelected())
					pCamera->FileSave(*Clipboard);
			}
/*
			for (i = 0, pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsSelected())
					i++;
			Clipboard->Write(&i, sizeof(i));

			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsSelected())
					pLight->FileSave(Clipboard);
*/
			if (id == LC_EDIT_CUT)
			{
				RemoveSelectedObjects();
				gMainWindow->UpdateFocusObject(GetFocusObject());
				UpdateSelection();
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Cutting");
			}

			g_App->ExportClipboard(Clipboard);
		} break;

		case LC_EDIT_PASTE:
		{
			lcFile* file = g_App->mClipboard;
			if (file == NULL)
				break;
			file->Seek(0, SEEK_SET);
			SelectAndFocusNone(false);

			lcArray<Piece*> PastedPieces;
			int NumPieces;
			file->ReadBuffer(&NumPieces, sizeof(NumPieces));

			while (NumPieces--)
			{
				lcPiece* Piece = new lcPiece(NULL);
				Piece->FileLoad(*file);
				PastedPieces.Add(Piece);
			}

			lcArray<lcGroup*> Groups;
			int NumGroups;
			file->ReadBuffer(&NumGroups, sizeof(NumGroups));

			while (NumGroups--)
			{
				lcGroup* Group = new lcGroup();
				Group->FileLoad(file);
				Groups.Add(Group);
			}

			for (int PieceIdx = 0; PieceIdx < PastedPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = PastedPieces[PieceIdx];
				Piece->CreateName(mPieces);
				Piece->SetStepShow(m_nCurStep);
				mPieces.Add(Piece);
				Piece->SetSelected(true);

				int GroupIndex = LC_POINTER_TO_INT(Piece->GetGroup());
				if (GroupIndex != -1)
					Piece->SetGroup(Groups[GroupIndex]);
				else
					Piece->SetGroup(NULL);
			}

			for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = Groups[GroupIdx];
				int GroupIndex = LC_POINTER_TO_INT(Group->mGroup);
				Group->mGroup = (GroupIndex != -1) ? Groups[GroupIndex] : NULL;
			}

			for (int GroupIdx = 0; GroupIdx < Groups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = Groups[GroupIdx];
				bool Add = false;

				for (int PieceIdx = 0; PieceIdx < PastedPieces.GetSize(); PieceIdx++)
				{
					lcPiece* Piece = PastedPieces[PieceIdx];
					
					for (lcGroup* PieceGroup = Piece->GetGroup(); PieceGroup; PieceGroup = PieceGroup->mGroup)
					{
						if (PieceGroup == Group)
						{
							Add = true;
							break;
						}
					}

					if (Add)
						break;
				}

				if (Add)
				{
					int a, max = 0;

					for (int SearchGroupIdx = 0; SearchGroupIdx < mGroups.GetSize(); SearchGroupIdx++)
					{
						lcGroup* SearchGroup = mGroups[SearchGroupIdx];

						if (strncmp("Pasted Group #", SearchGroup ->m_strName, 14) == 0)
							if (sscanf(SearchGroup ->m_strName + 14, "%d", &a) == 1)
								if (a > max)
									max = a;
					}

					sprintf(Group->m_strName, "Pasted Group #%.2d", max+1);
					mGroups.Add(Group);
				}
				else
					delete Group;
			}

			int NumCameras;
			file->ReadBuffer(&NumCameras, sizeof(NumCameras));

			while (NumCameras--)
			{
				Camera* pCamera = new Camera(false);
				pCamera->FileLoad(*file);
				pCamera->CreateName(mCameras);
				pCamera->SetSelected(true);
				mCameras.Add(pCamera);
			}

			// TODO: lights
			CalculateStep();
			SetModifiedFlag(true);
			CheckPoint("Pasting");
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_EDIT_FIND:
			if (gMainWindow->DoDialog(LC_DIALOG_FIND, &mSearchOptions))
				FindPiece(true, true);
			break;

		case LC_EDIT_FIND_NEXT:
			FindPiece(false, true);
			break;

		case LC_EDIT_FIND_PREVIOUS:
			FindPiece(false, false);
			break;

		case LC_EDIT_SELECT_ALL:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsVisible(m_nCurStep))
					Piece->SetSelected(true);
			}

			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_EDIT_SELECT_NONE:
		{
			SelectAndFocusNone(false);
			gMainWindow->UpdateFocusObject(NULL);
			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_EDIT_SELECT_INVERT:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsVisible(m_nCurStep))
					Piece->SetSelected(!Piece->IsSelected());
			}

			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_EDIT_SELECT_BY_NAME:
		{
			lcSelectDialogOptions Options;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				Options.Selection.Add(mPieces[PieceIdx]->IsSelected());

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				if (mCameras[CameraIdx]->IsVisible())
					Options.Selection.Add(mCameras[CameraIdx]->IsSelected());

			for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
				if (mLights[LightIdx]->IsVisible())
					Options.Selection.Add(mLights[LightIdx]->IsSelected());

			if (Options.Selection.GetSize() == 0)
			{
				gMainWindow->DoMessageBox("Nothing to select.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			if (!gMainWindow->DoDialog(LC_DIALOG_SELECT_BY_NAME, &Options))
				break;

			SelectAndFocusNone(false);

			int ObjectIndex = 0;
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					mPieces[PieceIdx]->SetSelected(true);

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					mCameras[CameraIdx]->SetSelected(true);

			for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
				if (Options.Selection[ObjectIndex])
					mLights[LightIdx]->SetSelected(true);

			UpdateSelection();
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdateFocusObject(GetFocusObject());
		} break;

		case LC_VIEW_SPLIT_HORIZONTAL:
			gMainWindow->SplitHorizontal();
			break;

		case LC_VIEW_SPLIT_VERTICAL:
			gMainWindow->SplitVertical();
			break;

		case LC_VIEW_REMOVE_VIEW:
			gMainWindow->RemoveView();
			break;

		case LC_VIEW_RESET_VIEWS:
			gMainWindow->ResetViews();
			break;

		case LC_VIEW_FULLSCREEN:
			gMainWindow->ToggleFullScreen();
			break;

	case LC_VIEW_PROJECTION_PERSPECTIVE:
		{
			View* ActiveView = gMainWindow->GetActiveView();
			lcCamera* Camera = ActiveView->mCamera;

			Camera->SetOrtho(false);

			if (Camera->IsFocused())
				gMainWindow->UpdateFocusObject(Camera);
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdatePerspective();
		}
		break;

	case LC_VIEW_PROJECTION_ORTHO:
		{
			View* ActiveView = gMainWindow->GetActiveView();
			lcCamera* Camera = ActiveView->mCamera;

			Camera->SetOrtho(true);

			if (Camera->IsFocused())
				gMainWindow->UpdateFocusObject(Camera);
			gMainWindow->UpdateAllViews();
			gMainWindow->UpdatePerspective();
		}
		break;

	case LC_VIEW_PROJECTION_CYCLE:
		{
			View* ActiveView = gMainWindow->GetActiveView();
			lcCamera* Camera = ActiveView->mCamera;

			Camera->SetOrtho(!Camera->IsOrtho());

			gMainWindow->UpdateAllViews();
			gMainWindow->UpdatePerspective();
		}
		break;

	case LC_VIEW_PROJECTION_FOCUS:
		{
			lcVector3 FocusVector;
			GetSelectionCenter(FocusVector);
			gMainWindow->GetActiveView()->mCamera->SetFocalPoint(FocusVector, m_nCurStep, gMainWindow->GetAddKeys());
			gMainWindow->UpdateAllViews();
		}
		break;

		case LC_PIECE_INSERT:
		{
			if (m_pCurPiece == NULL)
				break;
			Piece* Last = mPieces.IsEmpty() ? NULL : mPieces[mPieces.GetSize() - 1];
			Piece* pPiece = new Piece(m_pCurPiece);

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsFocused())
				{
					Last = Piece;
					break;
				}
			}

			if (Last != NULL)
			{
				lcVector3 Pos;
				lcVector4 Rot;

				GetPieceInsertPosition(Last, Pos, Rot);

				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_nCurStep);

				pPiece->ChangeKey(m_nCurStep, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_nCurStep);
			}
			else
				pPiece->Initialize(0, 0, 0, m_nCurStep);

			pPiece->SetColorIndex(gMainWindow->mColorIndex);
			pPiece->CreateName(mPieces);
			mPieces.Add(pPiece);
			ClearSelectionAndSetFocus(pPiece, LC_PIECE_SECTION_POSITION);
			SystemPieceComboAdd(m_pCurPiece->m_strDescription);

			SetModifiedFlag(true);
			CheckPoint("Inserting");
		} break;

		case LC_PIECE_DELETE:
		{
			if (RemoveSelectedObjects())
			{
				gMainWindow->UpdateFocusObject(NULL);
				UpdateSelection();
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Deleting");
			}
		} break;

		case LC_PIECE_MOVE_PLUSX:
		case LC_PIECE_MOVE_MINUSX:
		case LC_PIECE_MOVE_PLUSY:
		case LC_PIECE_MOVE_MINUSY:
		case LC_PIECE_MOVE_PLUSZ:
		case LC_PIECE_MOVE_MINUSZ:
		case LC_PIECE_ROTATE_PLUSX:
		case LC_PIECE_ROTATE_MINUSX:
		case LC_PIECE_ROTATE_PLUSY:
		case LC_PIECE_ROTATE_MINUSY:
		case LC_PIECE_ROTATE_PLUSZ:
		case LC_PIECE_ROTATE_MINUSZ:
		{
			lcVector3 axis;
			bool Rotate = id >= LC_PIECE_ROTATE_PLUSX && id <= LC_PIECE_ROTATE_MINUSZ;

			if (Rotate)
			{
				if (m_nSnap & LC_DRAW_SNAP_A)
					axis[0] = axis[1] = axis[2] = m_nAngleSnap;
				else
					axis[0] = axis[1] = axis[2] = 1;
			}
			else
			{
				float xy, z;
				GetSnapDistance(&xy, &z);

				axis[0] = axis[1] = xy;
				axis[2] = z;

				if ((m_nSnap & LC_DRAW_SNAP_X) == 0)// || bControl)
					axis[0] = 0.01f;
				if ((m_nSnap & LC_DRAW_SNAP_Y) == 0)// || bControl)
					axis[1] = 0.01f;
				if ((m_nSnap & LC_DRAW_SNAP_Z) == 0)// || bControl)
					axis[2] = 0.01f;
			}

			if (id == LC_PIECE_MOVE_PLUSX || id ==  LC_PIECE_ROTATE_PLUSX)
				axis = lcVector3(axis[0], 0, 0);
			else if (id == LC_PIECE_MOVE_MINUSX || id == LC_PIECE_ROTATE_MINUSX)
				axis = lcVector3(-axis[0], 0, 0);
			else if (id == LC_PIECE_MOVE_PLUSY || id == LC_PIECE_ROTATE_PLUSY)
				axis = lcVector3(0, axis[1], 0);
			else if (id == LC_PIECE_MOVE_MINUSY || id == LC_PIECE_ROTATE_MINUSY)
				axis = lcVector3(0, -axis[1], 0);
			else if (id == LC_PIECE_MOVE_PLUSZ || id == LC_PIECE_ROTATE_PLUSZ)
				axis = lcVector3(0, 0, axis[2]);
			else if (id == LC_PIECE_MOVE_MINUSZ || id == LC_PIECE_ROTATE_MINUSZ)
				axis = lcVector3(0, 0, -axis[2]);

			if ((m_nSnap & LC_DRAW_MOVEAXIS) == 0)
			{
				// TODO: rewrite this
			
				lcVector3 Pts[3] = { lcVector3(5.0f, 5.0f, 0.1f), lcVector3(10.0f, 5.0f, 0.1f), lcVector3(5.0f, 10.0f, 0.1f) };
				gMainWindow->GetActiveView()->UnprojectPoints(Pts, 3);

				float ax, ay;
				lcVector3 vx((Pts[1][0] - Pts[0][0]), (Pts[1][1] - Pts[0][1]), 0);//Pts[1][2] - Pts[0][2] };
				vx.Normalize();
				lcVector3 x(1, 0, 0);
				ax = acosf(lcDot(vx, x));

				lcVector3 vy((Pts[2][0] - Pts[0][0]), (Pts[2][1] - Pts[0][1]), 0);//Pts[2][2] - Pts[0][2] };
				vy.Normalize();
				lcVector3 y(0, -1, 0);
				ay = acosf(lcDot(vy, y));

				if (ax > 135)
					axis[0] = -axis[0];

				if (ay < 45)
					axis[1] = -axis[1];

				if (ax >= 45 && ax <= 135)
				{
					float tmp = axis[0];

					ax = acosf(lcDot(vx, y));
					if (ax > 90)
					{
						axis[0] = -axis[1];
						axis[1] = tmp;
					}
					else
					{
						axis[0] = axis[1];
						axis[1] = -tmp;
					}
				}
			}

			if (Rotate)
			{
				lcVector3 tmp;
				RotateSelectedObjects(axis, tmp, true, true);
			}
			else
			{
				lcVector3 tmp;
				MoveSelectedObjects(axis, tmp, false, true);
			}

			gMainWindow->UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint(Rotate ? "Rotating" : "Moving");
			gMainWindow->UpdateFocusObject(GetFocusObject());
		} break;

		case LC_PIECE_MINIFIG_WIZARD:
		{
			lcMinifig Minifig;
			int i;

			if (!gMainWindow->DoDialog(LC_DIALOG_MINIFIG, &Minifig))
				break;

			SelectAndFocusNone(false);

			for (i = 0; i < LC_MFW_NUMITEMS; i++)
			{
				if (Minifig.Parts[i] == NULL)
					continue;

				Piece* pPiece = new Piece(Minifig.Parts[i]);

				lcVector4& Position = Minifig.Matrices[i][3];
				lcVector4 Rotation = lcMatrix44ToAxisAngle(Minifig.Matrices[i]);
				Rotation[3] *= LC_RTOD;
				pPiece->Initialize(Position[0], Position[1], Position[2], m_nCurStep);
				pPiece->SetColorIndex(Minifig.Colors[i]);
				pPiece->CreateName(mPieces);
				mPieces.Add(pPiece);
				pPiece->SetSelected(true);

				pPiece->ChangeKey(1, false, Rotation, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_nCurStep);

				SystemPieceComboAdd(Minifig.Parts[i]->m_strDescription);
			}

			int Max = 0;

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = mGroups[GroupIdx];

				if (strncmp(Group->m_strName, "Minifig #", 9) == 0)
					if (sscanf(Group->m_strName, "Minifig #%d", &i) == 1)
						if (i > Max)
							Max = i;
			}

			lcGroup* Group = AddGroup(NULL);
			sprintf(Group->m_strName, "Minifig #%.2d", Max+1);

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
					Piece->SetGroup(Group);
			}

			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			gMainWindow->UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Minifig");
		} break;

		case LC_PIECE_ARRAY:
		{
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			int sel = 0;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					Piece->CompareBoundingBox(bs);
					sel++;
				}
			}

			if (!sel)
			{
				gMainWindow->DoMessageBox("No pieces selected.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			lcArrayDialogOptions Options;

			memset(&Options, 0, sizeof(Options));
			Options.Counts[0] = 10;
			Options.Counts[1] = 1;
			Options.Counts[2] = 1;

			if (!gMainWindow->DoDialog(LC_DIALOG_PIECE_ARRAY, &Options))
				break;

			if (Options.Counts[0] * Options.Counts[1] * Options.Counts[2] < 2)
			{
				gMainWindow->DoMessageBox("Array only has 1 element or less, no pieces added.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			ConvertFromUserUnits(Options.Offsets[0]);
			ConvertFromUserUnits(Options.Offsets[1]);
			ConvertFromUserUnits(Options.Offsets[2]);

			lcArray<Piece*> NewPieces;

			for (int Step1 = 0; Step1 < Options.Counts[0]; Step1++)
			{
				for (int Step2 = 0; Step2 < Options.Counts[1]; Step2++)
				{
					for (int Step3 = 0; Step3 < Options.Counts[2]; Step3++)
					{
						if (Step1 == 0 && Step2 == 0 && Step3 == 0)
							continue;

						lcMatrix44 ModelWorld;
						lcVector3 Position;

						lcVector3 RotationAngles = Options.Rotations[0] * Step1 + Options.Rotations[1] * Step2 + Options.Rotations[2] * Step3;
						lcVector3 Offset = Options.Offsets[0] * Step1 + Options.Offsets[1] * Step2 + Options.Offsets[2] * Step3;

						for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
						{
							Piece* pPiece = mPieces[PieceIdx];

							if (!pPiece->IsSelected())
								continue;

							if (sel == 1)
							{
								ModelWorld = lcMul(pPiece->mModelWorld, lcMatrix44RotationX(RotationAngles[0] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationY(RotationAngles[1] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationZ(RotationAngles[2] * LC_DTOR));

								Position = pPiece->mPosition;
							}
							else
							{
								lcVector4 Center((bs[0] + bs[3]) / 2, (bs[1] + bs[4]) / 2, (bs[2] + bs[5]) / 2, 0.0f);
								ModelWorld = pPiece->mModelWorld;

								ModelWorld.r[3] -= Center;
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationX(RotationAngles[0] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationY(RotationAngles[1] * LC_DTOR));
								ModelWorld = lcMul(ModelWorld, lcMatrix44RotationZ(RotationAngles[2] * LC_DTOR));
								ModelWorld.r[3] += Center;

								Position = lcVector3(ModelWorld.r[3].x, ModelWorld.r[3].y, ModelWorld.r[3].z);
							}

							lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
							AxisAngle[3] *= LC_RTOD;

							Piece* NewPiece = new Piece(pPiece->mPieceInfo);
							NewPieces.Add(NewPiece);

							NewPiece->Initialize(Position[0] + Offset[0], Position[1] + Offset[1], Position[2] + Offset[2], m_nCurStep);
							NewPiece->SetColorIndex(pPiece->mColorIndex);
							NewPiece->ChangeKey(1, false, AxisAngle, LC_PK_ROTATION);
						}
					}
				}
			}

			for (int PieceIdx = 0; PieceIdx < NewPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = NewPieces[PieceIdx];
				Piece->CreateName(mPieces);
				Piece->UpdatePosition(m_nCurStep);
				mPieces.Add(Piece);
			}

			SelectAndFocusNone(true);
//			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			gMainWindow->UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Array");
		} break;

		case LC_PIECE_GROUP:
		{
			int i, Max = 0;
			char name[65];
			int Selected = 0;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				if (mPieces[PieceIdx]->IsSelected())
				{
					Selected++;

					if (Selected > 1)
						break;
				}
			}

			if (!Selected)
			{
				gMainWindow->DoMessageBox("No pieces selected.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = mGroups[GroupIdx];

				if (strncmp(Group->m_strName, "Group #", 7) == 0)
					if (sscanf(Group->m_strName, "Group #%d", &i) == 1)
						if (i > Max)
							Max = i;
			}

			sprintf(name, "Group #%.2d", Max + 1);

			if (!gMainWindow->DoDialog(LC_DIALOG_PIECE_GROUP, name))
				break;

			lcGroup* NewGroup = new lcGroup();
			strcpy(NewGroup->m_strName, name);
			mGroups.Add(NewGroup);

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					lcGroup* Group = Piece->GetTopGroup();

					if (!Group)
						Piece->SetGroup(NewGroup);
					else if (Group != NewGroup)
						Group->mGroup = NewGroup;
				}
			}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Grouping");
		} break;

		case LC_PIECE_UNGROUP:
		{
			lcArray<lcGroup*> Groups;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					lcGroup* Group = Piece->GetTopGroup();

					if (Groups.FindIndex(Group) == -1)
					{
						mGroups.Remove(Group);
						Groups.Add(Group);
					}
				}
			}

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				lcPiece* Piece = mPieces[PieceIdx];
				lcGroup* Group = Piece->GetGroup();

				if (Groups.FindIndex(Group) != -1)
					Piece->SetGroup(NULL);
			}

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
			{
				lcGroup* Group = mGroups[GroupIdx];

				if (Groups.FindIndex(Group->mGroup) != -1)
					Group->mGroup = NULL;
			}

			Groups.DeleteAll();

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Ungrouping");
		} break;

		case LC_PIECE_GROUP_ADD:
		{
			Group* pGroup = NULL;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					pGroup = Piece->GetTopGroup();
					if (pGroup != NULL)
						break;
				}
			}

			if (pGroup != NULL)
			{
				for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				{
					Piece* Piece = mPieces[PieceIdx];

					if (Piece->IsFocused())
					{
						Piece->SetGroup(pGroup);
						break;
					}
				}
			}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Grouping");
		} break;

		case LC_PIECE_GROUP_REMOVE:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsFocused())
				{
					Piece->SetGroup(NULL);
					break;
				}
			}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Ungrouping");
		} break;

		case LC_PIECE_GROUP_EDIT:
		{
			lcEditGroupsDialogOptions Options;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				Options.PieceParents.Add(mPieces[PieceIdx]->GetGroup());

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
				Options.GroupParents.Add(mGroups[GroupIdx]->mGroup);

			if (!gMainWindow->DoDialog(LC_DIALOG_EDIT_GROUPS, &Options))
				break;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				mPieces[PieceIdx]->SetGroup(Options.PieceParents[PieceIdx]);

			for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
				mGroups[GroupIdx]->mGroup = Options.GroupParents[GroupIdx];

			RemoveEmptyGroups();
			SelectAndFocusNone(false);
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			gMainWindow->UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Editing");
		} break;

		case LC_PIECE_HIDE_SELECTED:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					Piece->SetHidden(true);
					Piece->SetSelected(false);
				}
			}

			UpdateSelection();
			gMainWindow->UpdateFocusObject(NULL);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_PIECE_HIDE_UNSELECTED:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (!Piece->IsSelected())
					Piece->SetHidden(true);
			}

			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_PIECE_UNHIDE_ALL:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				mPieces[PieceIdx]->SetHidden(false);
			UpdateSelection();
			gMainWindow->UpdateAllViews();
		} break;

		case LC_PIECE_SHOW_EARLIER:
		{
			bool redraw = false;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					unsigned char t = Piece->GetStepShow();
					if (t > 1)
					{
						redraw = true;
						Piece->SetStepShow(t-1);
					}
				}
			}

			if (redraw)
			{
				SetModifiedFlag(true);
				CheckPoint("Modifying");
				gMainWindow->UpdateAllViews();
			}
		} break;

		case LC_PIECE_SHOW_LATER:
		{
			bool redraw = false;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					unsigned char t = Piece->GetStepShow();
					if (t < 255)
					{
						redraw = true;
						Piece->SetStepShow(t+1);

						if (t == m_nCurStep)
							Piece->SetSelected(false);
					}
				}
			}

			if (redraw)
			{
				SetModifiedFlag(true);
				CheckPoint("Modifying");
				gMainWindow->UpdateAllViews();
				UpdateSelection ();
			}
		} break;

		case LC_VIEW_PREFERENCES:
		{
			g_App->ShowPreferencesDialog();

			const lcArray<View*> Views = gMainWindow->GetViews();
			for (int i = 0; i < Views.GetSize (); i++)
			{
				Views[i]->MakeCurrent();
				RenderInitialize(); // TODO: get rid of RenderInitialize(), most of it can be done once per frame
			}
		} break;

		case LC_VIEW_ZOOM_IN:
		{
			ZoomActiveView(-1);
		} break;

		case LC_VIEW_ZOOM_OUT:
		{
			ZoomActiveView(1);
		} break;

		case LC_VIEW_ZOOM_EXTENTS:
		{
			int FirstView, LastView;

			// TODO: Find a way to let users zoom extents all views
//			if (Sys_KeyDown(KEY_CONTROL))
//			{
//				FirstView = 0;
//				LastView = m_ViewList.GetSize();
//			}
//			else
			{
				FirstView = gMainWindow->GetViews().FindIndex(gMainWindow->GetActiveView());
				LastView = FirstView + 1;
			}

			ZoomExtents(FirstView, LastView);
		} break;

		case LC_VIEW_LOOK_AT:
		{
			lcVector3 Center;

			if (!GetSelectionCenter(Center))
			{
				float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

				for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
					mPieces[PieceIdx]->CompareBoundingBox(bs);

				Center = lcVector3((bs[0] + bs[3]) * 0.5f, (bs[1] + bs[4]) * 0.5f, (bs[2] + bs[5]) * 0.5f);
			}

			gMainWindow->GetActiveView()->mCamera->Center(Center, m_nCurStep, gMainWindow->GetAddKeys());
			gMainWindow->UpdateAllViews();
			break;
		}

		case LC_VIEW_TIME_NEXT:
		{
			m_nCurStep++;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();

			gMainWindow->UpdateTime(m_nCurStep, 255);
		} break;

		case LC_VIEW_TIME_PREVIOUS:
		{
			m_nCurStep--;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();

			gMainWindow->UpdateTime(m_nCurStep, 255);
		} break;

		case LC_VIEW_TIME_FIRST:
		{
			m_nCurStep = 1;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();

			gMainWindow->UpdateTime(m_nCurStep, 255);
		} break;

		case LC_VIEW_TIME_LAST:
		{
			m_nCurStep = GetLastStep ();

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();

			gMainWindow->UpdateTime(m_nCurStep, 255);
		} break;

		case LC_VIEW_TIME_INSERT:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];
				Piece->InsertTime(m_nCurStep, 1);
				if (Piece->IsSelected() && !Piece->IsVisible(m_nCurStep))
					Piece->SetSelected(false);
			}

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				mCameras[CameraIdx]->InsertTime(m_nCurStep, 1);

			for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
				mLights[LightIdx]->InsertTime(m_nCurStep, 1);

			SetModifiedFlag(true);
			CheckPoint("Adding Step");
			CalculateStep();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_TIME_DELETE:
		{
			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];
				Piece->RemoveTime(m_nCurStep, 1);
				if (Piece->IsSelected() && !Piece->IsVisible(m_nCurStep))
					Piece->SetSelected(false);
			}

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				mCameras[CameraIdx]->RemoveTime(m_nCurStep, 1);

			for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
				mLights[LightIdx]->RemoveTime(m_nCurStep, 1);

			SetModifiedFlag(true);
			CheckPoint("Removing Step");
			CalculateStep();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_VIEWPOINT_FRONT:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_FRONT, m_nCurStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_BACK:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_BACK, m_nCurStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_TOP:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_TOP, m_nCurStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_BOTTOM:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_BOTTOM, m_nCurStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_LEFT:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_LEFT, m_nCurStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_RIGHT:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_RIGHT, m_nCurStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPOINT_HOME:
		{
			gMainWindow->GetActiveView()->mCamera->SetViewpoint(LC_VIEWPOINT_HOME, m_nCurStep, gMainWindow->GetAddKeys());
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_CAMERA_NONE:
		case LC_VIEW_CAMERA1:
		case LC_VIEW_CAMERA2:
		case LC_VIEW_CAMERA3:
		case LC_VIEW_CAMERA4:
		case LC_VIEW_CAMERA5:
		case LC_VIEW_CAMERA6:
		case LC_VIEW_CAMERA7:
		case LC_VIEW_CAMERA8:
		case LC_VIEW_CAMERA9:
		case LC_VIEW_CAMERA10:
		case LC_VIEW_CAMERA11:
		case LC_VIEW_CAMERA12:
		case LC_VIEW_CAMERA13:
		case LC_VIEW_CAMERA14:
		case LC_VIEW_CAMERA15:
		case LC_VIEW_CAMERA16:
		{
			View* ActiveView = gMainWindow->GetActiveView();
			lcCamera* Camera = NULL;

			if (id == LC_VIEW_CAMERA_NONE)
			{
				Camera = ActiveView->mCamera;

				if (!Camera->IsSimple())
				{
					ActiveView->SetCamera(Camera, true);
					Camera = ActiveView->mCamera;
				}
			}
			else
			{
				if (id - LC_VIEW_CAMERA1 < mCameras.GetSize())
				{
					Camera = mCameras[id - LC_VIEW_CAMERA1];
					ActiveView->SetCamera(Camera, false);
				}
				else
					break;
			}

			gMainWindow->UpdateCurrentCamera(mCameras.FindIndex(ActiveView->mCamera));
			gMainWindow->UpdateAllViews();
		} break;

		case LC_VIEW_CAMERA_RESET:
		{
			const lcArray<View*> Views = gMainWindow->GetViews();
			for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
				Views[ViewIdx]->SetDefaultCamera();

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				delete mCameras[CameraIdx];
			mCameras.RemoveAll();

			gMainWindow->UpdateCameraMenu();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			gMainWindow->UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Reset Cameras");
		} break;

		case LC_HELP_HOMEPAGE:
			g_App->OpenURL("http://www.leocad.org/");
			break;

		case LC_HELP_EMAIL:
			g_App->OpenURL("mailto:leozide@gmail.com?subject=LeoCAD");
			break;

		case LC_HELP_UPDATES:
			gMainWindow->DoDialog(LC_DIALOG_CHECK_UPDATES, NULL);
			break;

		case LC_HELP_ABOUT:
		{
			String Info;
			char Text[256];

			gMainWindow->GetActiveView()->MakeCurrent();

			GLint Red, Green, Blue, Alpha, Depth, Stencil;
			GLboolean DoubleBuffer, RGBA;

			glGetIntegerv(GL_RED_BITS, &Red);
			glGetIntegerv(GL_GREEN_BITS, &Green);
			glGetIntegerv(GL_BLUE_BITS, &Blue);
			glGetIntegerv(GL_ALPHA_BITS, &Alpha);
			glGetIntegerv(GL_DEPTH_BITS, &Depth);
			glGetIntegerv(GL_STENCIL_BITS, &Stencil);
			glGetBooleanv(GL_DOUBLEBUFFER, &DoubleBuffer);
			glGetBooleanv(GL_RGBA_MODE, &RGBA);

			Info = "OpenGL Version ";
			Info += (const char*)glGetString(GL_VERSION);
			Info += "\n";
			Info += (const char*)glGetString(GL_RENDERER);
			Info += " - ";
			Info += (const char*)glGetString(GL_VENDOR);
			sprintf(Text, "\n\nColor Buffer: %d bits %s %s", Red + Green + Blue + Alpha, RGBA ? "RGBA" : "indexed", DoubleBuffer ? "double buffered" : "");
			Info += Text;
			sprintf(Text, "\nDepth Buffer: %d bits", Depth);
			Info += Text;
			sprintf(Text, "\nStencil Buffer: %d bits", Stencil);
			Info += Text;
			Info += "\nGL_ARB_vertex_buffer_object extension: ";
			Info += GL_HasVertexBufferObject() ? "supported" : "not supported";
			Info += "\nGL_ARB_framebuffer_object extension: ";
			Info += GL_HasFramebufferObjectARB() ? "supported" : "not supported";
			Info += "\nGL_EXT_framebuffer_object extension: ";
			Info += GL_HasFramebufferObjectEXT() ? "supported" : "not supported";
			Info += "\nGL_EXT_texture_filter_anisotropic extension: ";
			if (GL_SupportsAnisotropic)
			{
				sprintf(Text, "supported (max %d)", (int)GL_MaxAnisotropy);
				Info += Text;
			}
			else
				Info += "not supported";

			gMainWindow->DoDialog(LC_DIALOG_ABOUT, (char*)Info);
		} break;

		case LC_VIEW_TIME_ADD_KEYS:
			gMainWindow->SetAddKeys(!gMainWindow->GetAddKeys());
			break;

		case LC_EDIT_SNAP_RELATIVE:
			if (m_nSnap & LC_DRAW_GLOBAL_SNAP)
				m_nSnap &= ~LC_DRAW_GLOBAL_SNAP;
			else
				m_nSnap |= LC_DRAW_GLOBAL_SNAP;
			gMainWindow->UpdateLockSnap(m_nSnap);
			gMainWindow->UpdateAllViews();
			break;

		case LC_EDIT_SNAP_X:
			if (m_nSnap & LC_DRAW_SNAP_X)
				m_nSnap &= ~LC_DRAW_SNAP_X;
			else
				m_nSnap |= LC_DRAW_SNAP_X;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_SNAP_Y:
			if (m_nSnap & LC_DRAW_SNAP_Y)
				m_nSnap &= ~LC_DRAW_SNAP_Y;
			else
				m_nSnap |= LC_DRAW_SNAP_Y;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_SNAP_Z:
			if (m_nSnap & LC_DRAW_SNAP_Z)
				m_nSnap &= ~LC_DRAW_SNAP_Z;
			else
				m_nSnap |= LC_DRAW_SNAP_Z;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_SNAP_ALL:
				m_nSnap |= LC_DRAW_SNAP_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
				break;

		case LC_EDIT_SNAP_NONE:
			m_nSnap &= ~LC_DRAW_SNAP_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_SNAP_TOGGLE:
			if ((m_nSnap & LC_DRAW_SNAP_XYZ) == LC_DRAW_SNAP_XYZ)
				m_nSnap &= ~LC_DRAW_SNAP_XYZ;
			else
				m_nSnap |= LC_DRAW_SNAP_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_LOCK_X:
			if (m_nSnap & LC_DRAW_LOCK_X)
				m_nSnap &= ~LC_DRAW_LOCK_X;
			else
				m_nSnap |= LC_DRAW_LOCK_X;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_LOCK_Y:
			if (m_nSnap & LC_DRAW_LOCK_Y)
				m_nSnap &= ~LC_DRAW_LOCK_Y;
			else
				m_nSnap |= LC_DRAW_LOCK_Y;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_LOCK_Z:
			if (m_nSnap & LC_DRAW_LOCK_Z)
				m_nSnap &= ~LC_DRAW_LOCK_Z;
			else
				m_nSnap |= LC_DRAW_LOCK_Z;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_LOCK_NONE:
			m_nSnap &= ~LC_DRAW_LOCK_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_LOCK_TOGGLE:
			if ((m_nSnap & LC_DRAW_LOCK_XYZ) == LC_DRAW_LOCK_XYZ)
				m_nSnap &= ~LC_DRAW_LOCK_XYZ;
			else
				m_nSnap |= LC_DRAW_LOCK_XYZ;
			gMainWindow->UpdateLockSnap(m_nSnap);
			break;

		case LC_EDIT_SNAP_MOVE_XY0:
		case LC_EDIT_SNAP_MOVE_XY1:
		case LC_EDIT_SNAP_MOVE_XY2:
		case LC_EDIT_SNAP_MOVE_XY3:
		case LC_EDIT_SNAP_MOVE_XY4:
		case LC_EDIT_SNAP_MOVE_XY5:
		case LC_EDIT_SNAP_MOVE_XY6:
		case LC_EDIT_SNAP_MOVE_XY7:
		case LC_EDIT_SNAP_MOVE_XY8:
		case LC_EDIT_SNAP_MOVE_XY9:
		{
			m_nMoveSnap = (id - LC_EDIT_SNAP_MOVE_XY0) | (m_nMoveSnap & ~0xff);
			if (id != LC_EDIT_SNAP_MOVE_XY0)
				m_nSnap |= LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y;
			else
				m_nSnap &= ~(LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y);
			gMainWindow->UpdateSnap();
		} break;

		case LC_EDIT_SNAP_MOVE_Z0:
		case LC_EDIT_SNAP_MOVE_Z1:
		case LC_EDIT_SNAP_MOVE_Z2:
		case LC_EDIT_SNAP_MOVE_Z3:
		case LC_EDIT_SNAP_MOVE_Z4:
		case LC_EDIT_SNAP_MOVE_Z5:
		case LC_EDIT_SNAP_MOVE_Z6:
		case LC_EDIT_SNAP_MOVE_Z7:
		case LC_EDIT_SNAP_MOVE_Z8:
		case LC_EDIT_SNAP_MOVE_Z9:
		{
			m_nMoveSnap = (((id - LC_EDIT_SNAP_MOVE_Z0) << 8) | (m_nMoveSnap & ~0xff00));
			if (id != LC_EDIT_SNAP_MOVE_Z0)
				m_nSnap |= LC_DRAW_SNAP_Z;
			else
				m_nSnap &= ~LC_DRAW_SNAP_Z;
			gMainWindow->UpdateSnap();
		} break;

		case LC_EDIT_SNAP_ANGLE:
			if (m_nSnap & LC_DRAW_SNAP_A)
				m_nSnap &= ~LC_DRAW_SNAP_A;
			else
		{
				m_nSnap |= LC_DRAW_SNAP_A;
				m_nAngleSnap = lcMax(1, m_nAngleSnap);
			}
			gMainWindow->UpdateSnap();
			break;

		case LC_EDIT_SNAP_ANGLE0:
		case LC_EDIT_SNAP_ANGLE1:
		case LC_EDIT_SNAP_ANGLE2:
		case LC_EDIT_SNAP_ANGLE3:
		case LC_EDIT_SNAP_ANGLE4:
		case LC_EDIT_SNAP_ANGLE5:
		case LC_EDIT_SNAP_ANGLE6:
		case LC_EDIT_SNAP_ANGLE7:
		case LC_EDIT_SNAP_ANGLE8:
		case LC_EDIT_SNAP_ANGLE9:
		{
			const int Angles[] = { 0, 1, 5, 10, 15, 30, 45, 60, 90, 180 };

			if (id == LC_EDIT_SNAP_ANGLE0)
				m_nSnap &= ~LC_DRAW_SNAP_A;
			else
			{
				m_nSnap |= LC_DRAW_SNAP_A;
				m_nAngleSnap = Angles[id - LC_EDIT_SNAP_ANGLE0];
			}
			gMainWindow->UpdateSnap();
		} break;

		case LC_EDIT_TRANSFORM:
			TransformSelectedObjects((LC_TRANSFORM_TYPE)mTransformType, gMainWindow->GetTransformAmount());
			break;

		case LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION:
		case LC_EDIT_TRANSFORM_RELATIVE_TRANSLATION:
		case LC_EDIT_TRANSFORM_ABSOLUTE_ROTATION:
		case LC_EDIT_TRANSFORM_RELATIVE_ROTATION:
			mTransformType = id - LC_EDIT_TRANSFORM_ABSOLUTE_TRANSLATION;
			gMainWindow->UpdateTransformType(mTransformType);
			break;

		case LC_EDIT_ACTION_SELECT:
			gMainWindow->SetTool(LC_TOOL_SELECT);
			break;

		case LC_EDIT_ACTION_INSERT:
			gMainWindow->SetTool(LC_TOOL_INSERT);
			break;

		case LC_EDIT_ACTION_LIGHT:
			gMainWindow->SetTool(LC_TOOL_LIGHT);
			break;

		case LC_EDIT_ACTION_SPOTLIGHT:
			gMainWindow->SetTool(LC_TOOL_SPOTLIGHT);
			break;

		case LC_EDIT_ACTION_CAMERA:
			gMainWindow->SetTool(LC_TOOL_CAMERA);
			break;

		case LC_EDIT_ACTION_MOVE:
			gMainWindow->SetTool(LC_TOOL_MOVE);
			break;

		case LC_EDIT_ACTION_ROTATE:
			gMainWindow->SetTool(LC_TOOL_ROTATE);
			break;

		case LC_EDIT_ACTION_DELETE:
			gMainWindow->SetTool(LC_TOOL_ERASER);
			break;

		case LC_EDIT_ACTION_PAINT:
			gMainWindow->SetTool(LC_TOOL_PAINT);
			break;

		case LC_EDIT_ACTION_ZOOM:
			gMainWindow->SetTool(LC_TOOL_ZOOM);
			break;

		case LC_EDIT_ACTION_ZOOM_REGION:
			gMainWindow->SetTool(LC_TOOL_ZOOM_REGION);
			break;

		case LC_EDIT_ACTION_PAN:
			gMainWindow->SetTool(LC_TOOL_PAN);
			break;

		case LC_EDIT_ACTION_ROTATE_VIEW:
			gMainWindow->SetTool(LC_TOOL_ROTATE_VIEW);
			break;

		case LC_EDIT_ACTION_ROLL:
			gMainWindow->SetTool(LC_TOOL_ROLL);
			break;

		case LC_EDIT_CANCEL:
		{
			View* ActiveView = gMainWindow->GetActiveView();
			if (ActiveView && ActiveView->mTrackButton != LC_TRACKBUTTON_NONE)
				ActiveView->StopTracking(false);
			else
			{
				SelectAndFocusNone(false);
				UpdateSelection();
				gMainWindow->UpdateAllViews();
				gMainWindow->UpdateFocusObject(NULL);
			}
		} break;

		case LC_NUM_COMMANDS:
			break;
	}
}

// Remove unused groups
void Project::RemoveEmptyGroups()
{
	bool Removed;

	do
	{
		Removed = false;

		for (int GroupIdx = 0; GroupIdx < mGroups.GetSize();)
		{
			lcGroup* Group = mGroups[GroupIdx];
			int Ref = 0;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				if (mPieces[PieceIdx]->GetGroup() == Group)
					Ref++;

			for (int ParentIdx = 0; ParentIdx < mGroups.GetSize(); ParentIdx++)
				if (mGroups[ParentIdx]->mGroup == Group)
					Ref++;

			if (Ref > 1)
			{
				GroupIdx++;
				continue;
			}

			if (Ref != 0)
			{
				for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
				{
					lcPiece* Piece = mPieces[PieceIdx];

					if (Piece->GetGroup() == Group)
					{
						Piece->SetGroup(Group->mGroup);
						break;
					}
				}

				for (int ParentIdx = 0; ParentIdx < mGroups.GetSize(); ParentIdx++)
				{
					if (mGroups[ParentIdx]->mGroup == Group)
					{
						mGroups[ParentIdx]->mGroup = Group->mGroup;
						break;
					}
				}
			}

			mGroups.RemoveIndex(GroupIdx);
			delete Group;
			Removed = true;
		}
	}
	while (Removed);
}

lcGroup* Project::AddGroup(lcGroup* Parent)
{
	lcGroup* NewGroup = new lcGroup();

	int i, Max = 0;

	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		lcGroup* Group = mGroups[GroupIdx];

		if (strncmp(Group->m_strName, "Group #", 7) == 0)
			if (sscanf(Group->m_strName, "Group #%d", &i) == 1)
				if (i > Max)
					Max = i;
	}

	sprintf(NewGroup->m_strName, "Group #%.2d", Max + 1);
	mGroups.Add(NewGroup);

	NewGroup->mGroup = Parent;

	return NewGroup;
}

void Project::SelectAndFocusNone(bool FocusOnly)
{
	if (FocusOnly)
	{
		Object* FocusObject = GetFocusObject();

		if (FocusObject)
			FocusObject->SetFocused(FocusObject->GetFocusSection(), false);
	}
	else
	{
		for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			mPieces[PieceIdx]->SetSelected(false);

		for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			mCameras[CameraIdx]->SetSelected(false);

		for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
			mLights[LightIdx]->SetSelected(false);
	}
}

void Project::SelectGroup(Group* TopGroup, bool Select)
{
	if (!TopGroup)
		return;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (!Piece->IsSelected() && Piece->IsVisible(m_nCurStep) && (Piece->GetTopGroup() == TopGroup))
			Piece->SetSelected(Select);
	}
}

void Project::FocusOrDeselectObject(const lcObjectSection& ObjectSection)
{
	Object* FocusObject = GetFocusObject();
	Object* Object = ObjectSection.Object;
	lcuint32 Section = ObjectSection.Section;

	if (Object)
	{
		bool WasSelected = Object->IsSelected();

		if (!Object->IsFocused(Section))
		{
			if (FocusObject)
				FocusObject->SetFocused(FocusObject->GetFocusSection(), false);

			Object->SetFocused(Section, true);
		}
		else
			Object->SetSelected(Section, false);

		bool IsSelected = Object->IsSelected();

		if (Object->IsPiece() && (WasSelected != IsSelected))
			SelectGroup(((Piece*)Object)->GetTopGroup(), IsSelected);
	}
	else
	{
		if (FocusObject)
			FocusObject->SetFocused(FocusObject->GetFocusSection(), false);
	}

	UpdateSelection();
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateFocusObject(GetFocusObject());
}

void Project::ClearSelectionAndSetFocus(Object* Object, lcuint32 Section)
{
	SelectAndFocusNone(false);

	if (Object)
	{
		Object->SetFocused(Section, true);

		if (Object->IsPiece())
			SelectGroup(((Piece*)Object)->GetTopGroup(), true);
	}

	UpdateSelection();
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateFocusObject(Object);
}

void Project::SetSelection(const lcArray<lcObjectSection>& ObjectSections)
{
	SelectAndFocusNone(false);
	AddToSelection(ObjectSections);
}

void Project::AddToSelection(const lcArray<lcObjectSection>& ObjectSections)
{
	for (int ObjectIdx = 0; ObjectIdx < ObjectSections.GetSize(); ObjectIdx++)
	{
		lcObject* Object = ObjectSections[ObjectIdx].Object;

		bool WasSelected = Object->IsSelected();
		Object->SetSelected(ObjectSections[ObjectIdx].Section, true);

		if (!WasSelected && Object->GetType() == LC_OBJECT_PIECE)
			SelectGroup(((Piece*)Object)->GetTopGroup(), true);
	}

	UpdateSelection();
	gMainWindow->UpdateAllViews();
	gMainWindow->UpdateFocusObject(GetFocusObject());
}

bool Project::GetSelectionCenter(lcVector3& Center) const
{
	float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
	bool Selected = false;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			Piece->CompareBoundingBox(bs);
			Selected = true;
		}
	}

	Center = lcVector3((bs[0] + bs[3]) * 0.5f, (bs[1] + bs[4]) * 0.5f, (bs[2] + bs[5]) * 0.5f);

	return Selected;
}

lcVector3 Project::GetFocusOrSelectionCenter() const
{
	lcVector3 Center;

	if (GetFocusPosition(Center))
		return Center;

	GetSelectionCenter(Center);

	return Center;
}

void Project::ConvertToUserUnits(lcVector3& Value) const
{
	Value /= 0.04f;
}

void Project::ConvertFromUserUnits(lcVector3& Value) const
{
	Value *= 0.04f;
}

bool Project::GetFocusPosition(lcVector3& Position) const
{
	Object* FocusObject = GetFocusObject();

	if (FocusObject)
	{
		Position = FocusObject->GetSectionPosition(FocusObject->GetFocusSection());
		return true;
	}
	else
	{
		Position = lcVector3(0.0f, 0.0f, 0.0f);
		return false;
	}
}

// Returns the object that currently has focus.
Object* Project::GetFocusObject() const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (Piece->IsFocused())
			return Piece;
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* Camera = mCameras[CameraIdx];

		if (Camera->IsFocused())
			return Camera;
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		Light* Light = mLights[LightIdx];

		if (Light->IsFocused())
			return Light;
	}

	return NULL;
}

bool Project::AnyObjectsSelected(bool PiecesOnly) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		if (mPieces[PieceIdx]->IsSelected())
			return true;

	if (!PiecesOnly)
	{
		for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			if (mCameras[CameraIdx]->IsSelected())
				return true;

		for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
			if (mLights[LightIdx]->IsSelected())
				return true;
	}

	return false;
}

// Find a good starting position/orientation relative to an existing piece.
void Project::GetPieceInsertPosition(Piece* OffsetPiece, lcVector3& Position, lcVector4& Rotation)
{
	lcVector3 Dist(0, 0, OffsetPiece->mPieceInfo->m_fDimensions[2] - m_pCurPiece->m_fDimensions[5]);
	SnapVector(Dist);

	Position = lcMul31(Dist, OffsetPiece->mModelWorld);
	Rotation = OffsetPiece->mRotation;
}

// Try to find a good starting position/orientation for a new piece.
void Project::GetPieceInsertPosition(View* view, lcVector3& Position, lcVector4& Rotation)
{
	// Check if the mouse is over a piece.
	Piece* HitPiece = (Piece*)view->FindObjectUnderPointer(true).Object;

	if (HitPiece)
	{
		GetPieceInsertPosition(HitPiece, Position, Rotation);
		return;
	}

	// Try to hit the base grid.
	lcVector3 ClickPoints[2] = { lcVector3((float)view->mInputState.x, (float)view->mInputState.y, 0.0f), lcVector3((float)view->mInputState.x, (float)view->mInputState.y, 1.0f) };
	view->UnprojectPoints(ClickPoints, 2);

	lcVector3 Intersection;
	if (lcLinePlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], lcVector4(0, 0, 1, m_pCurPiece->m_fDimensions[5])))
	{
		SnapVector(Intersection);
		Position = Intersection;
		Rotation = lcVector4(0, 0, 1, 0);
		return;
	}

	// Couldn't find a good position, so just place the piece somewhere near the camera.
	Position =  view->UnprojectPoint(lcVector3((float)view->mInputState.x, (float)view->mInputState.y, 0.9f));
	Rotation = lcVector4(0, 0, 1, 0);
}

void Project::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(m_nCurStep))
			Piece->RayTest(ObjectRayTest);
	}

	if (ObjectRayTest.PiecesOnly)
		return;

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* Camera = mCameras[CameraIdx];

		if (Camera != ObjectRayTest.ViewCamera && Camera->IsVisible())
			Camera->RayTest(ObjectRayTest);
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		if (mLights[LightIdx]->IsVisible())
			mLights[LightIdx]->RayTest(ObjectRayTest);
}

void Project::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (Piece->IsVisible(m_nCurStep))
			Piece->BoxTest(ObjectBoxTest);
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* Camera = mCameras[CameraIdx];

		if (Camera != ObjectBoxTest.ViewCamera && Camera->IsVisible())
			Camera->BoxTest(ObjectBoxTest);
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		if (mLights[LightIdx]->IsVisible())
			mLights[LightIdx]->BoxTest(ObjectBoxTest);
}

void Project::GetSnapIndex(int* SnapXY, int* SnapZ, int* SnapAngle) const
{
	if (SnapXY)
		*SnapXY = (m_nMoveSnap & 0xff);

	if (SnapZ)
		*SnapZ = ((m_nMoveSnap >> 8) & 0xff);

	if (SnapAngle)
	{
		if (m_nSnap & LC_DRAW_SNAP_A)
		{
			int Angles[] = { 0, 1, 5, 10, 15, 30, 45, 60, 90, 180 };
			*SnapAngle = -1;

			for (unsigned int i = 0; i < sizeof(Angles)/sizeof(Angles[0]); i++)
			{
				if (m_nAngleSnap == Angles[i])
				{
					*SnapAngle = i;
					break;
				}
			}
		}
		else
			*SnapAngle = 0;
	}
}

void Project::GetSnapDistance(float* SnapXY, float* SnapZ) const
{
	const float SnapXYTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 1.6f, 2.4f, 3.2f, 6.4f };
	const float SnapZTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 0.96f, 1.92f, 3.84f, 7.68f };

	int SXY, SZ;
	GetSnapIndex(&SXY, &SZ, NULL);

	SXY = lcMin(SXY, 9);
	SZ = lcMin(SZ, 9);

	*SnapXY = SnapXYTable[SXY];
	*SnapZ = SnapZTable[SZ];
}

void Project::GetSnapText(char* SnapXY, char* SnapZ, char* SnapAngle) const
{
	const char* SnapXYText[] = { "0", "1/20S", "1/4S", "1F", "1/2S", "1S", "2S", "3S", "4S", "8S" };
	const char* SnapZText[] = { "0", "1/20S", "1/4S", "1F", "1/2S", "1S", "1B", "2B", "4B", "8B" };
	const char* SnapAngleText[] = { "0", "1", "5", "10", "15", "30", "45", "60", "90", "180" };

	int SXY, SZ, SA;
	GetSnapIndex(&SXY, &SZ, &SA);

	SXY = lcMin(SXY, 9);
	SZ = lcMin(SZ, 9);

	strcpy(SnapXY, SnapXYText[SXY]);
	strcpy(SnapZ, SnapZText[SZ]);
	strcpy(SnapAngle, SnapAngleText[SA]);
}

void Project::SnapVector(lcVector3& Delta, lcVector3& Leftover) const
{
	float SnapXY, SnapZ;
	GetSnapDistance(&SnapXY, &SnapZ);

	if (m_nSnap & LC_DRAW_SNAP_X)
	{
		int i = (int)(Delta[0] / SnapXY);
		Leftover[0] = Delta[0] - (SnapXY * i);

		if (Leftover[0] > SnapXY / 2)
		{
			Leftover[0] -= SnapXY;
			i++;
		}
		else if (Leftover[0] < -SnapXY / 2)
		{
			Leftover[0] += SnapXY;
			i--;
		}

		Delta[0] = SnapXY * i;
	}

	if (m_nSnap & LC_DRAW_SNAP_Y)
	{
		int i = (int)(Delta[1] / SnapXY);
		Leftover[1] = Delta[1] - (SnapXY * i);

		if (Leftover[1] > SnapXY / 2)
		{
			Leftover[1] -= SnapXY;
			i++;
		}
		else if (Leftover[1] < -SnapXY / 2)
		{
			Leftover[1] += SnapXY;
			i--;
		}

		Delta[1] = SnapXY * i;
	}

	if (m_nSnap & LC_DRAW_SNAP_Z)
	{
		int i = (int)(Delta[2] / SnapZ);
		Leftover[2] = Delta[2] - (SnapZ * i);

		if (Leftover[2] > SnapZ / 2)
		{
			Leftover[2] -= SnapZ;
			i++;
		}
		else if (Leftover[2] < -SnapZ / 2)
		{
			Leftover[2] += SnapZ;
			i--;
		}

		Delta[2] = SnapZ * i;
	}
}

void Project::SnapRotationVector(lcVector3& Delta, lcVector3& Leftover) const
{
	if (m_nSnap & LC_DRAW_SNAP_A)
	{
		int Snap[3];

		for (int i = 0; i < 3; i++)
		{
			Snap[i] = (int)(Delta[i] / (float)m_nAngleSnap);
		}

		lcVector3 NewDelta((float)(m_nAngleSnap * Snap[0]), (float)(m_nAngleSnap * Snap[1]), (float)(m_nAngleSnap * Snap[2]));
		Leftover = Delta - NewDelta;
		Delta = NewDelta;
	}
}

lcMatrix44 Project::GetRelativeRotation() const
{
	if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
	{
		Object* Focus = GetFocusObject();

		if ((Focus != NULL) && Focus->IsPiece())
		{
			lcMatrix44 WorldMatrix = ((Piece*)Focus)->mModelWorld;
			WorldMatrix.SetTranslation(lcVector3(0.0f, 0.0f, 0.0f));
			return WorldMatrix;
		}
	}

	return lcMatrix44Identity();
}

bool Project::MoveSelectedObjects(lcVector3& Move, lcVector3& Remainder, bool Snap, bool Lock)
{
	// Don't move along locked directions.
	if (Lock)
	{
		if (m_nSnap & LC_DRAW_LOCK_X)
			Move[0] = 0;

		if (m_nSnap & LC_DRAW_LOCK_Y)
			Move[1] = 0;

		if (m_nSnap & LC_DRAW_LOCK_Z)
			Move[2] = 0;
	}

	// Snap.
	if (Snap)
	{
		SnapVector(Move, Remainder);

		if (Move.LengthSquared() < 0.001f)
			return false;
	}

	lcVector3 TransformedMove = lcMul30(Move, GetRelativeRotation());

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			Piece->Move(m_nCurStep, gMainWindow->GetAddKeys(), TransformedMove);
			Piece->UpdatePosition(m_nCurStep);
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		lcCamera* Camera = mCameras[CameraIdx];

		if (Camera->IsSelected())
		{
			Camera->Move(m_nCurStep, gMainWindow->GetAddKeys(), TransformedMove);
			Camera->UpdatePosition(m_nCurStep);
		}
	}

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
	{
		lcLight* Light = mLights[LightIdx];

		if (Light->IsSelected())
		{
			Light->Move(m_nCurStep, gMainWindow->GetAddKeys(), TransformedMove);
			Light->UpdatePosition(m_nCurStep);
		}
	}

	return true;
}

bool Project::RotateSelectedObjects(lcVector3& Delta, lcVector3& Remainder, bool Snap, bool Lock)
{
	// Don't move along locked directions.
	if (Lock)
	{
		if (m_nSnap & LC_DRAW_LOCK_X)
			Delta[0] = 0;

		if (m_nSnap & LC_DRAW_LOCK_Y)
			Delta[1] = 0;

		if (m_nSnap & LC_DRAW_LOCK_Z)
			Delta[2] = 0;
	}

	// Snap.
	if (Snap)
		SnapRotationVector(Delta, Remainder);

	if (Delta.LengthSquared() < 0.001f)
		return false;

	float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
	lcVector3 pos;
	lcVector4 rot;
	int nSel = 0;
	Piece *pFocus = NULL;

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (Piece->IsSelected())
		{
			if (Piece->IsFocused())
				pFocus = Piece;

			Piece->CompareBoundingBox(bs);
			nSel++;
		}
	}

	if (pFocus != NULL)
	{
		pos = pFocus->mPosition;
		bs[0] = bs[3] = pos[0];
		bs[1] = bs[4] = pos[1];
		bs[2] = bs[5] = pos[2];
	}

	lcVector3 Center((bs[0]+bs[3])/2, (bs[1]+bs[4])/2, (bs[2]+bs[5])/2);

	// Create the rotation matrix.
	lcVector4 RotationQuaternion(0, 0, 0, 1);
	lcVector4 WorldToFocusQuaternion, FocusToWorldQuaternion;

	if (!(m_nSnap & LC_DRAW_LOCK_X) && (Delta[0] != 0.0f))
	{
		lcVector4 q = lcQuaternionRotationX(Delta[0] * LC_DTOR);
		RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
	}

	if (!(m_nSnap & LC_DRAW_LOCK_Y) && (Delta[1] != 0.0f))
	{
		lcVector4 q = lcQuaternionRotationY(Delta[1] * LC_DTOR);
		RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
	}

	if (!(m_nSnap & LC_DRAW_LOCK_Z) && (Delta[2] != 0.0f))
	{
		lcVector4 q = lcQuaternionRotationZ(Delta[2] * LC_DTOR);
		RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
	}

	// Transform the rotation relative to the focused piece.
	if (m_nSnap & LC_DRAW_GLOBAL_SNAP)
		pFocus = NULL;

	if (pFocus != NULL)
	{
		lcVector4 Rot;
		Rot = ((Piece*)pFocus)->mRotation;

		WorldToFocusQuaternion = lcQuaternionFromAxisAngle(lcVector4(Rot[0], Rot[1], Rot[2], -Rot[3] * LC_DTOR));
		FocusToWorldQuaternion = lcQuaternionFromAxisAngle(lcVector4(Rot[0], Rot[1], Rot[2], Rot[3] * LC_DTOR));

		RotationQuaternion = lcQuaternionMultiply(FocusToWorldQuaternion, RotationQuaternion);
	}

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		Piece* Piece = mPieces[PieceIdx];

		if (!Piece->IsSelected())
			continue;

		pos = Piece->mPosition;
		rot = Piece->mRotation;

		lcVector4 NewRotation;

		if ((nSel == 1) && (pFocus == Piece))
		{
			lcVector4 LocalToWorldQuaternion;
			LocalToWorldQuaternion = lcQuaternionFromAxisAngle(lcVector4(rot[0], rot[1], rot[2], rot[3] * LC_DTOR));

			lcVector4 NewLocalToWorldQuaternion;

			if (pFocus != NULL)
			{
				lcVector4 LocalToFocusQuaternion = lcQuaternionMultiply(WorldToFocusQuaternion, LocalToWorldQuaternion);
				NewLocalToWorldQuaternion = lcQuaternionMultiply(LocalToFocusQuaternion, RotationQuaternion);
			}
			else
			{
				NewLocalToWorldQuaternion = lcQuaternionMultiply(RotationQuaternion, LocalToWorldQuaternion);
			}

			NewRotation = lcQuaternionToAxisAngle(NewLocalToWorldQuaternion);
		}
		else
		{
			lcVector3 Distance = lcVector3(pos[0], pos[1], pos[2]) - Center;

			lcVector4 LocalToWorldQuaternion = lcQuaternionFromAxisAngle(lcVector4(rot[0], rot[1], rot[2], rot[3] * LC_DTOR));

			lcVector4 NewLocalToWorldQuaternion;

			if (pFocus != NULL)
			{
				lcVector4 LocalToFocusQuaternion = lcQuaternionMultiply(WorldToFocusQuaternion, LocalToWorldQuaternion);
				NewLocalToWorldQuaternion = lcQuaternionMultiply(RotationQuaternion, LocalToFocusQuaternion);

				lcVector4 WorldToLocalQuaternion = lcQuaternionFromAxisAngle(lcVector4(rot[0], rot[1], rot[2], -rot[3] * LC_DTOR));

				Distance = lcQuaternionMul(Distance, WorldToLocalQuaternion);
				Distance = lcQuaternionMul(Distance, NewLocalToWorldQuaternion);
			}
			else
			{
				NewLocalToWorldQuaternion = lcQuaternionMultiply(RotationQuaternion, LocalToWorldQuaternion);

				Distance = lcQuaternionMul(Distance, RotationQuaternion);
			}

			NewRotation = lcQuaternionToAxisAngle(NewLocalToWorldQuaternion);

			pos[0] = Center[0] + Distance[0];
			pos[1] = Center[1] + Distance[1];
			pos[2] = Center[2] + Distance[2];

			Piece->ChangeKey(m_nCurStep, gMainWindow->GetAddKeys(), pos, LC_PK_POSITION);
		}

		rot[0] = NewRotation[0];
		rot[1] = NewRotation[1];
		rot[2] = NewRotation[2];
		rot[3] = NewRotation[3] * LC_RTOD;

		Piece->ChangeKey(m_nCurStep, gMainWindow->GetAddKeys(), rot, LC_PK_ROTATION);
		Piece->UpdatePosition(m_nCurStep);
	}

	return true;
}

void Project::TransformSelectedObjects(LC_TRANSFORM_TYPE Type, const lcVector3& Transform)
{
	switch (Type)
	{
	case LC_TRANSFORM_ABSOLUTE_TRANSLATION:
		{
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			lcVector3 Center;
			int nSel = 0;
			Piece* pFocus = NULL;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					if (Piece->IsFocused())
						pFocus = Piece;

					Piece->CompareBoundingBox(bs);
					nSel++;
				}
			}

			if (pFocus != NULL)
				Center = pFocus->mPosition;
			else
				Center = lcVector3((bs[0]+bs[3])/2, (bs[1]+bs[4])/2, (bs[2]+bs[5])/2);

			lcVector3 Offset = Transform - Center;

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			{
				Camera* pCamera = mCameras[CameraIdx];

				if (pCamera->IsSelected())
				{
					pCamera->Move(m_nCurStep, gMainWindow->GetAddKeys(), Offset);
					pCamera->UpdatePosition(m_nCurStep);
				}
			}

			for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
			{
				Light* pLight = mLights[LightIdx];

				if (pLight->IsSelected())
				{
					pLight->Move(m_nCurStep, gMainWindow->GetAddKeys(), Offset);
					pLight->UpdatePosition (m_nCurStep);
				}
			}

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					Piece->Move(m_nCurStep, gMainWindow->GetAddKeys(), Offset);
					Piece->UpdatePosition(m_nCurStep);
				}
			}

			if (nSel)
			{
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Moving");
				gMainWindow->UpdateFocusObject(GetFocusObject());
			}
		} break;

	case LC_TRANSFORM_RELATIVE_TRANSLATION:
		{
			lcVector3 Move(Transform), Remainder;

			if (MoveSelectedObjects(Move, Remainder, false, false))
			{
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Moving");
				gMainWindow->UpdateFocusObject(GetFocusObject());
			}
		} break;

	case LC_TRANSFORM_ABSOLUTE_ROTATION:
		{
			// Create the rotation matrix.
			lcVector4 RotationQuaternion(0, 0, 0, 1);

			if (Transform[0] != 0.0f)
			{
				lcVector4 q = lcQuaternionRotationX(Transform[0] * LC_DTOR);
				RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
			}

			if (Transform[1] != 0.0f)
			{
				lcVector4 q = lcQuaternionRotationY(Transform[1] * LC_DTOR);
				RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
			}

			if (Transform[2] != 0.0f)
			{
				lcVector4 q = lcQuaternionRotationZ(Transform[2] * LC_DTOR);
				RotationQuaternion = lcQuaternionMultiply(q, RotationQuaternion);
			}

			lcVector4 NewRotation = lcQuaternionToAxisAngle(RotationQuaternion);
			NewRotation[3] *= LC_RTOD;

			int nSel = 0;

			for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
			{
				Piece* Piece = mPieces[PieceIdx];

				if (Piece->IsSelected())
				{
					Piece->ChangeKey(m_nCurStep, gMainWindow->GetAddKeys(), NewRotation, LC_PK_ROTATION);
					Piece->UpdatePosition(m_nCurStep);
					nSel++;
				}
			}

			if (nSel)
			{
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Rotating");
				gMainWindow->UpdateFocusObject(GetFocusObject());
			}
		} break;

	case LC_TRANSFORM_RELATIVE_ROTATION:
		{
			lcVector3 Rotate(Transform), Remainder;

			if (RotateSelectedObjects(Rotate, Remainder, false, false))
			{
				gMainWindow->UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Rotating");
				gMainWindow->UpdateFocusObject(GetFocusObject());
			}
		} break;
	}
}

void Project::ModifyObject(Object* Object, lcObjectProperty Property, void* Value)
{
	const char* CheckPointString = NULL;

	switch (Property)
	{
	case LC_PIECE_PROPERTY_POSITION:
		{
			const lcVector3& Position = *(lcVector3*)Value;
			Piece* Part = (Piece*)Object;

			if (Part->mPosition != Position)
			{
				Part->ChangeKey(m_nCurStep, gMainWindow->GetAddKeys(), Position, LC_PK_POSITION);
				Part->UpdatePosition(m_nCurStep);

				CheckPointString = "Moving";
			}
		} break;

	case LC_PIECE_PROPERTY_ROTATION:
		{
			const lcVector4& Rotation = *(lcVector4*)Value;
			Piece* Part = (Piece*)Object;

			if (Rotation != Part->mRotation)
			{
				Part->ChangeKey(m_nCurStep, gMainWindow->GetAddKeys(), Rotation, LC_PK_ROTATION);
				Part->UpdatePosition(m_nCurStep);

				CheckPointString = "Rotating";
			}
		} break;

	case LC_PIECE_PROPERTY_SHOW:
		{
			lcuint32 Show = *(lcuint32*)Value;
			Piece* Part = (Piece*)Object;

			if (Show != Part->GetStepShow())
			{
				Part->SetStepShow(Show);
				if (Part->IsSelected() && !Part->IsVisible(m_nCurStep))
					Part->SetSelected(false);

				CheckPointString = "Show";
			}
		} break;

	case LC_PIECE_PROPERTY_HIDE:
		{
			lcuint32 Hide = *(lcuint32*)Value;
			Piece* Part = (Piece*)Object;

			if (Hide != Part->GetStepHide())
			{
				Part->SetStepHide(Hide);

				CheckPointString = "Hide";
			}
		} break;

	case LC_PIECE_PROPERTY_COLOR:
		{
			int ColorIndex = *(int*)Value;
			Piece* Part = (Piece*)Object;

			if (ColorIndex != Part->mColorIndex)
			{
				Part->SetColorIndex(ColorIndex);

				CheckPointString = "Color";
			}
		} break;

	case LC_PIECE_PROPERTY_ID:
		{
			Piece* Part = (Piece*)Object;
			PieceInfo* Info = (PieceInfo*)Value;

			if (Info != Part->mPieceInfo)
			{
				Part->mPieceInfo->Release();
				Part->mPieceInfo = Info;
				Part->mPieceInfo->AddRef();

				CheckPointString = "Part";
			}
		} break;

	case LC_CAMERA_PROPERTY_POSITION:
			{
			const lcVector3& Position = *(lcVector3*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->mPosition != Position)
			{
				camera->ChangeKey(m_nCurStep, gMainWindow->GetAddKeys(), Position, LC_CK_EYE);
				camera->UpdatePosition(m_nCurStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_TARGET:
		{
			const lcVector3& TargetPosition = *(lcVector3*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->mTargetPosition != TargetPosition)
			{
				camera->ChangeKey(m_nCurStep, gMainWindow->GetAddKeys(), TargetPosition, LC_CK_TARGET);
				camera->UpdatePosition(m_nCurStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_UPVECTOR:
		{
			const lcVector3& Up = *(lcVector3*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->mUpVector != Up)
			{
				camera->ChangeKey(m_nCurStep, gMainWindow->GetAddKeys(), Up, LC_CK_UP);
				camera->UpdatePosition(m_nCurStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_ORTHO:
		{
			bool Ortho = *(bool*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->IsOrtho() != Ortho)
			{
				camera->SetOrtho(Ortho);
				camera->UpdatePosition(m_nCurStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_FOV:
		{
			float FOV = *(float*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->m_fovy != FOV)
			{
				camera->m_fovy = FOV;
				camera->UpdatePosition(m_nCurStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_NEAR:
		{
			float Near = *(float*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->m_zNear != Near)
			{
				camera->m_zNear= Near;
				camera->UpdatePosition(m_nCurStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_FAR:
		{
			float Far = *(float*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->m_zFar != Far)
			{
				camera->m_zFar = Far;
				camera->UpdatePosition(m_nCurStep);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_PROPERTY_NAME:
		{
			const char* Name = (const char*)Value;
			Camera* camera = (Camera*)Object;

			if (strcmp(camera->m_strName, Name))
			{
				strncpy(camera->m_strName, Name, sizeof(camera->m_strName));
				camera->m_strName[sizeof(camera->m_strName) - 1] = 0;

				gMainWindow->UpdateCameraMenu();

				CheckPointString = "Camera";
			}
		}
	}

	if (CheckPointString)
	{
		SetModifiedFlag(true);
		CheckPoint(CheckPointString);
		gMainWindow->UpdateFocusObject(GetFocusObject());
		gMainWindow->UpdateAllViews();
	}
}

void Project::ZoomActiveView(int Amount)
{
	float ScaledAmount = 2.0f * Amount / (21 - lcGetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY));

	gMainWindow->GetActiveView()->mCamera->Zoom(ScaledAmount, m_nCurStep, gMainWindow->GetAddKeys());
	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
}

void Project::BeginMouseTool()
{
	mMouseToolDistance = lcVector3(0.0f, 0.0f, 0.0f);
}

void Project::EndMouseTool(lcTool Tool, bool Accept)
{
	if (!Accept)
	{
		DeleteContents(true);
		FileLoad(&mUndoHistory[0]->File, true, false);
		return;
	}

	switch (Tool)
	{
	case LC_TOOL_INSERT:
	case LC_TOOL_LIGHT:
		break;

	case LC_TOOL_SPOTLIGHT:
		CheckPoint("New SpotLight");
		SetModifiedFlag(true);
		break;

	case LC_TOOL_CAMERA:
		gMainWindow->UpdateCameraMenu();
		CheckPoint("New Camera");
		SetModifiedFlag(true);
		break;

	case LC_TOOL_SELECT:
		break;

	case LC_TOOL_MOVE:
		CheckPoint("Move");
		SetModifiedFlag(true);
		break;

	case LC_TOOL_ROTATE:
		CheckPoint("Rotate");
		SetModifiedFlag(true);
		break;

	case LC_TOOL_ERASER:
	case LC_TOOL_PAINT:
		break;

	case LC_TOOL_ZOOM:
		if (!gMainWindow->GetActiveView()->mCamera->IsSimple())
			CheckPoint("Zoom");
		break;

	case LC_TOOL_PAN:
		if (!gMainWindow->GetActiveView()->mCamera->IsSimple())
			CheckPoint("Pan");
		break;

	case LC_TOOL_ROTATE_VIEW:
		if (!gMainWindow->GetActiveView()->mCamera->IsSimple())
			CheckPoint("Orbit");
		break;

	case LC_TOOL_ROLL:
		if (!gMainWindow->GetActiveView()->mCamera->IsSimple())
			CheckPoint("Roll");
		break;

	case LC_TOOL_ZOOM_REGION:
		break;
	}
}

void Project::InsertPieceToolClicked(const lcVector3& Position, const lcVector4& Rotation)
{
	lcPiece* Piece = new lcPiece(m_pCurPiece);
	Piece->Initialize(Position[0], Position[1], Position[2], m_nCurStep);
	Piece->SetColorIndex(gMainWindow->mColorIndex);
	Piece->ChangeKey(m_nCurStep, false, Rotation, LC_PK_ROTATION);
	Piece->UpdatePosition(m_nCurStep);
	Piece->CreateName(mPieces);
	mPieces.Add(Piece);

	SystemPieceComboAdd(m_pCurPiece->m_strDescription);
	ClearSelectionAndSetFocus(Piece, LC_PIECE_SECTION_POSITION);

	CheckPoint("Insert");
	SetModifiedFlag(true);
}

void Project::PointLightToolClicked(const lcVector3& Position)
{
	lcLight* Light = new lcLight(Position[0], Position[1], Position[2]);
	Light->CreateName(mLights);
	mLights.Add(Light);

	ClearSelectionAndSetFocus(Light, LC_LIGHT_SECTION_POSITION);
	CheckPoint("New Light");
	SetModifiedFlag(true);
}

void Project::BeginSpotLightTool(const lcVector3& Position, const lcVector3& Target)
{
	lcLight* Light = new lcLight(Position[0], Position[1], Position[2], Target[0], Target[1], Target[2]);
	mLights.Add(Light);

	ClearSelectionAndSetFocus(Light, LC_LIGHT_SECTION_TARGET);
}

void Project::UpdateSpotLightTool(const lcVector3& Target)
{
	lcLight* Light = mLights[mLights.GetSize() - 1];

	Light->Move(1, false, Target);
	Light->UpdatePosition(1);

	gMainWindow->UpdateFocusObject(Light);
	gMainWindow->UpdateAllViews();
}

void Project::BeginCameraTool(const lcVector3& Position, const lcVector3& Target)
{
	lcCamera* Camera = new lcCamera(Position[0], Position[1], Position[2], Target[0], Target[1], Target[2]);
	Camera->CreateName(mCameras);
	mCameras.Add(Camera);

	ClearSelectionAndSetFocus(Camera, LC_CAMERA_SECTION_TARGET);
}

void Project::UpdateCameraTool(const lcVector3& Target)
{
	lcCamera* Camera = mCameras[mCameras.GetSize() - 1];

	Camera->Move(1, false, Target);
	Camera->UpdatePosition(1);

	gMainWindow->UpdateFocusObject(Camera);
	gMainWindow->UpdateAllViews();
}

void Project::UpdateMoveTool(const lcVector3& Distance)
{
	lcVector3 Delta, Remainder;
	Delta = Distance - mMouseToolDistance;
	MoveSelectedObjects(Delta, Remainder, true, true);
	mMouseToolDistance += Delta;

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
}

void Project::UpdateRotateTool(const lcVector3& Angles)
{
	lcVector3 Delta, Remainder;
	Delta = Angles - mMouseToolDistance;
	RotateSelectedObjects(Delta, Remainder, true, true);
	mMouseToolDistance += Delta;

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();
}

void Project::EraserToolClicked(lcObject* Object)
{
	if (!Object)
		return;

	switch (Object->GetType())
	{
	case LC_OBJECT_PIECE:
		mPieces.Remove((Piece*)Object);
		RemoveEmptyGroups();
		break;

	case LC_OBJECT_CAMERA:
		{
			const lcArray<View*> Views = gMainWindow->GetViews();
			for (int ViewIdx = 0; ViewIdx < Views.GetSize(); ViewIdx++)
			{
				View* View = Views[ViewIdx];
				Camera* Camera = View->mCamera;

				if (Camera == Object)
					View->SetCamera(Camera, true);
			}

			mCameras.Remove((Camera*)Object);

			gMainWindow->UpdateCameraMenu();
		}
		break;

	case LC_OBJECT_LIGHT:
		mLights.Remove((Light*)Object);
		break;
	}

	delete Object;
	gMainWindow->UpdateFocusObject(GetFocusObject());
	UpdateSelection();
	gMainWindow->UpdateAllViews();
	SetModifiedFlag(true);
	CheckPoint("Deleting");
}

void Project::PaintToolClicked(lcObject* Object)
{
	if (!Object || Object->GetType() != LC_OBJECT_PIECE)
		return;

	lcPiece* Piece = (lcPiece*)Object;

	if (Piece->mColorIndex != gMainWindow->mColorIndex)
	{
		Piece->SetColorIndex(gMainWindow->mColorIndex);

		SetModifiedFlag(true);
		CheckPoint("Painting");
		gMainWindow->UpdateFocusObject(GetFocusObject());
		gMainWindow->UpdateAllViews();
	}
}

void Project::UpdateZoomTool(lcCamera* Camera, float Mouse)
{
	Camera->Zoom(Mouse - mMouseToolDistance.x, m_nCurStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = Mouse;
	gMainWindow->UpdateAllViews();
}

void Project::UpdatePanTool(lcCamera* Camera, float MouseX, float MouseY)
{
	Camera->Pan(MouseX - mMouseToolDistance.x, MouseY - mMouseToolDistance.y, m_nCurStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = MouseX;
	mMouseToolDistance.y = MouseY;
	gMainWindow->UpdateAllViews();
}

void Project::UpdateOrbitTool(lcCamera* Camera, float MouseX, float MouseY)
{
	lcVector3 Center;
	GetSelectionCenter(Center);
	Camera->Orbit(MouseX - mMouseToolDistance.x, MouseY - mMouseToolDistance.y, Center, m_nCurStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = MouseX;
	mMouseToolDistance.y = MouseY;
	gMainWindow->UpdateAllViews();
}

void Project::UpdateRollTool(lcCamera* Camera, float Mouse)
{
	Camera->Roll(Mouse - mMouseToolDistance.x, m_nCurStep, gMainWindow->GetAddKeys());
	mMouseToolDistance.x = Mouse;
	gMainWindow->UpdateAllViews();
}

void Project::ZoomRegionToolClicked(lcCamera* Camera, const lcVector3* Points, float RatioX, float RatioY)
{
	Camera->ZoomRegion(Points, m_nCurStep, gMainWindow->GetAddKeys(), RatioX, RatioY);

	gMainWindow->UpdateFocusObject(GetFocusObject());
	gMainWindow->UpdateAllViews();

	if (!Camera->IsSimple())
		CheckPoint("Zoom");
}

void Project::OnMouseWheel(View* view, float Direction)
{
	ZoomActiveView((int)((view->mInputState.Control ? 100 : 10) * Direction));
}

// Indicates if the existing string represents an instance of the candidate
// string in the form "<basename> (#<instance>)".
//
// Returns:
//	-1  if existing is not an instance of candidate.
//	0  if existing is an instance but not numbered.
//	>= 1  indicates the existing instance number.
//
int Project::InstanceOfName(const String& existingString, const String& candidateString, String& baseNameOut)
{
	int einst = 0;
	String estr = existingString;
	estr.TrimLeft();
	estr.TrimRight();

	int div = estr.ReverseFind('#');
	if (-1 != div)
	{
		char* endptr;
		einst = strtol(estr.Mid(div + 1), &endptr, 10);
		if (!*endptr)
		{
			estr = estr.Left(div);
			estr.TrimRight();
		}
	}

	String cstr = candidateString;
	cstr.TrimLeft();
	cstr.TrimRight();

	div = cstr.ReverseFind('#');
	if (-1 != div)
	{
		char* endptr;
        int Value = strtol(cstr.Mid(div + 1), &endptr, 10);
        (void)Value;
		if (!*endptr)
		{
			cstr = cstr.Left(div);
			cstr.TrimRight();
		}
	}

	if (estr.CompareNoCase(cstr))
		return -1;

	baseNameOut = estr;
	return einst;
}
