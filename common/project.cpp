// Everything that is a part of a LeoCAD project goes here.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <locale.h>
#include "opengl.h"
#include "vector.h"
#include "matrix.h"
#include "pieceinf.h"
#include "texture.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "terrain.h"
#include "project.h"
#include "image.h"
#include "system.h"
#include "globals.h"
#include "minifig.h"
#include "config.h"
#include "message.h"
#include "curve.h"
#include "mainwnd.h"
#include "view.h"
#include "library.h"
#include "texfont.h"
#include "algebra.h"
#include "debug.h"
#include "lc_application.h"

// FIXME: temporary function, replace the code !!!
void SystemUpdateFocus (void* p)
{
  messenger->Dispatch (LC_MSG_FOCUS_CHANGED, p);
}

static void ProjectListener(int Message, void* Data, void* User)
{
	((Project*)User)->HandleMessage(Message, Data);
}

typedef struct
{
  unsigned char n;
  float dim [4][4];
} LC_VIEWPORT;

static LC_VIEWPORT viewports[14] = {
  { 1,  {{ 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f },
	 { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } }}, // 1
  { 2,  {{ 0.0f, 0.0f, 0.5f, 1.0f }, { 0.5f, 0.0f, 0.5f, 1.0f },
	 { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } }}, // 2V
  { 2,  {{ 0.0f, 0.0f, 1.0f, 0.5f }, { 0.0f, 0.5f, 1.0f, 0.5f },
	 { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } }}, // 2H
  { 2,  {{ 0.0f, 0.0f, 1.0f, 0.7f }, { 0.0f, 0.7f, 1.0f, 0.3f },
	 { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } }}, // 2HT
  { 2,  {{ 0.0f, 0.0f, 1.0f, 0.3f }, { 0.0f, 0.3f, 1.0f, 0.7f },
	 { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } }}, // 2HB
  { 3,  {{ 0.0f, 0.0f, 0.5f, 0.5f }, { 0.0f, 0.5f, 0.5f, 0.5f },
	 { 0.5f, 0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } }}, // 3VL
  { 3,  {{ 0.0f, 0.0f, 0.5f, 1.0f }, { 0.5f, 0.0f, 0.5f, 0.5f },
	 { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f, 0.0f } }}, // 3VR
  { 3,  {{ 0.0f, 0.0f, 1.0f, 0.5f }, { 0.0f, 0.5f, 0.5f, 0.5f },
	 { 0.5f, 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 0.0f, 0.0f } }}, // 3HB
  { 3,  {{ 0.0f, 0.0f, 0.5f, 0.5f }, { 0.5f, 0.0f, 0.5f, 0.5f },
	 { 0.0f, 0.5f, 1.0f, 0.5f }, { 0.0f, 0.0f, 0.0f, 0.0f } }}, // 3HT
  { 4,  {{ 0.0f, 0.0f, 0.3f, 0.3f }, { 0.0f, 0.3f, 0.3f, 0.4f },
	 { 0.0f, 0.7f, 0.3f, 0.3f }, { 0.3f, 0.0f, 0.7f, 1.0f } }}, // 4VL
  { 4,  {{ 0.0f, 0.0f, 0.7f, 1.0f }, { 0.7f, 0.0f, 0.3f, 0.3f },
	 { 0.7f, 0.3f, 0.3f, 0.4f }, { 0.7f, 0.7f, 0.3f, 0.3f } }}, // 4VR
  { 4,  {{ 0.0f, 0.0f, 1.0f, 0.7f }, { 0.0f, 0.7f, 0.3f, 0.3f },
	 { 0.3f, 0.7f, 0.4f, 0.3f }, { 0.7f, 0.7f, 0.3f, 0.3f } }}, // 4HT
  { 4,  {{ 0.0f, 0.0f, 0.3f, 0.3f }, { 0.3f, 0.0f, 0.4f, 0.3f },
	 { 0.7f, 0.0f, 0.3f, 0.3f }, { 0.0f, 0.3f, 1.0f, 0.7f } }}, // 4HB
  { 4,  {{ 0.0f, 0.0f, 0.5f, 0.5f }, { 0.5f, 0.0f, 0.5f, 0.5f },
	 { 0.0f, 0.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f, 0.5f } }}};// 4

/////////////////////////////////////////////////////////////////////////////
// Project construction/destruction

Project::Project()
{
	int i;

	m_ActiveView = NULL;
	m_bModified = false;
	m_bTrackCancel = false;
	m_nTracking = LC_TRACK_NONE;
	m_pPieces = NULL;
	m_pCameras = NULL;
	m_pLights = NULL;
	m_pGroups = NULL;
	m_pUndoList = NULL;
	m_pRedoList = NULL;
	m_nGridList = 0;
	m_pTrackFile = NULL;
	m_nCurClipboard = 0;
	m_nCurAction = 0;
	m_pTerrain = new Terrain();
	m_pBackground = new Texture();
	m_nAutosave = Sys_ProfileLoadInt ("Settings", "Autosave", 10);
	m_nMouse = Sys_ProfileLoadInt ("Default", "Mouse", 11);
	strcpy(m_strModelsPath, Sys_ProfileLoadString ("Default", "Projects", ""));

	if (messenger == NULL)
		messenger = new Messenger ();
	messenger->AddRef();
	messenger->Listen(&ProjectListener, this);

  for (i = 0; i < LC_CONNECTIONS; i++)
	{
		m_pConnections[i].entries = NULL;
		m_pConnections[i].numentries = 0;
	}

	for (i = 0; i < 10; i++)
		m_pClipboard[i] = NULL;

	m_pScreenFont = new TexFont();
}

Project::~Project()
{
	DeleteContents(false);
	SystemFinish();

  if (m_pTrackFile)
  {
    delete m_pTrackFile;
    m_pTrackFile = NULL;
  }

	for (int i = 0; i < 10; i++)
		if (m_pClipboard[i] != NULL)
			delete m_pClipboard[i];

  messenger->DecRef ();

	delete m_pTerrain;
	delete m_pBackground;
	delete m_pScreenFont;
}


/////////////////////////////////////////////////////////////////////////////
// Project attributes, general services

void Project::UpdateInterface()
{
	// Update all user interface elements.
	SystemUpdateUndoRedo(m_pUndoList->pNext ? m_pUndoList->strText : NULL, m_pRedoList ? m_pRedoList->strText : NULL);
	SystemUpdatePaste(m_pClipboard[m_nCurClipboard] != NULL);
	SystemUpdatePlay(true, false);
	SystemUpdateCategories(false);
	SetTitle(m_strTitle);

	SystemUpdateFocus(NULL);
	SetAction(m_nCurAction);
//	SystemUpdateViewport(m_nViewportMode, 0);
	SystemUpdateColorList(m_nCurColor);
	SystemUpdateAnimation(m_bAnimation, m_bAddKeys);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_FAST) != 0);
	SystemUpdateSnap(m_nSnap);
	SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
	SystemUpdateCameraMenu(m_pCameras);
	SystemUpdateCurrentCamera(NULL, m_ActiveView ? m_ActiveView->m_Camera : NULL, m_pCameras);
	UpdateSelection();
	if (m_bAnimation)
		SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
	else
		SystemUpdateTime(m_bAnimation, m_nCurStep, 255);

	for (int i = 0; i < m_ViewList.GetSize(); i++)
	{
		m_ViewList[i]->MakeCurrent();
		RenderInitialize();
	}

	UpdateSelection();
}

void Project::SetTitle(const char* lpszTitle)
{
	if (lpszTitle != m_strTitle)
		strcpy(m_strTitle, lpszTitle);

	char title[LC_MAXPATH], *ptr, ext[4];
	strcpy(title, m_strTitle);

	ptr = strrchr(title, '.');
	if (ptr != NULL)
	{
		strncpy(ext, ptr+1, 3);
		ext[3] = 0;
		strlwr(ext);

		if (strcmp(ext, "lcd") == 0)
			*ptr = 0;
		if (strcmp(ext, "dat") == 0)
			*ptr = 0;
		if (strcmp(ext, "ldr") == 0)
			*ptr = 0;
		if (strcmp(ext, "mpd") == 0)
			*ptr = 0;
	}

	strcat(title, " - LeoCAD");

	SystemSetWindowCaption(title);
}

