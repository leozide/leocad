// Everything that is a part of a LeoCAD project goes here.
//

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
#include "curve.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "lc_library.h"
#include "texfont.h"
#include "debug.h"
#include "lc_application.h"
#include "lc_profile.h"

/////////////////////////////////////////////////////////////////////////////
// Project construction/destruction

Project::Project()
{
	m_ActiveView = NULL;
	m_bModified = false;
	m_bTrackCancel = false;
	m_nTracking = LC_TRACK_NONE;
	m_OverlayMode = LC_OVERLAY_NONE;
	mDropPiece = NULL;
	m_pPieces = NULL;
	m_pLights = NULL;
	m_pGroups = NULL;
	m_pUndoList = NULL;
	m_pRedoList = NULL;
	m_pTrackFile = NULL;
	m_nCurAction = 0;
	mTransformType = LC_TRANSFORM_RELATIVE_TRANSLATION;
	m_pTerrain = new Terrain();
	m_pBackground = new lcTexture();
	mGridTexture = new lcTexture();
	m_nAutosave = lcGetProfileInt(LC_PROFILE_AUTOSAVE_INTERVAL);
	m_nMouse = lcGetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY);
	m_nDownX = 0;
	m_nDownY = 0;
	memset(&mSearchOptions, 0, sizeof(mSearchOptions));

	m_pScreenFont = new TexFont();
}

Project::~Project()
{
	DeleteContents(false);

    delete m_pTrackFile;
	delete m_pTerrain;
	delete m_pBackground;
	delete mGridTexture;
	delete m_pScreenFont;
}


/////////////////////////////////////////////////////////////////////////////
// Project attributes, general services

void Project::UpdateInterface()
{
	// Update all user interface elements.
	gMainWindow->UpdateUndoRedo(m_pUndoList->pNext ? m_pUndoList->strText : NULL, m_pRedoList ? m_pRedoList->strText : NULL);
	gMainWindow->UpdatePaste(g_App->mClipboard != NULL);
	SystemUpdatePlay(true, false);
	gMainWindow->UpdateCategories();
	gMainWindow->UpdateTitle(m_strTitle, m_bModified);

	gMainWindow->UpdateFocusObject(GetFocusObject());
	SetAction(m_nCurAction);
	gMainWindow->UpdateTransformType(mTransformType);
	gMainWindow->UpdateAnimation(m_bAnimation, m_bAddKeys);
	gMainWindow->UpdateLockSnap(m_nSnap);
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu(mCameras, m_ActiveView ? m_ActiveView->mCamera : NULL);
	UpdateSelection();
	if (m_bAnimation)
		gMainWindow->UpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
	else
		gMainWindow->UpdateTime(m_bAnimation, m_nCurStep, 255);

	for (int i = 0; i < m_ViewList.GetSize(); i++)
	{
		m_ViewList[i]->MakeCurrent();
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
	Piece* pPiece;
	Light* pLight;
	Group* pGroup;

	memset(m_strAuthor, 0, sizeof(m_strAuthor));
	memset(m_strDescription, 0, sizeof(m_strDescription));
	memset(m_strComments, 0, sizeof(m_strComments));

	if (!bUndo)
	{
		LC_UNDOINFO* pUndo;

		while (m_pUndoList)
		{
			pUndo = m_pUndoList;
			m_pUndoList = m_pUndoList->pNext;
			delete pUndo;
		}

		while (m_pRedoList)
		{
			pUndo = m_pRedoList;
			m_pRedoList = m_pRedoList->pNext;
			delete pUndo;
		}

		m_pRedoList = NULL;
		m_pUndoList = NULL;

		m_pBackground->Unload();
		m_pTerrain->LoadDefaults(true);
	}

	while (m_pPieces)
	{
		pPiece = m_pPieces;
		m_pPieces = m_pPieces->m_pNext;
		delete pPiece;
	}

	if (!bUndo)
	{
		for (int ViewIdx = 0; ViewIdx < m_ViewList.GetSize(); ViewIdx++)
		{
			View* view = m_ViewList[ViewIdx];

			if (!view->mCamera->IsSimple())
				view->SetDefaultCamera();
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		delete mCameras[CameraIdx];
	mCameras.RemoveAll();

	while (m_pLights)
	{
		pLight = m_pLights;
		m_pLights = m_pLights->m_pNext;
		delete pLight;
	}

	while (m_pGroups)
	{
		pGroup = m_pGroups;
		m_pGroups = m_pGroups->m_pNext;
		delete pGroup;
	}


/*
	if (!m_strTempFile.IsEmpty())
	{
		DeleteFile (m_strTempFile);
		m_strTempFile.Empty();
	}
*/
}

// Only call after DeleteContents()
void Project::LoadDefaults(bool cameras)
{
	int i;
	unsigned long rgb;

	// Default values
	gMainWindow->SetColorIndex(lcGetColorIndex(4));
	SetAction(LC_ACTION_SELECT);
	m_bAnimation = false;
	m_bAddKeys = false;
	gMainWindow->UpdateAnimation(m_bAnimation, m_bAddKeys);
	m_bUndoOriginal = true;
	gMainWindow->UpdateUndoRedo(NULL, NULL);
	m_nDetail = lcGetProfileInt(LC_PROFILE_DETAIL);
	m_nAngleSnap = (unsigned short)lcGetProfileInt(LC_PROFILE_ANGLE_SNAP);
	m_nSnap = lcGetProfileInt(LC_PROFILE_SNAP);
	gMainWindow->UpdateLockSnap(m_nSnap);
	m_nMoveSnap = 0x0304;
	gMainWindow->UpdateSnap();
	m_fLineWidth = (float)lcGetProfileFloat(LC_PROFILE_LINE_WIDTH);
	m_fFogDensity = (float)lcGetProfileFloat(LC_PROFILE_DEFAULT_FOG_DENSITY);
	rgb = lcGetProfileInt(LC_PROFILE_DEFAULT_FOG_COLOR);
	m_fFogColor[0] = (float)((unsigned char) (rgb))/255;
	m_fFogColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fFogColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_fFogColor[3] = 1.0f;
	mGridStuds = lcGetProfileInt(LC_PROFILE_GRID_STUDS);
	mGridStudColor = lcGetProfileInt(LC_PROFILE_GRID_STUD_COLOR);
	mGridLines = lcGetProfileInt(LC_PROFILE_GRID_LINES);
	mGridLineSpacing = lcGetProfileInt(LC_PROFILE_GRID_LINE_SPACING);
	mGridLineColor = lcGetProfileInt(LC_PROFILE_GRID_LINE_COLOR);
	rgb = lcGetProfileInt(LC_PROFILE_DEFAULT_AMBIENT_COLOR);
	m_fAmbient[0] = (float)((unsigned char) (rgb))/255;
	m_fAmbient[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fAmbient[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_fAmbient[3] = 1.0f;
	rgb = lcGetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_COLOR);
	m_fBackground[0] = (float)((unsigned char) (rgb))/255;
	m_fBackground[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fBackground[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_fBackground[3] = 1.0f;
	rgb = lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR1);
	m_fGradient1[0] = (float)((unsigned char) (rgb))/255;
	m_fGradient1[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fGradient1[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	rgb = lcGetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR2);
	m_fGradient2[0] = (float)((unsigned char) (rgb))/255;
	m_fGradient2[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fGradient2[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_nFPS = 24;
	m_nCurStep = 1;
	m_nCurFrame = 1;
	m_nTotalFrames = 100;
	gMainWindow->UpdateTime(false, 1, 255);
	m_nScene = lcGetProfileInt(LC_PROFILE_DEFAULT_SCENE);
	m_nSaveTimer = 0;
	strcpy(m_strHeader, "");
	strcpy(m_strFooter, "Page &P");
	strcpy(m_strBackground, lcGetProfileString(LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE));
	m_pTerrain->LoadDefaults(true);
	m_OverlayActive = false;

	for (i = 0; i < m_ViewList.GetSize (); i++)
	{
		m_ViewList[i]->MakeCurrent();
		RenderInitialize();
	}

	if (cameras)
	{
		for (i = 0; i < m_ViewList.GetSize(); i++)
			if (!m_ViewList[i]->mCamera->IsSimple())
				m_ViewList[i]->SetDefaultCamera();

		gMainWindow->UpdateCameraMenu(mCameras, m_ActiveView ? m_ActiveView->mCamera : NULL);
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
	lcuint8 ch, action = m_nCurAction;
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
		m_fBackground[0] = (float)((unsigned char) (rgb))/255;
		m_fBackground[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
		m_fBackground[2] = (float)((unsigned char) ((rgb) >> 16))/255;
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
		pCam->ChangeKey(1, false, false, tmp, LC_CK_EYE);
		pCam->ChangeKey(1, true, false, tmp, LC_CK_EYE);
		tmp[0] = (float)target[0]; tmp[1] = (float)target[1]; tmp[2] = (float)target[2];
		pCam->ChangeKey(1, false, false, tmp, LC_CK_TARGET);
		pCam->ChangeKey(1, true, false, tmp, LC_CK_TARGET);

		// Create up vector
		lcVector3 UpVector(0, 0, 1), FrontVector((float)(eye[0] - target[0]), (float)(eye[1] - target[1]), (float)(eye[2] - target[2])), SideVector;
		FrontVector.Normalize();
		if (FrontVector == UpVector)
			SideVector = lcVector3(1, 0, 0);
		else
			SideVector = lcCross(FrontVector, UpVector);
		UpVector = lcNormalize(lcCross(SideVector, FrontVector));
		pCam->ChangeKey(1, false, false, UpVector, LC_CK_UP);
		pCam->ChangeKey(1, true, false, UpVector, LC_CK_UP);
	}

	if (bMerge)
		file->Seek(32, SEEK_CUR);
	else
	{
		lcuint32 u;
		file->ReadS32(&i, 1); m_nAngleSnap = i;
		file->ReadU32(&u, 1); //m_nSnap
		file->ReadFloats(&m_fLineWidth, 1);
		file->ReadU32(&u, 1); //m_nDetail
		file->ReadS32(&i, 1); //m_nCurGroup = i;
		file->ReadS32(&i, 1); //m_nCurColor = i;
		file->ReadS32(&i, 1); action = i;
		file->ReadS32(&i, 1); m_nCurStep = i;
	}

	if (fv > 0.8f)
		file->ReadU32(&m_nScene, 1);

	file->ReadS32(&count, 1);
//	SystemStartProgressBar(0, count, 1, "Loading project...");
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	Library->OpenCache();

	while (count--)
	{
		if (fv > 0.4f)
		{
			Piece* pPiece = new Piece(NULL);
			pPiece->FileLoad(*file);

			if (bMerge)
				for (Piece* p = m_pPieces; p; p = p->m_pNext)
					if (strcmp(p->GetName(), pPiece->GetName()) == 0)
					{
						pPiece->CreateName(m_pPieces);
						break;
					}

			if (strlen(pPiece->GetName()) == 0)
				pPiece->CreateName(m_pPieces);

			AddPiece(pPiece);
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

			pPiece->Initialize(pos[0], pos[1], pos[2], step, 1);
			pPiece->SetColorCode(lcGetColorCodeFromOriginalColor(color));
			pPiece->CreateName(m_pPieces);
			AddPiece(pPiece);

			lcMatrix44 ModelWorld = lcMul(lcMatrix44RotationZ(rot[2] * LC_DTOR), lcMul(lcMatrix44RotationY(rot[1] * LC_DTOR), lcMatrix44RotationX(rot[0] * LC_DTOR)));
			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(ModelWorld);
			AxisAngle[3] *= LC_RTOD;

			pPiece->ChangeKey(1, false, false, AxisAngle, LC_PK_ROTATION);
			pPiece->ChangeKey(1, true, false, AxisAngle, LC_PK_ROTATION);
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
				file->ReadBuffer(m_strAuthor, sh);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF) file->ReadU16(&sh, 1); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
				file->ReadBuffer(m_strDescription, sh);

			file->ReadBuffer(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->ReadU16(&sh, 1); else sh = ch;
			if (sh > 255)
				file->Seek(sh, SEEK_CUR);
			else
				file->ReadBuffer(m_strComments, sh);
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
		file->ReadS32(&count, 1);

		Group* pGroup;
		Group* pLastGroup = NULL;
		for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
			pLastGroup = pGroup;

		pGroup = pLastGroup;
		for (i = 0; i < count; i++)
		{
			if (pGroup)
			{
				pGroup->m_pNext = new Group();
				pGroup = pGroup->m_pNext;
			}
			else
				m_pGroups = pGroup = new Group();
		}
		pLastGroup = pLastGroup ? pLastGroup->m_pNext : m_pGroups;

		for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_pNext)
		{
			if (fv < 1.0f)
			{
				file->ReadBuffer(pGroup->m_strName, 65);
				file->ReadBuffer(&ch, 1);
				pGroup->m_fCenter[0] = 0;
				pGroup->m_fCenter[1] = 0;
				pGroup->m_fCenter[2] = 0;
				pGroup->m_pGroup = (Group*)-1;
			}
			else
				pGroup->FileLoad(file);
		}

		for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_pNext)
		{
			i = LC_POINTER_TO_INT(pGroup->m_pGroup);
			pGroup->m_pGroup = NULL;

			if (i > 0xFFFF || i == -1)
				continue;

			for (Group* g2 = pLastGroup; g2; g2 = g2->m_pNext)
			{
				if (i == 0)
				{
					pGroup->m_pGroup = g2;
					break;
				}

				i--;
			}
		}


		Piece* pPiece;
		for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		{
			i = LC_POINTER_TO_INT(pPiece->GetGroup());
			pPiece->SetGroup(NULL);

			if (i > 0xFFFF || i == -1)
				continue;

			for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_pNext)
			{
				if (i == 0)
				{
					pPiece->SetGroup(pGroup);
					break;
				}

				i--;
			}
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
			m_fFogColor[0] = (float)((unsigned char) (rgb))/255;
			m_fFogColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fFogColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv < 1.0f)
			{
				file->ReadU32(&rgb, 1);
				m_fFogDensity = (float)rgb/100;
			}
			else
				file->ReadFloats(&m_fFogDensity, 1);

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
				file->ReadBuffer(m_strBackground, sh);
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
			m_fAmbient[0] = (float)((unsigned char) (rgb))/255;
			m_fAmbient[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fAmbient[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv < 1.3f)
			{
				file->ReadS32(&i, 1); m_bAnimation = (i != 0);
				file->ReadS32(&i, 1); m_bAddKeys = (i != 0);
				file->ReadU8(&m_nFPS, 1);
				file->ReadS32(&i, 1); m_nCurFrame = i;
				file->ReadU16(&m_nTotalFrames, 1);
				file->ReadS32(&i, 1); //m_nGridSize = i;
				file->ReadS32(&i, 1); //m_nMoveSnap = i;
			}
			else
			{
				file->ReadU8(&ch, 1); m_bAnimation = (ch != 0);
				file->ReadU8(&ch, 1); m_bAddKeys = (ch != 0);
				file->ReadU8(&m_nFPS, 1);
				file->ReadU16(&m_nCurFrame, 1);
				file->ReadU16(&m_nTotalFrames, 1);
				file->ReadU16(&sh, 1); // m_nGridSize = sh;
				file->ReadU16(&sh, 1);
				if (fv >= 1.4f)
					m_nMoveSnap = sh;
			}
		}

		if (fv > 1.0f)
		{
			file->ReadU32(&rgb, 1);
			m_fGradient1[0] = (float)((unsigned char) (rgb))/255;
			m_fGradient1[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fGradient1[2] = (float)((unsigned char) ((rgb) >> 16))/255;
			file->ReadU32(&rgb, 1);
			m_fGradient2[0] = (float)((unsigned char) (rgb))/255;
			m_fGradient2[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fGradient2[2] = (float)((unsigned char) ((rgb) >> 16))/255;

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

	for (i = 0; i < m_ViewList.GetSize (); i++)
	{
		m_ViewList[i]->MakeCurrent();
		RenderInitialize();
	}

	CalculateStep();

	if (!bUndo)
		SelectAndFocusNone(false);

	if (!bMerge)
		gMainWindow->UpdateFocusObject(GetFocusObject());

	if (!bMerge)
	{
		for (int ViewIdx = 0; ViewIdx < m_ViewList.GetSize(); ViewIdx++)
		{
			View* view = m_ViewList[ViewIdx];

			if (!view->mCamera->IsSimple())
				view->SetDefaultCamera();
		}

		if (!bUndo)
			ZoomExtents(0, m_ViewList.GetSize());
	}

	SetAction(action);
	gMainWindow->UpdateAnimation(m_bAnimation, m_bAddKeys);
	gMainWindow->UpdateLockSnap(m_nSnap);
	gMainWindow->UpdateSnap();
	gMainWindow->UpdateCameraMenu(mCameras, m_ActiveView ? m_ActiveView->mCamera : NULL);
	UpdateSelection();
	if (m_bAnimation)
		gMainWindow->UpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
	else
		gMainWindow->UpdateTime(m_bAnimation, m_nCurStep, 255);
	UpdateAllViews ();

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

	rgb = LC_FLOATRGB(m_fBackground);
	file->WriteU32(&rgb, 1);

	i = m_nAngleSnap; file->WriteS32(&i, 1);
	file->WriteU32(&m_nSnap, 1);
	file->WriteFloats(&m_fLineWidth, 1);
	file->WriteU32(&m_nDetail, 1);
	i = 0;//i = m_nCurGroup;
	file->WriteS32(&i, 1);
	i = 0;//i = m_nCurColor;
	file->WriteS32(&i, 1);
	i = m_nCurAction; file->WriteS32(&i, 1);
	i = m_nCurStep; file->WriteS32(&i, 1);
	file->WriteU32(&m_nScene, 1);

	Piece* pPiece;
	for (i = 0, pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		i++;
	file->WriteS32(&i, 1);

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->FileSave(*file);

	ch = strlen(m_strAuthor);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strAuthor, ch);
	ch = strlen(m_strDescription);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strDescription, ch);
	ch = strlen(m_strComments);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strComments, ch);

	Group* pGroup;
	for (i = 0, pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
		i++;
	file->WriteS32(&i, 1);

	for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
		pGroup->FileSave(file, m_pGroups);

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

	rgb = LC_FLOATRGB(m_fFogColor);
	file->WriteU32(&rgb, 1);
	file->WriteFloats(&m_fFogDensity, 1);
	sh = strlen(m_strBackground);
	file->WriteU16(&sh, 1);
	file->WriteBuffer(m_strBackground, sh);
	ch = strlen(m_strHeader);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strHeader, ch);
	ch = strlen(m_strFooter);
	file->WriteBuffer(&ch, 1);
	file->WriteBuffer(m_strFooter, ch);
	// 0.60 (1.0)
	rgb = LC_FLOATRGB(m_fAmbient);
	file->WriteU32(&rgb, 1);
	ch = m_bAnimation;
	file->WriteBuffer(&ch, 1);
	ch = m_bAddKeys;
	file->WriteU8(&ch, 1);
	file->WriteU8 (&m_nFPS, 1);
	file->WriteU16(&m_nCurFrame, 1);
	file->WriteU16(&m_nTotalFrames, 1);
	file->WriteU16(0); // m_nGridSize
	file->WriteU16(&m_nMoveSnap, 1);
	// 0.62 (1.1)
	rgb = LC_FLOATRGB(m_fGradient1);
	file->WriteU32(&rgb, 1);
	rgb = LC_FLOATRGB(m_fGradient2);
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
		m_nSaveTimer = 0;
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

		lcMatrix44 IncludeTransform(lcVector4(Matrix[3], Matrix[9], -Matrix[6], 0.0f), lcVector4(Matrix[5], Matrix[11], -Matrix[8], 0.0f),
			lcVector4(-Matrix[4], -Matrix[10], Matrix[7], 0.0f), lcVector4(Matrix[0], Matrix[2], -Matrix[1], 1.0f));

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

				lcVector4 AxisAngle = lcMatrix44ToAxisAngle(IncludeTransform);
				AxisAngle[3] *= LC_RTOD;

				pPiece->Initialize(IncludeTransform[3].x / 25.0f, IncludeTransform[3].y / 25.0f, IncludeTransform[3].z / 25.0f, *nStep, 1);
				pPiece->SetColorCode(ColorCode);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				pPiece->ChangeKey(1, false, false, AxisAngle, LC_PK_ROTATION);
				pPiece->ChangeKey(1, true, false, AxisAngle, LC_PK_ROTATION);
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

			lcVector4 AxisAngle = lcMatrix44ToAxisAngle(IncludeTransform);
			AxisAngle[3] *= LC_RTOD;

			pPiece->Initialize(IncludeTransform[3].x, IncludeTransform[3].y, IncludeTransform[3].z, *nStep, 1);
			pPiece->SetColorCode(ColorCode);
			pPiece->CreateName(m_pPieces);
			AddPiece(pPiece);
			pPiece->ChangeKey(1, false, false, AxisAngle, LC_PK_ROTATION);
			pPiece->ChangeKey(1, true, false, AxisAngle, LC_PK_ROTATION);
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
		Piece* pPiece;
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
		if (strlen(m_strAuthor) != 0)
		{
			strcat(buf, "0 Author: ");
			strcat(buf, m_strAuthor);
			strcat(buf, "\r\n");
		}
		strcat(buf, "\r\n");
		file.WriteBuffer(buf, strlen(buf));

		const char* OldLocale = setlocale(LC_NUMERIC, "C");

		for (i = 1; i <= steps; i++)
		{
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if ((pPiece->IsVisible(i, false)) && (pPiece->GetStepShow() == i))
				{
					const float* f = pPiece->mModelWorld;
					sprintf (buf, " 1 %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %s.DAT\r\n",
					         pPiece->mColorCode, f[12] * 25.0f, -f[14] * 25.0f, f[13] *25.0f, f[0], -f[8], f[4], -f[2], f[10], -f[6], f[1], -f[9], f[5], pPiece->mPieceInfo->m_strName);
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
	strcpy(m_strAuthor, lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME));
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
      gMainWindow->UpdateTime(false, m_nCurStep, 255);
	  gMainWindow->UpdateFocusObject(GetFocusObject());
      UpdateSelection();
      CalculateStep();

      ZoomExtents(0, m_ViewList.GetSize());
      UpdateAllViews();

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
  m_nSaveTimer = 0;

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
void Project::CheckPoint (const char* text)
{
	LC_UNDOINFO* pTmp;
	LC_UNDOINFO* pUndo = new LC_UNDOINFO;
	int i;

	strcpy(pUndo->strText, text);
	FileSave(&pUndo->file, true);

	for (pTmp = m_pUndoList, i = 0; pTmp; pTmp = pTmp->pNext, i++)
		if ((i == 30) && (pTmp->pNext != NULL))
		{
			delete pTmp->pNext;
			pTmp->pNext = NULL;
			m_bUndoOriginal = false;
		}

	pUndo->pNext = m_pUndoList;
	m_pUndoList = pUndo;

	while (m_pRedoList)
	{
		pUndo = m_pRedoList;
		m_pRedoList = m_pRedoList->pNext;
		delete pUndo;
	}
	m_pRedoList = NULL;

	gMainWindow->UpdateUndoRedo(m_pUndoList->pNext ? m_pUndoList->strText : NULL, NULL);
}

void Project::AddView (View* pView)
{
	m_ViewList.Add (pView);

	pView->MakeCurrent ();
	RenderInitialize ();

	if (!m_ActiveView)
		m_ActiveView = pView;
}

void Project::RemoveView (View* pView)
{
	if (pView == m_ActiveView)
		m_ActiveView = NULL;

	m_ViewList.Remove(pView);
}

void Project::UpdateAllViews()
{
	for (int i = 0; i < m_ViewList.GetSize (); i++)
		m_ViewList[i]->Redraw ();
}

// Returns true if the active view changed.
bool Project::SetActiveView(View* view)
{
	m_ActiveView = view;
	return false;
	/*
	if (view == m_ActiveView)
		return false;

//	Camera* OldCamera = NULL;
	View* OldView = m_ActiveView;
	m_ActiveView = view;

	if (OldView)
	{
		OldView->Redraw();
//		OldCamera = OldView->GetCamera();
	}

	if (view)
	{
		view->Redraw();
//		SystemUpdateCurrentCamera(OldCamera, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
	}

	return true;
	*/
}

/////////////////////////////////////////////////////////////////////////////
// Project rendering

// Only this function should be called.
void Project::Render(View* view, bool ToMemory)
{
	glViewport(0, 0, view->mWidth, view->mHeight);

	RenderBackground(view);

	// Setup the projection and camera matrices.
	float ratio = (float)view->mWidth / (float)view->mHeight;
	view->mCamera->LoadProjection(ratio);

	if (ToMemory)
		RenderScenePieces(view);
	else
	{
		if ((m_nDetail & LC_DET_FAST) && (m_nTracking != LC_TRACK_NONE))
			RenderSceneBoxes(view);
		else
			RenderScenePieces(view);

		RenderSceneObjects(view);

		if (m_OverlayActive || ((m_nCurAction == LC_ACTION_SELECT) && (m_nTracking == LC_TRACK_LEFT) && (m_ActiveView == view)))
			RenderOverlays(view);

		RenderViewports(view);
	}
}

void Project::RenderBackground(View* view)
{
	if ((m_nScene & (LC_SCENE_GRADIENT | LC_SCENE_BG)) == 0)
	{
		glClearColor(m_fBackground[0], m_fBackground[1], m_fBackground[2], 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	float ViewWidth = (float)view->mWidth;
	float ViewHeight = (float)view->mHeight;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);

	// Draw gradient quad.
	if (m_nScene & LC_SCENE_GRADIENT)
	{
		glShadeModel(GL_SMOOTH);

		float Verts[4][2];
		float Colors[4][4];

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, Verts);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 0, Colors);

		Colors[0][0] = m_fGradient1[0]; Colors[0][1] = m_fGradient1[1]; Colors[0][2] = m_fGradient1[2]; Colors[0][3] = 1.0f;
		Verts[0][0] = ViewWidth; Verts[0][1] = ViewHeight;
		Colors[1][0] = m_fGradient1[0]; Colors[1][1] = m_fGradient1[1]; Colors[1][2] = m_fGradient1[2]; Colors[1][3] = 1.0f;
		Verts[1][0] = 0; Verts[1][1] = ViewHeight;
		Colors[2][0] = m_fGradient2[0]; Colors[2][1] = m_fGradient2[1]; Colors[2][2] = m_fGradient2[2]; Colors[2][3] = 1.0f;
		Verts[2][0] = 0; Verts[2][1] = 0;
		Colors[3][0] = m_fGradient2[0]; Colors[3][1] = m_fGradient2[1]; Colors[3][2] = m_fGradient2[2]; Colors[3][3] = 1.0f;
		Verts[3][0] = ViewWidth; Verts[3][1] = 0;

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		glShadeModel(GL_FLAT);
	}

	// Draw the background picture.
	if (m_nScene & LC_SCENE_BG)
	{
		glEnable(GL_TEXTURE_2D);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, m_pBackground->mTexture);

		float Verts[4][2];
		float Coords[4][2];

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, Verts);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, Coords);

		float tw = 1.0f, th = 1.0f;
		if (m_nScene & LC_SCENE_BG_TILE)
		{
			tw = ViewWidth / m_pBackground->mWidth;
			th = ViewHeight / m_pBackground->mHeight;
		}

		Coords[0][0] = 0; Coords[0][1] = 0;
		Verts[0][0] = 0; Verts[0][1] = ViewHeight;
		Coords[1][0] = tw; Coords[1][1] = 0;
		Verts[1][0] = ViewWidth; Verts[1][1] = ViewHeight;
		Coords[2][0] = tw; Coords[2][1] = th;
		Verts[2][0] = ViewWidth; Verts[2][1] = 0;
		Coords[3][0] = 0; Coords[3][1] = th;
		Verts[3][0] = 0; Verts[3][1] = 0;

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDisable(GL_TEXTURE_2D);
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}

struct lcTranslucentRenderSection
{
	float Distance;
	Piece* piece;
};

int lcTranslucentRenderCompare(const lcTranslucentRenderSection& a, const lcTranslucentRenderSection& b)
{
	if (a.Distance > b.Distance)
		return 1;
	else
		return -1;
}

int lcOpaqueRenderCompare(Piece* const& a, Piece* const& b)
{
	if (a->mPieceInfo > b->mPieceInfo)
		return 1;
	else
		return -1;
}

void Project::RenderScenePieces(View* view)
{
	float AspectRatio = (float)view->mWidth / (float)view->mHeight;
	view->mCamera->LoadProjection(AspectRatio);

	if (m_nDetail & LC_DET_LIGHTING)
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

		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_fAmbient);

		int index = 0;
		Light *pLight;

		for (pLight = m_pLights; pLight; pLight = pLight->m_pNext, index++)
			pLight->Setup (index);

		glEnable(GL_LIGHTING);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (m_nScene & LC_SCENE_FOG)
	{
		glFogi(GL_FOG_MODE, GL_EXP);
		glFogf(GL_FOG_DENSITY, m_fFogDensity);
		glFogfv(GL_FOG_COLOR, m_fFogColor);
		glEnable(GL_FOG);
	}

	if (m_nScene & LC_SCENE_FLOOR)
		m_pTerrain->Render(view->mCamera, AspectRatio);

	lcArray<Piece*> OpaquePieces(512);
	lcArray<lcTranslucentRenderSection> TranslucentSections(512);

	const lcMatrix44& WorldView = view->mCamera->mWorldView;

	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (!pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			continue;

		bool Translucent = lcIsColorTranslucent(pPiece->mColorIndex);
		PieceInfo* Info = pPiece->mPieceInfo;

		if ((Info->mFlags & (LC_PIECE_HAS_SOLID | LC_PIECE_HAS_LINES)) || ((Info->mFlags & LC_PIECE_HAS_DEFAULT) && !Translucent))
			OpaquePieces.AddSorted(pPiece, lcOpaqueRenderCompare);

		if ((Info->mFlags & LC_PIECE_HAS_TRANSLUCENT) || ((Info->mFlags & LC_PIECE_HAS_DEFAULT) && Translucent))
		{
			lcVector3 Pos = lcMul31(pPiece->mPosition, WorldView);

			lcTranslucentRenderSection RenderSection;

			RenderSection.Distance = Pos[2];
			RenderSection.piece = pPiece;

			TranslucentSections.AddSorted(RenderSection, lcTranslucentRenderCompare);
		}
	}

	lcMesh* PreviousMesh = NULL;
	bool PreviousSelected = false;
	lcTexture* PreviousTexture = NULL;
	char* ElementsOffset = NULL;
	char* BaseBufferOffset = NULL;
	char* PreviousOffset = (char*)(~0);

	glEnableClientState(GL_VERTEX_ARRAY);

	for (int PieceIdx = 0; PieceIdx < OpaquePieces.GetSize(); PieceIdx++)
	{
		Piece* piece = OpaquePieces[PieceIdx];
		lcMesh* Mesh = piece->mPieceInfo->mMesh;

		glPushMatrix();
		glMultMatrixf(piece->mModelWorld);

		if (PreviousMesh != Mesh)
		{
			if (GL_HasVertexBufferObject())
			{
				glBindBuffer(GL_ARRAY_BUFFER_ARB, Mesh->mVertexBuffer.mBuffer);
				BaseBufferOffset = NULL;
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, Mesh->mIndexBuffer.mBuffer);
				ElementsOffset = NULL;
			}
			else
			{
				BaseBufferOffset = (char*)Mesh->mVertexBuffer.mData;
				ElementsOffset = (char*)Mesh->mIndexBuffer.mData;
			}

			PreviousMesh = Mesh;
			PreviousOffset = (char*)(~0);
		}

		if (piece->IsSelected())
		{
			if (!PreviousSelected)
				glLineWidth(2.0f * m_fLineWidth);

			PreviousSelected = true;
		}
		else
		{
			if (PreviousSelected)
				glLineWidth(m_fLineWidth);

			PreviousSelected = false;
		}

		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];
			int ColorIdx = Section->ColorIndex;

			if (Section->PrimitiveType == GL_TRIANGLES)
			{
				if (ColorIdx == gDefaultColor)
					ColorIdx = piece->mColorIndex;

				if (lcIsColorTranslucent(ColorIdx))
					continue;

				lcSetColor(ColorIdx);
			}
			else
			{
				if (piece->IsFocused())
					lcSetColorFocused();
				else if (piece->IsSelected())
					lcSetColorSelected();
				else if (ColorIdx == gEdgeColor)
					lcSetEdgeColor(piece->mColorIndex);
				else
					lcSetColor(ColorIdx);
			}

			char* BufferOffset = BaseBufferOffset;
			lcTexture* Texture = Section->Texture;

			if (!Texture)
			{
				if (PreviousTexture)
				{
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glDisable(GL_TEXTURE_2D);
				}
			}
			else
			{
				BufferOffset += Mesh->mNumVertices * sizeof(lcVertex);

				if (Texture != PreviousTexture)
				{
					glBindTexture(GL_TEXTURE_2D, Section->Texture->mTexture);

					if (!PreviousTexture)
					{
						glEnableClientState(GL_TEXTURE_COORD_ARRAY);
						glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
						glEnable(GL_TEXTURE_2D);
					}
				}
			}

			PreviousTexture = Texture;

			if (PreviousOffset != BufferOffset)
			{
				if (!Texture)
					glVertexPointer(3, GL_FLOAT, 0, BufferOffset);
				else
				{
					glVertexPointer(3, GL_FLOAT, sizeof(lcVertexTextured), BufferOffset);
					glTexCoordPointer(2, GL_FLOAT, sizeof(lcVertexTextured), BufferOffset + sizeof(lcVector3));
				}

				PreviousOffset = BufferOffset;
			}

			glDrawElements(Section->PrimitiveType, Section->NumIndices, Mesh->mIndexType, ElementsOffset + Section->IndexOffset);
		}

		glPopMatrix();
	}

	if (PreviousSelected)
		glLineWidth(m_fLineWidth);

	if (TranslucentSections.GetSize())
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);

		for (int PieceIdx = 0; PieceIdx < TranslucentSections.GetSize(); PieceIdx++)
		{
			Piece* piece = TranslucentSections[PieceIdx].piece;
			lcMesh* Mesh = piece->mPieceInfo->mMesh;

			glPushMatrix();
			glMultMatrixf(piece->mModelWorld);

			if (PreviousMesh != Mesh)
			{
				if (GL_HasVertexBufferObject())
				{
					glBindBuffer(GL_ARRAY_BUFFER_ARB, Mesh->mVertexBuffer.mBuffer);
					BaseBufferOffset = NULL;
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, Mesh->mIndexBuffer.mBuffer);
					ElementsOffset = NULL;
				}
				else
				{
					BaseBufferOffset = (char*)Mesh->mVertexBuffer.mData;
					ElementsOffset = (char*)Mesh->mIndexBuffer.mData;
				}

				PreviousMesh = Mesh;
				PreviousOffset = (char*)(~0);
			}

			for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
			{
				lcMeshSection* Section = &Mesh->mSections[SectionIdx];
				int ColorIdx = Section->ColorIndex;

				if (Section->PrimitiveType != GL_TRIANGLES)
					continue;

				if (ColorIdx == gDefaultColor)
					ColorIdx = piece->mColorIndex;

				if (!lcIsColorTranslucent(ColorIdx))
					continue;

				lcSetColor(ColorIdx);

				char* BufferOffset = BaseBufferOffset;
				lcTexture* Texture = Section->Texture;

				if (!Texture)
				{
					if (PreviousTexture)
					{
						glDisableClientState(GL_TEXTURE_COORD_ARRAY);
						glDisable(GL_TEXTURE_2D);
					}
				}
				else
				{
					BufferOffset += Mesh->mNumVertices * sizeof(lcVertex);

					if (Texture != PreviousTexture)
					{
						glBindTexture(GL_TEXTURE_2D, Section->Texture->mTexture);

						if (!PreviousTexture)
						{
							glEnableClientState(GL_TEXTURE_COORD_ARRAY);
							glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
							glEnable(GL_TEXTURE_2D);
						}
					}
				}

				PreviousTexture = Texture;

				if (PreviousOffset != BufferOffset)
				{
					if (!Texture)
						glVertexPointer(3, GL_FLOAT, 0, BufferOffset);
					else
					{
						glVertexPointer(3, GL_FLOAT, sizeof(lcVertexTextured), BufferOffset);
						glTexCoordPointer(2, GL_FLOAT, sizeof(lcVertexTextured), BufferOffset + sizeof(lcVector3));
					}

					PreviousOffset = BufferOffset;
				}

				glDrawElements(Section->PrimitiveType, Section->NumIndices, Mesh->mIndexType, ElementsOffset + Section->IndexOffset);
			}

			glPopMatrix();
		}

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	if (m_nDetail & LC_DET_LIGHTING)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(GL_FLAT);
	}

	if (m_nScene & LC_SCENE_FOG)
		glDisable(GL_FOG);

	if (PreviousTexture)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	if (GL_HasVertexBufferObject())
	{
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	glVertexPointer(3, GL_FLOAT, 0, NULL);
	glTexCoordPointer(2, GL_FLOAT, 0, NULL);
}

void Project::RenderSceneBoxes(View* view)
{
	Piece* pPiece;

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			pPiece->RenderBox(true, m_fLineWidth);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Project::RenderSceneObjects(View* view)
{
#ifdef LC_DEBUG
	RenderDebugPrimitives();
#endif

	// Draw cameras & lights
	if (m_nCurAction == LC_ACTION_INSERT || mDropPiece)
	{
		lcVector3 Pos;
		lcVector4 Rot;
		GetPieceInsertPosition(view, m_nDownX, m_nDownY, Pos, Rot);
		PieceInfo* PreviewPiece = mDropPiece ? mDropPiece : m_pCurPiece;

		glPushMatrix();
		glTranslatef(Pos[0], Pos[1], Pos[2]);
		glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);
		glLineWidth(2*m_fLineWidth);
		PreviewPiece->RenderPiece(gMainWindow->mColorIndex);
		glLineWidth(m_fLineWidth);
		glPopMatrix();
	}

	if (m_nDetail & LC_DET_LIGHTING)
	{
		glDisable (GL_LIGHTING);
		int index = 0;
		Light *pLight;

		for (pLight = m_pLights; pLight; pLight = pLight->m_pNext, index++)
			glDisable ((GLenum)(GL_LIGHT0+index));
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if ((pCamera == view->mCamera) || !pCamera->IsVisible())
			continue;

		pCamera->Render(m_fLineWidth);
	}

	for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		if (pLight->IsVisible ())
			pLight->Render(m_fLineWidth);

	if (mGridStuds || mGridLines)
	{
		const int Spacing = lcMax(mGridLineSpacing, 1);
		int MinX = 0, MaxX = 0, MinY = 0, MaxY = 0;

		if (m_pPieces || mDropPiece)
		{
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
					pPiece->CompareBoundingBox(bs);

			if (mDropPiece)
			{
				lcVector3 Position;
				lcVector4 Rotation;
				GetPieceInsertPosition(view, m_nDownX, m_nDownY, Position, Rotation);

				lcVector3 Points[8] =
				{
					lcVector3(mDropPiece->m_fDimensions[0], mDropPiece->m_fDimensions[1], mDropPiece->m_fDimensions[5]),
					lcVector3(mDropPiece->m_fDimensions[3], mDropPiece->m_fDimensions[1], mDropPiece->m_fDimensions[5]),
					lcVector3(mDropPiece->m_fDimensions[0], mDropPiece->m_fDimensions[1], mDropPiece->m_fDimensions[2]),
					lcVector3(mDropPiece->m_fDimensions[3], mDropPiece->m_fDimensions[4], mDropPiece->m_fDimensions[5]),
					lcVector3(mDropPiece->m_fDimensions[3], mDropPiece->m_fDimensions[4], mDropPiece->m_fDimensions[2]),
					lcVector3(mDropPiece->m_fDimensions[0], mDropPiece->m_fDimensions[4], mDropPiece->m_fDimensions[2]),
					lcVector3(mDropPiece->m_fDimensions[0], mDropPiece->m_fDimensions[4], mDropPiece->m_fDimensions[5]),
					lcVector3(mDropPiece->m_fDimensions[3], mDropPiece->m_fDimensions[1], mDropPiece->m_fDimensions[2])
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

		if (mGridLines)
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
			glEnable(GL_ALPHA_TEST);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);

			glColor4fv(lcVector4FromColor(mGridStudColor));

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), Verts);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), Verts + 3);

			glDrawArrays(GL_QUADS, 0, 4);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
			glDisable(GL_ALPHA_TEST);
		}

		if (mGridLines)
		{
			glColor4fv(lcVector4FromColor(mGridLineColor));

			glEnableClientState(GL_VERTEX_ARRAY);
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
			glDisableClientState(GL_VERTEX_ARRAY);
			free(Verts);
		}
	}

	// Draw axis icon
	if (m_nSnap & LC_DRAW_AXIS)
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

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, view->mWidth, 0, view->mHeight, -50, 50);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(25.375f, 25.375f, 0.0f);

		// Draw the arrows.
		lcVector3 Verts[11];
		Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);

		glEnableClientState(GL_VERTEX_ARRAY);
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

		glDisableClientState(GL_VERTEX_ARRAY);

		// Draw the text.
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_pScreenFont->MakeCurrent();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);

		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		m_pScreenFont->PrintText(pts[0][0], pts[0][1], 40.0f, "X");
		m_pScreenFont->PrintText(pts[1][0], pts[1][1], 40.0f, "Y");
		m_pScreenFont->PrintText(pts[2][0], pts[2][1], 40.0f, "Z");

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
	}
}

