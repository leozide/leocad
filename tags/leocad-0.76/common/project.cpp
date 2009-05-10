// Everything that is a part of a LeoCAD project goes here.
//

#include "lc_global.h"
#include "project.h"

#include "lc_application.h"
#include "lc_model.h"
#include "lc_mesh.h"
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
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "terrain.h"
#include "image.h"
#include "system.h"
#include "minifig.h"
#include "curve.h"
#include "mainwnd.h"
#include "view.h"
#include "library.h"
#include "texfont.h"
#include "algebra.h"
#include "debug.h"
#include "console.h"

// TODO: temporary function, rewrite.
void SystemUpdateFocus(void* p)
{
	lcPostMessage(LC_MSG_FOCUS_OBJECT_CHANGED, p);
}

/////////////////////////////////////////////////////////////////////////////
// Project construction/destruction

Project::Project()
{
	int i;

	m_ActiveView = NULL;
	m_ActiveModel = NULL;

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
	strcpy(m_strModelsPath, Sys_ProfileLoadString ("Default", "Projects", ""));

	for (i = 0; i < 10; i++)
		m_pClipboard[i] = NULL;

	m_pScreenFont = new TexFont();

	VRMLScale = 0.01f; // centimeter to meter
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
	SystemUpdateColorList(g_App->m_SelectedColor);
	SystemUpdateAnimation(m_Animation, m_bAddKeys);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_BACKGROUND) != 0, (m_nDetail & LC_DET_FAST) != 0);
	SystemUpdateSnap(m_nSnap);
	SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
	SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
	SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
	UpdateSelection();
	SystemUpdateTime(m_Animation, m_ActiveModel->m_CurFrame, m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
	SystemUpdateModelMenu(m_ModelList, m_ActiveModel);

	for (int i = 0; i < m_ViewList.GetSize(); i++)
	{
		m_ViewList[i]->MakeCurrent();
		RenderInitialize();
	}
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
		if (strcmp(ext, "mpd") == 0)
			*ptr = 0;
	}

	strcat(title, " - LeoCAD");

	SystemSetWindowCaption(title);
}

void Project::DeleteContents(bool bUndo)
{
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
		m_pTerrain->LoadDefaults((m_nDetail & LC_DET_LINEAR) != 0);
	}

	for (int i = 0; i < m_ViewList.GetSize(); i++)
		m_ViewList[i]->SetCamera(NULL);

	// Remove all submodels.
	for (int i = 0; i < m_ModelList.GetSize(); i++)
		m_ModelList[i]->DeleteContents();

	for (int i = 0; i < m_ModelList.GetSize(); i++)
		delete m_ModelList[i];
	m_ModelList.RemoveAll();

	m_ActiveModel = NULL;
	SystemUpdateModelMenu(m_ModelList, m_ActiveModel);

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
	g_App->m_SelectedColor = lcConvertLDrawColor(4);
	SystemUpdateColorList(g_App->m_SelectedColor);
	m_Animation = false;
	m_bAddKeys = false;
	SystemUpdateAnimation(m_Animation, m_bAddKeys);
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
	m_nScene = Sys_ProfileLoadInt ("Default", "Scene", 0);
	m_nSaveTimer = 0;
	strcpy(m_strHeader, Sys_ProfileLoadString ("Default", "Header", ""));
	strcpy(m_strFooter, Sys_ProfileLoadString ("Default", "Footer", "Page &P"));
	strcpy(m_strBackground, Sys_ProfileLoadString ("Default", "BMP", ""));
	m_pTerrain->LoadDefaults((m_nDetail & LC_DET_LINEAR) != 0);
	m_OverlayActive = false;
	m_PlayingAnimation = false;

	for (i = 0; i < m_ViewList.GetSize (); i++)
	{
		m_ViewList[i]->MakeCurrent();
		RenderInitialize();
	}

	if (cameras)
	{
		lcModel* Model = new lcModel();
		Model->m_Name = "Main";
		m_ModelList.Add(Model);
		SetActiveModel(Model);

		m_ActiveModel->ResetCameras();

		for (int i = 0; i < m_ViewList.GetSize(); i++)
			m_ViewList[i]->UpdateCamera();

		SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
		if (m_ActiveView)
			SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
	}

	SystemPieceComboAdd(NULL);
}

/////////////////////////////////////////////////////////////////////////////
// Standard file menu commands

static u32 LC_FILE_VERSION = 0x0076;

// Read a .lcd file
bool Project::FileLoad(File* file, bool bUndo, bool bMerge)
{
	int i, count;
	char fid[32];
	float fv = 0.4f;
	u32 Version = 0;
	u16 sh;
	u8 ch;

	file->Seek(0, SEEK_SET);
	file->Read(fid, 32);
	sscanf(&fid[7], "%f", &fv);

	// Fix floating point reading on computers with different decimal separators.
	if (fv == 0.0f)
	{
		lconv *loc = localeconv();
		fid[8] = loc->decimal_point[0];
		sscanf(&fid[7], "%f", &fv);

		if (fv == 0.0f)
			return false;
	}

	if (fv > 0.4f)
		file->ReadFloat(&fv, 1);

	if (fv > 1.4f)
		file->ReadLong(&Version, 1);

	if (Version > LC_FILE_VERSION)
		return false;

	if (Version < 0x0076)
	{
		u32 rgb;
		file->ReadLong(&rgb, 1);

		if (!bMerge)
		{
			m_fBackground[0] = (float)((rgb & 0x0000ff) >> 0) / 255;
			m_fBackground[1] = (float)((rgb & 0x00ff00) >> 8) / 255;
			m_fBackground[2] = (float)((rgb & 0xff0000) >> 16) / 255;
		}

		if (fv < 0.6f)
		{
			// Old view format, ignore it.
			m_ActiveModel->ResetCameras();

			for (int i = 0; i < m_ViewList.GetSize(); i++)
				m_ViewList[i]->UpdateCamera();

			file->Seek(48, SEEK_SET); // eye, target
		}
	}

	if (bMerge)
		file->Seek(16, SEEK_CUR);
	else
	{
		file->ReadLong(&i, 1); m_nAngleSnap = i;
		file->ReadLong(&m_nSnap, 1);
		file->ReadFloat(&m_fLineWidth, 1);
		file->ReadLong(&m_nDetail, 1);
	}

	if (Version < 0x0076)
		file->Seek(16, SEEK_SET); // m_nCurGroup, m_nCurColor, m_nCurAction, m_CurFrame

	if (fv > 0.8f)
		file->ReadLong(&m_nScene, 1);

	int NumModels = 1;
	if (Version > 0x0075)
		file->ReadLong(&NumModels, 1);

	for (int ModelIndex = 0; ModelIndex < NumModels; i++)
	{
		lcModel* Model = new lcModel();
		m_ModelList.Add(Model);
		m_ActiveModel = Model;

		file->ReadLong(&count, 1);
		SystemStartProgressBar(0, count, 1, "Loading model...");

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
						for (lcPiece* p = m_ActiveModel->m_Pieces; p; p = (lcPiece*)p->m_Next)
							if (p->m_Name == Piece->m_Name)
							{
								Piece->SetUniqueName(m_ActiveModel->m_Pieces, pInfo->m_strDescription);
								break;
							}
					}

					if (Piece->m_Name.GetLength() == 0)
						Piece->SetUniqueName(m_ActiveModel->m_Pieces, pInfo->m_strDescription);

					m_ActiveModel->AddPiece(Piece);
					if (!bUndo)
						SystemPieceComboAdd(pInfo->m_strDescription);
				}
				else 
					delete Piece; // TODO: create a dummy piece instead.
			}
			else
			{
				char name[9];
				float pos[3], rot[3], param[4];
				unsigned char color, step, group;
			
				file->ReadFloat(pos, 3);
				file->ReadFloat(rot, 3);
				file->ReadByte(&color, 1);
				file->Read(name, sizeof(name));
				file->ReadByte(&step, 1);
				file->ReadByte(&group, 1);

				// Convert from the old colors to 0.75 and then to LDraw.
				const int conv[20] = { 0,2,4,9,7,6,22,8,10,11,14,16,18,9,21,20,22,8,10,11 };
				color = conv[color];
				const int ColorTable[28] = { 4,25,2,10,1,9,14,15,8,0,6,13,13,334,36,44,34,42,33,41,46,47,7,382,6,13,11,383 };
				color = lcConvertLDrawColor(ColorTable[color]);

				PieceInfo* Info = lcGetPiecesLibrary()->FindPieceInfo(name);
				if (Info != NULL)
				{
					lcPiece* Piece = new lcPiece(Info);
					Matrix mat;

					Piece->Initialize(pos[0], pos[1], pos[2], step, color);
					Piece->SetUniqueName(m_ActiveModel->m_Pieces, Info->m_strDescription);
					m_ActiveModel->AddPiece(Piece);
					mat.CreateOld(0,0,0, rot[0],rot[1],rot[2]);
					mat.ToAxisAngle(param);
					Piece->ChangeKey(1, false, param, LC_PK_ROTATION);
//					Piece->SetGroup((Group*)group);
					SystemPieceComboAdd(Info->m_strDescription);
				}
			}
			SystemStepProgressBar();
		}
		SystemEndProgressBar();

		if (!bMerge)
		{
			if (fv >= 0.4f)
			{
				file->Read(&ch, 1);
				if (ch == 0xFF) file->ReadShort(&sh, 1); else sh = ch;
				if (sh > 100)
					file->Seek(sh, SEEK_CUR);
				else
					file->Read(m_strAuthor, sh);

				file->Read(&ch, 1);
				if (ch == 0xFF) file->ReadShort(&sh, 1); else sh = ch;
				if (sh > 100)
					file->Seek(sh, SEEK_CUR);
				else
					file->Read(m_strDescription, sh);

				file->Read(&ch, 1);
				if (ch == 0xFF && fv < 1.3f) file->ReadShort(&sh, 1); else sh = ch;
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
				if (ch == 0xFF) file->ReadShort(&sh, 1); else sh = ch;
				file->Seek(sh, SEEK_CUR);

				file->Read(&ch, 1);
				if (ch == 0xFF) file->ReadShort(&sh, 1); else sh = ch;
				file->Seek(sh, SEEK_CUR);

				file->Read(&ch, 1);
				if (ch == 0xFF && fv < 1.3f) file->ReadShort(&sh, 1); else sh = ch;
				file->Seek(sh, SEEK_CUR);
			}
		}

		if (fv >= 0.5f)
		{
			file->ReadLong(&count, 1);

			lcGroup* pGroup;
			lcGroup* pLastGroup = NULL;
			for (pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
				pLastGroup = pGroup;

			pGroup = pLastGroup;
			for (i = 0; i < count; i++)
			{
				if (pGroup)
				{
					pGroup->m_Next = new lcGroup();
					pGroup = pGroup->m_Next;
				}
				else
					m_ActiveModel->m_Groups = pGroup = new lcGroup();
			}
			pLastGroup = pLastGroup ? pLastGroup->m_Next : m_ActiveModel->m_Groups;

			for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_Next)
			{
				if (fv < 1.0f)
				{
					file->Read(pGroup->m_strName, 65);
					file->Read(&ch, 1);
					pGroup->m_fCenter[0] = 0;
					pGroup->m_fCenter[1] = 0;
					pGroup->m_fCenter[2] = 0;
					pGroup->m_Group = (lcGroup*)-1;
				}
				else
					pGroup->FileLoad(file);
			}

			for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_Next)
			{
				i = (int)pGroup->m_Group;
				pGroup->m_Group = NULL;

				if (i > 0xFFFF || i == -1)
					continue;

				for (lcGroup* g2 = pLastGroup; g2; g2 = g2->m_Next)
				{
					if (i == 0)
					{
						pGroup->m_Group = g2;
						break;
					}

					i--;
				}
			}

			lcPiece* Piece;
			for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
			{
				i = (int)Piece->GetGroup();
				Piece->SetGroup(NULL);

				if (i > 0xFFFF || i == -1)
					continue;

				for (pGroup = pLastGroup; pGroup; pGroup = pGroup->m_Next)
				{
					if (i == 0)
					{
						Piece->SetGroup(pGroup);
						break;
					}

					i--;
				}
			}

			RemoveEmptyGroups();
		}

		if (fv >= 0.6f && Version < 0x0076)
		{
			if (fv < 1.0f)
				file->Seek(4, SEEK_CUR); // m_nViewportMode
			else
				file->Seek(2, SEEK_CUR); // m_nViewportMode, m_nActiveViewport 
		}

		if (fv >= 0.6f)
		{
			file->ReadLong(&count, 1);
			lcCamera* pCam = NULL;
			for (i = 0; i < count; i++)
			{
				pCam = new lcCamera(i);
				m_ActiveModel->AddCamera(pCam);
				pCam->FileLoad(*file);
			}

			if (count < 7)
				m_ActiveModel->ResetCameras();
		}

		if (fv >= 0.7f)
		{
			if (Version < 0x0076)
				file->Seek(16, SEEK_SET); // view camera index.

			u32 rgb;
			file->ReadLong(&rgb, 1);

			m_fFogColor[0] = (float)((rgb & 0x0000ff) >> 0) / 255;
			m_fFogColor[1] = (float)((rgb & 0x00ff00) >> 8) / 255;
			m_fFogColor[2] = (float)((rgb & 0xff0000) >> 16) / 255;

			if (fv < 1.0f)
			{
				file->ReadLong(&rgb, 1);
				m_fFogDensity = (float)rgb/100;
			}
			else
				file->ReadFloat(&m_fFogDensity, 1);

			if (fv < 1.3f)
			{
				file->ReadByte(&ch, 1);
				if (ch == 0xFF)
					file->ReadShort(&sh, 1);
				sh = ch;
			}
			else
				file->ReadShort(&sh, 1);

			if (sh < LC_MAXPATH)
				file->Read(m_strBackground, sh); // TODO: make background be part of the model.
			else
				file->Seek(sh, SEEK_CUR);
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
			u32 rgb;
			file->ReadLong(&rgb, 1);

			m_fAmbient[0] = (float)((rgb & 0x0000ff) >> 0) / 255;
			m_fAmbient[1] = (float)((rgb & 0x00ff00) >> 8) / 255;
			m_fAmbient[2] = (float)((rgb & 0xff0000) >> 16) / 255;

			if (fv < 1.3f)
			{
				file->ReadLong(&i, 1); m_Animation = (i != 0);
				file->ReadLong(&i, 1); m_bAddKeys = (i != 0);
				file->ReadByte(&m_nFPS, 1);
				file->ReadLong(&i, 1); // m_nCurFrame = i;
				file->ReadShort(&sh, 1); m_ActiveModel->m_TotalFrames = sh;
				file->ReadLong(&i, 1); // m_nGridSize = i;
				file->ReadLong(&i, 1); // m_nMoveSnap = i;
			}
			else
			{
				file->ReadByte(&ch, 1); m_Animation = (ch != 0);
				file->ReadByte(&ch, 1); m_bAddKeys = (ch != 0);
				file->ReadByte(&m_nFPS, 1);
				file->ReadShort(&sh, 1); // m_nCurFrame
				file->ReadShort(&sh, 1); m_ActiveModel->m_TotalFrames = sh;
				file->ReadShort(&sh, 1); // m_nGridSize
				file->ReadShort(&sh, 1);
				if (fv >= 1.4f)
					m_nMoveSnap = sh;
			}
		}

		if (fv > 1.0f)
		{
			u32 rgb;

			file->ReadLong(&rgb, 1);
			m_fGradient1[0] = (float)((rgb & 0x0000ff) >> 0) / 255;
			m_fGradient1[1] = (float)((rgb & 0x00ff00) >> 8) / 255;
			m_fGradient1[2] = (float)((rgb & 0xff0000) >> 16) / 255;
			file->ReadLong(&rgb, 1);
			m_fGradient2[0] = (float)((rgb & 0x0000ff) >> 0) / 255;
			m_fGradient2[1] = (float)((rgb & 0x00ff00) >> 8) / 255;
			m_fGradient2[2] = (float)((rgb & 0xff0000) >> 16) / 255;
			
			if (fv > 1.1f)
				m_pTerrain->FileLoad(file); // TODO: move terrain to model.
			else
			{
				file->Seek(4, SEEK_CUR);
				file->Read(&ch, 1);
				file->Seek(ch, SEEK_CUR);
			}
		}
	}

	int ActiveModelIndex = 0;

	if (Version > 0x0075)
	{
		file->ReadLong(&ActiveModelIndex, 1);
		UpdateAllModelMeshes();
	}

	m_ModelList[ActiveModelIndex]->SetActive(true);

	// TODO: update piece model mesh pointers.
	// TODO: load model references.

	if (!bMerge)
	{
		// TODO: fix file merge
	}

	for (i = 0; i < m_ViewList.GetSize(); i++)
	{
		m_ViewList[i]->MakeCurrent();
		RenderInitialize();
	}
	CalculateStep();
	if (!bUndo)
		SelectAndFocusNone(false);
	if (!bMerge)
		SystemUpdateFocus(NULL);
	SystemUpdateColorList(g_App->m_SelectedColor);
	SystemUpdateAnimation(m_Animation, m_bAddKeys);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_BACKGROUND) != 0, (m_nDetail & LC_DET_FAST) != 0);
	SystemUpdateSnap(m_nSnap);
	SystemUpdateSnap(m_nMoveSnap, m_nAngleSnap);
	SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
	SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);
	UpdateSelection();
	SystemUpdateTime(m_Animation, m_ActiveModel->m_CurFrame, m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
	UpdateAllViews();

	return true;
}