void Project::DeleteContents(bool bUndo)
{
	Piece* pPiece;
	Camera* pCamera;
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

	for (int i = 0; i < LC_CONNECTIONS; i++)
	{
		for (int j = 0; j < m_pConnections[i].numentries; j++)
			delete (m_pConnections[i].entries[j].cons);

		delete m_pConnections[i].entries;
		m_pConnections[i].entries = NULL;
		m_pConnections[i].numentries = 0;
	}

	while (m_pCameras)
	{
		pCamera = m_pCameras;
		m_pCameras = m_pCameras->m_pNext;
		delete pCamera;
	}

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
//	SystemUpdateViewport(0, m_nViewportMode);
//	m_nViewportMode = 0;
	SetAction(0);
	m_nCurColor = 0;
	SystemUpdateColorList(m_nCurColor);
	m_bAnimation = false;
	m_bAddKeys = false;
	SystemUpdateAnimation(m_bAnimation, m_bAddKeys);
	m_bUndoOriginal = true;
	SystemUpdateUndoRedo(NULL, NULL);
	m_nDetail = Sys_ProfileLoadInt ("Default", "Detail", LC_DET_BRICKEDGES);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_FAST) != 0);
	m_nAngleSnap = (unsigned short)Sys_ProfileLoadInt ("Default", "Angle", 30);
	m_nSnap = Sys_ProfileLoadInt ("Default", "Snap", LC_DRAW_SNAP_A | LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y | LC_DRAW_SNAP_Z | LC_DRAW_MOVE);
	SystemUpdateSnap(m_nSnap);
	m_nMoveSnap = 0x0304;
	SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
	m_fLineWidth = (float)Sys_ProfileLoadInt ("Default", "Line", 100)/100;
	m_fFogDensity = (float)Sys_ProfileLoadInt ("Default", "Density", 10)/100;
	rgb = Sys_ProfileLoadInt ("Default", "Fog", 0xFFFFFF);
	m_fFogColor[0] = (float)((unsigned char) (rgb))/255;
	m_fFogColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fFogColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_fFogColor[3] = 1.0f;
	m_nGridSize = (unsigned short)Sys_ProfileLoadInt ("Default", "Grid", 20);
	rgb = Sys_ProfileLoadInt ("Default", "Ambient", 0x4B4B4B);
	m_fAmbient[0] = (float)((unsigned char) (rgb))/255;
	m_fAmbient[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fAmbient[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_fAmbient[3] = 1.0f;
	rgb = Sys_ProfileLoadInt ("Default", "Background", 0xFFFFFF);
	m_fBackground[0] = (float)((unsigned char) (rgb))/255;
	m_fBackground[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fBackground[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_fBackground[3] = 1.0f;
	rgb = Sys_ProfileLoadInt ("Default", "Gradient1", 0xBF0000);
	m_fGradient1[0] = (float)((unsigned char) (rgb))/255;
	m_fGradient1[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fGradient1[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	rgb = Sys_ProfileLoadInt ("Default", "Gradient2", 0xFFFFFF);
	m_fGradient2[0] = (float)((unsigned char) (rgb))/255;
	m_fGradient2[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fGradient2[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_nFPS = Sys_ProfileLoadInt ("Default", "FPS", 24);
	m_nCurStep = 1;
	m_nCurFrame = 1;
	m_nTotalFrames = 100;
	SystemUpdateTime(false, 1, 255);
	m_nScene = Sys_ProfileLoadInt ("Default", "Scene", 0);
	m_nSaveTimer = 0;
	strcpy(m_strHeader, Sys_ProfileLoadString ("Default", "Header", ""));
	strcpy(m_strFooter, Sys_ProfileLoadString ("Default", "Footer", "Page &P"));
	strcpy(m_strBackground, Sys_ProfileLoadString ("Default", "BMP", ""));
	m_pTerrain->LoadDefaults(true);
	m_OverlayActive = false;

	for (i = 0; i < m_ViewList.GetSize (); i++)
	{
		m_ViewList[i]->MakeCurrent ();
		RenderInitialize();
	}

	if (cameras)
	{
		Camera* pCam;
		for (pCam = NULL, i = 0; i < 7; i++)
		{
			pCam = new Camera(i, pCam);
			if (m_pCameras == NULL)
				m_pCameras = pCam;
		}
		for (i = 0; i < m_ViewList.GetSize (); i++)
			m_ViewList[i]->m_Camera = pCam;
		SystemUpdateCameraMenu(m_pCameras);
		if (m_ActiveView)
			SystemUpdateCurrentCamera(NULL, m_ActiveView->m_Camera, m_pCameras);
	}
	SystemPieceComboAdd(NULL);
	UpdateSelection();
}

/////////////////////////////////////////////////////////////////////////////
// Standard file menu commands

// Read a .lcd file
bool Project::FileLoad(File* file, bool bUndo, bool bMerge)
{
	int i, count;
	char id[32];
	unsigned long rgb;
	float fv = 0.4f;
	unsigned char ch, action = m_nCurAction;
	unsigned short sh;

	file->Seek(0, SEEK_SET);
	file->Read(id, 32);
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
		file->ReadFloat (&fv, 1);

	file->ReadLong (&rgb, 1);
	if (!bMerge)
	{
		m_fBackground[0] = (float)((unsigned char) (rgb))/255;
		m_fBackground[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
		m_fBackground[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	}

	if (fv < 0.6f) // old view
	{
		Camera* pCam;
		for (pCam = NULL, i = 0; i < 7; i++)
		{
			pCam = new Camera(i, pCam);
			if (m_pCameras == NULL)
				m_pCameras = pCam;

//			switch (i) 
//			{
//			case LC_CAMERA_MAIN:  m_pViewCameras[0] = pCam; break;
//			case LC_CAMERA_FRONT: m_pViewCameras[1] = pCam; break;
//			case LC_CAMERA_TOP:   m_pViewCameras[2] = pCam; break;
//			case LC_CAMERA_RIGHT: m_pViewCameras[3] = pCam; break;
//			}
		}

		double eye[3], target[3];
		file->ReadDouble (&eye, 3);
		file->ReadDouble (&target, 3);
		float tmp[3] = { (float)eye[0], (float)eye[1], (float)eye[2] };
		pCam->ChangeKey(1, false, false, tmp, LC_CK_EYE);
		pCam->ChangeKey(1, true, false, tmp, LC_CK_EYE);
		tmp[0] = (float)target[0]; tmp[1] = (float)target[1]; tmp[2] = (float)target[2];
		pCam->ChangeKey(1, false, false, tmp, LC_CK_TARGET);
		pCam->ChangeKey(1, true, false, tmp, LC_CK_TARGET);

		// Create up vector
		Vector upvec(0,0,1), frontvec((float)(eye[0]-target[0]), (float)(eye[1]-target[1]), (float)(eye[2]-target[2])), sidevec;
		frontvec.Normalize();
		if (frontvec == upvec)
			sidevec = Vector(1,0,0);
		else
			sidevec.Cross(frontvec, upvec);
		upvec.Cross(sidevec, frontvec);
		upvec.Normalize();
		upvec.ToFloat(tmp);
		pCam->ChangeKey(1, false, false, tmp, LC_CK_UP);
		pCam->ChangeKey(1, true, false, tmp, LC_CK_UP);
	}

	if (bMerge)
		file->Seek(32, SEEK_CUR);
	else
	{
		file->ReadLong (&i, 1); m_nAngleSnap = i;
		file->ReadLong (&m_nSnap, 1);
		file->ReadFloat (&m_fLineWidth, 1);
		file->ReadLong (&m_nDetail, 1);
		file->ReadLong (&i, 1); //m_nCurGroup = i;
		file->ReadLong (&i, 1); m_nCurColor = i;
		file->ReadLong (&i, 1); action = i;
		file->ReadLong (&i, 1); m_nCurStep = i;
	}

	if (fv > 0.8f)
		file->ReadLong (&m_nScene, 1);

	file->ReadLong (&count, 1);
	SystemStartProgressBar(0, count, 1, "Loading project...");

	while (count--)
	{	
		if (fv > 0.4f)
		{
			char name[LC_PIECE_NAME_LEN];
			Piece* pPiece = new Piece(NULL);
			pPiece->FileLoad(*file, name);
			PieceInfo* pInfo = lcGetPiecesLibrary()->FindPieceInfo(name);
			if (pInfo)
			{
				pPiece->SetPieceInfo(pInfo);

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
					SystemPieceComboAdd(pInfo->m_strDescription);
			}
			else 
				delete pPiece;
		}
		else
		{
			char name[LC_PIECE_NAME_LEN];
			float pos[3], rot[3], param[4];
			unsigned char color, step, group;
		
			file->ReadFloat (pos, 3);
			file->ReadFloat (rot, 3);
			file->ReadByte (&color, 1);
			file->Read(name, sizeof(name));
			file->ReadByte (&step, 1);
			file->ReadByte (&group, 1);

			const unsigned char conv[20] = { 0,2,4,9,7,6,22,8,10,11,14,16,18,9,21,20,22,8,10,11 };
			color = conv[color];

			PieceInfo* pInfo = lcGetPiecesLibrary()->FindPieceInfo(name);
			if (pInfo != NULL)
			{
				Piece* pPiece = new Piece(pInfo);
				Matrix mat;

				pPiece->Initialize(pos[0], pos[1], pos[2], step, 1, color);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				mat.CreateOld(0,0,0, rot[0],rot[1],rot[2]);
				mat.ToAxisAngle(param);
				pPiece->ChangeKey(1, false, false, param, LC_PK_ROTATION);
				pPiece->ChangeKey(1, true, false, param, LC_PK_ROTATION);
//				pPiece->SetGroup((Group*)group);
				SystemPieceComboAdd(pInfo->m_strDescription);
			}
		}
		SytemStepProgressBar();
	}
	SytemEndProgressBar();

	if (!bMerge)
	{
		if (fv >= 0.4f)
		{
			file->Read(&ch, 1);
			if (ch == 0xFF) file->ReadShort (&sh, 1); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
				file->Read(m_strAuthor, sh);

			file->Read(&ch, 1);
			if (ch == 0xFF) file->ReadShort (&sh, 1); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
				file->Read(m_strDescription, sh);

			file->Read(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->ReadShort (&sh, 1); else sh = ch;
			if (sh > 255)
				file->Seek(sh, SEEK_CUR);
			else
				file->Read(m_strComments, sh);
		}
	}
	else
	{
		if (fv >= 0.4f)
		{
			file->Read (&ch, 1);
			if (ch == 0xFF) file->ReadShort (&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);

			file->Read (&ch, 1);
			if (ch == 0xFF) file->ReadShort (&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);

			file->Read (&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->ReadShort (&sh, 1); else sh = ch;
			file->Seek (sh, SEEK_CUR);
		}
	}

	if (fv >= 0.5f)
	{
		file->ReadLong (&count, 1);

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
				file->Read(pGroup->m_strName, 65);
				file->Read(&ch, 1);
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
				int ViewportMode;
				file->ReadLong(&ViewportMode, 1);
			}
			else
			{
				lcuint8 ViewportMode, ActiveViewport;
				file->ReadByte(&ViewportMode, 1);
				file->ReadByte(&ActiveViewport, 1);
			}

			file->ReadLong (&count, 1);
			Camera* pCam = NULL;
			for (i = 0; i < count; i++)
			{
				pCam = new Camera(i, pCam);
				if (m_pCameras == NULL)
					m_pCameras = pCam;

//				if (i < 4 && fv == 0.6f)
//					m_pViewCameras[i] = pCam;
			}

			if (count < 7)
			{
				pCam = new Camera(0, NULL);
				for (i = 0; i < count; i++)
					pCam->FileLoad(*file);
				delete pCam;
			}
			else
				for (pCam = m_pCameras; pCam; pCam = pCam->m_pNext)
					pCam->FileLoad(*file);
		}

		if (fv >= 0.7f)
		{
			for (count = 0; count < 4; count++)
			{
				file->ReadLong (&i, 1);

				Camera* pCam = m_pCameras;
				while (i--)
					pCam = pCam->m_pNext;
//				m_pViewCameras[count] = pCam;
			}

			file->ReadLong (&rgb, 1);
			m_fFogColor[0] = (float)((unsigned char) (rgb))/255;
			m_fFogColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fFogColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv < 1.0f)
			{
				file->ReadLong (&rgb, 1);
				m_fFogDensity = (float)rgb/100;
			}
			else
				file->ReadFloat (&m_fFogDensity, 1);

			if (fv < 1.3f)
			{
				file->ReadByte (&ch, 1);
				if (ch == 0xFF)
					file->ReadShort (&sh, 1);
				sh = ch;
			}
			else
				file->ReadShort (&sh, 1);

			if (sh < LC_MAXPATH)
				file->Read (m_strBackground, sh);
			else
				file->Seek (sh, SEEK_CUR);
		}

		if (fv >= 0.8f)
		{
			file->Read(&ch, 1);
			file->Read(m_strHeader, ch);
			file->Read(&ch, 1);
			file->Read(m_strFooter, ch);
		}

		if (fv > 0.9f)
		{
			file->ReadLong (&rgb, 1);
			m_fAmbient[0] = (float)((unsigned char) (rgb))/255;
			m_fAmbient[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fAmbient[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv < 1.3f)
			{
				file->ReadLong (&i, 1); m_bAnimation = (i != 0);
				file->ReadLong (&i, 1); m_bAddKeys = (i != 0);
				file->ReadByte (&m_nFPS, 1);
				file->ReadLong (&i, 1); m_nCurFrame = i;
				file->ReadShort (&m_nTotalFrames, 1);
				file->ReadLong (&i, 1); m_nGridSize = i;
				file->ReadLong (&i, 1); //m_nMoveSnap = i;
			}
			else
			{
				file->ReadByte (&ch, 1); m_bAnimation = (ch != 0);
				file->ReadByte (&ch, 1); m_bAddKeys = (ch != 0);
				file->ReadByte (&m_nFPS, 1);
				file->ReadShort (&m_nCurFrame, 1);
				file->ReadShort (&m_nTotalFrames, 1);
				file->ReadShort (&m_nGridSize, 1);
				file->ReadShort (&sh, 1);
				if (fv >= 1.4f)
					m_nMoveSnap = sh;
			}
		}
			
		if (fv > 1.0f)
		{
			file->ReadLong (&rgb, 1);
			m_fGradient1[0] = (float)((unsigned char) (rgb))/255;
			m_fGradient1[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fGradient1[2] = (float)((unsigned char) ((rgb) >> 16))/255;
			file->ReadLong (&rgb, 1);
			m_fGradient2[0] = (float)((unsigned char) (rgb))/255;
			m_fGradient2[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fGradient2[2] = (float)((unsigned char) ((rgb) >> 16))/255;
			
			if (fv > 1.1f)
				m_pTerrain->FileLoad (file);
			else
			{
				file->Seek (4, SEEK_CUR);
				file->Read (&ch, 1);
				file->Seek (ch, SEEK_CUR);
			}
		}
	}

	if (!bMerge)
	{
		Camera* Cam = GetCamera(LC_CAMERA_MAIN);
		for (i = 0; i < m_ViewList.GetSize (); i++)
			m_ViewList[i]->m_Camera = Cam;
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
		SystemUpdateFocus(NULL);
	SetAction(action);
//	SystemUpdateViewport(m_nViewportMode, 0);
	SystemUpdateColorList(m_nCurColor);
	SystemUpdateAnimation(m_bAnimation, m_bAddKeys);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_FAST) != 0);
	SystemUpdateSnap(m_nSnap);
	SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
	SystemUpdateCameraMenu(m_pCameras);
	SystemUpdateCurrentCamera(NULL, m_ActiveView->m_Camera, m_pCameras);
	UpdateSelection();
	if (m_bAnimation)
		SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
	else
		SystemUpdateTime(m_bAnimation, m_nCurStep, 255);
	UpdateAllViews ();

	return true;
}

void Project::FileSave(File* file, bool bUndo)
{
	float ver_flt = 1.4f; // LeoCAD 0.75 - (and this should have been an integer).
	unsigned long rgb;
	unsigned char ch;
	unsigned short sh;
	int i, j;

	file->Seek (0, SEEK_SET);
	file->Write (LC_STR_VERSION, 32);
	file->WriteFloat (&ver_flt, 1);

	rgb = FLOATRGB(m_fBackground);
	file->WriteLong (&rgb, 1);

	i = m_nAngleSnap; file->WriteLong (&i, 1);
	file->WriteLong (&m_nSnap, 1);
	file->WriteFloat (&m_fLineWidth, 1);
	file->WriteLong (&m_nDetail, 1);
	//i = m_nCurGroup;
	file->WriteLong (&i, 1);
	i = m_nCurColor; file->WriteLong (&i, 1);
	i = m_nCurAction; file->WriteLong (&i, 1);
	i = m_nCurStep; file->WriteLong (&i, 1);
	file->WriteLong (&m_nScene, 1);

	Piece* pPiece;
	for (i = 0, pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		i++;
	file->WriteLong (&i, 1);

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->FileSave (*file, m_pGroups);

	ch = strlen(m_strAuthor);
	file->Write(&ch, 1);
	file->Write(m_strAuthor, ch);
	ch = strlen(m_strDescription);
	file->Write(&ch, 1);
	file->Write(m_strDescription, ch);
	ch = strlen(m_strComments);
	file->Write(&ch, 1);
	file->Write(m_strComments, ch);

	Group* pGroup;
	for (i = 0, pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
		i++;
	file->WriteLong (&i, 1);

	for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
		pGroup->FileSave(file, m_pGroups);

	lcuint8 ViewportMode = 0, ActiveViewport = 0;
	file->WriteByte(&ViewportMode, 1);
	file->WriteByte(&ActiveViewport, 1);

	Camera* pCamera;
	for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		i++;
	file->WriteLong (&i, 1);

	for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		pCamera->FileSave(*file);

	for (j = 0; j < 4; j++)
	{
		i = 0;
//		for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
//			if (pCamera == m_pViewCameras[j])
//				break;
//			else
//				i++;

		file->WriteLong (&i, 1);
	}

	rgb = FLOATRGB(m_fFogColor);
	file->WriteLong (&rgb, 1);
	file->WriteFloat (&m_fFogDensity, 1);
	sh = strlen(m_strBackground);
	file->WriteShort (&sh, 1);
	file->Write(m_strBackground, sh);
	ch = strlen(m_strHeader);
	file->Write(&ch, 1);
	file->Write(m_strHeader, ch);
	ch = strlen(m_strFooter);
	file->Write(&ch, 1);
	file->Write(m_strFooter, ch);
	// 0.60 (1.0)
	rgb = FLOATRGB(m_fAmbient);
	file->WriteLong (&rgb, 1);
	ch = m_bAnimation;
	file->Write(&ch, 1);
	ch = m_bAddKeys;
	file->WriteByte (&ch, 1);
	file->WriteByte (&m_nFPS, 1);
	file->WriteShort (&m_nCurFrame, 1);
	file->WriteShort (&m_nTotalFrames, 1);
	file->WriteShort (&m_nGridSize, 1);
	file->WriteShort (&m_nMoveSnap, 1);
	// 0.62 (1.1)
	rgb = FLOATRGB(m_fGradient1);
	file->WriteLong (&rgb, 1);
	rgb = FLOATRGB(m_fGradient2);
	file->WriteLong (&rgb, 1);
	// 0.64 (1.2)
	m_pTerrain->FileSave(file);

	if (!bUndo)
	{
		unsigned long pos = 0;

		i = Sys_ProfileLoadInt ("Default", "Save Preview", 0);
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

		file->WriteLong (&pos, 1);
		m_nSaveTimer = 0;
	}
}

void Project::FileReadMPD(File& MPD, PtrArray<File>& FileArray) const
{
  FileMem* CurFile = NULL;
	char Buf[1024];

	while (MPD.ReadLine(Buf, 1024))
  {
    String Line(Buf);

    Line.TrimLeft();

    if (Line[0] != '0')
    {
      // Copy current line.
      if (CurFile != NULL)
        CurFile->Write(Buf, strlen(Buf));

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
      CurFile = new FileMem();
      CurFile->SetFileName(Line);
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
      CurFile->Write(Buf, strlen(Buf));
    }
  }
}

void Project::FileReadLDraw(File* file, Matrix* prevmat, int* nOk, int DefColor, int* nStep, PtrArray<File>& FileArray)
{
	char buf[1024];

	// Save file offset.
	lcuint32 Offset = file->GetPosition();
	file->Seek(0, SEEK_SET);

	while (file->ReadLine(buf, 1024))
	{
		strupr(buf);
		if (strstr(buf, "STEP"))
		{
			(*nStep)++;
			continue;
		}

		bool read = true;
		char *ptr, tmp[LC_MAXPATH], pn[LC_MAXPATH];
		int color, cmd;
		float fmat[12];

		if (sscanf(buf, "%d %d %g %g %g %g %g %g %g %g %g %g %g %g %s[12]",
			&cmd, &color, &fmat[0], &fmat[1], &fmat[2], &fmat[3], &fmat[4], &fmat[5], &fmat[6], 
			&fmat[7], &fmat[8], &fmat[9], &fmat[10], &fmat[11], &tmp[0]) != 15)
			continue;

		Matrix incmat, tmpmat;
		incmat.FromLDraw(fmat);
		tmpmat.Multiply(*prevmat, incmat);

		if (cmd == 1)
		{
			int cl = 0;
			if (color == 16) 
				cl = DefColor;
			else
				cl = ConvertColor(color);

			strcpy(pn, tmp);
			ptr = strrchr(tmp, '.');

			if (ptr != NULL)
				*ptr = 0;

			// See if it's a piece in the library
			if (strlen(tmp) < LC_PIECE_NAME_LEN)
			{
				char name[LC_PIECE_NAME_LEN];
				strcpy(name, tmp);

				PieceInfo* pInfo = lcGetPiecesLibrary()->FindPieceInfo(name);
				if (pInfo != NULL)
				{
					float x, y, z, rot[4];
					Piece* pPiece = new Piece(pInfo);
					read = false;

					tmpmat.GetTranslation(&x, &y, &z);
					pPiece->Initialize(x, y, z, *nStep, 1, cl);
					pPiece->CreateName(m_pPieces);
					AddPiece(pPiece);
					tmpmat.ToAxisAngle(rot);
					pPiece->ChangeKey(1, false, false, rot, LC_PK_ROTATION);
					pPiece->ChangeKey(1, true, false, rot, LC_PK_ROTATION);
					SystemPieceComboAdd(pInfo->m_strDescription);
					(*nOk)++;
				}
			}

			// Check for MPD files first.
			if (read)
			{
				for (int i = 0; i < FileArray.GetSize(); i++)
				{
					if (stricmp(FileArray[i]->GetFileName(), pn) == 0)
					{
						FileReadLDraw(FileArray[i], &tmpmat, nOk, cl, nStep, FileArray);
						read = false;
						break;
					}
				}
			}

			// Try to read the file from disk.
			if (read)
			{
				FileDisk tf;

				if (tf.Open(pn, "rt"))
				{
					FileReadLDraw(&tf, &tmpmat, nOk, cl, nStep, FileArray);
					read = false;
				}
			}

			if (read)
			{
				// Create a placeholder.
				PieceInfo* Info = lcGetPiecesLibrary()->CreatePiecePlaceholder(tmp);

				float x, y, z, rot[4];
				Piece* pPiece = new Piece(Info);
				read = false;

				tmpmat.GetTranslation(&x, &y, &z);
				pPiece->Initialize(x, y, z, *nStep, 1, cl);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				tmpmat.ToAxisAngle(rot);
				pPiece->ChangeKey(1, false, false, rot, LC_PK_ROTATION);
				pPiece->ChangeKey(1, true, false, rot, LC_PK_ROTATION);
				SystemPieceComboAdd(Info->m_strDescription);
				(*nOk)++;
			}
		}
	}

	// Restore file offset.
	file->Seek(Offset, SEEK_SET);
}

bool Project::DoFileSave()
{
/*
	DWORD dwAttrib = GetFileAttributes(m_strPathName);
	if (dwAttrib & FILE_ATTRIBUTE_READONLY)
	{
		// we do not have read-write access or the file does not (now) exist
		if (!DoSave(NULL, true))
			return false;
	}
	else
*/	{
		if (!DoSave(m_strPathName, true))
			return false;
	}
	return true;
}

// Save the document data to a file
// lpszPathName = path name where to save document file
// if lpszPathName is NULL then the user will be prompted (SaveAs)
// note: lpszPathName can be different than 'm_strPathName'
// if 'bReplace' is TRUE will change file name if successful (SaveAs)
// if 'bReplace' is FALSE will not change path name (SaveCopyAs)
bool Project::DoSave(char* lpszPathName, bool bReplace)
{
	FileDisk file;
	char newName[LC_MAXPATH];
	memset(newName, 0, sizeof(newName));
	if (lpszPathName)
		strcpy(newName, lpszPathName);

	char ext[4], *ptr;
	memset(ext, 0, 4);
	ptr = strrchr(newName, '.');
	if (ptr != NULL)
	{
		ptr++;
		strncpy(ext, ptr, 3);
		strlwr(ext);

		if ((strcmp(ext, "dat") == 0) || (strcmp(ext, "ldr") == 0))
		{
			*ptr = 0;
			strcat(newName, "lcd");
		}
	}

	if (strlen(newName) == 0)
	{
		strcpy(newName, m_strPathName);
		if (bReplace && strlen(newName) == 0)
		{
			strcpy(newName, m_strTitle);

			// check for dubious filename
			int iBad = strcspn(newName, " #%;/\\");
			if (iBad != -1)
				newName[iBad] = 0;

			strcat(newName, ".lcd");
		}

		if (!SystemDoDialog(LC_DLG_FILE_SAVE_PROJECT, &newName))
			return false; // don't even attempt to save
	}

//	CWaitCursor wait;

	if (!file.Open(newName, "wb"))
	{
//		MessageBox("Failed to save.");

		// be sure to delete the file
		if (lpszPathName == NULL)
			remove(newName);

		return false;
	}

	memset(ext, 0, 4);
	ptr = strrchr(newName, '.');
	if (ptr != NULL)
	{
		strncpy(ext, ptr+1, 3);
		strlwr(ext);
	}

	if ((strcmp(ext, "dat") == 0) || (strcmp(ext, "ldr") == 0))
	{
		const int col[28] = { 4,12,2,10,1,9,14,15,8,0,6,13,13,334,36,44,34,42,33,41,46,47,7,382,6,13,11,383 };
		Piece* pPiece;
		int i, steps = GetLastStep();
		char buf[256], *ptr;

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
		file.Write(buf, strlen(buf));

		const char* OldLocale = setlocale(LC_NUMERIC, "C");

		for (i = 1; i <= steps; i++)
		{
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				if ((pPiece->IsVisible(i, false)) && (pPiece->GetStepShow() == i))
				{
					float f[12], position[3], rotation[4];
					pPiece->GetPosition(position);
					pPiece->GetRotation(rotation);
					Matrix mat(rotation, position);
					mat.ToLDraw(f);
					sprintf (buf, " 1 %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %s.DAT\r\n",
						col[pPiece->GetColor()], f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], pPiece->GetPieceInfo()->m_strName);
					file.Write(buf, strlen(buf));
				}
			}

			if (i != steps)
				file.Write("0 STEP\r\n", 8);
		}
		file.Write("0\r\n", 3);

		setlocale(LC_NUMERIC, OldLocale);
	}
	else
		FileSave(&file, false);     // save me

	file.Close();

	SetModifiedFlag(false);     // back to unmodified

	// reset the title and change the document name
	if (bReplace)
		SetPathName(newName, true);

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

	switch (SystemDoMessageBox(prompt, LC_MB_YESNOCANCEL))
	{
	case LC_CANCEL:
		return false;       // don't continue

	case LC_YES:
		// If so, either Save or Update, as appropriate
		if (!DoFileSave())
			return false;       // don't continue
		break;

	case LC_NO:
		// If not saving changes, revert the document
		break;
	}

	return true;    // keep going
}


/////////////////////////////////////////////////////////////////////////////
// File operations

bool Project::OnNewDocument()
{
	SetTitle("Untitled");
	DeleteContents(false);
	memset(m_strPathName, 0, sizeof(m_strPathName)); // no path name yet
	strcpy(m_strAuthor, Sys_ProfileLoadString ("Default", "User", ""));
	SetModifiedFlag(false); // make clean
	LoadDefaults(true);
	CheckPoint("");

        //	SystemUpdateRecentMenu(m_strRecentFiles);
        messenger->Dispatch (LC_MSG_FOCUS_CHANGED, NULL);

//	CWnd* pFrame = AfxGetMainWnd();
//	if (pFrame != NULL)
//		pFrame->PostMessage (WM_LC_UPDATE_LIST, 1, m_nCurColor+1);
// set cur group to 0
 
	return true;
}

bool Project::OpenProject(const char* FileName)
{
	if (!SaveModified())
		return false;  // Leave the original one

//	CWaitCursor wait;
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
  FileDisk file;
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

  SystemDoWaitCursor(1);

  if (file.GetLength() != 0)
  {
    PtrArray<File> FileArray;

    // Unpack the MPD file.
    if (mpdfile)
    {
      FileReadMPD(file, FileArray);

      if (FileArray.GetSize() == 0)
      {
        file.Seek(0, SEEK_SET);
        mpdfile = false;
        datfile = true;
        console.PrintWarning("No files found inside the MPD, trying to load it as a .DAT file.\n");
      }
    }

    if (datfile || mpdfile)
    {
      int ok = 0, step = 1;
      Matrix mat;

      if (mpdfile)
        FileReadLDraw(FileArray[0], &mat, &ok, m_nCurColor, &step, FileArray);
      else
        FileReadLDraw(&file, &mat, &ok, m_nCurColor, &step, FileArray);

      m_nCurStep = step;
      SystemUpdateTime(false, m_nCurStep, 255);
      SystemUpdateFocus(NULL);
      UpdateSelection();
      CalculateStep();
      UpdateAllViews ();

      console.PrintMisc("%d objects imported.\n", ok);
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
  SystemDoWaitCursor(-1);

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
		main_window->AddToMRU(PathName);
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

	SystemUpdateUndoRedo(m_pUndoList->pNext ? m_pUndoList->strText : NULL, NULL);
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

	m_ViewList.RemovePointer(pView);
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
	glViewport(0, 0, view->GetWidth(), view->GetHeight());

	RenderBackground(view);

	// Setup the projection and camera matrices.
	float ratio = (float)view->GetWidth() / (float)view->GetHeight();
	view->m_Camera->LoadProjection(ratio);

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
		glClearColor(m_fBackground[0], m_fBackground[1], m_fBackground[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);

	float ViewWidth = (float)view->GetWidth();
	float ViewHeight = (float)view->GetHeight();

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
		m_pBackground->MakeCurrent();

		float Verts[4][2];
		float Coords[4][2];

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, Verts);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, Coords);

		float tw = 1.0f, th = 1.0f;
		if (m_nScene & LC_SCENE_BG_TILE)
		{
			tw = ViewWidth / m_pBackground->m_nWidth;
			th = ViewHeight / m_pBackground->m_nHeight;
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

typedef struct LC_BSPNODE
{
	float plane[4];
	Piece* piece;
	LC_BSPNODE* front;
	LC_BSPNODE* back;

	~LC_BSPNODE()
	{
		if (piece == NULL)
		{
			if (front)
				delete front;
			if (back)
				delete back;
		}
	}
} LC_BSPNODE;

static void RenderBSP(LC_BSPNODE* node, float* eye, bool* bSel, bool bLighting, bool bEdges, unsigned char* nLastColor, bool* bTrans)
{
	if (node->piece)
	{
		if (node->piece->IsSelected())
		{
			if (!*bSel)
			{
				*bSel = true;
				glLineWidth (2);//*m_fLineWidth);
			}
		}
		else
		{
			if (*bSel)
			{
				*bSel = false;
				glLineWidth(1);//m_fLineWidth);
			}
		}

		node->piece->Render(bLighting, bEdges, nLastColor, bTrans);
		return;
	}

	if (eye[0]*node->plane[0] + eye[1]*node->plane[1] +
		eye[2]*node->plane[2] + node->plane[3] > 0.0f)
	{
		RenderBSP(node->back, eye, bSel, bLighting, bEdges, nLastColor, bTrans);
		RenderBSP(node->front, eye, bSel, bLighting, bEdges, nLastColor, bTrans);
	}
	else
	{
		RenderBSP(node->front, eye, bSel, bLighting, bEdges, nLastColor, bTrans);
		RenderBSP(node->back, eye, bSel, bLighting, bEdges, nLastColor, bTrans);
	}
}

static void BuildBSP(LC_BSPNODE* node, Piece* pList)
{
	Piece *front_list = NULL, *back_list = NULL;
	Piece *pPiece, *pNext;

	node->piece = NULL;

	if (pList->m_pLink == NULL)
	{
		// This is a leaf
		node->piece = pList;
		return;
	}

	float dx, dy, dz, bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
	const float *pos;

	for (pPiece = pList; pPiece; pPiece = pPiece->m_pLink)
	{
		pos = pPiece->GetConstPosition();
		if (pos[0] < bs[0]) bs[0] = pos[0];
		if (pos[1] < bs[1]) bs[1] = pos[1];
		if (pos[2] < bs[2]) bs[2] = pos[2];
		if (pos[0] > bs[3]) bs[3] = pos[0];
		if (pos[1] > bs[4]) bs[4] = pos[1];
		if (pos[2] > bs[5]) bs[5] = pos[2];
	}

	dx = ABS(bs[0]-bs[3]);
	dy = ABS(bs[1]-bs[4]);
	dz = ABS(bs[2]-bs[5]);

	node->plane[0] = node->plane[1] = node->plane[2] = 0.0f;

	if (dx > dy)
	{
		if (dx > dz)
			node->plane[0] = 1.0f;
		else
			node->plane[2] = 1.0f;
	}
	else
	{
		if (dy > dz)
			node->plane[1] = 1.0f;
		else
			node->plane[2] = 1.0f;
	}

	// D = -Ax -By -Cz
	node->plane[3] = -(node->plane[0]*(bs[0]+bs[3])/2)-(node->plane[1]*(bs[1]+bs[4])/2)-(node->plane[2]*(bs[2]+bs[5])/2);

	for (pPiece = pList; pPiece;)
	{
		pos = pPiece->GetConstPosition();
		pNext = pPiece->m_pLink;

		if (pos[0]*node->plane[0] + pos[1]*node->plane[1] +
			pos[2]*node->plane[2] + node->plane[3] > 0.0f)
		{
			pPiece->m_pLink = front_list;
			front_list = pPiece;
		}
		else
		{
			pPiece->m_pLink = back_list;
			back_list = pPiece;
		}

		pPiece = pNext;
	}

	if (bs[0] == bs[3] && bs[1] == bs[4] && bs[2] == bs[5])
	{
		if (back_list)
		{
			front_list = back_list;
			back_list = back_list->m_pLink;
			front_list->m_pLink = NULL;
		}
		else
		{
			back_list = front_list;
			front_list = front_list->m_pLink;
			back_list->m_pLink = NULL;
		}
	}

	if (front_list)
	{
		node->front = new LC_BSPNODE;
		BuildBSP(node->front, front_list);
	}
	else
		node->front = NULL;

	if (back_list)
	{
		node->back = new LC_BSPNODE;
		BuildBSP(node->back, back_list);
	}
	else
		node->back = NULL;
}

void Project::RenderScenePieces(View* view)
{
	float AspectRatio = (float)view->GetWidth() / (float)view->GetHeight();
	view->m_Camera->LoadProjection(AspectRatio);

	if (m_nSnap & LC_DRAW_GRID)
	{
		glColor3f(1.0f - m_fBackground[0], 1.0f - m_fBackground[1], 1.0f - m_fBackground[2]);
		glCallList (m_nGridList);
	}

	if (m_nDetail & LC_DET_LIGHTING)
	{
		int index = 0;
		Light *pLight;

		for (pLight = m_pLights; pLight; pLight = pLight->m_pNext, index++)
			pLight->Setup (index);

		glEnable(GL_LIGHTING);
	}

	if (m_nScene & LC_SCENE_FOG)
		glEnable(GL_FOG);

	if (m_nScene & LC_SCENE_FLOOR)
		m_pTerrain->Render(view->m_Camera, AspectRatio);

	unsigned char nLastColor = 255;
	bool bTrans = false;
	bool bSel = false;
	bool bCull = false;
	Piece* pPiece;

	LC_BSPNODE tree;
	tree.front = tree.back = NULL;
	Piece* pList = NULL;

	// Draw opaque pieces first
	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (!pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			continue;

		if (!pPiece->IsTransparent())
		{
			if (pPiece->IsSelected())
			{
				if (!bSel)
				{
					bSel = true;
					glLineWidth (2*m_fLineWidth);
				}
			}
			else
			{
				if (bSel)
				{
					bSel = false;
					glLineWidth(m_fLineWidth);
				}
			}

			pPiece->Render((m_nDetail & LC_DET_LIGHTING) != 0, (m_nDetail & LC_DET_BRICKEDGES) != 0, &nLastColor, &bTrans);
		}
		else
		{
			pPiece->m_pLink = pList;
			pList = pPiece;
		}
	}

	if (pList)
	{
		float eye[3];
		view->m_Camera->GetEyePos (eye);
		BuildBSP(&tree, pList);
		RenderBSP(&tree, eye, &bSel, (m_nDetail & LC_DET_LIGHTING) != 0, (m_nDetail & LC_DET_BRICKEDGES) != 0, &nLastColor, &bTrans);
	}

	if (bTrans)
	{
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	if (bSel)
		glLineWidth(m_fLineWidth);

	if (bCull)
		glDisable(GL_CULL_FACE);
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
	if (m_nCurAction == LC_ACTION_INSERT)
	{
		Vector3 Pos;
		Vector4 Rot;
		GetPieceInsertPosition(view, m_nDownX, m_nDownY, Pos, Rot);

		glPushMatrix();
		glTranslatef(Pos[0], Pos[1], Pos[2]);
		glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);
		glLineWidth(2*m_fLineWidth);
		m_pCurPiece->RenderPiece(m_nCurColor);
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
			
	for (Camera* pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
	{
		if ((pCamera == view->m_Camera) || !pCamera->IsVisible())
			continue;
		pCamera->Render(m_fLineWidth);
	}

	for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		if (pLight->IsVisible ())
			pLight->Render(m_fLineWidth);

	// Draw axis icon
	if (m_nSnap & LC_DRAW_AXIS)
	{
//		glClear(GL_DEPTH_BUFFER_BIT);

		Matrix Mats[3];
		Mats[0].CreateLookat(view->m_Camera->GetEyePos(), view->m_Camera->GetTargetPos(), view->m_Camera->GetUpVec());
		Mats[0].SetTranslation(0, 0, 0);

		float m1[] = { 0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 };
		float m2[] = { 0, 0, 1, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 1 };
		Mats[1].Multiply(Mats[0], Matrix(m1));
		Mats[2].Multiply(Mats[0], Matrix(m2));

		float pts[3][3] = { { 20, 0, 0 }, { 0, 20, 0 }, { 0, 0, 20 } };
		Mats[0].TransformPoints(&pts[0][0], 3);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, view->GetWidth(), 0, view->GetHeight(), -50, 50);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(25.375f, 25.375f, 0.0f);

		// Draw the arrows.
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

			glBegin(GL_LINES);
			glVertex3f(pts[i][0], pts[i][1], pts[i][2]);
			glVertex3f(0, 0, 0);
			glEnd();

			glBegin(GL_TRIANGLE_FAN);
			glVertex3f(pts[i][0], pts[i][1], pts[i][2]);
			for (int j = 0; j < 9; j++)
			{
				float pt[3] = { 12.0f, cosf(LC_2PI * j / 8) * 3.0f, sinf(LC_2PI * j / 8) * 3.0f };
				Mats[i].TransformPoints(pt, 1);
				glVertex3f(pt[0], pt[1], pt[2]);
			}
			glEnd();
		}

		// Draw the text.
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_pScreenFont->MakeCurrent();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);

		glBegin(GL_QUADS);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		m_pScreenFont->PrintText(pts[0][0], pts[0][1], 40.0f, "X");
		m_pScreenFont->PrintText(pts[1][0], pts[1][1], 40.0f, "Y");
		m_pScreenFont->PrintText(pts[2][0], pts[2][1], 40.0f, "Z");
		glEnd();

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
	}
}

void Project::RenderOverlays(View* view)
{
	// Draw the selection rectangle.
	if ((m_nCurAction == LC_ACTION_SELECT) && (m_nTracking == LC_TRACK_LEFT) && (m_ActiveView == view))
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0f, view->GetWidth(), 0.0f, view->GetHeight(), -1.0f, 1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375f, 0.375f, 0.0f);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(5, 0x5555);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

		float pt1x = (float)m_nDownX;
		float pt1y = (float)m_nDownY;
		float pt2x = m_fTrack[0];
		float pt2y = m_fTrack[1];

		glBegin(GL_LINES);
		glVertex2f(pt1x, pt1y);
		glVertex2f(pt2x, pt1y);
		glVertex2f(pt2x, pt1y);
		glVertex2f(pt2x, pt2y);
		glVertex2f(pt2x, pt2y);
		glVertex2f(pt1x, pt2y);
		glVertex2f(pt1x, pt2y);
		glVertex2f(pt1x, pt1y);
		glEnd();

		glDisable(GL_LINE_STIPPLE);
		glEnable(GL_DEPTH_TEST);
	}

	const float OverlayScale = view->m_OverlayScale;

	if (m_nCurAction == LC_ACTION_MOVE)
	{
		const float OverlayMovePlaneSize = 0.5f;
		const float OverlayMoveArrowSize = 1.5f;
		const float OverlayMoveArrowCapSize = 0.9f;
		const float OverlayMoveArrowCapRadius = 0.1f;

		glDisable(GL_DEPTH_TEST);

		// Find the rotation from the focused piece if relative snap is enabled.
		Object* Focus = NULL;
		float Rot[4];

		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPiece())
				((Piece*)Focus)->GetRotation(Rot);
			else
				Focus = NULL;
		}

		// Draw a quad if we're moving on a plane.
		if ((m_OverlayMode == LC_OVERLAY_XY) || (m_OverlayMode == LC_OVERLAY_XZ) || (m_OverlayMode == LC_OVERLAY_YZ))
		{
			glPushMatrix();
			glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

			if (Focus)
				glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

			if (m_OverlayMode == LC_OVERLAY_XZ)
				glRotatef(90.0f, 0.0f, 0.0f, -1.0f);
			else if (m_OverlayMode == LC_OVERLAY_XY)
				glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);

			glBegin(GL_QUADS);
			glColor4f(0.8f, 0.8f, 0.0f, 0.3f);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(0.0f, OverlayScale * OverlayMovePlaneSize, 0.0f);
			glVertex3f(0.0f, OverlayScale * OverlayMovePlaneSize, OverlayScale * OverlayMovePlaneSize);
			glVertex3f(0.0f, 0.0f, OverlayScale * OverlayMovePlaneSize);
			glEnd();

			glDisable(GL_BLEND);

			glPopMatrix();
		}

		// Draw the arrows.
		for (int i = 0; i < 3; i++)
		{
			switch (i)
			{
			case 0:
				if ((m_OverlayMode == LC_OVERLAY_X) || (m_OverlayMode == LC_OVERLAY_XY) || (m_OverlayMode == LC_OVERLAY_XZ))
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else
					glColor4f(0.8f, 0.0f, 0.0f, 1.0f);
				break;
			case 1:
				if ((m_OverlayMode == LC_OVERLAY_Y) || (m_OverlayMode == LC_OVERLAY_XY) || (m_OverlayMode == LC_OVERLAY_YZ))
					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
				else
					glColor4f(0.0f, 0.8f, 0.0f, 1.0f);
				break;
			case 2:
				if ((m_OverlayMode == LC_OVERLAY_Z) || (m_OverlayMode == LC_OVERLAY_XZ) || (m_OverlayMode == LC_OVERLAY_YZ))
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
				glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
			else if (i == 2)
				glRotatef(90.0f, 0.0f, -1.0f, 0.0f);

			glBegin(GL_LINES);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(OverlayScale * OverlayMoveArrowSize, 0.0f, 0.0f);
			glEnd();

			glBegin(GL_TRIANGLE_FAN);
			glVertex3f(OverlayScale * OverlayMoveArrowSize, 0.0f, 0.0f);
			for (int j = 0; j < 9; j++)
			{
				float y = cosf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius * OverlayScale;
				float z = sinf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius * OverlayScale;
				glVertex3f(OverlayScale * OverlayMoveArrowCapSize, y, z);
			}
			glEnd();

			glPopMatrix();
		}

		glEnable(GL_DEPTH_TEST);
	}
	else if (m_nCurAction == LC_ACTION_ROTATE)
	{
		const float OverlayRotateRadius = 2.0f;

		glDisable(GL_DEPTH_TEST);

		Camera* Cam = view->m_Camera;
		Matrix44 Mat;
		int j;

		// Find the rotation from the focused piece if relative snap is enabled.
		Object* Focus = NULL;
		float Rot[4];

		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPiece())
				((Piece*)Focus)->GetRotation(Rot);
			else
				Focus = NULL;
		}

		// Draw a disc showing the rotation amount.
		if (m_MouseTotalDelta.LengthSquared() != 0.0f && (m_nTracking != LC_TRACK_NONE))
		{
			Vector4 Rotation;
			float Angle, Step;

			switch (m_OverlayMode)
			{
			case LC_OVERLAY_X:
				glColor4f(0.8f, 0.0f, 0.0f, 0.3f);
				Angle = m_MouseTotalDelta[0];
				Rotation = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
				break;
			case LC_OVERLAY_Y:
				glColor4f(0.0f, 0.8f, 0.0f, 0.3f);
				Angle = m_MouseTotalDelta[1];
				Rotation = Vector4(90.0f, 0.0f, 0.0f, 1.0f);
				break;
			case LC_OVERLAY_Z:
				glColor4f(0.0f, 0.0f, 0.8f, 0.3f);
				Angle = m_MouseTotalDelta[2];
				Rotation = Vector4(90.0f, 0.0f, -1.0f, 0.0f);
				break;
			default:
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

				glBegin(GL_TRIANGLE_FAN);

				glVertex3f(0.0f, 0.0f, 0.0f);

				float StartAngle;
				int i = 0;

				if (Step < 0)
					StartAngle = -Angle;
				else
					StartAngle = Angle;

				do
				{
					float x = cosf((Step * i - StartAngle) * DTOR) * OverlayRotateRadius * OverlayScale;
					float y = sinf((Step * i - StartAngle) * DTOR) * OverlayRotateRadius * OverlayScale;

					glVertex3f(0.0f, x, y);

					i++;
					if (Step > 0)
						Angle -= Step;
					else
						Angle += Step;

				} while (Angle >= 0.0f);

				glEnd();

				glDisable(GL_BLEND);

				glPopMatrix();
			}
		}

		Mat.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
		Mat.Transpose3();
		Mat.SetTranslation(m_OverlayCenter);

		// Draw the circles.
		glBegin(GL_LINE_LOOP);
		glColor3f(0.1f, 0.1f, 0.1f);

		for (j = 0; j < 32; j++)
		{
			Vector3 Pt;

			Pt[0] = cosf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
			Pt[1] = sinf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
			Pt[2] = 0.0f;

			Pt = Mul31(Pt, Mat);

			glVertex3f(Pt[0], Pt[1], Pt[2]);
		}

		glEnd();

		Vector3 ViewDir = Cam->GetTargetPosition() - Cam->GetEyePosition();
		ViewDir.Normalize();

		// Transform ViewDir to local space.
		if (Focus)
		{
			Matrix33 RotMat;
			RotMat.CreateFromAxisAngle(Vector3(Rot[0], Rot[1], Rot[2]), -Rot[3] * LC_DTOR);

			ViewDir = Mul(ViewDir, RotMat);
		}

		glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

		if (Focus)
			glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);

		// Draw each axis circle.
		for (int i = 0; i < 3; i++)
		{
			if (m_OverlayMode == LC_OVERLAY_X + i)
			{
				glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
			}
			else
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
			}

			glBegin(GL_LINES);

			for (int j = 0; j < 32; j++)
			{
				Vector3 v1, v2;

				switch (i)
				{
				case 0:
					v1 = Vector3(0.0f, cosf(LC_2PI * j / 32), sinf(LC_2PI * j / 32));
					v2 = Vector3(0.0f, cosf(LC_2PI * (j + 1) / 32), sinf(LC_2PI * (j + 1) / 32));
					break;

				case 1:
					v1 = Vector3(cosf(LC_2PI * j / 32), 0.0f, sinf(LC_2PI * j / 32));
					v2 = Vector3(cosf(LC_2PI * (j + 1) / 32), 0.0f, sinf(LC_2PI * (j + 1) / 32));
					break;

				case 2:
					v1 = Vector3(cosf(LC_2PI * j / 32), sinf(LC_2PI * j / 32), 0.0f);
					v2 = Vector3(cosf(LC_2PI * (j + 1) / 32), sinf(LC_2PI * (j + 1) / 32), 0.0f);
					break;
				}

				if (Dot3(ViewDir, v1+v2) <= 0.0f)
				{
					Vector3 Pt1 = v1 * OverlayRotateRadius * OverlayScale;
					Vector3 Pt2 = v2 * OverlayRotateRadius * OverlayScale;

					glVertex3f(Pt1[0], Pt1[1], Pt1[2]);
					glVertex3f(Pt2[0], Pt2[1], Pt2[2]);
				}
			}

			glEnd();
		}

		// Draw tangent vector.
		if (m_nTracking != LC_TRACK_NONE)
		{
			if ((m_OverlayMode == LC_OVERLAY_X) || (m_OverlayMode == LC_OVERLAY_Y) || (m_OverlayMode == LC_OVERLAY_Z))
			{
				Vector3 Tangent, Normal = m_OverlayTrackStart - m_OverlayCenter;
				Normal.Normalize();
				float Angle;

				switch (m_OverlayMode)
				{
				case LC_OVERLAY_X:
					Angle = m_MouseTotalDelta[0];
					Tangent = Vector3(0.0f, -Normal[2], Normal[1]);
					break;
				case LC_OVERLAY_Y:
					Angle = m_MouseTotalDelta[1];
					Tangent = Vector3(Normal[2], 0.0f, -Normal[0]);
					break;
				case LC_OVERLAY_Z:
					Angle = m_MouseTotalDelta[2];
					Tangent = Vector3(-Normal[1], Normal[0], 0.0f);
					break;
				}

				if (Angle < 0.0f)
				{
					Tangent = -Tangent;
					Angle = -Angle;
				}

				// Draw tangent arrow.
				if (Angle > 0.0f)
				{
					const float OverlayRotateArrowSize = 1.5f;
					const float OverlayRotateArrowCapSize = 0.25f;

					Vector3 Pt = Normal * OverlayScale * OverlayRotateRadius;
					Vector3 Tip = Pt + Tangent * OverlayScale * OverlayRotateArrowSize;
					Vector3 Arrow;
					Matrix33 Rot;

					glBegin(GL_LINES);
					glColor3f(0.8f, 0.8f, 0.0f);

					glVertex3f(Pt[0], Pt[1], Pt[2]);
					glVertex3f(Tip[0], Tip[1], Tip[2]);

					Rot.CreateFromAxisAngle(Normal, LC_PI * 0.15f);
					Arrow = Mul(Tangent, Rot) * OverlayRotateArrowCapSize;

					glVertex3f(Tip[0], Tip[1], Tip[2]);
					glVertex3f(Tip[0] - Arrow[0], Tip[1] - Arrow[1], Tip[2] - Arrow[2]);

					Rot.CreateFromAxisAngle(Normal, -LC_PI * 0.15f);
					Arrow = Mul(Tangent, Rot) * OverlayRotateArrowCapSize;

					glVertex3f(Tip[0], Tip[1], Tip[2]);
					glVertex3f(Tip[0] - Arrow[0], Tip[1] - Arrow[1], Tip[2] - Arrow[2]);

					glEnd();
				}

				// Draw text.
//				if (Viewport == m_nActiveViewport)
				{
					int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };
					float Aspect = (float)Viewport[2]/(float)Viewport[3];
					Camera* Cam = view->m_Camera;

					Matrix44 ModelView, Projection;
					ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
					Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

					Vector3 Screen = ProjectPoint(Vector3(0, 0, 0), ModelView, Projection, Viewport);

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
					sprintf(buf, "[%.2f]", Angle);

					int cx, cy;
					m_pScreenFont->GetStringDimensions(&cx, &cy, buf);

					glBegin(GL_QUADS);
					glColor3f(0.8f, 0.8f, 0.0f);
					m_pScreenFont->PrintText(Screen[0] - Viewport[0] - (cx / 2), Screen[1] - Viewport[1] + (cy / 2), 0.0f, buf);
					glEnd();

					glDisable(GL_TEXTURE_2D);
					glDisable(GL_ALPHA_TEST);

					glMatrixMode(GL_PROJECTION);
					glPopMatrix();
					glMatrixMode(GL_MODELVIEW);
					glPopMatrix();
				}
			}
		}

		glEnable(GL_DEPTH_TEST);
	}
	else if (m_nCurAction == LC_ACTION_ROTATE_VIEW)
	{
		int x, y, w, h;

		x = 0;
		y = 0;
		w = view->GetWidth();
		h = view->GetHeight();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375f, 0.375f, 0.0f);

		glDisable(GL_DEPTH_TEST);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

		// Draw circle.
		glBegin(GL_LINE_LOOP);

		float r = min(w, h) * 0.35f;
		float cx = x + w / 2.0f;
		float cy = y + h / 2.0f;

		for (int i = 0; i < 32; i++)
		{
			float x = cosf((float)i / 32.0f * (2.0f * LC_PI)) * r + cx;
			float y = sinf((float)i / 32.0f * (2.0f * LC_PI)) * r + cy;

			glVertex2f(x, y);
		}

		glEnd();

		const float OverlayCameraSquareSize = max(8.0f, (w+h)/200);

		// Draw squares.
		glBegin(GL_LINE_LOOP);
		glVertex2f(cx + OverlayCameraSquareSize, cy + r + OverlayCameraSquareSize);
		glVertex2f(cx - OverlayCameraSquareSize, cy + r + OverlayCameraSquareSize);
		glVertex2f(cx - OverlayCameraSquareSize, cy + r - OverlayCameraSquareSize);
		glVertex2f(cx + OverlayCameraSquareSize, cy + r - OverlayCameraSquareSize);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex2f(cx + OverlayCameraSquareSize, cy - r + OverlayCameraSquareSize);
		glVertex2f(cx - OverlayCameraSquareSize, cy - r + OverlayCameraSquareSize);
		glVertex2f(cx - OverlayCameraSquareSize, cy - r - OverlayCameraSquareSize);
		glVertex2f(cx + OverlayCameraSquareSize, cy - r - OverlayCameraSquareSize);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex2f(cx + r + OverlayCameraSquareSize, cy + OverlayCameraSquareSize);
		glVertex2f(cx + r - OverlayCameraSquareSize, cy + OverlayCameraSquareSize);
		glVertex2f(cx + r - OverlayCameraSquareSize, cy - OverlayCameraSquareSize);
		glVertex2f(cx + r + OverlayCameraSquareSize, cy - OverlayCameraSquareSize);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex2f(cx - r + OverlayCameraSquareSize, cy + OverlayCameraSquareSize);
		glVertex2f(cx - r - OverlayCameraSquareSize, cy + OverlayCameraSquareSize);
		glVertex2f(cx - r - OverlayCameraSquareSize, cy - OverlayCameraSquareSize);
		glVertex2f(cx - r + OverlayCameraSquareSize, cy - OverlayCameraSquareSize);
		glEnd();

		glEnable(GL_DEPTH_TEST);
	}
	else if (m_nCurAction == LC_ACTION_ZOOM_REGION)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, view->GetWidth(), 0, view->GetHeight(), -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375f, 0.375f, 0.0f);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(5, 0x5555);
		glColor3f(0, 0, 0);

		float pt1x = (float)m_nDownX;
		float pt1y = (float)m_nDownY;
		float pt2x = m_OverlayTrackStart[0];
		float pt2y = m_OverlayTrackStart[1];

		glBegin(GL_LINES);
		glVertex2f(pt1x, pt1y);
		glVertex2f(pt2x, pt1y);
		glVertex2f(pt2x, pt1y);
		glVertex2f(pt2x, pt2y);
		glVertex2f(pt2x, pt2y);
		glVertex2f(pt1x, pt2y);
		glVertex2f(pt1x, pt2y);
		glVertex2f(pt1x, pt1y);
		glEnd();

		glDisable(GL_LINE_STIPPLE);
		glEnable(GL_DEPTH_TEST);
	}
}