void Project::RenderOverlays(View* view)
{
	if (mDropPiece)
		return;

	if (((m_nCurAction == LC_ACTION_SELECT) && (m_nTracking == LC_TRACK_LEFT) && (m_ActiveView == view) && (m_OverlayMode == LC_OVERLAY_NONE)) ||
	    (m_nCurAction == LC_ACTION_ZOOM_REGION))
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0f, view->mWidth, 0.0f, view->mHeight, -1.0f, 1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375f, 0.375f, 0.0f);

		glDisable(GL_DEPTH_TEST);

		float pt1x = (float)m_nDownX;
		float pt1y = (float)m_nDownY;
		float pt2x, pt2y;

		if (m_nCurAction == LC_ACTION_ZOOM_REGION)
		{
			pt2x = m_OverlayTrackStart[0];
			pt2y = m_OverlayTrackStart[1];
		}
		else
		{
			pt2x = m_fTrack[0];
			pt2y = m_fTrack[1];
		}

		float Left, Right, Bottom, Top;

		if (pt1x < pt2x)
		{
			Left = pt1x;
			Right = pt2x;
		}
		else
		{
			Left = pt2x;
			Right = pt1x;
		}

		if (pt1y < pt2y)
		{
			Bottom = pt1y;
			Top = pt2y;
		}
		else
		{
			Bottom = pt2y;
			Top = pt1y;
		}

		float BorderX = lcMin(2.0f, Right - Left);
		float BorderY = lcMin(2.0f, Top - Bottom);

		float Verts[14][2] =
		{
			{ Left, Bottom },
			{ Left + BorderX, Bottom + BorderY },
			{ Right, Bottom },
			{ Right - BorderX, Bottom + BorderY },
			{ Right, Top },
			{ Right - BorderX, Top - BorderY },
			{ Left, Top },
			{ Left + BorderX, Top - BorderY },
			{ Left, Bottom },
			{ Left + BorderX, Bottom + BorderY },
			{ Left + BorderX, Bottom + BorderY },
			{ Right - BorderX, Bottom + BorderY },
			{ Left + BorderX, Top - BorderY },
			{ Right - BorderX, Top - BorderY },
		};

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, Verts);

		glColor4f(0.25f, 0.25f, 1.0f, 1.0f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

		glColor4f(0.25f, 0.25f, 1.0f, 0.25f);
		glDrawArrays(GL_TRIANGLE_STRIP, 10, 4);

		glDisableClientState(GL_VERTEX_ARRAY);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}

	const float OverlayScale = view->m_OverlayScale;

	if ((m_nCurAction == LC_ACTION_MOVE || m_nCurAction == LC_ACTION_SELECT) && (m_OverlayMode >= LC_OVERLAY_NONE && m_OverlayMode <= LC_OVERLAY_ROTATE_XYZ))
	{
		const float OverlayMovePlaneSize = 0.5f * OverlayScale;
		const float OverlayMoveArrowSize = 1.5f * OverlayScale;
		const float OverlayMoveArrowCapSize = 0.9f * OverlayScale;
		const float OverlayMoveArrowCapRadius = 0.1f * OverlayScale;
		const float OverlayMoveArrowBodyRadius = 0.05f * OverlayScale;
		const float OverlayRotateArrowStart = 1.0f * OverlayScale;
		const float OverlayRotateArrowEnd = 1.5f * OverlayScale;
		const float OverlayRotateArrowCenter = 1.2f * OverlayScale;

		glDisable(GL_DEPTH_TEST);

		// Find the rotation from the focused piece if relative snap is enabled.
		Object* Focus = NULL;
		lcVector4 Rot(0, 0, 1, 0);

		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPiece())
				Rot = ((Piece*)Focus)->mRotation;
			else
				Focus = NULL;
		}

		// Draw a quad if we're moving on a plane.
		if ((m_OverlayMode == LC_OVERLAY_MOVE_XY) || (m_OverlayMode == LC_OVERLAY_MOVE_XZ) || (m_OverlayMode == LC_OVERLAY_MOVE_YZ))
		{
			glPushMatrix();
			glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

			if (Focus)
				glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

			if (m_OverlayMode == LC_OVERLAY_MOVE_XZ)
				glRotatef(90.0f, 0.0f, 0.0f, -1.0f);
			else if (m_OverlayMode == LC_OVERLAY_MOVE_XY)
				glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);

			glColor4f(0.8f, 0.8f, 0.0f, 0.3f);

			float Verts[4][3] =
			{
				{ 0.0f, 0.0f, 0.0f },
				{ 0.0f, OverlayMovePlaneSize, 0.0f },
				{ 0.0f, OverlayMovePlaneSize, OverlayMovePlaneSize },
				{ 0.0f, 0.0f, OverlayMovePlaneSize }
			};

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			glDisableClientState(GL_VERTEX_ARRAY);

			glDisable(GL_BLEND);

			glPopMatrix();
		}

		// Draw the arrows.
		for (int i = 0; i < 3; i++)
		{
			switch (i)
			{
			case 0:
				if ((m_OverlayMode == LC_OVERLAY_MOVE_X) || (m_OverlayMode == LC_OVERLAY_MOVE_XY) || (m_OverlayMode == LC_OVERLAY_MOVE_XZ))
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else
					glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
				break;
			case 1:
				if ((m_OverlayMode == LC_OVERLAY_MOVE_Y) || (m_OverlayMode == LC_OVERLAY_MOVE_XY) || (m_OverlayMode == LC_OVERLAY_MOVE_YZ))
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else
					glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
				break;
			case 2:
				if ((m_OverlayMode == LC_OVERLAY_MOVE_Z) || (m_OverlayMode == LC_OVERLAY_MOVE_XZ) || (m_OverlayMode == LC_OVERLAY_MOVE_YZ))
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else
					glColor4f(0.0f, 0.0f, 0.8f, 1.0f);
				break;
			}

			glPushMatrix();
			glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

			if (Focus)
				glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

			if (i == 1)
				glMultMatrixf(lcMatrix44(lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 1, 0), lcVector4(0, 0, 0, 1)));
			else if (i == 2)
				glMultMatrixf(lcMatrix44(lcVector4(0, 0, 1, 0), lcVector4(0, 1, 0, 0), lcVector4(1, 0, 0, 0), lcVector4(0, 0, 0, 1)));

			// Translation arrows.
			if (m_nTracking == LC_TRACK_NONE || (m_OverlayMode >= LC_OVERLAY_NONE && m_OverlayMode <= LC_OVERLAY_MOVE_XYZ))
			{
				lcVector3 Verts[11];

				Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);
				Verts[1] = lcVector3(OverlayMoveArrowSize, 0.0f, 0.0f);

				for (int j = 0; j < 9; j++)
				{
					float y = cosf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					float z = sinf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					Verts[j + 2] = lcVector3(OverlayMoveArrowCapSize, y, z);
				}

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, Verts);
				glDrawArrays(GL_LINES, 0, 2);
				glDrawArrays(GL_TRIANGLE_FAN, 1, 10);
				glDisableClientState(GL_VERTEX_ARRAY);
			}

			// Rotation arrows.
			if (m_nCurAction == LC_ACTION_SELECT && m_nTracking == LC_TRACK_NONE)
			{
				switch (i)
				{
				case 0:
					if (m_OverlayMode == LC_OVERLAY_ROTATE_X)
						glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
					else
						glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
					break;
				case 1:
					if (m_OverlayMode == LC_OVERLAY_ROTATE_Y)
						glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
					else
						glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
					break;
				case 2:
					if (m_OverlayMode == LC_OVERLAY_ROTATE_Z)
						glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
					else
						glColor4f(0.0f, 0.0f, 0.8f, 1.0f);
					break;
				}

				lcVector3 Verts[18];
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, Verts);

				for (int j = 0; j < 9; j++)
				{
					const float Radius1 = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter - OverlayMoveArrowBodyRadius;
					const float Radius2 = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter + OverlayMoveArrowBodyRadius;
					float x = cosf(LC_2PI / 4 * j / 8);
					float y = sinf(LC_2PI / 4 * j / 8);

					Verts[j * 2 + 0] = lcVector3(0.0f, OverlayRotateArrowCenter + x * Radius1, OverlayRotateArrowCenter + y * Radius1);
					Verts[j * 2 + 1] = lcVector3(0.0f, OverlayRotateArrowCenter + x * Radius2, OverlayRotateArrowCenter + y * Radius2);
				}

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 18);

				for (int j = 0; j < 9; j++)
				{
					const float Radius = OverlayRotateArrowEnd - OverlayMoveArrowCapRadius - OverlayRotateArrowCenter;
					float x = cosf(LC_2PI / 4 * j / 8);
					float y = sinf(LC_2PI / 4 * j / 8);

					Verts[j * 2 + 0] = lcVector3(-OverlayMoveArrowBodyRadius, OverlayRotateArrowCenter + x * Radius, OverlayRotateArrowCenter + y * Radius);
					Verts[j * 2 + 1] = lcVector3( OverlayMoveArrowBodyRadius, OverlayRotateArrowCenter + x * Radius, OverlayRotateArrowCenter + y * Radius);
				}

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 18);

				Verts[0] = lcVector3(0.0f, OverlayRotateArrowEnd - OverlayMoveArrowCapRadius, OverlayRotateArrowStart);

				for (int j = 0; j < 9; j++)
				{
					float x = cosf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					float y = sinf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					Verts[j + 1] = lcVector3(x, OverlayRotateArrowEnd - OverlayMoveArrowCapRadius + y, OverlayRotateArrowCenter);
				}

				glDrawArrays(GL_TRIANGLE_FAN, 0, 10);

				Verts[0] = lcVector3(0.0f, OverlayRotateArrowStart, OverlayRotateArrowEnd - OverlayMoveArrowCapRadius);

				for (int j = 0; j < 9; j++)
				{
					float x = cosf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					float y = sinf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius;
					Verts[j + 1] = lcVector3(x, OverlayRotateArrowCenter, OverlayRotateArrowEnd - OverlayMoveArrowCapRadius + y);
				}

				glDrawArrays(GL_TRIANGLE_FAN, 0, 10);

				glDisableClientState(GL_VERTEX_ARRAY);
			}

			glPopMatrix();
		}

		glEnable(GL_DEPTH_TEST);
	}

	if (m_nCurAction == LC_ACTION_ROTATE || (m_nCurAction == LC_ACTION_SELECT && m_nTracking != LC_TRACK_NONE && m_OverlayMode >= LC_OVERLAY_ROTATE_X && m_OverlayMode <= LC_OVERLAY_ROTATE_XYZ))
	{
		const float OverlayRotateRadius = 2.0f;

		glDisable(GL_DEPTH_TEST);

		Camera* Cam = view->mCamera;
		int j;

		// Find the rotation from the focused piece if relative snap is enabled.
		Object* Focus = NULL;
		lcVector4 Rot(0, 0, 1, 0);

		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPiece())
				Rot = ((Piece*)Focus)->mRotation;
			else
				Focus = NULL;
		}

		bool HasAngle = false;

		// Draw a disc showing the rotation amount.
		if (m_MouseTotalDelta.LengthSquared() != 0.0f && (m_nTracking != LC_TRACK_NONE))
		{
			lcVector4 Rotation;
			float Angle, Step;

			HasAngle = true;

			switch (m_OverlayMode)
			{
			case LC_OVERLAY_ROTATE_X:
				glColor4f(0.8f, 0.0f, 0.0f, 0.3f);
				Angle = m_MouseTotalDelta[0];
				Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
				break;
			case LC_OVERLAY_ROTATE_Y:
				glColor4f(0.0f, 0.8f, 0.0f, 0.3f);
				Angle = m_MouseTotalDelta[1];
				Rotation = lcVector4(90.0f, 0.0f, 0.0f, 1.0f);
				break;
			case LC_OVERLAY_ROTATE_Z:
				glColor4f(0.0f, 0.0f, 0.8f, 0.3f);
				Angle = m_MouseTotalDelta[2];
				Rotation = lcVector4(90.0f, 0.0f, -1.0f, 0.0f);
				break;
			default:
				Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
				Angle = 0.0f;
				break;
			};

			if (Angle > 0.0f)
			{
				Step = 360.0f / 32;
			}
			else
			{
				Angle = -Angle;
				Step = -360.0f / 32;
			}

			if (fabsf(Angle) >= fabsf(Step))
			{
				glPushMatrix();
				glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

				if (Focus)
					glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

				glRotatef(Rotation[0], Rotation[1], Rotation[2], Rotation[3]);

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);

				lcVector3 Verts[33];
				Verts[0] = lcVector3(0.0f, 0.0f, 0.0f);
				int NumVerts = 1;

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, Verts);

				float StartAngle;
				int i = 0;

				if (Step < 0)
					StartAngle = -Angle;
				else
					StartAngle = Angle;

				do
				{
					float x = cosf((Step * i - StartAngle) * LC_DTOR) * OverlayRotateRadius * OverlayScale;
					float y = sinf((Step * i - StartAngle) * LC_DTOR) * OverlayRotateRadius * OverlayScale;

					Verts[NumVerts++] = lcVector3(0.0f, x, y);

					if (NumVerts == 33)
					{
						glDrawArrays(GL_TRIANGLE_FAN, 0, NumVerts);
						Verts[1] = Verts[32];
						NumVerts = 2;
					}

					i++;
					if (Step > 0)
						Angle -= Step;
					else
						Angle += Step;

				} while (Angle >= 0.0f);

				if (NumVerts > 2)
					glDrawArrays(GL_TRIANGLE_FAN, 0, NumVerts);

				glDisableClientState(GL_VERTEX_ARRAY);
				glDisable(GL_BLEND);

				glPopMatrix();
			}
		}

		glPushMatrix();

		lcMatrix44 Mat = lcMatrix44AffineInverse(Cam->mWorldView);
		Mat.SetTranslation(m_OverlayCenter);

		// Draw the circles.
		if (m_nCurAction == LC_ACTION_ROTATE && !HasAngle && m_nTracking == LC_TRACK_NONE)
		{
			lcVector3 Verts[32];

			for (j = 0; j < 32; j++)
			{
				lcVector3 Pt;

				Pt[0] = cosf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
				Pt[1] = sinf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
				Pt[2] = 0.0f;

				Verts[j] = lcMul31(Pt, Mat);
			}

			glColor4f(0.1f, 0.1f, 0.1f, 1.0f);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINE_LOOP, 0, 32);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		lcVector3 ViewDir = Cam->mTargetPosition - Cam->mPosition;
		ViewDir.Normalize();

		// Transform ViewDir to local space.
		if (Focus)
		{
			lcMatrix44 RotMat = lcMatrix44FromAxisAngle(lcVector3(Rot[0], Rot[1], Rot[2]), -Rot[3] * LC_DTOR);

			ViewDir = lcMul30(ViewDir, RotMat);
		}

		glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

		if (Focus)
			glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

		// Draw each axis circle.
		for (int i = 0; i < 3; i++)
		{
			if (m_OverlayMode == LC_OVERLAY_ROTATE_X + i)
			{
				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
			}
			else
			{
				if (m_nCurAction != LC_ACTION_ROTATE || HasAngle || m_nTracking != LC_TRACK_NONE)
					continue;

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
			}

			lcVector3 Verts[64];
			int NumVerts = 0;

			for (j = 0; j < 32; j++)
			{
				lcVector3 v1, v2;

				switch (i)
				{
				case 0:
					v1 = lcVector3(0.0f, cosf(LC_2PI * j / 32), sinf(LC_2PI * j / 32));
					v2 = lcVector3(0.0f, cosf(LC_2PI * (j + 1) / 32), sinf(LC_2PI * (j + 1) / 32));
					break;

				case 1:
					v1 = lcVector3(cosf(LC_2PI * j / 32), 0.0f, sinf(LC_2PI * j / 32));
					v2 = lcVector3(cosf(LC_2PI * (j + 1) / 32), 0.0f, sinf(LC_2PI * (j + 1) / 32));
					break;

				case 2:
					v1 = lcVector3(cosf(LC_2PI * j / 32), sinf(LC_2PI * j / 32), 0.0f);
					v2 = lcVector3(cosf(LC_2PI * (j + 1) / 32), sinf(LC_2PI * (j + 1) / 32), 0.0f);
					break;
				}

				if (m_nCurAction != LC_ACTION_ROTATE || HasAngle || m_nTracking != LC_TRACK_NONE || lcDot(ViewDir, v1 + v2) <= 0.0f)
				{
					Verts[NumVerts++] = v1 * (OverlayRotateRadius * OverlayScale);
					Verts[NumVerts++] = v2 * (OverlayRotateRadius * OverlayScale);
				}
			}

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINES, 0, NumVerts);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		glPopMatrix();

		// Draw tangent vector.
		if (m_nTracking != LC_TRACK_NONE)
		{
			if ((m_OverlayMode == LC_OVERLAY_ROTATE_X) || (m_OverlayMode == LC_OVERLAY_ROTATE_Y) || (m_OverlayMode == LC_OVERLAY_ROTATE_Z))
			{
				const float OverlayRotateArrowSize = 1.5f;
				const float OverlayRotateArrowCapSize = 0.25f;

				lcVector4 Rotation;
				float Angle;

				switch (m_OverlayMode)
				{
				case LC_OVERLAY_ROTATE_X:
					Angle = m_MouseTotalDelta[0];
					Rotation = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
					break;
				case LC_OVERLAY_ROTATE_Y:
					Angle = m_MouseTotalDelta[1];
					Rotation = lcVector4(90.0f, 0.0f, 0.0f, 1.0f);
					break;
				case LC_OVERLAY_ROTATE_Z:
					Angle = m_MouseTotalDelta[2];
					Rotation = lcVector4(90.0f, 0.0f, -1.0f, 0.0f);
					break;
				default:
					Angle = 0.0f;
					Rotation = lcVector4(0.0f, 0.0f, 1.0f, 0.0f);
					break;
				};

				glPushMatrix();
				glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

				if (Focus)
					glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

				glRotatef(Rotation[0], Rotation[1], Rotation[2], Rotation[3]);

				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);

				if (HasAngle)
				{
					float StartY = OverlayScale * OverlayRotateRadius;
					float EndZ = (Angle > 0.0f) ? OverlayScale * OverlayRotateArrowSize : -OverlayScale * OverlayRotateArrowSize;
					float TipZ = (Angle > 0.0f) ? -OverlayScale * OverlayRotateArrowCapSize : OverlayScale * OverlayRotateArrowCapSize;

					lcVector3 Verts[6];

					Verts[0] = lcVector3(0.0f, StartY, 0.0f);
					Verts[1] = lcVector3(0.0f, StartY, EndZ);

					Verts[2] = lcVector3(0.0f, StartY, EndZ);
					Verts[3] = lcVector3(0.0f, StartY + OverlayScale * OverlayRotateArrowCapSize, EndZ + TipZ);

					Verts[4] = lcVector3(0.0f, StartY, EndZ);
					Verts[5] = lcVector3(0.0f, StartY - OverlayScale * OverlayRotateArrowCapSize, EndZ + TipZ);

					glEnableClientState(GL_VERTEX_ARRAY);
					glVertexPointer(3, GL_FLOAT, 0, Verts);
					glDrawArrays(GL_LINES, 0, 6);
					glDisableClientState(GL_VERTEX_ARRAY);
				}

				glPopMatrix();

				// Draw text.
				int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
				float Aspect = (float)Viewport[2]/(float)Viewport[3];

				const lcMatrix44& ModelView = Cam->mWorldView;
				lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

				lcVector3 ScreenPos = lcProjectPoint(m_OverlayCenter, ModelView, Projection, Viewport);

				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glOrtho(0, Viewport[2], 0, Viewport[3], -1, 1);
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				glTranslatef(0.375, 0.375, 0.0);

				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				m_pScreenFont->MakeCurrent();
				glEnable(GL_TEXTURE_2D);
				glEnable(GL_ALPHA_TEST);

				char buf[32];
				sprintf(buf, "[%.2f]", fabsf(Angle));

				int cx, cy;
				m_pScreenFont->GetStringDimensions(&cx, &cy, buf);

				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				m_pScreenFont->PrintText(ScreenPos[0] - Viewport[0] - (cx / 2), ScreenPos[1] - Viewport[1] + (cy / 2), 0.0f, buf);

				glDisable(GL_TEXTURE_2D);
				glDisable(GL_ALPHA_TEST);

				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}

		glEnable(GL_DEPTH_TEST);
	}
	else if (m_nCurAction == LC_ACTION_ROTATE_VIEW)
	{
		int x, y, w, h;

		x = 0;
		y = 0;
		w = view->mWidth;
		h = view->mHeight;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375f, 0.375f, 0.0f);

		glDisable(GL_DEPTH_TEST);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

		// Draw circle.
		float verts[32][2];

		float r = lcMin(w, h) * 0.35f;
		float cx = x + w / 2.0f;
		float cy = y + h / 2.0f;

		for (int i = 0; i < 32; i++)
		{
			verts[i][0] = cosf((float)i / 32.0f * (2.0f * LC_PI)) * r + cx;
			verts[i][1] = sinf((float)i / 32.0f * (2.0f * LC_PI)) * r + cy;
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, verts);
		glDrawArrays(GL_LINE_LOOP, 0, 32);

		const float OverlayCameraSquareSize = lcMax(8.0f, (w+h)/200);

		// Draw squares.
		float Squares[16][2] =
		{
			{ cx + OverlayCameraSquareSize, cy + r + OverlayCameraSquareSize },
			{ cx - OverlayCameraSquareSize, cy + r + OverlayCameraSquareSize },
			{ cx - OverlayCameraSquareSize, cy + r - OverlayCameraSquareSize },
			{ cx + OverlayCameraSquareSize, cy + r - OverlayCameraSquareSize },
			{ cx + OverlayCameraSquareSize, cy - r + OverlayCameraSquareSize },
			{ cx - OverlayCameraSquareSize, cy - r + OverlayCameraSquareSize },
			{ cx - OverlayCameraSquareSize, cy - r - OverlayCameraSquareSize },
			{ cx + OverlayCameraSquareSize, cy - r - OverlayCameraSquareSize },
			{ cx + r + OverlayCameraSquareSize, cy + OverlayCameraSquareSize },
			{ cx + r - OverlayCameraSquareSize, cy + OverlayCameraSquareSize },
			{ cx + r - OverlayCameraSquareSize, cy - OverlayCameraSquareSize },
			{ cx + r + OverlayCameraSquareSize, cy - OverlayCameraSquareSize },
			{ cx - r + OverlayCameraSquareSize, cy + OverlayCameraSquareSize },
			{ cx - r - OverlayCameraSquareSize, cy + OverlayCameraSquareSize },
			{ cx - r - OverlayCameraSquareSize, cy - OverlayCameraSquareSize },
			{ cx - r + OverlayCameraSquareSize, cy - OverlayCameraSquareSize }
		};

		glVertexPointer(2, GL_FLOAT, 0, Squares);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		glDrawArrays(GL_LINE_LOOP, 4, 4);
		glDrawArrays(GL_LINE_LOOP, 8, 4);
		glDrawArrays(GL_LINE_LOOP, 12, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
		glEnable(GL_DEPTH_TEST);
	}
}