void Project::FileSave(File* file, bool bUndo)
{
	float ver_flt = 1.5f; // LeoCAD 0.76
	u32 rgb;
	u16 sh;
	u8 ch;
	int i, j;

	file->Seek(0, SEEK_SET);
	file->Write(LC_STR_VERSION, 32);
	file->WriteFloat(&ver_flt, 1);
	file->WriteLong(&LC_FILE_VERSION, 1);

	rgb = FLOATRGB(m_fBackground);
	file->WriteLong(&rgb, 1);

	i = m_nAngleSnap; file->WriteLong (&i, 1);
	file->WriteLong(&m_nSnap, 1);
	file->WriteFloat(&m_fLineWidth, 1);
	file->WriteLong(&m_nDetail, 1);
	//i = m_nCurGroup;
	file->WriteLong(&i, 1);
	i = 0; file->WriteLong(&i, 1); //m_nCurColor
	i = m_nCurAction; file->WriteLong (&i, 1);
	file->WriteLong(&m_ActiveModel->m_CurFrame, 1);
	file->WriteLong(&m_nScene, 1);

	lcPiece* Piece;
	for (i = 0, Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		i++;
	file->WriteLong(&i, 1);

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		Piece->FileSave(*file, m_ActiveModel->m_Groups);

	ch = strlen(m_strAuthor);
	file->Write(&ch, 1);
	file->Write(m_strAuthor, ch);
	ch = strlen(m_strDescription);
	file->Write(&ch, 1);
	file->Write(m_strDescription, ch);
	ch = strlen(m_strComments);
	file->Write(&ch, 1);
	file->Write(m_strComments, ch);

	lcGroup* pGroup;
	for (i = 0, pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
		i++;
	file->WriteLong (&i, 1);

	for (pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
		pGroup->FileSave(file, m_ActiveModel->m_Groups);

	ch = 0;
	file->WriteByte(&ch, 1); // m_nViewportMode
	file->WriteByte(&ch, 1); // m_nActiveViewport

	lcCamera* Camera;
	for (i = 0, Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
		i++;
	file->WriteLong (&i, 1);

	for (Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
		Camera->FileSave(*file);

	for (j = 0; j < 4; j++)
	{
		/*
		for (i = 0, Camera = m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
			if (Camera == m_pViewCameras[j])
				break;
			else
				i++;
		*/
		i = 0;

		file->WriteLong(&i, 1);
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
	ch = m_Animation;
	file->Write(&ch, 1);
	ch = m_bAddKeys;
	file->WriteByte (&ch, 1);
	file->WriteByte (&m_nFPS, 1);
	sh = 1; file->WriteShort (&sh, 1); // m_nCurFrame
	sh = 0xffff; file->WriteShort (&sh, 1); // m_nTotalFrames
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

			CreateImages(image, 120, 100, m_ActiveModel->m_CurFrame, m_ActiveModel->m_CurFrame, false);
			image[0].FileSave (*file, &opts);
			delete []image;
		}

		file->WriteLong (&pos, 1);
		m_nSaveTimer = 0;
	}
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
		if (!DoSave(m_strPathName))
			return false;
	}
	return true;
}

// Save the document data to a file
// PathName = path name where to save document file
// if PathName is NULL then the user will be prompted (SaveAs)
// note: PathName can be different than 'm_strPathName'
bool Project::DoSave(char* PathName)
{
	FileDisk file;
	char NewName[LC_MAXPATH];
	memset(NewName, 0, sizeof(NewName));
	if (PathName)
		strcpy(NewName, PathName);

	int Len = strlen(NewName);
	if (Len > 4)
	{
		char* ext = &NewName[Len - 3];

		if (!_stricmp(ext, "dat") || !_stricmp(ext, "ldr") || !_stricmp(ext, "mpd"))
		{
			*ext = 0;
			strcat(NewName, "lcd");
		}
	}

	if (Len == 0)
	{
		strcpy(NewName, m_strPathName);
		if (strlen(NewName) == 0)
		{
			strcpy(NewName, m_strTitle);

			// check for dubious filename
			char* Bad = strpbrk(NewName, " #%;/\\");
			if (Bad)
				*Bad = 0;

			strcat(NewName, ".lcd");
		}

		if (!SystemDoDialog(LC_DLG_FILE_SAVE_PROJECT, &NewName))
			return false; // don't even attempt to save
	}

	if (!file.Open(NewName, "wb"))
	{
		char Message[LC_MAXPATH+1024];
		sprintf(Message, "Failed to open file %s for writing.", NewName);
		main_window->MessageBox(Message, "LeoCAD", LC_MB_OK | LC_MB_ICONERROR);

		// be sure to delete the file
		if (PathName == NULL)
			remove(NewName);

		return false;
	}

	bool SaveLDR = false;
	bool SaveMPD = false;

	Len = strlen(NewName);
	if (Len > 4)
	{
		const char* ext = &NewName[Len - 3];

		if (!_stricmp(ext, "dat") || !_stricmp(ext, "ldr"))
			SaveLDR = true;
		else if (!_stricmp(ext, "mpd"))
			SaveMPD = true;
	}

	if (SaveLDR || SaveMPD)
	{
		const char* OldLocale = setlocale(LC_NUMERIC, "C");

		m_ActiveModel->ExportLDraw(file, SaveMPD, IdentityMatrix44(), LC_COLOR_DEFAULT);

		if (SaveMPD)
		{
			for (int ModelIndex = 0; ModelIndex < m_ModelList.GetSize(); ModelIndex++)
			{
				if (m_ModelList[ModelIndex] == m_ActiveModel)
					continue;

				m_ModelList[ModelIndex]->ExportLDraw(file, SaveMPD, IdentityMatrix44(), LC_COLOR_DEFAULT);
			}
		}

		setlocale(LC_NUMERIC, OldLocale);
	}
	else
		FileSave(&file, false);     // save me

	file.Close();

	SetModifiedFlag(false);     // back to unmodified

	// reset the title and change the document name
	SetPathName(NewName, true);

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

	lcPostMessage(LC_MSG_FOCUS_OBJECT_CHANGED, NULL);

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

	if (!file.Open(PathName, "rb") || !file.GetLength())
	{
		char Message[LC_MAXPATH + 64];

		sprintf(Message, "Failed to open %s for reading.", PathName);
		main_window->MessageBox(Message, "LeoCAD", LC_MB_OK | LC_MB_ICONERROR);

		return false;
	}

	bool LoadLDR = false;
	bool LoadMPD = false;

	int Len = strlen(PathName);
	if (Len > 4)
	{
		const char* ext = &PathName[Len - 3];

		if (!_stricmp(ext, "dat") || !_stricmp(ext, "ldr"))
			LoadLDR = true;
		else if (!_stricmp(ext, "mpd"))
			LoadMPD = true;
	}

	// Delete the current project.
	DeleteContents(false);
	LoadDefaults(false);
	SetModifiedFlag(true);  // dirty during loading

	SystemDoWaitCursor(1);

	if (LoadMPD || LoadLDR)
	{
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

		if (LoadMPD)
		{
			lcPtrArray<File> FileArray;
			File* CurrentFile = NULL;
			char Buf[1024];
			bool Skip = true;

			// Split MPD into separate files.
			while (file.ReadLine(Buf, 1024))
			{
				char* ptr = Buf;
				String LineType = GetToken(ptr);

				if (LineType == "0")
				{
					String Token = GetToken(ptr);

					if (!Token.CompareNoCase("FILE"))
					{
						while (*ptr && *ptr <= 32)
							ptr++;
						char* Name = ptr;
						while (*ptr > 32)
							ptr++;
						*ptr = 0;

						CurrentFile = new FileMem();
						CurrentFile->SetFileName(Name);
						FileArray.Add(CurrentFile);
						Skip = false;
						continue;
					}
					else if (!Token.CompareNoCase("NOFILE"))
					{
						CurrentFile = NULL;
						Skip = true;
						continue;
					}
				}

				if (!Skip)
					CurrentFile->Write(Buf, strlen(Buf));
			}

			// Read MPD files.
			for (int FileIndex = 0; FileIndex < FileArray.GetSize(); FileIndex++)
			{
				lcModel* Model = new lcModel();
				Model->m_Name = FileArray[FileIndex]->GetFileName();
				strncpy(Model->m_PieceInfo->m_strDescription, Model->m_Name, sizeof(Model->m_PieceInfo->m_strDescription)-1);
				Model->m_PieceInfo->m_strDescription[sizeof(Model->m_PieceInfo->m_strDescription)-1] = 0;
				FileArray[FileIndex]->Seek(0, SEEK_SET);
				m_ModelList.Add(Model);

				Model->ResetCameras();
			}

			for (int FileIndex = 0; FileIndex < FileArray.GetSize(); FileIndex++)
			{
				m_ModelList[FileIndex]->ImportLDraw(*FileArray[FileIndex], LC_COLOR_DEFAULT, IdentityMatrix44(), FilePath);
				delete FileArray[FileIndex];
			}

			UpdateAllModelMeshes();

			for (int ModelIndex = 0; ModelIndex < m_ModelList.GetSize(); ModelIndex++)
			{
				lcModel* Model = m_ModelList[ModelIndex];
				Model->ZoomExtents(m_ActiveView, Model->GetCamera(LC_CAMERA_MAIN), false);
			}

			if (FileArray.GetSize() == 0)
			{
				LoadMPD = false;
				LoadLDR = true;
				file.Seek(0, SEEK_SET);
				console.PrintWarning("No files found inside the MPD file, trying to load it as a .DAT file instead.\n");
			}
		}

		if (LoadLDR)
		{
			lcModel* Model = new lcModel();
			Model->m_Name = "Main";
			m_ModelList.Add(Model);

			Model->ResetCameras();
			Model->ImportLDraw(file, LC_COLOR_DEFAULT, IdentityMatrix44(), FilePath);
			Model->ZoomExtents(m_ActiveView, Model->GetCamera(LC_CAMERA_MAIN), false);
		}

		SetActiveModel(m_ModelList[0]);

		for (int i = 0; i < m_ViewList.GetSize(); i++)
			m_ViewList[i]->UpdateCamera();

		SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
		if (m_ActiveView)
			SystemUpdateCurrentCamera(NULL, m_ActiveView->GetCamera(), m_ActiveModel->m_Cameras);

		SystemUpdateTime(false, m_ActiveModel->m_CurFrame, LC_OBJECT_TIME_MAX);
		SystemUpdateFocus(NULL);
		UpdateSelection();
		CalculateStep();
		UpdateAllViews();

//		console.PrintMisc("%d objects imported.\n", Pieces);
	}
	else
	{
		// Load a LeoCAD file.
		if (!FileLoad(&file, false, false))
		{
			char Message[LC_MAXPATH + 64];

			sprintf(Message, "Failed to load %s.", PathName);
			main_window->MessageBox(Message, "LeoCAD", LC_MB_OK | LC_MB_ICONERROR);

			DeleteContents(false);   // remove failed contents
			SystemDoWaitCursor(-1);
			return false;
		}
	}

	file.Close();
	SystemDoWaitCursor(-1);

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
	m_ViewList.Add(pView);

	pView->MakeCurrent();
	RenderInitialize();

	if (m_ViewList.GetSize() == 1)
		lcCreateDefaultMeshes();
}

void Project::RemoveView(View* pView)
{
	if (m_ViewList.GetSize() == 1)
		lcDestroyDefaultMeshes();

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
#if LC_PROFILE
	memset(&g_RenderStats, 0, sizeof(g_RenderStats));

	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	u64 Start = li.QuadPart;
#endif

	// Setup the viewport.
	glViewport(0, 0, view->GetWidth(), view->GetHeight());

	// Render the background.
	RenderBackground(view);

	// Setup the projection and camera matrices.
	Matrix44 Projection = view->GetProjectionMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(Projection);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(view->GetCamera()->m_WorldView);

	// Render 3D objects.
	if (AllowFast && (m_nDetail & LC_DET_FAST) && (m_nTracking != LC_TRACK_NONE))
		RenderSceneBoxes(view);
	else
		RenderScene(view, Interface);

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

#ifdef LC_PROFILE
	glFlush();
	glFinish();
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

			float Verts[4][2];
			float Colors[4][4];

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2, GL_FLOAT, 0, Verts);
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, 0, Colors);

			Colors[0][0] = m_fGradient1[0]; Colors[0][1] = m_fGradient1[1]; Colors[0][2] = m_fGradient1[2]; Colors[0][3] = 1.0f;
			Verts[0][0] = (float)view->GetWidth(); Verts[0][1] = (float)view->GetHeight();
			Colors[1][0] = m_fGradient1[0]; Colors[1][1] = m_fGradient1[1]; Colors[1][2] = m_fGradient1[2]; Colors[1][3] = 1.0f;
			Verts[1][0] = 0; Verts[1][1] = (float)view->GetHeight();
			Colors[2][0] = m_fGradient2[0]; Colors[2][1] = m_fGradient2[1]; Colors[2][2] = m_fGradient2[2]; Colors[2][3] = 1.0f;
			Verts[2][0] = 0; Verts[2][1] = 0;
			Colors[3][0] = m_fGradient2[0]; Colors[3][1] = m_fGradient2[1]; Colors[3][2] = m_fGradient2[2]; Colors[3][3] = 1.0f;
			Verts[3][0] = (float)view->GetWidth(); Verts[3][1] = 0;

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
				tw = w/m_pBackground->m_nWidth;
				th = h/m_pBackground->m_nHeight;
			}

			Coords[0][0] = 0; Coords[0][1] = 0;
			Verts[0][0] = 0; Verts[0][1] = (float)view->GetHeight();
			Coords[1][0] = tw; Coords[1][1] = 0;
			Verts[1][0] = (float)view->GetWidth(); Verts[1][1] = (float)view->GetHeight();
			Coords[2][0] = tw; Coords[2][1] = th; 
			Verts[2][0] = (float)view->GetWidth(); Verts[2][1] = 0;
			Coords[3][0] = 0; Coords[3][1] = th;
			Verts[3][0] = 0; Verts[3][1] = 0;

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

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

struct lcRenderSection
{
//	Matrix44 ModelWorld;
	lcPiece* Owner;
	lcMesh* Mesh;
	lcMeshSection* Section;
	float Distance;
	int Color;
};

void Project::RenderScene(View* view, bool Interface)
{
	glDepthMask(GL_TRUE);
	glLineWidth(m_fLineWidth);

	// Draw the base grid.
	if (Interface && (m_nSnap & LC_DRAW_GRID))
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

			ymax = (Length(frontvec))*sinf(DTOR*Camera->m_FOV/2);
			ymin = -ymax;
			xmin = ymin * Aspect;
			xmax = ymax * Aspect;

			// Calculate camera offset.
			Matrix44 ModelView = Camera->m_WorldView;
			Vector3 offset = Mul30(Camera->m_Position, ModelView);

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
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
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
			Matrix44 ModelView = Camera->m_WorldView;
			Matrix44 Projection = view->GetProjectionMatrix();

			// Unproject edge center points to world space.
			Vector3 Points[10] =
			{
				Vector3(0, (float)view->m_Viewport[3] / 2, 0),
				Vector3(0, (float)view->m_Viewport[3] / 2, 1),
				Vector3((float)view->m_Viewport[2] / 2, 0, 0),
				Vector3((float)view->m_Viewport[2] / 2, 0, 1),
				Vector3((float)view->m_Viewport[2], (float)view->m_Viewport[3] / 2, 0),
				Vector3((float)view->m_Viewport[2], (float)view->m_Viewport[3] / 2, 1),
				Vector3((float)view->m_Viewport[2] / 2, (float)view->m_Viewport[3], 0),
				Vector3((float)view->m_Viewport[2] / 2, (float)view->m_Viewport[3], 1),
				Vector3((float)view->m_Viewport[2] / 2, (float)view->m_Viewport[3] / 2, 0),
				Vector3((float)view->m_Viewport[2] / 2, (float)view->m_Viewport[3] / 2, 1),
			};

			UnprojectPoints(Points, 10, ModelView, Projection, view->m_Viewport);

			// Intersect lines with base plane.
			Vector3 Intersections[5];
			int i;

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
	lcObjArray<lcRenderSection> OpaqueSections(1024);
	lcObjArray<lcRenderSection> TranslucentSections(1024);

	const Matrix44& WorldView = view->GetCamera()->m_WorldView;

	for (lcPiece * Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
	{
		if (!Piece->IsVisible(m_ActiveModel->m_CurFrame))
			continue;

		lcMesh* Mesh = Piece->m_PieceInfo->m_Mesh;

		for (int i = 0; i < Mesh->m_SectionCount; i++)
		{
			lcRenderSection RenderSection;
			lcMeshSection* Section = &Mesh->m_Sections[i];

			RenderSection.Owner = Piece;
			RenderSection.Mesh = Mesh;
			RenderSection.Section = Section;

			if (Section->ColorIndex == LC_COLOR_DEFAULT)
				RenderSection.Color = Piece->m_Color;
			else
				RenderSection.Color = Section->ColorIndex;

			if (RenderSection.Section->PrimitiveType == GL_LINES)
			{
				// FIXME: LC_DET_BRICKEDGES
//				if ((m_nDetail & LC_DET_BRICKEDGES) == 0)
//					continue;

				if (Piece->IsFocused())
					RenderSection.Color = LC_COLOR_FOCUS;
				else if (Piece->IsSelected())
					RenderSection.Color = LC_COLOR_SELECTION;
			}

			if (LC_COLOR_TRANSLUCENT(RenderSection.Color))
			{
				// Sort by distance to the camera.
				Vector3 Pos = Mul31(Section->Box.GetCenter(), Piece->m_ModelWorld);
				Pos = Mul31(Pos, WorldView);
				RenderSection.Distance = Pos[2];

				TranslucentSections.Add(RenderSection);
			}
			else
			{
				// Pieces are already sorted by vertex buffer, so no need to sort again here.
				// TODO: not true anymore, need to sort by vertex buffer here.
				OpaqueSections.Add(RenderSection);
			}
		}
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	lcVertexBuffer* LastVertexBuffer = NULL;
	lcIndexBuffer* LastIndexBuffer = NULL;
	lcPiece* LastPiece = NULL;
	lcMesh* LastMesh;

	glPushMatrix();

	// Render opaque sections.
	for (int i = 0; i < OpaqueSections.GetSize(); i++)
	{
		lcRenderSection& RenderSection = OpaqueSections[i];
		lcMesh* Mesh = RenderSection.Mesh;

		if (Mesh->m_VertexBuffer != LastVertexBuffer)
		{
			LastVertexBuffer = Mesh->m_VertexBuffer;
			LastVertexBuffer->BindBuffer();
		}

		if (Mesh->m_IndexBuffer != LastIndexBuffer)
		{
			LastIndexBuffer = Mesh->m_IndexBuffer;
			LastIndexBuffer->BindBuffer();
		}

		if (LastPiece != RenderSection.Owner || LastMesh != Mesh)
		{
			LastPiece = RenderSection.Owner;
			LastMesh = Mesh;
			glPopMatrix();
			glPushMatrix();
			glMultMatrixf(LastPiece->m_ModelWorld);

			if (LastPiece->IsSelected())
				glLineWidth(2.0f);
			else
				glLineWidth(1.0f);
		}

		lcSetColor(RenderSection.Color);

		lcMeshSection* Section = RenderSection.Section;

#if LC_PROFILE
		if (Section->PrimitiveType == GL_QUADS)
			g_RenderStats.QuadCount += Section->IndexCount / 4;
		else if (Section->PrimitiveType == GL_TRIANGLES)
			g_RenderStats.TriCount += Section->IndexCount / 3;
		else if (Section->PrimitiveType == GL_LINES)
			g_RenderStats.LineCount += Section->IndexCount / 2;
#endif

		glDrawElements(Section->PrimitiveType, Section->IndexCount, Mesh->m_IndexType, (char*)LastIndexBuffer->GetDrawElementsOffset() + Section->IndexOffset);
	}

	// Render translucent sections.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	for (int j = 0; j < TranslucentSections.GetSize(); j++)
	{
		lcRenderSection& RenderSection = TranslucentSections[j];
		lcMesh* Mesh = RenderSection.Mesh;

		if (Mesh->m_VertexBuffer != LastVertexBuffer)
		{
			LastVertexBuffer = Mesh->m_VertexBuffer;
			LastVertexBuffer->BindBuffer();
		}

		if (Mesh->m_IndexBuffer != LastIndexBuffer)
		{
			LastIndexBuffer = Mesh->m_IndexBuffer;
			LastIndexBuffer->BindBuffer();
		}

		if (LastPiece != RenderSection.Owner)
		{
			LastPiece = RenderSection.Owner;
			glPopMatrix();
			glPushMatrix();
			glMultMatrixf(LastPiece->m_ModelWorld);
		}

		lcSetColor(RenderSection.Color);

		lcMeshSection* Section = RenderSection.Section;

#if LC_PROFILE
		if (Section->PrimitiveType == GL_QUADS)
			g_RenderStats.QuadCount += Section->IndexCount / 4;
		else if (Section->PrimitiveType == GL_TRIANGLES)
			g_RenderStats.TriCount += Section->IndexCount / 3;
		else if (Section->PrimitiveType == GL_LINES)
			g_RenderStats.LineCount += Section->IndexCount / 2;
#endif

		glDrawElements(Section->PrimitiveType, Section->IndexCount, Mesh->m_IndexType, (char*)LastIndexBuffer->GetDrawElementsOffset() + Section->IndexOffset);
	}

	glPopMatrix();

	// Reset states.
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	if (LastVertexBuffer)
		LastVertexBuffer->UnbindBuffer();

	if (LastIndexBuffer)
		LastIndexBuffer->UnbindBuffer();

	if (Interface)
	{
		glLineWidth(2.0f);

		// Draw selection boxes.
		for (lcPiece * Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		{
			if (!Piece->IsVisible(m_ActiveModel->m_CurFrame) || !Piece->IsSelected())
				continue;

			const BoundingBox& Box = Piece->m_PieceInfo->m_BoundingBox;
			Vector3 Size = Box.m_Max - Box.m_Min;

			Matrix44 ScaleMatrix(Vector4(Size[0], 0.0f, 0.0f, 0.0f), Vector4(0.0f, Size[1], 0.0f, 0.0f), Vector4(0.0f, 0.0f, Size[2], 0.0f), Vector4(Box.GetCenter(), 1.0f));
			Matrix44 ModelWorld = Mul(ScaleMatrix, Piece->m_ModelWorld);

			glPushMatrix();
			glMultMatrixf(ModelWorld);
			lcSelectionMesh->Render(LC_COLOR_SELECTION, true, Piece->IsFocused());
			glPopMatrix();
		}

		// Add piece preview.
		if (m_nCurAction == LC_ACTION_INSERT)
		{
			Vector3 Pos;
			Vector4 Rot;
			GetPieceInsertPosition(m_nDownX, m_nDownY, Pos, Rot);

			glPushMatrix();
			glTranslatef(Pos[0], Pos[1], Pos[2]);
			glRotatef(Rot[3], Rot[0], Rot[1], Rot[2]);
			glLineWidth(2*m_fLineWidth);
			g_App->m_PiecePreview->m_Selection->RenderPiece(g_App->m_SelectedColor);
			glLineWidth(m_fLineWidth);
			glPopMatrix();
		}
	}

	if (m_nDetail & LC_DET_LIGHTING)
	{
		glDisable(GL_LIGHTING);

		int index = 0;
		lcLight* Light;

		for (Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next, index++)
			glDisable((GLenum)(GL_LIGHT0+index));
	}

	// Draw cameras and lights.
	for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
	{
		if ((Camera == view->GetCamera()) || !Camera->IsVisible())
			continue;
		Camera->Render(m_fLineWidth);
	}

	for (lcLight* Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
	{
		if (Light->IsVisible())
			Light->Render(m_fLineWidth);
	}

#ifdef LC_DEBUG
	RenderDebugPrimitives();
#endif
}

void Project::RenderSceneBoxes(View* view)
{
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glShadeModel(GL_FLAT);
//	glEnable(GL_CULL_FACE);

	if ((m_nDetail & LC_DET_BOX_FILL) == 0)
	{
		if ((m_nDetail & LC_DET_HIDDEN_LINE) != 0) // TODO: Remove LC_DET_HIDDEN_LINE
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
		glLoadIdentity();
		glOrtho(0, w, 0, h, -50, 50);
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

			float Verts[11][3];

			Verts[0][0] = 0.0f;
			Verts[0][1] = 0.0f;
			Verts[0][2] = 0.0f;

			Verts[1][0] = pts[i][0];
			Verts[1][1] = pts[i][1];
			Verts[1][2] = pts[i][2];

			for (int j = 0; j < 9; j++)
			{
				Vector3 pt(12.0f, cosf(LC_2PI * j / 8) * 3.0f, sinf(LC_2PI * j / 8) * 3.0f);
				pt = Mul30(pt, Mats[i]);
				Verts[j+2][0] = pt[0];
				Verts[j+2][1] = pt[1];
				Verts[j+2][2] = pt[2];
			}

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Verts);

			glDrawArrays(GL_LINES, 0, 2);
			glDrawArrays(GL_TRIANGLE_FAN, 1, 10);

			glDisableClientState(GL_VERTEX_ARRAY);
		}

		// Draw the text.
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_pScreenFont->MakeCurrent();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);

		glColor4f(0, 0, 0, 1);
		m_pScreenFont->PrintText(pts[0][0], pts[0][1], 40.0f, "X");
		m_pScreenFont->PrintText(pts[1][0], pts[1][1], 40.0f, "Y");
		m_pScreenFont->PrintText(pts[2][0], pts[2][1], 40.0f, "Z");

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
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
		glEnableLineStipple();
		glColor4f(0, 0, 0, 1);

		float pt1x = (float)m_nDownX;
		float pt1y = (float)m_nDownY;
		float pt2x = m_fTrack[0];
		float pt2y = m_fTrack[1];

		float verts[8][2] =
		{
			{ pt1x, pt1y }, { pt2x, pt1y },
			{ pt2x, pt1y }, { pt2x, pt2y },
			{ pt2x, pt2y }, { pt1x, pt2y },
			{ pt1x, pt2y }, { pt1x, pt1y }
		};

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, verts);
		glDrawArrays(GL_LINES, 0, 8);
		glDisableClientState(GL_VERTEX_ARRAY);

		glDisableLineStipple();
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
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
	else
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

	glEnableClientState(GL_VERTEX_ARRAY);

	float Verts[4][2] =
	{
		{ 0, 0 }, { (float)view->GetWidth(), 0 }, { (float)view->GetWidth(), (float)view->GetHeight() }, { 0, (float)view->GetHeight() }
	};

	glVertexPointer(2, GL_FLOAT, 0, Verts);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);

	// Draw camera name.
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	m_pScreenFont->MakeCurrent();
	glEnable(GL_ALPHA_TEST);

	glColor4f(0, 0, 0, 1);
	m_pScreenFont->PrintText(3, h - 6, 0.0f, view->GetCamera()->m_Name);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
}

void Project::RenderOverlays(View* view)
{
	Matrix44 Projection = view->GetProjectionMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(Projection);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(view->GetCamera()->m_WorldView);

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

			if ((Focus != NULL) && Focus->IsPiece())
			{
				Rot = ((lcPiece*)Focus)->m_ModelWorld;
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

			glColor4f(0.8f, 0.8f, 0.0f, 0.3f);

			float verts[4][3] =
			{
				{ 0.0f, 0.0f, 0.0f },
				{ 0.0f, OverlayScale * OverlayMovePlaneSize, 0.0f },
				{ 0.0f, OverlayScale * OverlayMovePlaneSize, OverlayScale * OverlayMovePlaneSize },
				{ 0.0f, 0.0f, OverlayScale * OverlayMovePlaneSize }
			};

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, verts);
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
				glMultMatrixf(Rot);

			if (i == 1)
				glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
			else if (i == 2)
				glRotatef(90.0f, 0.0f, -1.0f, 0.0f);

			float Verts[11][3];

			Verts[0][0] = 0.0f;
			Verts[0][1] = 0.0f;
			Verts[0][2] = 0.0f;

			Verts[1][0] = OverlayScale * OverlayMoveArrowSize;
			Verts[1][1] = 0.0f;
			Verts[1][2] = 0.0f;

			for (int j = 0; j < 9; j++)
			{
				Verts[j+2][0] = OverlayScale * OverlayMoveArrowCapSize;
				Verts[j+2][1] = cosf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius * OverlayScale;
				Verts[j+2][2] = sinf(LC_2PI * j / 8) * OverlayMoveArrowCapRadius * OverlayScale;
			}

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINES, 0, 2);
			glDrawArrays(GL_TRIANGLE_FAN, 1, 10);
			glDisableClientState(GL_VERTEX_ARRAY);

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

			if ((Focus != NULL) && Focus->IsPiece())
			{
				Rot = ((lcPiece*)Focus)->m_ModelWorld;
				Rot.SetTranslation(Vector3(0, 0, 0));
			}
			else
				Focus = NULL;
		}

		// Draw a disc showing the rotation amount.
		if (LengthSquared(m_MouseTotalDelta) != 0.0f && (m_nTracking != LC_TRACK_NONE))
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

				float Verts[33][3];
				int v = 0;

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, Verts);

				Verts[0][0] = 0.0f;
				Verts[0][1] = 0.0f;
				Verts[0][2] = 0.0f;
				v++;

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

					Verts[v][0] = 0.0f;
					Verts[v][1] = x;
					Verts[v][2] = y;
					v++;

					if (v == 33)
					{
						glDrawArrays(GL_TRIANGLE_FAN, 0, v);
						Verts[1][0] = Verts[32][0];
						Verts[1][1] = Verts[32][1];
						Verts[1][2] = Verts[32][2];
						v = 2;
					}

					i++;
					if (Step > 0)
						Angle -= Step;
					else
						Angle += Step;

				} while (Angle >= 0.0f);

				if (v > 2)
					glDrawArrays(GL_TRIANGLE_FAN, 0, v);

				glDisableClientState(GL_VERTEX_ARRAY);
				glDisable(GL_BLEND);

				glPopMatrix();
			}
		}

		Mat = Cam->m_ViewWorld;
		Mat.SetTranslation(m_OverlayCenter);

		// Draw the circles.
		float Verts[32][3];

		for (j = 0; j < 32; j++)
		{
			Vector3 Pt;

			Pt[0] = cosf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
			Pt[1] = sinf(LC_2PI * j / 32) * OverlayRotateRadius * OverlayScale;
			Pt[2] = 0.0f;

			Pt = Mul31(Pt, Mat);

			Verts[j][0] = Pt[0];
			Verts[j][1] = Pt[1];
			Verts[j][2] = Pt[2];
		}

		glColor4f(0.1f, 0.1f, 0.1f, 1.0f);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Verts);
		glDrawArrays(GL_LINE_LOOP, 0, 32);
		glDisableClientState(GL_VERTEX_ARRAY);

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

			float Verts[64][3];
			int v = 0;

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

					Verts[v][0] = Pt1[0];
					Verts[v][1] = Pt1[1];
					Verts[v][2] = Pt1[2];
					v++;
					Verts[v][0] = Pt2[0];
					Verts[v][1] = Pt2[1];
					Verts[v][2] = Pt2[2];
					v++;
				}
			}

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINES, 0, v);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		// Draw tangent vector.
		if (m_nTracking != LC_TRACK_NONE)
		{
			if ((m_OverlayMode == LC_OVERLAY_X) || (m_OverlayMode == LC_OVERLAY_Y) || (m_OverlayMode == LC_OVERLAY_Z))
			{
				Vector3 Normal = Normalize(m_OverlayTrackStart - m_OverlayCenter);
				Vector3 Tangent;
				float Angle = 0;

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

					float Verts[6][3];

					Verts[0][0] = Pt[0];
					Verts[0][1] = Pt[1];
					Verts[0][2] = Pt[2];
					Verts[1][0] = Tip[0];
					Verts[1][1] = Tip[1];
					Verts[1][2] = Tip[2];

					Rot = MatrixFromAxisAngle(Vector4(Normal, LC_PI * 0.15f));
					Arrow = Mul(Tangent, Rot) * OverlayRotateArrowCapSize;

					Verts[2][0] = Tip[0];
					Verts[2][1] = Tip[1];
					Verts[2][2] = Tip[2];
					Verts[3][0] = Tip[0] - Arrow[0];
					Verts[3][1] = Tip[1] - Arrow[1];
					Verts[3][2] = Tip[2] - Arrow[2];

					Rot = MatrixFromAxisAngle(Vector4(Normal, -LC_PI * 0.15f));
					Arrow = Mul(Tangent, Rot) * OverlayRotateArrowCapSize;

					Verts[4][0] = Tip[0];
					Verts[4][1] = Tip[1];
					Verts[4][2] = Tip[2];
					Verts[5][0] = Tip[0] - Arrow[0];
					Verts[5][1] = Tip[1] - Arrow[1];
					Verts[5][2] = Tip[2] - Arrow[2];

					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);

					glEnableClientState(GL_VERTEX_ARRAY);
					glVertexPointer(3, GL_FLOAT, 0, Verts);
					glDrawArrays(GL_LINES, 0, 6);
					glDisableClientState(GL_VERTEX_ARRAY);
				}

				// Draw text.
				if (view == m_ActiveView)
				{
					Vector3 ScreenPos = ProjectPoint(Vector3(0.0f, 0.0f, 0.0f), view->GetCamera()->m_WorldView, view->GetProjectionMatrix(), view->m_Viewport);

					glMatrixMode(GL_PROJECTION);
					glPushMatrix();
					glLoadIdentity();
					glOrtho(m_ActiveView->m_Viewport[0], m_ActiveView->m_Viewport[2], m_ActiveView->m_Viewport[1], m_ActiveView->m_Viewport[3], -1, 1);
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

					glColor4f(0.8f, 0.8f, 0.0f, 1.0f);
					m_pScreenFont->PrintText(ScreenPos[0] - m_ActiveView->m_Viewport[0] - (cx / 2), ScreenPos[1] - m_ActiveView->m_Viewport[1] + (cy / 2), 0.0f, buf);

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
		glColor4f(0, 0, 0, 1);

		// Draw circle.
		float verts[32][2];

		float r = min(w, h) * 0.35f;
		float cx = w / 2.0f;
		float cy = h / 2.0f;

		for (int i = 0; i < 32; i++)
		{
			verts[i][0] = cosf((float)i / 32.0f * (2.0f * LC_PI)) * r + cx;
			verts[i][1] = sinf((float)i / 32.0f * (2.0f * LC_PI)) * r + cy;
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, verts);
		glDrawArrays(GL_LINE_LOOP, 0, 32);

		const float OverlayCameraSquareSize = max(8.0f, (w+h)/200);

		// Draw squares.
		float Squares[16][3] =
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

		glVertexPointer(3, GL_FLOAT, 0, Squares);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		glDrawArrays(GL_LINE_LOOP, 4, 4);
		glDrawArrays(GL_LINE_LOOP, 8, 4);
		glDrawArrays(GL_LINE_LOOP, 12, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
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
		glEnableLineStipple();
		glColor4f(0, 0, 0, 1);

		float pt1x = (float)m_nDownX;
		float pt1y = (float)m_nDownY;
		float pt2x = m_OverlayTrackStart[0];
		float pt2y = m_OverlayTrackStart[1];

		float Verts[8][2] =
		{
			{ pt1x, pt1y }, { pt2x, pt1y },
			{ pt2x, pt1y }, { pt2x, pt2y },
			{ pt2x, pt2y }, { pt1x, pt2y },
			{ pt1x, pt2y }, { pt1x, pt1y }
		};

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, Verts);
		glDrawArrays(GL_LINES, 0, 8);
		glDisableClientState(GL_VERTEX_ARRAY);

		glDisableLineStipple();
		glEnable(GL_DEPTH_TEST);
	}
}