void Project::RenderViewports(View* view)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, view->GetWidth(), 0.0f, view->GetHeight(), -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);

	// Draw camera name
	glColor3f(0, 0, 0);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	m_pScreenFont->MakeCurrent();
	glEnable(GL_ALPHA_TEST);

	glBegin(GL_QUADS);
	m_pScreenFont->PrintText(3.0f, (float)view->GetHeight() - 1.0f - 6.0f, 0.0f, view->m_Camera->GetName());
	glEnd();
	
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}

// Initialize OpenGL
void Project::RenderInitialize()
{
	int i;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);

	glCullFace(GL_BACK);
	glDisable (GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	if (m_nDetail & LC_DET_SMOOTH)
		glShadeModel(GL_SMOOTH);
	else
		glShadeModel(GL_FLAT);

	if (m_nDetail & LC_DET_LIGHTING)
	{
		glEnable(GL_LIGHTING);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
		glEnable(GL_COLOR_MATERIAL);

		GLfloat mat_translucent[] = { (GLfloat)0.8, (GLfloat)0.8, (GLfloat)0.8, (GLfloat)1.0 };
		GLfloat mat_opaque[] = { (GLfloat)0.8, (GLfloat)0.8, (GLfloat)0.8, (GLfloat)1.0 };
		GLfloat medium_shininess[] = { (GLfloat)64.0 };

		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, medium_shininess);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_opaque);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_translucent);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_COLOR_MATERIAL);
	}

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, m_fAmbient);
	glClearColor(m_fBackground[0], m_fBackground[1], m_fBackground[2], 1.0f);

	if (m_nScene & LC_SCENE_FOG)
	{
		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE, GL_EXP);
		glFogf(GL_FOG_DENSITY, m_fFogDensity);
		glFogfv(GL_FOG_COLOR, m_fFogColor);
	}
	else
		glDisable (GL_FOG);

	// Load font
	if (!m_pScreenFont->IsLoaded())
	{
		char filename[LC_MAXPATH];
		FileDisk file;

		strcpy(filename, lcGetPiecesLibrary()->GetLibraryPath());
		strcat(filename, "sysfont.txf");

		if (file.Open(filename, "rb"))
			m_pScreenFont->FileLoad(file);
	}

	glAlphaFunc(GL_GREATER, 0.0625);

	if (m_nScene & LC_SCENE_FLOOR)
		m_pTerrain->LoadTexture(true);

	if (m_nScene & LC_SCENE_BG)
		if (!m_pBackground->LoadFromFile(m_strBackground, true))
		{
			m_nScene &= ~LC_SCENE_BG;
//			AfxMessageBox ("Could not load background");
		}

	// Set the perspective correction hint to fastest or nicest...
//	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	// Grid display list
	if (m_nGridList == 0)
		m_nGridList = glGenLists(1);
	glNewList (m_nGridList, GL_COMPILE);
	glEnableClientState(GL_VERTEX_ARRAY);
	i = 2*(4*m_nGridSize+2); // verts needed (2*lines)
	float *grid = (float*)malloc(i*sizeof(float[3]));
	float x = m_nGridSize*0.8f;

	for (int j = 0; j <= m_nGridSize*2; j++)
	{
		grid[j*12] = x;
		grid[j*12+1] = m_nGridSize*0.8f;
		grid[j*12+2] = 0;
		grid[j*12+3] = x;
		grid[j*12+4] = -m_nGridSize*0.8f;
		grid[j*12+5] = 0;
		grid[j*12+6] = m_nGridSize*0.8f;
		grid[j*12+7] = x;
		grid[j*12+8] = 0;
		grid[j*12+9] = -m_nGridSize*0.8f;
		grid[j*12+10] = x;
		grid[j*12+11] = 0;
		x -= 0.8f;
	}
	glVertexPointer(3, GL_FLOAT, 0, grid);
	glDrawArrays(GL_LINES, 0, i);
	glEndList();
	free(grid);
}

/////////////////////////////////////////////////////////////////////////////
// Project functions

void Project::AddPiece(Piece* pPiece)
{
/*
// Sort piece array to avoid OpenGL state changes (needs work)
void CCADDoc::AddPiece(CPiece* pNewPiece)
{
	POSITION pos1, pos2;

	for (pos1 = m_Pieces.GetHeadPosition(); (pos2 = pos1) != NULL;)
	{
		CPiece* pPiece = m_Pieces.GetNext(pos1);
		if (pPiece->IsTransparent())
			break;
	}

	if (pos2 == NULL || pNewPiece->IsTransparent())
		m_Pieces.AddTail(pNewPiece);
	else
		m_Pieces.InsertBefore(pos2, pNewPiece);
}
*/
	if (m_pPieces != NULL)
	{
		pPiece->m_pNext = m_pPieces;
		m_pPieces = pPiece;
	// TODO: sorting and BSP
	}
	else
	{
		m_pPieces = pPiece;
		pPiece->m_pNext = NULL;
	}

	pPiece->AddConnections(m_pConnections);
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

	pPiece->RemoveConnections(m_pConnections);

	// TODO: remove from BSP
}

void Project::CalculateStep()
{
	int PieceCount = 0;
	Piece* pPiece;
	Camera* pCamera;
	Light* pLight;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
		PieceCount++;
	}

  SystemDoWaitCursor(1);
	SystemStartProgressBar(0, PieceCount, 1, "Updating pieces...");

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, false);
		SytemStepProgressBar();
	}

	SytemEndProgressBar();
  SystemDoWaitCursor(-1);

	for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		pCamera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		pLight->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
}

// Returns true if anything was removed (used by cut and del)
bool Project::RemoveSelectedObjects()
{
	Piece* pPiece;
	Camera* pCamera;
	Light* pLight;
	void* pPrev;
	bool removed = false;

	pPiece = m_pPieces;
	while (pPiece)
	{
		if (pPiece->IsSelected())
		{
			Piece* pTemp;
			pTemp = pPiece->m_pNext;

			removed = true;
			RemovePiece(pPiece);
			delete pPiece;
			pPiece = pTemp;
		}
		else
			pPiece = pPiece->m_pNext;
	}

	// Cameras can't be removed while being used or default
	for (pPrev = NULL, pCamera = m_pCameras; pCamera; )
	{
		bool bCanDelete = true;

		for (int ViewIdx = 0; ViewIdx < m_ViewList.GetSize(); ViewIdx++)
		{
			if (pCamera == m_ViewList[ViewIdx]->m_Camera)
			{
				bCanDelete = false;
				break;
			}
		}

		if (bCanDelete && pCamera->IsSelected() && pCamera->IsUser())
		{
			if (pPrev)
			{
				((Camera*)pPrev)->m_pNext = pCamera->m_pNext;
				delete pCamera;
				pCamera = ((Camera*)pPrev)->m_pNext;
			}
			else
			{
				m_pCameras = m_pCameras->m_pNext;
				delete pCamera;
				pCamera = m_pCameras;
			}
			
			removed = true;

			SystemUpdateCameraMenu(m_pCameras);
			SystemUpdateCurrentCamera(NULL, m_ActiveView->m_Camera, m_pCameras);
		}
		else
		{
			pPrev = pCamera;
			pCamera = pCamera->m_pNext;
		}
	}

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

			removed = true;
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

	return removed;
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

	for (Camera* pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		if (pCamera->IsSelected())
		{
			flags |= LC_SEL_CAMERA;
			SelectedCount++;

			if (pCamera->IsEyeFocused() || pCamera->IsTargetFocused())
				Focus = pCamera;
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
		ActivateOverlay();
	}

	SystemUpdateSelected(flags, SelectedCount, Focus);
}

void Project::CheckAutoSave()
{
	m_nSaveTimer += 5;
	if (m_nAutosave & LC_AUTOSAVE_FLAG)
	{
		int nInterval;
		nInterval = m_nAutosave & ~LC_AUTOSAVE_FLAG;

		if (m_nSaveTimer >= (m_nAutosave*60))
		{
			m_nSaveTimer = 0;
/*
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
*/		}
	}
}

unsigned char Project::GetLastStep()
{
	unsigned char last = 1;
	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		last = max(last, pPiece->GetStepShow());

	return last;
}

// Create a series of pictures
void Project::CreateImages (Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite)
{
	unsigned short oldtime;
	void* render = Sys_StartMemoryRender (width, height);
	unsigned char* buf = (unsigned char*)malloc (width*height*3);
	oldtime = m_bAnimation ? m_nCurFrame : m_nCurStep;

	View view(this, m_ActiveView);
	view.m_Camera = m_ActiveView->m_Camera;
	view.OnSize(width, height);

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
	Sys_FinishMemoryRender (render);
}

void Project::CreateHTMLPieceList(FILE* f, int nStep, bool bImages, const char* ext)
{
	Piece* pPiece;
	int col[LC_MAXCOLORS], ID = 0, c;
	memset (&col, 0, sizeof (col));
	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if ((pPiece->GetStepShow() == nStep) || (nStep == 0))
			col[pPiece->GetColor()]++;
	}
	fputs("<br><table border=1><tr><td><center>Piece</center></td>\n",f);

	for (c = 0; c < LC_MAXCOLORS; c++)
	if (col[c])
	{
		col[c] = ID;
		ID++;
		fprintf(f, "<td><center>%s</center></td>\n", colornames[c]);
	}
	ID++;
	fputs("</tr>\n",f);

	PieceInfo* pInfo;
	for (int j = 0; j < lcGetPiecesLibrary()->GetPieceCount (); j++)
	{
		bool Add = false;
		int count[LC_MAXCOLORS];
		memset (&count, 0, sizeof (count));
		pInfo = lcGetPiecesLibrary()->GetPieceInfo (j);

		for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		{
			if ((pPiece->GetPieceInfo() == pInfo) && 
				((pPiece->GetStepShow() == nStep) || (nStep == 0)))
			{
				count [pPiece->GetColor()]++;
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
			for (c = 0; c < LC_MAXCOLORS; c++)
				if (count[c])
				{
					while (curcol != col[c] + 1)
					{
						fputs("<td><center>-</center></td>\n", f);
						curcol++;
					}

					fprintf(f, "<td><center>%d</center></td>\n", count[c]);
					curcol++;
				}

			while (curcol != ID)
			{
				fputs("<td><center>-</center></td>\n", f);
				curcol++;
			}

			fputs("</tr>\n", f);
		}
	}
	fputs("</table>\n<br>", f);
}

// Special notifications.
void Project::HandleNotify(LC_NOTIFY id, unsigned long param)
{
	switch (id)
	{
		case LC_COLOR_CHANGED:
		{
			m_nCurColor = (unsigned char)param;
		} break;

		case LC_CAPTURE_LOST:
		{
			if (m_nTracking != LC_TRACK_NONE)
				StopTracking(false);
		} break;

		// Application is (de)activated
		case LC_ACTIVATE:
		{
			if (param == 0)
				SystemExportClipboard(m_pClipboard[m_nCurClipboard]);
			else
			{
				free(m_pClipboard[m_nCurClipboard]);
				m_pClipboard[m_nCurClipboard] = SystemImportClipboard();
				SystemUpdatePaste(m_pClipboard[m_nCurClipboard] != NULL);
			}
		} break;

		// FIXME: don't change the keys with ChangeKey()
		// FIXME: even if pos == prevpos, the user might want to add a key

		case LC_PIECE_MODIFIED:
		{
			LC_PIECE_MODIFY* mod = (LC_PIECE_MODIFY*)param;
			Piece* pPiece = (Piece*)mod->piece;

			float rot[4];
			Vector3 Pos = pPiece->GetPosition();
			pPiece->GetRotation(rot);
			Matrix mat(rot, Pos);
			mat.ToEulerAngles(rot);

			if (Pos != mod->Position)
				pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->Position, LC_PK_POSITION);

			if (mod->Rotation[0] != rot[0] || mod->Rotation[1] != rot[1] || mod->Rotation[2] != rot[2])
			{
				mat.FromEulerAngles(mod->Rotation[0], mod->Rotation[1], mod->Rotation[2]);
				mat.ToAxisAngle(rot);
				pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, rot, LC_PK_ROTATION);
			}

			if (m_bAnimation)
			{
				pPiece->SetFrameShow(mod->from);
				pPiece->SetFrameHide(mod->to);
			}
			else
			{
				pPiece->SetStepShow(mod->from);
				pPiece->SetStepHide(mod->to);
			}

			if (mod->hidden)
				pPiece->Hide();
			else
				pPiece->UnHide();

			pPiece->SetName(mod->name);
			pPiece->SetColor(mod->color);
			pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
			pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);

			SetModifiedFlag(true);
			CheckPoint("Modifying");
			ActivateOverlay();
			UpdateAllViews();
		} break;

		case LC_CAMERA_MODIFIED:
		{
			LC_CAMERA_MODIFY* mod = (LC_CAMERA_MODIFY*)param;
			Camera* pCamera = (Camera*)mod->camera;

			if (mod->hidden)
				pCamera->Hide();
			else
				pCamera->UnHide();

			if (pCamera->GetEyePosition() != mod->Eye)
				pCamera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->Eye, LC_CK_EYE);

			if (pCamera->GetTargetPosition() != mod->Target)
				pCamera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->Target, LC_CK_TARGET);

			if (pCamera->GetUpVector() != mod->Up)
				pCamera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->Up, LC_CK_UP);

			pCamera->m_fovy = mod->fovy;
			pCamera->m_zNear = mod->znear;
			pCamera->m_zFar = mod->zfar;
			pCamera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
			UpdateAllViews ();
		} break;

		case LC_LIGHT_MODIFIED:
		{
		} break;
	}
}