void Project::RenderViewports(View* view)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, view->mWidth, 0.0f, view->mHeight, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	// Draw camera name
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	m_pScreenFont->MakeCurrent();
	glEnable(GL_ALPHA_TEST);

	m_pScreenFont->PrintText(3.0f, (float)view->mHeight - 1.0f - 6.0f, 0.0f, view->mCamera->GetName());

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

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

	glAlphaFunc(GL_GREATER, 0.0625);

	if (m_nScene & LC_SCENE_FLOOR)
		m_pTerrain->LoadTexture();

	if (m_nScene & LC_SCENE_BG)
		if (!m_pBackground->Load(m_strBackground, LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV))
		{
			m_nScene &= ~LC_SCENE_BG;
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

void Project::AddPiece(Piece* pPiece)
{
	if (m_pPieces != NULL)
	{
		pPiece->m_pNext = m_pPieces;
		m_pPieces = pPiece;
	}
	else
	{
		m_pPieces = pPiece;
		pPiece->m_pNext = NULL;
	}
}

void Project::RemovePiece(Piece* pPiece)
{
	Piece* pTemp, *pLast;
	pLast = NULL;

	for (pTemp = m_pPieces; pTemp; pLast = pTemp, pTemp = pTemp->m_pNext)
		if (pTemp == pPiece)
		{
			if (pLast != NULL)
				pLast->m_pNext = pTemp->m_pNext;
			else
				m_pPieces = pTemp->m_pNext;

			break;
		}
}

void Project::CalculateStep()
{
	Piece* pPiece;
	Light* pLight;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		mCameras[CameraIdx]->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		pLight->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
}

// Returns true if anything was removed (used by cut and del)
bool Project::RemoveSelectedObjects()
{
	Piece* pPiece;
	Light* pLight;
	void* pPrev;
	bool RemovedPiece = false;
	bool RemovedCamera = false;
	bool RemovedLight = false;

	pPiece = m_pPieces;
	while (pPiece)
	{
		if (pPiece->IsSelected())
		{
			Piece* pTemp;
			pTemp = pPiece->m_pNext;

			RemovedPiece = true;
			RemovePiece(pPiece);
			delete pPiece;
			pPiece = pTemp;
		}
		else
			pPiece = pPiece->m_pNext;
	}

	// Cameras can't be removed while being used or default
	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if (!pCamera->IsSelected())
			continue;

		bool CanDelete = true;

		for (int ViewIdx = 0; ViewIdx < m_ViewList.GetSize(); ViewIdx++)
		{
			if (pCamera == m_ViewList[ViewIdx]->mCamera)
			{
				CanDelete = false;
				break;
			}
		}

		if (!CanDelete)
			continue;

		mCameras.RemoveIndex(CameraIdx);
		CameraIdx--;
		delete pCamera;

		RemovedCamera = true;
	}

	if (RemovedCamera)
		gMainWindow->UpdateCameraMenu(mCameras, m_ActiveView ? m_ActiveView->mCamera : NULL);

	for (pPrev = NULL, pLight = m_pLights; pLight; )
	{
		if (pLight->IsSelected())
		{
			if (pPrev)
			{
			  ((Light*)pPrev)->m_pNext = pLight->m_pNext;
				delete pLight;
				pLight = ((Light*)pPrev)->m_pNext;
			}
			else
			{
			  m_pLights = m_pLights->m_pNext;
				delete pLight;
				pLight = m_pLights;
			}

			RemovedLight = true;
		}
		else
		{
			pPrev = pLight;
			pLight = pLight->m_pNext;
		}
	}

	RemoveEmptyGroups();
//	CalculateStep();
//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);

	return RemovedPiece || RemovedCamera || RemovedLight;
}

