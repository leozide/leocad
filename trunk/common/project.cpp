// Everything that is a part of a LeoCAD project goes here.
//

#include "lc_global.h"
#include "project.h"

#include "lc_application.h"
#include "lc_model.h"
#include "lc_object.h"
#include "lc_pieceobj.h"
#include "lc_piece.h"
#include "lc_flexpiece.h"
#include "lc_modelref.h"
#include "lc_pivot.h"
#include "lc_camera.h"
#include "lc_light.h"
#include "lc_mesh.h"
#include "lc_scene.h"
#include "lc_colors.h"
#include "preview.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <locale.h>
#include "opengl.h"
#include "matrix.h"
#include "pieceinf.h"
#include "texture.h"
#include "light.h"
#include "group.h"
#include "terrain.h"
#include "image.h"
#include "system.h"
#include "globals.h"
#include "minifig.h"
#include "message.h"
#include "mainwnd.h"
#include "view.h"
#include "library.h"
#include "texfont.h"
#include "algebra.h"
#include "debug.h"

// TODO: temporary function, rewrite.
void SystemUpdateFocus (void* p)
{
  messenger->Dispatch (LC_MSG_FOCUS_CHANGED, p);
}

static void ProjectListener(int Message, void* Data, void* User)
{
	((Project*)User)->HandleMessage(Message, Data);
}

/////////////////////////////////////////////////////////////////////////////
// Project construction/destruction

Project::Project()
{
	int i;

	m_ActiveView = NULL;
	m_ActiveModel = NULL;

	m_Scene = new lcScene(1000, 500, 1000, 500);

	m_bModified = false;
	m_bTrackCancel = false;
	m_nTracking = LC_TRACK_NONE;
	m_pUndoList = NULL;
	m_pRedoList = NULL;
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

	messenger->DecRef();

	delete m_Scene;
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
	SystemUpdateColorList(m_nCurColor);
	SystemUpdateAnimation(false, m_bAddKeys);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_BACKGROUND) != 0, (m_nDetail & LC_DET_FAST) != 0);
	SystemUpdateSnap(m_nSnap);
	SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
	SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
	SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
	UpdateSelection();
	SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
	SystemUpdateModelMenu(m_ModelList, m_ActiveModel);

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
		_strlwr(ext);

		if (strcmp(ext, "lcd") == 0)
			*ptr = 0;
		if (strcmp(ext, "dat") == 0)
			*ptr = 0;
		if (strcmp(ext, "ldr") == 0)
			*ptr = 0;
	}

	strcat(title, " - LeoCAD");

	SystemSetWindowCaption(title);
}