// bHilite - Draws focus/selection, not used for the 
// first rendering pass if remove hidden lines is enabled
void Project::RenderBoxes(bool bHilite)
{
	for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		if (Piece->IsVisible(m_ActiveModel->m_CurFrame))
			Piece->RenderBox(bHilite, m_fLineWidth);
}

// Initialize OpenGL
void Project::RenderInitialize()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);

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

			lcPiece* Piece = Model->m_Pieces;
			lcPiece* Prev = NULL;

			while (Piece)
			{
				PieceInfo* Info = Piece->m_PieceInfo;

				if (!(Info->m_nFlags & LC_PIECE_MODEL) || (Info->m_Model != DeleteModel))
				{
					Prev = Piece;
					Piece = (lcPiece*)Piece->m_Next;
					continue;
				}

				if (Inline)
					Model->InlineModel(DeleteModel, Piece->m_ModelWorld, Piece->m_Color, Piece->m_TimeShow, Piece->m_TimeHide);

				Model->RemovePiece(Piece);
				delete Piece;

				if (Prev)
					Piece = Prev;
				else
					Piece = Model->m_Pieces;
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

void Project::UpdateAllModelMeshes()
{
	lcPtrArray<lcModel> ModelList;

	// Sort models by dependency.
	for (int ModelIndex = 0; ModelIndex < m_ModelList.GetSize(); ModelIndex++)
	{
		lcModel* Model = m_ModelList[ModelIndex];

		int ListIndex;
		for (ListIndex = 0; ListIndex < ModelList.GetSize(); ListIndex++)
		{
			if (Model->IsSubModel(ModelList[ListIndex]))
			{
				ModelList.InsertAt(ListIndex, Model);
				break;
			}
		}

		if (ListIndex == ModelList.GetSize())
			ModelList.Add(Model);
	}

	for (int ListIndex = ModelList.GetSize()-1; ListIndex >= 0; ListIndex--)
		ModelList[ListIndex]->UpdateMesh();
}

void Project::CalculateStep()
{
	lcPiece* Piece;
	lcCamera* Camera;
	lcLight* Light;

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		Piece->UpdatePosition(m_ActiveModel->m_CurFrame);

	for (Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
		Camera->UpdatePosition(m_ActiveModel->m_CurFrame);

	for (Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
		Light->UpdatePosition(m_ActiveModel->m_CurFrame);
}

// Returns true if anything was removed (used by cut and del)
bool Project::RemoveSelectedObjects()
{
	lcPiece* Piece;
	lcCamera* Camera;
	lcLight* Light;
	lcObject* Prev;
	bool Removed = false;

	Piece = m_ActiveModel->m_Pieces;
	while (Piece)
	{
		if (Piece->IsSelected())
		{
			lcPiece* Temp;
			Temp = (lcPiece*)Piece->m_Next;

			Removed = true;
			m_ActiveModel->RemovePiece(Piece);
			delete Piece;
			Piece = Temp;
		}
		else
			Piece = (lcPiece*)Piece->m_Next;
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

		if (CanDelete && Camera->IsSelected() && Camera->IsUser())
		{
			if (Prev)
			{
				Prev->m_Next = Camera->m_Next;
				delete Camera;
				Camera = (lcCamera*)Prev->m_Next;
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
		{
			Prev = Camera;
			Camera = (lcCamera*)Camera->m_Next;
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
				Light = (lcLight*)Prev->m_Next;
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
			Light = (lcLight*)Light->m_Next;
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
		lcPiece* Piece;
		lcGroup* pGroup = NULL;
		bool first = true;

		for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
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

	for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
		if (Camera->IsSelected())
		{
			flags |= LC_SEL_CAMERA;
			SelectedCount++;

			if (Camera->IsEyeFocused() || Camera->IsTargetFocused())
				Focus = Camera;
		}

	for (lcLight* Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
		if (Light->IsSelected())
		{
			flags |= LC_SEL_LIGHT;
			SelectedCount++;

			if (Light->IsEyeFocused() || Light->IsTargetFocused())
				Focus = Light;
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

u32 Project::GetLastStep() // TODO: remove function
{
	return m_ActiveModel->GetLastStep();
}

// Create a series of pictures
void Project::CreateImages (Image* images, int width, int height, unsigned short from, unsigned short to, bool hilite)
{
	void* render = Sys_StartMemoryRender (width, height);
	unsigned char* buf = (unsigned char*)malloc(width*height*3);
	u32 oldtime = m_ActiveModel->m_CurFrame;

	View view(this, m_ActiveView);
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
			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
			{
				if (Piece->m_TimeShow == i)
					Piece->Select(true, false, false);
				else
					Piece->Select(false, false, false);
			}
		}

		CalculateStep();
		Render(&view, false, false);
		images[i-from].FromOpenGL (width, height);
	}

	m_ActiveModel->m_CurFrame = oldtime;
	CalculateStep();
	free (buf);
	Sys_FinishMemoryRender (render);
}

void Project::CreateHTMLPieceList(FILE* f, int nStep, bool bImages, const char* ext)
{
	lcPiece* Piece;
	int* col = new int[lcNumColors], ID = 0, c;
	int* count = new int[lcNumColors];
	memset(&col, 0, sizeof(int)*lcNumColors);

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
	{
		if ((Piece->m_TimeShow == nStep) || (nStep == 0))
			col[Piece->m_Color]++;
	}

	fputs("<br><table border=1><tr><td><center>Piece</center></td>\n",f);

	for (c = 0; c < lcNumColors; c++)
	if (col[c])
	{
		col[c] = ID;
		ID++;
		fprintf(f, "<td><center>%s</center></td>\n", g_ColorList[c].Name);
	}
	ID++;
	fputs("</tr>\n",f);

	PieceInfo* pInfo;
	for (int j = 0; j < lcGetPiecesLibrary()->GetPieceCount (); j++)
	{
		bool Add = false;
		memset(&count, 0, sizeof(int)*lcNumColors);
		pInfo = lcGetPiecesLibrary()->GetPieceInfo (j);

		for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		{
			if ((Piece->m_PieceInfo == pInfo) && ((Piece->m_TimeShow == nStep) || (nStep == 0)))
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
			for (c = 0; c < lcNumColors; c++)
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
	delete[] col;
	delete[] count;
}

// Special notifications.
void Project::HandleNotify(LC_NOTIFY id, unsigned long param)
{
	switch (id)
	{
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
			lcPiece* Piece = (lcPiece*)mod->piece;

			Matrix33 Mat = Piece->m_ModelWorld;
			Vector3 Angles = MatrixToEulerAngles(Mat) * LC_RTOD;

			if (Piece->m_Position != mod->Position)
				Piece->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->Position, LC_PK_POSITION);

			if (mod->Rotation[0] != Angles[0] || mod->Rotation[1] != Angles[1] || mod->Rotation[2] != Angles[2])
			{
				Mat = MatrixFromEulerAngles(mod->Rotation * LC_DTOR);
				Vector4 Rot = MatrixToAxisAngle(Mat);
				Piece->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, Rot, LC_PK_ROTATION);
			}

			Piece->m_TimeShow = mod->from;
			Piece->m_TimeHide = mod->to;

			if (mod->hidden)
				Piece->Hide();
			else
				Piece->UnHide();

			Piece->m_Name = mod->name;
			Piece->m_Color = mod->color;
			Piece->UpdatePosition(m_ActiveModel->m_CurFrame);

			SetModifiedFlag(true);
			CheckPoint("Modifying");
			ActivateOverlay();
			UpdateAllViews();
		} break;

		case LC_CAMERA_MODIFIED:
		{
			LC_CAMERA_MODIFY* mod = (LC_CAMERA_MODIFY*)param;
			lcCamera* Camera = (lcCamera*)mod->camera;

			if (mod->hidden)
				Camera->Hide();
			else
				Camera->UnHide();
//			Camera->SetOrtho(mod->ortho);
//			Camera->ShowCone(mod->cone);

			if (Camera->m_Position != mod->Eye)
				Camera->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->Eye, LC_CK_EYE);

			if (Camera->m_TargetPosition != mod->Target)
				Camera->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, mod->Target, LC_CK_TARGET);

			if (Camera->m_Roll != mod->Roll)
				Camera->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, &mod->Roll, LC_CK_ROLL);

			Camera->m_Name = mod->name;
			Camera->m_FOV = mod->fovy;
			Camera->m_NearDist = mod->znear;
			Camera->m_FarDist = mod->zfar;
			Camera->UpdatePosition(m_ActiveModel->m_CurFrame);
			UpdateAllViews();
		} break;

		case LC_LIGHT_MODIFIED:
		{
		} break;
	}
}

void Project::ProcessMessage(lcMessageType Message, void* Data)
{
	switch (Message)
	{
	case LC_MSG_FOCUS_OBJECT_CHANGED:
		UpdateSelection();
		break;

	case LC_MSG_MOUSE_CAPTURE_LOST:
		if (m_nTracking != LC_TRACK_NONE)
			StopTracking(false);
		break;
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
			DoSave(NULL);
		} break;

		case LC_FILE_PICTURE:
		{
			LC_IMAGEDLG_OPTS opts;
			opts.from = 1;
			opts.to = m_Animation ? m_ActiveModel->m_TotalFrames : GetLastStep();
			opts.multiple = m_Animation;
			opts.imopts.background[0] = (unsigned char)(m_fBackground[0]*255);
			opts.imopts.background[1] = (unsigned char)(m_fBackground[1]*255);
			opts.imopts.background[2] = (unsigned char)(m_fBackground[2]*255);

			if (SystemDoDialog(LC_DLG_PICTURE_SAVE, &opts))
			{
				if (m_Animation)
					opts.to = min(opts.to, m_ActiveModel->m_TotalFrames);
				else
					opts.to = min(opts.to, GetLastStep());
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
					SaveVideo(opts.filename, images, opts.to-opts.from+1, m_Animation ? m_nFPS : 60.0f/opts.imopts.pause);
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
				u32 i;
				u32 last = GetLastStep();

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
                                        if (f == NULL)
                                        {
                                                perror(fn);
						return;
                                        }
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
        	                                if (f == NULL)
        	                                {
        	                                        perror(fn);
							return;
        	                                }
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
	                                        if (f == NULL)
	                                        {
	                                                perror(fn);
							return;
	                                        }
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
	                                        if (f == NULL)
	                                        {
	                                                perror(fn);
							return;
	                                        }
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

					lcPiece *p1, *p2;
					PieceInfo* pInfo;
					for (p1 = m_ActiveModel->m_Pieces; p1; p1 = (lcPiece*)p1->m_Next)
					{
						bool bSkip = false;
						pInfo = p1->m_PieceInfo;

						for (p2 = m_ActiveModel->m_Pieces; p2; p2 = (lcPiece*)p2->m_Next)
						{
							if (p2 == p1)
								break;

							if (p2->m_PieceInfo == pInfo)
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
						glEnable(GL_COLOR_MATERIAL);
						glDisable (GL_DITHER);
						glShadeModel(GL_FLAT);
						pInfo->ZoomExtents(30.0f, aspect);

						float pos[4] = { 0, 0, 10, 0 };
						glLightfv(GL_LIGHT0, GL_POSITION, pos);
						glEnable(GL_LIGHTING);
						glEnable(GL_LIGHT0);
						glEnable(GL_DEPTH_TEST);
						pInfo->RenderPiece(g_App->m_SelectedColor);
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
			lcPiece* pPiece;
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
			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
			{
				lcPiece* pNext;

				for (pNext = m_ActiveModel->m_Pieces; pNext; pNext = (lcPiece*)pNext->m_Next)
				{
					pInfo = pNext->m_PieceInfo;

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

					if (pInfo == pPiece->m_PieceInfo)
						break;
				}
			}

			// TODO: table doesn't match new colors in 0.76
			const char* lg_colors[28] = { "red", "Orange", "green", "mint", "blue", "LightBlue", "yellow", 
				"white", "dark_grey", "black", "brown", "pink", "purple", "gold_chrome", "clear_red",
				"clear_neon_orange", "clear_green", "clear_neon_yellow", "clear_blue", "clear_cyan", 
				"clear_yellow", "clear", "grey", "tan", "LightBrown", "rose", "Turquoise", "chrome" };

			const float mycol[4][4] = { { 1.0f, 0.5f, 0.2f, 1 }, { 0.2f, 0.4f, 0.9f, 5 }, 
				{ 0.6f, 0.4f, 0.4f, 24 }, { 0.1f, 0.7f, 0.8f, 26 }};

			if (strlen(opts.libpath))
			{
				for (u = 0; u < 4; u++)
				{
					char altname[256];
					strcpy(altname, g_ColorList[(int)mycol[u][3]].Name);
					while (char* ptr = (char*)strchr(altname, ' '))
						*ptr = '_';

					fprintf(f, "\n#declare lg_%s = texture {\n pigment { rgb <%.2f, %.2f, %.2f> }\n finish {\n  ambient 0.1\n  phong 0.3\n  phong_size 20\n }\n}\n",
						altname, mycol[u][0], mycol[u][1], mycol[u][2]);
				}
			}
			else
			{
				fputs("#include \"colors.inc\"\n\n", f);

				for (int u = 0; u < lcNumColors; u++)
					fprintf(f, "\n#declare lg_%s = texture {\n pigment { rgbf <%.2f, %.2f, %.2f, %.2f> }\n finish {\n  ambient 0.1\n  phong 0.3\n  phong_size 20\n }\n}\n",
						lg_colors[u], g_ColorList[u].Value[0], g_ColorList[u].Value[1], g_ColorList[u].Value[2], ((g_ColorList[u].Value[3] == 1.0f) ? 0.0f : 0.9f));
			}

			// if not in lgeo, create it
			fputs("\n// The next objects (if any) were generated by LeoCAD.\n\n", f);

			bool* ExportList = new bool[lcGetPiecesLibrary()->GetPieceCount()];
			memset(ExportList, 0, sizeof(bool)*lcGetPiecesLibrary()->GetPieceCount());

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
			{
				pInfo = pPiece->m_PieceInfo;
				int idx = lcGetPiecesLibrary()->GetPieceIndex(pInfo);
				if (conv[idx*9] != 0)
					continue;

				ExportList[idx] = true;
			}

			for (int idx = 0; idx < lcGetPiecesLibrary()->GetPieceCount(); idx++)
			{
				if (!ExportList[idx])
					continue;

				pInfo = lcGetPiecesLibrary()->GetPieceInfo(idx);

				char name[20];
				strcpy(name, pInfo->m_strName);
				while ((ptr = strchr(name, '-')))
					*ptr = '_';
				fprintf(f, "#declare lc_%s = union {\n", name);

				void* indices = pInfo->m_Mesh->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);
				float* verts = (float*)pInfo->m_Mesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);

				for (int SectionIndex = 0; SectionIndex < pInfo->m_Mesh->m_SectionCount; SectionIndex++)
				{
					lcMeshSection* Section = &pInfo->m_Mesh->m_Sections[SectionIndex];

					if (Section->PrimitiveType != GL_TRIANGLES)
						continue;

					if (pInfo->m_Mesh->m_IndexType == GL_UNSIGNED_INT)
					{
						fputs(" mesh {\n", f);
						u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
						for (int c = 0; c < Section->IndexCount; c += 3)
						{
							int i0 = IndexPtr[c+0] * 3;
							int i1 = IndexPtr[c+1] * 3;
							int i2 = IndexPtr[c+2] * 3;

							fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
							        -verts[i0+1], -verts[i0], verts[i0+2],
							        -verts[i1+1], -verts[i1], verts[i1+2],
							        -verts[i2+1], -verts[i2], verts[i2+2]);
						}

						if (Section->ColorIndex != LC_COLOR_DEFAULT && Section->ColorIndex != LC_COLOR_EDGE)
							fprintf (f, "  texture { lg_%s }\n", lg_colors[Section->ColorIndex]);
						fputs(" }\n", f);
					}
					else
					{
						fputs(" mesh {\n", f);
						u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
						for (int c = 0; c < Section->IndexCount; c += 3)
						{
							int i0 = IndexPtr[c+0] * 3;
							int i1 = IndexPtr[c+1] * 3;
							int i2 = IndexPtr[c+2] * 3;

							fprintf(f, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
							        -verts[i0+1], -verts[i0], verts[i0+2],
							        -verts[i1+1], -verts[i1], verts[i1+2],
							        -verts[i2+1], -verts[i2], verts[i2+2]);
						}

						if (Section->ColorIndex != LC_COLOR_DEFAULT && Section->ColorIndex != LC_COLOR_EDGE)
							fprintf (f, "  texture { lg_%s }\n", lg_colors[Section->ColorIndex]);
						fputs(" }\n", f);
					}
				}
				fputs("}\n\n", f);

				pInfo->m_Mesh->m_IndexBuffer->UnmapBuffer();
				pInfo->m_Mesh->m_VertexBuffer->UnmapBuffer();
			}

			delete[] ExportList;

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
			Vector3 target = m_ActiveView->GetCamera()->m_TargetPosition;
			Vector3 up = Vector3(m_ActiveView->GetCamera()->m_ViewWorld[1]);

			fprintf(f, "\ncamera {\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f\n}\n\n",
				up[0], up[1], up[2], eye[1], eye[0], eye[2], target[1], target[0], target[2], m_ActiveView->GetCamera()->m_FOV);
			fprintf(f, "background { color rgb <%1g, %1g, %1g> }\n\nlight_source { <0, 0, 20> White shadowless }\n\n",
				m_fBackground[0], m_fBackground[1], m_fBackground[2]);

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
			{
				float fl[12];
				char name[20];
				int idx = lcGetPiecesLibrary()->GetPieceIndex(pPiece->m_PieceInfo);

				if (conv[idx*9] == 0)
				{
					char* ptr;
					sprintf(name, "lc_%s", pPiece->m_PieceInfo->m_strName);
					while ((ptr = strchr(name, '-')))
						*ptr = '_';
				}
				else
				{
					strcpy (name, &conv[idx*9]);
					if (LC_COLOR_TRANSLUCENT(pPiece->m_Color))
						strcat(name, "_clear");			
				}

				Matrix mat(pPiece->m_AxisAngle, pPiece->m_Position);
				const Vector3& Position = pPiece->m_Position;
				mat.ToLDraw(fl);

				// Slope needs to be handled correctly
				if (flags[idx] == 1)
					fprintf (f, "merge {\n object {\n  %s\n  texture { lg_%s }\n }\n"
						 " object {\n  %s_slope\n  texture { lg_%s normal { bumps 0.3 scale 0.02 } }\n }\n"
						 " matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
						name, lg_colors[pPiece->m_Color], &conv[idx*9], lg_colors[pPiece->m_Color],
						 -fl[11], -fl[5], fl[8], -fl[9], -fl[3], fl[6],
						 -fl[10], -fl[4], fl[7], Position[1], Position[0], Position[2]);
				else
					fprintf(f, "object {\n %s\n texture { lg_%s }\n matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
						name, lg_colors[pPiece->m_Color], -fl[11], -fl[5], fl[8], -fl[9], -fl[3], fl[6],
						-fl[10], -fl[4], fl[7], Position[1], Position[0], Position[2]);
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

		// Export to VRML97, exchange x -> z, z -> y, y -> x, 
		case LC_FILE_VRML97:
		{
			char filename[LC_MAXPATH];
			if (!SystemDoDialog(LC_DLG_VRML97, filename))
				break;

			exportVRML97File(filename);
		} break;

		// Export to X3DV for rigid body component
                // exchange x -> z, z -> y, y -> x, 
		case LC_FILE_X3DV:
		{
			char filename[LC_MAXPATH];
			if (!SystemDoDialog(LC_DLG_X3DV, filename))
				break;

			exportX3DVFile(filename);
		} break;

		case LC_FILE_WAVEFRONT:
		{
			char filename[LC_MAXPATH];
			if (!SystemDoDialog(LC_DLG_WAVEFRONT, filename))
				break;

			char buf[LC_MAXPATH], *ptr;
			FILE* stream = fopen(filename, "wt");
			unsigned long vert = 1, i;
			lcPiece* pPiece;

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
			for (int i = 0; i < lcNumColors; i++)
			{
				char altname[256];
				strcpy(altname, g_ColorList[i].Name);
				while (char* ptr = (char*)strchr(altname, ' '))
					*ptr = '_';

				fprintf(mat, "newmtl %s\nKd %.2f %.2f %.2f\n\n", altname, g_ColorList[i].Value[0], g_ColorList[i].Value[1], g_ColorList[i].Value[2]);
			}
			fclose(mat);

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
			{
				PieceInfo* pInfo = pPiece->m_PieceInfo;

				float* VertexPtr = (float*)pInfo->m_Mesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);

				for (int v = 0; v < pInfo->m_Mesh->m_VertexCount*3; v += 3)
				{
					Vector3 tmp = Mul31(Vector3(VertexPtr[v+0], VertexPtr[v+1], VertexPtr[v+2]), pPiece->m_ModelWorld);
					fprintf(stream, "v %.2f %.2f %.2f\n", tmp[0], tmp[1], tmp[2]);
				}
				fputs("#\n\n", stream);

				pInfo->m_Mesh->m_VertexBuffer->UnmapBuffer();
			}

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
			{
				strcpy(buf, pPiece->m_Name);
				for (i = 0; i < strlen(buf); i++)
					if ((buf[i] == '#') || (buf[i] == ' '))
						buf[i] = '_';

				fprintf(stream, "g %s\n", buf);
				pPiece->m_PieceInfo->WriteWavefront(stream, pPiece->m_Color, &vert);
			}

			fclose(stream);
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
			opts.Author = Sys_ProfileLoadString("Default", "User", "LeoCAD");
			opts.PiecesUsed = NULL;

			if (SystemDoDialog(LC_DLG_PROPERTIES, &opts))
			{
				// Create and add new model.
				lcModel* Model = new lcModel();
				Model->m_Name = opts.Name;
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
			DeleteModel(m_ModelList[nParam]);
		} break;

		case LC_MODEL_SET_ACTIVE:
		{
			if ((int)nParam < m_ModelList.GetSize())
			{
				SetActiveModel(m_ModelList[nParam]);
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

			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
			{
				if (Piece->m_PieceInfo->m_nFlags & LC_PIECE_MODEL)
					continue;

				int idx = lcGetPiecesLibrary()->GetPieceIndex(Piece->m_PieceInfo);
				opts.PiecesUsed[idx*lcNumUserColors+Piece->m_Color]++;
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

		case LC_MODEL_LIST:
		{
			SystemDoDialog(LC_DLG_MODEL_LIST, NULL);
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
			if (m_pClipboard[m_nCurClipboard] != NULL)
				delete m_pClipboard[m_nCurClipboard];
			m_pClipboard[m_nCurClipboard] = new FileMem;

			int i = 0;
			lcPiece* pPiece;
			lcCamera* pCamera;
			lcGroup* pGroup;
//			lcLight* pLight;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				if (pPiece->IsSelected())
					i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				if (pPiece->IsSelected())
					pPiece->FileSave(*m_pClipboard[m_nCurClipboard], m_ActiveModel->m_Groups);

			for (i = 0, pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
				i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
				pGroup->FileSave(m_pClipboard[m_nCurClipboard], m_ActiveModel->m_Groups);

			for (i = 0, pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (lcCamera*)pCamera->m_Next)
				if (pCamera->IsSelected())
					i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (lcCamera*)pCamera->m_Next)
				if (pCamera->IsSelected())
					pCamera->FileSave(*m_pClipboard[m_nCurClipboard]);
/*
			for (i = 0, pLight = m_Lights; pLight; pLight = pLight->m_Next)
				if (pLight->IsSelected())
					i++;
			m_pClipboard[m_nCurClipboard]->Write(&i, sizeof(i));

			for (pLight = m_Lights; pLight; pLight = pLight->m_Next)
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
			lcPiece* pPasted = NULL;
			File* file = m_pClipboard[m_nCurClipboard];
			if (file == NULL)
				break;
			file->Seek(0, SEEK_SET);
			SelectAndFocusNone(false);

			file->Read(&i, sizeof(i));
			while (i--)
			{
				char name[9];
				lcPiece* pPiece = new lcPiece(NULL);
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
			lcPiece* pPiece;
			lcGroup** groups = (lcGroup**)malloc(i*sizeof(lcGroup**));
			for (j = 0; j < i; j++)
			{
				groups[j] = new lcGroup();
				groups[j]->FileLoad(file);
			}

			while (pPasted)
			{
				pPiece = pPasted;
				pPasted = (lcPiece*)pPasted->m_Next;
				pPiece->SetUniqueName(m_ActiveModel->m_Pieces, pPiece->m_PieceInfo->m_strDescription);
				pPiece->m_TimeShow = m_ActiveModel->m_CurFrame;
				m_ActiveModel->AddPiece(pPiece);
				pPiece->Select(true, false, false);

				j = (int)pPiece->GetGroup();
				if (j != -1)
					pPiece->SetGroup(groups[j]);
				else
					pPiece->UnGroup(NULL);
			}

			for (j = 0; j < i; j++)
			{
				int g = (int)groups[j]->m_Group;
				groups[j]->m_Group = (g != -1) ? groups[g] : NULL;
			}

			for (j = 0; j < i; j++)
			{
				lcGroup* pGroup;
				bool add = false;
				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				{
					for (pGroup = pPiece->GetGroup(); pGroup; pGroup = pGroup->m_Group)
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

					for (pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
						if (strncmp("Pasted Group #", pGroup->m_strName, 14) == 0)
							if (sscanf(pGroup->m_strName + 14, "%d", &a) == 1)
								if (a > max) 
									max = a;

					sprintf(groups[j]->m_strName, "Pasted Group #%.2d", max+1);
					groups[j]->m_Next = m_ActiveModel->m_Groups;
					m_ActiveModel->m_Groups = groups[j];
				}
				else
					delete groups[j];
			}
			
			free(groups);

			lcCamera* pCamera = m_ActiveModel->m_Cameras;
			while (pCamera->m_Next)
				pCamera = (lcCamera*)pCamera->m_Next;
			file->Read(&i, sizeof(i));

			while (i--)
			{
				pCamera = new lcCamera(8);
				m_ActiveModel->AddCamera(pCamera);
				pCamera->FileLoad(*file);
				pCamera->Select(true, false, false);
				pCamera->m_Target->Select(true, false, false);
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
			lcPiece* pPiece;
			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
					pPiece->Select(true, false, false);

//	pFrame->UpdateInfo();
			UpdateSelection();
			UpdateAllViews ();
		} break;
		
		case LC_EDIT_SELECT_NONE:
		{
			SelectAndFocusNone(false);
			lcPostMessage(LC_MSG_FOCUS_OBJECT_CHANGED, NULL);
			UpdateSelection();
			UpdateAllViews();
		} break;
		
		case LC_EDIT_SELECT_INVERT:
		{
			lcPiece* pPiece;
			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
				{
                                  if (pPiece->IsSelected())
                                    pPiece->Select(false, false, false);
                                  else
                                    pPiece->Select(true, false, false);
				}

			lcPostMessage(LC_MSG_FOCUS_OBJECT_CHANGED, NULL);
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_EDIT_SELECT_BYNAME:
		{
			lcPiece* pPiece;
			lcCamera* pCamera;
			lcLight* pLight;
			lcGroup* pGroup;
			int i = 0;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
					i++;

			for (pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (lcCamera*)pCamera->m_Next)
				if (pCamera != m_ActiveView->GetCamera())
					if (pCamera->IsVisible())
						i++;

			for (pLight = m_ActiveModel->m_Lights; pLight; pLight = (lcLight*)pLight->m_Next)
				if (pLight->IsVisible())
					i++;

			// TODO: add only groups with visible pieces
			for (pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
				i++;

			if (i == 0)
			{
				// TODO: say 'Nothing to select'
				break;
			}

			LC_SEL_DATA* opts = (LC_SEL_DATA*)malloc((i+1)*sizeof(LC_SEL_DATA));
			i = 0;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
				{
					opts[i].name = pPiece->m_Name;
					opts[i].type = LC_SELDLG_PIECE;
					opts[i].selected = pPiece->IsSelected();
					opts[i].pointer = pPiece;
					i++;
				}

			for (pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (lcCamera*)pCamera->m_Next)
				if (pCamera != m_ActiveView->GetCamera())
					if (pCamera->IsVisible())
					{
						opts[i].name = pCamera->m_Name;
						opts[i].type = LC_SELDLG_CAMERA;
						opts[i].selected = pCamera->IsSelected();
						opts[i].pointer = pCamera;
						i++;
					}

			for (pLight = m_ActiveModel->m_Lights; pLight; pLight = (lcLight*)pLight->m_Next)
				if (pLight->IsVisible())
				{
					opts[i].name = pLight->m_Name;
					opts[i].type = LC_SELDLG_LIGHT;
					opts[i].selected = pLight->IsSelected();
					opts[i].pointer = pLight;
					i++;
				}

			// TODO: add only groups with visible pieces
			for (pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
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
							((lcPiece*)opts[i].pointer)->Select(true, false, false);
						} break;

						case LC_SELDLG_CAMERA:
						{
							((lcCamera*)opts[i].pointer)->Select(true, false, false);
						} break;

						case LC_SELDLG_LIGHT:
						{
							((lcLight*)opts[i].pointer)->Select(true, false, false);
						} break;

						case LC_SELDLG_GROUP:
						{
							pGroup = (lcGroup*)opts[i].pointer;
							pGroup = pGroup->GetTopGroup();
							for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
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
			lcPiece* Last = NULL;
			lcPiece* Piece = new lcPiece(g_App->m_PiecePreview->m_Selection);

			for (Last = m_ActiveModel->m_Pieces; Last; Last = (lcPiece*)Last->m_Next)
				if ((Last->IsFocused()) || (Last->m_Next == NULL))
					break;

			if (Last != NULL)
			{
				Vector3 Pos;
				Vector4 Rot;

				GetPieceInsertPosition(Last, Pos, Rot);

				Piece->Initialize(Pos[0], Pos[1], Pos[2], m_ActiveModel->m_CurFrame, g_App->m_SelectedColor);

				Piece->ChangeKey(m_ActiveModel->m_CurFrame, false, Rot, LC_PK_ROTATION);
				Piece->UpdatePosition(m_ActiveModel->m_CurFrame);
			}
			else
				Piece->Initialize(0, 0, 0, m_ActiveModel->m_CurFrame, g_App->m_SelectedColor);

			SelectAndFocusNone(false);
			Piece->SetUniqueName(m_ActiveModel->m_Pieces, Piece->m_PieceInfo->m_strDescription);
			m_ActiveModel->AddPiece(Piece);
			Piece->Select (true, true, false);
			lcPostMessage(LC_MSG_FOCUS_OBJECT_CHANGED, Piece);
			UpdateSelection();
			SystemPieceComboAdd(g_App->m_PiecePreview->m_Selection->m_strDescription);

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
				lcPostMessage(LC_MSG_FOCUS_OBJECT_CHANGED, NULL);
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
					lcPiece* Piece = new lcPiece(Wizard.m_Info[i]);

					Piece->Initialize(Wizard.m_Position[i][0], Wizard.m_Position[i][1], Wizard.m_Position[i][2], m_ActiveModel->m_CurFrame, Wizard.m_Colors[i]);
					Piece->SetUniqueName(m_ActiveModel->m_Pieces, Piece->m_PieceInfo->m_strDescription);
					m_ActiveModel->AddPiece(Piece);
					Piece->Select(true, false, false);

					Piece->ChangeKey(1, false, Wizard.m_Rotation[i], LC_PK_ROTATION);
					Piece->UpdatePosition(m_ActiveModel->m_CurFrame);

					SystemPieceComboAdd(Wizard.m_Info[i]->m_strDescription);
				}

				int max = 0;

				lcGroup* pGroup;
				for (pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
					if (strncmp (pGroup->m_strName, "Minifig #", 9) == 0)
						if (sscanf(pGroup->m_strName, "Minifig #%d", &i) == 1)
							if (i > max)
								max = i;
				pGroup = new lcGroup;
				sprintf(pGroup->m_strName, "Minifig #%.2d", max+1);

				pGroup->m_Next = m_ActiveModel->m_Groups;
				m_ActiveModel->m_Groups = pGroup;

				BoundingBox Box;
				Box.Reset();

				for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
					if (Piece->IsSelected())
					{
						Piece->SetGroup(pGroup);
						Piece->MergeBoundingBox(&Box);
					}

				Vector3 Center = Box.GetCenter();
				pGroup->m_fCenter[0] = Center[0];
				pGroup->m_fCenter[1] = Center[1];
				pGroup->m_fCenter[2] = Center[2];

				lcPostMessage(LC_MSG_FOCUS_OBJECT_CHANGED, NULL);
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

				lcPiece *pPiece, *pFirst = NULL, *pLast = NULL;
				int sel = 0;
				unsigned long i, j, k;

				BoundingBox Box;
				Box.Reset();

				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
					if (pPiece->IsSelected())
					{
						pPiece->MergeBoundingBox(&Box);
						sel++;
					}

				Vector3 Center = Box.GetCenter();

				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				{
					if (!pPiece->IsSelected())
						continue;

					for (i = 0; i < opts.n1DCount; i++)
					{
						Vector3 pos = pPiece->m_Position;
						Vector4 param = pPiece->m_AxisAngle;
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
							mat.RotateCenter(i*opts.fRotate[0],1,0,0,Center[0],Center[1],Center[2]);
							mat.RotateCenter(i*opts.fRotate[1],0,1,0,Center[0],Center[1],Center[2]);
							mat.RotateCenter(i*opts.fRotate[2],0,0,1,Center[0],Center[1],Center[2]);
						}
						mat.ToAxisAngle(param);
						mat.GetTranslation(pos);

						if (i != 0)
						{
							if (pLast)
							{
								pLast->m_Next = new lcPiece(pPiece->m_PieceInfo);
								pLast = (lcPiece*)pLast->m_Next;
							}
							else
								pLast = pFirst = new lcPiece(pPiece->m_PieceInfo);

							pLast->Initialize(pos[0]+i*opts.fMove[0], pos[1]+i*opts.fMove[1], pos[2]+i*opts.fMove[2], m_ActiveModel->m_CurFrame, pPiece->m_Color);
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
									pLast->m_Next = new lcPiece(pPiece->m_PieceInfo);
									pLast = (lcPiece*)pLast->m_Next;
								}
								else
									pLast = pFirst = new lcPiece(pPiece->m_PieceInfo);

								pLast->Initialize(pos[0]+i*opts.fMove[0]+j*opts.f2D[0], pos[1]+i*opts.fMove[1]+j*opts.f2D[1], pos[2]+i*opts.fMove[2]+j*opts.f2D[2], m_ActiveModel->m_CurFrame, pPiece->m_Color);
								pLast->ChangeKey(1, false, param, LC_PK_ROTATION);
							}

							if (opts.nArrayDimension == 1)
								continue;

							for (k = 1; k < opts.n3DCount; k++)
							{
								if (pLast)
								{
									pLast->m_Next = new lcPiece(pPiece->m_PieceInfo);
									pLast = (lcPiece*)pLast->m_Next;
								}
								else
									pLast = pFirst = new lcPiece(pPiece->m_PieceInfo);

								pLast->Initialize(pos[0]+i*opts.fMove[0]+j*opts.f2D[0]+k*opts.f3D[0], pos[1]+i*opts.fMove[1]+j*opts.f2D[1]+k*opts.f3D[1], pos[2]+i*opts.fMove[2]+j*opts.f2D[2]+k*opts.f3D[2], m_ActiveModel->m_CurFrame, pPiece->m_Color);
								pLast->ChangeKey(1, false, param, LC_PK_ROTATION);
							}
						}
					}
				}

				while (pFirst)
				{
					pPiece = (lcPiece*)pFirst->m_Next;
					pFirst->SetUniqueName(m_ActiveModel->m_Pieces, pFirst->m_PieceInfo->m_strDescription);
					pFirst->UpdatePosition(m_ActiveModel->m_CurFrame);
					m_ActiveModel->AddPiece(pFirst);
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

		case LC_PIECE_GROUP:
		{
			lcGroup* pGroup;
			int i, max = 0;
			char name[65];

			for (pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
				if (strncmp (pGroup->m_strName, "Group #", 7) == 0)
					if (sscanf(pGroup->m_strName, "Group #%d", &i) == 1)
						if (i > max)
							max = i;
			sprintf(name, "Group #%.2d", max+1);

			if (SystemDoDialog(LC_DLG_GROUP, name))
			{
				pGroup = new lcGroup();
				strcpy(pGroup->m_strName, name);
				pGroup->m_Next = m_ActiveModel->m_Groups;
				m_ActiveModel->m_Groups = pGroup;

				BoundingBox Box;
				Box.Reset();

				for (lcPiece* pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
					if (pPiece->IsSelected())
					{
						pPiece->DoGroup(pGroup);
						pPiece->MergeBoundingBox(&Box);
					}

				Vector3 Center = Box.GetCenter();
				pGroup->m_fCenter[0] = Center[0];
				pGroup->m_fCenter[1] = Center[1];
				pGroup->m_fCenter[2] = Center[2];

				RemoveEmptyGroups();
				SetModifiedFlag(true);
				CheckPoint("Grouping");
			}
		} break;

		case LC_PIECE_UNGROUP:
		{
			lcGroup* pList = NULL;
			lcGroup* pGroup;
			lcGroup* tmp;
			lcPiece* pPiece;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				if (pPiece->IsSelected())
				{
					pGroup = pPiece->GetTopGroup();

					// Check if we already removed the group
					for (tmp = pList; tmp; tmp = tmp->m_Next)
						if (pGroup == tmp)
							pGroup = NULL;

					if (pGroup != NULL)
					{
						// First remove the group from the array
						for (tmp = m_ActiveModel->m_Groups; tmp->m_Next; tmp = tmp->m_Next)
							if (tmp->m_Next == pGroup)
							{
								tmp->m_Next = pGroup->m_Next;
								break;
							}

						if (pGroup == m_ActiveModel->m_Groups)
							m_ActiveModel->m_Groups = pGroup->m_Next;

						// Now add it to the list of top groups
						pGroup->m_Next = pList;
						pList = pGroup;
					}
				}

			while (pList)
			{
				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
					if (pPiece->IsSelected())
						pPiece->UnGroup(pList);

				pGroup = pList;
				pList = pList->m_Next;
				delete pGroup;
			}

			RemoveEmptyGroups();
			SetModifiedFlag(true);
			CheckPoint("Ungrouping");
		} break;

		case LC_PIECE_GROUP_ADD:
		{
			lcGroup* pGroup = NULL;
			lcPiece* pPiece;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				if (pPiece->IsSelected())
				{
					pGroup = pPiece->GetTopGroup();
					if (pGroup != NULL)
						break;
				}

			if (pGroup != NULL)
			{
				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
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
			lcPiece* pPiece;

			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
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
			lcGroup* pGroup;
			lcPiece* pPiece;
			LC_GROUPEDITDLG_OPTS opts;

			for (i = 0, pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
				i++;
			opts.piececount = i;
			opts.pieces = (lcPiece**)malloc(i*sizeof(lcPiece*));
			opts.piecesgroups = (lcGroup**)malloc(i*sizeof(lcGroup*));

			for (i = 0, pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next, i++)
			{
				opts.pieces[i] = pPiece;
				opts.piecesgroups[i] = pPiece->GetGroup();
			}

			for (i = 0, pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
				i++;
			opts.groupcount = i;
			opts.groups = (lcGroup**)malloc(i*sizeof(lcGroup*));
			opts.groupsgroups = (lcGroup**)malloc(i*sizeof(lcGroup*));

			for (i = 0, pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next, i++)
			{
				opts.groups[i] = pGroup;
				opts.groupsgroups[i] = pGroup->m_Group;
			}

			if (SystemDoDialog(LC_DLG_EDITGROUPS, &opts))
			{
				for (i = 0; i < opts.piececount; i++)
					opts.pieces[i]->SetGroup(opts.piecesgroups[i]);

				for (i = 0; i < opts.groupcount; i++)
					opts.groups[i]->m_Group = opts.groupsgroups[i];

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
			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
				if (Piece->IsSelected())
					Piece->Hide();
			UpdateSelection();
			lcPostMessage(LC_MSG_FOCUS_OBJECT_CHANGED, NULL);
			UpdateAllViews();
		} break;

		case LC_PIECE_HIDE_UNSELECTED:
		{
			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
				if (!Piece->IsSelected())
					Piece->Hide();
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_PIECE_UNHIDE_ALL:
		{
			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
				Piece->UnHide();
			UpdateSelection();
			UpdateAllViews();
		} break;

		case LC_PIECE_PREVIOUS:
		{
			bool Redraw = false;

			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
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
			u32 MaxTime = m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX;

			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
			{
				if (!Piece->IsSelected())
					continue;

				u32 t = Piece->m_TimeShow;
				if (t < MaxTime)
				{
					Piece->m_TimeShow++;
					Redraw = true;

					if (Piece->IsSelected() && t == m_ActiveModel->m_CurFrame)
						Piece->Select(false, false, false);
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
			opts.nMouse = g_App->m_MouseSensitivity;
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
				g_App->m_MouseSensitivity = opts.nMouse;
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
			m_ActiveView->GetCamera()->Zoom(m_ActiveModel->m_CurFrame, m_bAddKeys, 0, nParam);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMIN:
		{
			m_ActiveView->GetCamera()->Zoom(m_ActiveModel->m_CurFrame, m_bAddKeys, 0, -1);
			SystemUpdateFocus(NULL);
			UpdateOverlayScale();
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMOUT:
		{
			m_ActiveView->GetCamera()->Zoom(m_ActiveModel->m_CurFrame, m_bAddKeys, 0, 1);
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

			for (int vp = FirstView; vp < LastView; vp++)
			{
				View* view = m_ViewList[vp];
				lcCamera* Camera = view->GetCamera();

				m_ActiveModel->ZoomExtents(view, Camera, m_bAddKeys);
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

			SystemUpdateTime(m_Animation, m_ActiveModel->m_CurFrame, m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
		} break;
		
		case LC_VIEW_STEP_PREVIOUS:
		{
			m_ActiveModel->m_CurFrame--;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateTime(m_Animation, m_ActiveModel->m_CurFrame, m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
		} break;
		
		case LC_VIEW_STEP_FIRST:
		{
			m_ActiveModel->m_CurFrame = 1;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateTime(m_Animation, m_ActiveModel->m_CurFrame, m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
		} break;

		case LC_VIEW_STEP_LAST:
		{
			if (m_Animation)
				m_ActiveModel->m_CurFrame = m_ActiveModel->m_TotalFrames;
			else
				m_ActiveModel->m_CurFrame = GetLastStep();

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateTime(m_Animation, m_ActiveModel->m_CurFrame, m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
		} break;

		case LC_VIEW_STEP_CHOOSE:
		{
			SystemDoDialog(LC_DLG_STEPCHOOSE, NULL);
			SystemUpdateTime(m_Animation, m_ActiveModel->m_CurFrame, m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
		} break;

		case LC_VIEW_STEP_SET:
		{
			if (m_Animation)
				m_ActiveModel->m_CurFrame = lcMin(nParam, m_ActiveModel->m_TotalFrames);
			else
				m_ActiveModel->m_CurFrame = nParam;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateTime(m_Animation, m_ActiveModel->m_CurFrame, m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
		} break;

		case LC_VIEW_STEP_INSERT:
		{
			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
				Piece->InsertTime(m_ActiveModel->m_CurFrame, 1);

			for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
				Camera->InsertTime(m_ActiveModel->m_CurFrame, 1);

			for (lcLight* Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
				Light->InsertTime(m_ActiveModel->m_CurFrame, 1);

			SetModifiedFlag(true);
			if (m_Animation)
				CheckPoint("Adding Frame");
			else
				CheckPoint("Adding Step");
			CalculateStep();
			UpdateAllViews();
			UpdateSelection();
		} break;

		case LC_VIEW_STEP_DELETE:
		{
			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
				Piece->RemoveTime(m_ActiveModel->m_CurFrame, 1);

			for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
				Camera->RemoveTime(m_ActiveModel->m_CurFrame, 1);

			for (lcLight* Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
				Light->RemoveTime(m_ActiveModel->m_CurFrame, 1);

			SetModifiedFlag(true);
			if (m_Animation)
				CheckPoint ("Removing Frame");
			else
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

			m_ActiveView->GetCamera()->Pan(m_ActiveModel->m_CurFrame, m_bAddKeys, x/4, y/4);
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
			m_Animation = !m_Animation;

			CalculateStep();
			SystemUpdateFocus(NULL);
			UpdateAllViews();

			SystemUpdateAnimation(m_Animation, m_bAddKeys);
			SystemUpdateTime(m_Animation, m_ActiveModel->m_CurFrame, m_Animation ? m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX);
		} break;
		
		case LC_TOOLBAR_ADDKEYS:
		{
			m_bAddKeys = !m_bAddKeys;
			SystemUpdateAnimation(m_Animation, m_bAddKeys);
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
	bool recurse = false;
	lcGroup *g1, *g2;
	lcPiece* pPiece;
	int ref;

	for (g1 = m_ActiveModel->m_Groups; g1;)
	{
		ref = 0;

		for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
			if (pPiece->GetGroup() == g1)
				ref++;

		for (g2 = m_ActiveModel->m_Groups; g2; g2 = g2->m_Next)
			if (g2->m_Group == g1)
				ref++;

		if (ref < 2)
		{
			if (ref != 0)
			{
				for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
					if (pPiece->GetGroup() == g1)
						pPiece->SetGroup(g1->m_Group);

				for (g2 = m_ActiveModel->m_Groups; g2; g2 = g2->m_Next)
					if (g2->m_Group == g1)
						g2->m_Group = g1->m_Group;
			}

			if (g1 == m_ActiveModel->m_Groups)
			{
				m_ActiveModel->m_Groups = g1->m_Next;
				delete g1;
				g1 = m_ActiveModel->m_Groups;
			}
			else
			{
				for (g2 = m_ActiveModel->m_Groups; g2; g2 = g2->m_Next)
					if (g2->m_Next == g1)
					{
						g2->m_Next = g1->m_Next;
						break;
					}

				delete g1;
				g1 = g2->m_Next;
			}

			recurse = true;
		}
		else
			g1 = g1->m_Next;
	}

	if (recurse)
		RemoveEmptyGroups();
}

lcGroup* Project::AddGroup(const char* name, lcGroup* pParent, float x, float y, float z)
{
  lcGroup *pNewGroup = new lcGroup();

  if (name == NULL)
  {
    int i, max = 0;
    char str[65];

    for (lcGroup *pGroup = m_ActiveModel->m_Groups; pGroup; pGroup = pGroup->m_Next)
      if (strncmp (pGroup->m_strName, "Group #", 7) == 0)
        if (sscanf(pGroup->m_strName, "Group #%d", &i) == 1)
          if (i > max)
            max = i;
    sprintf (str, "Group #%.2d", max+1);

    strcpy (pNewGroup->m_strName, str);
  }
  else
    strcpy (pNewGroup->m_strName, name);

  pNewGroup->m_Next = m_ActiveModel->m_Groups;
  m_ActiveModel->m_Groups = pNewGroup;

  pNewGroup->m_fCenter[0] = x;
  pNewGroup->m_fCenter[1] = y;
  pNewGroup->m_fCenter[2] = z;
  pNewGroup->m_Group = pParent;

  return pNewGroup;
}

void Project::SelectAndFocusNone(bool bFocusOnly)
{
	lcPiece* Piece;
	lcCamera* Camera;
	lcLight* Light;

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		Piece->Select (false, bFocusOnly, false);

	for (Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
	{
		Camera->Select(false, bFocusOnly, false);
		Camera->m_Target->Select(false, bFocusOnly, false);
	}

	for (Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
	{
		Light->Select(false, bFocusOnly, false);
		if (Light->GetTarget())
			Light->GetTarget()->Select(false, bFocusOnly, false);
	}
//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);
}

bool Project::GetSelectionCenter(Vector3& Center) const
{
	bool Selected = false;

	BoundingBox Box;
	Box.Reset();

	for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
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
	lcPiece* pPiece;
	lcCamera* pCamera;

	for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
		if (pPiece->IsFocused())
		{
			Position = pPiece->m_Position;
			return true;
		}

	for (pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (lcCamera*)pCamera->m_Next)
	{
		if (pCamera->IsEyeFocused())
		{
			Position = pCamera->m_Position;
			return true;
		}

		if (pCamera->IsTargetFocused())
		{
			Position = pCamera->m_TargetPosition;
			return true;
		}
	}

	// TODO: light

	Position = Vector3(0, 0, 0);

	return false;
}

// Returns the object that currently has focus.
lcObject* Project::GetFocusObject() const
{
	for (lcPiece* pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
	{
		if (pPiece->IsFocused())
			return pPiece;
	}

	for (lcCamera* pCamera = m_ActiveModel->m_Cameras; pCamera; pCamera = (lcCamera*)pCamera->m_Next)
	{
		if (pCamera->IsEyeFocused() || pCamera->IsTargetFocused())
			return pCamera;
	}

	// TODO: light

	return NULL;
}

// Find a good starting position/orientation relative to an existing piece.
void Project::GetPieceInsertPosition(lcPiece* OffsetPiece, Vector3& Position, Vector4& Rotation)
{
	Vector3 Dist(0, 0, 0);

	if (OffsetPiece)
		Dist[2] += OffsetPiece->m_PieceInfo->m_BoundingBox.m_Max[2];

	PieceInfo* Selection = g_App->m_PiecePreview->m_Selection;
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
	lcPiece* HitPiece = (lcPiece*)FindObjectFromPoint(MouseX, MouseY, true);

	if (HitPiece)
	{
		GetPieceInsertPosition(HitPiece, Position, Rotation);
		return;
	}

	// Try to hit the base grid.
	lcCamera* Camera = m_ActiveView->GetCamera();

	// Build the matrices.
	Matrix44 Projection = m_ActiveView->GetProjectionMatrix();

	Vector3 ClickPoints[2] = { Vector3((float)m_nDownX, (float)m_nDownY, 0.0f), Vector3((float)m_nDownX, (float)m_nDownY, 1.0f) };
	UnprojectPoints(ClickPoints, 2, Camera->m_WorldView, Projection, m_ActiveView->m_Viewport);

	Vector3 Intersection;
	if (LinePlaneIntersection(&Intersection, ClickPoints[0], ClickPoints[1], Vector4(0, 0, 1, 0)))
	{
		PieceInfo* Selection = g_App->m_PiecePreview->m_Selection;
		if (Selection)
			Intersection[2] -= Selection->m_BoundingBox.m_Min[2];

		SnapVector(Intersection);
		Position = Intersection;
		Rotation = Vector4(0, 0, 1, 0);
		return;
	}

	// Couldn't find a good position, so just place the piece somewhere near the camera.
	Position = UnprojectPoint(Vector3((float)m_nDownX, (float)m_nDownY, 0.9f), Camera->m_WorldView, Projection, m_ActiveView->m_Viewport);
	Rotation = Vector4(0, 0, 1, 0);
}

lcObject* Project::FindObjectFromPoint(int x, int y, bool PiecesOnly)
{
	Matrix44 Projection = m_ActiveView->GetProjectionMatrix();
	const Matrix44& ModelView = m_ActiveView->GetCamera()->m_WorldView;

	Vector3 Start = UnprojectPoint(Vector3((float)x, (float)y, 0.0f), ModelView, Projection, m_ActiveView->m_Viewport);
	Vector3 End = UnprojectPoint(Vector3((float)x, (float)y, 1.0f), ModelView, Projection, m_ActiveView->m_Viewport);

	lcClickLine ClickLine;
	ClickLine.Start = Start;
	ClickLine.End = End;
	ClickLine.Dist = FLT_MAX;
	ClickLine.Object = NULL;

	for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		if (Piece->IsVisible(m_ActiveModel->m_CurFrame))
			Piece->ClosestLineIntersect(ClickLine);

	if (!PiecesOnly)
	{
		for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
			if (Camera != m_ActiveView->GetCamera() && Camera->IsVisible())
				Camera->ClosestLineIntersect(ClickLine);

		for (lcLight* Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
			if (Light->IsVisible())
				Light->ClosestLineIntersect(ClickLine);
	}

	return (lcObject*)ClickLine.Object;
}

void Project::FindObjectsInBox(float x1, float y1, float x2, float y2, lcPtrArray<lcObject>& Objects)
{
	lcCamera* Camera = m_ActiveView->GetCamera();

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

	Matrix44 Projection = m_ActiveView->GetProjectionMatrix();
	UnprojectPoints(Corners, 6, Camera->m_WorldView, Projection, m_ActiveView->m_Viewport);

	// Build the box planes.
	Vector4 Planes[6];

	Planes[0] = Vector4(Normalize(Cross(Corners[4] - Corners[0], Corners[1] - Corners[0]))); // Left
	Planes[1] = Vector4(Normalize(Cross(Corners[5] - Corners[2], Corners[3] - Corners[2]))); // Right
	Planes[2] = Vector4(Normalize(Cross(Corners[3] - Corners[0], Corners[4] - Corners[0]))); // Top
	Planes[3] = Vector4(Normalize(Cross(Corners[1] - Corners[2], Corners[5] - Corners[2]))); // Bottom
	Planes[4] = Vector4(Normalize(Cross(Corners[1] - Corners[0], Corners[3] - Corners[0]))); // Front
	Planes[5] = Vector4(Normalize(Cross(Corners[1] - Corners[2], Corners[3] - Corners[2]))); // Back

	Planes[0][3] = -Dot3(Planes[0], Corners[0]);
	Planes[1][3] = -Dot3(Planes[1], Corners[5]);
	Planes[2][3] = -Dot3(Planes[2], Corners[0]);
	Planes[3][3] = -Dot3(Planes[3], Corners[5]);
	Planes[4][3] = -Dot3(Planes[4], Corners[0]);
	Planes[5][3] = -Dot3(Planes[5], Corners[5]);

	// Check if any objects are inside the volume.
	for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece != NULL; Piece = (lcPiece*)Piece->m_Next)
	{
		if (!Piece->IsVisible(m_ActiveModel->m_CurFrame))
			continue;

		if (Piece->IntersectsVolume(Planes, 6))
			Objects.Add(Piece);
	}

	for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera != NULL; Camera = (lcCamera*)Camera->m_Next)
	{
		if (!Camera->IsVisible())
			continue;

		if (Camera->IntersectsVolume(Planes, 6))
			Objects.Add(Camera);
	}

	for (lcLight* Light = m_ActiveModel->m_Lights; Light != NULL; Light = (lcLight*)Light->m_Next)
	{
		if (!Light->IsVisible())
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
					lcPtrArray<lcObject> Objects;
					FindObjectsInBox((float)m_nDownX, (float)m_nDownY, m_fTrack[0], m_fTrack[1], Objects);

					// Deselect old pieces.
					bool Control = Sys_KeyDown(KEY_CONTROL);
					SelectAndFocusNone(Control);

					// Select new pieces.
					for (int i = 0; i < Objects.GetSize(); i++)
					{
						if (Objects[i]->GetType() == LC_OBJECT_PIECE)
						{
							lcGroup* pGroup = ((lcPiece*)Objects[i])->GetTopGroup();
							if (pGroup != NULL)
							{
								for (lcPiece* pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
									if ((pPiece->IsVisible(m_ActiveModel->m_CurFrame)) &&
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
					Vector3((float)m_ActiveView->m_Viewport[2] / 2.0f, (float)m_ActiveView->m_Viewport[3] / 2.0f, 0.9f),
					Vector3((float)m_ActiveView->m_Viewport[2] / 2.0f, (float)m_ActiveView->m_Viewport[3] / 2.0f, 0.1f),
				};

				lcCamera* Camera = m_ActiveView->GetCamera();
				Matrix44 Projection = m_ActiveView->GetProjectionMatrix();
				UnprojectPoints(Points, 3, Camera->m_WorldView, Projection, m_ActiveView->m_Viewport);

				// Center camera.
				Vector3 Eye = Camera->m_Position;
				Eye = Eye + (Points[0] - Points[1]);

				Vector3 Target = Camera->m_TargetPosition;
				Target = Target + (Points[0] - Points[1]);

				// Zoom in/out.
				float RatioX = (Right - Left) / m_ActiveView->m_Viewport[2];
				float RatioY = (Top - Bottom) / m_ActiveView->m_Viewport[3];
				float ZoomFactor = -max(RatioX, RatioY) + 0.75f;

				Vector3 Dir = Points[1] - Points[2];
				Eye = Eye + Dir * ZoomFactor;
				Target = Target + Dir * ZoomFactor;

				// Change the camera and redraw.
				Camera->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, Eye, LC_CK_EYE);
				Camera->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, Target, LC_CK_TARGET);
				Camera->UpdatePosition(m_ActiveModel->m_CurFrame);

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

void Project::GetSnapIndex(int* SnapXY, int* SnapZ, int* SnapAngle) const
{
	*SnapXY = (m_nMoveSnap & 0xff);
	*SnapZ = ((m_nMoveSnap >> 8) & 0xff);

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

void Project::GetSnapDistance(float* SnapXY, float* SnapZ) const
{
	const float SnapXYTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 1.6f, 2.4f, 3.2f, 6.4f };
	const float SnapZTable[] = { 0.01f, 0.04f, 0.2f, 0.32f, 0.4f, 0.8f, 0.96f, 1.92f, 3.84f, 7.68f };

	int SXY, SZ, SA;
	GetSnapIndex(&SXY, &SZ, &SA);

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

		int SXY, SZ, SA;
		GetSnapIndex(&SXY, &SZ, &SA);

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

		if (Leftover[0] -SnapXY / 2 > 0.001f)
		{
			Leftover[0] -= SnapXY;
			i++;
		}
		else if (Leftover[0] + SnapXY / 2 < 0.001f)
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

		if (Leftover[1] - SnapXY / 2 > 0.001f)
		{
			Leftover[1] -= SnapXY;
			i++;
		}
		else if (Leftover[1] + SnapXY / 2 < 0.001f)
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

		if (Leftover[2] - SnapZ / 2 > 0.001f)
		{
			Leftover[2] -= SnapZ;
			i++;
		}
		else if (Leftover[2] + SnapZ / 2 < 0.001f)
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

	if (LengthSquared(Move) < 0.00001f)
		return false;

	// Transform the translation if we're in relative mode.
	if ((m_nSnap & LC_DRAW_GLOBAL_SNAP) == 0)
	{
		lcObject* Focus = GetFocusObject();

		if ((Focus != NULL) && Focus->IsPiece())
		{
			Move = Mul30(Move, ((lcPiece*)Focus)->m_ModelWorld);
		}
	}

	for (lcCamera* Camera = m_ActiveModel->m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
		if (Camera->IsSelected())
		{
			Camera->Move(m_ActiveModel->m_CurFrame, m_bAddKeys, Move);
			Camera->UpdatePosition(m_ActiveModel->m_CurFrame);
		}

	for (lcLight* Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
		if (Light->IsSelected())
		{
			Light->Move(m_ActiveModel->m_CurFrame, m_bAddKeys, Move);
			Light->UpdatePosition(m_ActiveModel->m_CurFrame);
		}

	for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		if (Piece->IsSelected())
		{
			Piece->Move(m_ActiveModel->m_CurFrame, m_bAddKeys, Move);
			Piece->UpdatePosition(m_ActiveModel->m_CurFrame);
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

	if (LengthSquared(Delta) < 0.001f)
		return false;

	BoundingBox Box;
	Box.Reset();

	lcPiece* Piece;
	lcPiece* Focus = NULL;
	int nSel = 0;

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
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
		Center = Focus->m_Position;
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

	for (Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
	{
		if (!Piece->IsSelected())
			continue;

		Vector3 Pos = Piece->m_Position;
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

			Piece->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, Pos, LC_PK_POSITION);
		}

		Rot = NewRotation;

		Piece->ChangeKey(m_ActiveModel->m_CurFrame, m_bAddKeys, Rot, LC_PK_ROTATION);
		Piece->UpdatePosition(m_ActiveModel->m_CurFrame);
	}

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
			if (m_ActiveModel->m_Pieces == NULL)
				break;

			lcPiece* pFocus = NULL, *pPiece;
			for (pFocus = m_ActiveModel->m_Pieces; pFocus; pFocus = (lcPiece*)pFocus->m_Next)
				if (pFocus->IsFocused())
					break;

			SelectAndFocusNone(false);

			if (pFocus == NULL)
			{
				if (bShift)
				{
					// Focus the last visible piece.
					for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
						if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
							pFocus = pPiece;
				}
				else
				{
					// Focus the first visible piece.
					for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
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
					lcPiece* pBest = pPiece = pFocus;
					for (;;)
					{
						if (pPiece->IsVisible(m_ActiveModel->m_CurFrame))
							pBest = pPiece;

						if (pPiece->m_Next != NULL)
						{
							if (pPiece->m_Next == pFocus)
								break;
							else
								pPiece = (lcPiece*)pPiece->m_Next;
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
								pPiece = (lcPiece*)pPiece->m_Next;
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
        pFocus->Select (true, true, false);
        lcGroup* pGroup = pFocus->GetTopGroup();
        if (pGroup != NULL)
        {
          for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
            if ((pPiece->IsVisible(m_ActiveModel->m_CurFrame)) &&
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

					Vector3 Pts[3] = { Vector3(5.0f, 5.0f, 0.1f), Vector3(10.0f, 5.0f, 0.1f), Vector3(5.0f, 10.0f, 0.1f) };
					UnprojectPoints(Pts, 3, m_ActiveView->GetCamera()->m_WorldView, m_ActiveView->GetProjectionMatrix(), m_ActiveView->m_Viewport);

					float ax, ay;
					Vector3 vx = Normalize(Vector3((Pts[1][0] - Pts[0][0]), (Pts[1][1] - Pts[0][1]), 0));//Pts[1][2] - Pts[0][2] };
					Vector3 x(1, 0, 0);
					ax = acosf(Dot3(vx, x));

					Vector3 vy = Normalize(Vector3((Pts[2][0] - Pts[0][0]), (Pts[2][1] - Pts[0][1]), 0));//Pts[2][2] - Pts[0][2] };
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

void Project::BeginPieceDrop()
{
	m_PreviousAction = m_nCurAction;

	StartTracking(LC_TRACK_LEFT);
	SetAction(LC_ACTION_INSERT);
}

void Project::OnTouch(View* view, LC_TOUCH_PHASE Phase, int TapCount, int x, int y, int PrevX, int PrevY)
{
	if (Phase == LC_TOUCH_BEGAN)
	{
		// Reset action if it's the first touch.
		if (m_TouchState.GetSize() == 0)
			SetAction(LC_ACTION_SELECT);

		// Add touch to the array.
		if (m_nCurAction == LC_ACTION_SELECT)
		{
			TouchState Touch;
			Touch.Phase = Phase;
			Touch.TapCount = TapCount;
			Touch.x = x;
			Touch.y = y;
			Touch.StartX = x;
			Touch.StartY = y;
			m_TouchState.Add(Touch);
		}
		else
		{
			// TODO: cancel action
		}
	}
	else if (Phase == LC_TOUCH_ENDED)
	{
		// Remove touch from the array.
		int TouchIndex = FindTouchIndex(x, y);
		if (TouchIndex < 0)
			return;

		m_TouchState.RemoveIndex(TouchIndex);

		switch (m_nCurAction)
		{
			case LC_ACTION_SELECT:
				break;
//				LC_ACTION_INSERT,
//				LC_ACTION_LIGHT,
//				LC_ACTION_SPOTLIGHT,
//				LC_ACTION_CAMERA,
//				LC_ACTION_MOVE,
//				LC_ACTION_ROTATE,
//				LC_ACTION_ERASER,
//				LC_ACTION_PAINT,
//				LC_ACTION_ZOOM,
//				LC_ACTION_ZOOM_REGION,
			case LC_ACTION_PAN:
				if (m_TouchState.GetSize() == 0)
					OnLeftButtonUp(view, x, y, false, false);
				break;
//				LC_ACTION_ROTATE_VIEW,
//				LC_ACTION_ROLL,
//				LC_ACTION_CURVE
		}
	}
	else if (Phase == LC_TOUCH_MOVED)
	{
		int TouchIndex = FindTouchIndex(PrevX, PrevY);

		if (TouchIndex < 0)
			return;
		
		m_TouchState[TouchIndex].x = x;
		m_TouchState[TouchIndex].y = y;
		const int MinTouchDist = 10;

		switch (m_nCurAction)
		{
			case LC_ACTION_SELECT:
				if (ABS(m_TouchState[TouchIndex].StartX - x) + ABS(m_TouchState[TouchIndex].StartY - y) < MinTouchDist)
					break;

				if (0)//AnyObjectSelected())
				{
				}
				else
				{
					if (m_TouchState.GetSize() == 1)
					{
						SetAction(LC_ACTION_PAN);
						OnLeftButtonDown(view, x, y, false, false);
					}
				}
				break;

//				LC_ACTION_INSERT,
//				LC_ACTION_LIGHT,
//				LC_ACTION_SPOTLIGHT,
//				LC_ACTION_CAMERA,
//				LC_ACTION_MOVE,
//				LC_ACTION_ROTATE,
//				LC_ACTION_ERASER,
//				LC_ACTION_PAINT,
//				LC_ACTION_ZOOM,
//				LC_ACTION_ZOOM_REGION,
			case LC_ACTION_PAN:
				OnMouseMove(view, x, y, false, false);
				break;
//				LC_ACTION_ROTATE_VIEW,
//				LC_ACTION_ROLL,
//				LC_ACTION_CURVE
		};
	}
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

	Matrix44 ProjectionMatrix = m_ActiveView->GetProjectionMatrix();
	Vector3 point = UnprojectPoint(Vector3((float)x, (float)y, 0.9f), m_ActiveView->GetCamera()->m_WorldView, ProjectionMatrix, m_ActiveView->m_Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	switch (m_nCurAction)
	{
		case LC_ACTION_SELECT:
		case LC_ACTION_ERASER:
		case LC_ACTION_PAINT:
		{
			lcObject* Object = FindObjectFromPoint(x, y);

			if (m_nCurAction == LC_ACTION_SELECT) 
			{
				if (Object != NULL)
				{
					switch (Object->GetType ())
					{
						case LC_OBJECT_PIECE:
						{
							lcPiece* pPiece = (lcPiece*)Object;
							lcGroup* pGroup = pPiece->GetTopGroup();
							bool bFocus = pPiece->IsFocused ();

							SelectAndFocusNone (bControl);

							// if a piece has focus deselect it, otherwise set the focus
							pPiece->Select (!bFocus, !bFocus, false);

							if (pGroup != NULL)
								for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
									if (pPiece->GetTopGroup() == pGroup)
										pPiece->Select (!bFocus, false, false);
						} break;

						case LC_OBJECT_CAMERA:
						case LC_OBJECT_CAMERA_TARGET:
						case LC_OBJECT_LIGHT:
						case LC_OBJECT_LIGHT_TARGET:
						{
							SelectAndFocusNone (bControl);
							Object->Select (true, true, bControl);
						} break;
					}
				}
				else
					SelectAndFocusNone (bControl);

				UpdateSelection();
				UpdateAllViews();
				SystemUpdateFocus(Object);

				StartTracking(LC_TRACK_START_LEFT);
			}

			if ((m_nCurAction == LC_ACTION_ERASER) && (Object != NULL))
			{
				switch (Object->GetType ())
				{
					case LC_OBJECT_PIECE:
					{
						lcPiece* pPiece = (lcPiece*)Object;
						m_ActiveModel->RemovePiece(pPiece);
						delete pPiece;
//						CalculateStep();
						RemoveEmptyGroups();
					} break;

					case LC_OBJECT_CAMERA:
					case LC_OBJECT_CAMERA_TARGET:
					{
						lcCamera* pCamera;
						if (Object->GetType () == LC_OBJECT_CAMERA)
							pCamera = (lcCamera*)Object;
						else
							pCamera = ((CameraTarget*)Object)->m_Parent;
						bool bCanDelete = pCamera->IsUser();

						for (int i = 0; i < m_ViewList.GetSize(); i++)
							if (pCamera == m_ViewList[i]->GetCamera())
								bCanDelete = false;

						if (bCanDelete)
						{
							lcCamera* pPrev;
							for (pPrev = m_ActiveModel->m_Cameras; pPrev; pPrev = (lcCamera*)pPrev->m_Next)
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
			}

			if ((m_nCurAction == LC_ACTION_PAINT) && (Object != NULL) && 
				(Object->GetType() == LC_OBJECT_PIECE))
			{
				lcPiece* pPiece = (lcPiece*)Object;

				if (pPiece->m_Color != g_App->m_SelectedColor)
				{
					pPiece->m_Color = g_App->m_SelectedColor;

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

				GetPieceInsertPosition(x, y, Pos, Rot);

				lcPiece* pPiece = new lcPiece(g_App->m_PiecePreview->m_Selection);
				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_ActiveModel->m_CurFrame, g_App->m_SelectedColor);

				pPiece->ChangeKey(m_ActiveModel->m_CurFrame, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_ActiveModel->m_CurFrame);

				SelectAndFocusNone(false);
				pPiece->SetUniqueName(m_ActiveModel->m_Pieces, pPiece->m_PieceInfo->m_strDescription);
				m_ActiveModel->AddPiece(pPiece);
				pPiece->Select (true, true, false);
				UpdateSelection();
				SystemPieceComboAdd(g_App->m_PiecePreview->m_Selection->m_strDescription);
				SystemUpdateFocus(pPiece);

				if (m_nSnap & LC_DRAW_MOVE)
					SetAction(LC_ACTION_MOVE);
			}
			else if (m_nCurAction == LC_ACTION_LIGHT)
			{
				GLint max;
				int count = 0;
				lcLight *pLight;

				glGetIntegerv (GL_MAX_LIGHTS, &max);
				for (pLight = m_ActiveModel->m_Lights; pLight; pLight = (lcLight*)pLight->m_Next)
					count++;

				if (count == max)
					break;

				pLight = new lcLight(m_fTrack[0], m_fTrack[1], m_fTrack[2]);

				SelectAndFocusNone (false);

				pLight->SetUniqueName(m_ActiveModel->m_Lights, "Light");
				pLight->m_Next = m_ActiveModel->m_Lights;
				m_ActiveModel->m_Lights = pLight;
				SystemUpdateFocus (pLight);
				pLight->Select (true, true, false);
				UpdateSelection();
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
			lcLight* Light;

			// TODO: Warn user that the maximum light count was reached but still add the light anyway.
			glGetIntegerv(GL_MAX_LIGHTS, &max);
			for (Light = m_ActiveModel->m_Lights; Light; Light = (lcLight*)Light->m_Next)
				count++;

			if (count == max)
				break;

			Vector3 tmp = UnprojectPoint(Vector3(x+1.0f, y-1.0f, 0.9f), m_ActiveView->GetCamera()->m_WorldView, ProjectionMatrix, m_ActiveView->m_Viewport);

			SelectAndFocusNone(false);
			StartTracking(LC_TRACK_START_LEFT);
			Light = new lcLight(m_fTrack[0], m_fTrack[1], m_fTrack[2], (float)tmp[0], (float)tmp[1], (float)tmp[2]);
			Light->GetTarget()->Select (true, true, false);
			Light->m_Next = m_ActiveModel->m_Lights;
			Light->SetUniqueName(m_ActiveModel->m_Lights, "Light");
			m_ActiveModel->m_Lights = Light;
			UpdateSelection();
			UpdateAllViews();
			SystemUpdateFocus(Light);
		} break;

		case LC_ACTION_CAMERA:
		{
			Vector3 tmp = UnprojectPoint(Vector3(x+1.0f, y-1.0f, 0.9f), m_ActiveView->GetCamera()->m_WorldView, ProjectionMatrix, m_ActiveView->m_Viewport);
			SelectAndFocusNone(false);
			StartTracking(LC_TRACK_START_LEFT);

			lcCamera* pCamera = new lcCamera(Vector3(m_fTrack[0], m_fTrack[1], m_fTrack[2]), tmp);
			pCamera->SetUniqueName(m_ActiveModel->m_Cameras, "Camera");
			m_ActiveModel->AddCamera(pCamera);
			pCamera->m_Target->Select (true, true, false);
			UpdateSelection();
			UpdateAllViews();
			SystemUpdateFocus(pCamera);
		} break;

		case LC_ACTION_MOVE:
		{
			if (m_ActiveModel->AnyObjectsSelected())
			{
				StartTracking(LC_TRACK_START_LEFT);
				m_OverlayDelta = Vector3(0.0f, 0.0f, 0.0f);
				m_MouseSnapLeftover = Vector3(0.0f, 0.0f, 0.0f);
			}
		} break;

		case LC_ACTION_ROTATE:
		{
			for (lcPiece* Piece = m_ActiveModel->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
			{
				if (Piece->IsSelected())
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
	if (SetActiveView(view))
		return;

	// todo: check if this needs to be done here.
	Vector3 point = UnprojectPoint(Vector3((float)x, (float)y, 0.9f), m_ActiveView->GetCamera()->m_WorldView, m_ActiveView->GetProjectionMatrix(), m_ActiveView->m_Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

	lcObject* Object = FindObjectFromPoint(x, y);

    SelectAndFocusNone(bControl);

	if (Object != NULL)
      switch (Object->GetType ())
      {
        case LC_OBJECT_PIECE:
        {
          lcPiece* pPiece = (lcPiece*)Object;
          pPiece->Select (true, true, false);
          lcGroup* pGroup = pPiece->GetTopGroup();

          if (pGroup != NULL)
            for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
              if (pPiece->GetTopGroup() == pGroup)
                pPiece->Select (true, false, false);
        } break;

        case LC_OBJECT_CAMERA:
        case LC_OBJECT_CAMERA_TARGET:
        case LC_OBJECT_LIGHT:
        case LC_OBJECT_LIGHT_TARGET:
        {
          Object->Select (true, true, bControl);
        } break;
      }

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

				lcPiece* pPiece = new lcPiece(g_App->m_PiecePreview->m_Selection);
				pPiece->Initialize(Pos[0], Pos[1], Pos[2], m_ActiveModel->m_CurFrame, g_App->m_SelectedColor);

				pPiece->ChangeKey(m_ActiveModel->m_CurFrame, false, Rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(m_ActiveModel->m_CurFrame);

				SelectAndFocusNone(false);
				pPiece->SetUniqueName(m_ActiveModel->m_Pieces, pPiece->m_PieceInfo->m_strDescription);
				m_ActiveModel->AddPiece(pPiece);
				pPiece->Select (true, true, false);
				UpdateSelection();
				SystemPieceComboAdd(g_App->m_PiecePreview->m_Selection->m_strDescription);
				SystemUpdateFocus(pPiece);

				if (m_nSnap & LC_DRAW_MOVE)
					SetAction(LC_ACTION_MOVE);
				else
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
	if (StopTracking(false))
		return;

	if (SetActiveView(view))
		return;

	m_nDownX = x;
	m_nDownY = y;
	m_bTrackCancel = false;

	Vector3 point = UnprojectPoint(Vector3((float)x, (float)y, 0.9f), m_ActiveView->GetCamera()->m_WorldView, m_ActiveView->GetProjectionMatrix(), m_ActiveView->m_Viewport);
	m_fTrack[0] = point[0]; m_fTrack[1] = point[1]; m_fTrack[2] = point[2];

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

	Vector3 tmp = UnprojectPoint(Vector3((float)x, (float)y, 0.9f), m_ActiveView->GetCamera()->m_WorldView, m_ActiveView->GetProjectionMatrix(), m_ActiveView->m_Viewport);
	float ptx, pty, ptz;
	ptx = tmp[0]; pty = tmp[1]; ptz = tmp[2];

	switch (m_nCurAction)
	{
		case LC_ACTION_SELECT:
		{
			int ptx = x, pty = y;

			if (ptx >= m_ActiveView->m_Viewport[0] + m_ActiveView->m_Viewport[2])
				ptx = m_ActiveView->m_Viewport[0] + m_ActiveView->m_Viewport[2] - 1;
			else if (ptx <= m_ActiveView->m_Viewport[0])
				ptx = m_ActiveView->m_Viewport[0] + 1;

			if (pty >= m_ActiveView->m_Viewport[1] + m_ActiveView->m_Viewport[3])
				pty = m_ActiveView->m_Viewport[1] + m_ActiveView->m_Viewport[3] - 1;
			else if (pty <= m_ActiveView->m_Viewport[1])
				pty = m_ActiveView->m_Viewport[1] + 1;

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
			float mouse = 10.0f/(21 - g_App->m_MouseSensitivity);
			Vector3 Delta((ptx - m_fTrack[0])*mouse, (pty - m_fTrack[1])*mouse, (ptz - m_fTrack[2])*mouse);

			m_fTrack[0] = ptx;
			m_fTrack[1] = pty;
			m_fTrack[2] = ptz;
			
			lcLight* Light = m_ActiveModel->m_Lights;

			Light->Move(1, false, Delta);
			Light->UpdatePosition(1);

			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;

		case LC_ACTION_CAMERA:
		{
			float mouse = 10.0f/(21 - g_App->m_MouseSensitivity);
			Vector3 Delta((ptx - m_fTrack[0])*mouse, (pty - m_fTrack[1])*mouse, (ptz - m_fTrack[2])*mouse);

			m_fTrack[0] = ptx;
			m_fTrack[1] = pty;
			m_fTrack[2] = ptz;
			
			lcCamera* Camera = m_ActiveModel->m_Cameras;
			while (Camera->m_Next != NULL)
				Camera = (lcCamera*)Camera->m_Next;

			Camera->Move(1, false, Delta);
			Camera->UpdatePosition(1);

			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;

		case LC_ACTION_MOVE:
		{
			// Check if the mouse moved since the last update.
			if ((x == m_nDownX) && (y == m_nDownY))
				break;

			float Sensitivity = 0.25f / (LC_MAX_MOUSE_SENSITIVITY+1 - g_App->m_MouseSensitivity);
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

					if ((Focus != NULL) && Focus->IsPiece())
					{
						Axis1 = Mul30(Dir1, ((lcPiece*)Focus)->m_ModelWorld);
						Axis2 = Mul30(Dir2, ((lcPiece*)Focus)->m_ModelWorld);
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

				MoveX *= (float)(x - m_nDownX) * Sensitivity;
				MoveY *= (float)(y - m_nDownY) * Sensitivity;

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

					MoveX = ScreenX * (float)(x - m_nDownX) * Sensitivity;
					MoveY = ScreenY * (float)(y - m_nDownY) * Sensitivity;

					TotalMove = MoveX + MoveY + m_MouseSnapLeftover;
				}
				else
				{
					Vector3 MoveZ;

					MoveZ = ScreenZ * (float)(y - m_nDownY) * Sensitivity;

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
			float Sensitivity = 36.0f / (LC_MAX_MOUSE_SENSITIVITY+1 - g_App->m_MouseSensitivity);
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

				MoveX *= (float)(x - m_nDownX) * Sensitivity;
				MoveY *= (float)(y - m_nDownY) * Sensitivity;

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

					MoveX = ScreenX * (float)(x - m_nDownX) * Sensitivity;
					MoveY = ScreenY * (float)(y - m_nDownY) * Sensitivity;

					Delta = MoveX + MoveY + m_MouseSnapLeftover;
				}
				else
				{
					Vector3 MoveZ;

					MoveZ = ScreenZ * (float)(y - m_nDownY) * Sensitivity;

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

			m_ActiveView->GetCamera()->Zoom(m_ActiveModel->m_CurFrame, m_bAddKeys, x - m_nDownX, y - m_nDownY);
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

			m_ActiveView->GetCamera()->Pan(m_ActiveModel->m_CurFrame, m_bAddKeys, x - m_nDownX, y - m_nDownY);
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
				// Create a new camera if the user is trying to rotate a side camera.
				Camera = new lcCamera(Camera);
				Camera->SetUniqueName(m_ActiveModel->m_Cameras, "Camera");
				m_ActiveModel->AddCamera(Camera);

				m_ActiveView->SetCamera(Camera);
				SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
				SystemUpdateCurrentCamera(NULL, Camera, m_ActiveModel->m_Cameras);
			}

			Camera->Rotate(m_ActiveModel->m_CurFrame, m_bAddKeys, x - m_nDownX, y - m_nDownY);

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
				// Create a new camera if the user is trying to rotate a side camera.
				Camera = new lcCamera(Camera);
				Camera->SetUniqueName(m_ActiveModel->m_Cameras, "Camera");
				m_ActiveModel->AddCamera(Camera);

				m_ActiveView->SetCamera(Camera);
				SystemUpdateCameraMenu(m_ActiveModel->m_Cameras);
				SystemUpdateCurrentCamera(NULL, Camera, m_ActiveModel->m_Cameras);
			}

			switch (m_OverlayMode)
			{
				case LC_OVERLAY_XYZ:
					Camera->Orbit(m_ActiveModel->m_CurFrame, m_bAddKeys, x - m_nDownX, y - m_nDownY);
					break;

				case LC_OVERLAY_X:
					Camera->Orbit(m_ActiveModel->m_CurFrame, m_bAddKeys, x - m_nDownX, 0);
					break;

				case LC_OVERLAY_Y:
					Camera->Orbit(m_ActiveModel->m_CurFrame, m_bAddKeys, 0, y - m_nDownY);
					break;

				case LC_OVERLAY_Z:
					Camera->Roll(m_ActiveModel->m_CurFrame, m_bAddKeys, x - m_nDownX, y - m_nDownY);
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

			m_ActiveView->GetCamera()->Roll(m_ActiveModel->m_CurFrame, m_bAddKeys, x - m_nDownX, y - m_nDownY);
			m_nDownX = x;
			SystemUpdateFocus(NULL);
			UpdateAllViews();
		} break;
		/*
    case LC_ACTION_CURVE:
    {
      float mouse = 10.0f/(21 - g_App->m_MouseSensitivity);
      float dx = (ptx - m_fTrack[0])*mouse;
      float dy = (pty - m_fTrack[1])*mouse;
      float dz = (ptz - m_fTrack[2])*mouse;
      lcObject *pObj = NULL;
      Curve *pCurve;

      m_fTrack[0] = ptx;
      m_fTrack[1] = pty;
      m_fTrack[2] = ptz;

      for (pObj = m_pObjects; pObj != NULL; pObj = pObj->m_Next)
	if (pObj->IsSelected ())
	  break;

      if (pObj == NULL)
	break;
      pCurve = (Curve*)pObj;

      pCurve->Move (1, false, dx, dy, dz);
      pCurve->UpdatePosition(1);

      SystemUpdateFocus(NULL);
      UpdateAllViews();
    } break;
                */
	}
}

// Check if the mouse is over a different area of the overlay and redraw it.
void Project::MouseUpdateOverlays(int x, int y)
{
	const float OverlayScale = m_ActiveView->m_OverlayScale;

	if (m_nCurAction == LC_ACTION_MOVE)
	{
		const float OverlayMoveArrowSize = 1.5f;

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

			if ((Focus != NULL) && Focus->IsPiece())
			{
				const Matrix44& Mat = ((lcPiece*)Focus)->m_ModelWorld;

				for (int i = 1; i < 4; i++)
					Points[i] = Mul30(Points[i], Mat);
			}
		}

		int i, Mode = -1;
		Vector3 Pt((float)x, (float)y, 0);

		for (i = 1; i < 4; i++)
			Points[i] += Points[0];

		ProjectPoints(Points, 4, m_ActiveView->GetCamera()->m_WorldView, m_ActiveView->GetProjectionMatrix(), m_ActiveView->m_Viewport);

		// Check if the mouse is over an arrow.
		for (i = 1; i < 4; i++)
		{
			Vector3 Line = Points[i] - Points[0];
			Vector3 Vec = Pt - Points[0];

			float u = Dot3(Vec, Line) / LengthSquared(Line);

			// Point is outside the line segment.
			if (u < 0.0f || u > 1.0f)
				continue;

			// Closest point in the line segment to the mouse.
			Vector3 Closest = Points[0] + u * Line;

			if (LengthSquared(Closest - Pt) < 100.0f)
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
		Matrix44 Projection = m_ActiveView->GetProjectionMatrix();
		const Matrix44& ModelView = m_ActiveView->GetCamera()->m_WorldView;

		Vector3 SegStart = UnprojectPoint(Vector3((float)x, (float)y, 1.0f), ModelView, Projection, m_ActiveView->m_Viewport);
		Vector3 SegEnd = UnprojectPoint(Vector3((float)x, (float)y, 0.0f), ModelView, Projection, m_ActiveView->m_Viewport);
		Vector3 Center(m_OverlayCenter[0], m_OverlayCenter[1], m_OverlayCenter[2]);

		Vector3 Line = SegEnd - SegStart;
		Vector3 Vec = Center - SegStart;

		float u = Dot3(Vec, Line) / LengthSquared(Line);

		// Closest point in the line to the mouse.
		Vector3 Closest = SegStart + u * Line;

		int Mode = -1;
		float Distance = Length(Closest - Center);
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

						if ((Focus != NULL) && Focus->IsPiece())
						{
							Matrix44 WorldModel = RotTranInverse(((lcPiece*)Focus)->m_ModelWorld);
							Dist = Mul30(Dist, WorldModel);
						}
					}

					// Check if we're close enough to one of the axis.
					Dist = Normalize(Dist);

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

// VRML97 and X3DV export is very similar, cause X3D is the successor of VRML97
// therefore a lot of the export routines can be reused
// the member variable VRMLdialect used with the following enum has this information

enum 
{
	VRML97,
	X3DV_WITH_RIGID_BODY_PHYSICS
};

void Project::exportVRML97File(char *filename)
{
	exportVRMLFile(filename, VRML97);
}

void Project::exportX3DVFile(char *filename)
{
	exportVRMLFile(filename, X3DV_WITH_RIGID_BODY_PHYSICS);
}

void Project::writeIndent(FILE* stream)
{
	for (int i = 0; i < indent; i++)
		fprintf(stream, " ");
}

#define INDENT_INC 2

// routines to write VRML/X3DV shape related commands
// for details see 
// http://www.web3d.org/x3d/specifications/vrml/ISO-IEC-14772-VRML97/part1/
// http://www.web3d.org/x3d/specifications/ISO-IEC-19775-X3DAbstractSpecification/Part01/Architecture.html

void Project::writeVRMLShapeBegin(FILE *stream, unsigned long currentColor, bool blackLines)
{
	// http://www.web3d.org/x3d/specifications/ISO-IEC-19775-X3DAbstractSpecification_Revision1_to_Part1/Part01/components/rigid_physics.html#CollidableShape

	if (VRMLdialect == X3DV_WITH_RIGID_BODY_PHYSICS)
	{
		numFaceColors = 0;
		faceColors = (int *) malloc(1);

		writeIndent(stream);
		fprintf(stream, "DEF CollidableShape%d CollidableShape {\n", numDEF++);
		indent += INDENT_INC;
		writeIndent(stream);
		fprintf(stream, "shape ");
	}
	else
		writeIndent(stream);

	// http://www.web3d.org/x3d/specifications/vrml/ISO-IEC-14772-VRML97/part1/nodesRef.html#Shape

	fprintf(stream, "Shape {\n");
	indent += INDENT_INC;

	// http://www.web3d.org/x3d/specifications/vrml/ISO-IEC-14772-VRML97/part1/nodesRef.html#Appearance
	// http://www.web3d.org/x3d/specifications/vrml/ISO-IEC-14772-VRML97/part1/nodesRef.html#Material
	writeIndent(stream);
	fprintf(stream, "appearance Appearance {\n");
	indent += INDENT_INC;
	writeIndent(stream);
	fprintf(stream, "material Material {\n");
	indent += INDENT_INC;
	if (blackLines)
	{
		writeIndent(stream);
		fprintf(stream, "diffuseColor 0 0 0\n");
		writeIndent(stream);
		fprintf(stream, "emissiveColor 0 0 0\n");
	}
	else
	{
		writeIndent(stream);
		fprintf(stream, "diffuseColor %g %g %g\n", g_ColorList[currentColor].Value[0], g_ColorList[currentColor].Value[1], g_ColorList[currentColor].Value[2]);
		if (currentColor > 13 && currentColor < 22) 
		{
			writeIndent(stream);
			fprintf(stream, "transparency 0.5\n");
		}
	}
	indent -= INDENT_INC;
	writeIndent(stream);
	fprintf(stream, "}\n");                                
	indent -= INDENT_INC;
	writeIndent(stream);
	fprintf(stream, "}\n");                                
	
	if (blackLines)
	{
		// http://www.web3d.org/x3d/specifications/vrml/ISO-IEC-14772-VRML97/part1/nodesRef.html#IndexedLineSet
		writeIndent(stream);
		fprintf(stream, "geometry IndexedLineSet {\n");
		indent += INDENT_INC;
	}
	else
	{
		// http://www.web3d.org/x3d/specifications/ISO-IEC-19775-X3DAbstractSpecification/Part01/components/rendering.html#TriangleSet
		// http://www.web3d.org/x3d/specifications/vrml/ISO-IEC-14772-VRML97/part1/nodesRef.html#IndexedFaceSet
		writeIndent(stream);
		if (VRMLdialect == X3DV_WITH_RIGID_BODY_PHYSICS)
			fprintf(stream, "geometry TriangleSet {\n");
		else
			fprintf(stream, "geometry IndexedFaceSet {\n");
		indent += INDENT_INC;
		writeIndent(stream);
		fprintf(stream, "solid FALSE\n");
		if (VRMLdialect != X3DV_WITH_RIGID_BODY_PHYSICS)
		{
			writeIndent(stream);
			fprintf(stream, "creaseAngle 0.79\n");
		}
	}
}

void Project::writeVRMLShapeEnd(FILE *stream)
{
	indent -= INDENT_INC;
	writeIndent(stream);
	fprintf(stream, "}\n");
	indent -= INDENT_INC;
	writeIndent(stream);
	fprintf(stream, "}\n");
}

// search for vertex (vertex[0], vertex[1], vertex[2]) in coords and give
// back index (-1 if not found)

int Project::searchForVertex(float* vertex) 
{
	for (int i = 0; i < numCoords; i++)
		if (coords[i * 3] == vertex[0])
			if (coords[i * 3 + 1] == vertex[1])
				if (coords[i * 3 + 2] == vertex[2])
					return i;
	return -1;
}

// routines to collect VRML indexed polygon mesh data or X3DV triangle mesh data

template<class type> void Project::generateMeshData(type* info, float *pos, lcPiece* pPiece, int numVertices, int currentColor)
{
	Matrix matrix(pPiece->m_AxisAngle, pPiece->m_Position);

	bool rigidBody = (VRMLdialect == X3DV_WITH_RIGID_BODY_PHYSICS);

	if (rigidBody)
	{
		// IndexedLineSet not supported by xj3d RigidBody node
		if (numVertices == 2)
			return;

		int maxJ = 1;
		// write 2 triangles instead of 1 quad
		if (numVertices == 4)
			maxJ = 2;

		lcMesh* Mesh = pPiece->m_PieceInfo->m_Mesh;
		float* verts = (float*)Mesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);

		for (int j = 0; j < maxJ; j++) 
		{
			for (int i = 0; i < 3; i++)
			{
				int index = i;
				if (j == 1)
				{
					switch (i) {
						case 0:
							index = 2;
							break;
						case 1:
							index = 0;
							break;
						case 2:
							index = 3;
							break;
					}
				}
				float *localVertex = &verts[info[index] * 3];
				float vertex[3];
				matrix.TransformPoint(vertex, localVertex);
				coords = (float *) realloc(coords, (numCoords + 1) * 3 * sizeof(float));
				coords[numCoords * 3 + 0] = vertex[1] - pos[1];
				coords[numCoords * 3 + 1] = vertex[2] - pos[2];
				coords[numCoords * 3 + 2] = vertex[0] - pos[0];
				numCoords++;
			}
			faceColors = (int *) realloc(faceColors, (numFaceColors + 1) * sizeof(int));
			faceColors[numFaceColors] = currentColor;
			numFaceColors++;

		}
		Mesh->m_VertexBuffer->UnmapBuffer();
		return;
	}			

	lcMesh* Mesh = pPiece->m_PieceInfo->m_Mesh;
	float* verts = (float*)Mesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	for (int i = 0; i < numVertices; i++)
	{
		float *localVertex = &verts[info[i] * 3];
		int index = searchForVertex(localVertex);
		if (index == -1)
		{
			float vertex[3];
			if (rigidBody)
				matrix.TransformPoint(vertex, localVertex);
			else
			{
				vertex[0] = localVertex[0];
				vertex[1] = localVertex[1];
				vertex[2] = localVertex[2];
			}
			coords = (float *) realloc(coords, (numCoords + 1) * 3 * sizeof(float));
			coords[numCoords * 3 + 0] = vertex[1] - (rigidBody ? pos[1] : 0);
			coords[numCoords * 3 + 1] = vertex[2] - (rigidBody ? pos[2] : 0);
			coords[numCoords * 3 + 2] = vertex[0] - (rigidBody ? pos[0] : 0);
			index = numCoords;
			numCoords++;
		}
		coordIndices = (int *) realloc(coordIndices, (numCoordIndices + 1) * sizeof(int));
		coordIndices[numCoordIndices] = index;
		numCoordIndices++;
	}
	coordIndices = (int *) realloc(coordIndices, (numCoordIndices + 1) * sizeof(int));
	coordIndices[numCoordIndices] = -1;
	numCoordIndices++;
	faceColors = (int *) realloc(faceColors, (numFaceColors + 1) * sizeof(int));
	faceColors[numFaceColors] = currentColor;
	numFaceColors++;

	Mesh->m_VertexBuffer->UnmapBuffer();
}

// write collected mesh data

void Project::writeVRMLShapeMeshBegin(FILE *stream)
{
	writeIndent(stream);

	fprintf(stream, "coord Coordinate {\n");
	indent += INDENT_INC;
	writeIndent(stream);
	fprintf(stream, "point [\n");
	indent += INDENT_INC;
}

void Project::writeVRMLShapeMeshData(FILE *stream)
{
	for (int i = 0; i < numCoords; i++) 
	{
		writeIndent(stream);
		fprintf(stream, "%f %f %f\n", coords[i * 3] * VRMLScale, coords[i * 3 + 1] * VRMLScale, coords[i * 3 + 2] * VRMLScale);
	}
}

void Project::writeVRMLShapeMeshEnd(FILE *stream)
{
	indent -= INDENT_INC;
	writeIndent(stream);
	fprintf(stream, "]\n");
	indent -= INDENT_INC;
	writeIndent(stream);
	fprintf(stream, "}\n");

	
	if (VRMLdialect == X3DV_WITH_RIGID_BODY_PHYSICS)
	{
		writeIndent(stream);
		fprintf(stream, "color ColorRGBA {\n"); 
		indent += INDENT_INC;		
		writeIndent(stream);
		fprintf(stream, "color [\n"); 
		indent += INDENT_INC;		

		for (int i = 0; i < numFaceColors; i++)
		{
			int currentColor = faceColors[i];
			writeIndent(stream);
			fprintf(stream, "%g %g %g %g\n", g_ColorList[currentColor].Value[0], g_ColorList[currentColor].Value[1], g_ColorList[currentColor].Value[2], LC_COLOR_TRANSLUCENT(currentColor) ? 0.5f : 1.0f);
			writeIndent(stream);
			fprintf(stream, "%g %g %g %g\n", g_ColorList[currentColor].Value[0], g_ColorList[currentColor].Value[1], g_ColorList[currentColor].Value[2], LC_COLOR_TRANSLUCENT(currentColor) ? 0.5f : 1.0f);
			writeIndent(stream);
			fprintf(stream, "%g %g %g %g\n", g_ColorList[currentColor].Value[0], g_ColorList[currentColor].Value[1], g_ColorList[currentColor].Value[2], LC_COLOR_TRANSLUCENT(currentColor) ? 0.5f : 1.0f);
		}

		indent -= INDENT_INC;		
		writeIndent(stream);
		fprintf(stream, "]\n"); 

		indent -= INDENT_INC;
		writeIndent(stream);
		fprintf(stream, "}\n");
	}
	else
	{
		writeIndent(stream);
		fprintf(stream, "coordIndex [\n"); 
		indent += INDENT_INC;

		for (int i = 0; i < numCoordIndices; i ++) 
		{
			writeIndent(stream);
			fprintf(stream, "%d\n", coordIndices[i]);
		}

		indent -= INDENT_INC;
		writeIndent(stream);
		fprintf(stream, "]\n");
	}
}

// routine to run through leoCADs internal data space, collect and write data

template<class type> void Project::writeVRMLShapes(type color, FILE *stream, int coordinateCounter, lcPiece* pPiece, unsigned short group, float *pos, bool beginAndEnd)
{ 
	lcMesh* Mesh = pPiece->m_PieceInfo->m_Mesh;
	void* indices = Mesh->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	for (int SectionIndex = 0; SectionIndex < Mesh->m_SectionCount; SectionIndex++)
	{
		lcMeshSection* Section = &Mesh->m_Sections[SectionIndex];
		const char* colname;
		type currentColor;

		numCoords = 0;
		coords = (float *)malloc(1);
		numCoordIndices = 0;
		coordIndices = (int *)malloc(1);

		if ((Section->ColorIndex == LC_COLOR_DEFAULT) || (Section->ColorIndex == LC_COLOR_EDGE))
		{
			colname = g_ColorList[color].Name;
			currentColor = color;
		}
		else
		{
			colname = g_ColorList[Section->ColorIndex].Name;
			currentColor = Section->ColorIndex;
		}

		type* IndexPtr = (type*)((char*)indices + Section->IndexOffset);

		if (Section->PrimitiveType == GL_TRIANGLES)
		{
			for (int i = 0; i < Section->IndexCount; i += 3)
			{
				generateMeshData(IndexPtr, pos, pPiece, 3, currentColor);
				IndexPtr += 3;
			}

			writeIndent(stream);
			char altname[256];
			strcpy(altname, colname);
			while (char* ptr = (char*)strchr(altname, ' '))
				*ptr = '_';
			fprintf(stream, "# %s\n", altname);

			bool rigidBody = (VRMLdialect == X3DV_WITH_RIGID_BODY_PHYSICS);

			bool writeBegin = ((!rigidBody) || (beginAndEnd && (SectionIndex == 0))); 
			bool writeEnd = ((!rigidBody) || (beginAndEnd && (SectionIndex == (Mesh->m_SectionCount - 1))));

			if (writeBegin) 
			{
				writeVRMLShapeBegin(stream, currentColor, false);
				writeVRMLShapeMeshBegin(stream);
			}

			writeVRMLShapeMeshData(stream);

			if (writeEnd)
			{
				writeVRMLShapeMeshEnd(stream);
				writeVRMLShapeEnd(stream);
			}
		}
		else
		{
			// IndexedLineSet not supported in RigidBody node for the xj3d browser 8-(
			if (VRMLdialect != X3DV_WITH_RIGID_BODY_PHYSICS) 
			{
				writeIndent(stream);
				fprintf(stream, "# lines of color %s\n", colname);
			}

			for (int i = 0; i < Section->IndexCount; i += 3)
			{
				generateMeshData(IndexPtr, pos, pPiece, 2, currentColor);
				IndexPtr += 2;
			}

			// IndexedLineSet not supported in RigidBody node for the xj3d browser 8-(
			if (VRMLdialect != X3DV_WITH_RIGID_BODY_PHYSICS) 
			{
				writeVRMLShapeBegin(stream, currentColor, true);
				writeVRMLShapeMeshBegin(stream);
				writeVRMLShapeMeshData(stream);
				writeVRMLShapeMeshEnd(stream);
				writeVRMLShapeEnd(stream);
			}
		}

		free(coords);
		free(coordIndices);
	}

	Mesh->m_IndexBuffer->UnmapBuffer();
}

// The X3DV export need to "melt together" faces of different pieces into one triangleSet
// based on the leocad "piece -> group" menupoint, otherwise the rigid body simulation would simulate all pieces seperatly
// Additionally a center of mass is required for the X3DV export 
// Unfortunalty, the origin of a piece in leocad is not usefull for use as center of mass
// So the exporter use the mid of the boundingbox of all pieces in a group as center of mass
// the needed information is stored in the following compound datatype

class GroupInfo
{
public:
	lcGroup *group;
	float minBoundingBox[3];
	float maxBoundingBox[3];
	char  groupname[65];
	bool  firstData;
	lcPiece *firstPiece; 
	lcPiece *lastPiece; 
};

// routines to account a boundingbox 

template<class type> void Project::getMinMaxData(type* info, lcPiece* pPiece, GroupInfo* groupInfo)
{
	Matrix matrix(pPiece->m_AxisAngle, pPiece->m_Position);

	lcMesh* Mesh = pPiece->m_PieceInfo->m_Mesh;
	float* verts = (float*)Mesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	for (int i = 0; i < 3; i++)
	{
		float vertex[3];
		float *localVertex = &verts[info[i] * 3];
		matrix.TransformPoint(vertex, localVertex);

		for (int j = 0; j < 3; j++)
		{
			if (groupInfo->firstData)
			{
				groupInfo->minBoundingBox[j] = vertex[j];
				groupInfo->maxBoundingBox[j] = vertex[j];
			}
			if (vertex[j] < groupInfo->minBoundingBox[j])
				groupInfo->minBoundingBox[j] = vertex[j];
			if (vertex[j] > groupInfo->maxBoundingBox[j])
				groupInfo->maxBoundingBox[j] = vertex[j];
		}
		groupInfo->firstData = false;
	}

	Mesh->m_VertexBuffer->UnmapBuffer();
}

template<class type> void Project::getMinMax(type col, lcPiece* piece, unsigned short group, GroupInfo* groupInfo)
{ 
	lcMesh* Mesh = piece->m_PieceInfo->m_Mesh;
	void* indices = Mesh->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	for (int SectionIndex = 0; SectionIndex < Mesh->m_SectionCount; SectionIndex++)
	{
		lcMeshSection* Section = &Mesh->m_Sections[SectionIndex];
		type* IndexPtr = (type*)((char*)indices + Section->IndexOffset);

		// Skip lines.
		if (Section->PrimitiveType != GL_TRIANGLES)
			continue;

		for (int i = 0; i < Section->IndexCount; i += 3)
		{
			getMinMaxData(IndexPtr, piece, groupInfo);
			IndexPtr += 3;
		}
	}

	Mesh->m_IndexBuffer->UnmapBuffer();
}

// Pieces without TopGroup represent Pieces without "Piece->Group" in LeoCAD
// The handleAsGroup function is used with "if" in loops over pieces, 
// to run the if/loop content for either only single pieces or all pieces part
// of a LeoCAD "Piece->Group"

bool Project::handleAsGroup(lcPiece* pPiece, GroupInfo groupInfo)
{
	if (pPiece->GetTopGroup() == NULL)
	{
		if (groupInfo.firstPiece == pPiece)
			return true;
	}
	else
	{
		if (pPiece->GetTopGroup() == groupInfo.group)
			return true;
	}
	return false;
}

// main routine to export VRML97 or X3DV files

void Project::exportVRMLFile(char *filename, int dialect)
{
	numDEF = 0;
	indent = 0;
	int coordinateCounter = 1;
	char buf[LC_MAXPATH], *ptr;
	FILE* stream = fopen(filename, "wt");
	lcPiece* pPiece;
	bool rigidBody = (dialect == X3DV_WITH_RIGID_BODY_PHYSICS);
	VRMLdialect = dialect;	
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

	// write header
	switch (VRMLdialect)
	{
	case VRML97:
		fputs("#VRML V2.0 utf8\n", stream);
		break;
	case X3DV_WITH_RIGID_BODY_PHYSICS:
		fputs("#X3D V3.0 utf8\n", stream);
		fputs("PROFILE Immersive\n", stream);
		fputs("COMPONENT RigidBodyPhysics:2\n", stream);
		break;
	}

	// write leading comments
	fputs("# Model exported from LeoCAD\n", stream);
	if (strlen(buf) != 0)
		fprintf(stream,"# Original name: %s\n", ptr);
	if (strlen(m_strAuthor))
		fprintf(stream, "# Author: %s\n", m_strAuthor);

	// write leading once needed X3DV commands
	if (rigidBody)
	{
		fputs("\n", stream);
		writeIndent(stream);
		fputs("Group {\n", stream);
		indent += INDENT_INC;
		writeIndent(stream);
		fputs("children [\n", stream);
		indent += INDENT_INC;
	}

	// initalise "melt together" group information
	lcObjArray<GroupInfo> allGroups;
	GroupInfo groupObject;
	for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
	{
		lcGroup *topGroup = pPiece->GetTopGroup();
		int foundGroup = false;;
		if (topGroup != NULL)
		{
			for (int i = 0; i < allGroups.GetSize(); i++)
				if (allGroups[i].group == topGroup)
				{
					allGroups[i].lastPiece = pPiece;
					foundGroup = true;
				} 
		}
		if (!foundGroup)
		{
			groupObject.group = topGroup;
			groupObject.firstPiece = pPiece;
			groupObject.lastPiece = pPiece;
			groupObject.firstData = true;
			if (topGroup != NULL)
				snprintf(groupObject.groupname, 64, "%s", topGroup->m_strName);
			else
				snprintf(groupObject.groupname, 64, "%s", (char*)pPiece->m_Name);
			allGroups.Add(groupObject);
		}
	}

	// account bounding box information
	for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
	{
		u32 color = pPiece->m_Color;
		PieceInfo *pInfo = pPiece->m_PieceInfo;
		for (int j = 0; j < allGroups.GetSize(); j++)
		{
			if (handleAsGroup(pPiece, allGroups[j]))
			{
				unsigned short group;

				for (group = 0; group < pInfo->m_nGroupCount; group++)
				{
					if (pInfo->m_Mesh->m_IndexType == GL_UNSIGNED_INT)
					{
						unsigned long col = color;
						getMinMax(col, pPiece, group, &(allGroups[j]));
					}
					else
					{
						unsigned short col = color;
						getMinMax(col, pPiece, group, &(allGroups[j]));
					}
				}
			}
		}
	}		

	// write main VRML97/X3DV data
	for (int j = 0; j < allGroups.GetSize(); j++)
	{
		for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
		{
			bool beginGroup = ((allGroups[j].group == NULL) || (allGroups[j].firstPiece == pPiece));
			bool endGroup = ((allGroups[j].group == NULL) || (allGroups[j].lastPiece == pPiece));

			if (handleAsGroup(pPiece, allGroups[j]))
			{				
				PieceInfo* pInfo = pPiece->m_PieceInfo;
				u32 color = pPiece->m_Color;
			
				strcpy(buf, pPiece->m_Name);
				for (unsigned int i = 0; i < strlen(buf); i++)
					if ((buf[i] == '#') || (buf[i] == ' '))
						buf[i] = '_';

				writeIndent(stream);
				fprintf(stream, "# %s\n", buf);

				Vector3 pos;
				Vector4 rot;

				switch (VRMLdialect)
				{
				case VRML97:
					pos = pPiece->m_Position;
					rot = pPiece->m_AxisAngle;
					writeIndent(stream);
					fprintf(stream, "Transform {\n");
					indent += INDENT_INC;
					writeIndent(stream);
					fprintf(stream, "translation %g %g %g\n", pos[1] * VRMLScale, pos[2] * VRMLScale, pos[0] * VRMLScale);
					writeIndent(stream);
					fprintf(stream, "rotation %g %g %g %g\n", rot[1], rot[2], rot[0], rot[3] * M_PI / 180.0);
					writeIndent(stream);
					fprintf(stream, "children [\n");
					indent += INDENT_INC;
					break;
				case X3DV_WITH_RIGID_BODY_PHYSICS:
					for (int k = 0; k < 3; k++)
						pos[k] = allGroups[j].minBoundingBox[k] + (allGroups[j].maxBoundingBox[k] - allGroups[j].minBoundingBox[k]) / 2.0f;
				}

				unsigned short group;

				if (pInfo->m_nGroupCount > 0)
				{
					if (beginGroup && rigidBody)
					{
						if (pInfo->m_Mesh->m_IndexType == GL_UNSIGNED_INT)
						{
							unsigned long col = color;
							writeVRMLShapeBegin(stream, col, false);
							writeVRMLShapeMeshBegin(stream);
						}
						else
						{
							unsigned short col = color;
							writeVRMLShapeBegin(stream, col, false);
							writeVRMLShapeMeshBegin(stream);
						}
					}

					for (group = 0; group < pInfo->m_nGroupCount; group++)
					{
						writeIndent(stream);
						fprintf(stream, "# group %d\n",(int)group);
						if (pInfo->m_Mesh->m_IndexType == GL_UNSIGNED_INT)
						{
							unsigned long col = color;
							writeVRMLShapes(col, stream, coordinateCounter, pPiece, group, pos, !rigidBody);
						}
						else
						{
							unsigned short col = color;
							writeVRMLShapes(col, stream, coordinateCounter, pPiece, group, pos, !rigidBody);
						}
					}

					if (endGroup && rigidBody)
					{
						writeVRMLShapeMeshEnd(stream);
						writeVRMLShapeEnd(stream);
					}
				}

				if (!rigidBody)
				{
					indent -= INDENT_INC;
					writeIndent(stream);
					fprintf(stream, "]\n");
				}

				if (endGroup || (!rigidBody))
				{
					indent -= INDENT_INC;
					writeIndent(stream);
					fprintf(stream, "} # endShape\n");
				}

				coordinateCounter++;
			}

		}
	}

	if (rigidBody)
	{
		// write trailing once needed X3DV commands
        	// http://www.xj3d.org/extensions/rigid_physics.html
        	// http://www.web3d.org/x3d/specifications/ISO-IEC-19775-X3DAbstractSpecification_Revision1_to_Part1/Part01/components/rigid_physics.html
		indent -= INDENT_INC;
		writeIndent(stream);
		fputs("]\n", stream);
		indent -= INDENT_INC;
		writeIndent(stream);
		fputs("}\n", stream);

		writeIndent(stream);
		fputs("DEF RigidBodyCollection1 RigidBodyCollection {\n", stream);
		indent += INDENT_INC;
		writeIndent(stream);
		fputs("bodies [\n", stream);
		indent += INDENT_INC;
		coordinateCounter = 0;
		for (int j = 0; j < allGroups.GetSize(); j++)
		{
			for (pPiece = m_ActiveModel->m_Pieces; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
			{
				bool beginGroup = ((allGroups[j].group == NULL) || (allGroups[j].firstPiece == pPiece));
				bool endGroup = ((allGroups[j].group == NULL) || (allGroups[j].lastPiece == pPiece));

				if (handleAsGroup(pPiece, allGroups[j]))
				{				
					float pos[3];

					if (VRMLdialect == X3DV_WITH_RIGID_BODY_PHYSICS)
					{
						if (beginGroup)
						{
							for (int k = 0; k < 3; k++)
								pos[k] = allGroups[j].minBoundingBox[k] + (allGroups[j].maxBoundingBox[k] - allGroups[j].minBoundingBox[k]) / 2.0f;
							writeIndent(stream);
							fprintf(stream, "# %s\n", allGroups[j].groupname);
							writeIndent(stream);
							fprintf(stream, "RigidBody {\n");
							indent += INDENT_INC;
							writeIndent(stream);
							fprintf(stream, "position %g %g %g\n", pos[1] * VRMLScale, pos[2] * VRMLScale, pos[0] * VRMLScale);
							writeIndent(stream);
							fprintf(stream, "geometry USE CollidableShape%d\n", coordinateCounter);
						}
						if (endGroup)
						{
							indent -= INDENT_INC;
							writeIndent(stream);
							fprintf(stream, "}\n");
						}
					}
				}

			}
			coordinateCounter++;
		}
		indent -= INDENT_INC;
		writeIndent(stream);
		fputs("]\n", stream);

		writeIndent(stream);
		fputs("collider DEF CollisionCollection1 CollisionCollection", stream);
		fputs(" {\n", stream);
		indent += INDENT_INC;                
		writeIndent(stream);
		fputs("collidables [\n", stream);  
		indent += INDENT_INC;                
		for (int i = 0; i < numDEF; i++)
		{
			writeIndent(stream);
			fprintf(stream, "USE CollidableShape%d\n", i);
		}
		indent -= INDENT_INC;                
		writeIndent(stream);
		fputs("]\n", stream);
		indent -= INDENT_INC;                
		writeIndent(stream);
		fputs("}\n", stream);
		indent -= INDENT_INC;
		writeIndent(stream);
		fputs("}\n", stream);

		writeIndent(stream);
		fputs("DEF CollisionSensor1 CollisionSensor {\n", stream);
		indent += INDENT_INC;
		writeIndent(stream);
		fputs("collidables USE CollisionCollection1\n", stream);
		indent -= INDENT_INC;
		writeIndent(stream);
		fputs("}\n", stream);
		fputs("\n", stream);
		fputs("ROUTE CollisionSensor1.contacts TO RigidBodyCollection1.set_contacts\n", stream);
	}
	
	if (indent != 0)
		fprintf(stderr, "internal error: indent %d\n", indent);
	fclose(stream);
}