void Project::UpdateSelection()
{
	unsigned long flags = 0;
	int SelectedCount = 0;
	Object* Focus = NULL;

	if (m_pPieces == NULL)
		flags |= LC_SEL_NO_PIECES;
	else
	{
		Piece* pPiece;
		Group* pGroup = NULL;
		bool first = true;

		for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		{
			if (pPiece->IsSelected())
			{
				SelectedCount++;

				if (pPiece->IsFocused())
					Focus = pPiece;

				if (flags & LC_SEL_PIECE)
					flags |= LC_SEL_MULTIPLE;
				else
					flags |= LC_SEL_PIECE;

				if (pPiece->GetGroup() != NULL)
				{
					flags |= LC_SEL_GROUP;
					if (pPiece->IsFocused())
						flags |= LC_SEL_FOCUSGROUP;
				}

				if (first)
				{
					pGroup = pPiece->GetGroup();
					first = false;
				}
				else
				{
					if (pGroup != pPiece->GetGroup())
						flags |= LC_SEL_CANGROUP;
					else
						if (pGroup == NULL)
							flags |= LC_SEL_CANGROUP;
				}
			}
			else
			{
				flags |= LC_SEL_UNSELECTED;

				if (pPiece->IsHidden())
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

			if (pCamera->IsEyeFocused() || pCamera->IsTargetFocused())
				Focus = pCamera;
		}
	}

	for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		if (pLight->IsSelected())
		{
			flags |= LC_SEL_LIGHT;
			SelectedCount++;

			if (pLight->IsEyeFocused() || pLight->IsTargetFocused())
				Focus = pLight;
		}

	if (m_nTracking == LC_TRACK_NONE)
	{
		ActivateOverlay(m_ActiveView, m_nCurAction, LC_OVERLAY_NONE);
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
	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		last = lcMax(last, pPiece->GetStepShow());

	return last;
}

void Project::FindPiece(bool FindFirst, bool SearchForward)
{
	if (!m_pPieces)
		return;

	Piece* Start = NULL;
	if (!FindFirst)
	{
		for (Start = m_pPieces; Start; Start = Start->m_pNext)
			if (Start->IsFocused())
				break;

		if (Start && !Start->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			Start = NULL;
	}

	SelectAndFocusNone(false);

	Piece* Current = Start;

	for (;;)
	{
		if (SearchForward)
		{
			Current = Current ? Current->m_pNext : m_pPieces;
		}
		else
		{
			if (Current == m_pPieces)
				Current = NULL;

			for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
			{
				if (piece->m_pNext == Current)
				{
					Current = piece;
					break;
				}
			}
		}

		if (Current == Start)
			break;

		if (!Current)
			continue;

		if (!Current->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			continue;

		if ((!mSearchOptions.MatchInfo || Current->mPieceInfo == mSearchOptions.Info) &&
			(!mSearchOptions.MatchColor || Current->mColorIndex == mSearchOptions.ColorIndex) &&
			(!mSearchOptions.MatchName || strcasestr(Current->GetName(), mSearchOptions.Name)))
		{
			Current->Select(true, true, false);
			Group* TopGroup = Current->GetTopGroup();
			if (TopGroup)
			{
				for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
					if ((piece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation)) &&
						(piece->GetTopGroup() == TopGroup))
						piece->Select (true, false, false);
			}

			UpdateSelection();
			UpdateAllViews();
			gMainWindow->UpdateFocusObject(Current);

			break;
		}
	}
}

void Project::ZoomExtents(int FirstView, int LastView)
{
	if (!m_pPieces)
		return;

	float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			pPiece->CompareBoundingBox(bs);

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

	for (int vp = FirstView; vp < LastView; vp++)
	{
		View* view = m_ViewList[vp];

		view->mCamera->ZoomExtents(view, Center, Points, 8, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
	}

	gMainWindow->UpdateFocusObject(GetFocusObject());
	UpdateOverlayScale();
	UpdateAllViews();
}

void Project::GetPiecesUsed(lcArray<lcPiecesUsedEntry>& PiecesUsed) const
{
	for (Piece* Piece = m_pPieces; Piece; Piece = Piece->m_pNext)
	{
		if (Piece->mPieceInfo->m_strDescription[0] == '~')
			continue;

		int PieceIdx;

		for (PieceIdx = 0; PieceIdx < PiecesUsed.GetSize(); PieceIdx++)
		{
			if (PiecesUsed[PieceIdx].Info != Piece->mPieceInfo || PiecesUsed[PieceIdx].ColorIndex != Piece->mColorIndex)
				continue;

			PiecesUsed[PieceIdx].Count++;
			break;
		}

		if (PieceIdx == PiecesUsed.GetSize())
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
	oldtime = m_bAnimation ? m_nCurFrame : m_nCurStep;

	View view(this);
	view.SetCamera(m_ActiveView->mCamera, false);
	view.mWidth = width;
	view.mHeight = height;

	if (!hilite)
		SelectAndFocusNone(false);

	RenderInitialize();

	for (int i = from; i <= to; i++)
	{
		if (m_bAnimation)
			m_nCurFrame = i;
		else
			m_nCurStep = i;

		if (hilite)
		{
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if ((m_bAnimation && pPiece->GetFrameShow() == i) ||
					(!m_bAnimation && pPiece->GetStepShow() == i))
					pPiece->Select (true, false, false);
				else
					pPiece->Select (false, false, false);
			}
		}

		CalculateStep();
		Render(&view, true);
		images[i-from].FromOpenGL (width, height);
	}

	if (m_bAnimation)
		m_nCurFrame = oldtime;
	else
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

	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if ((pPiece->GetStepShow() == nStep) || (nStep == 0))
			ColorsUsed[pPiece->mColorIndex]++;
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

		for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		{
			if ((pPiece->mPieceInfo == pInfo) && ((pPiece->GetStepShow() == nStep) || (nStep == 0)))
			{
				PiecesUsed[pPiece->mColorIndex]++;
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
	if (!m_pPieces)
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

	File.WriteFloat(m_fAmbient[0]);
	File.WriteFloat(m_fAmbient[1]);
	File.WriteFloat(m_fAmbient[2]);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fAmbient[0]);
	File.WriteFloat(m_fAmbient[1]);
	File.WriteFloat(m_fAmbient[2]);

	File.WriteU16(0x1200); // CHK_SOLID_BGND
	File.WriteU32(42);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fBackground[0]);
	File.WriteFloat(m_fBackground[1]);
	File.WriteFloat(m_fBackground[2]);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fBackground[0]);
	File.WriteFloat(m_fBackground[1]);
	File.WriteFloat(m_fBackground[2]);

	File.WriteU16(0x1100); // CHK_BIT_MAP
	File.WriteU32(6 + 1 + strlen(m_strBackground));
	File.WriteBuffer(m_strBackground, strlen(m_strBackground) + 1);

	File.WriteU16(0x1300); // CHK_V_GRADIENT
	File.WriteU32(118);

	File.WriteFloat(1.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fGradient1[0]);
	File.WriteFloat(m_fGradient1[1]);
	File.WriteFloat(m_fGradient1[2]);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fGradient1[0]);
	File.WriteFloat(m_fGradient1[1]);
	File.WriteFloat(m_fGradient1[2]);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat((m_fGradient1[0] + m_fGradient2[0]) / 2.0f);
	File.WriteFloat((m_fGradient1[1] + m_fGradient2[1]) / 2.0f);
	File.WriteFloat((m_fGradient1[2] + m_fGradient2[2]) / 2.0f);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat((m_fGradient1[0] + m_fGradient2[0]) / 2.0f);
	File.WriteFloat((m_fGradient1[1] + m_fGradient2[1]) / 2.0f);
	File.WriteFloat((m_fGradient1[2] + m_fGradient2[2]) / 2.0f);

	File.WriteU16(0x0010); // CHK_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fGradient2[0]);
	File.WriteFloat(m_fGradient2[1]);
	File.WriteFloat(m_fGradient2[2]);

	File.WriteU16(0x0013); // CHK_LIN_COLOR_F
	File.WriteU32(18);

	File.WriteFloat(m_fGradient2[0]);
	File.WriteFloat(m_fGradient2[1]);
	File.WriteFloat(m_fGradient2[2]);

	if (m_nScene & LC_SCENE_GRADIENT)
	{
		File.WriteU16(0x1301); // LIB3DS_USE_V_GRADIENT
		File.WriteU32(6);
	}
	else if (m_nScene & LC_SCENE_BG)
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

	File.WriteFloat(m_fFogColor[0]);
	File.WriteFloat(m_fFogColor[1]);
	File.WriteFloat(m_fFogColor[2]);

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

	File.WriteFloat(m_fFogColor[0]);
	File.WriteFloat(m_fFogColor[1]);
	File.WriteFloat(m_fFogColor[2]);

	File.WriteU16(0x2300); // CHK_DISTANCE_CUE
	File.WriteU32(28);

	File.WriteFloat(0.0f);
	File.WriteFloat(0.0f);
	File.WriteFloat(1000.0f);
	File.WriteFloat(100.0f);

	File.WriteU16(0x2310); // CHK_DICHK_DCUE_BGNDSTANCE_CUE
	File.WriteU32(6);

	int NumPieces = 0;
	for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
	{
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

		for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
		{
			PieceInfo* Info = piece->mPieceInfo;

			for (Piece* FirstPiece = m_pPieces; FirstPiece; FirstPiece = FirstPiece->m_pNext)
			{
				if (FirstPiece->mPieceInfo != Info)
					continue;

				if (FirstPiece != piece)
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
	for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
	{
		PieceInfo* Info = piece->mPieceInfo;
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

	const lcVector3& Position = m_ActiveView->mCamera->mPosition;
	const lcVector3& Target = m_ActiveView->mCamera->mTargetPosition;
	const lcVector3& Up = m_ActiveView->mCamera->mUpVector;

	sprintf(Line, "camera {\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f\n}\n\n",
			Up[0], Up[1], Up[2], Position[1], Position[0], Position[2], Target[1], Target[0], Target[2], m_ActiveView->mCamera->m_fovy);
	POVFile.WriteLine(Line);
	sprintf(Line, "background { color rgb <%1g, %1g, %1g> }\n\nlight_source { <0, 0, 20> White shadowless }\n\n",
			m_fBackground[0], m_fBackground[1], m_fBackground[2]);
	POVFile.WriteLine(Line);

	for (Piece* piece = m_pPieces; piece; piece = piece->m_pNext)
	{
		int Index = Library->mPieces.FindIndex(piece->mPieceInfo);
		int Color;

		Color = piece->mColorIndex;
		const char* Suffix = lcIsColorTranslucent(Color) ? "_clear" : "";

		const float* f = piece->mModelWorld;

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

// Special notifications.
void Project::HandleNotify(LC_NOTIFY id, unsigned long param)
{
	switch (id)
	{
		case LC_CAPTURE_LOST:
	{
			if (m_nTracking != LC_TRACK_NONE)
				StopTracking(false);
		} break;
	}
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
			UpdateAllViews ();
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

			if (m_bAnimation)
			{
				Options.Start = 1;
				Options.End = m_nTotalFrames;
			}
			else
			{
				Options.Start = m_nCurStep;
				Options.End = m_nCurStep;
			}

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

			if (m_bAnimation)
				Options.End = lcMin(Options.End, m_nTotalFrames);
			else
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

					Piece *p1, *p2;
					PieceInfo* pInfo;
					for (p1 = m_pPieces; p1; p1 = p1->m_pNext)
					{
						bool bSkip = false;
						pInfo = p1->mPieceInfo;

						for (p2 = m_pPieces; p2; p2 = p2->m_pNext)
						{
							if (p2 == p1)
								break;

							if (p2->mPieceInfo == pInfo)
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
			if (!m_pPieces)
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
			if (!m_pPieces)
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
			Piece* pPiece;

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

			if (strlen(m_strAuthor))
			{
				sprintf(Line, "# Author: %s\n", m_strAuthor);
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

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				const lcMatrix44& ModelWorld = pPiece->mModelWorld;
				PieceInfo* pInfo = pPiece->mPieceInfo;
				float* Verts = (float*)pInfo->mMesh->mVertexBuffer.mData;

				for (int i = 0; i < pInfo->mMesh->mNumVertices * 3; i += 3)
				{
					lcVector3 Vertex = lcMul31(lcVector3(Verts[i], Verts[i+1], Verts[i+2]), ModelWorld);
					sprintf(Line, "v %.2f %.2f %.2f\n", Vertex[0], Vertex[1], Vertex[2]);
					OBJFile.WriteLine(Line);
				}

				OBJFile.WriteLine("#\n\n");
			}

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				PieceInfo* Info = pPiece->mPieceInfo;

				strcpy(buf, pPiece->GetName());
				for (unsigned int i = 0; i < strlen(buf); i++)
					if ((buf[i] == '#') || (buf[i] == ' '))
						buf[i] = '_';

				sprintf(Line, "g %s\n", buf);
				OBJFile.WriteLine(Line);

				Info->mMesh->ExportWavefrontIndices(OBJFile, pPiece->mColorCode, vert);
				vert += Info->mMesh->mNumVertices;
			}

			setlocale(LC_NUMERIC, OldLocale);
		} break;

		case LC_FILE_PROPERTIES:
		{
			lcPropertiesDialogOptions Options;

			Options.Title = m_strTitle;

			strcpy(Options.Author, m_strAuthor);
			strcpy(Options.Description, m_strDescription);
			strcpy(Options.Comments, m_strComments);

			if (m_nScene & LC_SCENE_BG)
				Options.BackgroundType = 2;
			else if (m_nScene & LC_SCENE_GRADIENT)
				Options.BackgroundType = 1;
			else
				Options.BackgroundType = 0;

			Options.SolidColor = lcVector3(m_fBackground[0], m_fBackground[1], m_fBackground[2]);
			Options.GradientColor1 = lcVector3(m_fGradient1[0], m_fGradient1[1], m_fGradient1[2]);
			Options.GradientColor2 = lcVector3(m_fGradient2[0], m_fGradient2[1], m_fGradient2[2]);
			strcpy(Options.BackgroundFileName, m_strBackground);
			Options.BackgroundTile = (m_nScene & LC_SCENE_BG_TILE) != 0;
			Options.FogEnabled = (m_nScene & LC_SCENE_FOG) != 0;
			Options.FogDensity = m_fFogDensity * 100.0f;
			Options.FogColor = lcVector3(m_fFogColor[0], m_fFogColor[1], m_fFogColor[2]);
			Options.AmbientColor = lcVector3(m_fAmbient[0], m_fAmbient[1], m_fAmbient[2]);
			Options.DrawFloor = (m_nScene & LC_SCENE_FLOOR) != 0;
			Options.SetDefault = false;

			GetPiecesUsed(Options.PartsUsed);

			if (!gMainWindow->DoDialog(LC_DIALOG_PROPERTIES, &Options))
				break;

			bool Modified = false;
			
			if (strcmp(m_strAuthor, Options.Author))
			{
				strcpy(m_strAuthor, Options.Author);
				Modified = true;
			}

			if (strcmp(m_strDescription, Options.Description))
			{
				strcpy(m_strDescription, Options.Description);
				Modified = true;
			}

			if (strcmp(m_strComments, Options.Comments))
			{
				strcpy(m_strComments, Options.Comments);
				Modified = true;
			}

			lcuint32 Scene = 0;

			if (Options.BackgroundType == 2)
				Scene |= LC_SCENE_BG;
			else if (Options.BackgroundType == 1)
				Scene |= LC_SCENE_GRADIENT;

			if (Options.BackgroundTile)
				Scene |= LC_SCENE_BG_TILE;

			if (Options.FogEnabled)
				Scene |= LC_SCENE_FOG;

			if (Options.DrawFloor)
				Scene |= LC_SCENE_FLOOR;

			if (m_nScene != Scene)
			{
				m_nScene = Scene;
				Modified = true;
			}

			if (strcmp(m_strBackground, Options.BackgroundFileName))
			{
				strcpy(m_strBackground, Options.BackgroundFileName);
				Modified = true;
			}

			if (m_fFogDensity * 100.0f != Options.FogDensity)
			{
				m_fFogDensity = Options.FogDensity / 100.0f;
				Modified = true;
			}

			if (memcmp(m_fBackground, Options.SolidColor, sizeof(Options.SolidColor)))
			{
				memcpy(m_fBackground, Options.SolidColor, sizeof(Options.SolidColor));
				Modified = true;
			}

			if (memcmp(m_fGradient1, Options.GradientColor1, sizeof(Options.GradientColor1)))
			{
				memcpy(m_fGradient1, Options.GradientColor1, sizeof(Options.GradientColor1));
				Modified = true;
			}

			if (memcmp(m_fGradient2, Options.GradientColor2, sizeof(Options.GradientColor2)))
				{
				memcpy(m_fGradient2, Options.GradientColor2, sizeof(Options.GradientColor2));
				Modified = true;
				}

			if (memcmp(m_fFogColor, Options.FogColor, sizeof(Options.FogColor)))
			{
				memcpy(m_fFogColor, Options.FogColor, sizeof(Options.FogColor));
				Modified = true;
			}

			if (memcmp(m_fAmbient, Options.AmbientColor, sizeof(Options.AmbientColor)))
			{
				memcpy(m_fAmbient, Options.AmbientColor, sizeof(Options.AmbientColor));
				Modified = true;
			}

			if (Options.SetDefault)
		{
				lcSetProfileInt(LC_PROFILE_DEFAULT_SCENE, Scene);
				lcSetProfileFloat(LC_PROFILE_DEFAULT_FOG_DENSITY, Options.FogDensity);
				lcSetProfileString(LC_PROFILE_DEFAULT_BACKGROUND_TEXTURE, Options.BackgroundFileName);
				lcSetProfileInt(LC_PROFILE_DEFAULT_BACKGROUND_COLOR, LC_RGB(Options.SolidColor[0] * 255, Options.SolidColor[1] * 255, Options.SolidColor[2] * 255));
				lcSetProfileInt(LC_PROFILE_DEFAULT_FOG_COLOR, LC_RGB(Options.FogColor[0] * 255, Options.FogColor[1] * 255, Options.FogColor[2] * 255));
				lcSetProfileInt(LC_PROFILE_DEFAULT_AMBIENT_COLOR, LC_RGB(Options.AmbientColor[0] * 255, Options.AmbientColor[1] * 255, Options.AmbientColor[2] * 255));
				lcSetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR1, LC_RGB(Options.GradientColor1[0] * 255, Options.GradientColor1[1] * 255, Options.GradientColor1[2] * 255));
				lcSetProfileInt(LC_PROFILE_DEFAULT_GRADIENT_COLOR2, LC_RGB(Options.GradientColor2[0] * 255, Options.GradientColor1[1] * 255, Options.GradientColor1[2] * 255));
			}

			if (Modified)
			{
				for (int i = 0; i < m_ViewList.GetSize (); i++)
			{
					m_ViewList[i]->MakeCurrent();
					RenderInitialize();
				}

				SetModifiedFlag(true);
				CheckPoint("Properties");
			}
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
		{
			if (!OpenProject(gMainWindow->mRecentFiles[id - LC_FILE_RECENT1]))
				gMainWindow->RemoveRecentFile(id - LC_FILE_RECENT1);
		} break;

		case LC_FILE_EXIT:
		{
			gMainWindow->Close();
		} break;

		case LC_EDIT_UNDO:
		case LC_EDIT_REDO:
		{
			LC_UNDOINFO *pUndo, *pTmp;
			int i;

			if (id == LC_EDIT_UNDO)
			{
				if ((m_pUndoList != NULL) && (m_pUndoList->pNext != NULL))
				{
					// Remove the first item from the undo list.
					pUndo = m_pUndoList;
					m_pUndoList = pUndo->pNext;

					// Check if we need to delete the last redo info.
					for (pTmp = m_pRedoList, i = 0; pTmp; pTmp = pTmp->pNext, i++)
						if ((i == 29) && (pTmp->pNext != NULL))
						{
							delete pTmp->pNext;
							pTmp->pNext = NULL;
						}

					pUndo->pNext = m_pRedoList;
					m_pRedoList = pUndo;

					pUndo = m_pUndoList;
					DeleteContents(true);
					FileLoad(&pUndo->file, true, false);
				}

				if (m_bUndoOriginal && (m_pUndoList != NULL) && (m_pUndoList->pNext == NULL))
					SetModifiedFlag(false);
			}
			else
			{
				if (m_pRedoList != NULL)
				{
					// Remove the first element from the redo list.
					pUndo = m_pRedoList;
					m_pRedoList = pUndo->pNext;

					// Check if we can delete the last undo info.
					for (pTmp = m_pUndoList, i = 0; pTmp; pTmp = pTmp->pNext, i++)
						if ((i == 30) && (pTmp->pNext != NULL))
						{
							delete pTmp->pNext;
							pTmp->pNext = NULL;
						}

					// Add info to the start of the undo list.
					pUndo->pNext = m_pUndoList;
					m_pUndoList = pUndo;

					// Load state.
					DeleteContents(true);
					FileLoad(&pUndo->file, true, false);
				}
			}

			gMainWindow->UpdateUndoRedo(m_pUndoList->pNext ? m_pUndoList->strText : NULL, m_pRedoList ? m_pRedoList->strText : NULL);
		} break;

		case LC_EDIT_CUT:
		case LC_EDIT_COPY:
		{
			lcMemFile* Clipboard = new lcMemFile();

			int i = 0;
			Piece* pPiece;
			Group* pGroup;
//			Light* pLight;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
					i++;
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
					pPiece->FileSave(*Clipboard);

			for (i = 0, pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				i++;
			Clipboard->WriteBuffer(&i, sizeof(i));

			for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				pGroup->FileSave(Clipboard, m_pGroups);

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
				UpdateAllViews ();
				SetModifiedFlag(true);
				CheckPoint("Cutting");
			}

			g_App->ExportClipboard(Clipboard);
		} break;

		case LC_EDIT_PASTE:
		{
			int i, j;
			Piece* pPasted = NULL;
			lcFile* file = g_App->mClipboard;
			if (file == NULL)
				break;
			file->Seek(0, SEEK_SET);
			SelectAndFocusNone(false);

			file->ReadBuffer(&i, sizeof(i));
			while (i--)
			{
				Piece* pPiece = new Piece(NULL);
				pPiece->FileLoad(*file);
					pPiece->m_pNext = pPasted;
					pPasted = pPiece;
				}

			file->ReadBuffer(&i, sizeof(i));
			Piece* pPiece;
			Group** groups = (Group**)malloc(i*sizeof(Group**));
			for (j = 0; j < i; j++)
			{
				groups[j] = new Group();
				groups[j]->FileLoad(file);
			}

			while (pPasted)
			{
				pPiece = pPasted;
				pPasted = pPasted->m_pNext;
				pPiece->CreateName(m_pPieces);
				pPiece->SetFrameShow(m_nCurFrame);
				pPiece->SetStepShow(m_nCurStep);
				AddPiece(pPiece);
				pPiece->Select(true, false, false);

				j = LC_POINTER_TO_INT(pPiece->GetGroup());
				if (j != -1)
					pPiece->SetGroup(groups[j]);
				else
					pPiece->UnGroup(NULL);
			}

			for (j = 0; j < i; j++)
			{
				int g = LC_POINTER_TO_INT(groups[j]->m_pGroup);
				groups[j]->m_pGroup = (g != -1) ? groups[g] : NULL;
			}

			for (j = 0; j < i; j++)
			{
				Group* pGroup;
				bool add = false;
				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				{
					for (pGroup = pPiece->GetGroup(); pGroup; pGroup = pGroup->m_pGroup)
						if (pGroup == groups[j])
						{
							add = true;
							break;
						}

					if (add)
						break;
				}

				if (add)
				{
					int a, max = 0;

					for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
						if (strncmp("Pasted Group #", pGroup->m_strName, 14) == 0)
							if (sscanf(pGroup->m_strName + 14, "%d", &a) == 1)
								if (a > max)
									max = a;

					sprintf(groups[j]->m_strName, "Pasted Group #%.2d", max+1);
					groups[j]->m_pNext = m_pGroups;
					m_pGroups = groups[j];
				}
				else
					delete groups[j];
			}

			free(groups);

			file->ReadBuffer(&i, sizeof(i));

			while (i--)
			{
				Camera* pCamera = new Camera(false);
				pCamera->FileLoad(*file);
				pCamera->CreateName(mCameras);
				pCamera->Select(true, false, false);
				pCamera->GetTarget()->Select(true, false, false);
				mCameras.Add(pCamera);
			}

			// TODO: lights
			CalculateStep();
			SetModifiedFlag(true);
			CheckPoint("Pasting");
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			UpdateAllViews ();
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
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
					pPiece->Select(true, false, false);

//	pFrame->UpdateInfo();
			UpdateSelection();
			UpdateAllViews ();
		} break;

		case LC_EDIT_SELECT_NONE:
		{
			SelectAndFocusNone(false);
			gMainWindow->UpdateFocusObject(NULL);
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_EDIT_SELECT_INVERT:
		{
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
				{
                                  if (pPiece->IsSelected())
                                    pPiece->Select(false, false, false);
                                  else
                                    pPiece->Select(true, false, false);
				}

			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_EDIT_SELECT_BY_NAME:
		{
			lcSelectDialogOptions Options;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				Options.Selection.Add(pPiece->IsSelected());

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				if (mCameras[CameraIdx]->IsVisible())
					Options.Selection.Add(mCameras[CameraIdx]->IsSelected());

			for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsVisible())
					Options.Selection.Add(pLight->IsSelected());

			if (Options.Selection.GetSize() == 0)
			{
				gMainWindow->DoMessageBox("Nothing to select.", LC_MB_OK | LC_MB_ICONINFORMATION);
				break;
			}

			if (!gMainWindow->DoDialog(LC_DIALOG_SELECT_BY_NAME, &Options))
				break;

			SelectAndFocusNone(false);

			int ObjectIndex = 0;
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					pPiece->Select(true, false, false);

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					mCameras[CameraIdx]->Select(true, false, false);

			for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext, ObjectIndex++)
				if (Options.Selection[ObjectIndex])
					pLight->Select(true, false, false);

			UpdateSelection();
			UpdateAllViews();
//	pFrame->UpdateInfo();
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

		case LC_PIECE_INSERT:
		{
			if (m_pCurPiece == NULL)
				break;
			Piece* pLast = NULL;
			Piece* pPiece = new Piece(m_pCurPiece);

			for (pLast = m_pPieces; pLast; pLast = pLast->m_pNext)
				if ((pLast->IsFocused()) || (pLast->m_pNext == NULL))
					break;

			if (pLast != NULL)
			{
				lcVector3 Pos;
				lcVector4 Rot;

				GetPieceInsertPosition(pLast, Pos, Rot);

				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_nCurStep, m_nCurFrame);

				pPiece->ChangeKey(m_nCurStep, false, false, Rot, LC_PK_ROTATION);
				pPiece->ChangeKey(m_nCurFrame, true, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
			}
			else
				pPiece->Initialize(0, 0, 0, m_nCurStep, m_nCurFrame);

			SelectAndFocusNone(false);
			pPiece->SetColorIndex(gMainWindow->mColorIndex);
			pPiece->CreateName(m_pPieces);
			AddPiece(pPiece);
			pPiece->Select (true, true, false);
			gMainWindow->UpdateFocusObject(pPiece);
			UpdateSelection();
			SystemPieceComboAdd(m_pCurPiece->m_strDescription);

//			AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)pNew, OT_PIECE);
			UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Inserting");
		} break;

		case LC_PIECE_DELETE:
		{
			if (RemoveSelectedObjects())
			{
				gMainWindow->UpdateFocusObject(NULL);
				UpdateSelection();
				UpdateAllViews();
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

				int Viewport[4] = { 0, 0, m_ActiveView->mWidth, m_ActiveView->mHeight };
				float Aspect = (float)Viewport[2]/(float)Viewport[3];
				Camera* Cam = m_ActiveView->mCamera;

				const lcMatrix44& ModelView = Cam->mWorldView;
				lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

				lcVector3 Pts[3] = { lcVector3(5.0f, 5.0f, 0.1f), lcVector3(10.0f, 5.0f, 0.1f), lcVector3(5.0f, 10.0f, 0.1f) };
				lcUnprojectPoints(Pts, 3, ModelView, Projection, Viewport);

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

			UpdateOverlayScale();
			UpdateAllViews();
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
					pPiece->Initialize(Position[0], Position[1], Position[2], m_nCurStep, m_nCurFrame);
					pPiece->SetColorIndex(Minifig.Colors[i]);
					pPiece->CreateName(m_pPieces);
					AddPiece(pPiece);
					pPiece->Select(true, false, false);

					pPiece->ChangeKey(1, false, false, Rotation, LC_PK_ROTATION);
					pPiece->ChangeKey(1, true, false, Rotation, LC_PK_ROTATION);
					pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

					SystemPieceComboAdd(Minifig.Parts[i]->m_strDescription);
				}

				float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
				int max = 0;

				Group* pGroup;
				for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
					if (strncmp (pGroup->m_strName, "Minifig #", 9) == 0)
						if (sscanf(pGroup->m_strName, "Minifig #%d", &i) == 1)
							if (i > max)
								max = i;
				pGroup = new Group;
				sprintf(pGroup->m_strName, "Minifig #%.2d", max+1);

				pGroup->m_pNext = m_pGroups;
				m_pGroups = pGroup;

				for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->IsSelected())
					{
						pPiece->SetGroup(pGroup);
						pPiece->CompareBoundingBox(bs);
					}

				pGroup->m_fCenter[0] = (bs[0]+bs[3])/2;
				pGroup->m_fCenter[1] = (bs[1]+bs[4])/2;
				pGroup->m_fCenter[2] = (bs[2]+bs[5])/2;

			gMainWindow->UpdateFocusObject(GetFocusObject());
				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Minifig");
		} break;

		case LC_PIECE_ARRAY:
		{
			Piece *pPiece, *pFirst = NULL, *pLast = NULL;
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			int sel = 0;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
			{
					pPiece->CompareBoundingBox(bs);
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

				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				{
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

								if (pLast)
								{
									pLast->m_pNext = new Piece(pPiece->mPieceInfo);
									pLast = pLast->m_pNext;
								}
								else
									pLast = pFirst = new Piece(pPiece->mPieceInfo);

							pLast->Initialize(Position[0] + Offset[0], Position[1] + Offset[1], Position[2] + Offset[2], m_nCurStep, m_nCurFrame);
								pLast->SetColorIndex(pPiece->mColorIndex);
								pLast->ChangeKey(1, false, false, AxisAngle, LC_PK_ROTATION);
								pLast->ChangeKey(1, true, false, AxisAngle, LC_PK_ROTATION);
							}
						}
					}
				}

				while (pFirst)
				{
					pPiece = pFirst->m_pNext;
					pFirst->CreateName(m_pPieces);
					pFirst->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
					AddPiece(pFirst);
					pFirst = pPiece;
				}

				SelectAndFocusNone(true);
//			gMainWindow->UpdateFocusObject(GetFocusObject());
				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Array");
		} break;

		case LC_PIECE_GROUP:
		{
			Group* pGroup;
			int i, max = 0;
			char name[65];
			int Selected = 0;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
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

			for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				if (strncmp (pGroup->m_strName, "Group #", 7) == 0)
					if (sscanf(pGroup->m_strName, "Group #%d", &i) == 1)
						if (i > max)
							max = i;
			sprintf(name, "Group #%.2d", max+1);

			if (!gMainWindow->DoDialog(LC_DIALOG_PIECE_GROUP, name))
				break;

				pGroup = new Group();
				strcpy(pGroup->m_strName, name);
				pGroup->m_pNext = m_pGroups;
				m_pGroups = pGroup;
				float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

				for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->IsSelected())
					{
						pPiece->DoGroup(pGroup);
						pPiece->CompareBoundingBox(bs);
					}

				pGroup->m_fCenter[0] = (bs[0]+bs[3])/2;
				pGroup->m_fCenter[1] = (bs[1]+bs[4])/2;
				pGroup->m_fCenter[2] = (bs[2]+bs[5])/2;

				RemoveEmptyGroups();
				SetModifiedFlag(true);
				CheckPoint("Grouping");
		} break;

		case LC_PIECE_UNGROUP:
		{
			Group* pList = NULL;
			Group* pGroup;
			Group* tmp;
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					pGroup = pPiece->GetTopGroup();

					// Check if we already removed the group
					for (tmp = pList; tmp; tmp = tmp->m_pNext)
						if (pGroup == tmp)
							pGroup = NULL;

					if (pGroup != NULL)
					{
						// First remove the group from the array
						for (tmp = m_pGroups; tmp->m_pNext; tmp = tmp->m_pNext)
							if (tmp->m_pNext == pGroup)
							{
								tmp->m_pNext = pGroup->m_pNext;
								break;
							}

						if (pGroup == m_pGroups)
							m_pGroups = pGroup->m_pNext;

						// Now add it to the list of top groups
						pGroup->m_pNext = pList;
						pList = pGroup;
					}
				}

			while (pList)
			{
				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->IsSelected())
						pPiece->UnGroup(pList);

				pGroup = pList;
				pList = pList->m_pNext;
				delete pGroup;
			}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Ungrouping");
		} break;

		case LC_PIECE_GROUP_ADD:
		{
			Group* pGroup = NULL;
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					pGroup = pPiece->GetTopGroup();
					if (pGroup != NULL)
						break;
				}

			if (pGroup != NULL)
			{
				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->IsFocused())
					{
						pPiece->SetGroup(pGroup);
						break;
					}
			}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Grouping");
		} break;

		case LC_PIECE_GROUP_REMOVE:
		{
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsFocused())
				{
					pPiece->UnGroup(NULL);
					break;
				}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Ungrouping");
		} break;

		case LC_PIECE_GROUP_EDIT:
		{
			lcEditGroupsDialogOptions Options;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				Options.PieceParents.Add(pPiece->GetGroup());

			for (Group* pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				Options.GroupParents.Add(pGroup->m_pGroup);

			if (!gMainWindow->DoDialog(LC_DIALOG_EDIT_GROUPS, &Options))
				break;

			int PieceIdx = 0;
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				pPiece->SetGroup(Options.PieceParents[PieceIdx++]);

			int GroupIdx = 0;
			for (Group* pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				pGroup->m_pGroup = Options.GroupParents[GroupIdx++];

				RemoveEmptyGroups();
				SelectAndFocusNone(false);
			gMainWindow->UpdateFocusObject(GetFocusObject());
				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Editing");
		} break;

		case LC_PIECE_HIDE_SELECTED:
		{
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
					pPiece->Hide();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(NULL);
			UpdateAllViews();
		} break;

		case LC_PIECE_HIDE_UNSELECTED:
		{
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (!pPiece->IsSelected())
					pPiece->Hide();
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_PIECE_UNHIDE_ALL:
		{
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				pPiece->UnHide();
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_PIECE_SHOW_EARLIER:
		{
			bool redraw = false;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					if (m_bAnimation)
					{
						unsigned short t = pPiece->GetFrameShow();
						if (t > 1)
						{
							redraw = true;
							pPiece->SetFrameShow(t-1);
						}
					}
					else
					{
						unsigned char t = pPiece->GetStepShow();
						if (t > 1)
						{
							redraw = true;
							pPiece->SetStepShow(t-1);
						}
					}
				}

			if (redraw)
			{
				SetModifiedFlag(true);
				CheckPoint("Modifying");
				UpdateAllViews();
			}
		} break;

		case LC_PIECE_SHOW_LATER:
		{
			bool redraw = false;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					if (m_bAnimation)
					{
						unsigned short t = pPiece->GetFrameShow();
						if (t < m_nTotalFrames)
						{
							redraw = true;
							pPiece->SetFrameShow(t+1);

							if (pPiece->IsSelected () && t == m_nCurFrame)
								pPiece->Select (false, false, false);
						}
					}
					else
					{
						unsigned char t = pPiece->GetStepShow();
						if (t < 255)
						{
							redraw = true;
							pPiece->SetStepShow(t+1);

							if (pPiece->IsSelected () && t == m_nCurStep)
								pPiece->Select (false, false, false);
						}
					}
				}

			if (redraw)
			{
				SetModifiedFlag(true);
				CheckPoint("Modifying");
				UpdateAllViews();
				UpdateSelection ();
			}
		} break;

		case LC_VIEW_PREFERENCES:
		{
			lcPreferencesDialogOptions Options;
			int CurrentAASamples = lcGetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES);

			strcpy(Options.DefaultAuthor, lcGetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME));
			strcpy(Options.ProjectsPath, lcGetProfileString(LC_PROFILE_PROJECTS_PATH));
			strcpy(Options.LibraryPath, lcGetProfileString(LC_PROFILE_PARTS_LIBRARY));
			strcpy(Options.POVRayPath, lcGetProfileString(LC_PROFILE_POVRAY_PATH));
			strcpy(Options.LGEOPath, lcGetProfileString(LC_PROFILE_POVRAY_LGEO_PATH));
			Options.MouseSensitivity = m_nMouse;
			Options.CheckForUpdates = lcGetProfileInt(LC_PROFILE_CHECK_UPDATES);

			Options.Detail = m_nDetail;
			Options.Snap = m_nSnap;
			Options.LineWidth = m_fLineWidth;
			Options.AASamples = CurrentAASamples;
			Options.GridStuds = mGridStuds;
			Options.GridStudColor = mGridStudColor;
			Options.GridLines = mGridLines;
			Options.GridLineSpacing = mGridLineSpacing;
			Options.GridLineColor = mGridLineColor;

			Options.Categories = gCategories;
			Options.CategoriesModified = false;
			Options.CategoriesDefault = false;

			Options.KeyboardShortcuts = gKeyboardShortcuts;
			Options.ShortcutsModified = false;
			Options.ShortcutsDefault = false;

			if (!gMainWindow->DoDialog(LC_DIALOG_PREFERENCES, &Options))
				break;

			bool LibraryChanged = strcmp(Options.LibraryPath, lcGetProfileString(LC_PROFILE_PARTS_LIBRARY));
			bool AAChanged = CurrentAASamples != Options.AASamples;

			m_nMouse = Options.MouseSensitivity;
			m_nSnap = Options.Snap;
			m_nDetail = Options.Detail;
			m_fLineWidth = Options.LineWidth;
			mGridStuds = Options.GridStuds;
			mGridStudColor = Options.GridStudColor;
			mGridLines = Options.GridLines;
			mGridLineSpacing = Options.GridLineSpacing;
			mGridLineColor = Options.GridLineColor;

			lcSetProfileString(LC_PROFILE_DEFAULT_AUTHOR_NAME, Options.DefaultAuthor);
			lcSetProfileString(LC_PROFILE_PROJECTS_PATH, Options.ProjectsPath);
			lcSetProfileString(LC_PROFILE_PARTS_LIBRARY, Options.LibraryPath);
			lcSetProfileString(LC_PROFILE_POVRAY_PATH, Options.POVRayPath);
			lcSetProfileString(LC_PROFILE_POVRAY_LGEO_PATH, Options.LGEOPath);
			lcSetProfileInt(LC_PROFILE_MOUSE_SENSITIVITY, m_nMouse);
			lcSetProfileInt(LC_PROFILE_CHECK_UPDATES, Options.CheckForUpdates);
			lcSetProfileInt(LC_PROFILE_SNAP, Options.Snap);
			lcSetProfileInt(LC_PROFILE_DETAIL, Options.Detail);
			lcSetProfileInt(LC_PROFILE_GRID_STUDS, Options.GridStuds);
			lcSetProfileInt(LC_PROFILE_GRID_STUD_COLOR, Options.GridStudColor);
			lcSetProfileInt(LC_PROFILE_GRID_LINES, Options.GridLines);
			lcSetProfileInt(LC_PROFILE_GRID_LINE_SPACING, Options.GridLineSpacing);
			lcSetProfileInt(LC_PROFILE_GRID_LINE_COLOR, Options.GridLineColor);
			lcSetProfileFloat(LC_PROFILE_LINE_WIDTH, Options.LineWidth);
			lcSetProfileInt(LC_PROFILE_ANTIALIASING_SAMPLES, Options.AASamples);

			if (LibraryChanged && AAChanged)
				gMainWindow->DoMessageBox("Parts library and Anti-aliasing changes will only take effect the next time you start LeoCAD.", LC_MB_OK);
			else if (LibraryChanged)
				gMainWindow->DoMessageBox("Parts library changes will only take effect the next time you start LeoCAD.", LC_MB_OK);
			else if (AAChanged)
				gMainWindow->DoMessageBox("Anti-aliasing changes will only take effect the next time you start LeoCAD.", LC_MB_OK);

			if (Options.CategoriesModified)
			{
				if (Options.CategoriesDefault)
					lcResetDefaultCategories();
				else
				{
					gCategories = Options.Categories;
					lcSaveDefaultCategories();
				}

				gMainWindow->UpdateCategories();
			}

			if (Options.ShortcutsModified)
			{
				if (Options.ShortcutsDefault)
					lcResetDefaultKeyboardShortcuts();
				else
				{
					gKeyboardShortcuts = Options.KeyboardShortcuts;
					lcSaveDefaultKeyboardShortcuts();
				}

				gMainWindow->UpdateShortcuts();
			}

			// TODO: printing preferences
			/*
			strcpy(opts.strFooter, m_strFooter);
			strcpy(opts.strHeader, m_strHeader);
			*/

			for (int i = 0; i < m_ViewList.GetSize (); i++)
			{
				m_ViewList[i]->MakeCurrent();
				RenderInitialize(); // TODO: get rid of RenderInitialize(), most of it can be done once per frame
			}

			UpdateAllViews();
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
				FirstView = m_ViewList.FindIndex(m_ActiveView);
				LastView = FirstView + 1;
			}

			ZoomExtents(FirstView, LastView);
		} break;

		case LC_VIEW_TIME_NEXT:
		{
			if (m_bAnimation)
				m_nCurFrame++;
			else
				m_nCurStep++;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();

			if (m_bAnimation)
				gMainWindow->UpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				gMainWindow->UpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;

		case LC_VIEW_TIME_PREVIOUS:
		{
			if (m_bAnimation)
				m_nCurFrame--;
			else
				m_nCurStep--;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();

			if (m_bAnimation)
				gMainWindow->UpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				gMainWindow->UpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;

		case LC_VIEW_TIME_FIRST:
		{
			if (m_bAnimation)
				m_nCurFrame = 1;
			else
				m_nCurStep = 1;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();

			if (m_bAnimation)
				gMainWindow->UpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				gMainWindow->UpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;

		case LC_VIEW_TIME_LAST:
		{
			if (m_bAnimation)
				m_nCurFrame = m_nTotalFrames;
			else
				m_nCurStep = GetLastStep ();

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();

			if (m_bAnimation)
				gMainWindow->UpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				gMainWindow->UpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;
/*
		case LC_VIEW_STEP_SET:
		{
			if (m_bAnimation)
				m_nCurFrame = (nParam < m_nTotalFrames) ? (unsigned short)nParam : m_nTotalFrames;
			else
				m_nCurStep = (nParam < 255) ? (unsigned char)nParam : 255;

			CalculateStep();
			UpdateSelection();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();

			if (m_bAnimation)
				gMainWindow->UpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				gMainWindow->UpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;
*/
		case LC_VIEW_TIME_INSERT:
		{
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				pPiece->InsertTime(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				mCameras[CameraIdx]->InsertTime(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

			for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				pLight->InsertTime(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

			SetModifiedFlag(true);
			if (m_bAnimation)
				CheckPoint("Adding Frame");
			else
				CheckPoint("Adding Step");
			CalculateStep();
			UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_TIME_DELETE:
		{
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				pPiece->RemoveTime(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				mCameras[CameraIdx]->RemoveTime(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

			for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				pLight->RemoveTime(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

			SetModifiedFlag (true);
			if (m_bAnimation)
				CheckPoint("Removing Frame");
			else
				CheckPoint("Removing Step");
			CalculateStep();
			UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_TIME_STOP:
		{
			m_bStopRender = true;
		} break;

		case LC_VIEW_TIME_PLAY:
		{
			// TODO: Rewrite animation playback by posting events, not looping here
		/*
			SelectAndFocusNone(false);
			UpdateSelection();
			m_bStopRender = false;
			SystemUpdatePlay(false, true);
			long time = SystemGetTicks();
			unsigned short tics;
			float rate = 1000.0f/m_nFPS;

			while (!m_bStopRender)
			{
				tics = (unsigned short)(SystemGetTicks() - time);
				if (tics < rate)
					continue; // nothing to do

				time = SystemGetTicks();
				m_nCurFrame += (unsigned short)((float)tics/rate);
				while (m_nCurFrame > m_nTotalFrames)
					m_nCurFrame -= m_nTotalFrames;
				CalculateStep();
				gMainWindow->UpdateTime(true, m_nCurFrame, m_nTotalFrames);
                UpdateAllViews();
			}
			SystemUpdatePlay(true, false);
			gMainWindow->UpdateFocusObject(GetFocusObject());
			*/
		} break;

		case LC_VIEW_VIEWPOINT_FRONT:
		{
			m_ActiveView->mCamera->SetViewpoint(LC_VIEWPOINT_FRONT, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
		} break;

		case LC_VIEW_VIEWPOINT_BACK:
		{
			m_ActiveView->mCamera->SetViewpoint(LC_VIEWPOINT_BACK, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
		} break;

		case LC_VIEW_VIEWPOINT_TOP:
		{
			m_ActiveView->mCamera->SetViewpoint(LC_VIEWPOINT_TOP, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
		} break;

		case LC_VIEW_VIEWPOINT_BOTTOM:
		{
			m_ActiveView->mCamera->SetViewpoint(LC_VIEWPOINT_BOTTOM, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
		} break;

		case LC_VIEW_VIEWPOINT_LEFT:
		{
			m_ActiveView->mCamera->SetViewpoint(LC_VIEWPOINT_LEFT, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
		} break;

		case LC_VIEW_VIEWPOINT_RIGHT:
		{
			m_ActiveView->mCamera->SetViewpoint(LC_VIEWPOINT_RIGHT, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
		} break;

		case LC_VIEW_VIEWPOINT_HOME:
		{
			m_ActiveView->mCamera->SetViewpoint(LC_VIEWPOINT_HOME, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			HandleCommand(LC_VIEW_ZOOM_EXTENTS);
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
			Camera* pCamera = NULL;

			if (id == LC_VIEW_CAMERA_NONE)
			{
				pCamera = m_ActiveView->mCamera;

				if (!pCamera->IsSimple())
				{
					m_ActiveView->SetCamera(pCamera, true);
					pCamera = m_ActiveView->mCamera;
				}
			}
			else
			{
				if (id - LC_VIEW_CAMERA1 < mCameras.GetSize())
				{
					pCamera = mCameras[id - LC_VIEW_CAMERA1];
					m_ActiveView->SetCamera(pCamera, false);
				}
				else
					break;
			}

			gMainWindow->UpdateCurrentCamera(mCameras.FindIndex(m_ActiveView->mCamera));
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_CAMERA_RESET:
		{
			for (int ViewIdx = 0; ViewIdx < m_ViewList.GetSize(); ViewIdx++)
				m_ViewList[ViewIdx]->SetDefaultCamera();

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
				delete mCameras[CameraIdx];
			mCameras.RemoveAll();

			gMainWindow->UpdateCameraMenu(mCameras, m_ActiveView ? m_ActiveView->mCamera : NULL);
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateOverlayScale();
			UpdateAllViews();
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

			m_ActiveView->MakeCurrent();

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
			Info += GL_HasFramebufferObject() ? "supported" : "not supported";
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

		case LC_VIEW_TIME_ANIMATION:
		{
			m_bAnimation = !m_bAnimation;

			CalculateStep();
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();

			gMainWindow->UpdateAnimation(m_bAnimation, m_bAddKeys);
			if (m_bAnimation)
				gMainWindow->UpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				gMainWindow->UpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;

		case LC_VIEW_TIME_ADD_KEYS:
		{
			m_bAddKeys = !m_bAddKeys;
			gMainWindow->UpdateAnimation(m_bAnimation, m_bAddKeys);
		} break;

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
		{
			SetAction(LC_ACTION_SELECT);
		} break;

		case LC_EDIT_ACTION_INSERT:
		{
			SetAction(LC_ACTION_INSERT);
		} break;

		case LC_EDIT_ACTION_LIGHT:
		{
			SetAction(LC_ACTION_LIGHT);
		} break;

		case LC_EDIT_ACTION_SPOTLIGHT:
		{
			SetAction(LC_ACTION_SPOTLIGHT);
		} break;

		case LC_EDIT_ACTION_CAMERA:
		{
			SetAction(LC_ACTION_CAMERA);
		} break;

		case LC_EDIT_ACTION_MOVE:
		{
			SetAction(LC_ACTION_MOVE);
		} break;

		case LC_EDIT_ACTION_ROTATE:
		{
			SetAction(LC_ACTION_ROTATE);
		} break;

		case LC_EDIT_ACTION_DELETE:
		{
			SetAction(LC_ACTION_ERASER);
		} break;

		case LC_EDIT_ACTION_PAINT:
		{
			SetAction(LC_ACTION_PAINT);
		} break;

		case LC_EDIT_ACTION_ZOOM:
		{
			SetAction(LC_ACTION_ZOOM);
		} break;

		case LC_EDIT_ACTION_ZOOM_REGION:
		{
			SetAction(LC_ACTION_ZOOM_REGION);
		} break;

		case LC_EDIT_ACTION_PAN:
		{
			SetAction(LC_ACTION_PAN);
		} break;

		case LC_EDIT_ACTION_ROTATE_VIEW:
		{
			SetAction(LC_ACTION_ROTATE_VIEW);
		} break;

		case LC_EDIT_ACTION_ROLL:
		{
			SetAction(LC_ACTION_ROLL);
		} break;

		case LC_EDIT_CANCEL:
		{
			if (m_nTracking != LC_TRACK_NONE)
				StopTracking(false);
			else
			{
				SelectAndFocusNone(false);
				UpdateSelection();
				UpdateAllViews();
				gMainWindow->UpdateFocusObject(NULL);
			}
		} break;

		case LC_NUM_COMMANDS:
			break;
	}
}

void Project::SetAction(int nAction)
{
	m_nCurAction = nAction;

	gMainWindow->UpdateAction(m_nCurAction);

	ActivateOverlay(m_ActiveView, m_nCurAction, LC_OVERLAY_NONE);

	UpdateAllViews();
}

int Project::GetAction() const
{
	int Action = m_nCurAction;

	if (m_OverlayActive)
	{
		switch (m_OverlayMode)
		{
		case LC_OVERLAY_NONE:
			break;

		case LC_OVERLAY_MOVE_X:
		case LC_OVERLAY_MOVE_Y:
		case LC_OVERLAY_MOVE_Z:
		case LC_OVERLAY_MOVE_XY:
		case LC_OVERLAY_MOVE_XZ:
		case LC_OVERLAY_MOVE_YZ:
		case LC_OVERLAY_MOVE_XYZ:
			Action = LC_ACTION_MOVE;
			break;

		case LC_OVERLAY_ROTATE_X:
		case LC_OVERLAY_ROTATE_Y:
		case LC_OVERLAY_ROTATE_Z:
		case LC_OVERLAY_ROTATE_XY:
		case LC_OVERLAY_ROTATE_XZ:
		case LC_OVERLAY_ROTATE_YZ:
		case LC_OVERLAY_ROTATE_XYZ:
			Action = LC_ACTION_ROTATE;
			break;

		case LC_OVERLAY_ZOOM:
			Action = LC_ACTION_ZOOM;
			break;

		case LC_OVERLAY_PAN:
			Action = LC_ACTION_PAN;
			break;

		case LC_OVERLAY_ROTATE_VIEW_X:
		case LC_OVERLAY_ROTATE_VIEW_Y:
		case LC_OVERLAY_ROTATE_VIEW_Z:
		case LC_OVERLAY_ROTATE_VIEW_XYZ:
			Action = LC_ACTION_ROTATE_VIEW;
			break;
		}
	}

	return Action;
}

// Remove unused groups
void Project::RemoveEmptyGroups()
{
	bool recurse = false;
	Group *g1, *g2;
	Piece* pPiece;
	int ref;

	for (g1 = m_pGroups; g1;)
	{
		ref = 0;

		for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			if (pPiece->GetGroup() == g1)
				ref++;

		for (g2 = m_pGroups; g2; g2 = g2->m_pNext)
			if (g2->m_pGroup == g1)
				ref++;

		if (ref < 2)
		{
			if (ref != 0)
			{
				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->GetGroup() == g1)
						pPiece->SetGroup(g1->m_pGroup);

				for (g2 = m_pGroups; g2; g2 = g2->m_pNext)
					if (g2->m_pGroup == g1)
						g2->m_pGroup = g1->m_pGroup;
			}

			if (g1 == m_pGroups)
			{
				m_pGroups = g1->m_pNext;
				delete g1;
				g1 = m_pGroups;
			}
			else
			{
				for (g2 = m_pGroups; g2; g2 = g2->m_pNext)
					if (g2->m_pNext == g1)
					{
						g2->m_pNext = g1->m_pNext;
						break;
					}

				delete g1;
				g1 = g2->m_pNext;
			}

			recurse = true;
		}
		else
			g1 = g1->m_pNext;
	}

	if (recurse)
		RemoveEmptyGroups();
}

Group* Project::AddGroup (const char* name, Group* pParent, float x, float y, float z)
{
  Group *pNewGroup = new Group();

  if (name == NULL)
  {
    int i, max = 0;
    char str[65];

    for (Group *pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
      if (strncmp (pGroup->m_strName, "Group #", 7) == 0)
        if (sscanf(pGroup->m_strName, "Group #%d", &i) == 1)
          if (i > max)
            max = i;
    sprintf (str, "Group #%.2d", max+1);

    strcpy (pNewGroup->m_strName, str);
  }
  else
    strcpy (pNewGroup->m_strName, name);

  pNewGroup->m_pNext = m_pGroups;
  m_pGroups = pNewGroup;

  pNewGroup->m_fCenter[0] = x;
  pNewGroup->m_fCenter[1] = y;
  pNewGroup->m_fCenter[2] = z;
  pNewGroup->m_pGroup = pParent;

  return pNewGroup;
}

void Project::SelectAndFocusNone(bool bFocusOnly)
{
	Piece* pPiece;
	Light* pLight;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->Select(false, bFocusOnly, false);

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		pCamera->Select (false, bFocusOnly, false);
		pCamera->GetTarget()->Select (false, bFocusOnly, false);
	}

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
	{
		pLight->Select(false, bFocusOnly, false);
		if (pLight->GetTarget())
			pLight->GetTarget()->Select (false, bFocusOnly, false);
	}
//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);
}

bool Project::GetSelectionCenter(lcVector3& Center) const
{
	float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
	bool Selected = false;

	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (pPiece->IsSelected())
		{
			pPiece->CompareBoundingBox(bs);
			Selected = true;
		}
	}

	Center = lcVector3((bs[0] + bs[3]) * 0.5f, (bs[1] + bs[4]) * 0.5f, (bs[2] + bs[5]) * 0.5f);

	return Selected;
}

void Project::ConvertToUserUnits(lcVector3& Value) const
{
	if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
		Value /= 0.04f;
}

void Project::ConvertFromUserUnits(lcVector3& Value) const
{
	if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
		Value *= 0.04f;
}

bool Project::GetFocusPosition(lcVector3& Position) const
{
	Piece* pPiece;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsFocused())
		{
			Position = pPiece->mPosition;
			return true;
		}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if (pCamera->IsEyeFocused())
		{
			Position = pCamera->mPosition;
			return true;
		}

		if (pCamera->IsTargetFocused())
		{
			Position = pCamera->mTargetPosition;
			return true;
		}
	}

	// TODO: light

	Position = lcVector3(0.0f, 0.0f, 0.0f);

	return false;
}

// Returns the object that currently has focus.
Object* Project::GetFocusObject() const
{
	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (pPiece->IsFocused())
			return pPiece;
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if (pCamera->IsEyeFocused() || pCamera->IsTargetFocused())
			return pCamera;
	}

	// TODO: light

	return NULL;
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
void Project::GetPieceInsertPosition(View* view, int MouseX, int MouseY, lcVector3& Position, lcVector4& Rotation)
{
	// Check if the mouse is over a piece.
	Piece* HitPiece = (Piece*)FindObjectFromPoint(view, MouseX, MouseY, true);

	if (HitPiece)
	{
		GetPieceInsertPosition(HitPiece, Position, Rotation);
		return;
	}

	// Try to hit the base grid.
	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };

	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->mCamera;

	const lcMatrix44& ModelView = Cam->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	lcVector3 ClickPoints[2] = { lcVector3((float)m_nDownX, (float)m_nDownY, 0.0f), lcVector3((float)m_nDownX, (float)m_nDownY, 1.0f) };
	lcUnprojectPoints(ClickPoints, 2, ModelView, Projection, Viewport);

	lcVector3 Intersection;
	if (lcLinePlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], lcVector4(0, 0, 1, m_pCurPiece->m_fDimensions[5])))
	{
		SnapVector(Intersection);
		Position = Intersection;
		Rotation = lcVector4(0, 0, 1, 0);
		return;
	}

	// Couldn't find a good position, so just place the piece somewhere near the camera.
	Position = lcUnprojectPoint(lcVector3((float)m_nDownX, (float)m_nDownY, 0.9f), ModelView, Projection, Viewport);
	Rotation = lcVector4(0, 0, 1, 0);
}

Object* Project::FindObjectFromPoint(View* view, int x, int y, bool PiecesOnly)
{
	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->mCamera;

	const lcMatrix44& ModelView = Cam->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	lcVector3 Start = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.0f), ModelView, Projection, Viewport);
	lcVector3 End = lcUnprojectPoint(lcVector3((float)x, (float)y, 1.0f), ModelView, Projection, Viewport);

	lcClickLine ClickLine;

	ClickLine.Start = Start;
	ClickLine.End = End;
	ClickLine.MinDist = FLT_MAX;
	ClickLine.Closest = NULL;

	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			pPiece->MinIntersectDist(&ClickLine);

	if (!PiecesOnly)
	{
		for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		{
			Camera* pCamera = mCameras[CameraIdx];

			if (pCamera != view->mCamera && pCamera->IsVisible())
				pCamera->MinIntersectDist(&ClickLine);
		}

		for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
			if (pLight->IsVisible())
				pLight->MinIntersectDist(&ClickLine);
	}

	return ClickLine.Closest;
}

void Project::FindObjectsInBox(float x1, float y1, float x2, float y2, lcArray<Object*>& Objects)
{
	int Viewport[4] = { 0, 0, m_ActiveView->mWidth, m_ActiveView->mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = m_ActiveView->mCamera;

	const lcMatrix44& ModelView = Cam->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	// Find out the top-left and bottom-right corners in screen coordinates.
	float Left, Top, Bottom, Right;

	if (x1 < x2)
	{
		Left = x1;
		Right = x2;
	}
	else
	{
		Left = x2;
		Right = x1;
	}

	if (y1 > y2)
	{
		Top = y1;
		Bottom = y2;
	}
	else
	{
		Top = y2;
		Bottom = y1;
	}

	// Unproject 6 points to world space.
	lcVector3 Corners[6] =
	{
		lcVector3(Left, Top, 0), lcVector3(Left, Bottom, 0), lcVector3(Right, Bottom, 0),
		lcVector3(Right, Top, 0), lcVector3(Left, Top, 1), lcVector3(Right, Bottom, 1)
	};

	lcUnprojectPoints(Corners, 6, ModelView, Projection, Viewport);

	// Build the box planes.
	lcVector3 PlaneNormals[6];

	PlaneNormals[0] = lcNormalize(lcCross(Corners[4] - Corners[0], Corners[1] - Corners[0])); // Left
	PlaneNormals[1] = lcNormalize(lcCross(Corners[5] - Corners[2], Corners[3] - Corners[2])); // Right
	PlaneNormals[2] = lcNormalize(lcCross(Corners[3] - Corners[0], Corners[4] - Corners[0])); // Top
	PlaneNormals[3] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[5] - Corners[2])); // Bottom
	PlaneNormals[4] = lcNormalize(lcCross(Corners[1] - Corners[0], Corners[3] - Corners[0])); // Front
	PlaneNormals[5] = lcNormalize(lcCross(Corners[1] - Corners[2], Corners[3] - Corners[2])); // Back

	lcVector4 Planes[6];
	Planes[0] = lcVector4(PlaneNormals[0], -lcDot(PlaneNormals[0], Corners[0]));
	Planes[1] = lcVector4(PlaneNormals[1], -lcDot(PlaneNormals[1], Corners[5]));
	Planes[2] = lcVector4(PlaneNormals[2], -lcDot(PlaneNormals[2], Corners[0]));
	Planes[3] = lcVector4(PlaneNormals[3], -lcDot(PlaneNormals[3], Corners[5]));
	Planes[4] = lcVector4(PlaneNormals[4], -lcDot(PlaneNormals[4], Corners[0]));
	Planes[5] = lcVector4(PlaneNormals[5], -lcDot(PlaneNormals[5], Corners[5]));

	// Check if any objects are inside the volume.
	for (Piece* piece = m_pPieces; piece != NULL; piece = piece->m_pNext)
	{
		if (piece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
		{
			if (piece->IntersectsVolume(Planes))
				Objects.Add(piece);
		}
	}

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if (!pCamera->IsVisible() || pCamera == Cam)
			continue;

		if (pCamera->IntersectsVolume(Planes))
			Objects.Add(pCamera);

		if (pCamera->GetTarget()->IntersectsVolume(Planes))
			Objects.Add(pCamera->GetTarget());
	}

	// TODO: lights and cameras.
}

/////////////////////////////////////////////////////////////////////////////
// Mouse handling

// Returns true if the mouse was being tracked.
bool Project::StopTracking(bool bAccept)
{
	if (m_nTracking == LC_TRACK_NONE)
		return false;

	int Action = GetAction();

	if ((m_nTracking == LC_TRACK_START_LEFT) || (m_nTracking == LC_TRACK_START_RIGHT))
	{
		if (m_pTrackFile)
		{
			delete m_pTrackFile;
			m_pTrackFile = NULL;
		}

		m_nTracking = LC_TRACK_NONE;
		m_ActiveView->ReleaseMouse();
		return false;
	}

	m_bTrackCancel = true;
	m_nTracking = LC_TRACK_NONE;
	m_ActiveView->ReleaseMouse();

	// Reset the mouse overlay.
	if (m_OverlayActive)
	{
		ActivateOverlay(m_ActiveView, m_nCurAction, LC_OVERLAY_NONE);
		UpdateAllViews();
	}

	if (bAccept)
	{
		if (mDropPiece)
		{
			int x = m_nDownX;
			int y = m_nDownY;

			if ((x > 0) && (x < m_ActiveView->mWidth) && (y > 0) && (y < m_ActiveView->mHeight))
			{
				lcVector3 Pos;
				lcVector4 Rot;

				GetPieceInsertPosition(m_ActiveView, x, y, Pos, Rot);

				Piece* pPiece = new Piece(mDropPiece);
				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_nCurStep, m_nCurFrame);
				pPiece->SetColorIndex(gMainWindow->mColorIndex);

				pPiece->ChangeKey(m_nCurStep, false, false, Rot, LC_PK_ROTATION);
				pPiece->ChangeKey(m_nCurFrame, true, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				SelectAndFocusNone(false);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				SystemPieceComboAdd(mDropPiece->m_strDescription);
				pPiece->Select (true, true, false);

				if (mDropPiece)
				{
					mDropPiece->Release();
					mDropPiece = NULL;
				}

				UpdateSelection();
				UpdateAllViews();
				gMainWindow->UpdateFocusObject(pPiece);

				SetModifiedFlag(true);
				CheckPoint("Inserting");
			}
		}
		else
		{
		switch (Action)
		{
			case LC_ACTION_SELECT:
			{
				if (((float)m_nDownX != m_fTrack[0]) && ((float)m_nDownY != m_fTrack[1]))
				{
					// Find objects inside the rectangle.
					lcArray<Object*> Objects;
					FindObjectsInBox((float)m_nDownX, (float)m_nDownY, m_fTrack[0], m_fTrack[1], Objects);

					// Deselect old pieces.
					bool Control = m_ActiveView->mInputState.Control;
					SelectAndFocusNone(Control);

					// Select new pieces.
					for (int i = 0; i < Objects.GetSize(); i++)
					{
						if (Objects[i]->GetType() == LC_OBJECT_PIECE)
						{
							Group* pGroup = ((Piece*)Objects[i])->GetTopGroup();
							if (pGroup != NULL)
							{
								for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
									if ((pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation)) &&
											(pPiece->GetTopGroup() == pGroup))
										pPiece->Select (true, false, false);
							}
							else
								Objects[i]->Select(true, false, true);
						}
						else
							Objects[i]->Select(true, false, true);
					}
				}

				// Update screen and UI.
				UpdateSelection();
				UpdateAllViews();
					gMainWindow->UpdateFocusObject(GetFocusObject());

			} break;

			case LC_ACTION_MOVE:
			{
				SetModifiedFlag(true);
				CheckPoint("Moving");
			} break;

			case LC_ACTION_ROTATE:
			{
				SetModifiedFlag(true);
				CheckPoint("Rotating");
			} break;

			case LC_ACTION_CAMERA:
			{
					gMainWindow->UpdateCameraMenu(mCameras, m_ActiveView ? m_ActiveView->mCamera : NULL);
				SetModifiedFlag(true);
				CheckPoint("Inserting");
			} break;

			case LC_ACTION_SPOTLIGHT:
			{
				SetModifiedFlag(true);
				CheckPoint("Inserting");
			} break;

			case LC_ACTION_ZOOM:
			case LC_ACTION_PAN:
			case LC_ACTION_ROTATE_VIEW:
			case LC_ACTION_ROLL:
			{
				// For some reason the scene doesn't get redrawn when changing a camera but it does
				// when moving things around, so manually get the full scene rendered again.
				if (m_nDetail & LC_DET_FAST)
					UpdateAllViews();
			} break;

			case LC_ACTION_ZOOM_REGION:
			{
				// Find the top-left and bottom-right corners in screen coordinates.
				float Left, Top, Bottom, Right;

				if (m_OverlayTrackStart[0] < m_nDownX)
				{
					Left = m_OverlayTrackStart[0];
					Right = (float)m_nDownX;
				}
				else
				{
					Left = (float)m_nDownX;
					Right = m_OverlayTrackStart[0];
				}

				if (m_OverlayTrackStart[1] > m_nDownY)
				{
					Top = m_OverlayTrackStart[1];
					Bottom = (float)m_nDownY;
				}
				else
				{
					Top = (float)m_nDownY;
					Bottom = m_OverlayTrackStart[1];
				}

				m_ActiveView->mCamera->ZoomRegion(m_ActiveView, Left, Right, Bottom, Top, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);

					gMainWindow->UpdateFocusObject(GetFocusObject());
				UpdateAllViews();
			} break;

			case LC_ACTION_INSERT:
			case LC_ACTION_LIGHT:
			case LC_ACTION_ERASER:
			case LC_ACTION_PAINT:
				break;
		}
		}
	}
	else if (m_pTrackFile != NULL)
	{
		if ((Action == LC_ACTION_SELECT) || (Action == LC_ACTION_ZOOM_REGION))
		{
			UpdateAllViews();
		}
		else
		{
			DeleteContents (true);
			FileLoad (m_pTrackFile, true, false);
			delete m_pTrackFile;
			m_pTrackFile = NULL;
		}
	}

	if (mDropPiece)
	{
		mDropPiece->Release();
		mDropPiece = NULL;
	}

	return true;
}

void Project::StartTracking(int mode)
{
	m_ActiveView->CaptureMouse();
	m_nTracking = mode;

  if (m_pTrackFile != NULL)
    m_pTrackFile->SetLength (0);
  else
  	m_pTrackFile = new lcMemFile;

	FileSave(m_pTrackFile, true);
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
	if (m_nSnap & LC_DRAW_CM_UNITS)
	{
		float xy, z;

		GetSnapDistance(&xy, &z);

		sprintf(SnapXY, "%.2f", xy);
		sprintf(SnapZ, "%.2f", z);
	}
	else
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

	// Transform the translation if we're in relative mode.
	if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
	{
		Object* Focus = GetFocusObject();

		if ((Focus != NULL) && Focus->IsPiece())
			Move = lcMul30(Move, ((Piece*)Focus)->mModelWorld);
	}

	Piece* pPiece;
	Light* pLight;
	float x = Move[0], y = Move[1], z = Move[2];

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
	{
		Camera* pCamera = mCameras[CameraIdx];

		if (pCamera->IsSelected())
		{
			pCamera->Move(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, x, y, z);
			pCamera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
		}
	}

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		if (pLight->IsSelected())
		{
			pLight->Move (m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, x, y, z);
			pLight->UpdatePosition (m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
		}

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsSelected())
		{
			pPiece->Move(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, x, y, z);
			pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
		}

	// TODO: move group centers

	if (m_OverlayActive)
	{
		if (!GetFocusPosition(m_OverlayCenter))
			GetSelectionCenter(m_OverlayCenter);
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
	Piece *pPiece, *pFocus = NULL;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (pPiece->IsSelected())
		{
			if (pPiece->IsFocused())
				pFocus = pPiece;

			pPiece->CompareBoundingBox(bs);
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

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (!pPiece->IsSelected())
			continue;

		pos = pPiece->mPosition;
		rot = pPiece->mRotation;

		lcVector4 NewRotation;

		if ((nSel == 1) && (pFocus == pPiece))
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

			pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, pos, LC_PK_POSITION);
		}

		rot[0] = NewRotation[0];
		rot[1] = NewRotation[1];
		rot[2] = NewRotation[2];
		rot[3] = NewRotation[3] * LC_RTOD;

		pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, rot, LC_PK_ROTATION);
		pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
	}

	if (m_OverlayActive)
	{
		if (!GetFocusPosition(m_OverlayCenter))
			GetSelectionCenter(m_OverlayCenter);
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
			Piece *pPiece, *pFocus = NULL;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
				{
					if (pPiece->IsFocused())
						pFocus = pPiece;

					pPiece->CompareBoundingBox(bs);
					nSel++;
				}
			}

			if (pFocus != NULL)
				Center = pFocus->mPosition;
			else
				Center = lcVector3((bs[0]+bs[3])/2, (bs[1]+bs[4])/2, (bs[2]+bs[5])/2);

			lcVector3 Offset = Transform - Center;

			Light* pLight;

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
			{
				Camera* pCamera = mCameras[CameraIdx];

				if (pCamera->IsSelected())
				{
					pCamera->Move(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Offset.x, Offset.y, Offset.z);
					pCamera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
				}
			}

			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
			{
				if (pLight->IsSelected())
				{
					pLight->Move(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Offset.x, Offset.y, Offset.z);
					pLight->UpdatePosition (m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
				}
			}

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
				{
					pPiece->Move(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Offset.x, Offset.y, Offset.z);
					pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
				}
			}

			if (m_OverlayActive)
			{
				if (!GetFocusPosition(m_OverlayCenter))
					GetSelectionCenter(m_OverlayCenter);
			}

			if (nSel)
			{
				UpdateOverlayScale();
				UpdateAllViews();
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
				UpdateOverlayScale();
				UpdateAllViews();
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

			Piece *pPiece;
			int nSel = 0;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
				{
					pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, NewRotation, LC_PK_ROTATION);
					pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
					nSel++;
				}
			}

			if (m_OverlayActive)
			{
				if (!GetFocusPosition(m_OverlayCenter))
					GetSelectionCenter(m_OverlayCenter);
			}

			if (nSel)
			{
				UpdateOverlayScale();
				UpdateAllViews();
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
				UpdateOverlayScale();
				UpdateAllViews();
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
	case LC_PART_POSITION:
		{
			const lcVector3& Position = *(lcVector3*)Value;
			Piece* Part = (Piece*)Object;

			if (Part->mPosition != Position)
		{
				Part->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Position, LC_PK_POSITION);
				Part->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				CheckPointString = "Moving";
			}
		} break;

	case LC_PART_ROTATION:
		{
			const lcVector4& Rotation = *(lcVector4*)Value;
			Piece* Part = (Piece*)Object;

			if (Rotation != Part->mRotation)
		{
				Part->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Rotation, LC_PK_ROTATION);
				Part->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				CheckPointString = "Rotating";
			}
		} break;

	case LC_PART_SHOW:
		{
			lcuint32 Show = *(lcuint32*)Value;
			Piece* Part = (Piece*)Object;

			if (m_bAnimation)
			{
				if (Show != Part->GetFrameShow())
				{
					Part->SetFrameShow(Show);

					CheckPointString = "Show";
				}
				}
				else
				{
				if (Show != Part->GetStepShow())
						{
					Part->SetStepShow(Show);

					CheckPointString = "Show";
						}
				}
		} break;

	case LC_PART_HIDE:
			{
			lcuint32 Hide = *(lcuint32*)Value;
			Piece* Part = (Piece*)Object;

			if (m_bAnimation)
				{
				if (Hide != Part->GetFrameHide())
					{
					Part->SetFrameHide(Hide);

					CheckPointString = "Hide";
						}
					}
				else
				{
				if (Hide != Part->GetStepHide())
							{
					Part->SetStepHide(Hide);

					CheckPointString = "Hide";
				}
			}
		} break;

	case LC_PART_COLOR:
			{
			int ColorIndex = *(int*)Value;
			Piece* Part = (Piece*)Object;

			if (ColorIndex != Part->mColorIndex)
        {
				Part->SetColorIndex(ColorIndex);

				CheckPointString = "Color";
			}
		} break;

	case LC_PART_ID:
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

	case LC_CAMERA_POSITION:
			{
			const lcVector3& Position = *(lcVector3*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->mPosition != Position)
{
				camera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Position, LC_CK_EYE);
				camera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_TARGET:
			{
			const lcVector3& TargetPosition = *(lcVector3*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->mTargetPosition != TargetPosition)
				{
				camera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, TargetPosition, LC_CK_TARGET);
				camera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
				}
		} break;

	case LC_CAMERA_UP:
				{
			const lcVector3& Up = *(lcVector3*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->mUpVector != Up)
					{
				camera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Up, LC_CK_UP);
				camera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_FOV:
		{
			float FOV = *(float*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->m_fovy != FOV)
		{
				camera->m_fovy = FOV;
				camera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
					}
		} break;

	case LC_CAMERA_NEAR:
				{
			float Near = *(float*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->m_zNear != Near)
			{
				camera->m_zNear= Near;
				camera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
					}
		} break;

	case LC_CAMERA_FAR:
		{
			float Far = *(float*)Value;
			Camera* camera = (Camera*)Object;

			if (camera->m_zFar != Far)
		{
				camera->m_zFar = Far;
				camera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				CheckPointString = "Camera";
			}
		} break;

	case LC_CAMERA_NAME:
		{
			const char* Name = (const char*)Value;
			Camera* camera = (Camera*)Object;

			if (strcmp(camera->m_strName, Name))
			{
				strncpy(camera->m_strName, Name, sizeof(camera->m_strName));
				camera->m_strName[sizeof(camera->m_strName) - 1] = 0;

				gMainWindow->UpdateCameraMenu(mCameras, m_ActiveView ? m_ActiveView->mCamera : NULL);

				CheckPointString = "Camera";
			}
		}
	}

	if (CheckPointString)
					{
		SetModifiedFlag(true);
		CheckPoint(CheckPointString);
		gMainWindow->UpdateFocusObject(GetFocusObject());
		ActivateOverlay(m_ActiveView, m_nCurAction, LC_OVERLAY_NONE);
		UpdateAllViews();
	}
}

void Project::ZoomActiveView(int Amount)
						{
	m_ActiveView->mCamera->DoZoom(Amount, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
	gMainWindow->UpdateFocusObject(GetFocusObject());
	UpdateOverlayScale();
	UpdateAllViews();
						}

void Project::BeginPieceDrop(PieceInfo* Info)
						{
	StartTracking(LC_TRACK_LEFT);

	mDropPiece = Info;
	mDropPiece->AddRef();
			}

void Project::OnPieceDropMove(int x, int y)
			{
	if (!mDropPiece)
		return;

	if (m_nDownX != x || m_nDownY != y)
			{
		m_nDownX = x;
		m_nDownY = y;

			UpdateAllViews();
	}
}

void Project::EndPieceDrop(bool Accept)
{
	StopTracking(Accept);

	if (!Accept)
			UpdateAllViews();
}

void Project::BeginColorDrop()
{
	StartTracking(LC_TRACK_LEFT);
	SetAction(LC_ACTION_PAINT);
//	m_RestoreAction = true;
}

void Project::OnLeftButtonDown(View* view)
{
	if (m_nTracking != LC_TRACK_NONE)
		if (StopTracking(false))
			return;

	if (SetActiveView(view))
		return;

	int x = view->mInputState.x;
	int y = view->mInputState.y;
	bool Control = view->mInputState.Control;
	bool Alt = view->mInputState.Alt;

	m_bTrackCancel = false;
	m_nDownX = x;
	m_nDownY = y;
	m_MouseTotalDelta = lcVector3(0, 0, 0);
	m_MouseSnapLeftover = lcVector3(0, 0, 0);

	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->mCamera;

	const lcMatrix44& ModelView = Cam->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	lcVector3 point = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	if (Alt)
		ActivateOverlay(view, LC_ACTION_ROTATE_VIEW, LC_OVERLAY_ROTATE_VIEW_XYZ);

	int Action = GetAction();

	switch (Action)
	{
		case LC_ACTION_SELECT:
		case LC_ACTION_ERASER:
		case LC_ACTION_PAINT:
		{
			Object* Closest = FindObjectFromPoint(view, x, y);

			if (Action == LC_ACTION_SELECT)
			{
				if (Closest != NULL)
				{
					switch (Closest->GetType ())
					{
						case LC_OBJECT_PIECE:
						{
							Piece* pPiece = (Piece*)Closest;
							Group* pGroup = pPiece->GetTopGroup();
							bool bFocus = pPiece->IsFocused ();

							SelectAndFocusNone(Control);

							// if a piece has focus deselect it, otherwise set the focus
							pPiece->Select (!bFocus, !bFocus, false);

							if (pGroup != NULL)
								for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
									if (pPiece->GetTopGroup() == pGroup)
										pPiece->Select (!bFocus, false, false);
						} break;

						case LC_OBJECT_CAMERA:
						case LC_OBJECT_CAMERA_TARGET:
						case LC_OBJECT_LIGHT:
						case LC_OBJECT_LIGHT_TARGET:
						{
							SelectAndFocusNone(Control);
							Closest->Select(true, true, Control);
						} break;
					}
				}
				else
					SelectAndFocusNone(Control);

				UpdateSelection();
				UpdateAllViews();
				gMainWindow->UpdateFocusObject(Closest);

				StartTracking(LC_TRACK_START_LEFT);
			}

			if ((Action == LC_ACTION_ERASER) && (Closest != NULL))
			{
				switch (Closest->GetType ())
				{
					case LC_OBJECT_PIECE:
					{
						Piece* pPiece = (Piece*)Closest;
						RemovePiece(pPiece);
						delete pPiece;
//						CalculateStep();
						RemoveEmptyGroups();
					} break;

					case LC_OBJECT_CAMERA:
					case LC_OBJECT_CAMERA_TARGET:
					{
						Camera* pCamera;
						if (Closest->GetType() == LC_OBJECT_CAMERA)
							pCamera = (Camera*)Closest;
						else
							pCamera = ((CameraTarget*)Closest)->GetParent();

						bool CanDelete = true;

						for (int ViewIdx = 0; ViewIdx < m_ViewList.GetSize() && CanDelete; ViewIdx++)
							CanDelete = pCamera != m_ViewList[ViewIdx]->mCamera;

						if (CanDelete)
						{
							mCameras.Remove(pCamera);
							delete pCamera;

							gMainWindow->UpdateCameraMenu(mCameras, m_ActiveView ? m_ActiveView->mCamera : NULL);
						}
					} break;

					case LC_OBJECT_LIGHT:
					case LC_OBJECT_LIGHT_TARGET:
					{
//						pos = m_Lights.Find(pObject->m_pParent);
//						m_Lights.RemoveAt(pos);
//						delete pObject->m_pParent;
					} break;
				}

				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Deleting");
//				AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);
			}

			if ((Action == LC_ACTION_PAINT) && (Closest != NULL) && (Closest->GetType() == LC_OBJECT_PIECE))
			{
				Piece* pPiece = (Piece*)Closest;

				if (pPiece->mColorIndex != gMainWindow->mColorIndex)
				{
					pPiece->SetColorIndex(gMainWindow->mColorIndex);

					SetModifiedFlag(true);
					CheckPoint("Painting");
					gMainWindow->UpdateFocusObject(GetFocusObject());
					UpdateAllViews();
				}
			}
		} break;

		case LC_ACTION_INSERT:
		case LC_ACTION_LIGHT:
		{
			if (Action == LC_ACTION_INSERT)
			{
				lcVector3 Pos;
				lcVector4 Rot;

				GetPieceInsertPosition(view, x, y, Pos, Rot);

				Piece* pPiece = new Piece(m_pCurPiece);
				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_nCurStep, m_nCurFrame);
				pPiece->SetColorIndex(gMainWindow->mColorIndex);

				pPiece->ChangeKey(m_nCurStep, false, false, Rot, LC_PK_ROTATION);
				pPiece->ChangeKey(m_nCurFrame, true, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				SelectAndFocusNone(false);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				pPiece->Select (true, true, false);
				UpdateSelection();
				SystemPieceComboAdd(m_pCurPiece->m_strDescription);
				gMainWindow->UpdateFocusObject(pPiece);

				if (!Control)
					SetAction(LC_ACTION_SELECT);
			}
			else if (Action == LC_ACTION_LIGHT)
			{
				GLint max;
				int count = 0;
				Light *pLight;

				glGetIntegerv (GL_MAX_LIGHTS, &max);
				for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
					count++;

				if (count == max)
					break;

				pLight = new Light (m_fTrack[0], m_fTrack[1], m_fTrack[2]);

				SelectAndFocusNone (false);

				pLight->CreateName(m_pLights);
				pLight->m_pNext = m_pLights;
				m_pLights = pLight;
				gMainWindow->UpdateFocusObject(pLight);
				pLight->Select (true, true, false);
				UpdateSelection ();
			}

//			AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)pNew, OT_PIECE);
			UpdateSelection();
			UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Inserting");
		} break;

    case LC_ACTION_SPOTLIGHT:
    {
      GLint max;
      int count = 0;
      Light *pLight;

      glGetIntegerv (GL_MAX_LIGHTS, &max);
      for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
        count++;

      if (count == max)
        break;

	  lcVector3 tmp = lcUnprojectPoint(lcVector3(x+1.0f, y-1.0f, 0.9f), ModelView, Projection, Viewport);
      SelectAndFocusNone(false);
      StartTracking(LC_TRACK_START_LEFT);
      pLight = new Light (m_fTrack[0], m_fTrack[1], m_fTrack[2], tmp[0], tmp[1], tmp[2]);
      pLight->GetTarget ()->Select (true, true, false);
      pLight->m_pNext = m_pLights;
      m_pLights = pLight;
      UpdateSelection();
      UpdateAllViews();
	  gMainWindow->UpdateFocusObject(pLight);
    } break;

		case LC_ACTION_CAMERA:
		{
			lcVector3 tmp = lcUnprojectPoint(lcVector3(x+1.0f, y-1.0f, 0.9f), ModelView, Projection, Viewport);
			SelectAndFocusNone(false);
			StartTracking(LC_TRACK_START_LEFT);

			Camera* NewCamera = new Camera(m_fTrack[0], m_fTrack[1], m_fTrack[2], tmp[0], tmp[1], tmp[2]);
			NewCamera->GetTarget()->Select (true, true, false);
			NewCamera->CreateName(mCameras);
			mCameras.Add(NewCamera);

			UpdateSelection();
			UpdateAllViews();
			gMainWindow->UpdateFocusObject(NewCamera);
		} break;

		case LC_ACTION_MOVE:
		{
			bool sel = false;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
				{
					sel = true;
					break;
				}
			}

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize() && !sel; CameraIdx++)
				sel = mCameras[CameraIdx]->IsSelected();

			if (!sel)
			{
				for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				{
					if (pLight->IsSelected())
					{
						sel = true;
						break;
					}
				}
			}

			if (sel)
			{
				StartTracking(LC_TRACK_START_LEFT);
				m_OverlayDelta = lcVector3(0.0f, 0.0f, 0.0f);
				m_MouseSnapLeftover = lcVector3(0.0f, 0.0f, 0.0f);
			}
		} break;

		case LC_ACTION_ROTATE:
		{
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if (pPiece->IsSelected())
				{
					StartTracking(LC_TRACK_START_LEFT);
					m_OverlayDelta = lcVector3(0.0f, 0.0f, 0.0f);
					break;
				}
			}
		} break;

		case LC_ACTION_ZOOM_REGION:
		{
			m_OverlayTrackStart[0] = (float)x;
			m_OverlayTrackStart[1] = (float)y;
			StartTracking(LC_TRACK_START_LEFT);
			ActivateOverlay(view, m_nCurAction, LC_OVERLAY_NONE);
		} break;

		case LC_ACTION_ZOOM:
		case LC_ACTION_ROLL:
		case LC_ACTION_PAN:
		case LC_ACTION_ROTATE_VIEW:
		{
			StartTracking(LC_TRACK_START_LEFT);
		} break;
	}
}

void Project::OnLeftButtonDoubleClick(View* view)
{
	if (SetActiveView(view))
		return;

	int x = view->mInputState.x;
	int y = view->mInputState.y;
	bool Control = view->mInputState.Control;

	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->mCamera;

	const lcMatrix44& ModelView = Cam->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	lcVector3 point = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	Object* Closest = FindObjectFromPoint(view, x, y);

//  if (m_nCurAction == LC_ACTION_SELECT)
  {
	SelectAndFocusNone(Control);

    if (Closest != NULL)
      switch (Closest->GetType ())
      {
        case LC_OBJECT_PIECE:
        {
          Piece* pPiece = (Piece*)Closest;
          pPiece->Select (true, true, false);
          Group* pGroup = pPiece->GetTopGroup();

          if (pGroup != NULL)
            for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
              if (pPiece->GetTopGroup() == pGroup)
                pPiece->Select (true, false, false);
        } break;

        case LC_OBJECT_CAMERA:
        case LC_OBJECT_CAMERA_TARGET:
        case LC_OBJECT_LIGHT:
        case LC_OBJECT_LIGHT_TARGET:
        {
		  Closest->Select (true, true, Control);
        } break;
      }

    UpdateSelection();
    UpdateAllViews();
	gMainWindow->UpdateFocusObject(Closest);
  }
}

void Project::OnLeftButtonUp(View* view)
		{
	StopTracking(true);
}

void Project::OnMiddleButtonDown(View* view)
{
	if (StopTracking(false))
		return;

	if (SetActiveView(view))
		return;

	int x = view->mInputState.x;
	int y = view->mInputState.y;
	bool Alt = view->mInputState.Alt;

	m_nDownX = x;
	m_nDownY = y;
	m_bTrackCancel = false;

	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->mCamera;

	const lcMatrix44& ModelView = Cam->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	lcVector3 point = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	if (Alt)
		ActivateOverlay(view, LC_ACTION_PAN, LC_OVERLAY_PAN);

	switch (GetAction())
	{
		case LC_ACTION_PAN:
		{
			StartTracking(LC_TRACK_START_RIGHT);
		} break;
	}
}

void Project::OnMiddleButtonUp(View* view)
{
	StopTracking(true);
}

void Project::OnRightButtonDown(View* view)
{
	if (StopTracking(false))
		return;

	if (SetActiveView(view))
		return;

	int x = view->mInputState.x;
	int y = view->mInputState.y;
	bool Alt = view->mInputState.Alt;

	m_nDownX = x;
	m_nDownY = y;
	m_bTrackCancel = false;

	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->mCamera;

	const lcMatrix44& ModelView = Cam->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	lcVector3 point = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	if (Alt)
		ActivateOverlay(view, LC_ACTION_ZOOM, LC_OVERLAY_ZOOM);

	switch (GetAction())
	{
		case LC_ACTION_MOVE:
		{
			bool sel = false;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					sel = true;
					break;
				}

			for (int CameraIdx = 0; CameraIdx < mCameras.GetSize() && !sel; CameraIdx++)
				sel = mCameras[CameraIdx]->IsSelected();

			if (!sel)
			for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsSelected())
				{
					sel = true;
					break;
				}

			if (sel)
			{
				StartTracking(LC_TRACK_START_RIGHT);
				m_fTrack[0] = m_fTrack[1] = m_fTrack[2] = 0.0f;
			}
		} break;

		case LC_ACTION_ROTATE:
		{
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					StartTracking(LC_TRACK_START_RIGHT);
					m_fTrack[0] = m_fTrack[1] = m_fTrack[2] = 0.0f;
					break;
				}
		} break;

		case LC_ACTION_ZOOM:
		{
			StartTracking(LC_TRACK_START_RIGHT);
		} break;
	}
}