void Project::HandleMessage(int Message, void* Data)
{
	if (Message == LC_MSG_FOCUS_CHANGED)
	{
	}
}

// Handle (almost) all menu/toolbar commands here.
void Project::HandleCommand(LC_COMMANDS id, unsigned long nParam)
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
			char filename[LC_MAXPATH];
			strcpy(filename, m_strModelsPath);

			if (SystemDoDialog(LC_DLG_FILE_OPEN_PROJECT, filename))
			{
				OpenProject(filename);
			}
		} break;
		
		case LC_FILE_MERGE:
		{
			char filename[LC_MAXPATH];
			strcpy(filename, m_strModelsPath);

			if (SystemDoDialog(LC_DLG_FILE_MERGE_PROJECT, filename))
			{
				FileDisk file;
				if (file.Open(filename, "rb"))
				{
//				CWaitCursor wait;
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
			DoFileSave();
		} break;

		case LC_FILE_SAVEAS:
		{
			DoSave(NULL, true);
		} break;

		case LC_FILE_PICTURE:
		{
			LC_IMAGEDLG_OPTS opts;
			opts.from = 1;
			opts.to = m_bAnimation ? m_nTotalFrames : GetLastStep();
			opts.multiple = m_bAnimation;
			opts.imopts.background[0] = (unsigned char)(m_fBackground[0]*255);
			opts.imopts.background[1] = (unsigned char)(m_fBackground[1]*255);
			opts.imopts.background[2] = (unsigned char)(m_fBackground[2]*255);

			if (SystemDoDialog(LC_DLG_PICTURE_SAVE, &opts))
			{
				if (m_bAnimation)
					opts.to = min(opts.to, m_nTotalFrames);
				else
					opts.to = min(opts.to, 255);
				opts.from = max(1, opts.from);

				if (opts.multiple)
				{
					if (opts.from > opts.to)
					{
						unsigned short t = opts.from;
						opts.from = opts.to;
						opts.to = t;
					}
				}
				else
					opts.from = opts.to = m_bAnimation ? m_nCurFrame : m_nCurStep;

				Image* images = new Image[opts.to-opts.from+1];
				CreateImages (images, opts.width, opts.height, opts.from, opts.to, false);

				char *ptr, ext[4];
				ptr = strrchr(opts.filename, '.');
				if (ptr != NULL)
				{
					strncpy(ext, ptr+1, 3);
					strlwr(ext);
				}

				if (strcmp(ext, "avi") == 0)
				{
					SaveVideo(opts.filename, images, opts.to-opts.from+1, m_bAnimation ? m_nFPS : 60.0f/opts.imopts.pause);
        }
				else
				{
					for (int i = 0; i <= opts.to-opts.from; i++)
					{
						char filename[LC_MAXPATH];
						if (opts.multiple)
						{
							char* ext = strrchr(opts.filename, '.');
							*ext = 0;
							sprintf(filename, "%s%02d.%s", opts.filename, i+1, ext+1);
              *ext = '.';
						}
						else
							strcpy(filename, opts.filename);

						images[i].FileSave (filename, &opts.imopts);
					}
				}
				delete []images;
			}
		} break;

		case LC_FILE_3DS:
		{
#ifdef LC_WINDOWS
			Export3DStudio();
#endif
		} break;

    case LC_FILE_HTML:
    {
      LC_HTMLDLG_OPTS opts;

      strcpy (opts.path, Sys_ProfileLoadString ("Default", "HTML Path", ""));
      if (strlen (opts.path) == 0)
      {
        strcpy (opts.path, m_strPathName);
        if (strlen(opts.path) > 0)
		  	{
			  	char* ptr = strrchr(opts.path, '/');
				  if (ptr == NULL)
					  ptr = strrchr(opts.path, '\\');
				  if (ptr)
          {
            ptr++;
            *ptr = 0;
          }
  			}
      }

      unsigned long image = Sys_ProfileLoadInt ("Default", "HTML Image Options", 1|LC_IMAGE_TRANSPARENT);
			opts.imdlg.imopts.background[0] = (unsigned char)(m_fBackground[0]*255);
			opts.imdlg.imopts.background[1] = (unsigned char)(m_fBackground[1]*255);
			opts.imdlg.imopts.background[2] = (unsigned char)(m_fBackground[2]*255);
			opts.imdlg.from = 1;
			opts.imdlg.to = 1;
			opts.imdlg.multiple = false;
			opts.imdlg.width = Sys_ProfileLoadInt ("Default", "HTML Image Width", 256);
			opts.imdlg.height = Sys_ProfileLoadInt ("Default", "HTML Image Height", 160);
			opts.imdlg.imopts.quality = Sys_ProfileLoadInt ("Default", "JPEG Quality", 70);
			opts.imdlg.imopts.interlaced = (image & LC_IMAGE_PROGRESSIVE) != 0;
			opts.imdlg.imopts.transparent = (image & LC_IMAGE_TRANSPARENT) != 0;
			opts.imdlg.imopts.truecolor = (image & LC_IMAGE_HIGHCOLOR) != 0;
			opts.imdlg.imopts.pause = 1;
			opts.imdlg.imopts.format = (unsigned char)(image & ~(LC_IMAGE_MASK));

      unsigned long ul = Sys_ProfileLoadInt ("Default", "HTML Options", LC_HTML_SINGLEPAGE);
      opts.singlepage = (ul & LC_HTML_SINGLEPAGE) != 0;
      opts.index = (ul & LC_HTML_INDEX) != 0;
      opts.images = (ul & LC_HTML_IMAGES) != 0;
      opts.listend = (ul & LC_HTML_LISTEND) != 0;
      opts.liststep = (ul & LC_HTML_LISTSTEP) != 0;
      opts.highlight = (ul & LC_HTML_HIGHLIGHT) != 0;
      opts.htmlext = (ul & LC_HTML_HTMLEXT) != 0;

			if (SystemDoDialog(LC_DLG_HTML, &opts))
			{
				FILE* f;
				const char *ext, *htmlext;
				char fn[LC_MAXPATH];
				int i;
				unsigned short last = GetLastStep();

        // Save HTML options
        ul = 0;
        if (opts.singlepage) ul |= LC_HTML_SINGLEPAGE;
        if (opts.index) ul |= LC_HTML_INDEX;
        if (opts.images) ul |= LC_HTML_IMAGES;
        if (opts.listend) ul |= LC_HTML_LISTEND;
        if (opts.liststep) ul |= LC_HTML_LISTSTEP;
        if (opts.highlight) ul |= LC_HTML_HIGHLIGHT;
        if (opts.htmlext) ul |= LC_HTML_HTMLEXT;
        Sys_ProfileSaveInt ("Default", "HTML Options", ul);

        // Save image options
        ul = opts.imdlg.imopts.format;
        if (opts.imdlg.imopts.interlaced)
          ul |= LC_IMAGE_PROGRESSIVE;
        if (opts.imdlg.imopts.transparent)
          ul |= LC_IMAGE_TRANSPARENT;
        if (opts.imdlg.imopts.truecolor)
          ul |= LC_IMAGE_HIGHCOLOR;
        Sys_ProfileSaveInt ("Default", "HTML Image Options", ul);
			  Sys_ProfileSaveInt ("Default", "HTML Image Width", opts.imdlg.width);
			  Sys_ProfileSaveInt ("Default", "HTML Image Height", opts.imdlg.height);

				switch (opts.imdlg.imopts.format)
				{
				case LC_IMAGE_BMP: ext = ".bmp"; break;
        default:
				case LC_IMAGE_GIF: ext = ".gif"; break;
				case LC_IMAGE_JPG: ext = ".jpg"; break;
				case LC_IMAGE_PNG: ext = ".png"; break;
				}

        if (opts.htmlext)
          htmlext = ".html";
        else
          htmlext = ".htm";

        i = strlen (opts.path);
        if (i && opts.path[i] != '/' && opts.path[i] != '\\')
          strcat (opts.path, "/");
        Sys_ProfileSaveString ("Default", "HTML Path", opts.path);

/*
				// Create destination folder
        char *MyPath = strdup(dlg.m_strFolder);
        char *p = MyPath;
        int psave;
        while(*p)
        {
          while(*p && *p != '\\')
						++p;

					psave = *p;
					if (*p == '\\')
						*p = 0;

					if (strlen(MyPath) > 3)
						CreateDirectory(MyPath, NULL);

					*p = psave;
					if (*p)
						++p;
				}
				free(MyPath);
*/
        main_window->BeginWait ();

				if (opts.singlepage)
				{
					strcpy (fn, opts.path);
					strcat (fn, m_strTitle);
          strcat (fn, htmlext);
					f = fopen (fn, "wt");
					fprintf (f, "<HTML>\n<HEAD>\n<TITLE>Instructions for %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

					for (i = 1; i <= last; i++)
					{
						fprintf(f, "<IMG SRC=\"%s-%02d%s\" ALT=\"Step %02d\" WIDTH=%d HEIGHT=%d><BR><BR>\n", 
							m_strTitle, i, ext, i, opts.imdlg.width, opts.imdlg.height);
	
						if (opts.liststep)
							CreateHTMLPieceList(f, i, opts.images, ext);
					}

					if (opts.listend)
						CreateHTMLPieceList(f, 0, opts.images, ext);

					fputs("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\n", f);
					fclose(f);
				}
				else
				{
					if (opts.index)
					{
						strcpy (fn, opts.path);
						strcat (fn, m_strTitle);
						strcat (fn, "-index");
            strcat (fn, htmlext);
						f = fopen (fn, "wt");

						fprintf(f, "<HTML>\n<HEAD>\n<TITLE>Instructions for %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

						for (i = 1; i <= last; i++)
							fprintf(f, "<A HREF=\"%s-%02d%s\">Step %d<BR>\n</A>", m_strTitle, i, htmlext, i);

						if (opts.listend)
							fprintf(f, "<A HREF=\"%s-pieces%s\">Pieces Used</A><BR>\n", m_strTitle, htmlext);

						fputs("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.leocad.org\">LeoCAD</A></B></I><BR></HTML>\n", f);
						fclose(f);
					}

					// Create each step
					for (i = 1; i <= last; i++)
					{
						sprintf(fn, "%s%s-%02d%s", opts.path, m_strTitle, i, htmlext);
						f = fopen(fn, "wt");

						fprintf(f, "<HTML>\n<HEAD>\n<TITLE>%s - Step %02d</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle, i);
						fprintf(f, "<IMG SRC=\"%s-%02d%s\" ALT=\"Step %02d\" WIDTH=%d HEIGHT=%d><BR><BR>\n", 
							m_strTitle, i, ext, i, opts.imdlg.width, opts.imdlg.height);
	
						if (opts.liststep)
							CreateHTMLPieceList(f, i, opts.images, ext);

						fputs("</CENTER>\n<BR><HR><BR>", f);
						if (i != 1)
							fprintf(f, "<A HREF=\"%s-%02d%s\">Previous</A> ", m_strTitle, i-1, htmlext);

						if (opts.index)
							fprintf(f, "<A HREF=\"%s-index%s\">Index</A> ", m_strTitle, htmlext);

						if (i != last)
							fprintf(f, "<A HREF=\"%s-%02d%s\">Next</A>", m_strTitle, i+1, htmlext);
						else
							if (opts.listend)
								fprintf(f, "<A HREF=\"%s-pieces%s\">Pieces Used</A>", m_strTitle, htmlext);

						fputs("<BR></HTML>\n",f);
						fclose(f);
					}

					if (opts.listend)
					{
						strcpy (fn, opts.path);
						strcat (fn, m_strTitle);
						strcat (fn, "-pieces");
            strcat (fn, htmlext);
						f = fopen (fn, "wt");
						fprintf (f, "<HTML>\n<HEAD>\n<TITLE>Pieces used by %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);
				
						CreateHTMLPieceList(f, 0, opts.images, ext);

						fputs("</CENTER>\n<BR><HR><BR>", f);
						fprintf(f, "<A HREF=\"%s-%02d%s\">Previous</A> ", m_strTitle, i-1, htmlext);

						if (opts.index)
							fprintf(f, "<A HREF=\"%s-index%s\">Index</A> ", m_strTitle, htmlext);

						fputs("<BR></HTML>\n",f);
						fclose(f);
					}
				}

				// Save step pictures
				Image* images = new Image[last];
				CreateImages (images, opts.imdlg.width, opts.imdlg.height, 1, last, opts.highlight);

				for (i = 0; i < last; i++)
				{
					sprintf(fn, "%s%s-%02d%s", opts.path, m_strTitle, i+1, ext);
					images[i].FileSave (fn, &opts.imdlg.imopts);
				}
				delete []images;

				if (opts.images)
				{
					int cx = 120, cy = 100;
					void* render = Sys_StartMemoryRender (cx, cy);

					float aspect = (float)cx/(float)cy;
					glViewport(0, 0, cx, cy);

					Piece *p1, *p2;
					PieceInfo* pInfo;
					for (p1 = m_pPieces; p1; p1 = p1->m_pNext)
					{
						bool bSkip = false;
						pInfo = p1->GetPieceInfo();

						for (p2 = m_pPieces; p2; p2 = p2->m_pNext)
						{
							if (p2 == p1)
								break;

							if (p2->GetPieceInfo() == pInfo)
							{
								bSkip = true;
								break;
							}
						}

						if (bSkip)
							continue;

						glMatrixMode(GL_PROJECTION);
						glLoadIdentity();
						gluPerspective(30.0f, aspect, 1.0f, 50.0f);
						glMatrixMode(GL_MODELVIEW);
						glLoadIdentity();
						glDepthFunc(GL_LEQUAL);
						glClearColor(1,1,1,1); 
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
						glEnable(GL_COLOR_MATERIAL);
						glDisable (GL_DITHER);
						glShadeModel(GL_FLAT);
						pInfo->ZoomExtents(30.0f, aspect);

						float pos[4] = { 0, 0, 10, 0 };
						glLightfv(GL_LIGHT0, GL_POSITION, pos);
						glEnable(GL_LIGHTING);
						glEnable(GL_LIGHT0);
						glEnable(GL_DEPTH_TEST);
						pInfo->RenderPiece(m_nCurColor);
						glFinish();

						Image image;
            image.FromOpenGL (cx, cy);

						sprintf(fn, "%s%s%s", opts.path, pInfo->m_strName, ext);
						image.FileSave (fn, &opts.imdlg.imopts);
					}
					Sys_FinishMemoryRender (render);
				}
        main_window->EndWait ();
			}
		} break;

		// Export to POV-Ray, swap X & Y from our cs to work with LGEO.
		case LC_FILE_POVRAY:
		{
			LC_POVRAYDLG_OPTS opts;
			if (!SystemDoDialog(LC_DLG_POVRAY, &opts))
				break;

//	CWaitCursor wc;
			char fn[LC_MAXPATH], tmp[10], *ptr;
			unsigned long u;
			PieceInfo* pInfo;
			Piece* pPiece;
			FILE* f;
			char *conv = (char*)malloc (9*lcGetPiecesLibrary()->GetPieceCount ());
			char *flags = (char*)malloc (lcGetPiecesLibrary()->GetPieceCount ());
			memset (conv, 0, 9*lcGetPiecesLibrary()->GetPieceCount());
			memset (flags, 0, lcGetPiecesLibrary()->GetPieceCount());

			// read LGEO conversion table
			if (strlen (opts.libpath))
			{
				strcpy (fn, opts.libpath);
				strcat (fn, "l2p_elmt.tab");
				f = fopen(fn, "rb");

				if (f == NULL)
				{
					free (conv);
					free (flags);
					SystemDoMessageBox("Could not find LGEO.", LC_MB_OK|LC_MB_ICONERROR);
					return;
				}

				unsigned char bt[4];
				while (fread (&bt, 4, 1, f))
				{
					u = (((unsigned char)(bt[3])|((unsigned short)(bt[2]) << 8))|
						(((unsigned long)(bt[1])) << 16)) + bt[0] * 16581375;
					sprintf(tmp, "%d", (int)u);
					pInfo = lcGetPiecesLibrary()->FindPieceInfo(tmp);

					fread(&tmp, 9, 1, f);
					if (tmp[8] != 0)
						fread(&tmp[9], 1, 1, f);

					if (pInfo != NULL)
					{
						int idx = lcGetPiecesLibrary()->GetPieceIndex (pInfo);
						memcpy (&conv[idx*9], &tmp[1], 9);
						flags[idx] = tmp[0];
					}
				}
				fclose (f);

				strcpy(fn, opts.libpath);
				strcat(fn, "l2p_ptrn.tab");
				f = fopen(fn, "rb");

				if (f == NULL)
				{
					SystemDoMessageBox("Could not find LGEO.", LC_MB_OK|LC_MB_ICONERROR);
					free (conv);
					free (flags);
					return;
				}

				u = 0;
				do 
				{
					if ((tmp[u] == 0) && (u != 0))
					{
						u = 0;
						pInfo = lcGetPiecesLibrary()->FindPieceInfo(tmp);
						fread(&tmp, 8, 1, f);

						if (pInfo != NULL)
						{
							int idx = lcGetPiecesLibrary()->GetPieceIndex (pInfo);
							memcpy(&conv[idx*9], tmp, 9);
						}
					}
					else
						u++;
				}	
				while (fread(&tmp[u], 1, 1, f));
				fclose(f);
			}

			strcpy(fn, opts.outpath);
			if ((ptr = strrchr (fn, '.')))
				*ptr = 0;
			strcat (fn, ".inc");
			f = fopen(fn, "wt");

			if (!f)
			{
				SystemDoMessageBox("Could not open file for writing.", LC_MB_OK|LC_MB_ICONERROR);
				break;
			}

			const char* OldLocale = setlocale(LC_NUMERIC, "C");
			fputs("// Stuff that doesn't need to be changed\n\n", f);

			if (strlen(opts.libpath))
				fputs("#include \"lg_color.inc\"\n#include \"lg_defs.inc\"\n\n", f);

			// Add include files
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				Piece* pNext;

				for (pNext = m_pPieces; pNext; pNext = pNext->m_pNext)
				{
					pInfo = pNext->GetPieceInfo();

					if (pNext == pPiece)
					{
						int idx = lcGetPiecesLibrary()->GetPieceIndex (pInfo);
						char pat[] = "patterns/";
						if (conv[idx*9+1] != 'p')
							strcpy(pat, "");

						if (conv[idx*9] != 0)
							fprintf(f, "#include \"%s%s.inc\"\n", pat, &conv[idx*9]);
						break;
					}

					if (pInfo == pPiece->GetPieceInfo())
						break;
				}
			}

			const char* lg_colors[28] = { "red", "Orange", "green", "mint", "blue", "LightBlue", "yellow", 
				"white", "dark_grey", "black", "brown", "pink", "purple", "gold_chrome", "clear_red",
				"clear_neon_orange", "clear_green", "clear_neon_yellow", "clear_blue", "clear_cyan", 
				"clear_yellow", "clear", "grey", "tan", "LightBrown", "rose", "Turquoise", "chrome" };

			const float mycol[4][4] = { { 1.0f, 0.5f, 0.2f, 1 }, { 0.2f, 0.4f, 0.9f, 5 }, 
				{ 0.6f, 0.4f, 0.4f, 24 }, { 0.1f, 0.7f, 0.8f, 26 }};

			if (strlen(opts.libpath))
			{
				for (u = 0; u < 4; u++)
					fprintf(f, "\n#declare lg_%s = texture {\n pigment { rgb <%.2f, %.2f, %.2f> }\n finish {\n  ambient 0.1\n  phong 0.3\n  phong_size 20\n }\n}\n",
						altcolornames[(int)mycol[u][3]], mycol[u][0], mycol[u][1], mycol[u][2]);
			}
			else
			{
				fputs("#include \"colors.inc\"\n\n", f);

				for (u = 0; u < LC_MAXCOLORS; u++)
					fprintf(f, "\n#declare lg_%s = texture {\n pigment { rgbf <%.2f, %.2f, %.2f, %.2f> }\n finish {\n  ambient 0.1\n  phong 0.3\n  phong_size 20\n }\n}\n",
						lg_colors[u], (float)ColorArray[u][0]/255, (float)ColorArray[u][1]/255, (float)ColorArray[u][2]/255, ((ColorArray[u][3] == 255) ? 0.0f : 0.9f));
			}

			// if not in lgeo, create it
			fputs("\n// The next objects (if any) were generated by LeoCAD.\n\n", f);
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				pInfo = pPiece->GetPieceInfo();
				int idx = lcGetPiecesLibrary()->GetPieceIndex (pInfo);
				if (conv[idx*9] != 0)
					continue;

				for (Piece* pNext = m_pPieces; pNext; pNext = pNext->m_pNext)
				{
					if (pNext == pPiece)
					{
						char name[20];
						strcpy(name, pInfo->m_strName);
						while ((ptr = strchr(name, '-')))
							*ptr = '_';
						fprintf(f, "#declare lc_%s = union {\n", name);

						unsigned short g;
						for (g = 0; g < pInfo->m_nGroupCount; g++)
						if (pInfo->m_nFlags & LC_PIECE_LONGDATA_INDICES)
						{
							unsigned long* info = (unsigned long*)pInfo->m_pGroups[g].drawinfo;
							unsigned long count, curcolor, colors = *info;
							info++;

							while (colors--)
							{
								curcolor = *info;
								info++;

								// skip if color only have lines
								if ((*info == 0) && (info[1] == 0))
								{
									info += 2;
									info += *info + 1;
									continue;
								}

								fputs(" mesh {\n", f);

								for (count = *info, info++; count; count -= 4)
								{
									fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
										-pInfo->m_fVertexArray[info[0]*3+1], -pInfo->m_fVertexArray[info[0]*3], pInfo->m_fVertexArray[info[0]*3+2],
										-pInfo->m_fVertexArray[info[1]*3+1], -pInfo->m_fVertexArray[info[1]*3], pInfo->m_fVertexArray[info[1]*3+2],
										-pInfo->m_fVertexArray[info[2]*3+1], -pInfo->m_fVertexArray[info[2]*3], pInfo->m_fVertexArray[info[2]*3+2]);
									fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
										-pInfo->m_fVertexArray[info[2]*3+1], -pInfo->m_fVertexArray[info[2]*3], pInfo->m_fVertexArray[info[2]*3+2],
										-pInfo->m_fVertexArray[info[3]*3+1], -pInfo->m_fVertexArray[info[3]*3], pInfo->m_fVertexArray[info[3]*3+2],
										-pInfo->m_fVertexArray[info[0]*3+1], -pInfo->m_fVertexArray[info[0]*3], pInfo->m_fVertexArray[info[0]*3+2]);
									info += 4;
								}

								for (count = *info, info++; count; count -= 3)
								{
									fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
										-pInfo->m_fVertexArray[info[0]*3+1], -pInfo->m_fVertexArray[info[0]*3], pInfo->m_fVertexArray[info[0]*3+2],
										-pInfo->m_fVertexArray[info[1]*3+1], -pInfo->m_fVertexArray[info[1]*3], pInfo->m_fVertexArray[info[1]*3+2],
										-pInfo->m_fVertexArray[info[2]*3+1], -pInfo->m_fVertexArray[info[2]*3], pInfo->m_fVertexArray[info[2]*3+2]);
									info += 3;
								}
								info += *info + 1;

								if (curcolor != LC_COL_DEFAULT && curcolor != LC_COL_EDGES)
									fprintf (f, "  texture { lg_%s }\n", lg_colors[curcolor]);
								fputs(" }\n", f);
							}
						}
						else
						{
							unsigned short* info = (unsigned short*)pInfo->m_pGroups[g].drawinfo;
							unsigned short count, curcolor, colors = *info;
							info++;

							while (colors--)
							{
								curcolor = *info;
								info++;

								// skip if color only have lines
								if ((*info == 0) && (info[1] == 0))
								{
									info += 2;
									info += *info + 1;
									continue;
								}

								fputs(" mesh {\n", f);

								for (count = *info, info++; count; count -= 4)
								{
									fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
										-pInfo->m_fVertexArray[info[0]*3+1], -pInfo->m_fVertexArray[info[0]*3], pInfo->m_fVertexArray[info[0]*3+2],
										-pInfo->m_fVertexArray[info[1]*3+1], -pInfo->m_fVertexArray[info[1]*3], pInfo->m_fVertexArray[info[1]*3+2],
										-pInfo->m_fVertexArray[info[2]*3+1], -pInfo->m_fVertexArray[info[2]*3], pInfo->m_fVertexArray[info[2]*3+2]);
									fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
										-pInfo->m_fVertexArray[info[2]*3+1], -pInfo->m_fVertexArray[info[2]*3], pInfo->m_fVertexArray[info[2]*3+2],
										-pInfo->m_fVertexArray[info[3]*3+1], -pInfo->m_fVertexArray[info[3]*3], pInfo->m_fVertexArray[info[3]*3+2],
										-pInfo->m_fVertexArray[info[0]*3+1], -pInfo->m_fVertexArray[info[0]*3], pInfo->m_fVertexArray[info[0]*3+2]);
									info += 4;
								}

								for (count = *info, info++; count; count -= 3)
								{
									fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
										-pInfo->m_fVertexArray[info[0]*3+1], -pInfo->m_fVertexArray[info[0]*3], pInfo->m_fVertexArray[info[0]*3+2],
										-pInfo->m_fVertexArray[info[1]*3+1], -pInfo->m_fVertexArray[info[1]*3], pInfo->m_fVertexArray[info[1]*3+2],
										-pInfo->m_fVertexArray[info[2]*3+1], -pInfo->m_fVertexArray[info[2]*3], pInfo->m_fVertexArray[info[2]*3+2]);
									info += 3;
								}
								info += *info + 1;

								if (curcolor != LC_COL_DEFAULT && curcolor != LC_COL_EDGES)
									fprintf (f, "  texture { lg_%s }\n", lg_colors[curcolor]);
								fputs(" }\n", f);
							}
						}

						fputs("}\n\n", f);
						break;
					}

					if (pNext->GetPieceInfo() == pInfo)
						break;
				}
			}

			fclose(f);
			if ((ptr = strrchr (fn, '.')))
				*ptr = 0;
			strcat (fn, ".pov");
			f = fopen(fn, "wt");

			if (!f)
			{
				SystemDoMessageBox("Could not open file for writing.", LC_MB_OK|LC_MB_ICONERROR);
				setlocale(LC_NUMERIC, OldLocale);
				break;
			}

			if ((ptr = strrchr (fn, '.')))
				*ptr = 0;
			ptr = strrchr (fn, '\\');
			if (!ptr)
				ptr = strrchr (fn, '/');
			if (ptr)
				ptr++;
			else
				ptr = fn;

			fprintf(f, "// File created by LeoCAD\n//\n\n#include \"%s.inc\"\n", ptr);
			float eye[3], target[3], up[3];
			m_ActiveView->m_Camera->GetEyePos (eye);
			m_ActiveView->m_Camera->GetTargetPos (target);
			m_ActiveView->m_Camera->GetUpVec (up);

			fprintf(f, "\ncamera {\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f\n}\n\n",
				up[0], up[1], up[2], eye[1], eye[0], eye[2], target[1], target[0], target[2], m_ActiveView->m_Camera->m_fovy);
			fprintf(f, "background { color rgb <%1g, %1g, %1g> }\n\nlight_source { <0, 0, 20> White shadowless }\n\n",
				m_fBackground[0], m_fBackground[1], m_fBackground[2]);

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				float fl[12], pos[3], rot[4];
				char name[20];
				int idx = lcGetPiecesLibrary()->GetPieceIndex (pPiece->GetPieceInfo ());

				if (conv[idx*9] == 0)
				{
					char* ptr;
					sprintf(name, "lc_%s", pPiece->GetPieceInfo()->m_strName);
					while ((ptr = strchr(name, '-')))
						*ptr = '_';
				}
				else
				{
					strcpy (name, &conv[idx*9]);
					if (pPiece->IsTransparent())
						strcat(name, "_clear");			
				}

				pPiece->GetPosition(pos);
				pPiece->GetRotation(rot);
				Matrix mat(rot, pos);
				mat.ToLDraw(fl);

				// Slope needs to be handled correctly
				if (flags[idx] == 1)
					fprintf (f, "merge {\n object {\n  %s\n  texture { lg_%s }\n }\n"
						 " object {\n  %s_slope\n  texture { lg_%s normal { bumps 0.3 scale 0.02 } }\n }\n"
						 " matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
						name, lg_colors[pPiece->GetColor()], &conv[idx*9], lg_colors[pPiece->GetColor()],
						 -fl[11], -fl[5], fl[8], -fl[9], -fl[3], fl[6],
						 -fl[10], -fl[4], fl[7], pos[1], pos[0], pos[2]);
				else
					fprintf(f, "object {\n %s\n texture { lg_%s }\n matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
						name, lg_colors[pPiece->GetColor()], -fl[11], -fl[5], fl[8], -fl[9], -fl[3], fl[6],
						-fl[10], -fl[4], fl[7], pos[1], pos[0], pos[2]);
			}
			fclose (f);
			free (conv);
			free (flags);
			setlocale(LC_NUMERIC, OldLocale);

			if (opts.render)
			{
#ifdef LC_WINDOWS
				// TODO: Linux support
				char buf[600];
				char tmp[LC_MAXPATH], out[LC_MAXPATH];
				sprintf(out, "%s.pov", fn);
				GetShortPathName(out, out, LC_MAXPATH);
				GetShortPathName(opts.libpath, tmp, LC_MAXPATH);
				sprintf (buf, "+L%s +I%s", tmp, out);
				ptr = strrchr (out, '\\');
				if (ptr)
					*(ptr+1) = 0;
				ShellExecute(::GetDesktopWindow(), "open", opts.povpath, buf, out, SW_SHOWNORMAL);
#endif
			}
		} break;

		case LC_FILE_WAVEFRONT:
		{
			char filename[LC_MAXPATH];
			if (!SystemDoDialog(LC_DLG_WAVEFRONT, filename))
				break;

			char buf[LC_MAXPATH], *ptr;
			FILE* stream = fopen(filename, "wt");
			unsigned long vert = 1, i;
			Piece* pPiece;

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
			fputs("# Model exported from LeoCAD\n", stream);
			if (strlen(buf) != 0)
				fprintf(stream,"# Original name: %s\n", ptr);
			if (strlen(m_strAuthor))
				fprintf(stream, "# Author: %s\n", m_strAuthor);

			strcpy(buf, filename);
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

			fprintf(stream, "#\n\nmtllib %s\n\n", ptr);

			FILE* mat = fopen(buf, "wt");
			fputs("# Colors used by LeoCAD\n# You need to add transparency values\n#\n\n", mat);
			for (i = 0; i < LC_MAXCOLORS; i++)
				fprintf(mat, "newmtl %s\nKd %.2f %.2f %.2f\n\n", altcolornames[i], (float)FlatColorArray[i][0]/255, (float)FlatColorArray[i][1]/255, (float)FlatColorArray[i][2]/255);
			fclose(mat);

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				float pos[3], rot[4], tmp[3];
				pPiece->GetPosition(pos);
				pPiece->GetRotation(rot);
				Matrix mat(rot, pos);
				PieceInfo* pInfo = pPiece->GetPieceInfo();

				for (i = 0; i < pInfo->m_nVertexCount*3; i += 3)
				{
					mat.TransformPoint(tmp, &pInfo->m_fVertexArray[i]);
					fprintf(stream, "v %.2f %.2f %.2f\n", tmp[0], tmp[1], tmp[2]);
				}
				fputs("#\n\n", stream);
			}

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				strcpy(buf, pPiece->GetName());
				for (i = 0; i < strlen(buf); i++)
					if ((buf[i] == '#') || (buf[i] == ' '))
						buf[i] = '_';

				fprintf(stream, "g %s\n", buf);
				pPiece->GetPieceInfo()->WriteWavefront(stream, pPiece->GetColor(), &vert);
			}

			fclose(stream);
		} break;

		case LC_FILE_PROPERTIES:
		{
			LC_PROPERTIESDLG_OPTS opts;

			opts.strTitle = m_strTitle;
			strcpy(opts.strAuthor, m_strAuthor);
			strcpy(opts.strDescription, m_strDescription);
			strcpy(opts.strComments, m_strComments);
			opts.strFilename = m_strPathName;

			opts.lines = lcGetPiecesLibrary()->GetPieceCount();
			opts.count = (unsigned short*)malloc(lcGetPiecesLibrary()->GetPieceCount()*LC_MAXCOLORS*sizeof(unsigned short));
			memset (opts.count, 0, lcGetPiecesLibrary()->GetPieceCount()*LC_MAXCOLORS*sizeof(unsigned short));
			opts.names = (char**)malloc(lcGetPiecesLibrary()->GetPieceCount()*sizeof(char*));
			for (int i = 0; i < lcGetPiecesLibrary()->GetPieceCount(); i++)
				opts.names[i] = lcGetPiecesLibrary()->GetPieceInfo (i)->m_strDescription;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				int idx = lcGetPiecesLibrary()->GetPieceIndex (pPiece->GetPieceInfo ());
				opts.count[idx*LC_MAXCOLORS+pPiece->GetColor()]++;
			}

			if (SystemDoDialog(LC_DLG_PROPERTIES, &opts))
			{
				if (strcmp(m_strAuthor, opts.strAuthor) ||
					strcmp(m_strDescription, opts.strDescription) ||
					strcmp(m_strComments, opts.strComments))
				{
					strcpy(m_strAuthor, opts.strAuthor);
					strcpy(m_strDescription, opts.strDescription);
					strcpy(m_strComments, opts.strComments);
					SetModifiedFlag(true);
				}
			}

			free(opts.count);
			free(opts.names);
		} break;

		case LC_FILE_TERRAIN:
		{
			Terrain* temp = new Terrain();
			*temp = *m_pTerrain;

			if (SystemDoDialog(LC_DLG_TERRAIN, temp))
			{
				*m_pTerrain = *temp;
				m_pTerrain->LoadTexture(true);
			}
			delete temp;
		} break;

		case LC_FILE_LIBRARY:
		{
			FileMem file;
			FileSave(&file, true);

			if (SystemDoDialog(LC_DLG_LIBRARY, NULL))
			{
				if (lcGetPiecesLibrary()->m_Modified)
				{
					DeleteContents(true);
					FileLoad(&file, true, false);
				}
			}
		} break;

		case LC_FILE_RECENT:
		{
			OpenProject(main_window->GetMRU(nParam));
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

			SystemUpdateUndoRedo(m_pUndoList->pNext ? m_pUndoList->strText : NULL, m_pRedoList ? m_pRedoList->strText : NULL);
		} break;

		case LC_EDIT_CUT:
		case LC_EDIT_COPY:
		{
			if (m_pClipboard[m_nCurClipboard] != NULL)
				delete m_pClipboard[m_nCurClipboard];
			m_pClipboard[m_nCurClipboard] = new FileMem;

			int i = 0;
			Piece* pPiece;
			Camera* pCamera;
			Group* pGroup;
//			Light* pLight;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
					i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
					pPiece->FileSave(*m_pClipboard[m_nCurClipboard], m_pGroups);

			for (i = 0, pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				pGroup->FileSave(m_pClipboard[m_nCurClipboard], m_pGroups);

			for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
				if (pCamera->IsSelected())
					i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
				if (pCamera->IsSelected())
					pCamera->FileSave(*m_pClipboard[m_nCurClipboard]);
/*
			for (i = 0, pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsSelected())
					i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsSelected())
					pLight->FileSave(m_pClipboard[m_nCurClipboard]);
*/
			if (id == LC_EDIT_CUT)
			{
				RemoveSelectedObjects();
				SystemUpdateFocus(NULL);
				UpdateSelection();
				UpdateAllViews ();
				SetModifiedFlag(true);
				CheckPoint("Cutting");
			}
			SystemExportClipboard(m_pClipboard[m_nCurClipboard]);
			SystemUpdatePaste(true);
		} break;

		case LC_EDIT_PASTE:
		{
			int i, j;
			Piece* pPasted = NULL;
			File* file = m_pClipboard[m_nCurClipboard];
			if (file == NULL)
				break;
			file->Seek(0, SEEK_SET);
			SelectAndFocusNone(false);

			file->Read(&i, sizeof(i));
			while (i--)
			{
				char name[LC_PIECE_NAME_LEN];
				Piece* pPiece = new Piece(NULL);
				pPiece->FileLoad(*file, name);
				PieceInfo* pInfo = lcGetPiecesLibrary()->FindPieceInfo(name);
				if (pInfo)
				{
					pPiece->SetPieceInfo(pInfo);
					pPiece->m_pNext = pPasted;
					pPasted = pPiece;
				}
				else 
					delete pPiece;
			}

			file->Read(&i, sizeof(i));
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

			Camera* pCamera = m_pCameras;
			while (pCamera->m_pNext)
				pCamera = pCamera->m_pNext;
			file->Read(&i, sizeof(i));

			while (i--)
			{
				pCamera = new Camera(8, pCamera);
				pCamera->FileLoad(*file);
				pCamera->Select(true, false, false);
				pCamera->GetTarget ()->Select(true, false, false);
			}

			// TODO: lights
			CalculateStep();
			SetModifiedFlag(true);
			CheckPoint("Pasting");
			SystemUpdateFocus(NULL);
			UpdateSelection();
			UpdateAllViews ();
		} break;

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
                        messenger->Dispatch (LC_MSG_FOCUS_CHANGED, NULL);
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

                        messenger->Dispatch (LC_MSG_FOCUS_CHANGED, NULL);
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_EDIT_SELECT_BYNAME:
		{
			Piece* pPiece;
			Camera* pCamera;
			Light* pLight;
			Group* pGroup;
			int i = 0;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
					i++;

			for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
				if (pCamera != m_ActiveView->m_Camera)
					if (pCamera->IsVisible())
						i++;

			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsVisible())
					i++;

			// TODO: add only groups with visible pieces
			for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				i++;

			if (i == 0)
			{
				// TODO: say 'Nothing to select'
				break;
			}

			LC_SEL_DATA* opts = (LC_SEL_DATA*)malloc((i+1)*sizeof(LC_SEL_DATA));
			i = 0;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
				{
					opts[i].name = pPiece->GetName();
					opts[i].type = LC_SELDLG_PIECE;
					opts[i].selected = pPiece->IsSelected();
					opts[i].pointer = pPiece;
					i++;
				}

			for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
				if (pCamera != m_ActiveView->m_Camera)
					if (pCamera->IsVisible())
					{
						opts[i].name = pCamera->GetName();
						opts[i].type = LC_SELDLG_CAMERA;
						opts[i].selected = pCamera->IsSelected();
						opts[i].pointer = pCamera;
						i++;
					}

			for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
				if (pLight->IsVisible())
				{
					opts[i].name = pLight->GetName();
					opts[i].type = LC_SELDLG_LIGHT;
					opts[i].selected = pLight->IsSelected();
					opts[i].pointer = pLight;
					i++;
				}

			// TODO: add only groups with visible pieces
			for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
			{
				opts[i].name = pGroup->m_strName;
				opts[i].type = LC_SELDLG_GROUP;
				// TODO: check if selected
				opts[i].selected = false;
				opts[i].pointer = pGroup;
				i++;
			}
			opts[i].pointer = NULL;

			if (SystemDoDialog(LC_DLG_SELECTBYNAME, opts))
			{
				SelectAndFocusNone(false);

				for (i = 0; opts[i].pointer != NULL; i++)
				{
					if (!opts[i].selected)
						continue;

					switch (opts[i].type)
					{
						case LC_SELDLG_PIECE:
						{
							((Piece*)opts[i].pointer)->Select(true, false, false);
						} break;

						case LC_SELDLG_CAMERA:
						{
							((Camera*)opts[i].pointer)->Select(true, false, false);
						} break;

						case LC_SELDLG_LIGHT:
						{
							((Light*)opts[i].pointer)->Select(true, false, false);
						} break;

						case LC_SELDLG_GROUP:
						{
							pGroup = (Group*)opts[i].pointer;
							pGroup = pGroup->GetTopGroup();
							for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
								if (pPiece->GetTopGroup() == pGroup)
									pPiece->Select(true, false, false);
						} break;
					}
				}

				UpdateSelection();
				UpdateAllViews();
//	pFrame->UpdateInfo();
			}

			free(opts);
		} break;

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
				Vector3 Pos;
				Vector4 Rot;

				GetPieceInsertPosition(pLast, Pos, Rot);

				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_nCurStep, m_nCurFrame, m_nCurColor);

				pPiece->ChangeKey(m_nCurStep, false, false, Rot, LC_PK_ROTATION);
				pPiece->ChangeKey(m_nCurFrame, true, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
			}
			else
				pPiece->Initialize(0, 0, 0, m_nCurStep, m_nCurFrame, m_nCurColor);

			SelectAndFocusNone(false);
			pPiece->CreateName(m_pPieces);
			AddPiece(pPiece);
			pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, true, true);
			pPiece->Select (true, true, false);
			messenger->Dispatch (LC_MSG_FOCUS_CHANGED, pPiece);
			UpdateSelection();
			SystemPieceComboAdd(m_pCurPiece->m_strDescription);

			if (m_nSnap & LC_DRAW_MOVE)
				SetAction(LC_ACTION_MOVE);