void Project::DeleteContents(bool bUndo)
{
	int i;

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
		m_pTerrain->LoadDefaults((m_nDetail & LC_DET_LINEAR) != 0);
	}

	for (i = 0; i < m_ViewList.GetSize(); i++)
		m_ViewList[i]->SetCamera(NULL);

	// Remove all submodels.
	for (i = 0; i < m_ModelList.GetSize(); i++)
		delete m_ModelList[i];
	m_ModelList.RemoveAll();

	m_ActiveModel = NULL;
	SystemUpdateModelMenu(m_ModelList, m_ActiveModel);

	for (i = 0; i < LC_CONNECTIONS; i++)
	{
		for (int j = 0; j < m_pConnections[i].numentries; j++)
			delete (m_pConnections[i].entries[j].cons);

		delete m_pConnections[i].entries;
		m_pConnections[i].entries = NULL;
		m_pConnections[i].numentries = 0;
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
	SetAction(0);
	m_nCurColor = 0;
	SystemUpdateColorList(m_nCurColor);
	m_bAddKeys = false;
	SystemUpdateAnimation(false, m_bAddKeys);
	m_bUndoOriginal = true;
	SystemUpdateUndoRedo(NULL, NULL);
	m_nDetail = Sys_ProfileLoadInt ("Default", "Detail", LC_DET_BRICKEDGES);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_BACKGROUND) != 0, (m_nDetail & LC_DET_FAST) != 0);
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
	SystemUpdateTime(false, 1, 255);
	m_nScene = Sys_ProfileLoadInt ("Default", "Scene", 0);
	m_nSaveTimer = 0;
	strcpy(m_strHeader, Sys_ProfileLoadString ("Default", "Header", ""));
	strcpy(m_strFooter, Sys_ProfileLoadString ("Default", "Footer", "Page &P"));
	strcpy(m_strBackground, Sys_ProfileLoadString ("Default", "BMP", ""));
	m_pTerrain->LoadDefaults((m_nDetail & LC_DET_LINEAR) != 0);
	m_OverlayActive = false;
	m_PlayingAnimation = false;

	lcModel* Model = new lcModel("Main");
	m_ModelList.Add(Model);
	SetActiveModel(Model);

	for (i = 0; i < m_ViewList.GetSize (); i++)
	{
		m_ViewList[i]->MakeCurrent ();
		RenderInitialize();
	}

	if (cameras)
	{
		m_ActiveModel->ResetCameras();

		for (int i = 0; i < m_ViewList.GetSize(); i++)
			m_ViewList[i]->UpdateCamera();

		SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
		if (m_ActiveView)
			SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
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

	if (fv < 0.6f)
	{
		// Old view format, ignore it.
		m_ActiveModel->ResetCameras();

		for (int i = 0; i < m_ViewList.GetSize(); i++)
			m_ViewList[i]->UpdateCamera();

		double eye[3], target[3];
		file->ReadDouble(&eye, 3);
		file->ReadDouble(&target, 3);
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
		file->ReadLong (&i, 1); //m_nCurStep = i;
	}

	if (fv > 0.8f)
		file->ReadLong (&m_nScene, 1);

	file->ReadLong (&count, 1);
	SystemStartProgressBar(0, count, 1, "Loading project...");
/* FIXME: file load
	while (count--)
	{	
		if (fv > 0.4f)
		{
			char name[9];
			lcPiece* Piece = new lcPiece(NULL);
			Piece->FileLoad(*file, name);
			PieceInfo* pInfo = lcGetPiecesLibrary()->FindPieceInfo(name);
			if (pInfo)
			{
				Piece->SetPieceInfo(pInfo);

				if (bMerge)
				{
					for (lcObject* p = m_ActiveModel->m_Pieces; p; p = p->m_Next)
					{
						if (p->m_Name == Piece->m_Name)
						{
							Piece->SetUniqueName(m_ActiveModel->m_Pieces, pInfo->m_strDescription);
							break;
						}
					}
				}

				if (Piece->m_Name.GetLength() == 0)
					Piece->SetUniqueName(m_ActiveModel->m_Pieces, pInfo->m_strDescription);

				AddPiece(Piece);
				if (!bUndo)
					SystemPieceComboAdd(pInfo->m_strDescription);
			}
			else 
				delete Piece;
		}
		else
		{
			char name[9];
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

			PieceInfo* Info = lcGetPiecesLibrary()->FindPieceInfo(name);
			if (Info != NULL)
			{
				lcPiece* Piece = new lcPiece(Info);

				Piece->SetUniqueName(m_ActiveModel->m_Pieces, Info->m_strDescription);
				Piece->m_TimeShow = step;
				Piece->m_Color = color;

				Matrix mat;
				mat.CreateOld(0,0,0, rot[0],rot[1],rot[2]);
				mat.ToAxisAngle(param);
				Piece->SetPosition(1, false, Vector3(param[0], param[1], param[2]));

//				pPiece->SetGroup((Group*)group);
				AddPiece(Piece);
				SystemPieceComboAdd(Info->m_strDescription);
			}
		}
		SystemStepProgressBar();
	}
*/
	SystemEndProgressBar();

	if (!bMerge)
	{
		if (fv >= 0.4f)
		{
			char buf[512];
			file->Read(&ch, 1);
			if (ch == 0xFF) file->ReadShort (&sh, 1); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
				file->Read(buf, sh);
			m_ActiveModel->m_Author = buf;

			file->Read(&ch, 1);
			if (ch == 0xFF) file->ReadShort (&sh, 1); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
				file->Read(buf, sh);
			m_ActiveModel->m_Description = buf;

			file->Read(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->ReadShort (&sh, 1); else sh = ch;
			if (sh > 255)
				file->Seek(sh, SEEK_CUR);
			else
				file->Read(buf, sh);
			m_ActiveModel->m_Comments = buf;
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
/*
FIXME: groups
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
			i = (int)pGroup->m_pGroup;
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

		lcObject* Piece;
		for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
		{
			i = (int)Piece->GetGroup();
			Piece->SetGroup(NULL);

			if (i > 0xFFFF || i == -1)
				continue;

			for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_pNext)
			{
				if (i == 0)
				{
					Piece->SetGroup(pGroup);
					break;
				}

				i--;
			}
		}
*/
		RemoveEmptyGroups();
	}

	if (!bMerge)
	{
		if (fv >= 0.6f)
		{
			if (fv < 1.0f)
			{
				file->ReadLong (&i, 1);
				// m_nViewportMode = i;
			}
			else
			{
				unsigned char dummy;
				file->ReadByte(&dummy, 1); // m_nViewportMode
				file->ReadByte(&dummy, 1); // m_nActiveViewport
			}

			file->ReadLong (&count, 1);
			for (i = 0; i < count; i++)
			{
				lcCamera* Camera = new lcCamera();
				Camera->CreateCamera(lcMin(i, LC_CAMERA_USER), true);
				Camera->Update(m_ActiveModel->m_CurFrame);
				Camera->SetUniqueName(m_ActiveModel->m_Cameras, "Camera");
				m_ActiveModel->AddCamera(Camera);

				if (i < 4 && fv == 0.6f)
				{
					if (m_ViewList.GetSize() > i)
						m_ViewList[i]->SetCamera(Camera);
				}
			}
/* FIXME: file load
			if (count < 7)
			{
				lcCamera* Camera = new lcCamera();
				for (i = 0; i < count; i++)
					Camera->FileLoad(*file);
				delete Camera;
			}
			else
			{
				for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
					Camera->FileLoad(*file);
			}
*/
		}

		if (fv >= 0.7f)
		{
			if (fv >= 1.5f)
			{
				String Layout;
				file->ReadString(Layout);
				main_window->SetViewLayout(Layout);
			}
			else
			{
				for (count = 0; count < 4; count++)
				{
					file->ReadLong (&i, 1);

					if (m_ViewList.GetSize() > count)
						m_ViewList[count]->SetCamera(m_ActiveModel->GetCamera(i));
				}
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
				file->ReadLong (&i, 1); //m_bAnimation = (i != 0);
				file->ReadLong (&i, 1); m_bAddKeys = (i != 0);
				file->ReadByte (&m_nFPS, 1);
				file->ReadLong (&i, 1); //m_nCurFrame = i;
				file->ReadShort (&sh, 1); // m_nTotalFrames
				file->ReadLong (&i, 1); //m_nGridSize = i;
				file->ReadLong (&i, 1); //m_nMoveSnap = i;
			}
			else
			{
				unsigned short sh;
				file->ReadByte (&ch, 1); //m_bAnimation = (ch != 0);
				file->ReadByte (&ch, 1); m_bAddKeys = (ch != 0);
				file->ReadByte (&m_nFPS, 1);
				file->ReadShort (&sh, 1); // m_nCurFrame
				file->ReadShort (&sh, 1); // m_nTotalFrames
				file->ReadShort (&sh, 1); // m_nGridSize
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

	for (i = 0; i < m_ViewList.GetSize (); i++)
	{
		m_ViewList[i]->MakeCurrent ();
		RenderInitialize();
	}
	CalculateStep();
	if (!bUndo)
		SelectAndFocusNone(false);
	if (!bMerge)
		SystemUpdateFocus(NULL);
	SetAction(action);
	SystemUpdateColorList(m_nCurColor);
	SystemUpdateAnimation(false, m_bAddKeys);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_BACKGROUND) != 0, (m_nDetail & LC_DET_FAST) != 0);
	SystemUpdateSnap(m_nSnap);
	SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
	SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
	SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
	UpdateSelection();
	SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
	UpdateAllViews ();

	return true;
}

void Project::FileSave(File* file, bool bUndo)
{
/* FIXME: file save
	float ver_flt = 1.5f; // LeoCAD 0.76 - (and this should have been an integer).
	unsigned long rgb;
	unsigned char ch;
	unsigned short sh;
	int i;

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
	i = m_ActiveModel->m_CurFrame; file->WriteLong (&i, 1);
	file->WriteLong (&m_nScene, 1);

	lcObject* Piece;
	for (i = 0, Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
		i++;
	file->WriteLong(&i, 1);

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
		Piece->FileSave(*file, m_pGroups);

	ch = strlen(m_ActiveModel->m_Author);
	file->Write(&ch, 1);
	file->Write(m_ActiveModel->m_Author, ch);
	ch = strlen(m_ActiveModel->m_Description);
	file->Write(&ch, 1);
	file->Write(m_ActiveModel->m_Description, ch);
	ch = strlen(m_ActiveModel->m_Comments);
	file->Write(&ch, 1);
	file->Write(m_ActiveModel->m_Comments, ch);

	Group* pGroup;
	for (i = 0, pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
		i++;
	file->WriteLong (&i, 1);

	for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
		pGroup->FileSave(file, m_pGroups);

	unsigned char dummy = 255; 
	file->WriteByte(&dummy, 1);
	file->WriteByte(&dummy, 1);

	lcObject* Camera;
	for (i = 0, Camera = m_ActiveModel->m_Cameras; Camera; Camera = Camera->m_Next)
		i++;
	file->WriteLong (&i, 1);

	for (Camera = m_ActiveModel->m_Cameras; Camera; Camera = Camera->m_Next)
		Camera->FileSave(*file);

	file->WriteString(main_window->GetViewLayout(true));

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
	ch = false;//m_bAnimation;
	file->Write(&ch, 1);
	ch = m_bAddKeys;
	file->WriteByte (&ch, 1);
	file->WriteByte (&m_nFPS, 1);
	sh = 1; file->WriteShort (&sh, 1); // m_nCurFrame
	sh = 1; file->WriteShort (&sh, 1); // m_nTotalFrames
	sh = 10; file->WriteShort (&sh, 1); // m_nGridSize
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

			i = m_ActiveModel->m_CurFrame;
			CreateImages(image, 120, 100, i, i, false);
			image[0].FileSave (*file, &opts);
			delete []image;
		}

		file->WriteLong (&pos, 1);
		m_nSaveTimer = 0;
	}
*/
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

void Project::FileReadLDraw(File* file, Matrix* prevmat, int* nOk, int DefColor, int* nStep, PtrArray<File>& FileArray, const String& FilePath)
{
	char buf[1024];

	// Save file offset.
	u32 Offset = file->GetPosition();
	file->Seek(0, SEEK_SET);

	while (file->ReadLine(buf, 1024))
	{
		_strupr(buf);
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
			if (strlen(tmp) < 9)
			{
				/* FIXME file load
				char name[9];
				strcpy(name, tmp);

				PieceInfo* Info = lcGetPiecesLibrary()->FindPieceInfo(name);
				if (Info != NULL)
				{
					float x, y, z, rot[4];
					lcPiece* Piece = new lcPiece(Info);
					read = false;

					tmpmat.GetTranslation(&x, &y, &z);
					Piece->Initialize(x, y, z, *nStep, cl);
					Piece->SetUniqueName(m_ActiveModel->m_Pieces, Info->m_strDescription);
					AddPiece(Piece);
					tmpmat.ToAxisAngle(rot);
					Piece->SetRotation(1, false, Vector4(rot[0], rot[1], rot[2], rot[3]));
					SystemPieceComboAdd(Info->m_strDescription);
					(*nOk)++;
				}
				*/
			}

			// Check for MPD files first.
			if (read)
			{
				for (int i = 0; i < FileArray.GetSize(); i++)
				{
					if (_stricmp(FileArray[i]->GetFileName(), pn) == 0)
					{
						FileReadLDraw(FileArray[i], &tmpmat, nOk, cl, nStep, FileArray, FilePath);
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
					// Read from the current directory.
					FileReadLDraw(&tf, &tmpmat, nOk, cl, nStep, FileArray, FilePath);
					read = false;
				}
				else
				{
					// Try the file's directory.
					String Path = FilePath + pn;

					if (tf.Open(Path, "rt"))
					{
						// Read from the current directory.
						FileReadLDraw(&tf, &tmpmat, nOk, cl, nStep, FileArray, FilePath);
						read = false;
					}
					else
					{
						console.PrintWarning("Could not find %s.\n", pn);
					}
				}
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
		_strlwr(ext);

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
		_strlwr(ext);
	}

	if ((strcmp(ext, "dat") == 0) || (strcmp(ext, "ldr") == 0))
	{
		const int col[28] = { 4,25,2,10,1,9,14,15,8,0,6,13,13,334,36,44,34,42,33,41,46,47,7,382,6,13,11,383 };
		lcPieceObject* pPiece;
		int i, steps = GetLastStep();
		char buf[256], *ptr;

		ptr = strrchr(m_strPathName, '\\');
		if (ptr == NULL)
			ptr = strrchr(m_strPathName, '/');
		if (ptr == NULL)
			ptr = m_strPathName;
		else
			ptr++;

		sprintf(buf, "0 Model exported from LeoCAD\r\n0 Original name: %s\r\n", ptr);
		if (strlen(m_ActiveModel->m_Author) != 0)
		{
			strcat(buf, "0 Author: ");
			strcat(buf, m_ActiveModel->m_Author);
			strcat(buf, "\r\n");
		}
		strcat(buf, "\r\n");
		file.Write(buf, strlen(buf));

		for (i = 1; i <= steps; i++)
		{
			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPieceObject*)pPiece->m_Next)
			{
				if ((pPiece->IsVisible(i)) && (pPiece->m_TimeShow == i))
					pPiece->ExportLDraw(file);
			}

			if (i != steps)
				file.Write("0 STEP\r\n", 8);
		}
		file.Write("0\r\n", 3);
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
	SetModifiedFlag(false); // make clean
	LoadDefaults(true);
	CheckPoint("");

	messenger->Dispatch (LC_MSG_FOCUS_CHANGED, NULL);

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

bool Project::OnOpenDocument(const char* PathName)
{
	FileDisk file;
	bool bSuccess = false;

	if (!file.Open(PathName, "rb"))
	{
		char Message[LC_MAXPATH + 64];

		sprintf(Message, "Failed to open %s for reading.", PathName);
		main_window->MessageBox(Message, "LeoCAD", LC_MB_OK | LC_MB_ICONERROR);

		return false;
	}

	char ext[4];
	memset(ext, 0, 4);
	const char* ptr = strrchr(PathName, '.');
	if (ptr != NULL)
	{
		strncpy(ext, ptr+1, 3);
		_strlwr(ext);
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

			// Extract the file's directory.
			String FilePath(PathName);
			int s1 = FilePath.ReverseFind('\\');
			int s2 = FilePath.ReverseFind('/');

			if (s2 > s1)
				s1 = s2;

			if (s1 == -1)
				FilePath = "";
			else
				FilePath[s1+1] = 0;

			if (mpdfile)
				FileReadLDraw(FileArray[0], &mat, &ok, m_nCurColor, &step, FileArray, FilePath);
			else
				FileReadLDraw(&file, &mat, &ok, m_nCurColor, &step, FileArray, FilePath);

			m_ActiveModel->m_CurFrame = step;
			SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
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
		char Message[LC_MAXPATH + 64];

		sprintf(Message, "Failed to load %s.", PathName);
		main_window->MessageBox(Message, "LeoCAD", LC_MB_OK | LC_MB_ICONERROR);

		DeleteContents(false);   // remove failed contents
		return false;
	}

	CheckPoint("");
	m_nSaveTimer = 0;

	SetModifiedFlag(false);     // start off with unmodified

	return true;
}

void Project::SetPathName(const char* lpszPathName, bool bAddToMRU)
{
	strcpy(m_strPathName, lpszPathName);

	// always capture the complete file name including extension (if present)
	const char* lpszTemp = lpszPathName;
	for (const char* lpsz = lpszPathName; *lpsz != '\0'; lpsz++)
	{
		// remember last directory/drive separator
		if (*lpsz == '\\' || *lpsz == '/' || *lpsz == ':')
			lpszTemp = lpsz + 1;
	}

	// set the document title based on path name
	SetTitle(lpszTemp);

	// add it to the file MRU list
	if (bAddToMRU)
		main_window->AddToMRU (lpszPathName);
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
	m_ViewList.Add(pView);

	pView->MakeCurrent();
	RenderInitialize();
}

void Project::RemoveView (View* pView)
{
	if (pView == m_ActiveView)
		m_ActiveView = NULL;
	m_ViewList.RemovePointer(pView);
}

void Project::UpdateAllViews()
{
	for (int i = 0; i < m_ViewList.GetSize(); i++)
		m_ViewList[i]->Redraw(true);
}

// Returns true if the active view changed.
bool Project::SetActiveView(View* view)
{
	if (view == m_ActiveView)
		return false;

	lcCamera* OldCamera = NULL;
	View* OldView = m_ActiveView;
	m_ActiveView = view;

	if (OldView)
	{
		OldView->Redraw();
		OldCamera = OldView->GetCamera();
	}

	if (view)
	{
		view->Redraw();
		SystemUpdateCurrentCamera(OldCamera, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Project rendering

void Project::Render(View* view, bool AllowFast, bool Interface)
{
#ifdef LC_PROFILE
	memset(&g_RenderStats, 0, sizeof(g_RenderStats));

	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	u64 Start = li.QuadPart;
#endif

	// Setup the viewport.
	glViewport(0, 0, view->GetWidth(), view->GetHeight());

	// Render the background.
	RenderBackground(view);

	// Load the camera.
	float w = (float)view->GetWidth();
	float h = (float)view->GetHeight();
	float ratio = w/h;

	view->GetCamera()->LoadProjection(ratio);

	// Render 3D objects.
	if (AllowFast && ((m_nDetail & LC_DET_FAST) && (m_nTracking != LC_TRACK_NONE)))
		RenderSceneBoxes(view);
	else
		RenderScene(view);

	// Render user interface elements.
	if (Interface)
	{
		RenderInterface(view);
	}

	if (m_PlayingAnimation)
	{
		if (view == m_ActiveView)
		{
			m_LastFrameTime = SystemGetTicks();
		}
	}

	glFlush();
	glFinish();

#ifdef LC_PROFILE
	QueryPerformanceCounter(&li);
	u64 End = li.QuadPart;

	QueryPerformanceFrequency(&li);
	g_RenderStats.RenderMS = (int)((End - Start) / (li.QuadPart / 1000.0));

	lcRenderProfileStats(view);

	char szMsg[30];
	static int FrameCount = 0;
	FrameCount++;
	sprintf(szMsg, "%d - %d ms", FrameCount, g_RenderStats.RenderMS);
	AfxGetMainWnd()->SetWindowText(szMsg);
#endif
}

void Project::RenderBackground(View* view)
{
	if (m_nScene & (LC_SCENE_GRADIENT|LC_SCENE_BG))
	{
		glClear(GL_DEPTH_BUFFER_BIT);

		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_FOG);

		float w = (float)view->GetWidth();
		float h = (float)view->GetHeight();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375, 0.375, 0.0);

		// Draw gradient quad.
		if (m_nScene & LC_SCENE_GRADIENT)
		{
			glShadeModel(GL_SMOOTH);

			glBegin(GL_QUADS);
			glColor3fv(m_fGradient1);
			glVertex2f(w-1, h-1);
			glVertex2f(0, h-1);
			glColor3fv(m_fGradient2);
			glVertex2f(0, 0);
			glVertex2f(w-1, 0);
			glEnd();

			glShadeModel(GL_FLAT);
		}

		// Draw the background picture.
		if (m_nScene & LC_SCENE_BG)
		{
			glEnable(GL_TEXTURE_2D);

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			m_pBackground->MakeCurrent();

			float tw = 1.0f, th = 1.0f;

			if (m_nScene & LC_SCENE_BG_TILE)
			{
				tw = w/m_pBackground->m_nWidth;
				th = h/m_pBackground->m_nHeight;
			}

			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(0, h-1);
			glTexCoord2f(tw, 0);
			glVertex2f(w-1, h-1);
			glTexCoord2f(tw, th); 
			glVertex2f(w-1, 0);
			glTexCoord2f(0, th);
			glVertex2f(0, 0);
			glEnd();

			glDisable(GL_TEXTURE_2D);
		}

		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glClearColor(m_fBackground[0], m_fBackground[1], m_fBackground[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
}

void Project::RenderScene(View* view)
{
	glDepthMask(GL_TRUE);
	glLineWidth(m_fLineWidth);

	// Draw the base grid.
	if (m_nSnap & LC_DRAW_GRID)
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_FOG);
		glShadeModel(GL_FLAT);

		lcCamera* Camera = view->GetCamera();

		if (Camera->IsSide() && Camera->IsOrtho())
		{
			Vector3 frontvec = Vector3(Camera->m_ViewWorld[2]);
			float Aspect = (float)view->GetWidth()/(float)view->GetHeight();
			float ymax, ymin, xmin, xmax, x, y;

			ymax = (frontvec.Length())*sinf(DTOR*Camera->m_FOV/2);
			ymin = -ymax;
			xmin = ymin * Aspect;
			xmax = ymax * Aspect;

			// Calculate camera offset.
			Matrix44 ModelView = Camera->m_WorldView;
			Vector3 offset = Mul30(Camera->m_WorldPosition, ModelView);

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glTranslatef(-offset[0], -offset[1], 0.0f);

			xmin += offset[0];
			xmax += offset[0];
			ymin += offset[1];
			ymax += offset[1];

			float z = -Camera->m_FarDist;

			Vector3 up = Abs(Vector3(Camera->m_ViewWorld[1]));
			float incx = 0.8f, incy;

			if ((up[2] > up[0]) && (up[2] > up[1]))
				incy = 0.96f;
			else
				incy = 0.8f;

			float scale = 1;

			// Calculate minor line increment.
			while ((ymax - ymin) / (incy * scale) > 100)
				scale *= 2;

			while ((xmax - xmin) / (incx * scale) > 100)
				scale *= 2;

			incx *= scale;
			incy *= scale;

			glBegin(GL_LINES);

			// Draw minor lines.
			glColor3f(0.75f, 0.75f, 0.75f);
			for (x = (int)(xmin / incx) * incx; x < xmax; x += incx)
			{
				glVertex3f(x, ymin, z);
				glVertex3f(x, ymax, z);
			}

			for (y = (int)(ymin / incy) * incy; y < ymax; y += incy)
			{
				glVertex3f(xmin, y, z);
				glVertex3f(xmax, y, z);
			}

			// Reset increments and scale.
			incx /= scale;
			incy /= scale;
			scale = 1;

			// Calculate major line increment.
			while ((ymax - ymin) / (incy * scale) > 10)
				scale *= 2;

			while ((xmax - xmin) / (incx * scale) > 10)
				scale *= 2;

			incx *= scale;
			incy *= scale;

			// Draw major lines.
			glColor3f(0.0f, 0.0f, 0.0f);
			for (x = (int)(xmin / incx) * incx; x < xmax; x += incx)
			{
				glVertex3f(x, ymin, z);
				glVertex3f(x, ymax, z);
			}

			for (y = (int)(ymin / incy) * incy; y < ymax; y += incy)
			{
				glVertex3f(xmin, y, z);
				glVertex3f(xmax, y, z);
			}

			glEnd();

			glPopMatrix();
		}
		else
		{
			// Calculate view matrices.
			float Aspect = (float)view->GetWidth()/(float)view->GetHeight();

			Matrix44 ModelView = Camera->m_WorldView;
			Matrix44 Projection = CreatePerspectiveMatrix(Camera->m_FOV, Aspect, Camera->m_NearDist, Camera->m_FarDist);

			// Unproject edge center points to world space.
			int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() }, i;

			Vector3 Points[10] =
			{
				Vector3(0, (float)Viewport[3] / 2, 0),
				Vector3(0, (float)Viewport[3] / 2, 1),
				Vector3((float)Viewport[2] / 2, 0, 0),
				Vector3((float)Viewport[2] / 2, 0, 1),
				Vector3((float)Viewport[2], (float)Viewport[3] / 2, 0),
				Vector3((float)Viewport[2], (float)Viewport[3] / 2, 1),
				Vector3((float)Viewport[2] / 2, (float)Viewport[3], 0),
				Vector3((float)Viewport[2] / 2, (float)Viewport[3], 1),
				Vector3((float)Viewport[2] / 2, (float)Viewport[3] / 2, 0),
				Vector3((float)Viewport[2] / 2, (float)Viewport[3] / 2, 1),
			};

			UnprojectPoints(Points, 10, ModelView, Projection, Viewport);

			// Intersect lines with base plane.
			Vector3 Intersections[5];

			for (i = 0; i < 5; i++)
				LinePlaneIntersection(&Intersections[i], Points[i*2], Points[i*2+1], Vector4(0, 0, 1, 0));

			// Find the smallest edge length.
			float MinSize = FLT_MAX;
			for (i = 0; i < 4; i++)
			{
				float d1 = LinePointMinDistance(Intersections[4], Intersections[i], i == 0 ? Intersections[3] : Intersections[i-1]);

				if (d1 < MinSize)
					MinSize = d1;
			}

			// Draw grid.
			float inc = 0.8f;
			float z = 0.0f;
			float xsteps = ceilf(MinSize / inc);

			while (xsteps > 10)
			{
				xsteps = floorf(xsteps / 2);
				inc *= 2;
			}

			float ysteps = xsteps;

			float xmin = floorf((Intersections[4][0] + (10 * inc)) / (20 * inc)) * (20 * inc) - (xsteps * inc);
			float ymin = floorf((Intersections[4][1] + (10 * inc)) / (20 * inc)) * (20 * inc) - (ysteps * inc);
			xmin -= inc * 10;
			ymin -= inc * 10;

			xsteps *= 2;
			ysteps *= 2;

			if (fmodf(Intersections[4][0] + (10 * inc), (20 * inc)) > 0.5f)
				xsteps += 20;
			else
				xsteps += 10;

			if (fmodf(Intersections[4][1] + (10 * inc), (20 * inc)) > 0.5f)
				ysteps += 20;
			else
				ysteps += 10;

			float xmax = xmin + (xsteps + 1) * inc;
			float ymax = ymin + (ysteps + 1) * inc;

			glBegin(GL_LINES);

			// Draw lines.
			glColor3f(0.75f, 0.75f, 0.75f);

			for (int x = 0; x < xsteps + 2; x++)
			{
				glVertex3f(xmin + x * inc, ymin, z);
				glVertex3f(xmin + x * inc, ymax, z);
			}

			for (int y = 0; y < ysteps + 2; y++)
			{
				glVertex3f(xmin, ymin + y * inc, z);
				glVertex3f(xmax, ymin + y * inc, z);
			}

			glEnd();

		}
	}

	// Setup lights.
	if (m_nDetail & LC_DET_LIGHTING)
	{
		glEnable(GL_LIGHTING);

		int index = 0;
		lcObject* Light;

		for (Light = m_ActiveModel->m_Lights; Light; Light = Light->m_Next, index++)
			((lcLight*)Light)->Setup(index);
	}

	// Set render states.
	if (m_nScene & LC_SCENE_FOG)
	{
		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE, GL_EXP);
		glFogf(GL_FOG_DENSITY, m_fFogDensity);
		glFogfv(GL_FOG_COLOR, m_fFogColor);
	}
	else
		glDisable(GL_FOG);

	if (m_nDetail & LC_DET_SMOOTH)
		glShadeModel(GL_SMOOTH);

	// Render the terrain.
	if (m_nScene & LC_SCENE_FLOOR)
	{
		float w = (float)view->GetWidth();
		float h = (float)view->GetHeight();
		float ratio = w/h;

		m_pTerrain->Render(view->GetCamera(), ratio);
	}

	// TODO: Build the render lists outside of the render function and only sort translucent sections here.

	// Build a list for opaque and another for translucent mesh sections.
	m_Scene->m_OpaqueSections.RemoveAll();
	m_Scene->m_TranslucentSections.RemoveAll();
	m_Scene->m_WorldView = view->GetCamera()->m_WorldView;

	for (lcObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
	{
		if (!Piece->IsVisible(m_ActiveModel->m_CurFrame))
			continue;

		Piece->AddToScene(m_Scene, LC_COLOR_DEFAULT);
	}

	// Add piece preview.
	if (m_nCurAction == LC_ACTION_INSERT)
	{
		Vector3 Pos;
		Vector4 Rot;
		GetPieceInsertPosition(m_nDownX, m_nDownY, Pos, Rot);

		Matrix44 ModelWorld;
		ModelWorld = MatrixFromAxisAngle(Rot);
		ModelWorld.SetTranslation(Pos);

		g_App->m_PiecePreview->m_Selection->m_ModelWorld = ModelWorld; // FIXME: preview piece hack
		g_App->m_PiecePreview->m_Selection->AddToScene(m_Scene, m_nCurColor);
		g_App->m_PiecePreview->m_Selection->m_ModelWorld = IdentityMatrix44();
	}

	m_Scene->Render();

	if (m_nDetail & LC_DET_LIGHTING)
	{
		glDisable(GL_LIGHTING);

		int index = 0;
		lcObject* Light;
		
		for (Light = m_ActiveModel->m_Lights; Light; Light = Light->m_Next, index++)
			glDisable ((GLenum)(GL_LIGHT0+index));
	}

	// Draw cameras and lights.
	for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
	{
		if ((Camera != view->GetCamera()) && Camera->IsVisible(m_ActiveModel->m_CurFrame))
			Camera->Render();
	}

/* FIXME: render lights
	for (lcObject* Light = m_ActiveModel->m_Lights; Light; Light = Light->m_Next)
	{
		if (Light->IsVisible(m_ActiveModel->m_CurFrame))
			Light->Render(m_fLineWidth);
	}
*/
#ifdef LC_DEBUG
	RenderDebugPrimitives();
#endif
}

void Project::RenderSceneBoxes(View* view)
{
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glShadeModel(GL_FLAT);
//	glEnable (GL_CULL_FACE);

	if ((m_nDetail & LC_DET_BOX_FILL) == 0)
	{
		if ((m_nDetail & LC_DET_HIDDEN_LINE) != 0)
		{
			// Wireframe with hidden lines removed
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			RenderBoxes(false);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			RenderBoxes(true);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else
		{
			// Wireframe
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			RenderBoxes(true);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}
	else
	{
		RenderBoxes(true);
	}
}

void Project::RenderInterface(View* view)
{
	// Set render states.
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glShadeModel(GL_FLAT);
	glLineWidth(1.0f);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	// Start with a new Z buffer.
	glClear(GL_DEPTH_BUFFER_BIT);

	float w = (float)view->GetWidth();
	float h = (float)view->GetHeight();

	// Render axis icon.
	if (m_nSnap & LC_DRAW_AXIS)
	{
		Matrix44 Mats[3];
		Mats[0] = view->GetCamera()->m_WorldView;
		Mats[1] = Matrix44(Mats[0][1], Mats[0][0], Mats[0][2], Mats[0][3]);
		Mats[2] = Matrix44(Mats[0][2], Mats[0][1], Mats[0][0], Mats[0][3]);

		Vector3 pts[3] = { Vector3(Mats[0][0]) * 20, Vector3(Mats[0][1]) * 20, Vector3(Mats[0][2]) * 20 };

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, w, 0, h, -50, 50);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(25.375f, 25.375f, 0.0f);

		// Draw the arrows.
		for (int i = 0; i < 3; i++)
		{
			switch (i)
			{
			case 0:
				glColor3f(0.8f, 0.0f, 0.0f);
				break;
			case 1:
				glColor3f(0.0f, 0.8f, 0.0f);
				break;
			case 2:
				glColor3f(0.0f, 0.0f, 0.8f);
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
				Vector3 pt(12.0f, cosf(LC_2PI * j / 8) * 3.0f, sinf(LC_2PI * j / 8) * 3.0f);
				pt = Mul30(pt, Mats[i]);
				glVertex3fv(pt);
			}
			glEnd();
		}

		// Draw the text.
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_pScreenFont->MakeCurrent();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);

		glBegin(GL_QUADS);
		glColor3f(0, 0, 0);
		m_pScreenFont->PrintText(pts[0][0], pts[0][1], 40.0f, "X");
		m_pScreenFont->PrintText(pts[1][0], pts[1][1], 40.0f, "Y");
		m_pScreenFont->PrintText(pts[2][0], pts[2][1], 40.0f, "Z");
		glEnd();

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	// Render overlays.
	if (m_OverlayActive)
		RenderOverlays(view);

	// Draw the selection rectangle.
	if ((m_nCurAction == LC_ACTION_SELECT) && (m_nTracking == LC_TRACK_LEFT))
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375, 0.375, 0.0);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(5, 0x5555);
		glColor3f(0, 0, 0);

		float pt1x = (float)(m_nDownX);
		float pt1y = (float)(m_nDownY);
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

	// Setup the viewport.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0);

	// Draw border.
	glLineWidth(1.0f);
	if (view == m_ActiveView)
		glColor3f(1.0f, 0.0f, 0.0f);
	else
		glColor3f(0.0f, 0.0f, 0.0f);

	glBegin(GL_LINE_LOOP);
	glVertex2f(0, 0);
	glVertex2f(w-1, 0);
	glVertex2f(w-1, h-1);
	glVertex2f(0, h-1);
	glEnd();

	// Draw camera name.
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	m_pScreenFont->MakeCurrent();
	glEnable(GL_ALPHA_TEST);

	glColor3f(0, 0, 0);
	glBegin(GL_QUADS);
	m_pScreenFont->PrintText(3, h - 6, 0.0f, view->GetCamera()->m_Name);
	glEnd();

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}

void Project::RenderOverlays(View* view)
{
	const float OverlayScale = view->m_OverlayScale;

	if (m_nCurAction == LC_ACTION_MOVE)
	{
		const float OverlayMovePlaneSize = 0.5f;
		const float OverlayMoveArrowSize = 1.5f;
		const float OverlayMoveArrowCapSize = 0.9f;
		const float OverlayMoveArrowCapRadius = 0.1f;

		glDisable(GL_DEPTH_TEST);

		// Find the rotation from the focused piece if relative snap is enabled.
		lcObject* Focus = NULL;
		Matrix44 Rot;

		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPieceObject())
			{
				Rot = ((lcPieceObject*)Focus)->m_ModelWorld;
				Rot.SetTranslation(Vector3(0, 0, 0));
			}
			else
				Focus = NULL;
		}

		// Draw a quad if we're moving on a plane.
		if ((m_OverlayMode == LC_OVERLAY_XY) || (m_OverlayMode == LC_OVERLAY_XZ) || (m_OverlayMode == LC_OVERLAY_YZ))
		{
			glPushMatrix();
			glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

			if (Focus)
				glMultMatrixf(Rot);

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
					glColor3f(0.8f, 0.8f, 0.0f);
				else
					glColor3f(0.8f, 0.0f, 0.0f);
				break;
			case 1:
				if ((m_OverlayMode == LC_OVERLAY_Y) || (m_OverlayMode == LC_OVERLAY_XY) || (m_OverlayMode == LC_OVERLAY_YZ))
					glColor3f(0.8f, 0.8f, 0.0f);
				else
					glColor3f(0.0f, 0.8f, 0.0f);
				break;
			case 2:
				if ((m_OverlayMode == LC_OVERLAY_Z) || (m_OverlayMode == LC_OVERLAY_XZ) || (m_OverlayMode == LC_OVERLAY_YZ))
					glColor3f(0.8f, 0.8f, 0.0f);
				else
					glColor3f(0.0f, 0.0f, 0.8f);
				break;
			}

			glPushMatrix();
			glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

			if (Focus)
				glMultMatrixf(Rot);

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

		lcCamera* Cam = view->GetCamera();
		Matrix44 Mat;
		int j;

		// Find the rotation from the focused piece if relative snap is enabled.
		lcObject* Focus = NULL;
		Matrix44 Rot;

		if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
		{
			Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPieceObject())
			{
				Rot = ((lcPieceObject*)Focus)->m_ModelWorld;
				Rot.SetTranslation(Vector3(0, 0, 0));
			}
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
					glMultMatrixf(Rot);

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

		Mat = Cam->m_ViewWorld;
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

		Vector3 ViewDir = Vector3(Cam->m_ViewWorld[2]);

		// Transform ViewDir to local space.
		if (Focus)
		{
			Matrix33 WorldModel = RotTranInverse(Rot);
			ViewDir = Mul30(ViewDir, WorldModel);
		}

		glTranslatef(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

		if (Focus)
			glMultMatrixf(Rot);

		// Draw each axis circle.
		for (int i = 0; i < 3; i++)
		{
			if (m_OverlayMode == LC_OVERLAY_X + i)
			{
				glColor3f(0.8f, 0.8f, 0.0f);
			}
			else
			{
				switch (i)
				{
				case 0:
					glColor3f(0.8f, 0.0f, 0.0f);
					break;
				case 1:
					glColor3f(0.0f, 0.8f, 0.0f);
					break;
				case 2:
					glColor3f(0.0f, 0.0f, 0.8f);
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

					Rot = MatrixFromAxisAngle(Normal, LC_PI * 0.15f);
					Arrow = Mul(Tangent, Rot) * OverlayRotateArrowCapSize;

					glVertex3f(Tip[0], Tip[1], Tip[2]);
					glVertex3f(Tip[0] - Arrow[0], Tip[1] - Arrow[1], Tip[2] - Arrow[2]);

					Rot = MatrixFromAxisAngle(Normal, -LC_PI * 0.15f);
					Arrow = Mul(Tangent, Rot) * OverlayRotateArrowCapSize;

					glVertex3f(Tip[0], Tip[1], Tip[2]);
					glVertex3f(Tip[0] - Arrow[0], Tip[1] - Arrow[1], Tip[2] - Arrow[2]);

					glEnd();
				}

				// Draw text.
				if (view == m_ActiveView)
				{
					GLdouble ScreenX, ScreenY, ScreenZ;
					GLdouble ModelMatrix[16], ProjMatrix[16];
					GLint Vp[4];

					glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
					glGetDoublev(GL_PROJECTION_MATRIX, ProjMatrix);
					glGetIntegerv(GL_VIEWPORT, Vp);

					gluProject(0, 0, 0, ModelMatrix, ProjMatrix, Vp, &ScreenX, &ScreenY, &ScreenZ);

					glMatrixMode(GL_PROJECTION);
					glPushMatrix();
					glLoadIdentity();
					glOrtho(0, Vp[2], 0, Vp[3], -1, 1);
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
					m_pScreenFont->PrintText((float)ScreenX - Vp[0] - (cx / 2), (float)ScreenY - Vp[1] + (cy / 2), 0.0f, buf);
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
	else if (m_nCurAction == LC_ACTION_ORBIT)
	{
		float w = (float)view->GetWidth();
		float h = (float)view->GetHeight();

		glViewport(0, 0, view->GetWidth(), view->GetHeight());
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375f, 0.375f, 0.0f);

		glDisable(GL_DEPTH_TEST);
		glColor3f(0, 0, 0);

		// Draw circle.
		glBegin(GL_LINE_LOOP);

		float r = min(w, h) * 0.35f;
		float cx = w / 2.0f;
		float cy = h / 2.0f;

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
		float w = (float)view->GetWidth();
		float h = (float)view->GetHeight();

		glViewport(0, 0, view->GetWidth(), view->GetHeight());
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w, 0, h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.375f, 0.375f, 0.0f);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(5, 0x5555);
		glColor3f(0, 0, 0);

		float pt1x = (float)(m_nDownX);
		float pt1y = (float)(m_nDownY);
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

// bHilite - Draws focus/selection, not used for the 
// first rendering pass if remove hidden lines is enabled
void Project::RenderBoxes(bool bHilite)
{
	/* FIXME: render boxes
	Piece* pPiece;

	for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
		if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
			pPiece->RenderBox(bHilite, m_fLineWidth);
	*/
}

// Initialize OpenGL
void Project::RenderInitialize()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);

	glDrawBuffer(GL_BACK);
	glCullFace(GL_BACK);
	glDisable (GL_CULL_FACE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	if (m_nDetail & LC_DET_DITHER)
		glEnable(GL_DITHER);
	else
		glDisable(GL_DITHER);

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
		m_pTerrain->LoadTexture((m_nDetail & LC_DET_LINEAR) != 0);

	if (m_nScene & LC_SCENE_BG)
		if (!m_pBackground->LoadFromFile(m_strBackground, (m_nDetail & LC_DET_LINEAR) != 0))
		{
			m_nScene &= ~LC_SCENE_BG;
//			AfxMessageBox ("Could not load background");
		}
}

/////////////////////////////////////////////////////////////////////////////
// Project functions

void Project::AddModel(lcModel* Model)
{
	m_ModelList.Add(Model);
}

void Project::DeleteModel(lcModel* DeleteModel)
{
	// Make sure we aren't deleting the last model.
	if (m_ModelList.GetSize() < 2)
	{
		SystemDoMessageBox("Cannot delete model, projects must have at least 1 model.", LC_MB_OK | LC_MB_ICONINFORMATION);
		return;
	}

	// Ask the user if he really wants to delete this model.
	char Text[1024];
	sprintf(Text, "Are you sure you want to delete the model \"%s\"?", (char*)DeleteModel->m_Name);

	if (SystemDoMessageBox(Text, LC_MB_YESNO | LC_MB_ICONQUESTION) != LC_YES)
		return;

	// Check if the current model is referenced by another model.
	bool Referenced = false;

	for (int i = 0; i < m_ModelList.GetSize(); i++)
	{
		lcModel* Model = m_ModelList[i];

		if (Model == DeleteModel)
			continue;

		if (Model->IsSubModel(DeleteModel))
		{
			Referenced = true;
			break;
		}
	}

	if (Referenced)
	{
		const char* Prompt = "The model you are about to delete is referenced by other models in this project.\nDo you want to inline those references before deleting?";
		bool Inline = (SystemDoMessageBox(Prompt, LC_MB_YESNO | LC_MB_ICONQUESTION) == LC_YES);

		for (int i = 0; i < m_ModelList.GetSize(); i++)
		{
			lcModel* Model = m_ModelList[i];

			if (Model == DeleteModel)
				continue;

			for (lcObject* Piece = Model->m_Pieces; Piece; Piece = Piece->m_Next)
			{
				// fixme: pivot
				if (Piece->GetType() != LC_OBJECT_MODELREF)
					continue;

				lcModelRef* ModelRef = (lcModelRef*)Piece;

				if (ModelRef->m_Model != DeleteModel)
					continue;

				if (Inline)
					Model->InlineModel(DeleteModel, ModelRef->m_ModelWorld, ModelRef->m_Color);
			}
		}
	}

	if (m_ActiveModel == DeleteModel)
		SetActiveModel(m_ModelList[0]);

	m_ModelList.RemovePointer(DeleteModel);
	delete DeleteModel;

	SetModifiedFlag(true);
	CheckPoint("Deleting Model");
	SystemUpdateModelMenu(m_ModelList, m_ActiveModel);
	UpdateAllViews();
}

void Project::SetActiveModel(lcModel* Model)
{
	if (m_ActiveModel)
		m_ActiveModel->SetActive(false);

	m_ActiveModel = Model;

	if (m_ActiveModel)
		m_ActiveModel->SetActive(true);

	SystemUpdateModelMenu(m_ModelList, m_ActiveModel);
	UpdateSelection();
	UpdateAllViews();
}

void Project::AddPiece(Vector3 Pos, Vector4 Rot)
{
	lcPieceObject* Piece;
	lcPieceObject* Selection = g_App->m_PiecePreview->m_Selection;

	if (Selection->GetType() == LC_OBJECT_PIECE)
	{
		Piece = new lcPiece((lcPiece*)Selection);
		const String& Name = ((lcPiece*)Selection)->m_PieceInfo->m_strDescription;
		Piece->SetUniqueName(m_ActiveModel->m_Pieces, Name);
		SystemPieceComboAdd(Name);
	}
	else if (Selection->GetType() == LC_OBJECT_FLEXPIECE)
	{
		Piece = new lcFlexiblePiece((lcFlexiblePiece*)Selection);
		const String& Name = ((lcFlexiblePiece*)Selection)->m_PieceInfo->m_strDescription;
		Piece->SetUniqueName(m_ActiveModel->m_Pieces, Name);
		SystemPieceComboAdd(Name);
	}
	else if (Selection->GetType() == LC_OBJECT_MODELREF)
	{
		if (((lcModelRef*)Selection)->m_Model->IsSubModel(m_ActiveModel))
		{
			SystemDoMessageBox("The current model is already a submodel of the model you are trying to add.", LC_MB_OK|LC_MB_ICONWARNING);
			return;
		}

		Piece = new lcModelRef((lcModelRef*)Selection);
		const String& Name = ((lcModelRef*)Selection)->m_Name;
		Piece->SetUniqueName(m_ActiveModel->m_Pieces, Name);
		SystemPieceComboAdd(Name);
	}
	else
		return;

	Piece->m_TimeShow = m_ActiveModel->m_CurFrame;
	Piece->m_Color = m_nCurColor;
	Piece->SetPosition(1, false, Pos);
	Piece->SetRotation(1, false, Rot);

	Piece->Update(m_ActiveModel->m_CurFrame);

	SelectAndFocusNone(false);
	m_ActiveModel->AddPiece(Piece);
	Piece->SetFocus(true, false);
	messenger->Dispatch(LC_MSG_FOCUS_CHANGED, Piece);
	UpdateSelection();
	SystemUpdateFocus(Piece);

	if (m_nSnap & LC_DRAW_MOVE)
		SetAction(LC_ACTION_MOVE);

//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)pNew, OT_PIECE);
	UpdateAllViews();
	SetModifiedFlag(true);
	CheckPoint("Adding Piece");
}

void Project::CalculateStep()
{
	int PieceCount = 0;
	lcObject* Piece;
	lcObject* Camera;
	lcObject* Light;

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
	{
		Piece->Update(m_ActiveModel->m_CurFrame);
		PieceCount++;
	}

  SystemDoWaitCursor(1);
	SystemStartProgressBar(0, PieceCount, 1, "Updating pieces...");

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
	{
//		Piece->CalculateConnections(m_pConnections, m_ActiveModel->m_CurFrame, false, false);
		SystemStepProgressBar();
	}

	SystemEndProgressBar();
	SystemDoWaitCursor(-1);

	for (Camera = m_ActiveModel->m_Cameras; Camera; Camera = Camera->m_Next)
		Camera->Update(m_ActiveModel->m_CurFrame);

	for (Light = m_ActiveModel->m_Lights; Light; Light = Light->m_Next)
		Light->Update(m_ActiveModel->m_CurFrame);
}

// Returns true if anything was removed (used by cut and del)
bool Project::RemoveSelectedObjects()
{
	lcObject* Camera;
	lcObject* Light;
	lcObject* Prev;
	bool Removed = false;

	lcObject* Piece = m_ActiveModel->m_Pieces;
	while (Piece)
	{
		if (Piece->IsSelected())
		{
			lcObject* Temp;
			Temp = Piece->m_Next;

			Removed = true;
			m_ActiveModel->RemovePiece((lcPieceObject*)Piece);
			delete Piece;
			Piece = Temp;
		}
		else
			Piece = Piece->m_Next;
	}

	// Cameras can't be removed while being used or default
	for (Prev = NULL, Camera = m_ActiveModel->m_Cameras; Camera; )
	{
		bool CanDelete = true;

		for (int i = 0; i < m_ViewList.GetSize(); i++)
		{
			if (Camera == m_ViewList[i]->GetCamera())
			{
				CanDelete = false;
				break;
			}
		}
/* FIXME camera delete
		if (CanDelete && Camera->IsSelected() && Camera->IsUser())
		{
			if (Prev)
			{
				Prev->m_Next = Camera->m_Next;
				delete Camera;
				Camera = Prev->m_Next;
			}
			else
			{
				m_ActiveModel->m_Cameras = (lcCamera*)m_ActiveModel->m_Cameras->m_Next;
				delete Camera;
				Camera = m_ActiveModel->m_Cameras;
			}
			
			Removed = true;

			SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
			SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
		}
		else
*/		{
			Prev = Camera;
			Camera = Camera->m_Next;
		}
	}

	for (Prev = NULL, Light = m_ActiveModel->m_Lights; Light; )
	{
		if (Light->IsSelected())
		{
			if (Prev)
			{
			  Prev->m_Next = Light->m_Next;
				delete Light;
				Light = Prev->m_Next;
			}
			else
			{
			  m_ActiveModel->m_Lights = (lcLight*)m_ActiveModel->m_Lights->m_Next;
				delete Light;
				Light = m_ActiveModel->m_Lights;
			}

			Removed = true;
		}
		else
		{
			Prev = Light;
			Light = Light->m_Next;
		}
	}

	RemoveEmptyGroups();
//	CalculateStep();
//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);

	return Removed;
}

void Project::UpdateSelection()
{
	unsigned long flags = 0;
	int SelectedCount = 0;
	lcObject* Focus = NULL;

	if (m_ActiveModel->m_Pieces == NULL)
		flags |= LC_SEL_NO_PIECES;
	else
	{
		lcObject* Piece;
		Group* pGroup = NULL;
		bool first = true;

		for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
		{
			if (Piece->IsSelected())
			{
				SelectedCount++;

				if (Piece->IsFocused())
					Focus = Piece;

				if (flags & LC_SEL_PIECE)
					flags |= LC_SEL_MULTIPLE;
				else
					flags |= LC_SEL_PIECE;
/* FIXME: menu updates
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
*/
			}
			else
			{
				flags |= LC_SEL_UNSELECTED;

				if (Piece->IsHidden())
					flags |= LC_SEL_HIDDEN;
			}
		}
	}

	for (lcObject* Camera = m_ActiveModel->m_Cameras; Camera; Camera = Camera->m_Next)
	{
		if (Camera->IsSelected())
		{
			flags |= LC_SEL_CAMERA;
			SelectedCount++;

			if (Camera->IsFocused())
				Focus = Camera;

			if (Camera->m_Children && Camera->m_Children->IsFocused())
				Focus = Camera->m_Children;
		}
	}

	for (lcObject* Light = m_ActiveModel->m_Lights; Light; Light = Light->m_Next)
		if (Light->IsSelected())
		{
			flags |= LC_SEL_LIGHT;
			SelectedCount++;

			if (Light->IsFocused())
				Focus = Light;

			if (Light->m_Children && Light->m_Children->IsFocused())
				Focus = Light->m_Children;
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

void Project::CheckAnimation()
{
	if (!m_PlayingAnimation)
		return;

	u64 Now = SystemGetTicks();
	u64 Elapsed = Now - m_LastFrameTime;
	int Frames = (int)((float)Elapsed / 1000 * m_nFPS);

	if (Frames)
	{
		m_ActiveModel->m_CurFrame += Frames;
		while (m_ActiveModel->m_CurFrame > m_ActiveModel->m_TotalFrames)
			m_ActiveModel->m_CurFrame -= m_ActiveModel->m_TotalFrames;

		CalculateStep();
		SystemUpdateTime(true, m_ActiveModel->m_CurFrame, m_ActiveModel->m_TotalFrames);

		UpdateAllViews();
	}
}

unsigned char Project::GetLastStep()
{
	u32 Last = 1;
	for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
		Last = lcMax(Last, Piece->m_TimeShow);

	return Last;
}

// Create a series of pictures
void Project::CreateImages (Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite)
{
	unsigned short oldtime;
	void* render = Sys_StartMemoryRender (width, height);
	unsigned char* buf = (unsigned char*)malloc(width*height*3);
	oldtime = m_ActiveModel->m_CurFrame;

	View view(this, NULL);
	view.OnSize(width, height);
	view.SetCamera(m_ActiveModel->GetCamera(LC_CAMERA_MAIN));

	if (!hilite)
		SelectAndFocusNone(false);

	RenderInitialize();

	for (u32 i = from; i <= to; i++)
	{
		m_ActiveModel->m_CurFrame = i;

		if (hilite)
		{
			for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
			{
				if (Piece->m_TimeShow == i)
					Piece->SetSelection(true, true);
				else
					Piece->SetSelection(false, true);
			}
		}

		CalculateStep();
		Render(&view, false, false);
		images[i-from].FromOpenGL (width, height);
	}

	m_ActiveModel->m_CurFrame = (unsigned char)oldtime;
	CalculateStep();
	free (buf);
	Sys_FinishMemoryRender (render);
}

void Project::CreateHTMLPieceList(FILE* f, int nStep, bool bImages, char* ext)
{
	/* FIXME: html export
	lcPieceObject* PieceObj;
	int col[LC_MAXCOLORS], ID = 0, c;
	memset (&col, 0, sizeof (col));
	for (PieceObj = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
	{
		if ((Piece->m_TimeShow == nStep) || (nStep == 0))
			col[Piece->m_Color]++;
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
		memset(&count, 0, sizeof (count));
		pInfo = lcGetPiecesLibrary()->GetPieceInfo(j);

		for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
		{
			if ((Piece->GetPieceInfo() == pInfo) && 
			    ((Piece->m_TimeShow == nStep) || (nStep == 0)))
			{
				count[Piece->m_Color]++;
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
	*/
}

// Special notifications.
void Project::HandleNotify(LC_NOTIFY id, unsigned long param)
{
	switch (id)
	{
		case LC_COLOR_CHANGED:
		{
			m_nCurColor = (unsigned char)param;

			if (g_App->m_PiecePreview->m_Selection)
				g_App->m_PiecePreview->m_Selection->m_Color = m_nCurColor;
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

		// FIXME: don't change the keys with ChangeKey(), even if pos == prevpos, the user might want to add a key

		case LC_PIECE_MODIFIED:
		{
			LC_PIECE_MODIFY* mod = (LC_PIECE_MODIFY*)param;
			lcPieceObject* Piece = (lcPieceObject*)mod->piece;

			Matrix33 Mat = Piece->m_ModelWorld;
			Vector3 Angles = MatrixToEulerAngles(Mat) * LC_RTOD;

			if (Piece->m_ParentPosition != mod->Position)
				Piece->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->Position);

			if (mod->Rotation[0] != Angles[0] || mod->Rotation[1] != Angles[1] || mod->Rotation[2] != Angles[2])
			{
				Mat = MatrixFromEulerAngles(Vector3(mod->Rotation[0], mod->Rotation[1], mod->Rotation[2]) * LC_DTOR);
				Piece->SetRotation(m_ActiveModel->m_CurFrame, m_bAddKeys, MatrixToAxisAngle(Mat));
			}

			Piece->m_TimeShow = mod->from;
			Piece->m_TimeHide = mod->to;

			Piece->SetVisible(!mod->hidden);
			Piece->m_Name = mod->name;
			Piece->m_Color = mod->color;
			Piece->Update(m_ActiveModel->m_CurFrame);
//			Piece->CalculateConnections(m_pConnections, m_ActiveModel->m_CurFrame, false, true);

			SetModifiedFlag(true);
			CheckPoint("Modifying");
			ActivateOverlay();
			UpdateAllViews();
		} break;

		case LC_CAMERA_MODIFIED:
		{
			LC_CAMERA_MODIFY* mod = (LC_CAMERA_MODIFY*)param;
			lcCamera* Camera = (lcCamera*)mod->camera;

			Camera->SetVisible(!mod->hidden);
			Camera->SetOrtho(mod->ortho);
			Camera->ShowCone(mod->cone);
//			Camera->SetAutoClip(!mod->clip);

			if (Camera->m_ParentPosition != mod->Eye)
				Camera->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->Eye);

			if (Camera->m_Children && Camera->m_Children->m_ParentPosition != mod->Target)
				Camera->m_Children->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->Target);

			if (Camera->m_Roll != mod->Roll)
				Camera->SetRoll(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->Roll);

			Camera->m_Name = mod->name;
			Camera->m_FOV = mod->fovy;
			Camera->m_NearDist = mod->znear;
			Camera->m_FarDist = mod->zfar;

			Camera->Update(m_ActiveModel->m_CurFrame);
			UpdateAllViews();
		} break;

		case LC_LIGHT_MODIFIED:
		{
/* FIXME: light modify
			LC_LIGHT_MODIFY* mod = (LC_LIGHT_MODIFY*)param;
			lcLight* Light = (lcLight*)mod->light;

			Light->SetVisible(!mod->Hidden);

			if (Light->m_ParentPosition != mod->Position)
				Light->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->Position);

			if (Light->GetTargetPosition() != mod->Target)
				Light->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->Target, LC_LK_TARGET);

			if (Light->m_AmbientColor != mod->AmbientColor)
				Light->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->AmbientColor, LC_LK_AMBIENT_COLOR);

			if (Light->m_DiffuseColor != mod->DiffuseColor)
				Light->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->DiffuseColor, LC_LK_DIFFUSE_COLOR);

			if (Light->m_SpecularColor != mod->SpecularColor)
				Light->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->SpecularColor, LC_LK_SPECULAR_COLOR);

			if (Light->m_ConstantAttenuation != mod->ConstantAttenuation)
				Light->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, &mod->ConstantAttenuation, LC_LK_CONSTANT_ATTENUATION);

			if (Light->m_LinearAttenuation != mod->LinearAttenuation)
				Light->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, &mod->LinearAttenuation, LC_LK_LINEAR_ATTENUATION);

			if (Light->m_QuadraticAttenuation != mod->QuadraticAttenuation)
				Light->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, &mod->QuadraticAttenuation, LC_LK_QUADRATIC_ATTENUATION);

			if (Light->m_SpotCutoff != mod->SpotCutoff)
				Light->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, &mod->SpotCutoff, LC_LK_SPOT_CUTOFF);

			if (Light->m_SpotExponent != mod->SpotExponent)
				Light->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, &mod->SpotExponent, LC_LK_SPOT_EXPONENT);

			Light->m_Name = mod->name;

			Light->Update(m_ActiveModel->m_CurFrame);
			UpdateAllViews();
*/
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
			opts.to = m_ActiveModel->m_TotalFrames;
			opts.multiple = false;
			opts.imopts.background[0] = (unsigned char)(m_fBackground[0]*255);
			opts.imopts.background[1] = (unsigned char)(m_fBackground[1]*255);
			opts.imopts.background[2] = (unsigned char)(m_fBackground[2]*255);

			if (SystemDoDialog(LC_DLG_PICTURE_SAVE, &opts))
			{
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
					opts.from = opts.to = m_ActiveModel->m_CurFrame;

				Image* images = new Image[opts.to-opts.from+1];
				CreateImages (images, opts.width, opts.height, opts.from, opts.to, false);

				char *ptr, ext[4];
				ptr = strrchr(opts.filename, '.');
				if (ptr != NULL)
				{
					strncpy(ext, ptr+1, 3);
					_strlwr(ext);
				}

				if (strcmp(ext, "avi") == 0)
				{
//					SaveVideo(opts.filename, images, opts.to-opts.from+1, m_bAnimation ? m_nFPS : 60.0f/opts.imopts.pause);
					SaveVideo(opts.filename, images, opts.to-opts.from+1, m_nFPS);
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
#if LC_WINDOWS
			Export3DStudio();
#endif
		} break;

    case LC_FILE_HTML:
    {
/*
FIXME: html export
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
				char *ext, *htmlext, fn[LC_MAXPATH];
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
					for (p1 = m_ActiveModel->m_Pieces; p1; p1 = (Piece*)p1->m_Next)
					{
						bool bSkip = false;
						pInfo = p1->GetPieceInfo();

						for (p2 = m_ActiveModel->m_Pieces; p2; p2 = (Piece*)p2->m_Next)
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
*/
		} break;

		// Export to POV-Ray, swap X & Y from our cs to work with LGEO.
		case LC_FILE_POVRAY:
		{
/* FIXME: pov export
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

			fputs("// Stuff that doesn't need to be changed\n\n", f);

			if (strlen(opts.libpath))
				fputs("#include \"lg_color.inc\"\n#include \"lg_defs.inc\"\n\n", f);

			// Add include files
			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
			{
				Piece* pNext;

				for (pNext = m_ActiveModel->m_Pieces; pNext; pNext = (Piece*)pNext->m_Next)
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
			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
			{
				pInfo = pPiece->GetPieceInfo();
				int idx = lcGetPiecesLibrary()->GetPieceIndex (pInfo);
				if (conv[idx*9] != 0)
					continue;

				for (Piece* pNext = m_ActiveModel->m_Pieces; pNext; pNext = (Piece*)pNext->m_Next)
				{
					if (pNext == pPiece)
					{
						pInfo->WritePOV(f);
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
			Vector3 eye = m_ActiveView->GetCamera()->m_Position;
			Vector3 target = m_ActiveView->GetCamera()->GetTargetPosition();
			Vector3 up = m_ActiveView->GetCamera()->GetUpVector();

			fprintf(f, "\ncamera {\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f\n}\n\n",
				up[0], up[1], up[2], eye[1], eye[0], eye[2], target[1], target[0], target[2], m_ActiveView->GetCamera()->m_FOV);
			fprintf(f, "background { color rgb <%1g, %1g, %1g> }\n\nlight_source { <0, 0, 20> White shadowless }\n\n",
				m_fBackground[0], m_fBackground[1], m_fBackground[2]);

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
			{
				float fl[12], pos[3], rot[4];
				char name[20];
				int idx = lcGetPiecesLibrary()->GetPieceIndex(pPiece->GetPieceInfo());

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

			if (opts.render)
			{
#if LC_WINDOWS
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
			if (strlen(m_ActiveModel->m_Author))
				fprintf(stream, "# Author: %s\n", m_ActiveModel->m_Author);

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

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
			{
				float pos[3], rot[4], tmp[3];
				pPiece->GetPosition(pos);
				pPiece->GetRotation(rot);
				Matrix mat(rot, pos);
				PieceInfo* pInfo = pPiece->GetPieceInfo();

				float* VertexPtr = (float*)pInfo->GetMesh()->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);

				for (i = 0; i < (u32)pInfo->GetMesh()->m_VertexCount; i++)
				{
					mat.TransformPoint(tmp, &VertexPtr[i*3]);
					fprintf(stream, "v %.2f %.2f %.2f\n", tmp[0], tmp[1], tmp[2]);
				}
				fputs("#\n\n", stream);

				pInfo->GetMesh()->m_VertexBuffer->UnmapBuffer();
			}

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
			{
				strcpy(buf, pPiece->GetName());
				for (i = 0; i < strlen(buf); i++)
					if ((buf[i] == '#') || (buf[i] == ' '))
						buf[i] = '_';

				fprintf(stream, "g %s\n", buf);
				pPiece->GetPieceInfo()->WriteWavefront(stream, pPiece->GetColor(), &vert);
			}

			fclose(stream);
*/
		} break;

		case LC_MODEL_NEW:
		{
			int Max = 0;

			// Find out the number of the last model added.
			for (int i = 0; i < m_ModelList.GetSize(); i++)
			{
				int Num;

				if (sscanf((char*)m_ModelList[i]->m_Name, "New Model %d", &Num) == 1)
					if (Num > Max)
						Max = Num;
			}

			char Name[256];
			sprintf(Name, "New Model %d", Max + 1);

			// Prompt the user for the model properties.
			LC_PROPERTIESDLG_OPTS opts;

			opts.Name = Name;
			opts.Author = Sys_ProfileLoadString("Default", "User", "");
			opts.PiecesUsed = NULL;

			if (SystemDoDialog(LC_DLG_PROPERTIES, &opts))
			{
				// Create and add new model.
				lcModel* Model = new lcModel(Name);
				Model->m_Author = opts.Author;
				Model->m_Description = opts.Description;
				Model->m_Comments = opts.Comments;

				AddModel(Model);
				SetActiveModel(Model);

				SetModifiedFlag(true);
				CheckPoint("Adding Model");
				SystemUpdateModelMenu(m_ModelList, m_ActiveModel);
				UpdateAllViews();
			}
		} break;

		case LC_MODEL_DELETE:
		{
			DeleteModel(m_ActiveModel);
		} break;

		case LC_MODEL_MODEL1:
		case LC_MODEL_MODEL2:
		case LC_MODEL_MODEL3:
		case LC_MODEL_MODEL4:
		case LC_MODEL_MODEL5:
		case LC_MODEL_MODEL6:
		case LC_MODEL_MODEL7:
		case LC_MODEL_MODEL8:
		case LC_MODEL_MODEL9:
		case LC_MODEL_MODEL10:
		case LC_MODEL_MODEL11:
		case LC_MODEL_MODEL12:
		case LC_MODEL_MODEL13:
		case LC_MODEL_MODEL14:
		case LC_MODEL_MODEL15:
		case LC_MODEL_MODEL16:
		{
			int Index = id - LC_MODEL_MODEL1;
			if (Index < m_ModelList.GetSize())
			{
				SetActiveModel(m_ModelList[Index]);
			}
		} break;

		case LC_MODEL_PROPERTIES:
		{
			LC_PROPERTIESDLG_OPTS opts;

			// Text fields.
			opts.Name = m_ActiveModel->m_Name;
			opts.Author = m_ActiveModel->m_Author;
			opts.Description = m_ActiveModel->m_Description;
			opts.Comments = m_ActiveModel->m_Comments;

			// Pieces list.
			PiecesLibrary* Lib = lcGetPiecesLibrary();
			int NumPieces = Lib->GetPieceCount() * lcNumUserColors;
			opts.PiecesUsed = new int[NumPieces];
			memset(opts.PiecesUsed, 0, NumPieces * sizeof(int));

			ObjArray<LC_PIECELIST_ENTRY> Pieces;
			m_ActiveModel->GetPieceList(Pieces, LC_COLOR_DEFAULT);

			for (int i = 0; i < Pieces.GetSize(); i++)
			{
				int Index = Lib->GetPieceIndex(Pieces[i].Info) * lcNumUserColors + Pieces[i].Color;
				opts.PiecesUsed[Index]++;
			}

			if (SystemDoDialog(LC_DLG_PROPERTIES, &opts))
			{
				if ((m_ActiveModel->m_Author != opts.Author) || (m_ActiveModel->m_Description != opts.Description) ||
				    (m_ActiveModel->m_Comments!= opts.Comments) || (m_ActiveModel->m_Name != opts.Name))
				{
					m_ActiveModel->m_Name = opts.Name;
					m_ActiveModel->m_Author = opts.Author;
					m_ActiveModel->m_Description = opts.Description;
					m_ActiveModel->m_Comments = opts.Comments;
					SystemUpdateModelMenu(m_ModelList, m_ActiveModel);
					SetModifiedFlag(true);
				}
			}

			delete[] opts.PiecesUsed;
		} break;

		case LC_FILE_TERRAIN:
		{
			Terrain* temp = new Terrain();
			*temp = *m_pTerrain;

			if (SystemDoDialog(LC_DLG_TERRAIN, temp))
			{
				*m_pTerrain = *temp;
				m_pTerrain->LoadTexture((m_nDetail & LC_DET_LINEAR) != 0);
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
			if (!OpenProject(main_window->GetMRU(nParam)))
				main_window->RemoveFromMRU(nParam);
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
/*
FIXME: cut copy
			if (m_pClipboard[m_nCurClipboard] != NULL)
				delete m_pClipboard[m_nCurClipboard];
			m_pClipboard[m_nCurClipboard] = new FileMem;

			int i = 0;
			Piece* pPiece;
			Camera* pCamera;
			Group* pGroup;
//			Light* pLight;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
				if (pPiece->IsSelected())
					i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
				if (pPiece->IsSelected())
					pPiece->FileSave(*m_pClipboard[m_nCurClipboard], m_pGroups);

			for (i = 0, pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
				pGroup->FileSave(m_pClipboard[m_nCurClipboard], m_pGroups);

			for (i = 0, pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (Camera*)pCamera->m_Next)
				if (pCamera->IsSelected())
					i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (Camera*)pCamera->m_Next)
				if (pCamera->IsSelected())
					pCamera->FileSave(*m_pClipboard[m_nCurClipboard]);

//		for (i = 0, pLight = m_ActiveModel->m_Lights; pLight; pLight = pLight->m_pNext)
//			if (pLight->IsSelected())
//				i++;
//		m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

//		for (pLight = m_ActiveModel->m_Lights; pLight; pLight = pLight->m_pNext)
//			if (pLight->IsSelected())
//				pLight->FileSave(m_pClipboard[m_nCurClipboard]);

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
*/
		} break;

		case LC_EDIT_PASTE:
		{
/*
FIXME: paste
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
				char name[9];
				Piece* pPiece = new Piece(NULL);
				pPiece->FileLoad(*file, name);
				PieceInfo* pInfo = lcGetPiecesLibrary()->FindPieceInfo(name);
				if (pInfo)
				{
					pPiece->SetPieceInfo(pInfo);
					pPiece->m_Next = pPasted;
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
				pPasted = (Piece*)pPasted->m_Next;
				pPiece->CreateName(m_ActiveModel->m_Pieces);
				pPiece->SetTimeShow(m_ActiveModel->m_CurFrame);
				AddPiece(pPiece);
				pPiece->Select(true, false);

				j = (int)pPiece->GetGroup();
				if (j != -1)
					pPiece->SetGroup(groups[j]);
				else
					pPiece->UnGroup(NULL);
			}

			for (j = 0; j < i; j++)
			{
				int g = (int)groups[j]->m_pGroup;
				groups[j]->m_pGroup = (g != -1) ? groups[g] : NULL;
			}

			for (j = 0; j < i; j++)
			{
				Group* pGroup;
				bool add = false;
				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
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

			Camera* pCamera = m_ActiveModel->m_Cameras;
			while (pCamera->m_Next)
				pCamera = (Camera*)pCamera->m_Next;
			file->Read(&i, sizeof(i));

			while (i--)
			{
				pCamera = new Camera(8, pCamera);
				pCamera->FileLoad(*file);
				pCamera->Select(true, false);
				pCamera->GetTarget()->Select(true, false);
			}

			// TODO: lights
			CalculateStep();
			SetModifiedFlag(true);
			CheckPoint("Pasting");
			SystemUpdateFocus(NULL);
			UpdateSelection();
			UpdateAllViews ();
*/
		} break;

		case LC_EDIT_SELECT_ALL:
		{
			m_ActiveModel->SelectAllPieces();

//	pFrame->UpdateInfo();
			UpdateSelection();
			UpdateAllViews ();
		} break;
		
		case LC_EDIT_SELECT_NONE:
		{
			SelectAndFocusNone(false);
			messenger->Dispatch(LC_MSG_FOCUS_CHANGED, NULL);
			UpdateSelection();
			UpdateAllViews();
		} break;
		
		case LC_EDIT_SELECT_INVERT:
		{
			m_ActiveModel->SelectInvertAllPieces();
			messenger->Dispatch(LC_MSG_FOCUS_CHANGED, NULL);
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_EDIT_SELECT_BYNAME:
		{
	/* FIXME: select by name
			Piece* pPiece;
			Camera* pCamera;
			Light* pLight;
			Group* pGroup;
			int i = 0;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
				if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
					i++;

			for (pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (Camera*)pCamera->m_Next)
				if (pCamera != m_ActiveView->GetCamera())
					if (pCamera->IsVisible())
						i++;

			for (pLight = m_ActiveModel->m_Lights; pLight; pLight = (Light*)pLight->m_Next)
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

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
				if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
				{
					opts[i].name = pPiece->GetName();
					opts[i].type = LC_SELDLG_PIECE;
					opts[i].selected = pPiece->IsSelected();
					opts[i].pointer = pPiece;
					i++;
				}

			for (pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (Camera*)pCamera->m_Next)
				if (pCamera != m_ActiveView->GetCamera())
					if (pCamera->IsVisible())
					{
						opts[i].name = pCamera->GetName();
						opts[i].type = LC_SELDLG_CAMERA;
						opts[i].selected = pCamera->IsSelected();
						opts[i].pointer = pCamera;
						i++;
					}

			for (pLight = m_ActiveModel->m_Lights; pLight; pLight = (Light*)pLight->m_Next)
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
							((Piece*)opts[i].pointer)->Select(true, false);
						} break;

						case LC_SELDLG_CAMERA:
						{
							((Camera*)opts[i].pointer)->Select(true, false);
						} break;

						case LC_SELDLG_LIGHT:
						{
							((Light*)opts[i].pointer)->Select(true, false);
						} break;

						case LC_SELDLG_GROUP:
						{
							pGroup = (Group*)opts[i].pointer;
							pGroup = pGroup->GetTopGroup();
							for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
								if (pPiece->GetTopGroup() == pGroup)
									pPiece->Select(true, false);
						} break;
					}
				}

				UpdateSelection();
				UpdateAllViews();
//	pFrame->UpdateInfo();
			}

			free(opts);
*/
		} break;

		case LC_PIECE_INSERT:
		{
			lcPieceObject* Last = NULL;

			for (Last = m_ActiveModel->m_Pieces; Last; Last = (lcPieceObject*)Last->m_Next)
				if ((Last->IsFocused()) || (Last->m_Next == NULL))
					break;

			Vector3 Pos;
			Vector4 Rot;

			if (Last)
			{
				GetPieceInsertPosition(Last, Pos, Rot);
			}
			else
			{
				lcPieceObject* Selection = g_App->m_PiecePreview->m_Selection;

				if (Selection)
					Pos = Vector3(0, 0, -Selection->m_BoundingBox.m_Min[2]);
				else
					Pos = Vector3(0, 0, 0);

				Rot = Vector4(0, 0, 1, 0);
			}

			AddPiece(Pos, Rot);
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
		  MinifigWizard Wizard(m_ViewList[0]);
		  int i;

			if (SystemDoDialog(LC_DLG_MINIFIG, &Wizard))
			{
				SelectAndFocusNone(false);

				for (i = 0; i < LC_MFW_NUMITEMS; i++)
				{
					if (Wizard.m_Info[i] == NULL)
						continue;

					Matrix mat;
					lcPiece* Piece = new lcPiece(Wizard.m_Info[i]);

					Piece->m_TimeShow = m_ActiveModel->m_CurFrame;
					Piece->m_Color = Wizard.m_Colors[i];

					Piece->SetPosition(1, true, Vector3(Wizard.m_Position[i][0], Wizard.m_Position[i][1], Wizard.m_Position[i][2]));
					Piece->SetRotation(1, true, Vector4(Wizard.m_Rotation[i][0], Wizard.m_Rotation[i][1], Wizard.m_Rotation[i][2], Wizard.m_Rotation[i][3]));

					Piece->SetUniqueName(m_ActiveModel->m_Pieces, Wizard.m_Info[i]->m_strDescription);
					m_ActiveModel->AddPiece(Piece);
					Piece->SetSelection(true, false);

					Piece->Update(m_ActiveModel->m_CurFrame);

					SystemPieceComboAdd(Wizard.m_Info[i]->m_strDescription);
				}
/* FIXME: groups
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

				for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
					if (Piece->IsSelected())
					{
						Piece->SetGroup(pGroup);
						Piece->MergeBoundingBox(bs);
					}

				pGroup->m_fCenter[0] = (bs[0]+bs[3])/2;
				pGroup->m_fCenter[1] = (bs[1]+bs[4])/2;
				pGroup->m_fCenter[2] = (bs[2]+bs[5])/2;
*/
        messenger->Dispatch (LC_MSG_FOCUS_CHANGED, NULL);
				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Minifig");
			}
		} break;

		case LC_PIECE_ARRAY:
		{
	/* FIXME: array
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

				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
					if (pPiece->IsSelected())
					{
						pPiece->MergeBoundingBox(bs);
						sel++;
					}

				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
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
								pLast->m_Next = new Piece(pPiece->GetPieceInfo());
								pLast = (Piece*)pLast->m_Next;
							}
							else
								pLast = pFirst = new Piece(pPiece->GetPieceInfo());

							pLast->Initialize(pos[0]+i*opts.fMove[0], pos[1]+i*opts.fMove[1], pos[2]+i*opts.fMove[2], m_ActiveModel->m_CurFrame, pPiece->GetColor());
							pLast->ChangeKey(1, false, param, LC_PK_ROTATION);
						}

						if (opts.nArrayDimension == 0)
							continue;
						
						for (j = 0; j < opts.n2DCount; j++)
						{
							if (j != 0)
							{
								if (pLast)
								{
									pLast->m_Next = new Piece(pPiece->GetPieceInfo());
									pLast = (Piece*)pLast->m_Next;
								}
								else
									pLast = pFirst = new Piece(pPiece->GetPieceInfo());

								pLast->Initialize(pos[0]+i*opts.fMove[0]+j*opts.f2D[0], pos[1]+i*opts.fMove[1]+j*opts.f2D[1], pos[2]+i*opts.fMove[2]+j*opts.f2D[2],
									m_ActiveModel->m_CurFrame, pPiece->GetColor());
								pLast->ChangeKey(1, false, param, LC_PK_ROTATION);
							}

							if (opts.nArrayDimension == 1)
								continue;

							for (k = 1; k < opts.n3DCount; k++)
							{
								if (pLast)
								{
									pLast->m_Next = new Piece(pPiece->GetPieceInfo());
									pLast = (Piece*)pLast->m_Next;
								}
								else
									pLast = pFirst = new Piece(pPiece->GetPieceInfo());

								pLast->Initialize(pos[0]+i*opts.fMove[0]+j*opts.f2D[0]+k*opts.f3D[0], pos[1]+i*opts.fMove[1]+j*opts.f2D[1]+k*opts.f3D[1], pos[2]+i*opts.fMove[2]+j*opts.f2D[2]+k*opts.f3D[2],
									m_ActiveModel->m_CurFrame, pPiece->GetColor());
								pLast->ChangeKey(1, false, param, LC_PK_ROTATION);
							}
						}
					}
				}

				while (pFirst)
				{
					pPiece = (Piece*)pFirst->m_Next;
					pFirst->CreateName(m_ActiveModel->m_Pieces);
					pFirst->UpdatePosition(m_ActiveModel->m_CurFrame);
					AddPiece(pFirst);
					pFirst->CalculateConnections(m_pConnections, m_ActiveModel->m_CurFrame, false, true);
					pFirst = pPiece;
				}

				SelectAndFocusNone(true);
//				SystemUpdateFocus(NULL);
				UpdateSelection();
				UpdateAllViews();
				SetModifiedFlag(true);
				CheckPoint("Array");
			}
			*/
		} break;

		case LC_PIECE_GROUP:
		{
			lcPivot* Group = new lcPivot();

			Group->SetUniqueName(m_ActiveModel->m_Pieces, "Group");

			if (!SystemDoDialog(LC_DLG_GROUP, &Group->m_Name))
			{
				delete Group;
				break;
			}

			BoundingBox Box;
			Box.Reset();

			lcPieceObject* Add = NULL;
			lcPieceObject* Remove = NULL;
			lcPieceObject* Piece = m_ActiveModel->m_Pieces;

			while (Piece)
			{
				lcPieceObject* Next = (lcPieceObject*)Piece->m_Next;

				if (Piece->IsSelected())
				{
					Piece->MergeBoundingBox(&Box);

					// Remove from model.
					if (Remove)
						Remove->m_Next = Piece->m_Next;
					else
						m_ActiveModel->m_Pieces = (lcPieceObject*)Piece->m_Next;

					Piece->m_Next = NULL;
					Piece->m_Parent = Group;

					// Add to group.
					if (Add)
						Add->m_Next = Piece;
					else
						Group->m_Children = Piece;

					Add = Piece;
				}
				else
					Remove = Piece;

				Piece = Next;
			}

			Group->SetSelection(false, true);
			Group->SetFocus(true, false);

//			Group->SetPosition(Box.GetCenter());
			// FIXME: move pieces to be relative to the parent

			Group->Update(m_ActiveModel->m_CurFrame);
			m_ActiveModel->AddPiece(Group);

			SetModifiedFlag(true);
			CheckPoint("Grouping");

		} break;

		case LC_PIECE_UNGROUP:
		{
			/* FIXME: ungroup
			Group* pList = NULL;
			Group* pGroup;
			Group* tmp;
			Piece* pPiece;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
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
				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
					if (pPiece->IsSelected())
						pPiece->UnGroup(pList);

				pGroup = pList;
				pList = pList->m_pNext;
				delete pGroup;
			}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Ungrouping");
			*/
		} break;

		case LC_PIECE_GROUP_ADD:
		{
			/* FIXME: group add
			Group* pGroup = NULL;
			Piece* pPiece;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
				if (pPiece->IsSelected())
				{
					pGroup = pPiece->GetTopGroup();
					if (pGroup != NULL)
						break;
				}

			if (pGroup != NULL)
			{
				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
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

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
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

			for (i = 0, pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
				i++;
			opts.piececount = i;
			opts.pieces = (Piece**)malloc(i*sizeof(Piece*));
			opts.piecesgroups = (Group**)malloc(i*sizeof(Group*));

			for (i = 0, pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next, i++)
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
			*/
		} break;

		case LC_PIECE_HIDE_SELECTED:
		{
			for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
				if (Piece->IsSelected())
					Piece->SetVisible(false);
			UpdateSelection();
			messenger->Dispatch (LC_MSG_FOCUS_CHANGED, NULL);
			UpdateAllViews();
		} break;

		case LC_PIECE_HIDE_UNSELECTED:
		{
			for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
				if (!Piece->IsSelected())
					Piece->SetVisible(false);
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_PIECE_UNHIDE_ALL:
		{
			for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
				Piece->SetVisible(true);
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_PIECE_PREVIOUS:
		{
			bool Redraw = false;

			for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
			{
				if (Piece->IsSelected())
				{
					if (Piece->m_TimeShow > 1)
					{
						Piece->m_TimeShow--;
						Redraw = true;
					}
				}
			}

			if (Redraw)
			{
				SetModifiedFlag(true);
				CheckPoint("Modifying");
				UpdateAllViews();
			}
		} break;

		case LC_PIECE_NEXT:
		{
			bool Redraw = false;

			for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
			{
				if (Piece->IsSelected())
				{
					if (Piece->m_TimeShow < LC_MAX_TIME)
					{
						Piece->m_TimeShow++;
						Redraw = true;

						if (Piece->IsSelected() && (Piece->m_TimeShow > m_ActiveModel->m_CurFrame))
							Piece->SetSelection(false, true);
					}
				}
			}

			if (Redraw)
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
			m_ActiveView->GetCamera()->DoZoom(nParam, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMIN:
		{
			m_ActiveView->GetCamera()->DoZoom(-1, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMOUT:
		{
			m_ActiveView->GetCamera()->DoZoom(1, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMEXTENTS:
		{
			if (!m_ActiveModel->m_Pieces)
				break;

			// Calculate a bounding box that includes all pieces and use its center as the camera target.
			ObjArray<Vector3> Points;
			BoundingBox Box;
			Box.Reset();

			for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
			{
				if (!Piece->IsVisible(m_ActiveModel->m_CurFrame))
					continue;

				Vector3 Corners[8];
				Piece->m_BoundingBox.GetPoints(Corners);

				for (int i = 0; i < 8; i++)
				{
					Vector3 Point = Mul31(Corners[i], Piece->m_ModelWorld);
					Points.Add(Point);
					Box.AddPoint(Point);
				}
			}

			Vector3 Center = Box.GetCenter();

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

			for (int vp = FirstView; vp < LastView; vp++)
			{
				View* view = m_ViewList[vp];
				lcCamera* Camera = view->GetCamera();

				// Update eye and target positions.
				Vector3 Eye = Camera->m_WorldPosition;
				Vector3 Target = Camera->m_Children->m_WorldPosition;

				if (Camera->IsSide())
					Eye += Center - Target;

				Target = Center;

				Camera->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, Eye);
				Camera->m_Children->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, Target);
				Camera->Update(m_ActiveModel->m_CurFrame);

				float Aspect = (float)view->GetWidth()/(float)view->GetHeight();
				Matrix44 Projection = CreatePerspectiveMatrix(Camera->m_FOV, Aspect, Camera->m_NearDist, Camera->m_FarDist);

				Eye = ZoomExtents(Eye, Camera->m_WorldView, Projection, &Points[0], Points.GetSize());

				// Save new positions.
				Camera->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, Eye);
				Camera->Update(m_ActiveModel->m_CurFrame);
			}

			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPORTS:
		{
			// Predefined viewports (must match the menu).
			const char* Viewports[14] =
			{
				"V4|Main", // 1
				"SV49V4|MainV4|Left", // 2V
				"SH49V4|MainV4|Left", // 2H
				"SH20V4|LeftV4|Main", // 2HT
				"SH80V4|MainV4|Left", // 2HB
				"SV49SH49V3|TopV4|LeftV4|Main", // 3VL
				"SV49V4|MainSH49V3|TopV4|Left", // 3VR
				"SH49SV49V3|TopV4|LeftV4|Main", // 3HB
				"SH49V4|MainSV49V3|TopV4|Left", // 3HT
				"SV32SH32V3|TopSH49V4|LeftV5|FrontV4|Main", // 4VL
				"SV65V4|MainSH32V3|TopSH49V4|LeftV5|Front", // 4VR
				"SH32SV32V3|TopSV49V4|LeftV5|FrontV4|Main", // 4HT
				"SH65V4|MainSV32V3|TopSV49V4|LeftV5|Front", // 4HB
				"SH49SV49V4|MainV3|TopSV49V4|LeftV5|Front"  // 4
			};

			LC_ASSERT((nParam >= 0) && (nParam < 14), "Invalid view parameter.");

			lcCamera* OldCamera = m_ActiveView->GetCamera();

			main_window->SetViewLayout(Viewports[nParam]);

			SystemUpdateCurrentCamera(OldCamera, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
			UpdateOverlayScale();
		} break;

		case LC_VIEW_STEP_NEXT:
		{
			m_ActiveModel->m_CurFrame++;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
		} break;
		
		case LC_VIEW_STEP_PREVIOUS:
		{
			m_ActiveModel->m_CurFrame--;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
		} break;
		
		case LC_VIEW_STEP_FIRST:
		{
			m_ActiveModel->m_CurFrame = 1;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
		} break;

		case LC_VIEW_STEP_LAST:
		{
			m_ActiveModel->m_CurFrame = GetLastStep ();

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
		} break;

		case LC_VIEW_STEP_CHOOSE:
		{
			SystemDoDialog(LC_DLG_STEPCHOOSE, NULL);
			SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
		} break;

		case LC_VIEW_STEP_SET:
		{
			m_ActiveModel->m_CurFrame = (nParam < 255) ? (unsigned char)nParam : 255;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
		} break;

		case LC_VIEW_STEP_INSERT:
		{
			for (lcObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
				Piece->InsertTime(m_ActiveModel->m_CurFrame, 1);

			for (lcObject* Camera = m_ActiveModel->m_Cameras; Camera; Camera = Camera->m_Next)
				Camera->InsertTime(m_ActiveModel->m_CurFrame, 1);

			for (lcObject* Light = m_ActiveModel->m_Lights; Light; Light = Light->m_Next)
				Light->InsertTime(m_ActiveModel->m_CurFrame, 1);

			SetModifiedFlag(true);
			CheckPoint("Adding Step");
			CalculateStep();
			UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_STEP_DELETE:
		{
			for (lcObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
				Piece->RemoveTime(m_ActiveModel->m_CurFrame, 1);

			for (lcObject* Camera = m_ActiveModel->m_Cameras; Camera; Camera = Camera->m_Next)
				Camera->RemoveTime(m_ActiveModel->m_CurFrame, 1);

			for (lcObject* Light = m_ActiveModel->m_Lights; Light; Light = Light->m_Next)
				Light->RemoveTime(m_ActiveModel->m_CurFrame, 1);

			SetModifiedFlag(true);
			CheckPoint("Removing Step");
			CalculateStep();
			UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_STOP:
		{
			m_PlayingAnimation = false;
			SystemUpdatePlay(true, false);
		} break;

		case LC_VIEW_PLAY:
		{ 
			SelectAndFocusNone(false);
			UpdateSelection();
			SystemUpdatePlay(false, true);
			m_LastFrameTime = SystemGetTicks();
			m_PlayingAnimation = true;
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
			lcCamera* Camera = m_ActiveModel->m_Cameras;

			while (nParam--)
				Camera = (lcCamera*)Camera->m_Next;

			SystemUpdateCurrentCamera(m_ActiveView->GetCamera(), Camera, m_ActiveModel->m_Cameras);
			m_ActiveView->SetCamera(Camera);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_CAMERA_RESET:
		{
			m_ActiveModel->ResetCameras();

			for (int i = 0; i < m_ViewList.GetSize(); i++)
				m_ViewList[i]->UpdateCamera();

			SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
			SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Reset Cameras");
		} break;

		case LC_VIEW_AUTOPAN:
		{
			short x = (short)nParam;
			short y = (short)((nParam >> 16) & 0xFFFF);

			x -= x > 0 ? 5 : -5;
			y -= y > 0 ? 5 : -5;

			m_ActiveView->GetCamera()->DoPan(x/4, y/4, 1, m_ActiveModel->m_CurFrame, m_bAddKeys);
			m_ActiveView->GetCamera()->Update(m_ActiveModel->m_CurFrame);
			m_nDownX = x;
			m_nDownY = y;
			UpdateOverlayScale();
			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;

		case LC_HELP_ABOUT:
		{
		  SystemDoDialog(LC_DLG_ABOUT, 0);
		} break;

		case LC_TOOLBAR_ANIMATION:
		{
			CalculateStep();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateAnimation(false, m_bAddKeys);
			SystemUpdateTime(false, m_ActiveModel->m_CurFrame, 255);
		} break;
		
		case LC_TOOLBAR_ADDKEYS:
		{
			m_bAddKeys = !m_bAddKeys;
			SystemUpdateAnimation(false, m_bAddKeys);
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
				if ((m_nSnap & LC_DRAW_SNAP_XYZ) == LC_DRAW_SNAP_XYZ)
					m_nSnap &= ~LC_DRAW_SNAP_XYZ;
				else
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
			}
			SystemUpdateSnap(m_nSnap);
		} break;

		case LC_TOOLBAR_BACKGROUND:
		case LC_TOOLBAR_FASTRENDER:
		{
			if (id == LC_TOOLBAR_BACKGROUND)
			{
				if (m_nDetail & LC_DET_BACKGROUND) 
					m_nDetail &= ~LC_DET_BACKGROUND; 
				else
					m_nDetail |= LC_DET_BACKGROUND;
			}
			else
			{
				if (m_nDetail & LC_DET_FAST) 
					m_nDetail &= ~(LC_DET_FAST | LC_DET_BACKGROUND); 
				else
					m_nDetail |= LC_DET_FAST; 
				UpdateAllViews();
			}

			SystemUpdateRenderingMode((m_nDetail & LC_DET_BACKGROUND) != 0, (m_nDetail & LC_DET_FAST) != 0);
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
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_0:
		{
			m_nAngleSnap = 1;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_1:
		{
			m_nAngleSnap = 5;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_2:
		{
			m_nAngleSnap = 10;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_3:
		{
			m_nAngleSnap = 15;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_4:
		{
			m_nAngleSnap = 30;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_5:
		{
			m_nAngleSnap = 45;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_6:
		{
			m_nAngleSnap = 60;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_7:
		{
			m_nAngleSnap = 90;
			SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
		} break;

		case LC_EDIT_ANGLE_SNAP_8:
		{
			m_nAngleSnap = 180;
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

		case LC_EDIT_ACTION_ORBIT:
		{
			SetAction(LC_ACTION_ORBIT);
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

	SystemUpdateAction(nAction, m_nCurAction);
	m_nCurAction = nAction;

	if ((m_nCurAction == LC_ACTION_MOVE) || (m_nCurAction == LC_ACTION_ROTATE) || (m_nCurAction == LC_ACTION_ORBIT))
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
	/* FIXME: groups
	bool recurse = false;
	Group *g1, *g2;
	Piece* pPiece;
	int ref;

	for (g1 = m_pGroups; g1;)
	{
		ref = 0;

		for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
			if (pPiece->GetGroup() == g1)
				ref++;

		for (g2 = m_pGroups; g2; g2 = g2->m_pNext)
			if (g2->m_pGroup == g1)
				ref++;

		if (ref < 2)
		{
			if (ref != 0)
			{
				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
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
		*/
}

Group* Project::AddGroup (const char* name, Group* pParent, float x, float y, float z)
{
	/*
	FIXME: addgroup
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
	*/
return NULL;
}

void Project::SelectAndFocusNone(bool FocusOnly)
{
	// TODO: Move to the model class
	if (FocusOnly)
	{
	  for (lcObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
		  Piece->SetFocus(false, true);

		for (lcObject* Camera = m_ActiveModel->m_Cameras; Camera; Camera = Camera->m_Next)
			Camera->SetFocus(false, true);

		for (lcObject* Light = m_ActiveModel->m_Lights; Light; Light = Light->m_Next)
			Light->SetFocus(false, true);
	}
	else
	{
	  for (lcObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
		  Piece->SetSelection(false, true);

		for (lcObject* Camera = m_ActiveModel->m_Cameras; Camera; Camera = Camera->m_Next)
			Camera->SetSelection(false, true);

		for (lcObject* Light = m_ActiveModel->m_Lights; Light; Light = Light->m_Next)
			Light->SetSelection(false, true);
	}

//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);
}

bool Project::GetSelectionCenter(Vector3& Center) const
{
	bool Selected = false;

	BoundingBox Box;
	Box.Reset();

	for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
	{
		if (Piece->IsSelected())
		{
			Piece->MergeBoundingBox(&Box);
			Selected = true;
		}
	}

	Center = Box.GetCenter();

	return Selected;
}

void Project::GetActiveViewportMatrices(Matrix44& ModelView, Matrix44& Projection, int Viewport[4])
{
	Viewport[0] = 0;
	Viewport[1] = 0;
	Viewport[2] = m_ActiveView->GetWidth();
	Viewport[3] = m_ActiveView->GetHeight();

	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	lcCamera* Camera = m_ActiveView->GetCamera();

	// Build the matrices.
	ModelView = Camera->m_WorldView;
	Projection = CreatePerspectiveMatrix(Camera->m_FOV, Aspect, Camera->m_NearDist, Camera->m_FarDist);
}

void Project::ConvertToUserUnits(Vector3& Value) const
{
	if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
		Value /= 0.08f;
}

void Project::ConvertFromUserUnits(Vector3& Value) const
{
	if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
		Value *= 0.08f;
}

bool Project::GetFocusPosition(Vector3& Position) const
{
	lcObject* Focus = GetFocusObject();

	if (Focus)
	{
		Position = Focus->m_WorldPosition;
		return true;
	}

	Position = Vector3(0, 0, 0);

	return false;
}

// Returns the object that currently has focus.
lcObject* Project::GetFocusObject() const
{
	lcObject* Focus = NULL;

	for (lcObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
	{
		Focus = Piece->GetFocusedChildren();

		if (Focus)
			return Focus;
	}

	for (lcObject* Camera = m_ActiveModel->m_Cameras; Camera; Camera = Camera->m_Next)
	{
		Focus = Camera->GetFocusedChildren();

		if (Focus)
			return Focus;
	}

	// TODO: light

	return Focus;
}

// Find a good starting position/orientation relative to an existing piece.
void Project::GetPieceInsertPosition(lcPieceObject* OffsetPiece, Vector3& Position, Vector4& Rotation)
{
	Vector3 Dist(0, 0, 0);

	if (OffsetPiece)
		Dist[2] += OffsetPiece->m_BoundingBox.m_Max[2];

	lcPieceObject* Selection = g_App->m_PiecePreview->m_Selection;
	if (Selection)
		Dist[2] -= Selection->m_BoundingBox.m_Min[2];

	SnapVector(Dist);

	if (OffsetPiece)
	{
		Position = Mul31(Dist, OffsetPiece->m_ModelWorld);
		Rotation = OffsetPiece->m_AxisAngle;
	}
	else
	{
		Position = Dist;
		Rotation = Vector4(0, 0, 1, 0);
	}
}

// Try to find a good starting position/orientation for a new piece.
void Project::GetPieceInsertPosition(int MouseX, int MouseY, Vector3& Position, Vector4& Rotation)
{
	// See if the mouse is over a piece.
	lcPieceObject* HitPiece = (lcPieceObject*)FindObjectFromPoint(MouseX, MouseY, true);

	if (HitPiece)
	{
		GetPieceInsertPosition(HitPiece, Position, Rotation);
		return;
	}

	// Try to hit the base grid.
	int Viewport[4] = { 0, 0, m_ActiveView->GetWidth(), m_ActiveView->GetHeight() };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	lcCamera* Camera = m_ActiveView->GetCamera();

	// Build the matrices.
	Matrix44 ModelView = Camera->m_WorldView;
	Matrix44 Projection = CreatePerspectiveMatrix(Camera->m_FOV, Aspect, Camera->m_NearDist, Camera->m_FarDist);

	Vector3 ClickPoints[2] = { Vector3((float)m_nDownX, (float)m_nDownY, 0.0f), Vector3((float)m_nDownX, (float)m_nDownY, 1.0f) };
	UnprojectPoints(ClickPoints, 2, ModelView, Projection, Viewport);

	Vector3 Intersection;
	if (LinePlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], Vector4(0, 0, 1, 0)))
	{
		lcPieceObject* Selection = g_App->m_PiecePreview->m_Selection;
		if (Selection)
			Intersection[2] -= Selection->m_BoundingBox.m_Min[2];

		SnapVector(Intersection);
		Position = Intersection;
		Rotation = Vector4(0, 0, 1, 0);
		return;
	}

	// Couldn't find a good position, so just place the piece somewhere near the camera.
	Position = UnprojectPoint(Vector3((float)m_nDownX, (float)m_nDownY, 0.9f), ModelView, Projection, Viewport);
	Rotation = Vector4(0, 0, 1, 0);
}

lcObject* Project::FindObjectFromPoint(int x, int y, bool PiecesOnly)
{
	LC_CLICK_RAY ClickRay;

	// TODO: use math functions instead of OpenGL.
	GLdouble px, py, pz, rx, ry, rz;
	GLdouble modelMatrix[16], projMatrix[16];
	GLint viewport[4];

	m_ActiveView->LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX,projMatrix);
	glGetIntegerv(GL_VIEWPORT,viewport);

	// Unproject the selected point against both the front and the back clipping plane
	gluUnProject(x, y, 0, modelMatrix, projMatrix, viewport, &px, &py, &pz);
	gluUnProject(x, y, 1, modelMatrix, projMatrix, viewport, &rx, &ry, &rz);

	ClickRay.Start = Vector3((float)px, (float)py, (float)pz);
	ClickRay.End = Vector3((float)(rx-px), (float)(ry-py), (float)(rz-pz));
	ClickRay.Dist = FLT_MAX;
	ClickRay.Object = NULL;

	for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
		if (Piece->IsVisible(m_ActiveModel->m_CurFrame))
			Piece->ClosestRayIntersect(&ClickRay);

	if (!PiecesOnly)
	{
		for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
			if (Camera != m_ActiveView->GetCamera() && Camera->IsVisible(m_ActiveModel->m_CurFrame))
				Camera->ClosestRayIntersect(&ClickRay);

		for (lcLight* Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
			Light->ClosestRayIntersect(&ClickRay);
	}

	return (lcObject*)ClickRay.Object;
}

void Project::FindObjectsInBox(float x1, float y1, float x2, float y2, PtrArray<lcObject>& Objects)
{
	int Viewport[4] = { 0, 0, m_ActiveView->GetWidth(), m_ActiveView->GetHeight() };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];
	lcCamera* Camera = m_ActiveView->GetCamera();

	// Build the matrices.
	Matrix44 ModelView = Camera->m_WorldView;
	Matrix44 Projection = CreatePerspectiveMatrix(Camera->m_FOV, Aspect, Camera->m_NearDist, Camera->m_FarDist);

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

	Planes[0] = Vector4(Cross3(Corners[4] - Corners[0], Corners[1] - Corners[0]).Normalize()); // Left
	Planes[1] = Vector4(Cross3(Corners[5] - Corners[2], Corners[3] - Corners[2]).Normalize()); // Right
	Planes[2] = Vector4(Cross3(Corners[3] - Corners[0], Corners[4] - Corners[0]).Normalize()); // Top
	Planes[3] = Vector4(Cross3(Corners[1] - Corners[2], Corners[5] - Corners[2]).Normalize()); // Bottom
	Planes[4] = Vector4(Cross3(Corners[1] - Corners[0], Corners[3] - Corners[0]).Normalize()); // Front
	Planes[5] = Vector4(Cross3(Corners[1] - Corners[2], Corners[3] - Corners[2]).Normalize()); // Back

	Planes[0][3] = -Dot3(Planes[0], Corners[0]);
	Planes[1][3] = -Dot3(Planes[1], Corners[5]);
	Planes[2][3] = -Dot3(Planes[2], Corners[0]);
	Planes[3][3] = -Dot3(Planes[3], Corners[5]);
	Planes[4][3] = -Dot3(Planes[4], Corners[0]);
	Planes[5][3] = -Dot3(Planes[5], Corners[5]);

	// Check if any objects are inside the volume.
	for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece != NULL; Piece = (lcPieceObject*)Piece->m_Next)
	{
		if (!Piece->IsVisible(m_ActiveModel->m_CurFrame))
			continue;

		if (Piece->IntersectsVolume(Planes, 6))
			Objects.Add(Piece);
	}

	for (Camera = m_ActiveModel->m_Cameras; Camera != NULL; Camera = (lcCamera*)Camera->m_Next)
	{
		if (!Camera->IsVisible(m_ActiveModel->m_CurFrame))
			continue;

		if (Camera->IntersectsVolume(Planes, 6))
			Objects.Add(Camera);
	}

	for (lcLight* Light = m_ActiveModel->m_Lights; Light != NULL; Light = (lcLight*)Light->m_Next)
	{
		if (!Light->IsVisible(m_ActiveModel->m_CurFrame))
			continue;

		if (Light->IntersectsVolume(Planes, 6))
			Objects.Add(Light);
	}
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
					PtrArray<lcObject> Objects;
					FindObjectsInBox((float)m_nDownX, (float)m_nDownY, m_fTrack[0], m_fTrack[1], Objects);

					// Deselect old pieces.
					bool Control = Sys_KeyDown(KEY_CONTROL);
					SelectAndFocusNone(Control);

					// Select new pieces.
					for (int i = 0; i < Objects.GetSize(); i++)
						Objects[i]->SetSelection(true, true);
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
				SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
				SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
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
			case LC_ACTION_ORBIT:
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
				lcCamera* Camera = m_ActiveView->GetCamera();

				// Build the matrices.
				Matrix44 ModelView = Camera->m_WorldView;
				Matrix44 Projection = CreatePerspectiveMatrix(Camera->m_FOV, Aspect, Camera->m_NearDist, Camera->m_FarDist);

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
				Vector3 Eye = Camera->m_WorldPosition;
				Eye = Eye + (Points[0] - Points[1]);

				Vector3 Target = Camera->m_Children->m_WorldPosition;
				Target = Target + (Points[0] - Points[1]);

				// Zoom in/out.
				float RatioX = (Right - Left) / Viewport[2];
				float RatioY = (Top - Bottom) / Viewport[3];
				float ZoomFactor = -max(RatioX, RatioY) + 0.75f;

				Vector3 Dir = Points[1] - Points[2];
				Eye = Eye + Dir * ZoomFactor;
				Target = Target + Dir * ZoomFactor;

				// Change the camera and redraw.
				Camera->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, Eye);
				Camera->m_Children->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, Target);
			  Camera->Update(m_ActiveModel->m_CurFrame);

				SystemUpdateFocus(NULL);
				UpdateAllViews();
			} break;

			case LC_ACTION_INSERT:
			case LC_ACTION_LIGHT:
			case LC_ACTION_ERASER:
			case LC_ACTION_PAINT:
				break;
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

void Project::GetSnapIndex(int* SnapXY, int* SnapZ) const
{
	*SnapXY = (m_nMoveSnap & 0xff);
	*SnapZ = ((m_nMoveSnap >> 8) & 0xff);
}

void Project::GetSnapDistance(float* SnapXY, float* SnapZ) const
{
	const float SnapXYTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 1.6f, 2.4f, 3.2f, 6.4f };
	const float SnapZTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 0.96f, 1.92f, 3.84f, 7.68f };

	int SXY, SZ;
	GetSnapIndex(&SXY, &SZ);

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
		GetSnapIndex(&SXY, &SZ);

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
		SnapVector(Move, Remainder);

	if (Move.LengthSquared() < 0.00001f)
		return false;

	// Transform the translation if we're in relative mode.
	if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
	{
		lcObject* Focus = GetFocusObject();

		if ((Focus != NULL) && Focus->IsPieceObject())
		{
			Move = Mul30(Move, ((lcPieceObject*)Focus)->m_ModelWorld);
		}
	}

	for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
	{
			Camera->Move(m_ActiveModel->m_CurFrame, m_bAddKeys, Move);
			Camera->Update(m_ActiveModel->m_CurFrame);
	}

	for (lcLight* Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
	{
	    Light->Move(m_ActiveModel->m_CurFrame, m_bAddKeys, Move);
			Light->Update(m_ActiveModel->m_CurFrame);
	}

	for (lcPieceObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
	{
			Piece->Move(m_ActiveModel->m_CurFrame, m_bAddKeys, Move);
			Piece->Update(m_ActiveModel->m_CurFrame);
	}

	// TODO: move group centers

	if (m_OverlayActive)
	{
		if (!GetFocusPosition(m_OverlayCenter))
			GetSelectionCenter(m_OverlayCenter);
	}

	return true;
}

bool Project::RotateSelectedObjects(Vector3& Delta, Vector3& Remainder, bool Snap)
{
	// Don't move along locked directions.
	if (m_nSnap & LC_DRAW_LOCK_X)
		Delta[0] = 0;

	if (m_nSnap & LC_DRAW_LOCK_Y)
		Delta[1] = 0;

	if (m_nSnap & LC_DRAW_LOCK_Z)
		Delta[2] = 0;

	// Snap.
	if (Snap)
		SnapRotationVector(Delta, Remainder);

	if (Delta.LengthSquared() < 0.001f)
		return false;

	BoundingBox Box;
	Box.Reset();

	lcPieceObject* Piece;
	lcPieceObject* Focus = NULL;
	int nSel = 0;

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
	{
		if (Piece->IsSelected())
		{
			if (Piece->IsFocused())
				Focus = Piece;

			Piece->MergeBoundingBox(&Box);
			nSel++;
		}
	}

	Vector3 Center;
	if (Focus != NULL)
		Center = Focus->m_WorldPosition;
	else
		Center = Box.GetCenter();

	// Create the rotation matrix.
	Quaternion Rotation(0, 0, 0, 1);
	Quaternion WorldToFocus, FocusToWorld;

	if (!(m_nSnap & LC_DRAW_LOCK_X) && (Delta[0] != 0.0f))
	{
		Quaternion q = CreateRotationXQuaternion(Delta[0] * LC_DTOR);
		Rotation = Mul(q, Rotation);
	}

	if (!(m_nSnap & LC_DRAW_LOCK_Y) && (Delta[1] != 0.0f))
	{
		Quaternion q = CreateRotationYQuaternion(Delta[1] * LC_DTOR);
		Rotation = Mul(q, Rotation);
	}

	if (!(m_nSnap & LC_DRAW_LOCK_Z) && (Delta[2] != 0.0f))
	{
		Quaternion q = CreateRotationZQuaternion(Delta[2] * LC_DTOR);
		Rotation = Mul(q, Rotation);
	}

	// Transform the rotation relative to the focused piece.
	if (m_nSnap & LC_DRAW_GLOBAL_SNAP)
		Focus = NULL;

	if (Focus != NULL)
	{
		Vector4 Rot = Focus->m_AxisAngle;
		FocusToWorld = QuaternionFromAxisAngle(Rot);

		Rot[3] = -Rot[3];
		WorldToFocus = QuaternionFromAxisAngle(Rot);

		Rotation = Mul(FocusToWorld, Rotation);
	}

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
	{
		if (!Piece->IsSelected())
			continue;

		Vector3 Pos = Piece->m_WorldPosition;
		Vector4 Rot = Piece->m_AxisAngle;

		Vector4 NewRotation;

		if ((nSel == 1) && (Focus == Piece))
		{
			Quaternion LocalToWorld = QuaternionFromAxisAngle(Rot);
			Quaternion NewLocalToWorld;

			if (Focus != NULL)
			{
				Quaternion LocalToFocus = Mul(WorldToFocus, LocalToWorld);
				NewLocalToWorld = Mul(LocalToFocus, Rotation);
			}
			else
			{
				NewLocalToWorld = Mul(Rotation, LocalToWorld);
			}

			NewRotation = QuaternionToAxisAngle(NewLocalToWorld);
		}
		else
		{
			Vector3 Distance = Pos - Center;

			Quaternion LocalToWorld = QuaternionFromAxisAngle(Rot);
			Quaternion NewLocalToWorld;

			if (Focus != NULL)
			{
				Quaternion LocalToFocus = Mul(WorldToFocus, LocalToWorld);
				NewLocalToWorld = Mul(Rotation, LocalToFocus);

				Quaternion WorldToLocal = QuaternionFromAxisAngle(Vector4(Rot[0], Rot[1], Rot[2], -Rot[3]));

				Distance = Mul(Distance, WorldToLocal);
				Distance = Mul(Distance, NewLocalToWorld);
			}
			else
			{
				NewLocalToWorld = Mul(Rotation, LocalToWorld);

				Distance = Mul(Distance, Rotation);
			}

			NewRotation = QuaternionToAxisAngle(NewLocalToWorld);

			Pos = Center + Distance;

			Piece->SetPosition(m_ActiveModel->m_CurFrame, m_bAddKeys, Pos);
		}

		Piece->SetRotation(m_ActiveModel->m_CurFrame, m_bAddKeys, NewRotation);
		Piece->Update(m_ActiveModel->m_CurFrame);
	}

//	for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
//		if (pPiece->IsSelected())
//			pPiece->CalculateConnections(m_pConnections, m_ActiveModel->m_CurFrame, false, true);

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

	// TODO: Almost all of this should go through the keyboard shortcut system.
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
			  HandleCommand(LC_VIEW_ZOOM, (unsigned int)-10);
			else
			  HandleCommand(LC_VIEW_ZOOM, (unsigned int)-1);

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
/* FIXME: tab focus
			if (m_ActiveModel->m_Pieces == NULL)
				break;

			Piece* pFocus = NULL, *pPiece;
			for (pFocus = m_ActiveModel->m_Pieces; pFocus; pFocus = (Piece*)pFocus->m_Next)
				if (pFocus->IsFocused())
					break;

			SelectAndFocusNone(false);

			if (pFocus == NULL)
			{
				if (bShift)
				{
					// Focus the last visible piece.
					for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
						if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
							pFocus = pPiece;
				}
				else
				{
					// Focus the first visible piece.
					for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
						if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
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
						if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
							pBest = pPiece;

						if (pPiece->m_Next != NULL)
						{
							if (pPiece->m_Next == pFocus)
								break;
							else
								pPiece = (Piece*)pPiece->m_Next;
						}
						else
						{
							if (pFocus == m_ActiveModel->m_Pieces)
								break;
							else
								pPiece = m_ActiveModel->m_Pieces;
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
							if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
							{
								pFocus = pPiece;
								break;
							}

						if (pPiece->m_Next != NULL)
						{
							if (pPiece->m_Next == pFocus)
								break;
							else
								pPiece = (Piece*)pPiece->m_Next;
						}
						else
						{
							if (pFocus == m_ActiveModel->m_Pieces)
								break;
							else
								pPiece = m_ActiveModel->m_Pieces;
						}
					}
				}
			}

			if (pFocus != NULL)
			{
        pFocus->Select (true, true);
        Group* pGroup = pFocus->GetTopGroup();
        if (pGroup != NULL)
        {
          for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (Piece*)pPiece->m_Next)
            if ((pPiece->IsVisible(m_ActiveModel->m_CurFrame)) &&
                (pPiece->GetTopGroup() == pGroup))
              pPiece->Select (true, false);
        }
			}

			UpdateSelection();
			UpdateAllViews();
			SystemUpdateFocus(pFocus);
			ret = true;
*/
		} break;

		case KEY_UP:    case KEY_DOWN: case KEY_LEFT: 
		case KEY_RIGHT: case KEY_NEXT: case KEY_PRIOR:
//		if (AnyObjectSelected(FALSE))
		{
			Vector3 axis;
			if (bShift)
			{
				if ((m_nSnap & LC_DRAW_SNAP_A) && !bControl)
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
					case KEY_UP: {
						axis[1] = axis[2] = 0; axis[0] = -axis[0];
					} break;
					case KEY_DOWN: {
						axis[1] = axis[2] = 0;
					} break;
					case KEY_LEFT: {
						axis[0] = axis[2] = 0; axis[1] = -axis[1];
					} break;
					case KEY_RIGHT: {
						axis[0] = axis[2] = 0;
					} break;
					case KEY_NEXT: {
						axis[0] = axis[1] = 0; axis[2] = -axis[2];
					} break;
					case KEY_PRIOR: {
						axis[0] = axis[1] = 0;
					} break;
				}
			}
			else
			{
        lcCamera* Camera = m_ActiveView->GetCamera();

        if (Camera->IsSide())
        {
					Matrix44 mat = Camera->m_ViewWorld;

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

          axis = Mul30(axis, mat);
        }
        else
        {

          // TODO: rewrite this

  				switch (nKey)
	  			{
		  			case KEY_UP: {
			  			axis[1] = axis[2] = 0; axis[0] = -axis[0];
				  	} break;
					  case KEY_DOWN: {
              axis[1] = axis[2] = 0;
            } break;
            case KEY_LEFT: {
              axis[0] = axis[2] = 0; axis[1] = -axis[1];
            } break;
            case KEY_RIGHT: {
              axis[0] = axis[2] = 0;
            } break;
            case KEY_NEXT: {
              axis[0] = axis[1] = 0; axis[2] = -axis[2];
            } break;
            case KEY_PRIOR: {
              axis[0] = axis[1] = 0;
            } break;
          }

  				GLdouble modelMatrix[16], projMatrix[16], p1[3], p2[3], p3[3];
	  			float ax, ay;
		  		GLint viewport[4];

          glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
          glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
          glGetIntegerv(GL_VIEWPORT, viewport);
          gluUnProject( 5, 5, 0.1, modelMatrix,projMatrix,viewport,&p1[0],&p1[1],&p1[2]);
          gluUnProject(10, 5, 0.1, modelMatrix,projMatrix,viewport,&p2[0],&p2[1],&p2[2]);
          gluUnProject( 5,10, 0.1, modelMatrix,projMatrix,viewport,&p3[0],&p3[1],&p3[2]);
				
          Vector3 vx = Normalize(Vector3((float)(p2[0] - p1[0]), (float)(p2[1] - p1[1]), 0));//p2[2] - p1[2] };
          Vector3 x(1, 0, 0);
          ax = acosf(Dot3(vx, x));
				
          Vector3 vy = Normalize(Vector3((float)(p3[0] - p1[0]), (float)(p3[1] - p1[1]), 0));//p2[2] - p1[2] };
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
				RotateSelectedObjects(axis, tmp, false);
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
	m_PreviousAction = m_nCurAction;

	StartTracking(LC_TRACK_LEFT);
	SetAction(LC_ACTION_INSERT);
}

void Project::OnLeftButtonDown(View* view, int x, int y, bool bControl, bool bShift)
{
	GLdouble modelMatrix[16], projMatrix[16], point[3];
	GLint viewport[4];

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

	m_ActiveView->LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluUnProject(x, y, 0.9, modelMatrix, projMatrix, viewport, &point[0], &point[1], &point[2]);
	m_fTrack[0] = (float)point[0]; m_fTrack[1] = (float)point[1]; m_fTrack[2] = (float)point[2];

	switch (m_nCurAction)
	{
		case LC_ACTION_SELECT:
		{
			lcObject* Object = FindObjectFromPoint(x, y);

			if (Object)
			{
				bool Focus = Object->IsFocused();

				SelectAndFocusNone(bControl);

				if (Focus)
					Object->SetSelection(false, true);
				else
					Object->SetFocus(true, false);
			}
			else
				SelectAndFocusNone(bControl);

			UpdateSelection();
			UpdateAllViews();
			SystemUpdateFocus(Object);

			StartTracking(LC_TRACK_START_LEFT);

		} break;

		case LC_ACTION_ERASER:
		{
			lcObject* Object = FindObjectFromPoint(x, y);

			if (!Object)
				break;

/* FIXME: mouse click
				switch (Object->GetType ())
				{
					case LC_OBJECT_PIECE:
					{
						Piece* pPiece = (Piece*)ClickLine.pClosest;
						RemovePiece(pPiece);
						delete pPiece;
//						CalculateStep();
						RemoveEmptyGroups();
					} break;

					case LC_OBJECT_CAMERA:
					case LC_OBJECT_CAMERA_TARGET:
					{
						Camera* pCamera;
						if (ClickLine.pClosest->GetType () == LC_OBJECT_CAMERA)
							pCamera = (Camera*)ClickLine.pClosest;
						else
							pCamera = ((CameraTarget*)ClickLine.pClosest)->GetParent();
						bool bCanDelete = pCamera->IsUser();

						for (int i = 0; i < m_ViewList.GetSize(); i++)
							if (pCamera == m_ViewList[i]->GetCamera())
								bCanDelete = false;

						if (bCanDelete)
						{
							Camera* pPrev;
							for (pPrev = m_ActiveModel->m_Cameras; pPrev; pPrev = (Camera*)pPrev->m_Next)
								if (pPrev->m_Next == pCamera)
								{
									pPrev->m_Next = pCamera->m_Next;
									delete pCamera;
									SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
									SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
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
*/
		} break;

		case LC_ACTION_PAINT:
		{
			lcObject* Object = FindObjectFromPoint(x, y);

			if (!Object || !Object->IsPieceObject())
				break;

			lcPieceObject* Piece = (lcPieceObject*)Object;

			if (Piece->m_Color != m_nCurColor)
			{
				Piece->m_Color = m_nCurColor;

				SetModifiedFlag(true);
				CheckPoint("Painting");
				SystemUpdateFocus(NULL);
				UpdateAllViews();
			}
		} break;

		case LC_ACTION_INSERT:
		{
			Vector3 Pos;
			Vector4 Rot;

			GetPieceInsertPosition(x, y, Pos, Rot);

			AddPiece(Pos, Rot);
		} break;

		case LC_ACTION_LIGHT:
		{
			GLint max;
			int count = 0;
			lcLight* Light;

			// TODO: Warn user that the maximum light count was reached but still add the light anyway.
			glGetIntegerv(GL_MAX_LIGHTS, &max);
			for (Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
				count++;

			if (count == max)
				break;

			Light = new lcLight();
			Light->CreateLight(LC_LIGHT_POINT);
			Light->SetPosition(1, false, Vector3(m_fTrack[0], m_fTrack[1], m_fTrack[2]));
			Light->SetUniqueName(m_ActiveModel->m_Lights, "Point Light");
			m_ActiveModel->AddLight(Light);

			SelectAndFocusNone(false);
			Light->SetFocus(true, false);
			SystemUpdateFocus(Light);
			UpdateSelection();

//			AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)pNew, OT_PIECE);
			UpdateSelection();
			UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Adding Light");
		} break;

		case LC_ACTION_SPOTLIGHT:
		{
			GLint max;
			int count = 0;
			lcLight* Light;

			// TODO: Warn user that the maximum light count was reached but still add the light anyway.
			glGetIntegerv(GL_MAX_LIGHTS, &max);
			for (Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
				count++;

			if (count == max)
				break;

			double tmp[3];
			gluUnProject(x+1, y-1, 0.9, modelMatrix, projMatrix, viewport, &tmp[0], &tmp[1], &tmp[2]);

			StartTracking(LC_TRACK_START_LEFT);

			Light = new lcLight();
			Light->CreateLight(LC_LIGHT_SPOT);
			Light->SetPosition(1, false, Vector3(m_fTrack[0], m_fTrack[1], m_fTrack[2]));
			Light->m_Children->SetPosition(1, false, Vector3((float)tmp[0], (float)tmp[1], (float)tmp[2]));
			m_ActiveModel->AddLight(Light);

			SelectAndFocusNone(false);
			Light->m_Children->SetFocus(true, false);
			UpdateSelection();
			UpdateAllViews();
			SystemUpdateFocus(Light);
		} break;

		case LC_ACTION_CAMERA:
		{
			double tmp[3];
			gluUnProject(x+1, y-1, 0.9, modelMatrix, projMatrix, viewport, &tmp[0], &tmp[1], &tmp[2]);
			SelectAndFocusNone(false);
			StartTracking(LC_TRACK_START_LEFT);

			lcCamera* Camera = new lcCamera();
			Camera->CreateCamera(LC_CAMERA_USER, true); // TODO: Create free cameras.
			Camera->SetUniqueName(m_ActiveModel->m_Cameras, "Camera");

			Camera->SetSelection(true, false);
			Camera->SetPosition(1, false, Vector3(m_fTrack[0], m_fTrack[1], m_fTrack[2]));
			Camera->SetSelection(false, false);

			Camera->m_Children->SetFocus(true, false);
			Camera->m_Children->SetPosition(1, false, Vector3((float)tmp[0], (float)tmp[1], (float)tmp[2]));

			Camera->Update(m_ActiveModel->m_CurFrame);

			m_ActiveModel->AddCamera(Camera);
			UpdateSelection();
			UpdateAllViews();
			SystemUpdateFocus(Camera);
		} break;

		case LC_ACTION_MOVE:
		{
//			if (m_ActiveModel->AnyObjectsSelected())
			{
				StartTracking(LC_TRACK_START_LEFT);
				m_OverlayDelta = Vector3(0.0f, 0.0f, 0.0f);
				m_MouseSnapLeftover = Vector3(0.0f, 0.0f, 0.0f);
			}
		} break;

		case LC_ACTION_ROTATE:
		{
//			for (lcObject* Piece = m_ActiveModel->m_Pieces; Piece; Piece = Piece->m_Next)
			{
//				if (Piece->IsSelected())
				{
					StartTracking(LC_TRACK_START_LEFT);
					m_OverlayDelta = Vector3(0.0f, 0.0f, 0.0f);
					m_MouseSnapLeftover = Vector3(0.0f, 0.0f, 0.0f);
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
		case LC_ACTION_ORBIT:
		{
			StartTracking(LC_TRACK_START_LEFT);
		} break;
	}
}

void Project::OnLeftButtonDoubleClick(View* view, int x, int y, bool bControl, bool bShift)
{
	GLdouble modelMatrix[16], projMatrix[16], point[3];
	GLint viewport[4];

	if (SetActiveView(view))
		return;

	m_ActiveView->LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	// why this is here ?
	gluUnProject(x, y, 0.9, modelMatrix, projMatrix, viewport, &point[0], &point[1], &point[2]);
	m_fTrack[0] = (float)point[0]; m_fTrack[1] = (float)point[1]; m_fTrack[2] = (float)point[2];

	lcObject* Object = FindObjectFromPoint(x, y);

	if (Object)
	{
		bool Focus = Object->IsFocused();

		SelectAndFocusNone(bControl);

		if (Focus)
			Object->SetSelection(false, true);
		else
			Object->SetFocus(true, false);
	}
	else
		SelectAndFocusNone(bControl);

	UpdateSelection();
	UpdateAllViews();
	SystemUpdateFocus(Object);
}

void Project::OnLeftButtonUp(View* view, int x, int y, bool bControl, bool bShift)
{
	if (m_nTracking == LC_TRACK_LEFT)
	{
		// Dragging a new piece from the tree.
		if (m_nCurAction == LC_ACTION_INSERT)
		{
			if ((x > 0) && (x < m_ActiveView->GetWidth()) && (y > 0) && (y < m_ActiveView->GetHeight()))
			{
				Vector3 Pos;
				Vector4 Rot;

				GetPieceInsertPosition(x, y, Pos, Rot);

				AddPiece(Pos, Rot);

				if (!(m_nSnap & LC_DRAW_MOVE))
					SetAction(m_PreviousAction);
			}
			else
				SetAction(m_PreviousAction);
		}
	}

	StopTracking(true);
}

void Project::OnRightButtonDown(View* view, int x, int y, bool bControl, bool bShift)
{
	GLdouble modelMatrix[16], projMatrix[16], point[3];
	GLint viewport[4];

	if (StopTracking(false))
		return;

	if (SetActiveView(view))
		return;

	m_nDownX = x;
	m_nDownY = y;
	m_bTrackCancel = false;

	m_ActiveView->LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluUnProject(x, y, 0.9, modelMatrix, projMatrix, viewport, &point[0], &point[1], &point[2]);
	m_fTrack[0] = (float)point[0]; m_fTrack[1] = (float)point[1]; m_fTrack[2] = (float)point[2];

	switch (m_nCurAction)
	{
		case LC_ACTION_MOVE:
		{
			if (m_ActiveModel->AnyObjectsSelected())
      {
				StartTracking(LC_TRACK_START_RIGHT);
        m_fTrack[0] = m_fTrack[1] = m_fTrack[2] = 0.0f;
      }
		} break;

		case LC_ACTION_ROTATE:
		{
			if (m_ActiveModel->AnyPiecesSelected())
			{
				StartTracking(LC_TRACK_START_RIGHT);
				m_fTrack[0] = m_fTrack[1] = m_fTrack[2] = 0.0f;
			}
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
		{
			MouseUpdateOverlays(x, y);
		}

		return;
	}

	if (m_nTracking == LC_TRACK_START_RIGHT)
		m_nTracking = LC_TRACK_RIGHT;

	if (m_nTracking == LC_TRACK_START_LEFT)
		m_nTracking = LC_TRACK_LEFT;

	GLdouble modelMatrix[16], projMatrix[16], tmp[3];
	GLint viewport[4];
	float ptx, pty, ptz;

	m_ActiveView->LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluUnProject(x, y, 0.9, modelMatrix, projMatrix, viewport, &tmp[0], &tmp[1], &tmp[2]);
	ptx = (float)tmp[0]; pty = (float)tmp[1]; ptz = (float)tmp[2];

	switch (m_nCurAction)
	{
		case LC_ACTION_SELECT:
		{
			int ptx = x, pty = y;

			if (ptx >= viewport[0] + viewport[2])
				ptx = viewport[0] + viewport[2] - 1;
			else if (ptx <= viewport[0])
				ptx = viewport[0] + 1;

			if (pty >= viewport[1] + viewport[3])
				pty = viewport[1] + viewport[3] - 1;
			else if (pty <= viewport[1])
				pty = viewport[1] + 1;

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
			Vector3 Delta((ptx - m_fTrack[0])*mouse, (pty - m_fTrack[1])*mouse, (ptz - m_fTrack[2])*mouse);

			m_fTrack[0] = ptx;
			m_fTrack[1] = pty;
			m_fTrack[2] = ptz;
			
			lcObject* Light = m_ActiveModel->m_Lights;
			while (Light->m_Next)
				Light = Light->m_Next;

			Light->Move(1, false, Delta);
			Light->Update(1);

			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;

		case LC_ACTION_CAMERA:
		{
			float mouse = 10.0f/(21 - m_nMouse);
			Vector3 Delta((ptx - m_fTrack[0])*mouse, (pty - m_fTrack[1])*mouse, (ptz - m_fTrack[2])*mouse);

			m_fTrack[0] = ptx;
			m_fTrack[1] = pty;
			m_fTrack[2] = ptz;
			
			lcObject* Camera = m_ActiveModel->m_Cameras;
			while (Camera->m_Next != NULL)
				Camera = Camera->m_Next;

			Camera->Move(1, false, Delta);
			Camera->Update(1);

			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;

		case LC_ACTION_MOVE:
		{
			// Check if the mouse moved since the last update.
			if ((x == m_nDownX) && (y == m_nDownY))
				break;

			lcCamera* Camera = m_ActiveView->GetCamera();
			bool Redraw;

			if ((m_OverlayActive && (m_OverlayMode != LC_OVERLAY_XYZ)) || (!Camera->IsSide()))
			{
				Vector3 ScreenX = Vector3(Camera->m_ViewWorld[0]);
				Vector3 ScreenY = Vector3(Camera->m_ViewWorld[1]);

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
					lcObject* Focus = GetFocusObject();

					if ((Focus != NULL) && Focus->IsPieceObject())
					{
						Axis1 = Mul30(Dir1, ((lcPieceObject*)Focus)->m_ModelWorld);
						Axis2 = Mul30(Dir2, ((lcPieceObject*)Focus)->m_ModelWorld);
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
				Vector3 ScreenX = Vector3(Camera->m_ViewWorld[0]);
				Vector3 ScreenY = Vector3(Camera->m_ViewWorld[1]);
				Vector3 ScreenZ = Vector3(Camera->m_ViewWorld[2]);

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
			lcCamera* Camera = m_ActiveView->GetCamera();
			bool Redraw;

			if ((m_OverlayActive && (m_OverlayMode != LC_OVERLAY_XYZ)) || (!Camera->IsSide()))
			{
				Vector3 ScreenX = Vector3(Camera->m_ViewWorld[0]);
				Vector3 ScreenY = Vector3(Camera->m_ViewWorld[1]);

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
				Redraw = RotateSelectedObjects(Delta, m_MouseSnapLeftover, true);
				m_MouseTotalDelta += Delta;
			}
			else
			{
				// 3D movement.
				Vector3 ScreenX = Vector3(Camera->m_ViewWorld[0]);
				Vector3 ScreenY = Vector3(Camera->m_ViewWorld[1]);
				Vector3 ScreenZ = Vector3(Camera->m_ViewWorld[2]);

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

				Redraw = RotateSelectedObjects(Delta, m_MouseSnapLeftover, true);
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

			m_ActiveView->GetCamera()->DoZoom(y - m_nDownY, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
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

			m_ActiveView->GetCamera()->DoPan(x - m_nDownX, y - m_nDownY, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
			m_ActiveView->GetCamera()->Update(m_ActiveModel->m_CurFrame);
			m_nDownX = x;
			m_nDownY = y;
			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;
		
		case LC_ACTION_ROTATE_VIEW:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			lcCamera* Camera = m_ActiveView->GetCamera();

			if (Camera->IsSide())
			{
				Vector3 Pos = Camera->m_WorldPosition;
				Vector3 Target = Camera->m_Children->m_WorldPosition;
				float Roll = Camera->m_Roll;

				Camera = new lcCamera();
				Camera->CreateCamera(LC_CAMERA_USER, true);

				Camera->Update(m_ActiveModel->m_CurFrame);
				Camera->SetUniqueName(m_ActiveModel->m_Cameras, "Camera");
				m_ActiveModel->AddCamera(Camera);

				Camera->SetPosition(1, false, Pos);
				Camera->m_Children->SetPosition(1, false, Pos);
				Camera->SetRoll(1, false, Roll);

				m_ActiveView->SetCamera(Camera);
				SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
				SystemUpdateCurrentCamera(NULL, Camera, m_ActiveModel->m_Cameras);
			}

			Camera->DoRotate(x - m_nDownX, y - m_nDownY, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
			Camera->Update(m_ActiveModel->m_CurFrame);

			m_nDownX = x;
			m_nDownY = y;
			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;

		case LC_ACTION_ORBIT:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			lcCamera* Camera = m_ActiveView->GetCamera();

			if (Camera->IsSide())
			{
				Vector3 Pos = Camera->m_WorldPosition;
				Vector3 Target = Camera->m_Children->m_WorldPosition;
				float Roll = Camera->m_Roll;

				Camera = new lcCamera();
				Camera->CreateCamera(LC_CAMERA_USER, true);

				Camera->Update(m_ActiveModel->m_CurFrame);
				Camera->SetUniqueName(m_ActiveModel->m_Cameras, "Camera");
				m_ActiveModel->AddCamera(Camera);

				Camera->SetPosition(1, false, Pos);
				Camera->m_Children->SetPosition(1, false, Pos);
				Camera->SetRoll(1, false, Roll);

				m_ActiveView->SetCamera(Camera);
				SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
				SystemUpdateCurrentCamera(NULL, Camera, m_ActiveModel->m_Cameras);
			}

			switch (m_OverlayMode)
			{
				case LC_OVERLAY_XYZ:
					Camera->DoOrbit(x - m_nDownX, y - m_nDownY, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
					break;

				case LC_OVERLAY_X:
					Camera->DoOrbit(x - m_nDownX, 0, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
					break;

				case LC_OVERLAY_Y:
					Camera->DoOrbit(0, y - m_nDownY, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
					break;

				case LC_OVERLAY_Z:
					Camera->DoRoll(x - m_nDownX, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
					break;
			}

			Camera->Update(m_ActiveModel->m_CurFrame);

			m_nDownX = x;
			m_nDownY = y;
			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;
		
		case LC_ACTION_ROLL:
		{
			if (m_nDownX == x)
				break;

			m_ActiveView->GetCamera()->DoRoll(x - m_nDownX, m_nMouse, m_ActiveModel->m_CurFrame, m_bAddKeys);
			m_ActiveView->GetCamera()->Update(m_ActiveModel->m_CurFrame);
			m_nDownX = x;
			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;
	}
}

// Check if the mouse is over a different area of the overlay and redraw it.
void Project::MouseUpdateOverlays(int x, int y)
{
	const float OverlayScale = m_ActiveView->m_OverlayScale;

	if (m_nCurAction == LC_ACTION_MOVE)
	{
		const float OverlayMoveArrowSize = 1.5f;

		Matrix44 ModelView, Projection;
		int Viewport[4];

		GetActiveViewportMatrices(ModelView, Projection, Viewport);

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
			lcObject* Focus = GetFocusObject();

			if ((Focus != NULL) && Focus->IsPieceObject())
			{
				const Matrix44& Mat = ((lcPieceObject*)Focus)->m_ModelWorld;

				for (int i = 1; i < 4; i++)
					Points[i] = Mul30(Points[i], Mat);
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
		GLdouble px, py, pz, rx, ry, rz;
		GLdouble ModelMatrix[16], ProjMatrix[16];
		GLint Viewport[4];

		m_ActiveView->LoadViewportProjection();
		glGetDoublev(GL_MODELVIEW_MATRIX, ModelMatrix);
		glGetDoublev(GL_PROJECTION_MATRIX, ProjMatrix);
		glGetIntegerv(GL_VIEWPORT, Viewport);

		// Unproject the mouse point against both the front and the back clipping planes.
		gluUnProject(x, y, 0, ModelMatrix, ProjMatrix, Viewport, &px, &py, &pz);
		gluUnProject(x, y, 1, ModelMatrix, ProjMatrix, Viewport, &rx, &ry, &rz);

		Vector3 SegStart((float)rx, (float)ry, (float)rz);
		Vector3 SegEnd((float)px, (float)py, (float)pz);
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

			float x1 = (float)px, y1 = (float)py, z1 = (float)pz;
			float x2 = (float)rx, y2 = (float)ry, z2 = (float)rz;
			float x3 = m_OverlayCenter[0], y3 = m_OverlayCenter[1], z3 = m_OverlayCenter[2];
			float r = OverlayRotateRadius * OverlayScale;

			// TODO: rewrite using vectors.
			float a = (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1) + (z2 - z1)*(z2 - z1);
			float b = 2 * ((x2 - x1)*(x1 - x3) + (y2 - y1)*(y1 - y3) + (z2 - z1)*(z1 - z3));
			float c = x3*x3 + y3*y3 + z3*z3 + x1*x1 + y1*y1 + z1*z1 - 2*(x3*x1 + y3*y1 + z3*z1) - r*r;
			float f = b * b - 4 * a * c;

			if (f >= 0.0f)
			{
				lcCamera* Camera = m_ActiveView->GetCamera();
				Vector3 ViewDir = Vector3(Camera->m_ViewWorld[2]);

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
						lcObject* Focus = GetFocusObject();

						if ((Focus != NULL) && Focus->IsPieceObject())
						{
							Matrix44 WorldModel = RotTranInverse(((lcPieceObject*)Focus)->m_ModelWorld);
							Dist = Mul30(Dist, WorldModel);
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
	else if (m_nCurAction == LC_ACTION_ORBIT)
	{
		int vw = m_ActiveView->GetWidth();
		int vh = m_ActiveView->GetHeight();

		int cx = vw / 2;
		int cy = vh / 2;

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
	else if (m_nCurAction == LC_ACTION_ORBIT)
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
	if (m_OverlayActive)
	{
		for (int i = 0; i < m_ViewList.GetSize(); i++)
			m_ViewList[i]->UpdateOverlayScale();
	}
}