void Project::OnRightButtonUp(View* view)
{
	if (!StopTracking(true) && !m_bTrackCancel)
		view->ShowPopupMenu();
	m_bTrackCancel = false;
}

void Project::OnMouseMove(View* view)
{
	int x = view->mInputState.x;
	int y = view->mInputState.y;

	if ((m_nTracking == LC_TRACK_NONE) && (m_nCurAction != LC_ACTION_INSERT))
	{
		if (m_OverlayActive)
			MouseUpdateOverlays(view, x, y);

		return;
	}

	if (m_nTracking == LC_TRACK_START_RIGHT)
		m_nTracking = LC_TRACK_RIGHT;

	if (m_nTracking == LC_TRACK_START_LEFT)
		m_nTracking = LC_TRACK_LEFT;

	float ptx, pty, ptz;

	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->mCamera;

	const lcMatrix44& ModelView = Cam->mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	lcVector3 tmp = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	ptx = tmp[0]; pty = tmp[1]; ptz = tmp[2];

	switch (GetAction())
	{
		case LC_ACTION_SELECT:
		{
			int ClampX = x, ClampY = y;

			if (ClampX >= Viewport[0] + Viewport[2])
				ClampX = Viewport[0] + Viewport[2] - 1;
			else if (ClampX <= Viewport[0])
				ClampX = Viewport[0] + 1;

			if (ClampY >= Viewport[1] + Viewport[3])
				ClampY = Viewport[1] + Viewport[3] - 1;
			else if (ClampY <= Viewport[1])
				ClampY = Viewport[1] + 1;

			m_fTrack[0] = (float)ClampX;
			m_fTrack[1] = (float)ClampY;

			if (m_nTracking != LC_TRACK_NONE)
			{
				ActivateOverlay(view, m_nCurAction, LC_OVERLAY_NONE);
				UpdateOverlayScale();
			}

			UpdateAllViews();
		} break;

		case LC_ACTION_INSERT:
		{
			if (m_nDownX != x || m_nDownY != y)
			{
				m_nDownX = x;
				m_nDownY = y;

				UpdateAllViews();
			}
		}	break;

		case LC_ACTION_SPOTLIGHT:
		{
			float mouse = 10.0f/(21 - m_nMouse);
			float delta[3] = { (ptx - m_fTrack[0])*mouse,
				(pty - m_fTrack[1])*mouse, (ptz - m_fTrack[2])*mouse };

			m_fTrack[0] = ptx;
			m_fTrack[1] = pty;
			m_fTrack[2] = ptz;

			Light* pLight = m_pLights;

			pLight->Move (1, m_bAnimation, false, delta[0], delta[1], delta[2]);
			pLight->UpdatePosition (1, m_bAnimation);

			gMainWindow->UpdateFocusObject(pLight);
			UpdateAllViews();
		} break;

		case LC_ACTION_CAMERA:
		{
			float mouse = 10.0f/(21 - m_nMouse);
			float delta[3] = { (ptx - m_fTrack[0])*mouse,
				(pty - m_fTrack[1])*mouse, (ptz - m_fTrack[2])*mouse };

			m_fTrack[0] = ptx;
			m_fTrack[1] = pty;
			m_fTrack[2] = ptz;

			Camera* pCamera = mCameras[mCameras.GetSize() - 1];

			pCamera->Move(1, m_bAnimation, false, delta[0], delta[1], delta[2]);
			pCamera->UpdatePosition(1, m_bAnimation);

			gMainWindow->UpdateFocusObject(pCamera);
			UpdateAllViews();
		} break;

		case LC_ACTION_MOVE:
		{
			// Check if the mouse moved since the last update.
			if ((x == m_nDownX) && (y == m_nDownY))
				break;

			Camera* Camera = view->mCamera;
			bool Redraw;

			if ((m_OverlayActive && (m_OverlayMode != LC_OVERLAY_MOVE_XYZ)) || (!Camera->IsSide()))
			{
				lcVector3 ScreenX = lcNormalize(lcCross(Camera->mTargetPosition - Camera->mPosition, Camera->mUpVector));
				lcVector3 ScreenY = Camera->mUpVector;
				lcVector3 Dir1(0.0f, 0.0f, 0.0f), Dir2(0.0f, 0.0f, 0.0f);
				bool SingleDir = true;

				int OverlayMode;

				if (m_OverlayActive && (m_OverlayMode != LC_OVERLAY_MOVE_XYZ))
					OverlayMode = m_OverlayMode;
				else if (m_nTracking == LC_TRACK_LEFT)
					OverlayMode = LC_OVERLAY_MOVE_XY;
				else
					OverlayMode = LC_OVERLAY_MOVE_Z;

				switch (OverlayMode)
				{
				case LC_OVERLAY_MOVE_X:
					Dir1 = lcVector3(1, 0, 0);
					break;
				case LC_OVERLAY_MOVE_Y:
					Dir1 = lcVector3(0, 1, 0);
					break;
				case LC_OVERLAY_MOVE_Z:
					Dir1 = lcVector3(0, 0, 1);
					break;
				case LC_OVERLAY_MOVE_XY:
					Dir1 = lcVector3(1, 0, 0);
					Dir2 = lcVector3(0, 1, 0);
					SingleDir = false;
					break;
				case LC_OVERLAY_MOVE_XZ:
					Dir1 = lcVector3(1, 0, 0);
					Dir2 = lcVector3(0, 0, 1);
					SingleDir = false;
					break;
				case LC_OVERLAY_MOVE_YZ:
					Dir1 = lcVector3(0, 1, 0);
					Dir2 = lcVector3(0, 0, 1);
					SingleDir = false;
					break;
				}

				// Transform the translation axis.
				lcVector3 Axis1 = Dir1;
				lcVector3 Axis2 = Dir2;

				if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
				{
					Object* Focus = GetFocusObject();

					if ((Focus != NULL) && Focus->IsPiece())
					{
						const lcMatrix44& ModelWorld = ((Piece*)Focus)->mModelWorld;

						Axis1 = lcMul30(Dir1, ModelWorld);
						Axis2 = lcMul30(Dir2, ModelWorld);
					}
				}

				// Find out what direction the mouse is going to move stuff.
				lcVector3 MoveX, MoveY;

				if (SingleDir)
				{
					float dx1 = lcDot(ScreenX, Axis1);
					float dy1 = lcDot(ScreenY, Axis1);

					if (fabsf(dx1) > fabsf(dy1))
					{
						if (dx1 >= 0.0f)
							MoveX = Dir1;
						else
							MoveX = -Dir1;

						MoveY = lcVector3(0, 0, 0);
					}
					else
					{
						MoveX = lcVector3(0, 0, 0);

						if (dy1 > 0.0f)
							MoveY = Dir1;
						else
							MoveY = -Dir1;
					}
				}
				else
				{
					float dx1 = lcDot(ScreenX, Axis1);
					float dy1 = lcDot(ScreenY, Axis1);
					float dx2 = lcDot(ScreenX, Axis2);
					float dy2 = lcDot(ScreenY, Axis2);

					if (fabsf(dx1) > fabsf(dx2))
					{
						if (dx1 >= 0.0f)
							MoveX = Dir1;
						else
							MoveX = -Dir1;

						if (dy2 >= 0.0f)
							MoveY = Dir2;
						else
							MoveY = -Dir2;
					}
					else
					{
						if (dx2 >= 0.0f)
							MoveX = Dir2;
						else
							MoveX = -Dir2;

						if (dy1 > 0.0f)
							MoveY = Dir1;
						else
							MoveY = -Dir1;
					}
				}

				MoveX *= (float)(x - m_nDownX) * 0.25f / (21 - m_nMouse);
				MoveY *= (float)(y - m_nDownY) * 0.25f / (21 - m_nMouse);

				m_nDownX = x;
				m_nDownY = y;

				lcVector3 Delta = MoveX + MoveY + m_MouseSnapLeftover;
				Redraw = MoveSelectedObjects(Delta, m_MouseSnapLeftover, true, true);
				m_MouseTotalDelta += Delta;
			}
			else
			{
				// 3D movement.
				lcVector3 ScreenZ = lcNormalize(Camera->mTargetPosition - Camera->mPosition);
				lcVector3 ScreenX = lcCross(ScreenZ, Camera->mUpVector);
				lcVector3 ScreenY = Camera->mUpVector;

				lcVector3 TotalMove;

				if (m_nTracking == LC_TRACK_LEFT)
				{
					lcVector3 MoveX, MoveY;

					MoveX = ScreenX * (float)(x - m_nDownX) * 0.25f / (float)(21 - m_nMouse);
					MoveY = ScreenY * (float)(y - m_nDownY) * 0.25f / (float)(21 - m_nMouse);

					TotalMove = MoveX + MoveY + m_MouseSnapLeftover;
				}
				else
				{
					lcVector3 MoveZ;

					MoveZ = ScreenZ * (float)(y - m_nDownY) * 0.25f / (float)(21 - m_nMouse);

					TotalMove = MoveZ + m_MouseSnapLeftover;
				}

				m_nDownX = x;
				m_nDownY = y;

				Redraw = MoveSelectedObjects(TotalMove, m_MouseSnapLeftover, true, true);
			}

			gMainWindow->UpdateFocusObject(GetFocusObject());

			if (m_nTracking != LC_TRACK_NONE)
				UpdateOverlayScale();

			if (Redraw)
				UpdateAllViews();
		} break;

		case LC_ACTION_ROTATE:
		{
			Camera* Camera = m_ActiveView->mCamera;
			bool Redraw;

			if ((m_OverlayActive && (m_OverlayMode != LC_OVERLAY_ROTATE_XYZ)) || (!Camera->IsSide()))
			{
				lcVector3 ScreenX = lcNormalize(lcCross(Camera->mTargetPosition - Camera->mPosition, Camera->mUpVector));
				lcVector3 ScreenY = Camera->mUpVector;
				lcVector3 Dir1, Dir2;
				bool SingleDir = true;

				int OverlayMode;

				if (m_OverlayActive && (m_OverlayMode != LC_OVERLAY_ROTATE_XYZ))
					OverlayMode = m_OverlayMode;
				else if (m_nTracking == LC_TRACK_LEFT)
					OverlayMode = LC_OVERLAY_ROTATE_XY;
				else
					OverlayMode = LC_OVERLAY_ROTATE_Z;

				switch (OverlayMode)
				{
				case LC_OVERLAY_ROTATE_X:
					Dir1 = lcVector3(1, 0, 0);
					break;
				case LC_OVERLAY_ROTATE_Y:
					Dir1 = lcVector3(0, 1, 0);
					break;
				case LC_OVERLAY_ROTATE_Z:
					Dir1 = lcVector3(0, 0, 1);
					break;
				case LC_OVERLAY_ROTATE_XY:
					Dir1 = lcVector3(1, 0, 0);
					Dir2 = lcVector3(0, 1, 0);
					SingleDir = false;
					break;
				case LC_OVERLAY_ROTATE_XZ:
					Dir1 = lcVector3(1, 0, 0);
					Dir2 = lcVector3(0, 0, 1);
					SingleDir = false;
					break;
				case LC_OVERLAY_ROTATE_YZ:
					Dir1 = lcVector3(0, 1, 0);
					Dir2 = lcVector3(0, 0, 1);
					SingleDir = false;
					break;
				default:
					Dir1 = lcVector3(1, 0, 0);
					break;
				}

				// Find out what direction the mouse is going to move stuff.
				lcVector3 MoveX, MoveY;

				if (SingleDir)
				{
					float dx1 = lcDot(ScreenX, Dir1);
					float dy1 = lcDot(ScreenY, Dir1);

					if (fabsf(dx1) > fabsf(dy1))
					{
						if (dx1 >= 0.0f)
							MoveX = Dir1;
						else
							MoveX = -Dir1;

						MoveY = lcVector3(0, 0, 0);
					}
					else
					{
						MoveX = lcVector3(0, 0, 0);

						if (dy1 > 0.0f)
							MoveY = Dir1;
						else
							MoveY = -Dir1;
					}
				}
				else
				{
					float dx1 = lcDot(ScreenX, Dir1);
					float dy1 = lcDot(ScreenY, Dir1);
					float dx2 = lcDot(ScreenX, Dir2);
					float dy2 = lcDot(ScreenY, Dir2);

					if (fabsf(dx1) > fabsf(dx2))
					{
						if (dx1 >= 0.0f)
							MoveX = Dir1;
						else
							MoveX = -Dir1;

						if (dy2 >= 0.0f)
							MoveY = Dir2;
						else
							MoveY = -Dir2;
					}
					else
					{
						if (dx2 >= 0.0f)
							MoveX = Dir2;
						else
							MoveX = -Dir2;

						if (dy1 > 0.0f)
							MoveY = Dir1;
						else
							MoveY = -Dir1;
					}
				}

				MoveX *= (float)(x - m_nDownX) * 36.0f / (21 - m_nMouse);
				MoveY *= (float)(y - m_nDownY) * 36.0f / (21 - m_nMouse);

				m_nDownX = x;
				m_nDownY = y;

				lcVector3 Delta = MoveX + MoveY + m_MouseSnapLeftover;
				Redraw = RotateSelectedObjects(Delta, m_MouseSnapLeftover, true, true);
				m_MouseTotalDelta += Delta;
			}
			else
			{
				// 3D movement.
				lcVector3 ScreenZ = lcNormalize(Camera->mTargetPosition - Camera->mPosition);
				lcVector3 ScreenX = lcCross(ScreenZ, Camera->mUpVector);
				lcVector3 ScreenY = Camera->mUpVector;

				lcVector3 Delta;

				if (m_nTracking == LC_TRACK_LEFT)
				{
					lcVector3 MoveX, MoveY;

					MoveX = ScreenX * (float)(x - m_nDownX) * 36.0f / (float)(21 - m_nMouse);
					MoveY = ScreenY * (float)(y - m_nDownY) * 36.0f / (float)(21 - m_nMouse);

					Delta = MoveX + MoveY + m_MouseSnapLeftover;
				}
				else
				{
					lcVector3 MoveZ;

					MoveZ = ScreenZ * (float)(y - m_nDownY) * 36.0f / (float)(21 - m_nMouse);

					Delta = MoveZ + m_MouseSnapLeftover;
				}

				m_nDownX = x;
				m_nDownY = y;

				Redraw = RotateSelectedObjects(Delta, m_MouseSnapLeftover, true, true);
				m_MouseTotalDelta += Delta;
			}

			gMainWindow->UpdateFocusObject(GetFocusObject());
			if (Redraw)
				UpdateAllViews();
		} break;

		case LC_ACTION_ZOOM:
		{
			if (m_nDownY == y)
				break;

			m_ActiveView->mCamera->DoZoom(y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownY = y;
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();
		} break;

		case LC_ACTION_ZOOM_REGION:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			m_nDownX = x;
			m_nDownY = y;
			UpdateAllViews();
		} break;

		case LC_ACTION_PAN:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			m_ActiveView->mCamera->DoPan(x - m_nDownX, y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownX = x;
			m_nDownY = y;
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();
		} break;

		case LC_ACTION_ROTATE_VIEW:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
					pPiece->CompareBoundingBox(bs);
			bs[0] = (bs[0]+bs[3])/2;
			bs[1] = (bs[1]+bs[4])/2;
			bs[2] = (bs[2]+bs[5])/2;

			switch (m_OverlayMode)
			{
				case LC_OVERLAY_ROTATE_VIEW_XYZ:
					m_ActiveView->mCamera->DoRotate(x - m_nDownX, y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, bs);
					break;

				case LC_OVERLAY_ROTATE_VIEW_X:
					m_ActiveView->mCamera->DoRotate(x - m_nDownX, 0, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, bs);
					break;

				case LC_OVERLAY_ROTATE_VIEW_Y:
					m_ActiveView->mCamera->DoRotate(0, y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, bs);
					break;

				case LC_OVERLAY_ROTATE_VIEW_Z:
					m_ActiveView->mCamera->DoRoll(x - m_nDownX, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
					break;
			}

			m_nDownX = x;
			m_nDownY = y;
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();
		} break;

		case LC_ACTION_ROLL:
		{
			if (m_nDownX == x)
				break;

			m_ActiveView->mCamera->DoRoll(x - m_nDownX, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownX = x;
			gMainWindow->UpdateFocusObject(GetFocusObject());
			UpdateAllViews();
		} break;
		/*
    case LC_ACTION_CURVE:
    {
      float mouse = 10.0f/(21 - m_nMouse);
      float dx = (ptx - m_fTrack[0])*mouse;
      float dy = (pty - m_fTrack[1])*mouse;
      float dz = (ptz - m_fTrack[2])*mouse;
      Object *pObj = NULL;
      Curve *pCurve;

      m_fTrack[0] = ptx;
      m_fTrack[1] = pty;
      m_fTrack[2] = ptz;

      for (pObj = m_pObjects; pObj != NULL; pObj = pObj->m_pNext)
	if (pObj->IsSelected ())
	  break;

      if (pObj == NULL)
	break;
      pCurve = (Curve*)pObj;

      pCurve->Move (1, m_bAnimation, false, dx, dy, dz);
      pCurve->UpdatePosition(1, m_bAnimation);

	  gMainWindow->UpdateFocusObject(GetFocusObject());
      UpdateAllViews();
    } break;
                */
	}
}

void Project::OnMouseWheel(View* view, float Direction)
{
	ZoomActiveView((int)(10 * Direction));
}

// Check if the mouse is over a different area of the overlay and redraw it.
void Project::MouseUpdateOverlays(View* view, int x, int y)
{
	const float OverlayScale = view->m_OverlayScale;

	if (m_nCurAction == LC_ACTION_SELECT || m_nCurAction == LC_ACTION_MOVE)
	{
		const float OverlayMovePlaneSize = 0.5f * OverlayScale;
		const float OverlayMoveArrowSize = 1.5f * OverlayScale;
		const float OverlayMoveArrowCapRadius = 0.1f * OverlayScale;
		const float OverlayRotateArrowStart = 1.0f * OverlayScale;
		const float OverlayRotateArrowEnd = 1.5f * OverlayScale;

		int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
		float Aspect = (float)Viewport[2]/(float)Viewport[3];
		Camera* Cam = view->mCamera;

		const lcMatrix44& ModelView = Cam->mWorldView;
		lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

		// Intersect the mouse with the 3 planes.
		lcVector3 PlaneNormals[3] =
		{
			lcVector3(1.0f, 0.0f, 0.0f),
			lcVector3(0.0f, 1.0f, 0.0f),
			lcVector3(0.0f, 0.0f, 1.0f),
		};

		// Find the rotation from the focused piece if relative snap is enabled.
		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Object* Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPiece())
			{
				const lcMatrix44& RotMat = ((Piece*)Focus)->mModelWorld;

				for (int i = 0; i < 3; i++)
					PlaneNormals[i] = lcMul30(PlaneNormals[i], RotMat);
			}
		}

		int Mode = (m_nCurAction == LC_ACTION_MOVE) ? LC_OVERLAY_MOVE_XYZ : LC_OVERLAY_NONE;
		lcVector3 Start = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.0f), ModelView, Projection, Viewport);
		lcVector3 End = lcUnprojectPoint(lcVector3((float)x, (float)y, 1.0f), ModelView, Projection, Viewport);
		float ClosestIntersectionDistance = FLT_MAX;

		for (int AxisIndex = 0; AxisIndex < 3; AxisIndex++)
		{
			lcVector4 Plane(PlaneNormals[AxisIndex], -lcDot(PlaneNormals[AxisIndex], m_OverlayCenter));
			lcVector3 Intersection;

			if (!lcLinePlaneIntersection(&Intersection, Start, End, Plane))
				continue;

			float IntersectionDistance = lcLengthSquared(Intersection - Start);

			if (IntersectionDistance > ClosestIntersectionDistance)
				continue;

			lcVector3 Dir(Intersection - m_OverlayCenter);

			float Proj1 = lcDot(Dir, PlaneNormals[(AxisIndex + 1) % 3]);
			float Proj2 = lcDot(Dir, PlaneNormals[(AxisIndex + 2) % 3]);

			if (Proj1 > 0.0f && Proj1 < OverlayMovePlaneSize && Proj2 > 0.0f && Proj2 < OverlayMovePlaneSize)
			{
				LC_OVERLAY_MODES PlaneModes[] = { LC_OVERLAY_MOVE_YZ, LC_OVERLAY_MOVE_XZ, LC_OVERLAY_MOVE_XY };

				Mode = PlaneModes[AxisIndex];

				ClosestIntersectionDistance = IntersectionDistance;
			}

			if (Proj1 > OverlayRotateArrowStart && Proj1 < OverlayRotateArrowEnd && Proj2 > OverlayRotateArrowStart && Proj2 < OverlayRotateArrowEnd)
			{
				LC_OVERLAY_MODES PlaneModes[] = { LC_OVERLAY_ROTATE_X, LC_OVERLAY_ROTATE_Y, LC_OVERLAY_ROTATE_Z };

				Mode = PlaneModes[AxisIndex];

				ClosestIntersectionDistance = IntersectionDistance;
			}

			if (fabs(Proj1) < OverlayMoveArrowCapRadius && Proj2 > 0.0f && Proj2 < OverlayMoveArrowSize)
			{
				LC_OVERLAY_MODES DirModes[] = { LC_OVERLAY_MOVE_Z, LC_OVERLAY_MOVE_X, LC_OVERLAY_MOVE_Y };

				Mode = DirModes[AxisIndex];

				ClosestIntersectionDistance = IntersectionDistance;
			}

			if (fabs(Proj2) < OverlayMoveArrowCapRadius && Proj1 > 0.0f && Proj1 < OverlayMoveArrowSize)
			{
				LC_OVERLAY_MODES DirModes[] = { LC_OVERLAY_MOVE_Y, LC_OVERLAY_MOVE_Z, LC_OVERLAY_MOVE_X };

				Mode = DirModes[AxisIndex];

				ClosestIntersectionDistance = IntersectionDistance;
			}
		}

		if (Mode != m_OverlayMode)
		{
			m_OverlayMode = Mode;
			UpdateAllViews();
		}
	}
	else if (m_nCurAction == LC_ACTION_ROTATE)
	{
		const float OverlayRotateRadius = 2.0f;

		// Calculate the distance from the mouse pointer to the center of the sphere.
		int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
		float Aspect = (float)Viewport[2]/(float)Viewport[3];
		Camera* Cam = view->mCamera;

		const lcMatrix44& ModelView = Cam->mWorldView;
		lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

		// Unproject the mouse point against both the front and the back clipping planes.
		lcVector3 SegStart = lcUnprojectPoint(lcVector3((float)x, (float)y, 0.0f), ModelView, Projection, Viewport);
		lcVector3 SegEnd = lcUnprojectPoint(lcVector3((float)x, (float)y, 1.0f), ModelView, Projection, Viewport);

		lcVector3 Center(m_OverlayCenter);

		lcVector3 Line = SegEnd - SegStart;
		lcVector3 Vec = Center - SegStart;

		float u = lcDot(Vec, Line) / Line.LengthSquared();

		// Closest point in the line to the mouse.
		lcVector3 Closest = SegStart + Line * u;

		int Mode = -1;
		float Distance = (Closest - Center).Length();
		const float Epsilon = 0.25f * OverlayScale;

		if (Distance > (OverlayRotateRadius * OverlayScale + Epsilon))
		{
			Mode = LC_OVERLAY_ROTATE_XYZ;
		}
		else if (Distance < (OverlayRotateRadius * OverlayScale + Epsilon))
		{
			// 3D rotation unless we're over one of the axis circles.
			Mode = LC_OVERLAY_ROTATE_XYZ;

			// Point P on a line defined by two points P1 and P2 is described by P = P1 + u (P2 - P1)
			// A sphere centered at P3 with radius r is described by (x - x3)^2 + (y - y3)^2 + (z - z3)^2 = r^2
			// Substituting the equation of the line into the sphere gives a quadratic equation where:
			// a = (x2 - x1)^2 + (y2 - y1)^2 + (z2 - z1)^2
			// b = 2[ (x2 - x1) (x1 - x3) + (y2 - y1) (y1 - y3) + (z2 - z1) (z1 - z3) ]
			// c = x32 + y32 + z32 + x12 + y12 + z12 - 2[x3 x1 + y3 y1 + z3 z1] - r2
			// The solutions to this quadratic are described by: (-b +- sqrt(b^2 - 4 a c) / 2 a
			// The exact behavior is determined by b^2 - 4 a c:
			// If this is less than 0 then the line does not intersect the sphere.
			// If it equals 0 then the line is a tangent to the sphere intersecting it at one point
			// If it is greater then 0 the line intersects the sphere at two points.

			float x1 = SegStart[0], y1 = SegStart[1], z1 = SegStart[2];
			float x2 = SegEnd[0], y2 = SegEnd[1], z2 = SegEnd[2];
			float x3 = m_OverlayCenter[0], y3 = m_OverlayCenter[1], z3 = m_OverlayCenter[2];
			float r = OverlayRotateRadius * OverlayScale;

			// TODO: rewrite using vectors.
			float a = (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) + (z2 - z1)*(z2 - z1);
			float b = 2 * ((x2 - x1)*(x1 - x3) + (y2 - y1)*(y1 - y3) + (z2 - z1)*(z1 - z3));
			float c = x3*x3 + y3*y3 + z3*z3 + x1*x1 + y1*y1 + z1*z1 - 2*(x3*x1 + y3*y1 + z3*z1) - r*r;
			float f = b * b - 4 * a * c;

			if (f >= 0.0f)
			{
				lcVector3 ViewDir(Cam->mTargetPosition - Cam->mPosition);

				float u1 = (-b + sqrtf(f)) / (2*a);
				float u2 = (-b - sqrtf(f)) / (2*a);

				lcVector3 Intersections[2] =
				{
					lcVector3(x1 + u1*(x2-x1), y1 + u1*(y2-y1), z1 + u1*(z2-z1)),
					lcVector3(x1 + u2*(x2-x1), y1 + u2*(y2-y1), z1 + u2*(z2-z1))
				};

				for (int i = 0; i < 2; i++)
				{
					lcVector3 Dist = Intersections[i] - Center;

					if (lcDot(ViewDir, Dist) > 0.0f)
						continue;

					// Find the rotation from the focused piece if relative snap is enabled.
					if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
					{
						Object* Focus = GetFocusObject();

						if ((Focus != NULL) && Focus->IsPiece())
						{
							const lcVector4& Rot = ((Piece*)Focus)->mRotation;

							lcMatrix44 RotMat = lcMatrix44FromAxisAngle(lcVector3(Rot[0], Rot[1], Rot[2]), -Rot[3] * LC_DTOR);

							Dist = lcMul30(Dist, RotMat);
						}
					}

					// Check if we're close enough to one of the axis.
					Dist.Normalize();

					float dx = fabsf(Dist[0]);
					float dy = fabsf(Dist[1]);
					float dz = fabsf(Dist[2]);

					if (dx < dy)
					{
						if (dx < dz)
						{
							if (dx < Epsilon)
								Mode = LC_OVERLAY_ROTATE_X;
						}
						else
						{
							if (dz < Epsilon)
								Mode = LC_OVERLAY_ROTATE_Z;
						}
					}
					else
					{
						if (dy < dz)
						{
							if (dy < Epsilon)
								Mode = LC_OVERLAY_ROTATE_Y;
						}
						else
						{
							if (dz < Epsilon)
								Mode = LC_OVERLAY_ROTATE_Z;
						}
					}

					if (Mode != LC_OVERLAY_ROTATE_XYZ)
					{
						switch (Mode)
						{
						case LC_OVERLAY_ROTATE_X:
							Dist[0] = 0.0f;
							break;
						case LC_OVERLAY_ROTATE_Y:
							Dist[1] = 0.0f;
							break;
						case LC_OVERLAY_ROTATE_Z:
							Dist[2] = 0.0f;
							break;
						}

						Dist *= r;

						m_OverlayTrackStart = Center + Dist;

						break;
					}
				}
			}
		}

		if (Mode != m_OverlayMode)
		{
			m_OverlayMode = Mode;
			UpdateAllViews();
		}
	}
	else if (m_nCurAction == LC_ACTION_ROTATE_VIEW)
	{
		int vx, vy, vw, vh;

		vx = 0;
		vy = 0;
		vw = view->mWidth;
		vh = view->mHeight;

		int cx = vx + vw / 2;
		int cy = vy + vh / 2;

		float d = sqrtf((float)((cx - x) * (cx - x) + (cy - y) * (cy - y)));
		float r = lcMin(vw, vh) * 0.35f;

		const float SquareSize = lcMax(8.0f, (vw+vh)/200);

		if ((d < r + SquareSize) && (d > r - SquareSize))
		{
			if ((cx - x < SquareSize) && (cx - x > -SquareSize))
				m_OverlayMode = LC_OVERLAY_ROTATE_VIEW_Y;

			if ((cy - y < SquareSize) && (cy - y > -SquareSize))
				m_OverlayMode = LC_OVERLAY_ROTATE_VIEW_X;
		}
		else
		{
			if (d < r)
				m_OverlayMode = LC_OVERLAY_ROTATE_VIEW_XYZ;
			else
				m_OverlayMode = LC_OVERLAY_ROTATE_VIEW_Z;
		}
	}

	view->SetCursor(view->GetCursor());
}