//			AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)pNew, OT_PIECE);
			UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Inserting");
		} break;

		case LC_PIECE_DELETE:
		{
			if (RemoveSelectedObjects())
			{
                          messenger->Dispatch (LC_MSG_FOCUS_CHANGED, NULL);
				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Deleting");
			}
		} break;

		case LC_PIECE_MINIFIG:
		{
			MinifigWizard Wizard(m_ActiveView);
			int i;

			if (SystemDoDialog(LC_DLG_MINIFIG, &Wizard))
			{
				SelectAndFocusNone(false);

				for (i = 0; i < LC_MFW_NUMITEMS; i++)
				{
					if (Wizard.m_Info[i] == NULL)
						continue;

					Matrix mat;
					Piece* pPiece = new Piece(Wizard.m_Info[i]);

					Vector4& Position = Wizard.m_Matrices[i][3];
					Vector4 Rotation = Wizard.m_Matrices[i].ToAxisAngle();
					Rotation[3] *= LC_RTOD;
					pPiece->Initialize(Position[0], Position[1], Position[2], m_nCurStep, m_nCurFrame, Wizard.m_Colors[i]);
					pPiece->CreateName(m_pPieces);
					AddPiece(pPiece);
					pPiece->Select(true, false, false);

					pPiece->ChangeKey(1, false, false, Rotation, LC_PK_ROTATION);
					pPiece->ChangeKey(1, true, false, Rotation, LC_PK_ROTATION);
					pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
					pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);

					SystemPieceComboAdd(Wizard.m_Info[i]->m_strDescription);
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

				messenger->Dispatch (LC_MSG_FOCUS_CHANGED, NULL);
				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Minifig");
			}
		} break;

		case LC_PIECE_ARRAY:
		{
			LC_ARRAYDLG_OPTS opts;

			if (SystemDoDialog(LC_DLG_ARRAY, &opts))
			{
				int total;
				
				total = opts.n1DCount;
				if (opts.nArrayDimension > 0)
					total *= opts.n2DCount;
				if (opts.nArrayDimension > 1)
					total *= opts.n3DCount;

				if (total < 2)
				{
					SystemDoMessageBox("Array only have 1 element.", LC_MB_OK|LC_MB_ICONWARNING);
					break;
				}

				if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
				{
					opts.f2D[0] *= 0.08f;
					opts.f2D[1] *= 0.08f;
					opts.f2D[2] *= 0.08f;
					opts.f3D[0] *= 0.08f;
					opts.f3D[1] *= 0.08f;
					opts.f3D[2] *= 0.08f;
					opts.fMove[0] *= 0.08f;
					opts.fMove[1] *= 0.08f;
					opts.fMove[2] *= 0.08f;
				}

				Piece *pPiece, *pFirst = NULL, *pLast = NULL;
				float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
				int sel = 0;
				unsigned long i, j, k;

				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
					if (pPiece->IsSelected())
					{
						pPiece->CompareBoundingBox(bs);
						sel++;
					}

				for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				{
					if (!pPiece->IsSelected())
						continue;

					for (i = 0; i < opts.n1DCount; i++)
					{
						float pos[3], param[4];
						pPiece->GetRotation(param);
						pPiece->GetPosition(pos);
						Matrix mat(param, pos);

						if (sel == 1)
						{
							mat.GetTranslation(pos);
							mat.Rotate(i*opts.fRotate[0], 1, 0, 0);
							mat.Rotate(i*opts.fRotate[1], 0, 1, 0);
							mat.Rotate(i*opts.fRotate[2], 0, 0, 1);
							mat.SetTranslation(pos);
						}
						else
						{
							mat.RotateCenter(i*opts.fRotate[0],1,0,0,(bs[0]+bs[3])/2,(bs[1]+bs[4])/2,(bs[2]+bs[5])/2);
							mat.RotateCenter(i*opts.fRotate[1],0,1,0,(bs[0]+bs[3])/2,(bs[1]+bs[4])/2,(bs[2]+bs[5])/2);
							mat.RotateCenter(i*opts.fRotate[2],0,0,1,(bs[0]+bs[3])/2,(bs[1]+bs[4])/2,(bs[2]+bs[5])/2);
						}
						mat.ToAxisAngle(param);
						mat.GetTranslation(pos);

						if (i != 0)
						{
							if (pLast)
							{
								pLast->m_pNext = new Piece(pPiece->GetPieceInfo());
								pLast = pLast->m_pNext;
							}
							else
								pLast = pFirst = new Piece(pPiece->GetPieceInfo());

							pLast->Initialize(pos[0]+i*opts.fMove[0], pos[1]+i*opts.fMove[1], pos[2]+i*opts.fMove[2], 
								m_nCurStep, m_nCurFrame, pPiece->GetColor());
							pLast->ChangeKey(1, false, false, param, LC_PK_ROTATION);
							pLast->ChangeKey(1, true, false, param, LC_PK_ROTATION);
						}

						if (opts.nArrayDimension == 0)
							continue;
						
						for (j = 0; j < opts.n2DCount; j++)
						{
							if (j != 0)
							{
								if (pLast)
								{
									pLast->m_pNext = new Piece(pPiece->GetPieceInfo());
									pLast = pLast->m_pNext;
								}
								else
									pLast = pFirst = new Piece(pPiece->GetPieceInfo());

								pLast->Initialize(pos[0]+i*opts.fMove[0]+j*opts.f2D[0], pos[1]+i*opts.fMove[1]+j*opts.f2D[1], pos[2]+i*opts.fMove[2]+j*opts.f2D[2],
									m_nCurStep, m_nCurFrame, pPiece->GetColor());
								pLast->ChangeKey(1, false, false, param, LC_PK_ROTATION);
								pLast->ChangeKey(1, true, false, param, LC_PK_ROTATION);
							}

							if (opts.nArrayDimension == 1)
								continue;

							for (k = 1; k < opts.n3DCount; k++)
							{
								if (pLast)
								{
									pLast->m_pNext = new Piece(pPiece->GetPieceInfo());
									pLast = pLast->m_pNext;
								}
								else
									pLast = pFirst = new Piece(pPiece->GetPieceInfo());

								pLast->Initialize(pos[0]+i*opts.fMove[0]+j*opts.f2D[0]+k*opts.f3D[0], pos[1]+i*opts.fMove[1]+j*opts.f2D[1]+k*opts.f3D[1], pos[2]+i*opts.fMove[2]+j*opts.f2D[2]+k*opts.f3D[2],
									m_nCurStep, m_nCurFrame, pPiece->GetColor());
								pLast->ChangeKey(1, false, false, param, LC_PK_ROTATION);
								pLast->ChangeKey(1, true, false, param, LC_PK_ROTATION);
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
					pFirst->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);
					pFirst = pPiece;
				}

				SelectAndFocusNone(true);
//				SystemUpdateFocus(NULL);
				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Array");
			}
		} break;

		case LC_PIECE_COPYKEYS:
		{
			float move[3], rot[4];
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					pPiece->CalculateSingleKey (m_bAnimation ? m_nCurStep : m_nCurFrame, !m_bAnimation, LC_PK_POSITION, move);
					pPiece->CalculateSingleKey (m_bAnimation ? m_nCurStep : m_nCurFrame, !m_bAnimation, LC_PK_ROTATION, rot);
					pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, move, LC_PK_POSITION);
					pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, rot, LC_PK_ROTATION);
				}

			// TODO: cameras and lights

			CalculateStep();
			UpdateAllViews();
		} break;

		case LC_PIECE_GROUP:
		{
			Group* pGroup;
			int i, max = 0;
			char name[65];

			for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				if (strncmp (pGroup->m_strName, "Group #", 7) == 0)
					if (sscanf(pGroup->m_strName, "Group #%d", &i) == 1)
						if (i > max)
							max = i;
			sprintf(name, "Group #%.2d", max+1);

			if (SystemDoDialog(LC_DLG_GROUP, name))
			{
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
			}
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
			int i;
			Group* pGroup;
			Piece* pPiece;
			LC_GROUPEDITDLG_OPTS opts;

			for (i = 0, pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				i++;
			opts.piececount = i;
			opts.pieces = (Piece**)malloc(i*sizeof(Piece*));
			opts.piecesgroups = (Group**)malloc(i*sizeof(Group*));

			for (i = 0, pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext, i++)
			{
				opts.pieces[i] = pPiece;
				opts.piecesgroups[i] = pPiece->GetGroup();
			}

			for (i = 0, pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				i++;
			opts.groupcount = i;
			opts.groups = (Group**)malloc(i*sizeof(Group*));
			opts.groupsgroups = (Group**)malloc(i*sizeof(Group*));

			for (i = 0, pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext, i++)
			{
				opts.groups[i] = pGroup;
				opts.groupsgroups[i] = pGroup->m_pGroup;
			}

			if (SystemDoDialog(LC_DLG_EDITGROUPS, &opts))
			{
				for (i = 0; i < opts.piececount; i++)
					opts.pieces[i]->SetGroup(opts.piecesgroups[i]);

				for (i = 0; i < opts.groupcount; i++)
					opts.groups[i]->m_pGroup = opts.groupsgroups[i];

				RemoveEmptyGroups();
				SelectAndFocusNone(false);
				SystemUpdateFocus(NULL);
				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Editing");
			}

			free(opts.pieces);
			free(opts.piecesgroups);
			free(opts.groups);
			free(opts.groupsgroups);
		} break;

		case LC_PIECE_HIDE_SELECTED:
		{
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
					pPiece->Hide();
			UpdateSelection();
			messenger->Dispatch (LC_MSG_FOCUS_CHANGED, NULL);
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

		case LC_PIECE_PREVIOUS:
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

		case LC_PIECE_NEXT:
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
			LC_PREFERENCESDLG_OPTS opts;
			opts.nMouse = m_nMouse;
			opts.nSaveInterval = m_nAutosave;
			strcpy(opts.strUser, Sys_ProfileLoadString ("Default", "User", ""));
			strcpy(opts.strPath, m_strModelsPath);
			opts.nDetail = m_nDetail;
			opts.fLineWidth = m_fLineWidth;
			opts.nSnap = m_nSnap;
			opts.nAngleSnap = m_nAngleSnap;
			opts.nGridSize = m_nGridSize;
			opts.nScene = m_nScene;
			opts.fDensity = m_fFogDensity;
			strcpy(opts.strBackground, m_strBackground);
			memcpy(opts.fBackground, m_fBackground, sizeof(m_fBackground));
			memcpy(opts.fFog, m_fFogColor, sizeof(m_fFogColor));
			memcpy(opts.fAmbient, m_fAmbient, sizeof(m_fAmbient));
			memcpy(opts.fGrad1, m_fGradient1, sizeof(m_fGradient1));
			memcpy(opts.fGrad2, m_fGradient2, sizeof(m_fGradient2));
			strcpy(opts.strFooter, m_strFooter);
			strcpy(opts.strHeader, m_strHeader);

			if (SystemDoDialog(LC_DLG_PREFERENCES, &opts))
			{
				m_nMouse = opts.nMouse;
				m_nAutosave = opts.nSaveInterval;
				strcpy(m_strModelsPath, opts.strPath);
				Sys_ProfileSaveString ("Default", "User", opts.strUser);
				m_nDetail = opts.nDetail;
				m_fLineWidth = opts.fLineWidth;
				m_nSnap = opts.nSnap;
				m_nAngleSnap = opts.nAngleSnap;
				m_nGridSize = opts.nGridSize;
				m_nScene = opts.nScene;
				m_fFogDensity = opts.fDensity;
				strcpy(m_strBackground, opts.strBackground);
				memcpy(m_fBackground, opts.fBackground, sizeof(m_fBackground));
				memcpy(m_fFogColor, opts.fFog, sizeof(m_fFogColor));
				memcpy(m_fAmbient, opts.fAmbient, sizeof(m_fAmbient));
				memcpy(m_fGradient1, opts.fGrad1, sizeof(m_fGradient1));
				memcpy(m_fGradient2, opts.fGrad2, sizeof(m_fGradient2));
				strcpy(m_strFooter, opts.strFooter);
				strcpy(m_strHeader, opts.strHeader);
				SystemUpdateSnap(m_nSnap);
				SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);

				for (int i = 0; i < m_ViewList.GetSize (); i++)
				{
					m_ViewList[i]->MakeCurrent ();
					RenderInitialize();
				}

				UpdateAllViews();
			}
		} break;

		case LC_VIEW_ZOOM:
		{
			m_ActiveView->m_Camera->DoZoom(nParam, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMIN:
		{
			m_ActiveView->m_Camera->DoZoom(-1, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMOUT:
		{
			m_ActiveView->m_Camera->DoZoom(1, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMEXTENTS:
		{
			// If the control key is down then zoom all views, otherwise zoom only the active view.
			int FirstView, LastView;

			if (Sys_KeyDown(KEY_CONTROL))
			{
				FirstView = 0;
				LastView = m_ViewList.GetSize();
			}
			else
			{
				FirstView = m_ViewList.FindIndex(m_ActiveView);
				LastView = FirstView + 1;
			}

			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
					pPiece->CompareBoundingBox(bs);

			Vector3 Center((bs[0] + bs[3]) / 2, (bs[1] + bs[4]) / 2, (bs[2] + bs[5]) / 2);

			ObjArray<Vector3> Points;
			Points.Add(Vector3(bs[0], bs[1], bs[5]));
			Points.Add(Vector3(bs[3], bs[1], bs[5]));
			Points.Add(Vector3(bs[0], bs[1], bs[2]));
			Points.Add(Vector3(bs[3], bs[4], bs[5]));
			Points.Add(Vector3(bs[3], bs[4], bs[2]));
			Points.Add(Vector3(bs[0], bs[4], bs[2]));
			Points.Add(Vector3(bs[0], bs[4], bs[5]));
			Points.Add(Vector3(bs[3], bs[1], bs[2]));

			for (int vp = FirstView; vp < LastView; vp++)
			{
				View* view = m_ViewList[vp];

				int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };

				float Aspect = (float)Viewport[2]/(float)Viewport[3];
				Camera* Cam = view->m_Camera;

				Vector3 Position(Cam->GetEyePosition() + Center - Cam->GetTargetPosition());

				Matrix44 ModelView, Projection;
				ModelView.CreateLookAt(Position, Center, Cam->GetUpVector());
				Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

				Position = ZoomExtents(Position, ModelView, Projection, &Points[0], Points.GetSize());

				Cam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Position, LC_CK_EYE);
				Cam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Center, LC_CK_TARGET);
				Cam->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
			}

			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPORTS:
		{
			/*
			// Safety check
			if (m_nActiveViewport >= viewports[nParam].n)
			{
				SystemUpdateCurrentCamera(m_ActiveView->m_Camera, m_ActiveView->m_Camera, m_pCameras);
				m_nActiveViewport = 0;
			}

			SystemUpdateViewport(nParam, m_nViewportMode);
			m_nViewportMode = (unsigned char)nParam;
			UpdateOverlayScale();
			UpdateAllViews();
			*/
		} break;

		case LC_VIEW_STEP_NEXT:
		{
			if (m_bAnimation)
				m_nCurFrame++;
			else
				m_nCurStep++;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			if (m_bAnimation)
				SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				SystemUpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;
		
		case LC_VIEW_STEP_PREVIOUS:
		{
			if (m_bAnimation)
				m_nCurFrame--;
			else
				m_nCurStep--;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			if (m_bAnimation)
				SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				SystemUpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;
		
		case LC_VIEW_STEP_FIRST:
		{
			if (m_bAnimation)
				m_nCurFrame = 1;
			else
				m_nCurStep = 1;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			if (m_bAnimation)
				SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				SystemUpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;

		case LC_VIEW_STEP_LAST:
		{
			if (m_bAnimation)
				m_nCurFrame = m_nTotalFrames;
			else
				m_nCurStep = GetLastStep ();

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			if (m_bAnimation)
				SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				SystemUpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;

		case LC_VIEW_STEP_CHOOSE:
		{
			SystemDoDialog(LC_DLG_STEPCHOOSE, NULL);
			if (m_bAnimation)
				SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				SystemUpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;

		case LC_VIEW_STEP_SET:
		{
			if (m_bAnimation)
				m_nCurFrame = (nParam < m_nTotalFrames) ? (unsigned short)nParam : m_nTotalFrames;
			else
				m_nCurStep = (nParam < 255) ? (unsigned char)nParam : 255;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			if (m_bAnimation)
				SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				SystemUpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;

    case LC_VIEW_STEP_INSERT:
    {
      for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
        pPiece->InsertTime (m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

      for (Camera* pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
        pCamera->InsertTime (m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

      for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
        pLight->InsertTime (m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

      SetModifiedFlag (true);
      if (m_bAnimation)
        CheckPoint ("Adding Frame");
      else
        CheckPoint ("Adding Step");
			CalculateStep ();
      UpdateAllViews ();
      UpdateSelection ();
    } break;

    case LC_VIEW_STEP_DELETE:
    {
      for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
        pPiece->RemoveTime (m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

      for (Camera* pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
        pCamera->RemoveTime (m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

      for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
        pLight->RemoveTime (m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, 1);

      SetModifiedFlag (true);
      if (m_bAnimation)
        CheckPoint ("Removing Frame");
      else
        CheckPoint ("Removing Step");
			CalculateStep ();
      UpdateAllViews ();
      UpdateSelection ();
    } break;

		case LC_VIEW_STOP:
		{
			m_bStopRender = true;
		} break;

		case LC_VIEW_PLAY:
		{ 
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
				SystemUpdateTime(true, m_nCurFrame, m_nTotalFrames);
                UpdateAllViews();
				SystemPumpMessages();
			}
			SystemUpdatePlay(true, false);
			SystemUpdateFocus(NULL);
		} break;

		case LC_VIEW_CAMERA_FRONT:
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_FRONT);
		} break;

		case LC_VIEW_CAMERA_BACK:
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_BACK);
		} break;

		case LC_VIEW_CAMERA_TOP:
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_TOP);
		} break;

		case LC_VIEW_CAMERA_BOTTOM:
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_UNDER);
		} break;

		case LC_VIEW_CAMERA_LEFT:
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_LEFT);
		} break;

		case LC_VIEW_CAMERA_RIGHT:
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_RIGHT);
		} break;

		case LC_VIEW_CAMERA_MAIN:
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_MAIN);
		} break;

		case LC_VIEW_CAMERA_MENU:
		{
			Camera* pCamera = m_pCameras;

			while (nParam--)
				pCamera = pCamera->m_pNext;

			SystemUpdateCurrentCamera(m_ActiveView->m_Camera, pCamera, m_pCameras);
			m_ActiveView->m_Camera = pCamera;
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_CAMERA_RESET:
		{
			Camera* pCamera;
			int i;

			while (m_pCameras)
			{
				pCamera = m_pCameras;
				m_pCameras = m_pCameras->m_pNext;
				delete pCamera;
			}

			for (m_pCameras = pCamera = NULL, i = 0; i < 7; i++)
			{
				pCamera = new Camera(i, pCamera);
				if (m_pCameras == NULL)
					m_pCameras = pCamera;
			}

			for (int ViewIdx = 0; ViewIdx < m_ViewList.GetSize(); ViewIdx++)
				m_ViewList[ViewIdx]->m_Camera = pCamera;

			SystemUpdateCameraMenu(m_pCameras);
			SystemUpdateCurrentCamera(NULL, m_ActiveView->m_Camera, m_pCameras);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Reset Cameras");
		} break;

		case LC_HELP_ABOUT:
		{
		  SystemDoDialog(LC_DLG_ABOUT, 0);
		} break;

		case LC_TOOLBAR_ANIMATION:
		{
			m_bAnimation = !m_bAnimation;

			CalculateStep();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateAnimation(m_bAnimation, m_bAddKeys);
			if (m_bAnimation)
				SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				SystemUpdateTime(m_bAnimation, m_nCurStep, 255);
		} break;
		
		case LC_TOOLBAR_ADDKEYS:
		{
			m_bAddKeys = !m_bAddKeys;
			SystemUpdateAnimation(m_bAnimation, m_bAddKeys);
		} break;

		// Change snap X, Y, Z, All, None or Angle.
		case LC_TOOLBAR_SNAPMENU:
		{
			switch (nParam)
			{
			case 0:
				if (m_nSnap & LC_DRAW_SNAP_X)
					m_nSnap &= ~LC_DRAW_SNAP_X;
				else
					m_nSnap |= LC_DRAW_SNAP_X;
				break;
			case 1:
				if (m_nSnap & LC_DRAW_SNAP_Y)
					m_nSnap &= ~LC_DRAW_SNAP_Y;
				else
					m_nSnap |= LC_DRAW_SNAP_Y;
				break;
			case 2:
				if (m_nSnap & LC_DRAW_SNAP_Z)
					m_nSnap &= ~LC_DRAW_SNAP_Z;
				else
					m_nSnap |= LC_DRAW_SNAP_Z;
				break;
			case 3:
				m_nSnap |= LC_DRAW_SNAP_XYZ;
				break;
			case 4:
				m_nSnap &= ~LC_DRAW_SNAP_XYZ;
				break;
			case 5:
				if (m_nSnap & LC_DRAW_SNAP_A)
					m_nSnap &= ~LC_DRAW_SNAP_A;
				else
					m_nSnap |= LC_DRAW_SNAP_A;
				break;
			case 6:
				if ((m_nSnap & LC_DRAW_SNAP_XYZ) == LC_DRAW_SNAP_XYZ)
					m_nSnap &= ~LC_DRAW_SNAP_XYZ;
				else
					m_nSnap |= LC_DRAW_SNAP_XYZ;
				break;
			}
			SystemUpdateSnap(m_nSnap);
		} break;

		case LC_TOOLBAR_LOCKMENU:
		{
			switch (nParam)
			{
			case 0:
				if (m_nSnap & LC_DRAW_LOCK_X)
					m_nSnap &= ~LC_DRAW_LOCK_X;
				else
					m_nSnap |= LC_DRAW_LOCK_X;
				break;
			case 1:
				if (m_nSnap & LC_DRAW_LOCK_Y)
					m_nSnap &= ~LC_DRAW_LOCK_Y;
				else
					m_nSnap |= LC_DRAW_LOCK_Y;
				break;
			case 2:
				if (m_nSnap & LC_DRAW_LOCK_Z)
					m_nSnap &= ~LC_DRAW_LOCK_Z;
				else
					m_nSnap |= LC_DRAW_LOCK_Z;
				break;
			case 3:
				m_nSnap &= ~LC_DRAW_LOCK_XYZ;
				break;
			case 4:
				if ((m_nSnap & LC_DRAW_LOCK_XYZ) == LC_DRAW_LOCK_XYZ)
					m_nSnap &= ~LC_DRAW_LOCK_XYZ;
				else
					m_nSnap |= LC_DRAW_LOCK_XYZ;
				break;
			}
			SystemUpdateSnap(m_nSnap);
		} break;

		case LC_TOOLBAR_FASTRENDER:
		{
			if (m_nDetail & LC_DET_FAST) 
				m_nDetail &= ~LC_DET_FAST; 
			else
				m_nDetail |= LC_DET_FAST; 
			UpdateAllViews();

			SystemUpdateRenderingMode((m_nDetail & LC_DET_FAST) != 0);
		} break;

		case LC_EDIT_MOVEXY_SNAP_0:
		case LC_EDIT_MOVEXY_SNAP_1:
		case LC_EDIT_MOVEXY_SNAP_2:
		case LC_EDIT_MOVEXY_SNAP_3:
		case LC_EDIT_MOVEXY_SNAP_4:
		case LC_EDIT_MOVEXY_SNAP_5:
		case LC_EDIT_MOVEXY_SNAP_6:
		case LC_EDIT_MOVEXY_SNAP_7:
		case LC_EDIT_MOVEXY_SNAP_8:
		case LC_EDIT_MOVEXY_SNAP_9:
		{
			m_nMoveSnap = (id - LC_EDIT_MOVEXY_SNAP_0) | (m_nMoveSnap & ~0xff);
			if (id != LC_EDIT_MOVEXY_SNAP_0)
				m_nSnap |= LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y;
			else
				m_nSnap &= ~(LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y);
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_MOVEZ_SNAP_0:
		case LC_EDIT_MOVEZ_SNAP_1:
		case LC_EDIT_MOVEZ_SNAP_2:
		case LC_EDIT_MOVEZ_SNAP_3:
		case LC_EDIT_MOVEZ_SNAP_4:
		case LC_EDIT_MOVEZ_SNAP_5:
		case LC_EDIT_MOVEZ_SNAP_6:
		case LC_EDIT_MOVEZ_SNAP_7:
		case LC_EDIT_MOVEZ_SNAP_8:
		case LC_EDIT_MOVEZ_SNAP_9:
		{
			m_nMoveSnap = (((id - LC_EDIT_MOVEZ_SNAP_0) << 8) | (m_nMoveSnap & ~0xff00));
			if (id != LC_EDIT_MOVEZ_SNAP_0)
				m_nSnap |= LC_DRAW_SNAP_Z;
			else
				m_nSnap &= ~LC_DRAW_SNAP_Z;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_0:
		case LC_EDIT_ANGLE_SNAP_1:
		case LC_EDIT_ANGLE_SNAP_2:
		case LC_EDIT_ANGLE_SNAP_3:
		case LC_EDIT_ANGLE_SNAP_4:
		case LC_EDIT_ANGLE_SNAP_5:
		case LC_EDIT_ANGLE_SNAP_6:
		case LC_EDIT_ANGLE_SNAP_7:
		case LC_EDIT_ANGLE_SNAP_8:
		case LC_EDIT_ANGLE_SNAP_9:
		{
			int Angles[] = { 0, 1, 5, 10, 15, 30, 45, 60, 90, 180 };
			m_nAngleSnap = Angles[id - LC_EDIT_ANGLE_SNAP_0];
			if (m_nAngleSnap)
				m_nSnap |= LC_DRAW_SNAP_A;
			else
				m_nSnap &= ~LC_DRAW_SNAP_A;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

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

		case LC_EDIT_ACTION_ERASER:
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
	}
}

void Project::SetAction(int nAction)
{
	bool Redraw = false;

	if (m_nCurAction == LC_ACTION_INSERT)
		Redraw = true;

	m_PreviousAction = m_nCurAction;
	m_nCurAction = nAction;
	SystemUpdateAction(m_nCurAction, m_PreviousAction);

	if ((m_nCurAction == LC_ACTION_MOVE) || (m_nCurAction == LC_ACTION_ROTATE) ||
	    (m_nCurAction == LC_ACTION_ROTATE_VIEW))
	{
		ActivateOverlay();

		if (m_OverlayActive)
			Redraw = true;
	}
	else
	{
		if (m_OverlayActive)
		{
			m_OverlayActive = false;
			Redraw = true;
		}
	}

	if (Redraw)
		UpdateAllViews();
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
  Camera* pCamera;
  Light* pLight;

  for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
    pPiece->Select (false, bFocusOnly, false);

  for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
  {
    pCamera->Select (false, bFocusOnly, false);
    pCamera->GetTarget ()->Select (false, bFocusOnly, false);
  }

  for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
  {
    pLight->Select (false, bFocusOnly, false);
    if (pLight->GetTarget ())
      pLight->GetTarget ()->Select (false, bFocusOnly, false);
  }
//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);
}

bool Project::GetSelectionCenter(Vector3& Center) const
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

	Center = Vector3((bs[0] + bs[3]) * 0.5f, (bs[1] + bs[4]) * 0.5f, (bs[2] + bs[5]) * 0.5f);

	return Selected;
}

Camera* Project::GetCamera(int i)
{
	Camera* pCamera;

	for (pCamera = m_pCameras; i-- > 0 && pCamera; pCamera = pCamera->m_pNext)
		;
	return pCamera;
}

void Project::ConvertToUserUnits(Vector3& Value) const
{
	if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
		Value /= 0.04f;
}

void Project::ConvertFromUserUnits(Vector3& Value) const
{
	if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
		Value *= 0.04f;
}

bool Project::GetFocusPosition(Vector3& Position) const
{
	Piece* pPiece;
	Camera* pCamera;
	float* pos = &Position[0];

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsFocused())
		{
			pPiece->GetPosition(pos);
			return true;
		}

	for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
	{
		if (pCamera->IsEyeFocused())
		{
			pCamera->GetEyePos(pos);
			return true;
		}

		if (pCamera->IsTargetFocused())
		{
			pCamera->GetTargetPos(pos);
			return true;
		}
	}

	// TODO: light

	pos[0] = pos[1] = pos[2] = 0.0f;

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

	for (Camera* pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
	{
		if (pCamera->IsEyeFocused() || pCamera->IsTargetFocused())
			return pCamera;
	}

	// TODO: light

	return NULL;
}

// Find a good starting position/orientation relative to an existing piece.
void Project::GetPieceInsertPosition(Piece* OffsetPiece, Vector3& Position, Vector4& Rotation)
{
	Vector3 Dist(0, 0, OffsetPiece->GetPieceInfo()->m_fDimensions[2] - m_pCurPiece->m_fDimensions[5]);
	SnapVector(Dist);

	float pos[3], rot[4];
	OffsetPiece->GetPosition(pos);
	OffsetPiece->GetRotation(rot);

	Matrix mat(rot, pos);
	mat.Translate(Dist[0], Dist[1], Dist[2]);
	mat.GetTranslation(pos);

	Position = Vector3(pos[0], pos[1], pos[2]);
	Rotation = Vector4(rot[0], rot[1], rot[2], rot[3]);
}

// Try to find a good starting position/orientation for a new piece.
void Project::GetPieceInsertPosition(View* view, int MouseX, int MouseY, Vector3& Position, Vector4& Rotation)
{
	// Check if the mouse is over a piece.
	Piece* HitPiece = (Piece*)FindObjectFromPoint(view, MouseX, MouseY, true);

	if (HitPiece)
	{
		GetPieceInsertPosition(HitPiece, Position, Rotation);
		return;
	}

	// Try to hit the base grid.
	int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };

	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->m_Camera;

	// Build the matrices.
	Matrix44 ModelView, Projection;
	ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
	Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	Vector3 ClickPoints[2] = { Vector3((float)m_nDownX, (float)m_nDownY, 0.0f), Vector3((float)m_nDownX, (float)m_nDownY, 1.0f) };
	UnprojectPoints(ClickPoints, 2, ModelView, Projection, Viewport);

	Vector3 Intersection;
	if (LinePlaneIntersection(Intersection, ClickPoints[0], ClickPoints[1], Vector4(0, 0, 1, 0)))
	{
		SnapVector(Intersection);
		Position = Intersection;
		Rotation = Vector4(0, 0, 1, 0);
		return;
	}

	// Couldn't find a good position, so just place the piece somewhere near the camera.
	Position = UnprojectPoint(Vector3((float)m_nDownX, (float)m_nDownY, 0.9f), ModelView, Projection, Viewport);
	Rotation = Vector4(0, 0, 1, 0);
}

Object* Project::FindObjectFromPoint(View* view, int x, int y, bool PiecesOnly)
{
	int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->m_Camera;

	Matrix44 ModelView, Projection;
	ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
	Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	Vector3 Start = UnprojectPoint(Vector3((float)x, (float)y, 0.0f), ModelView, Projection, Viewport);
	Vector3 End = UnprojectPoint(Vector3((float)x, (float)y, 1.0f), ModelView, Projection, Viewport);

	LC_CLICKLINE ClickLine;

	ClickLine.a1 = Start[0];
	ClickLine.b1 = Start[1];
	ClickLine.c1 = Start[2];
	ClickLine.a2 = End[0] - Start[0];
	ClickLine.b2 = End[1] - Start[1];
	ClickLine.c2 = End[2] - Start[2];
	ClickLine.mindist = FLT_MAX;
	ClickLine.pClosest = NULL;

	for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			pPiece->MinIntersectDist(&ClickLine);

	if (!PiecesOnly)
	{
		for (Camera* pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
			if (pCamera != view->m_Camera)
				pCamera->MinIntersectDist(&ClickLine);

		for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
			pLight->MinIntersectDist(&ClickLine);
	}

	return ClickLine.pClosest;
}

void Project::FindObjectsInBox(float x1, float y1, float x2, float y2, PtrArray<Object>& Objects)
{
	int Viewport[4] = { 0, 0, m_ActiveView->GetWidth(), m_ActiveView->GetHeight() };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = m_ActiveView->m_Camera;

	// Build the matrices.
	Matrix44 ModelView, Projection;
	ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
	Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

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
	Vector3 Corners[6] =
	{
		Vector3(Left, Top, 0), Vector3(Left, Bottom, 0), Vector3(Right, Bottom, 0),
		Vector3(Right, Top, 0), Vector3(Left, Top, 1), Vector3(Right, Bottom, 1)
	};

	UnprojectPoints(Corners, 6, ModelView, Projection, Viewport);

	// Build the box planes.
	Vector4 Planes[6];

	Planes[0] = Cross3(Corners[4] - Corners[0], Corners[1] - Corners[0]).Normalize(); // Left
	Planes[1] = Cross3(Corners[5] - Corners[2], Corners[3] - Corners[2]).Normalize(); // Right
	Planes[2] = Cross3(Corners[3] - Corners[0], Corners[4] - Corners[0]).Normalize(); // Top
	Planes[3] = Cross3(Corners[1] - Corners[2], Corners[5] - Corners[2]).Normalize(); // Bottom
	Planes[4] = Cross3(Corners[1] - Corners[0], Corners[3] - Corners[0]).Normalize(); // Front
	Planes[5] = Cross3(Corners[1] - Corners[2], Corners[3] - Corners[2]).Normalize(); // Back

	Planes[0][3] = -Dot3(Planes[0], Corners[0]);
	Planes[1][3] = -Dot3(Planes[1], Corners[5]);
	Planes[2][3] = -Dot3(Planes[2], Corners[0]);
	Planes[3][3] = -Dot3(Planes[3], Corners[5]);
	Planes[4][3] = -Dot3(Planes[4], Corners[0]);
	Planes[5][3] = -Dot3(Planes[5], Corners[5]);

	// Check if any objects are inside the volume.
	for (Piece* piece = m_pPieces; piece != NULL; piece = piece->m_pNext)
	{
		if (piece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
		{
			if (piece->IntersectsVolume(Planes, 6))
				Objects.Add(piece);
		}
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

	if ((m_nTracking == LC_TRACK_START_LEFT) || (m_nTracking == LC_TRACK_START_RIGHT))
	{
		if (m_pTrackFile)
		{
			delete m_pTrackFile;
			m_pTrackFile = NULL;
		}

		m_nTracking = LC_TRACK_NONE;
		SystemReleaseMouse();
		return false;
	}

	m_bTrackCancel = true;
	m_nTracking = LC_TRACK_NONE;
	SystemReleaseMouse();

	// Reset the mouse overlay.
	if (m_OverlayActive)
	{
		ActivateOverlay();
		UpdateAllViews();
	}

	if (bAccept)
	{
		switch (m_nCurAction)
		{
			case LC_ACTION_SELECT:
			{
				if (((float)m_nDownX != m_fTrack[0]) && ((float)m_nDownY != m_fTrack[1]))
				{
					// Find objects inside the rectangle.
					PtrArray<Object> Objects;
					FindObjectsInBox((float)m_nDownX, (float)m_nDownY, m_fTrack[0], m_fTrack[1], Objects);

					// Deselect old pieces.
					bool Control = Sys_KeyDown(KEY_CONTROL);
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
								Objects[i]->Select(true, false, Control);
						}
						else
							Objects[i]->Select(true, false, Control);
					}
				}

				// Update screen and UI.
				UpdateSelection();
				UpdateAllViews();
				SystemUpdateFocus(NULL);

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
				SystemUpdateCameraMenu(m_pCameras);
				SystemUpdateCurrentCamera(NULL, m_ActiveView->m_Camera, m_pCameras);
				SetModifiedFlag(true);
				CheckPoint("Inserting");
			} break;

			case LC_ACTION_CURVE:
			{
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
				int Viewport[4] = { 0, 0, m_ActiveView->GetWidth(), m_ActiveView->GetHeight() };
				float Aspect = (float)Viewport[2]/(float)Viewport[3];
				Camera* Cam = m_ActiveView->m_Camera;

				// Build the matrices.
				Matrix44 ModelView, Projection;
				ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
				Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

				// Find out the top-left and bottom-right corners in screen coordinates.
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

				// Unproject screen points to world space.
				Vector3 Points[3] =
				{
					Vector3((Left + Right) / 2, (Top + Bottom) / 2, 0.9f),
					Vector3((float)Viewport[2] / 2.0f, (float)Viewport[3] / 2.0f, 0.9f),
					Vector3((float)Viewport[2] / 2.0f, (float)Viewport[3] / 2.0f, 0.1f),
				};

				UnprojectPoints(Points, 3, ModelView, Projection, Viewport);

				// Center camera.
				Vector3 Eye = Cam->GetEyePosition();
				Eye = Eye + (Points[0] - Points[1]);

				Vector3 Target = Cam->GetTargetPosition();
				Target = Target + (Points[0] - Points[1]);

				// Zoom in/out.
				float RatioX = (Right - Left) / Viewport[2];
				float RatioY = (Top - Bottom) / Viewport[3];
				float ZoomFactor = -max(RatioX, RatioY) + 0.75f;

				Vector3 Dir = Points[1] - Points[2];
				Eye = Eye + Dir * ZoomFactor;
				Target = Target + Dir * ZoomFactor;

				// Change the camera and redraw.
			  Cam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Eye, LC_CK_EYE);
			  Cam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, Target, LC_CK_TARGET);
			  Cam->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				SystemUpdateFocus(NULL);
				UpdateAllViews();
			} break;

			case LC_ACTION_INSERT:
			case LC_ACTION_LIGHT:
			case LC_ACTION_ERASER:
			case LC_ACTION_PAINT:
				break;
		}

		if (m_RestoreAction)
		{
			SetAction(m_PreviousAction);
			m_RestoreAction = false;
		}
	}
	else if (m_pTrackFile != NULL)
	{
		if ((m_nCurAction == LC_ACTION_SELECT) || (m_nCurAction == LC_ACTION_ZOOM_REGION))
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

	return true;
}

void Project::StartTracking(int mode)
{
	SystemCaptureMouse();
	m_nTracking = mode;

  if (m_pTrackFile != NULL)
    m_pTrackFile->SetLength (0);
  else
  	m_pTrackFile = new FileMem;

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

			for (int i = 0; i < sizeof(Angles)/sizeof(Angles[0]); i++)
			{
				if (m_nAngleSnap == Angles[i])
				{
					*SnapAngle = i;
					break;
				}
			}
		}
		else
			SnapAngle = 0;
	}
}

void Project::GetSnapDistance(float* SnapXY, float* SnapZ) const
{
	const float SnapXYTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 1.6f, 2.4f, 3.2f, 6.4f };
	const float SnapZTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 0.96f, 1.92f, 3.84f, 7.68f };

	int SXY, SZ;
	GetSnapIndex(&SXY, &SZ, NULL);

	SXY = min(SXY, 9);
	SZ = min(SZ, 9);

	*SnapXY = SnapXYTable[SXY];
	*SnapZ = SnapZTable[SZ];
}