void Project::ActivateOverlay(View* view, int Action, int OverlayMode)
{
	if ((Action == LC_ACTION_SELECT) || (Action == LC_ACTION_MOVE) || (Action == LC_ACTION_ROTATE))
	{
		if (GetFocusPosition(m_OverlayCenter))
			m_OverlayActive = true;
		else if (GetSelectionCenter(m_OverlayCenter))
			m_OverlayActive = true;
		else
			m_OverlayActive = false;
	}
	else if ((Action == LC_ACTION_ZOOM_REGION) && (m_nTracking == LC_TRACK_START_LEFT))
		m_OverlayActive = true;
	else if (Action == LC_ACTION_ZOOM || Action == LC_ACTION_PAN || Action == LC_ACTION_ROTATE_VIEW)
		m_OverlayActive = true;
	else
		m_OverlayActive = false;

	if (m_OverlayActive)
	{
		m_OverlayMode = OverlayMode;
		UpdateOverlayScale();
	}

	if (view)
		view->SetCursor(view->GetCursor());
}

void Project::UpdateOverlayScale()
{
	// TODO: This is not needed, draw the overlays using an ortho matrix.
	if (m_OverlayActive)
	{
		// Calculate the scaling factor by projecting the center to the front plane then
		// projecting a point close to it back.
		int Viewport[4] = { 0, 0, m_ActiveView->mWidth, m_ActiveView->mHeight };
		float Aspect = (float)Viewport[2]/(float)Viewport[3];
		Camera* Cam = m_ActiveView->mCamera;

		const lcMatrix44& ModelView = Cam->mWorldView;
		lcMatrix44 Projection = lcMatrix44Perspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

		lcVector3 ScreenPos = lcProjectPoint(m_OverlayCenter, ModelView, Projection, Viewport);
		ScreenPos[0] += 10.0f;
		lcVector3 Point = lcUnprojectPoint(ScreenPos, ModelView, Projection, Viewport);

		lcVector3 Dist(Point - m_OverlayCenter);
		m_ActiveView->m_OverlayScale = Dist.Length() * 5.0f;
	}
}