void Project::GetSnapDistanceText(char* SnapXY, char* SnapZ) const
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

		int SXY, SZ;
		GetSnapIndex(&SXY, &SZ, NULL);

		SXY = min(SXY, 9);
		SZ = min(SZ, 9);

		strcpy(SnapXY, SnapXYText[SXY]);
		strcpy(SnapZ, SnapZText[SZ]);
	}
}

void Project::SnapVector(Vector3& Delta, Vector3& Leftover) const
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

void Project::SnapRotationVector(Vector3& Delta, Vector3& Leftover) const
{
	if (m_nSnap & LC_DRAW_SNAP_A)
	{
		int Snap[3];

		for (int i = 0; i < 3; i++)
		{
			Snap[i] = (int)(Delta[i] / (float)m_nAngleSnap);
		}

		Vector3 NewDelta((float)(m_nAngleSnap * Snap[0]), (float)(m_nAngleSnap * Snap[1]), (float)(m_nAngleSnap * Snap[2]));
		Leftover = Delta - NewDelta;
		Delta = NewDelta;
	}
}

bool Project::MoveSelectedObjects(Vector3& Move, Vector3& Remainder, bool Snap)
{
	// Don't move along locked directions.
	if (m_nSnap & LC_DRAW_LOCK_X)
		Move[0] = 0;

	if (m_nSnap & LC_DRAW_LOCK_Y)
		Move[1] = 0;

	if (m_nSnap & LC_DRAW_LOCK_Z)
		Move[2] = 0;

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
		{
			float Rot[4];
			((Piece*)Focus)->GetRotation(Rot);

			Matrix33 RotMat;
			RotMat.CreateFromAxisAngle(Vector3(Rot[0], Rot[1], Rot[2]), Rot[3] * LC_DTOR);

			Move = Mul(Move, RotMat);
		}
	}

	Piece* pPiece;
	Camera* pCamera;
	Light* pLight;
	float x = Move[0], y = Move[1], z = Move[2];

	for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		if (pCamera->IsSelected())
		{
			pCamera->Move(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, x, y, z);
			pCamera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
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

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsSelected())
			pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);

	// TODO: move group centers

	if (m_OverlayActive)
	{
		if (!GetFocusPosition(m_OverlayCenter))
			GetSelectionCenter(m_OverlayCenter);
	}

	return true;
}

bool Project::RotateSelectedObjects(Vector3& Delta, Vector3& Remainder)
{
	// Don't move along locked directions.
	if (m_nSnap & LC_DRAW_LOCK_X)
		Delta[0] = 0;

	if (m_nSnap & LC_DRAW_LOCK_Y)
		Delta[1] = 0;

	if (m_nSnap & LC_DRAW_LOCK_Z)
		Delta[2] = 0;

	// Snap.
	SnapRotationVector(Delta, Remainder);

	if (Delta.LengthSquared() < 0.001f)
		return false;

	float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
	float pos[3], rot[4];
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
		pFocus->GetPosition(pos);
		bs[0] = bs[3] = pos[0];
		bs[1] = bs[4] = pos[1];
		bs[2] = bs[5] = pos[2];
	}

	Vector3 Center((bs[0]+bs[3])/2, (bs[1]+bs[4])/2, (bs[2]+bs[5])/2);

	// Create the rotation matrix.
	Quaternion Rotation(0, 0, 0, 1);
	Quaternion WorldToFocus, FocusToWorld;

	if (!(m_nSnap & LC_DRAW_LOCK_X) && (Delta[0] != 0.0f))
	{
		Quaternion q;
		q.CreateRotationX(Delta[0] * LC_DTOR);
		Rotation = Mul(q, Rotation);
	}

	if (!(m_nSnap & LC_DRAW_LOCK_Y) && (Delta[1] != 0.0f))
	{
		Quaternion q;
		q.CreateRotationY(Delta[1] * LC_DTOR);
		Rotation = Mul(q, Rotation);
	}

	if (!(m_nSnap & LC_DRAW_LOCK_Z) && (Delta[2] != 0.0f))
	{
		Quaternion q;
		q.CreateRotationZ(Delta[2] * LC_DTOR);
		Rotation = Mul(q, Rotation);
	}

	// Transform the rotation relative to the focused piece.
	if (m_nSnap & LC_DRAW_GLOBAL_SNAP)
		pFocus = NULL;

	if (pFocus != NULL)
	{
		float Rot[4];
		((Piece*)pFocus)->GetRotation(Rot);

		WorldToFocus.FromAxisAngle(Vector4(Rot[0], Rot[1], Rot[2], -Rot[3] * LC_DTOR));
		FocusToWorld.FromAxisAngle(Vector4(Rot[0], Rot[1], Rot[2], Rot[3] * LC_DTOR));

		Rotation = Mul(FocusToWorld, Rotation);
	}

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		if (!pPiece->IsSelected())
			continue;

		pPiece->GetPosition(pos);
		pPiece->GetRotation(rot);

		Vector4 NewRotation;

		if ((nSel == 1) && (pFocus == pPiece))
		{
			Quaternion LocalToWorld;
			LocalToWorld.FromAxisAngle(Vector4(rot[0], rot[1], rot[2], rot[3] * LC_DTOR));

			Quaternion NewLocalToWorld;

			if (pFocus != NULL)
			{
				Quaternion LocalToFocus = Mul(WorldToFocus, LocalToWorld);
				NewLocalToWorld = Mul(LocalToFocus, Rotation);
			}
			else
			{
				NewLocalToWorld = Mul(Rotation, LocalToWorld);
			}

			NewLocalToWorld.ToAxisAngle(NewRotation);
		}
		else
		{
			Vector3 Distance = Vector3(pos[0], pos[1], pos[2]) - Center;

			Quaternion LocalToWorld;
			LocalToWorld.FromAxisAngle(Vector4(rot[0], rot[1], rot[2], rot[3] * LC_DTOR));

			Quaternion NewLocalToWorld;

			if (pFocus != NULL)
			{
				Quaternion LocalToFocus = Mul(WorldToFocus, LocalToWorld);
				NewLocalToWorld = Mul(Rotation, LocalToFocus);

				Quaternion WorldToLocal;
				WorldToLocal.FromAxisAngle(Vector4(rot[0], rot[1], rot[2], -rot[3] * LC_DTOR));

				Distance = Mul(Distance, WorldToLocal);
				Distance = Mul(Distance, NewLocalToWorld);
			}
			else
			{
				NewLocalToWorld = Mul(Rotation, LocalToWorld);

				Distance = Mul(Distance, Rotation);
			}

			NewLocalToWorld.ToAxisAngle(NewRotation);

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

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsSelected())
			pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);

	if (m_OverlayActive)
	{
		if (!GetFocusPosition(m_OverlayCenter))
			GetSelectionCenter(m_OverlayCenter);
	}

	return true;
}

bool Project::OnKeyDown(char nKey, bool bControl, bool bShift)
{
	bool ret = false;

	// FIXME: Almost all of this should go through the keyboard shortcut system.
	switch (nKey)
	{
		case KEY_ESCAPE:
		{
			if (m_nTracking != LC_TRACK_NONE)
				StopTracking(false);
			ret = true;
		} break;

		case KEY_INSERT:
		{
			HandleCommand(LC_PIECE_INSERT, 0);
			ret = true;
		} break;

		case KEY_DELETE:
		{
			HandleCommand(LC_PIECE_DELETE, 0);
			ret = true;
		} break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			if (bControl)
			{
				m_nCurClipboard = nKey - 0x30;
				SystemUpdatePaste(m_pClipboard[m_nCurClipboard] != NULL);
				ret = true;
			}
		} break;


		case KEY_PLUS: // case '+': case '=':
		{
			if (bShift)
				HandleCommand(LC_VIEW_ZOOM, -10);
			else
				HandleCommand(LC_VIEW_ZOOM, -1);

			ret = true;
		} break;
			
		case KEY_MINUS: // case '-': case '_':
		{
			if (bShift)
				HandleCommand(LC_VIEW_ZOOM, 10);
			else
				HandleCommand(LC_VIEW_ZOOM, 1);

			ret = true;
		} break;

		case KEY_TAB:
		{
			if (m_pPieces == NULL)
				break;

			Piece* pFocus = NULL, *pPiece;
			for (pFocus = m_pPieces; pFocus; pFocus = pFocus->m_pNext)
				if (pFocus->IsFocused())
					break;

			SelectAndFocusNone(false);

			if (pFocus == NULL)
			{
				if (bShift)
				{
					// Focus the last visible piece.
					for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
						if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
							pFocus = pPiece;
				}
				else
				{
					// Focus the first visible piece.
					for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
						if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
						{
							pFocus = pPiece;
							break;
						}
				}
			}
			else
			{
				if (bShift)
				{
					// Focus the previous visible piece.
					Piece* pBest = pPiece = pFocus;
					for (;;)
					{
						if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
							pBest = pPiece;

						if (pPiece->m_pNext != NULL)
						{
							if (pPiece->m_pNext == pFocus)
								break;
							else
								pPiece = pPiece->m_pNext;
						}
						else
						{
							if (pFocus == m_pPieces)
								break;
							else
								pPiece = m_pPieces;
						}
					}

					pFocus = pBest;
				}
				else
				{
					// Focus the next visible piece.
					pPiece = pFocus;
					for (;;)
					{
						if (pPiece != pFocus)
							if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
							{
								pFocus = pPiece;
								break;
							}

						if (pPiece->m_pNext != NULL)
						{
							if (pPiece->m_pNext == pFocus)
								break;
							else
								pPiece = pPiece->m_pNext;
						}
						else
						{
							if (pFocus == m_pPieces)
								break;
							else
								pPiece = m_pPieces;
						}
					}
				}
			}

			if (pFocus != NULL)
			{
        pFocus->Select (true, true, false);
        Group* pGroup = pFocus->GetTopGroup();
        if (pGroup != NULL)
        {
          for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
            if ((pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation)) &&
                (pPiece->GetTopGroup() == pGroup))
              pPiece->Select (true, false, false);
        }
			}

			UpdateSelection();
			UpdateAllViews();
			SystemUpdateFocus(pFocus);
			ret = true;
		} break;

		case KEY_UP:    case KEY_DOWN: case KEY_LEFT: 
		case KEY_RIGHT: case KEY_NEXT: case KEY_PRIOR:
//		if (AnyObjectSelected(FALSE))
		{
			Vector3 axis;
			if (bShift)
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

				if (((m_nSnap & LC_DRAW_SNAP_X) == 0) || bControl)
					axis[0] = 0.01f;
				if (((m_nSnap & LC_DRAW_SNAP_Y) == 0) || bControl)
					axis[1] = 0.01f;
				if (((m_nSnap & LC_DRAW_SNAP_Z) == 0) || bControl)
					axis[2] = 0.01f;
			}

			if (m_nSnap & LC_DRAW_MOVEAXIS)
			{
				switch (nKey)
				{
					case KEY_UP:
						axis[1] = axis[2] = 0; axis[0] = -axis[0];
						break;
					case KEY_DOWN:
						axis[1] = axis[2] = 0;
						break;
					case KEY_LEFT:
						axis[0] = axis[2] = 0; axis[1] = -axis[1];
						break;
					case KEY_RIGHT:
						axis[0] = axis[2] = 0;
						break;
					case KEY_NEXT:
						axis[0] = axis[1] = 0; axis[2] = -axis[2];
						break;
					case KEY_PRIOR:
						axis[0] = axis[1] = 0;
						break;
				}
			}
			else
			{
				Camera *camera = m_ActiveView->m_Camera;

				if (camera->IsSide ())
				{
					Matrix mat;

					mat.CreateLookat(camera->GetEyePos (), camera->GetTargetPos (), camera->GetUpVec ());
					mat.SetTranslation(0, 0, 0);
					mat.Invert();

					switch (nKey)
					{
					case KEY_UP:
						axis[0] = axis[2] = 0;
						break;

					case KEY_DOWN:
						axis[0] = axis[2] = 0; axis[1] = -axis[1];
						break;

					case KEY_LEFT:
						axis[0] = -axis[0]; axis[1] = axis[2] = 0;
						break;

					case KEY_RIGHT:
						axis[1] = axis[2] = 0;
						break;

					case KEY_NEXT:
						axis[0] = axis[1] = 0; axis[2] = -axis[2];
						break;

					case KEY_PRIOR:
						axis[0] = axis[1] = 0;
						break;
					}

					mat.TransformPoints (axis, 1);
				}
				else
				{

					// TODO: rewrite this

					switch (nKey)
					{
					case KEY_UP:
						axis[1] = axis[2] = 0; axis[0] = -axis[0];
						break;
					case KEY_DOWN:
						axis[1] = axis[2] = 0;
						break;
					case KEY_LEFT:
						axis[0] = axis[2] = 0; axis[1] = -axis[1];
						break;
					case KEY_RIGHT:
						axis[0] = axis[2] = 0;
						break;
					case KEY_NEXT:
						axis[0] = axis[1] = 0; axis[2] = -axis[2];
						break;
					case KEY_PRIOR:
						axis[0] = axis[1] = 0;
						break;
					}

					int Viewport[4] = { 0, 0, m_ActiveView->GetWidth(), m_ActiveView->GetHeight() };
					float Aspect = (float)Viewport[2]/(float)Viewport[3];
					Camera* Cam = m_ActiveView->m_Camera;

					Matrix44 ModelView, Projection;
					ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
					Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

					Vector3 Pts[3] = { Vector3(5.0f, 5.0f, 0.1f), Vector3(10.0f, 5.0f, 0.1f), Vector3(5.0f, 10.0f, 0.1f) };
					UnprojectPoints(Pts, 3, ModelView, Projection, Viewport);

					float ax, ay;
					Vector3 vx((Pts[1][0] - Pts[0][0]), (Pts[1][1] - Pts[0][1]), 0);//Pts[1][2] - Pts[0][2] };
					vx.Normalize();
					Vector3 x(1, 0, 0);
					ax = acosf(Dot3(vx, x));

					Vector3 vy((Pts[2][0] - Pts[0][0]), (Pts[2][1] - Pts[0][1]), 0);//Pts[2][2] - Pts[0][2] };
					vy.Normalize();
					Vector3 y(0, -1, 0);
					ay = acosf(Dot3(vy, y));

					if (ax > 135)
						axis[0] = -axis[0];

					if (ay < 45)
						axis[1] = -axis[1];

					if (ax >= 45 && ax <= 135)
					{
						float tmp = axis[0];

						ax = acosf(Dot3(vx, y));
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
			}

			if (bShift)
			{
				Vector3 tmp;
				RotateSelectedObjects(axis, tmp);
			}
			else
			{
				Vector3 tmp;
				MoveSelectedObjects(axis, tmp, false);
			}

			UpdateOverlayScale();
			UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint((bShift) ? "Rotating" : "Moving");
			SystemUpdateFocus(NULL);
			ret = true;
		} break;
	}

	return ret;
}

void Project::BeginPieceDrop(PieceInfo* Info)
{
	StartTracking(LC_TRACK_LEFT);
	SetAction(LC_ACTION_INSERT);
	m_RestoreAction = true;

	m_PreviousPiece = m_pCurPiece;

	m_pCurPiece = Info;
	m_pCurPiece->AddRef();
}

void Project::BeginColorDrop()
{
	StartTracking(LC_TRACK_LEFT);
	SetAction(LC_ACTION_PAINT);
	m_RestoreAction = true;
}

void Project::OnLeftButtonDown(View* view, int x, int y, bool bControl, bool bShift)
{
	if (m_nTracking != LC_TRACK_NONE)
		if (StopTracking(false))
			return;

	if (SetActiveView(view))
		return;

	m_bTrackCancel = false;
	m_nDownX = x;
	m_nDownY = y;
	m_MouseTotalDelta = Vector3(0, 0, 0);
	m_MouseSnapLeftover = Vector3(0, 0, 0);

	int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->m_Camera;

	Matrix44 ModelView, Projection;
	ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
	Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	Vector3 point = UnprojectPoint(Vector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	if (Sys_KeyDown(KEY_ALT))
	{
		SetAction(LC_ACTION_ROTATE_VIEW);
		m_RestoreAction = true;
	}
	else
		m_RestoreAction = false;

	switch (m_nCurAction)
	{
		case LC_ACTION_SELECT:
		case LC_ACTION_ERASER:
		case LC_ACTION_PAINT:
		{
			Object* Closest = FindObjectFromPoint(view, x, y);

			if (m_nCurAction == LC_ACTION_SELECT) 
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

							SelectAndFocusNone (bControl);

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
							SelectAndFocusNone (bControl);
							Closest->Select (true, true, bControl);
						} break;
					}
				}
				else
					SelectAndFocusNone (bControl);

				UpdateSelection();
				UpdateAllViews();
				SystemUpdateFocus(Closest);

				StartTracking(LC_TRACK_START_LEFT);
			}

			if ((m_nCurAction == LC_ACTION_ERASER) && (Closest != NULL))
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
						if (Closest->GetType () == LC_OBJECT_CAMERA)
							pCamera = (Camera*)Closest;
						else
							pCamera = ((CameraTarget*)Closest)->GetParent();

						bool bCanDelete = pCamera->IsUser();

						for (int ViewIdx = 0; ViewIdx < m_ViewList.GetSize(); ViewIdx++)
							if (pCamera == m_ViewList[ViewIdx]->m_Camera)
								bCanDelete = false;

						if (bCanDelete)
						{
							Camera* pPrev;
							for (pPrev = m_pCameras; pPrev; pPrev = pPrev->m_pNext)
								if (pPrev->m_pNext == pCamera)
								{
									pPrev->m_pNext = pCamera->m_pNext;
									delete pCamera;
									SystemUpdateCameraMenu(m_pCameras);
									SystemUpdateCurrentCamera(NULL, m_ActiveView->m_Camera, m_pCameras);
									break;
								}
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

			if ((m_nCurAction == LC_ACTION_PAINT) && (Closest != NULL) && (Closest->GetType() == LC_OBJECT_PIECE))
			{
				Piece* pPiece = (Piece*)Closest;

				if (pPiece->GetColor() != m_nCurColor)
				{
					bool bTrans = pPiece->IsTransparent();
					pPiece->SetColor(m_nCurColor);
					if (bTrans != pPiece->IsTransparent())
						pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, true, true);

					SetModifiedFlag(true);
					CheckPoint("Painting");
					SystemUpdateFocus(NULL);
					UpdateAllViews();
				}
			}
		} break;

		case LC_ACTION_INSERT:
		case LC_ACTION_LIGHT:
		{
			if (m_nCurAction == LC_ACTION_INSERT)
			{
				Vector3 Pos;
				Vector4 Rot;

				GetPieceInsertPosition(view, x, y, Pos, Rot);

				Piece* pPiece = new Piece(m_pCurPiece);
				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_nCurStep, m_nCurFrame, m_nCurColor);

				pPiece->ChangeKey(m_nCurStep, false, false, Rot, LC_PK_ROTATION);
				pPiece->ChangeKey(m_nCurFrame, true, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				SelectAndFocusNone(false);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);
				pPiece->Select (true, true, false);
				UpdateSelection();
				SystemPieceComboAdd(m_pCurPiece->m_strDescription);
				SystemUpdateFocus(pPiece);

				if (m_nSnap & LC_DRAW_MOVE)
					SetAction(LC_ACTION_MOVE);
			}
			else if (m_nCurAction == LC_ACTION_LIGHT)
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
				SystemUpdateFocus (pLight);
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

	  Vector3 tmp = UnprojectPoint(Vector3(x+1.0f, y-1.0f, 0.9f), ModelView, Projection, Viewport);
      SelectAndFocusNone(false);
      StartTracking(LC_TRACK_START_LEFT);
      pLight = new Light (m_fTrack[0], m_fTrack[1], m_fTrack[2], tmp[0], tmp[1], tmp[2]);
      pLight->GetTarget ()->Select (true, true, false);
      pLight->m_pNext = m_pLights;
      m_pLights = pLight;
      UpdateSelection();
      UpdateAllViews();
      SystemUpdateFocus(pLight);
    } break;

    case LC_ACTION_CAMERA:
    {
	  Vector3 tmp = UnprojectPoint(Vector3(x+1.0f, y-1.0f, 0.9f), ModelView, Projection, Viewport);
      SelectAndFocusNone(false);
      StartTracking(LC_TRACK_START_LEFT);
      Camera* pCamera = new Camera(m_fTrack[0], m_fTrack[1], m_fTrack[2], tmp[0], tmp[1], tmp[2], m_pCameras);
      pCamera->GetTarget ()->Select (true, true, false);
      UpdateSelection();
      UpdateAllViews();
      SystemUpdateFocus(pCamera);
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

			if (!sel)
			{
				for (Camera* pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
				{
					if (pCamera->IsSelected())
					{
						sel = true;
						break;
					}
				}
			}

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
				m_OverlayDelta = Vector3(0.0f, 0.0f, 0.0f);
				m_MouseSnapLeftover = Vector3(0.0f, 0.0f, 0.0f);
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
					m_OverlayDelta = Vector3(0.0f, 0.0f, 0.0f);
					break;
				}
			}
		} break;

		case LC_ACTION_ZOOM_REGION:
		{
			m_OverlayTrackStart[0] = (float)x;
			m_OverlayTrackStart[1] = (float)y;
			StartTracking(LC_TRACK_START_LEFT);
			ActivateOverlay();
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

void Project::OnLeftButtonDoubleClick(View* view, int x, int y, bool bControl, bool bShift)
{
	if (SetActiveView(view))
		return;

	int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->m_Camera;

	Matrix44 ModelView, Projection;
	ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
	Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	Vector3 point = UnprojectPoint(Vector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	Object* Closest = FindObjectFromPoint(view, x, y);

//  if (m_nCurAction == LC_ACTION_SELECT) 
  {
    SelectAndFocusNone(bControl);

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
          Closest->Select (true, true, bControl);
        } break;
      }

    UpdateSelection();
    UpdateAllViews();
    SystemUpdateFocus(Closest);
  }
}

void Project::OnLeftButtonUp(View* view, int x, int y, bool bControl, bool bShift)
{
	if (m_nTracking == LC_TRACK_LEFT)
	{
		// Dragging a new piece from the tree.
		if (m_nCurAction == LC_ACTION_INSERT)
		{
			if ((x > 0) && (x < view->GetWidth()) && (y > 0) && (y < view->GetHeight()))
			{
				Vector3 Pos;
				Vector4 Rot;

				GetPieceInsertPosition(view, x, y, Pos, Rot);

				Piece* pPiece = new Piece(m_pCurPiece);
				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_nCurStep, m_nCurFrame, m_nCurColor);

				pPiece->ChangeKey(m_nCurStep, false, false, Rot, LC_PK_ROTATION);
				pPiece->ChangeKey(m_nCurFrame, true, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

				SelectAndFocusNone(false);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);
				pPiece->Select (true, true, false);
				UpdateSelection();
				SystemPieceComboAdd(m_pCurPiece->m_strDescription);
				SystemUpdateFocus(pPiece);

				if (m_nSnap & LC_DRAW_MOVE)
				{
					SetAction(LC_ACTION_MOVE);
					m_RestoreAction = false;
				}
			}

			m_pCurPiece->DeRef();
			m_pCurPiece = m_PreviousPiece;
			m_PreviousPiece = NULL;
		}
	}

	StopTracking(true);
}

void Project::OnMiddleButtonDown(View* view, int x, int y, bool bControl, bool bShift)
{
	if (StopTracking(false))
		return;

	if (SetActiveView(view))
		return;

	m_nDownX = x;
	m_nDownY = y;
	m_bTrackCancel = false;

	int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->m_Camera;

	Matrix44 ModelView, Projection;
	ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
	Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	Vector3 point = UnprojectPoint(Vector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	if (Sys_KeyDown(KEY_ALT))
	{
		SetAction(LC_ACTION_PAN);
		m_RestoreAction = true;
	}

	switch (m_nCurAction)
	{
		case LC_ACTION_PAN:
		{
			StartTracking(LC_TRACK_START_RIGHT);
		} break;
	}
}

void Project::OnMiddleButtonUp(View* view, int x, int y, bool bControl, bool bShift)
{
	StopTracking(true);
}

void Project::OnRightButtonDown(View* view, int x, int y, bool bControl, bool bShift)
{
	if (StopTracking(false))
		return;

	if (SetActiveView(view))
		return;

	m_nDownX = x;
	m_nDownY = y;
	m_bTrackCancel = false;

	int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->m_Camera;

	Matrix44 ModelView, Projection;
	ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
	Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	Vector3 point = UnprojectPoint(Vector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	if (Sys_KeyDown(KEY_ALT))
	{
		SetAction(LC_ACTION_ZOOM);
		m_RestoreAction = true;
	}

	switch (m_nCurAction)
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

			if (!sel)
			for (Camera* pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
				if (pCamera->IsSelected())
				{
					sel = true;
					break;
				}

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

void Project::OnRightButtonUp(View* view, int x, int y, bool bControl, bool bShift)
{
	if (!StopTracking(true) && !m_bTrackCancel)
		SystemDoPopupMenu(1, -1, -1);
	m_bTrackCancel = false;
}

void Project::OnMouseMove(View* view, int x, int y, bool bControl, bool bShift)
{
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

	int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	Camera* Cam = view->m_Camera;

	Matrix44 ModelView, Projection;
	ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
	Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

	Vector3 tmp = UnprojectPoint(Vector3((float)x, (float)y, 0.9f), ModelView, Projection, Viewport);
	ptx = tmp[0]; pty = tmp[1]; ptz = tmp[2];

	switch (m_nCurAction)
	{
		case LC_ACTION_SELECT:
		{
			int ptx = x, pty = y;

			if (ptx >= Viewport[0] + Viewport[2])
				ptx = Viewport[0] + Viewport[2] - 1;
			else if (ptx <= Viewport[0])
				ptx = Viewport[0] + 1;

			if (pty >= Viewport[1] + Viewport[3])
				pty = Viewport[1] + Viewport[3] - 1;
			else if (pty <= Viewport[1])
				pty = Viewport[1] + 1;

			m_fTrack[0] = (float)ptx;
			m_fTrack[1] = (float)pty;

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

			SystemUpdateFocus(NULL);
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
			
			Camera* pCamera = m_pCameras;
			while (pCamera->m_pNext != NULL)
				pCamera = pCamera->m_pNext;

			pCamera->Move (1, m_bAnimation, false, delta[0], delta[1], delta[2]);
			pCamera->UpdatePosition(1, m_bAnimation);

			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;

		case LC_ACTION_MOVE:
		{
			// Check if the mouse moved since the last update.
			if ((x == m_nDownX) && (y == m_nDownY))
				break;

			Camera* Camera = view->m_Camera;
			bool Redraw;

			if ((m_OverlayActive && (m_OverlayMode != LC_OVERLAY_XYZ)) || (!Camera->IsSide()))
			{
				Vector3 ScreenX = Cross3(Camera->GetTargetPosition() - Camera->GetEyePosition(), Camera->GetUpVector());
				Vector3 ScreenY = Camera->GetUpVector();
				ScreenX.Normalize();
				Vector3 Dir1, Dir2;
				bool SingleDir = true;

				int OverlayMode;

				if (m_OverlayActive && (m_OverlayMode != LC_OVERLAY_XYZ))
					OverlayMode = m_OverlayMode;
				else if (m_nTracking == LC_TRACK_LEFT)
					OverlayMode = LC_OVERLAY_XY;
				else
					OverlayMode = LC_OVERLAY_Z;

				switch (OverlayMode)
				{
				case LC_OVERLAY_X:
					Dir1 = Vector3(1, 0, 0);
					break;
				case LC_OVERLAY_Y:
					Dir1 = Vector3(0, 1, 0);
					break;
				case LC_OVERLAY_Z:
					Dir1 = Vector3(0, 0, 1);
					break;
				case LC_OVERLAY_XY:
					Dir1 = Vector3(1, 0, 0);
					Dir2 = Vector3(0, 1, 0);
					SingleDir = false;
					break;
				case LC_OVERLAY_XZ:
					Dir1 = Vector3(1, 0, 0);
					Dir2 = Vector3(0, 0, 1);
					SingleDir = false;
					break;
				case LC_OVERLAY_YZ:
					Dir1 = Vector3(0, 1, 0);
					Dir2 = Vector3(0, 0, 1);
					SingleDir = false;
					break;
				}

				// Transform the translation axis.
				Vector3 Axis1 = Dir1;
				Vector3 Axis2 = Dir2;

				if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
				{
					Object* Focus = GetFocusObject();

					if ((Focus != NULL) && Focus->IsPiece())
					{
						float Rot[4];
						((Piece*)Focus)->GetRotation(Rot);

						Matrix33 RotMat;
						RotMat.CreateFromAxisAngle(Vector3(Rot[0], Rot[1], Rot[2]), Rot[3] * LC_DTOR);

						Axis1 = Mul(Dir1, RotMat);
						Axis2 = Mul(Dir2, RotMat);
					}
				}

				// Find out what direction the mouse is going to move stuff.
				Vector3 MoveX, MoveY;

				if (SingleDir)
				{
					float dx1 = Dot3(ScreenX, Axis1);
					float dy1 = Dot3(ScreenY, Axis1);

					if (fabsf(dx1) > fabsf(dy1))
					{
						if (dx1 >= 0.0f)
							MoveX = Dir1;
						else
							MoveX = -Dir1;

						MoveY = Vector3(0, 0, 0);
					}
					else
					{
						MoveX = Vector3(0, 0, 0);

						if (dy1 > 0.0f)
							MoveY = Dir1;
						else
							MoveY = -Dir1;
					}
				}
				else
				{
					float dx1 = Dot3(ScreenX, Axis1);
					float dy1 = Dot3(ScreenY, Axis1);
					float dx2 = Dot3(ScreenX, Axis2);
					float dy2 = Dot3(ScreenY, Axis2);

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

				Vector3 Delta = MoveX + MoveY + m_MouseSnapLeftover;
				Redraw = MoveSelectedObjects(Delta, m_MouseSnapLeftover, true);
				m_MouseTotalDelta += Delta;
			}
			else
			{
				// 3D movement.
				Vector3 ScreenZ = (Camera->GetTargetPosition() - Camera->GetEyePosition()).Normalize();
				Vector3 ScreenX = Cross3(ScreenZ, Camera->GetUpVector());
				Vector3 ScreenY = Camera->GetUpVector();

				Vector3 TotalMove;

				if (m_nTracking == LC_TRACK_LEFT)
				{
					Vector3 MoveX, MoveY;

					MoveX = ScreenX * (float)(x - m_nDownX) * 0.25f / (float)(21 - m_nMouse);
					MoveY = ScreenY * (float)(y - m_nDownY) * 0.25f / (float)(21 - m_nMouse);

					TotalMove = MoveX + MoveY + m_MouseSnapLeftover;
				}
				else
				{
					Vector3 MoveZ;

					MoveZ = ScreenZ * (float)(y - m_nDownY) * 0.25f / (float)(21 - m_nMouse);

					TotalMove = MoveZ + m_MouseSnapLeftover;
				}

				m_nDownX = x;
				m_nDownY = y;

				Redraw = MoveSelectedObjects(TotalMove, m_MouseSnapLeftover, true);
			}

			SystemUpdateFocus(NULL);
			if (Redraw)
				UpdateAllViews();
		} break;
		
		case LC_ACTION_ROTATE:
		{
			Camera* Camera = m_ActiveView->m_Camera;
			bool Redraw;

			if ((m_OverlayActive && (m_OverlayMode != LC_OVERLAY_XYZ)) || (!Camera->IsSide()))
			{
				Vector3 ScreenX = Cross3(Camera->GetTargetPosition() - Camera->GetEyePosition(), Camera->GetUpVector());
				Vector3 ScreenY = Camera->GetUpVector();
				ScreenX.Normalize();
				Vector3 Dir1, Dir2;
				bool SingleDir = true;

				int OverlayMode;

				if (m_OverlayActive && (m_OverlayMode != LC_OVERLAY_XYZ))
					OverlayMode = m_OverlayMode;
				else if (m_nTracking == LC_TRACK_LEFT)
					OverlayMode = LC_OVERLAY_XY;
				else
					OverlayMode = LC_OVERLAY_Z;

				switch (OverlayMode)
				{
				case LC_OVERLAY_X:
					Dir1 = Vector3(1, 0, 0);
					break;
				case LC_OVERLAY_Y:
					Dir1 = Vector3(0, 1, 0);
					break;
				case LC_OVERLAY_Z:
					Dir1 = Vector3(0, 0, 1);
					break;
				case LC_OVERLAY_XY:
					Dir1 = Vector3(1, 0, 0);
					Dir2 = Vector3(0, 1, 0);
					SingleDir = false;
					break;
				case LC_OVERLAY_XZ:
					Dir1 = Vector3(1, 0, 0);
					Dir2 = Vector3(0, 0, 1);
					SingleDir = false;
					break;
				case LC_OVERLAY_YZ:
					Dir1 = Vector3(0, 1, 0);
					Dir2 = Vector3(0, 0, 1);
					SingleDir = false;
					break;
				}

				// Find out what direction the mouse is going to move stuff.
				Vector3 MoveX, MoveY;

				if (SingleDir)
				{
					float dx1 = Dot3(ScreenX, Dir1);
					float dy1 = Dot3(ScreenY, Dir1);

					if (fabsf(dx1) > fabsf(dy1))
					{
						if (dx1 >= 0.0f)
							MoveX = Dir1;
						else
							MoveX = -Dir1;

						MoveY = Vector3(0, 0, 0);
					}
					else
					{
						MoveX = Vector3(0, 0, 0);

						if (dy1 > 0.0f)
							MoveY = Dir1;
						else
							MoveY = -Dir1;
					}
				}
				else
				{
					float dx1 = Dot3(ScreenX, Dir1);
					float dy1 = Dot3(ScreenY, Dir1);
					float dx2 = Dot3(ScreenX, Dir2);
					float dy2 = Dot3(ScreenY, Dir2);

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

				Vector3 Delta = MoveX + MoveY + m_MouseSnapLeftover;
				Redraw = RotateSelectedObjects(Delta, m_MouseSnapLeftover);
				m_MouseTotalDelta += Delta;
			}
			else
			{
				// 3D movement.
				Vector3 ScreenZ = (Camera->GetTargetPosition() - Camera->GetEyePosition()).Normalize();
				Vector3 ScreenX = Cross3(ScreenZ, Camera->GetUpVector());
				Vector3 ScreenY = Camera->GetUpVector();

				Vector3 Delta;

				if (m_nTracking == LC_TRACK_LEFT)
				{
					Vector3 MoveX, MoveY;

					MoveX = ScreenX * (float)(x - m_nDownX) * 36.0f / (float)(21 - m_nMouse);
					MoveY = ScreenY * (float)(y - m_nDownY) * 36.0f / (float)(21 - m_nMouse);

					Delta = MoveX + MoveY + m_MouseSnapLeftover;
				}
				else
				{
					Vector3 MoveZ;

					MoveZ = ScreenZ * (float)(y - m_nDownY) * 36.0f / (float)(21 - m_nMouse);

					Delta = MoveZ + m_MouseSnapLeftover;
				}

				m_nDownX = x;
				m_nDownY = y;

				Redraw = RotateSelectedObjects(Delta, m_MouseSnapLeftover);
				m_MouseTotalDelta += Delta;
			}

			SystemUpdateFocus(NULL);
			if (Redraw)
				UpdateAllViews();
		} break;

		case LC_ACTION_ZOOM:
		{
			if (m_nDownY == y)
				break;

			m_ActiveView->m_Camera->DoZoom(y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownY = y;
			SystemUpdateFocus(NULL);
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

			m_ActiveView->m_Camera->DoPan(x - m_nDownX, y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownX = x;
			m_nDownY = y;
			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;
		
		case LC_ACTION_ROTATE_VIEW:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			// We can't rotate the side cameras.
			if (m_ActiveView->m_Camera->IsSide())
			{
				float eye[3], target[3], up[3];
				m_ActiveView->m_Camera->GetEyePos(eye);
				m_ActiveView->m_Camera->GetTargetPos(target);
				m_ActiveView->m_Camera->GetUpVec(up);
				Camera* pCamera = new Camera(eye, target, up, m_pCameras);

				m_ActiveView->m_Camera = pCamera;
				SystemUpdateCameraMenu(m_pCameras);
				SystemUpdateCurrentCamera(NULL, pCamera, m_pCameras);
			}

			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
					pPiece->CompareBoundingBox(bs);
			bs[0] = (bs[0]+bs[3])/2;
			bs[1] = (bs[1]+bs[4])/2;
			bs[2] = (bs[2]+bs[5])/2;

			switch (m_OverlayMode)
			{
				case LC_OVERLAY_XYZ:
					m_ActiveView->m_Camera->DoRotate(x - m_nDownX, y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, bs);
					break;

				case LC_OVERLAY_X:
					m_ActiveView->m_Camera->DoRotate(x - m_nDownX, 0, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, bs);
					break;

				case LC_OVERLAY_Y:
					m_ActiveView->m_Camera->DoRotate(0, y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, bs);
					break;

				case LC_OVERLAY_Z:
					m_ActiveView->m_Camera->DoRoll(x - m_nDownX, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
					break;
			}

			m_nDownX = x;
			m_nDownY = y;
			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;
		
		case LC_ACTION_ROLL:
		{
			if (m_nDownX == x)
				break;

			m_ActiveView->m_Camera->DoRoll(x - m_nDownX, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownX = x;
			SystemUpdateFocus(NULL);
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

      SystemUpdateFocus(NULL);
      UpdateAllViews();
    } break;
                */
	}
}

// Check if the mouse is over a different area of the overlay and redraw it.
void Project::MouseUpdateOverlays(View* view, int x, int y)
{
	const float OverlayScale = view->m_OverlayScale;

	if (m_nCurAction == LC_ACTION_MOVE)
	{
		const float OverlayMoveArrowSize = 1.5f;

		int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };
		float Aspect = (float)Viewport[2]/(float)Viewport[3];
		Camera* Cam = view->m_Camera;

		Matrix44 ModelView, Projection;
		ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
		Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

		// Array of points for the arrow edges.
		Vector3 Points[4] =
		{
			Vector3(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]),
			Vector3(OverlayMoveArrowSize * OverlayScale, 0, 0),
			Vector3(0, OverlayMoveArrowSize * OverlayScale, 0),
			Vector3(0, 0, OverlayMoveArrowSize * OverlayScale),
		};

		// Find the rotation from the focused piece if relative snap is enabled.
		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Object* Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPiece())
			{
				float Rot[4];
				((Piece*)Focus)->GetRotation(Rot);

				Matrix33 RotMat;
				RotMat.CreateFromAxisAngle(Vector3(Rot[0], Rot[1], Rot[2]), Rot[3] * LC_DTOR);

				for (int i = 1; i < 4; i++)
					Points[i] = Mul(Points[i], RotMat);
			}
		}

		int i, Mode = -1;
		Vector3 Pt((float)x, (float)y, 0);

		for (i = 1; i < 4; i++)
			Points[i] += Points[0];

		ProjectPoints(Points, 4, ModelView, Projection, Viewport);

		// Check if the mouse is over an arrow.
		for (i = 1; i < 4; i++)
		{
			Vector3 Line = Points[i] - Points[0];
			Vector3 Vec = Pt - Points[0];

			float u = Dot3(Vec, Line) / Line.LengthSquared();

			// Point is outside the line segment.
			if (u < 0.0f || u > 1.0f)
				continue;

			// Closest point in the line segment to the mouse.
			Vector3 Closest = Points[0] + u * Line;

			if ((Closest - Pt).LengthSquared() < 100.0f)
			{
				// If we already know the mouse is close to another axis, select a plane.
				if (Mode != -1)
				{
					if (Mode == LC_OVERLAY_X)
					{
						if (i == 2)
						{
							Mode = LC_OVERLAY_XY;
						}
						else
						{
							Mode = LC_OVERLAY_XZ;
						}
					}
					else
					{
						Mode = LC_OVERLAY_YZ;
					}

					break;
				}
				else
				{
					Mode = LC_OVERLAY_X + i - 1;
				}
			}
		}

		if (Mode == -1)
		{
			Mode = LC_OVERLAY_XYZ;
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
		int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };
		float Aspect = (float)Viewport[2]/(float)Viewport[3];
		Camera* Cam = view->m_Camera;

		Matrix44 ModelView, Projection;
		ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
		Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

		// Unproject the mouse point against both the front and the back clipping planes.
		Vector3 SegStart = UnprojectPoint(Vector3((float)x, (float)y, 0.0f), ModelView, Projection, Viewport);
		Vector3 SegEnd = UnprojectPoint(Vector3((float)x, (float)y, 1.0f), ModelView, Projection, Viewport);

		Vector3 Center(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

		Vector3 Line = SegEnd - SegStart;
		Vector3 Vec = Center - SegStart;

		float u = Dot3(Vec, Line) / Line.LengthSquared();

		// Closest point in the line to the mouse.
		Vector3 Closest = SegStart + u * Line;

		int Mode = -1;
		float Distance = (Closest - Center).Length();
		const float Epsilon = 0.25f * OverlayScale;

		if (Distance > (OverlayRotateRadius * OverlayScale + Epsilon))
		{
			Mode = LC_OVERLAY_XYZ;
		}
		else if (Distance < (OverlayRotateRadius * OverlayScale + Epsilon))
		{
			// 3D rotation unless we're over one of the axis circles.
			Mode = LC_OVERLAY_XYZ;

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
				Camera* Cam = view->m_Camera;
				Vector3 ViewDir = Cam->GetTargetPosition() - Cam->GetEyePosition();

				float u1 = (-b + sqrtf(f)) / (2*a);
				float u2 = (-b - sqrtf(f)) / (2*a);

				Vector3 Intersections[2] =
				{
					Vector3(x1 + u1*(x2-x1), y1 + u1*(y2-y1), z1 + u1*(z2-z1)),
					Vector3(x1 + u2*(x2-x1), y1 + u2*(y2-y1), z1 + u2*(z2-z1))
				};

				for (int i = 0; i < 2; i++)
				{
					Vector3 Dist = Intersections[i] - Center;

					if (Dot3(ViewDir, Dist) > 0.0f)
						continue;

					// Find the rotation from the focused piece if relative snap is enabled.
					if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
					{
						Object* Focus = GetFocusObject();

						if ((Focus != NULL) && Focus->IsPiece())
						{
							float Rot[4];
							((Piece*)Focus)->GetRotation(Rot);

							Matrix33 RotMat;
							RotMat.CreateFromAxisAngle(Vector3(Rot[0], Rot[1], Rot[2]), -Rot[3] * LC_DTOR);

							Dist = Mul(Dist, RotMat);
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
								Mode = LC_OVERLAY_X;
						}
						else
						{
							if (dz < Epsilon)
								Mode = LC_OVERLAY_Z;
						}
					}
					else
					{
						if (dy < dz)
						{
							if (dy < Epsilon)
								Mode = LC_OVERLAY_Y;
						}
						else
						{
							if (dz < Epsilon)
								Mode = LC_OVERLAY_Z;
						}
					}

					if (Mode != LC_OVERLAY_XYZ)
					{
						switch (Mode)
						{
						case LC_OVERLAY_X:
							Dist[0] = 0.0f;
							break;
						case LC_OVERLAY_Y:
							Dist[1] = 0.0f;
							break;
						case LC_OVERLAY_Z:
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
		vw = view->GetWidth();
		vh = view->GetHeight();

		int cx = vx + vw / 2;
		int cy = vy + vh / 2;

		float d = sqrtf((float)((cx - x) * (cx - x) + (cy - y) * (cy - y)));
		float r = min(vw, vh) * 0.35f;

		const float SquareSize = max(8.0f, (vw+vh)/200);

		if ((d < r + SquareSize) && (d > r - SquareSize))
		{
			if ((cx - x < SquareSize) && (cx - x > -SquareSize))
				m_OverlayMode = LC_OVERLAY_Y;

			if ((cy - y < SquareSize) && (cy - y > -SquareSize))
				m_OverlayMode = LC_OVERLAY_X;
		}
		else
		{
			if (d < r)
				m_OverlayMode = LC_OVERLAY_XYZ;
			else
				m_OverlayMode = LC_OVERLAY_Z;
		}
	}
}

void Project::ActivateOverlay()
{
	if ((m_nCurAction == LC_ACTION_MOVE) || (m_nCurAction == LC_ACTION_ROTATE))
	{
		if (GetFocusPosition(m_OverlayCenter))
			m_OverlayActive = true;
		else if (GetSelectionCenter(m_OverlayCenter))
			m_OverlayActive = true;
		else
			m_OverlayActive = false;
	}
	else if ((m_nCurAction == LC_ACTION_ZOOM_REGION) && (m_nTracking == LC_TRACK_START_LEFT))
		m_OverlayActive = true;
	else if (m_nCurAction == LC_ACTION_ROTATE_VIEW)
		m_OverlayActive = true;
	else
		m_OverlayActive = false;

	if (m_OverlayActive)
	{
		m_OverlayMode = LC_OVERLAY_XYZ;
		UpdateOverlayScale();
	}
}

void Project::UpdateOverlayScale()
{
	// TODO: This is not needed, draw the overlays using an ortho matrix.
	if (m_OverlayActive)
	{
		// Calculate the scaling factor by projecting the center to the front plane then
		// projecting a point close to it back.
		int Viewport[4] = { 0, 0, m_ActiveView->GetWidth(), m_ActiveView->GetHeight() };
		float Aspect = (float)Viewport[2]/(float)Viewport[3];
		Camera* Cam = m_ActiveView->m_Camera;

		Matrix44 ModelView, Projection;
		ModelView.CreateLookAt(Cam->GetEyePosition(), Cam->GetTargetPosition(), Cam->GetUpVector());
		Projection.CreatePerspective(Cam->m_fovy, Aspect, Cam->m_zNear, Cam->m_zFar);

		Vector3 Screen = ProjectPoint(m_OverlayCenter, ModelView, Projection, Viewport);
		Screen[0] += 10.0f;
		Vector3 Point = UnprojectPoint(Screen, ModelView, Projection, Viewport);

		Vector3 Dist(Point - m_OverlayCenter);
		m_ActiveView->m_OverlayScale = Dist.Length() * 5.0f;
	}
}
