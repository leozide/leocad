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

// FIXME: temporary function, replace the code !!!
void SystemUpdateFocus (void* p, int i)
{
  messenger->Dispatch (LC_MSG_FOCUS_CHANGED, p);
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
	m_pTerrain = new Terrain();
	m_pBackground = new Texture();
	m_nAutosave = Sys_ProfileLoadInt ("Settings", "Autosave", 10);
	m_nMouse = Sys_ProfileLoadInt ("Default", "Mouse", 11);
	strcpy(m_strModelsPath, Sys_ProfileLoadString ("Default", "Projects", ""));

  if (messenger == NULL)
    messenger = new Messenger ();
  messenger->AddRef ();

  for (i = 0; i < LC_CONNECTIONS; i++)
	{
		m_pConnections[i].entries = NULL;
		m_pConnections[i].numentries = 0;
	}

	for (i = 0; i < 10; i++)
		m_pClipboard[i] = NULL;

  m_pLibrary = new PiecesLibrary ();
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
  delete m_pLibrary;
}


/////////////////////////////////////////////////////////////////////////////
// Project attributes, general services

// The main window should be created before calling this
bool Project::Initialize(int argc, char *argv[], char* binpath, char* libpath)
{
  char *env_path;
  bool loaded = false;

  strcpy (m_AppPath, binpath);

  // check if there's an environment variable for the piece library
  env_path = getenv ("LEOCAD_LIB");
  if (env_path != NULL)
    libpath = env_path;

  m_strPathName[0] = 0;

  LC_IMAGE_OPTS imopts;
  char picture[LC_MAXPATH];
  bool save_image = false;
  picture[0] = 0;

  unsigned long image = Sys_ProfileLoadInt  ("Default", "Image Options", 1|LC_IMAGE_TRANSPARENT);
  int width = Sys_ProfileLoadInt ("Default", "Image Width", 640);
  int height = Sys_ProfileLoadInt ("Default", "Image Height", 480);
//	int width = Sys_ProfileLoadInt ("Default", "Image Width", GetSystemMetrics(SM_CXSCREEN));
//	int height = Sys_ProfileLoadInt ("Default", "Image Height", GetSystemMetrics(SM_CYSCREEN));
  unsigned short from = 0, to = 0;
  int i, animation = -1;
  bool highlight = false;
  imopts.quality = Sys_ProfileLoadInt ("Default", "JPEG Quality", 70);
  imopts.interlaced = (image & LC_IMAGE_PROGRESSIVE) != 0;
  imopts.transparent = (image & LC_IMAGE_TRANSPARENT) != 0;
  imopts.truecolor = (image & LC_IMAGE_HIGHCOLOR) != 0;
  imopts.format = (unsigned char)(image & ~(LC_IMAGE_MASK));

	for (i = 1; i < argc; i++)
	{
		char* param = argv[i];

    if (param[0] == '-')
		{
			if (((strcmp (param, "-l") == 0) || (strcmp (param, "--libpath") == 0)) && ((i+1) < argc))
			{
				i++;
				libpath = argv[i];
			}
			else if ((strcmp (param, "-i") == 0) || (strcmp (param, "--image") == 0))
			{
				save_image = true;

				if (((i+1) != argc) && (argv[i+1][0] != '-'))
				{
					i++;
					strcpy (picture, argv[i]);
				}
			}
			else if (((strcmp (param, "-w") == 0) || (strcmp (param, "--width") == 0)) && ((i+1) < argc))
			{
				int w;
				i++;
				if (sscanf(argv[i], "%d", &w) == 1)
					width = w;
			}
			else if (((strcmp (param, "-h") == 0) || (strcmp (param, "--height") == 0)) && ((i+1) < argc))
			{
				int h;
				i++;
				if (sscanf(argv[i], "%d", &h) == 1)
					height = h;
			}
			else if (((strcmp (param, "-f") == 0) || (strcmp (param, "--from") == 0)) && ((i+1) < argc))
			{
				int f;
				i++;
				if (sscanf(argv[i], "%d", &f) == 1)
					from = f;
			}
			else if (((strcmp (param, "-t") == 0) || (strcmp (param, "--to") == 0)) && ((i+1) < argc))
			{
				int t;
				i++;
				if (sscanf(argv[i], "%d", &t) == 1)
					to = t;
			}
			else if (strcmp (param, "--animation") == 0)
				animation = 1;
			else if (strcmp (param, "--instructions") == 0)
				animation = 0;
			else if (strcmp (param, "--highlight") == 0)
				highlight = true;
			else if ((strcmp (param, "-v") == 0) || (strcmp (param, "--version") == 0))
			{
				printf ("LeoCAD version "LC_VERSION" for "LC_VERSION_OSNAME"\n");
				printf ("Copyright (c) 1996-2003, BT Software\n");
				printf ("Compiled "__DATE__"\n");

#ifdef LC_HAVE_JPEGLIB
				printf ("With JPEG support\n");
#else
				printf ("Without JPEG support\n");
#endif

#ifdef LC_HAVE_PNGLIB
				printf ("With PNG support\n");
#else
				printf ("Without PNG support\n");
#endif

	return false;
			}
			else if ((strcmp (param, "-h") == 0) || (strcmp (param, "--help") == 0))
			{
			}
			else
				printf ("Unknown parameter: %s\n", param);
		}
		else
		{
			strcpy (m_strPathName, param);
/*
			if (m_strFileName.IsEmpty())
				m_strFileName = pszParam;
			else if (m_nShellCommand == FilePrintTo && m_strPrinterName.IsEmpty())
				m_strPrinterName = pszParam;
			else if (m_nShellCommand == FilePrintTo && m_strDriverName.IsEmpty())
				m_strDriverName = pszParam;
			else if (m_nShellCommand == FilePrintTo && m_strPortName.IsEmpty())
				m_strPortName = pszParam;
*/
		}
	}

  // if the user specified a library, try to load it first
  if (libpath != NULL)
    loaded = m_pLibrary->Load (libpath);

  // if we couldn't find a library, try the executable path
  if (!loaded)
    loaded = m_pLibrary->Load (binpath);

  if (!loaded)
  {
#ifdef LC_WINDOWS
    // let's hope this message helps the users
    SystemDoMessageBox("Cannot load piece library.\n"
      "Make sure that you have the PIECES.IDX file in the same "
      "folder where you installed the program.", LC_MB_OK|LC_MB_ICONERROR);
#else
    printf("Cannot load piece library !\n");
#endif
    return false;
  }

  SystemInit();

  if (strlen(m_strPathName) && OnOpenDocument(m_strPathName))
  {
    SetPathName(m_strPathName, true);

    if (save_image == true)
    {
      bool need_ext = false;

      if (picture[0] == 0)
      {
	strcpy (picture, m_strPathName);
	char *p = strrchr (picture, '.');
	if (p != NULL)
	  *p = 0;
	need_ext = true;
      }
      else
      {
	char ext[5];
	char *p = strrchr(picture, '.');
	if (p != NULL)
	  strcpy(ext, p+1);
	strlwr(ext);

	if ((strcmp(ext, "bmp") != 0) && (strcmp(ext, "gif") != 0) && 
	    (strcmp(ext, "jpg") != 0) && (strcmp(ext, "jpeg") != 0) &&
	    (strcmp(ext, "png") != 0))
	  need_ext = true;
      }

      if (need_ext)
	switch (imopts.format)
	{
	case LC_IMAGE_BMP: strcat(picture, ".bmp"); break;
	case LC_IMAGE_GIF: strcat(picture, ".gif"); break;
	case LC_IMAGE_JPG: strcat(picture, ".jpg"); break;
	case LC_IMAGE_PNG: strcat(picture, ".png"); break;
	}

      imopts.background[0] = (unsigned char)(m_fBackground[0]*255);
      imopts.background[1] = (unsigned char)(m_fBackground[1]*255);
      imopts.background[2] = (unsigned char)(m_fBackground[2]*255);

      if (animation == 0)
	m_bAnimation = false;
      else if (animation == 1)
	m_bAnimation = true;

      if (to < from)
      {
	unsigned short tmp;
	tmp = from;
	from = to;
	to = tmp;
      }

      if ((from == 0) && (to == 0))
      {
	if (m_bAnimation)
	  from = to = m_nCurFrame;
	else
	  from = to = m_nCurStep;
      }
      else if ((from == 0) && (to != 0))
      {
	from = to;
      }
      else if ((from != 0) && (to == 0))
      {
	to = from;
      }

      if (m_bAnimation)
      {
	if (from > m_nTotalFrames)
	  from = m_nTotalFrames;

	if (to > m_nTotalFrames)
	  to = m_nTotalFrames;
      }
      else
      {
	if (from > 255)
	  from = 255;

	if (to > 255)
	  to = 255;
      }

      Image* images = new Image[to - from + 1];
      CreateImages (images, width, height, from, to, highlight);

      for (i = 0; i <= to - from; i++)
      {
        char filename[LC_MAXPATH];

        if (from != to)
        {
          char* ext = strrchr (picture, '.');
          *ext = 0;
          sprintf (filename, "%s%02d.%s", picture, i+1, ext+1);
          *ext = '.';
        }
        else
          strcpy (filename, picture);

        images[i].FileSave (filename, &imopts);
      }
      delete []images;

      return false;
    }
  }
  else
    OnNewDocument();

  return true;
}

void Project::SetTitle(const char* lpszTitle)
{
	strcpy(m_strTitle, lpszTitle);

	char title[LC_MAXPATH], *ptr, ext[4];
	strcpy(title, "LeoCAD - ");
	strcat(title, m_strTitle);

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
	}

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
		m_pTerrain->LoadDefaults((m_nDetail & LC_DET_LINEAR) != 0);
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
	m_nActiveViewport = 0;
	SystemUpdateViewport(0, m_nViewportMode);
	m_nViewportMode = 0;
	SetAction(0);
	m_nCurColor = 0;
	SystemUpdateColorList(m_nCurColor);
	m_nCurGroup = 1;
	SystemSetGroup(m_nCurGroup);
	m_bAnimation = false;
	m_bAddKeys = false;
	SystemUpdateAnimation(m_bAnimation, m_bAddKeys);
	m_bUndoOriginal = true;
	SystemUpdateUndoRedo(NULL, NULL);
	m_nDetail = Sys_ProfileLoadInt ("Default", "Detail", LC_DET_BRICKEDGES);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_BACKGROUND) != 0, (m_nDetail & LC_DET_FAST) != 0);
	m_nAngleSnap = (unsigned short)Sys_ProfileLoadInt ("Default", "Angle", 30);
	m_nSnap = Sys_ProfileLoadInt ("Default", "Snap", LC_DRAW_SNAP_A | LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y | LC_DRAW_SNAP_Z | LC_DRAW_MOVE | LC_DRAW_PREVIEW);
	SystemUpdateSnap(m_nSnap);
	m_nMoveSnap = 0;
	SystemUpdateMoveSnap(m_nMoveSnap);
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
	m_pTerrain->LoadDefaults((m_nDetail & LC_DET_LINEAR) != 0);

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

			switch (i) 
			{
			case LC_CAMERA_MAIN:  m_pViewCameras[0] = pCam; break;
			case LC_CAMERA_FRONT: m_pViewCameras[1] = pCam; break;
			case LC_CAMERA_TOP:   m_pViewCameras[2] = pCam; break;
			case LC_CAMERA_RIGHT: m_pViewCameras[3] = pCam; break;
			}
		}
		SystemUpdateCameraMenu(m_pCameras);
		SystemUpdateCurrentCamera(NULL, m_pViewCameras[0], m_pCameras);
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

			switch (i) 
			{
			case LC_CAMERA_MAIN:  m_pViewCameras[0] = pCam; break;
			case LC_CAMERA_FRONT: m_pViewCameras[1] = pCam; break;
			case LC_CAMERA_TOP:   m_pViewCameras[2] = pCam; break;
			case LC_CAMERA_RIGHT: m_pViewCameras[3] = pCam; break;
			}
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
			sidevec.FromFloat(1,0,0);
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
          file->ReadLong (&i, 1); m_nCurGroup = i;
          file->ReadLong (&i, 1); m_nCurColor = i;
          file->ReadLong (&i, 1); action = i;
          file->ReadLong (&i, 1); m_nCurStep = i;
	}

	if (fv > 0.8f)
          file->ReadLong (&m_nScene, 1);

	file->ReadLong (&count, 1);
	while (count--)
	{	
		if (fv > 0.4f)
		{
			char name[9];
			Piece* pPiece = new Piece(NULL);
			pPiece->FileLoad(*file, name);
			PieceInfo* pInfo = m_pLibrary->FindPieceInfo(name);
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

			PieceInfo* pInfo = m_pLibrary->FindPieceInfo(name);
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
	}

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


		Piece* pPiece;
		for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		{
			i = (int)pPiece->GetGroup();
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
                          file->ReadLong (&i, 1);
                          m_nViewportMode = i;
			}
			else
			{
                          file->ReadByte (&m_nViewportMode, 1);
                          file->ReadByte (&m_nActiveViewport, 1);
			}

			file->ReadLong (&count, 1);
			Camera* pCam = NULL;
			for (i = 0; i < count; i++)
			{
				pCam = new Camera(i, pCam);
				if (m_pCameras == NULL)
					m_pCameras = pCam;

				if (i < 4 && fv == 0.6f)
					m_pViewCameras[i] = pCam;
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
				m_pViewCameras[count] = pCam;
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
                          file->ReadLong (&i, 1); m_nMoveSnap = i;
			}
			else
			{
                          file->ReadByte (&ch, 1); m_bAnimation = (ch != 0);
                          file->ReadByte (&ch, 1); m_bAddKeys = (ch != 0);
                          file->ReadByte (&m_nFPS, 1);
                          file->ReadShort (&m_nCurFrame, 1);
                          file->ReadShort (&m_nTotalFrames, 1);
                          file->ReadShort (&m_nGridSize, 1);
                          file->ReadShort (&m_nMoveSnap, 1);
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
		SystemUpdateFocus(NULL, LC_PIECE|LC_UPDATE_TYPE|LC_UPDATE_OBJECT);
	SetAction(action);
	SystemUpdateViewport(m_nViewportMode, 0);
	SystemUpdateColorList(m_nCurColor);
	SystemSetGroup(m_nCurGroup);
	SystemUpdateAnimation(m_bAnimation, m_bAddKeys);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_BACKGROUND) != 0, (m_nDetail & LC_DET_FAST) != 0);
	SystemUpdateSnap(m_nSnap);
	SystemUpdateMoveSnap(m_nMoveSnap);
	SystemUpdateCameraMenu(m_pCameras);
	SystemUpdateCurrentCamera(NULL, m_pViewCameras[m_nActiveViewport], m_pCameras);
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
	float ver_flt = 1.3f; // LeoCAD 0.70
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
	i = m_nCurGroup; file->WriteLong (&i, 1);
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

	file->WriteByte (&m_nViewportMode, 1);
	file->WriteByte (&m_nActiveViewport, 1);

	Camera* pCamera;
	for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
          i++;
	file->WriteLong (&i, 1);

	for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
          pCamera->FileSave(*file);

	for (j = 0; j < 4; j++)
	{
          for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
            if (pCamera == m_pViewCameras[j])
              break;
            else
              i++;

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

void Project::FileReadLDraw(File* file, Matrix* prevmat, int* nOk, int DefColor, int* nStep)
{
	char buf[256];

	while (file->ReadString(buf, 256))
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
			ptr = strchr(tmp, '.');

			if (ptr != NULL)
				*ptr = 0;

			if (strlen(tmp) < 9)
			{
				char name[9];
				strcpy(name, tmp);

				PieceInfo* pInfo = m_pLibrary->FindPieceInfo(name);
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

			if (read)
			{
				FileDisk tf;
				if (tf.Open(pn, "rt"))
					FileReadLDraw(&tf, &tmpmat, nOk, cl, nStep);
			}
		}
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

bool Project::OnOpenDocument (const char* lpszPathName)
{
	FileDisk file;
	bool bSuccess = false;

	if (!file.Open(lpszPathName, "rb"))
	{
//		MessageBox("Failed to open file.");
		return false;
	}

	bool datfile = false;
	char ext[4], *ptr;
	memset(ext, 0, 4);
	ptr = strrchr(lpszPathName, '.');
	if (ptr != NULL)
	{
		strncpy(ext, ptr+1, 3);
		strlwr(ext);
	}

	if ((strcmp(ext, "dat") == 0) || (strcmp(ext, "ldr") == 0))
		datfile = true;

	DeleteContents(false);
	LoadDefaults(datfile);
	SetModifiedFlag(true);  // dirty during loading

	SystemDoWaitCursor(1);
	if (file.GetLength() != 0)
	{
		if (datfile)
		{
			int ok = 0, step = 1;
			Matrix mat;
			FileReadLDraw(&file, &mat, &ok, m_nCurColor, &step);
			m_nCurStep = step;
			SystemUpdateTime(false, m_nCurStep, 255);
			SystemUpdateFocus(NULL, LC_PIECE|LC_UPDATE_TYPE|LC_UPDATE_OBJECT);
			UpdateSelection();
			CalculateStep();
			UpdateAllViews ();

			char msg[50];
			sprintf(msg, "%d objects imported.", ok);
//			AfxMessageBox(msg, MB_OK|MB_ICONINFORMATION);
			bSuccess = true;
		}
		else
			bSuccess = FileLoad(&file, false, false); // load me
	}
	file.Close();
	SystemDoWaitCursor(-1);

	if (bSuccess == false)
	{
//		MessageBox("Failed to load.");
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
  m_ViewList.Add (pView);

  pView->MakeCurrent ();
  RenderInitialize ();
}

void Project::RemoveView (View* pView)
{
  m_ViewList.RemovePointer (pView);
}

void Project::UpdateAllViews (View* pSender)
{
  for (int i = 0; i < m_ViewList.GetSize (); i++)
    if (m_ViewList[i] != pSender)
      m_ViewList[i]->Redraw ();
}

/////////////////////////////////////////////////////////////////////////////
// Project rendering

// Only this function should be called.
void Project::Render(bool bToMemory)
{
	if (bToMemory)
	{
//		m_bDenyRender = TRUE;
//		m_bRendering = TRUE;
		RenderScene(true, false);
//		m_bRendering = FALSE;
//		m_bDenyRender = FALSE;
		return;
	}

//	if (m_bDenyRender)
//		return;

#ifdef _DEBUG
#ifdef LC_WINDOWS
#define BENCHMARK
#endif
#endif

#ifdef BENCHMARK
	DWORD dwMillis = GetTickCount();
#endif

	m_bStopRender = false;
	m_bRendering = true;
	RenderScene((m_nDetail & LC_DET_FAST) == 0, true);
        //	SystemSwapBuffers();

	if ((m_nDetail & LC_DET_FAST) && (m_nDetail & LC_DET_BACKGROUND))
	{
		RenderScene(true, true);
                //		if (!m_bStopRender)
                //			SystemSwapBuffers();
	}
	m_bRendering = false;


#ifdef BENCHMARK
	dwMillis = GetTickCount() - dwMillis;
	char szMsg[30];
	sprintf(szMsg, "%.4f sec", (float)dwMillis/1000);
	AfxGetMainWnd()->SetWindowText(szMsg);
#endif
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

static void RenderBSP(LC_BSPNODE* node, float* eye, bool* bSel,
	bool bLighting, bool bNoAlpha, bool bEdges, unsigned char* nLastColor, bool* bTrans)
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

		node->piece->Render(bLighting, bNoAlpha, bEdges, nLastColor, bTrans);
		return;
	}

	if (eye[0]*node->plane[0] + eye[1]*node->plane[1] +
		eye[2]*node->plane[2] + node->plane[3] > 0.0f)
	{
		RenderBSP(node->back, eye, bSel, bLighting, bNoAlpha, bEdges, nLastColor, bTrans);
		RenderBSP(node->front, eye, bSel, bLighting, bNoAlpha, bEdges, nLastColor, bTrans);
	}
	else
	{
		RenderBSP(node->front, eye, bSel, bLighting, bNoAlpha, bEdges, nLastColor, bTrans);
		RenderBSP(node->back, eye, bSel, bLighting, bNoAlpha, bEdges, nLastColor, bTrans);
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

void Project::RenderScene(bool bShaded, bool bDrawViewports)
{
  glViewport (0, 0, m_nViewX, m_nViewY);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (bDrawViewports)
  {
    if (bShaded)
    {
      if (m_nDetail & LC_DET_LIGHTING)
	glDisable (GL_LIGHTING);
      if (m_nScene & LC_SCENE_FOG)
	glDisable (GL_FOG);
      RenderViewports(true, true);
      if (m_nDetail & LC_DET_LIGHTING)
	glEnable(GL_LIGHTING);
      if (m_nScene & LC_SCENE_FOG)
	glEnable (GL_FOG);
    }
    else
      RenderViewports(true, true);
  }
  else
    RenderViewports(true, false);

  for (int vp = 0; vp < viewports[m_nViewportMode].n; vp++)
  {
    int x = (int)(viewports[m_nViewportMode].dim[vp][0] * ((float)m_nViewX));
    int y = (int)(viewports[m_nViewportMode].dim[vp][1] * ((float)m_nViewY));
    int w = (int)(viewports[m_nViewportMode].dim[vp][2] * ((float)m_nViewX));
    int h = (int)(viewports[m_nViewportMode].dim[vp][3] * ((float)m_nViewY));

    float ratio = (float)w/h;
    glViewport(x, y, w, h);
    m_pViewCameras[vp]->LoadProjection(ratio);

    if ((m_nSnap & LC_DRAW_AXIS) || (m_nSnap & LC_DRAW_GRID))
    {
      if ((bShaded) && (m_nDetail & LC_DET_LIGHTING))
	glDisable(GL_LIGHTING);

      glColor3f(1.0f - m_fBackground[0], 1.0f - m_fBackground[1], 1.0f - m_fBackground[2]);

      // There's got to be an easier way...
      if (m_nSnap & LC_DRAW_AXIS)
      {
	GLdouble model[16], proj[16], obj1x, obj1y, obj1z, obj2x, obj2y, obj2z;
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluUnProject(25, 25, 0.1, model, proj, viewport, &obj1x, &obj1y, &obj1z);
	gluUnProject(45, 25, 0.1, model, proj, viewport, &obj2x, &obj2y, &obj2z); 

	float ds = (float)sqrt((obj1x-obj2x)*(obj1x-obj2x)+(obj1y-obj2y)*(obj1y-obj2y)+(obj1z-obj2z)*(obj1z-obj2z));
	float verts[30][3] = { {0,0,0}, {0.8f*ds,0,0}, {0.8f*ds,0.2f*ds,0}, {ds,0,0}, {0.8f*ds,-0.2f*ds,0}, 
			       {0.8f*ds,0,0}, {0.8f*ds,0,0.2f*ds}, {ds,0,0}, {0.8f*ds,0,-0.2f*ds}, {0.8f*ds,0,0},
			       {0,0,0},{0,0.8f*ds,0}, {0,0.8f*ds,0.2f*ds}, {0,ds,0}, {0,0.8f*ds,-0.2f*ds},
			       {0,0.8f*ds,0}, {0.2f*ds,0.8f*ds,0}, {0,ds,0}, {-0.2f*ds,0.8f*ds,0}, {0,0.8f*ds,0},
			       {0,0,0}, {0,0,0.8f*ds}, {0.2f*ds,0,0.8f*ds}, {0,0,ds}, {-0.2f*ds,0,0.8f*ds},
			       {0,0,0.8f*ds}, {0,0.2f*ds,0.8f*ds}, {0,0,ds}, {0,-0.2f*ds,0.8f*ds}, {0,0,0.8f*ds} };

	glPushMatrix();
	glTranslated(obj1x, obj1y, obj1z);
	glVertexPointer (3, GL_FLOAT, 0, verts);
	glDrawArrays(GL_LINE_STRIP, 0, 30);

	Matrix m;
	m.FromInverse(model);
	m.SetTranslation(0,0,0);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	m_ScreenFont.MakeCurrent();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);

  glPushMatrix();
	glTranslatef(1.4f*ds, 0, 0);
	glMultMatrixf(m.m);
	glBegin(GL_QUADS);
  m_ScreenFont.PrintCharScaled (0.025f * ds, 'X');
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 1.4f*ds, 0);
	glMultMatrixf(m.m);
	glBegin(GL_QUADS);
  m_ScreenFont.PrintCharScaled (0.025f * ds, 'Y');
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 0, 1.4f*ds);
	glMultMatrixf(m.m);
	glBegin(GL_QUADS);
  m_ScreenFont.PrintCharScaled (0.025f * ds, 'Z');
	glEnd();
	glPopMatrix();

	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
      }

      if (m_nSnap & LC_DRAW_GRID)
	glCallList (m_nGridList);

      if ((bShaded) && (m_nDetail & LC_DET_LIGHTING))
	glEnable(GL_LIGHTING);
    }

    if ((m_nDetail & LC_DET_LIGHTING) != 0)
    {
      int index = 0;
      Light *pLight;

      for (pLight = m_pLights; pLight; pLight = pLight->m_pNext, index++)
	pLight->Setup (index);
    }

    //    glDisable (GL_COLOR_MATERIAL);
    /*
      {
	for (int i = -100; i < 100; i+=5)
	{
	  glBegin (GL_QUAD_STRIP);
	  glNormal3f (0,0,1);
	  for (int j = -100; j < 100; j+=5)
	  {
	    glVertex3f ((float)i/10, (float)j/10,0);
	    glVertex3f ((float)(i+5)/10, (float)j/10,0);
	  }
	  glEnd();
	}
      }
    */
    /*
    {
      LC_RENDER_INFO info;
      info.lighting = (m_nDetail & LC_DET_LIGHTING) != 0;
      info.stipple = (m_nDetail & LC_DET_SCREENDOOR) != 0;
      info.edges = (m_nDetail & LC_DET_BRICKEDGES) != 0;
      info.fLineWidth = m_fLineWidth;

      info.lastcolor = 255;
      info.transparent = false;


      float pos[] = { 0,0,0 };
      Curve curve (NULL, pos, 0);
      curve.Render (&info);
    }
    */
		if (bShaded)
		{
			if (m_nScene & LC_SCENE_FLOOR)
				m_pTerrain->Render(m_pViewCameras[vp], ratio);

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
				if (m_nDetail & LC_DET_BACKGROUND)
				{
					SystemPumpMessages();
					if (m_bStopRender)
						return;
				}

				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
				{
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
/*
						if (pPiece->m_pInfo->m_nConnectionCount == 1)
						{
							if (bCull)
							{
								bCull = false;
								glDisable(GL_CULL_FACE);
							}
						}
						else
						{
							if (!bCull)
							{
								bCull = true;
								glEnable(GL_CULL_FACE);
							}
						}
*/
						pPiece->Render((m_nDetail & LC_DET_LIGHTING) != 0, (m_nDetail & LC_DET_SCREENDOOR) != 0, (m_nDetail & LC_DET_BRICKEDGES) != 0, &nLastColor, &bTrans);
					}
					else
					{
						pPiece->m_pLink = pList;
						pList = pPiece;
					}
				}
			}

			if (pList)
			{
				float eye[3];
				m_pViewCameras[vp]->GetEyePos (eye);
				BuildBSP(&tree, pList);
				RenderBSP(&tree, eye, &bSel,
					(m_nDetail & LC_DET_LIGHTING) != 0, (m_nDetail & LC_DET_SCREENDOOR) != 0, (m_nDetail & LC_DET_BRICKEDGES) != 0, &nLastColor, &bTrans);
			}


#if 0
			// Draw transparent pieces
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
//				MSG msg;
//				while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//				{
//					TranslateMessage(&msg);
//					DispatchMessage(&msg);  
//					if (m_bStopRender) 
//						return;
//				}

				if ((pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation)) &&
					(pPiece->IsTransparent()))
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
/*
					if (pPiece->m_pInfo->m_nConnectionCount == 1)
					{
						if (bCull)
						{
							bCull = FALSE;
							glDisable(GL_CULL_FACE);
						}
					}
					else
					{
						if (!bCull)
						{
							bCull = TRUE;
							glEnable(GL_CULL_FACE);
						}
					}
*/
					pPiece->Render((m_nDetail & LC_DET_LIGHTING) != 0, (m_nDetail & LC_DET_SCREENDOOR) != 0, (m_nDetail & LC_DET_BRICKEDGES) != 0, &nLastColor, &bTrans);
				}
			}
#endif
			if (bTrans)
			{
				if (m_nDetail & LC_DET_SCREENDOOR)
					glDisable(GL_POLYGON_STIPPLE);
				else
				{
					glDepthMask(GL_TRUE);
					glDisable(GL_BLEND);
				}
			}
			if (bSel)
				glLineWidth(m_fLineWidth);
			if (bCull)
				glDisable(GL_CULL_FACE);
		}
		else
		{
//			glEnable (GL_CULL_FACE);
//			glShadeModel (GL_FLAT);
//			glDisable (GL_LIGHTING);
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
				RenderBoxes(true);
		}

		// Draw cameras & lights
		if (bDrawViewports)
		{
		  if (m_nDetail & LC_DET_LIGHTING)
		  {
		    glDisable (GL_LIGHTING);
		    int index = 0;
		    Light *pLight;

		    for (pLight = m_pLights; pLight; pLight = pLight->m_pNext, index++)
		      glDisable ((GLenum)(GL_LIGHT0+index));
		  }

		  Camera* pCamera;
		  Light* pLight;

		  for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		  {
		    if ((pCamera == m_pViewCameras[vp]) || !pCamera->IsVisible())
		      continue;
		    pCamera->Render(m_fLineWidth);
		  }

		  for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		    if (pLight->IsVisible ())
		      pLight->Render(m_fLineWidth);

		  if (m_nDetail & LC_DET_LIGHTING)
		    glEnable (GL_LIGHTING);
		}
	}
}

void Project::RenderViewports(bool bBackground, bool bLines)
{
	float x, y, w, h;
	int vp;

	glViewport(0, 0, m_nViewX, m_nViewY);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, m_nViewX, 0, m_nViewY, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375, 0.375, 0.0);

	if (bBackground)
	{
		// Draw gradient
		if (m_nScene & LC_SCENE_GRADIENT)
		{
			if ((m_nDetail & LC_DET_SMOOTH) == 0)
				glShadeModel(GL_SMOOTH);
			glDisable(GL_DEPTH_TEST);
			glBegin(GL_QUADS);

			for (vp = 0; vp < viewports[m_nViewportMode].n; vp++)
			{
				x = viewports[m_nViewportMode].dim[vp][0] * (float)m_nViewX;
				y = viewports[m_nViewportMode].dim[vp][1] * (float)m_nViewY;
				w = viewports[m_nViewportMode].dim[vp][2] * (float)m_nViewX;
				h = viewports[m_nViewportMode].dim[vp][3] * (float)m_nViewY;

				glColor3fv(m_fGradient1);
				glVertex2f(x+w, y+h);
				glVertex2f(x, y+h);
				glColor3fv(m_fGradient2);
				glVertex2f(x, y);
				glVertex2f(x+w, y);
			}
			glEnd();
			glEnable(GL_DEPTH_TEST);
			if ((m_nDetail & LC_DET_SMOOTH) == 0)
				glShadeModel(GL_FLAT);
		}

		glEnable(GL_TEXTURE_2D);

		// Draw the background
		if (m_nScene & LC_SCENE_BG)
		{
			glDisable (GL_DEPTH_TEST);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			m_pBackground->MakeCurrent();
			glBegin(GL_QUADS);

			for (vp = 0; vp < viewports[m_nViewportMode].n; vp++)
			{
				x = viewports[m_nViewportMode].dim[vp][0] * (float)m_nViewX;
				y = viewports[m_nViewportMode].dim[vp][1] * (float)m_nViewY;
				w = viewports[m_nViewportMode].dim[vp][2] * (float)m_nViewX;
				h = viewports[m_nViewportMode].dim[vp][3] * (float)m_nViewY;

				float tw = 1.0f, th = 1.0f;
				if (m_nScene & LC_SCENE_BG_TILE)
				{
					tw = w/m_pBackground->m_nWidth;
					th = h/m_pBackground->m_nHeight;
				}

				glTexCoord2f(0, 0);
				glVertex2f(x, y+h);
				glTexCoord2f(tw, 0);
				glVertex2f(x+w, y+h);
				glTexCoord2f(tw, th); 
				glVertex2f(x+w, y);
				glTexCoord2f(0, th);
				glVertex2f(x, y);
			}
			glEnd();
			glEnable(GL_DEPTH_TEST);
		}

		if (!bLines)
			glDisable(GL_TEXTURE_2D);
	}

	if (bLines)
	{
		// Draw text
		glColor3f(0, 0, 0);
		if (!bBackground)
			glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		m_ScreenFont.MakeCurrent();
		glEnable(GL_ALPHA_TEST);
		glBegin(GL_QUADS);

		for (vp = 0; vp < viewports[m_nViewportMode].n; vp++)
		{
			x = viewports[m_nViewportMode].dim[vp][0] * (float)m_nViewX;
			y = viewports[m_nViewportMode].dim[vp][1] * (float)m_nViewY;
			w = viewports[m_nViewportMode].dim[vp][2] * (float)(m_nViewX - 1);
			h = viewports[m_nViewportMode].dim[vp][3] * (float)(m_nViewY - 1);

      m_ScreenFont.PrintText (x + 3, y + h - 6, m_pViewCameras[vp]->GetName ());
		}
		glEnd();
	
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_TEXTURE_2D);

		// Borders
		if (m_fLineWidth != 1.0f)
			glLineWidth (1.0f);

		for (vp = 0; vp < viewports[m_nViewportMode].n; vp++)
		{
			if (vp == m_nActiveViewport)
				continue;

			x = viewports[m_nViewportMode].dim[vp][0] * (float)m_nViewX;
			y = viewports[m_nViewportMode].dim[vp][1] * (float)m_nViewY;
			w = viewports[m_nViewportMode].dim[vp][2] * (float)(m_nViewX - 1);
			h = viewports[m_nViewportMode].dim[vp][3] * (float)(m_nViewY - 1);

			glBegin(GL_LINE_LOOP);
			glVertex2f(x, y);
			glVertex2f(x+w, y);
			glVertex2f(x+w, y+h);
			glVertex2f(x, y+h);
			glEnd();
		}

		x = viewports[m_nViewportMode].dim[m_nActiveViewport][0] * (float)m_nViewX;
		y = viewports[m_nViewportMode].dim[m_nActiveViewport][1] * (float)m_nViewY;
		w = viewports[m_nViewportMode].dim[m_nActiveViewport][2] * (float)(m_nViewX - 1);
		h = viewports[m_nViewportMode].dim[m_nActiveViewport][3] * (float)(m_nViewY - 1);

		glColor3f(1.0f, 0, 0);
		glBegin(GL_LINE_LOOP);
		glVertex2f(x, y);
		glVertex2f(x+w, y);
		glVertex2f(x+w, y+h);
		glVertex2f(x, y+h);
		glEnd();

		if (m_fLineWidth != 1.0f)
			glLineWidth (m_fLineWidth);
	}
}

// bHilite - Draws focus/selection, not used for the 
// first rendering pass if remove hidden lines is enabled
void Project::RenderBoxes(bool bHilite)
{
	Piece* pPiece;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			pPiece->RenderBox(bHilite, m_fLineWidth);
}

// Initialize OpenGL
void Project::RenderInitialize()
{
	unsigned long stipple_pattern[32];
	int i;
	for (i = 0; i < 32; i += 2)
	{
		stipple_pattern[i]   = 0xAAAAAAAA;
		stipple_pattern[i+1] = 0x55555555;
	}
	glLineStipple (1, 65280);
	glPolygonStipple ((GLubyte*)&stipple_pattern[0]);
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
/*
	// TODO: use a blending function
	if (m_dwDetail & DET_ANTIALIAS)
	{
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);
	}
	else
	{
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POLYGON_SMOOTH);
	}
*/
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
  if (!m_ScreenFont.IsLoaded ())
  {
    char filename[LC_MAXPATH];
    FileDisk file;

    strcpy (filename, m_pLibrary->GetLibraryPath ());
    strcat (filename, "sysfont.txf");

    if (file.Open (filename, "rb"))
      m_ScreenFont.FileLoad (file);
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
	Piece* pPiece;
	Camera* pCamera;
	Light* pLight;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, false);

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
	for (pPrev = NULL, pCamera = m_pCameras; pCamera; pPrev = pCamera, pCamera = pCamera->m_pNext)
	{
		if (pCamera->IsSelected() && pCamera->IsUser())
		{
			bool bCanDelete = true;
			for (int i = 0; i < 4; i++)
				if (pCamera == m_pViewCameras[i])
				{
					bCanDelete = false;
					break;
				}

			if (bCanDelete)
			{
			  if (pPrev)
			    ((Camera*)pPrev)->m_pNext = pCamera->m_pNext;
			  else
			    m_pCameras = pCamera->m_pNext;
				delete pCamera;
				pCamera = (Camera*)pPrev;
				removed = true;
				SystemUpdateCameraMenu(m_pCameras);
				SystemUpdateCurrentCamera(NULL, m_pViewCameras[m_nActiveViewport], m_pCameras);
			}
		}
	}

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
	{
/*
		CLight* pLight = m_Lights.GetNext(pos1);
		if (pLight->m_bSelected || pLight->m_bTargetSelected)
		{
			CLight* tmp = m_Lights.GetAt(pos2);
			m_Lights.RemoveAt(pos2);
			delete tmp;
			ret = TRUE;
		}
*/
	}

	RemoveEmptyGroups();
//	CalculateStep();
//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);

	return removed;
}

void Project::UpdateSelection()
{
	unsigned long flags = 0;

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
			flags |= LC_SEL_CAMERA;

	for (Light* pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		if (pLight->IsSelected())
			flags |= LC_SEL_LIGHT;

	SystemUpdateSelected(flags);
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

void Project::SetViewSize(int cx, int cy)
{
	m_nViewX = cx;
	m_nViewY = (cy != 0) ? cy : 1; // Avoid divide by zero.
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
	int oldx, oldy;
	unsigned short oldtime;
	void* render = Sys_StartMemoryRender (width, height);
	unsigned char* buf = (unsigned char*)malloc (width*height*3);
	oldtime = m_bAnimation ? m_nCurFrame : m_nCurStep;
	oldx = m_nViewX;
	oldy = m_nViewY;
	m_nViewX = width;
	m_nViewY = height;

	if (!hilite)
		SelectAndFocusNone(false);

	RenderInitialize();

//	CCamera* pOld = pDoc->GetActiveCamera();
//	pDoc->m_ViewCameras[pDoc->m_nActiveViewport] = pDoc->GetCamera(CAMERA_MAIN);
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
		Render(true);
    images[i-from].FromOpenGL (width, height);
	}
//	pDoc->m_ViewCameras[pDoc->m_nActiveViewport] = pOld;
	m_nViewX = oldx;
	m_nViewY = oldy;
	if (m_bAnimation)
		m_nCurFrame = oldtime;
	else
		m_nCurStep = (unsigned char)oldtime;
	CalculateStep();
	free (buf);
	Sys_FinishMemoryRender (render);
}

void Project::CreateHTMLPieceList(FILE* f, int nStep, bool bImages, char* ext)
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
	for (int j = 0; j < m_pLibrary->GetPieceCount (); j++)
	{
		bool Add = false;
		int count[LC_MAXCOLORS];
		memset (&count, 0, sizeof (count));
		pInfo = m_pLibrary->GetPieceInfo (j);

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

		case LC_GROUP_CHANGED:
		{
			m_nCurGroup = (unsigned char)param;
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

			float pos[3], rot[4];
			pPiece->GetPosition(pos);
			pPiece->GetRotation(rot);
			Matrix mat(rot, pos);
			mat.ToEulerAngles(rot);

			if (mod->pos[0] != pos[0] || mod->pos[1] != pos[1] || mod->pos[2] != pos[2])
				pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->pos, LC_PK_POSITION);

			if (mod->rot[0] != rot[0] || mod->rot[1] != rot[1] || mod->rot[2] != rot[2])
			{
				mat.FromEulerAngles (mod->rot[0], mod->rot[1], mod->rot[2]);
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
			UpdateAllViews ();
		} break;

		case LC_CAMERA_MODIFIED:
		{
			LC_CAMERA_MODIFY* mod = (LC_CAMERA_MODIFY*)param;
			Camera* pCamera = (Camera*)mod->camera;
			float tmp[3];

			if (mod->hidden)
				pCamera->Hide();
			else
				pCamera->UnHide();

			pCamera->GetUpVec(tmp);

			if (tmp[0] != mod->eye[0] || tmp[1] != mod->eye[1] || tmp[2] != mod->eye[2])
				pCamera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->eye, LC_CK_EYE);
			if (tmp[0] != mod->target[0] || tmp[1] != mod->target[1] || tmp[2] != mod->target[2])
				pCamera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->target, LC_CK_TARGET);
			if (tmp[0] != mod->up[0] || tmp[1] != mod->up[1] || tmp[2] != mod->up[2])
				pCamera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->up, LC_CK_UP);

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
				if (!SaveModified())
					return;  // leave the original one

//	CWaitCursor wait;
				bool bWasModified = IsModified();
				SetModifiedFlag(false);  // not dirty for open

				if (!OnOpenDocument(filename))
				{
					// check if we corrupted the original document
					if (!IsModified())
						SetModifiedFlag(bWasModified);
					else
						OnNewDocument();

					return;  // open failed
				}
				SetPathName(filename, true);
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
						pInfo->ZoomExtents();

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
		  char *conv = (char*)malloc (9*m_pLibrary->GetPieceCount ());
		  char *flags = (char*)malloc (m_pLibrary->GetPieceCount ());
		  memset (conv, 0, 9*m_pLibrary->GetPieceCount());
		  memset (flags, 0, m_pLibrary->GetPieceCount());

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
//					AfxMessageBox(IDS_OPENFILE_ERROR, MB_OK|MB_ICONSTOP);
		      return;
		    }

		    unsigned char bt[4];
		    while (fread (&bt, 4, 1, f))
		    {
		      u = (((unsigned char)(bt[3])|((unsigned short)(bt[2]) << 8))|
			   (((unsigned long)(bt[1])) << 16)) + bt[0] * 16581375;
		      sprintf(tmp, "%d", (int)u);
		      pInfo = m_pLibrary->FindPieceInfo(tmp);

		      fread(&tmp, 9, 1, f);
		      if (tmp[8] != 0)
			fread(&tmp[9], 1, 1, f);

		      if (pInfo != NULL)
		      {
            int idx = m_pLibrary->GetPieceIndex (pInfo);
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
//					AfxMessageBox(IDS_OPENFILE_ERROR, MB_OK|MB_ICONSTOP);
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
			pInfo = m_pLibrary->FindPieceInfo(tmp);
			fread(&tmp, 8, 1, f);

			if (pInfo != NULL)
			{
			  int idx = m_pLibrary->GetPieceIndex (pInfo);
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
			int idx = m_pLibrary->GetPieceIndex (pInfo);
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
				int idx = m_pLibrary->GetPieceIndex (pInfo);
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
						if (pInfo->m_nFlags & LC_PIECE_LONGDATA)
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
			m_pViewCameras[m_nActiveViewport]->GetEyePos (eye);
			m_pViewCameras[m_nActiveViewport]->GetTargetPos (target);
			m_pViewCameras[m_nActiveViewport]->GetUpVec (up);

			fprintf(f, "\ncamera {\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f\n}\n\n",
				up[0], up[1], up[2], eye[1], eye[0], eye[2], target[1], target[0], target[2], m_pViewCameras[m_nActiveViewport]->m_fovy);
			fprintf(f, "background { color rgb <%1g, %1g, %1g> }\n\nlight_source { <0, 0, 20> White shadowless }\n\n",
				m_fBackground[0], m_fBackground[1], m_fBackground[2]);

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
			  float fl[12], pos[3], rot[4];
			  char name[20];
			  int idx = m_pLibrary->GetPieceIndex (pPiece->GetPieceInfo ());

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

			opts.lines = m_pLibrary->GetPieceCount();
			opts.count = (unsigned short*)malloc(m_pLibrary->GetPieceCount()*LC_MAXCOLORS*sizeof(unsigned short));
			memset (opts.count, 0, m_pLibrary->GetPieceCount()*LC_MAXCOLORS*sizeof(unsigned short));
			opts.names = (char**)malloc(m_pLibrary->GetPieceCount()*sizeof(char*));
			for (int i = 0; i < m_pLibrary->GetPieceCount(); i++)
				opts.names[i] = m_pLibrary->GetPieceInfo (i)->m_strDescription;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				int idx = m_pLibrary->GetPieceIndex (pPiece->GetPieceInfo ());
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
/*
		for (i = 0; i < pDoc->m_PartsIdx.GetSize(); i++)
			pDoc->m_PartsIdx[i].DeleteInformation();

		pDoc->LoadPieceLibrary();
		pDoc->m_bUndo = TRUE;
		PartFile.SeekToBegin();
		CArchive ar(&PartFile, CArchive::load);
		pDoc->Serialize(ar); 
		ar.Close();
		pDoc->m_bUndo = FALSE;
		pDoc->RebuildDisplayLists(FALSE);
		pDoc->UpdateAllViews(NULL);
		m_wndPiecesBar.m_wndPiecesList.UpdateList();
	}
*/
			}
		} break;

		case LC_FILE_RECENT:
		{
                  if (!SaveModified())
                    break;  // leave the original one

//	CWaitCursor wait;
                  bool bWasModified = IsModified();
                  SetModifiedFlag(false);  // not dirty for open
                  String filename = main_window->GetMRU (nParam);

                  if (!OnOpenDocument (filename))
                  {
                    // check if we corrupted the original document
                    if (!IsModified ())
                      SetModifiedFlag (bWasModified);
                    else
                      OnNewDocument ();

                    main_window->RemoveFromMRU (nParam);
                    return;  // open failed
                  }
                  SetPathName (filename, true);
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
			if (IsDrawing())
				return;

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
				SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
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
			if (IsDrawing())
				return;

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
				PieceInfo* pInfo = m_pLibrary->FindPieceInfo(name);
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
			SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
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
				if (pCamera != m_pViewCameras[m_nActiveViewport])
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
				if (pCamera != m_pViewCameras[m_nActiveViewport])
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
				float pos[3], rot[4];
				pLast->GetPosition(pos);
				pLast->GetRotation(rot);

				Matrix mat(rot, pos);
				mat.Translate(0, 0, pLast->GetPieceInfo()->m_fDimensions[2] - pLast->GetPieceInfo()->m_fDimensions[5]);
				mat.GetTranslation(pos);
				SnapPoint (pos, NULL);
				pPiece->Initialize(pos[0], pos[1], pos[2], m_nCurStep, m_nCurFrame, m_nCurColor);
				pPiece->ChangeKey(1, false, false, rot, LC_PK_ROTATION);
				pPiece->ChangeKey(1, true, false, rot, LC_PK_ROTATION);
				pPiece->UpdatePosition(1, false);
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
		  MinifigWizard *wiz = new MinifigWizard (m_ViewList[0]);
		  int i;

      wiz->IncRef ();

      if (SystemDoDialog (LC_DLG_MINIFIG, wiz))
		  {
		    SelectAndFocusNone(false);

		    for (i = 0; i < LC_MFW_NUMITEMS; i++)
		    {
		      if (wiz->m_Info[i] == NULL)
            continue;

		      Matrix mat;
		      Piece* pPiece = new Piece(wiz->m_Info[i]);

		      pPiece->Initialize(wiz->m_Position[i][0], wiz->m_Position[i][1], wiz->m_Position[i][2],
					 m_nCurStep, m_nCurFrame, wiz->m_Colors[i]);
		      pPiece->CreateName(m_pPieces);
		      AddPiece(pPiece);
		      pPiece->Select(true, false, false);

		      pPiece->ChangeKey(1, false, false, wiz->m_Rotation[i], LC_PK_ROTATION);
		      pPiece->ChangeKey(1, true, false, wiz->m_Rotation[i], LC_PK_ROTATION);
		      pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
		      pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep,
						   m_bAnimation, false, true);

		      SystemPieceComboAdd(wiz->m_Info[i]->m_strDescription);
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

			for (i = 0; i < LC_MFW_NUMITEMS; i++)
			  if (wiz->m_Info[i])
			    wiz->m_Info[i]->DeRef();

      wiz->DecRef ();
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
//				SystemUpdateFocus(NULL, 255);
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
				SystemUpdateFocus(NULL, 0);
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
			m_pViewCameras[m_nActiveViewport]->DoZoom(nParam, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMIN:
		{
			m_pViewCameras[m_nActiveViewport]->DoZoom(-1, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMOUT:
		{
			m_pViewCameras[m_nActiveViewport]->DoZoom(1, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;

		case LC_VIEW_ZOOMEXTENTS:
		{
		  // FIXME: rewrite using the FustrumCull function
			if (m_pPieces == 0) break;
			bool bControl = Sys_KeyDown (KEY_CONTROL);

			GLdouble modelMatrix[16], projMatrix[16];
			float up[3], eye[3], target[3];
			float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
			GLint viewport[4], out, x, y, w, h;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
					pPiece->CompareBoundingBox(bs);

			float v[24] = {
				bs[0], bs[1], bs[5],
				bs[3], bs[1], bs[5],
				bs[0], bs[1], bs[2],
				bs[3], bs[4], bs[5],
				bs[3], bs[4], bs[2],
				bs[0], bs[4], bs[2],
				bs[0], bs[4], bs[5],
				bs[3], bs[1], bs[2] };

			for (int vp = 0; vp < viewports[m_nViewportMode].n; vp++)
			{
				Camera* pCam;
				if (bControl)
					pCam = m_pViewCameras[vp];
				else
					pCam = m_pViewCameras[m_nActiveViewport];

				if (!bControl)
					vp = m_nActiveViewport;

				x = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][0] * (float)m_nViewX);
				y = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][1] * (float)m_nViewY);
				w = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][2] * (float)m_nViewX);
				h = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][3] * (float)m_nViewY);
				float ratio = (float)w/h;
	
				glViewport(x,y,w,h);
				pCam->LoadProjection(ratio);

				if (!bControl)
					vp = 4;

				pCam->GetTargetPos (target);
				pCam->GetEyePos (eye);

				up[0] = (bs[0] + bs[3])/2 - target[0];
				up[1] = (bs[1] + bs[4])/2 - target[1];
				up[2] = (bs[2] + bs[5])/2 - target[2];

				if (pCam->IsSide())
				{
					eye[0] += up[0];
					eye[1] += up[1];
					eye[2] += up[2];
				}
				target[0] += up[0];
				target[1] += up[1];
				target[2] += up[2];

				pCam->GetUpVec (up);
				Vector upvec(up), frontvec(eye[0]-target[0], eye[1]-target[1], eye[2]-target[2]), sidevec;
				frontvec.Normalize();
				sidevec.Cross(frontvec, upvec);
				upvec.Cross(sidevec, frontvec);
				upvec.Normalize();
				upvec.ToFloat(up);
				frontvec.Scale(0.25f);

				glMatrixMode(GL_MODELVIEW);
				glGetDoublev(GL_PROJECTION_MATRIX,projMatrix);
				glGetIntegerv(GL_VIEWPORT,viewport);

				for (out = 0; out < 10000; out++) // Zoom in
				{
					eye[0] -= frontvec.X();
					eye[1] -= frontvec.Y();
					eye[2] -= frontvec.Z();
					glLoadIdentity();
					gluLookAt(eye[0], eye[1], eye[2], target[0], target[1], target[2], up[0], up[1], up[2]);
					glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);

					for (int i = 0; i < 24; i+=3)
					{
						double winx, winy, winz;
						gluProject (v[i], v[i+1], v[i+2], modelMatrix, projMatrix, viewport, &winx, &winy, &winz);
						if ((winx < viewport[0] + 1) || (winy < viewport[1] + 1) || 
							(winx > viewport[0] + viewport[2] - 1) || (winy > viewport[1] + viewport[3] - 1))
						{
							out = 10000;
							continue;
						}
					}
				}

				bool stp = false;
				for (out = 0; out < 10000 && !stp; out++) // zoom out
				{
					stp = true;
					eye[0] += frontvec.X();
					eye[1] += frontvec.Y();
					eye[2] += frontvec.Z();
					glLoadIdentity();
					gluLookAt(eye[0], eye[1], eye[2], target[0], target[1], target[2], up[0], up[1], up[2]);
					glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);

					for (int i = 0; i < 24; i+=3)
					{
						double winx, winy, winz;
						gluProject (v[i], v[i+1], v[i+2], modelMatrix, projMatrix, viewport, &winx, &winy, &winz);
						if ((winx < viewport[0] + 1) || (winy < viewport[1] + 1) || 
							(winx > viewport[0] + viewport[2] - 1) || (winy > viewport[1] + viewport[3] - 1))
						{
							stp = false;
							continue;
						}
					}
				}

				pCam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, eye, LC_CK_EYE);
				pCam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, target, LC_CK_TARGET);
				pCam->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
			}

			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;

		case LC_VIEW_VIEWPORTS:
		{
			// Safety check
			if (m_nActiveViewport >= viewports[nParam].n)
			{
				SystemUpdateCurrentCamera(m_pViewCameras[m_nActiveViewport], m_pViewCameras[0], m_pCameras);
				m_nActiveViewport = 0;
			}

			SystemUpdateViewport(nParam, m_nViewportMode);
			m_nViewportMode = (unsigned char)nParam;
			UpdateAllViews();
		} break;

		case LC_VIEW_STEP_NEXT:
		{
			if (m_bAnimation)
				m_nCurFrame++;
			else
				m_nCurStep++;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL, 0);
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
			SystemUpdateFocus(NULL, 0);
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
			SystemUpdateFocus(NULL, 0);
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
			SystemUpdateFocus(NULL, 0);
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
			if (IsDrawing())
				break;

			if (m_bAnimation)
				m_nCurFrame = (nParam < m_nTotalFrames) ? (unsigned short)nParam : m_nTotalFrames;
			else
				m_nCurStep = (nParam < 255) ? (unsigned char)nParam : 255;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL, 0);
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
			m_bRendering = true;
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
                                //				RenderScene((m_nDetail & LC_DET_FAST) == 0, true);
                                //				SystemSwapBuffers();
                                UpdateAllViews ();
				SystemPumpMessages();
			}
			m_bRendering = false;
			SystemUpdatePlay(true, false);
			SystemUpdateFocus(NULL, 0);
		} break;

		case LC_VIEW_CAMERA_MENU:
		{
			Camera* pCamera = m_pCameras;

			while (nParam--)
				pCamera = pCamera->m_pNext;

			SystemUpdateCurrentCamera(m_pViewCameras[m_nActiveViewport], pCamera, m_pCameras);
			m_pViewCameras[m_nActiveViewport] = pCamera;
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
				
				switch (i) 
				{
				case LC_CAMERA_MAIN:  m_pViewCameras[0] = pCamera; break;
				case LC_CAMERA_FRONT: m_pViewCameras[1] = pCamera; break;
				case LC_CAMERA_TOP:   m_pViewCameras[2] = pCamera; break;
				case LC_CAMERA_RIGHT: m_pViewCameras[3] = pCamera; break;
				}
			}

			SystemUpdateCameraMenu(m_pCameras);
			SystemUpdateCurrentCamera(NULL, m_pViewCameras[m_nActiveViewport], m_pCameras);
			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint("Reset Cameras");
		} break;

		case LC_VIEW_AUTOPAN:
		{
			if (IsDrawing())
				break;

			short x = (short)nParam;
			short y = (short)((nParam >> 16) & 0xFFFF);

			x -= x > 0 ? 5 : -5;
			y -= y > 0 ? 5 : -5;

			m_pViewCameras[m_nActiveViewport]->DoPan(x/4, y/4, 1, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownX = x;
			m_nDownY = y;
			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;

		case LC_HELP_ABOUT:
		{
		  SystemDoDialog(LC_DLG_ABOUT, 0);
		} break;

		case LC_TOOLBAR_ANIMATION:
		{
			m_bAnimation = !m_bAnimation;

			CalculateStep();
			SystemUpdateFocus(NULL, 0);
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
				m_nSnap |= LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y | LC_DRAW_SNAP_Z;
				break;
			case 4:
				m_nSnap &= ~(LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y | LC_DRAW_SNAP_Z);
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
				m_nSnap &= ~(LC_DRAW_LOCK_X|LC_DRAW_LOCK_Y|LC_DRAW_LOCK_Z);
				break;
			case 4:
				m_nSnap &= ~LC_DRAW_3DMOUSE;
				break;
			case 5:
				m_nSnap |= LC_DRAW_3DMOUSE;
				break;
			}
			SystemUpdateSnap(m_nSnap);
		} break;

		case LC_TOOLBAR_SNAPMOVEMENU:
		{
			if (nParam < 11)
				m_nMoveSnap = (unsigned short)nParam;
			else
			{
				if (nParam == 19)
					m_nMoveSnap = 100;
				else
					m_nMoveSnap = (unsigned short)(nParam - 10)*5 + 10;
			}
			SystemUpdateMoveSnap(m_nMoveSnap);
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
	}
}

void Project::SetAction(int nAction)
{
	SystemUpdateAction(nAction, m_nCurAction);
	m_nCurAction = nAction;
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

Camera* Project::GetCamera(int i)
{
	Camera* pCamera;

	for (pCamera = m_pCameras; i-- > 0 && pCamera; pCamera = pCamera->m_pNext)
		;
	return pCamera;
}

void Project::GetFocusPosition(float* pos)
{
	Piece* pPiece;
	Camera* pCamera;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsFocused())
		{
			pPiece->GetPosition(pos);
			if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
			{
				pos[0] /= 0.08f;
				pos[1] /= 0.08f;
				pos[2] /= 0.08f;
			}
			return;
		}

	for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
	{
		if (pCamera->IsEyeFocused())
		{
			pCamera->GetEyePos (pos);
			if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
			{
				pos[0] /= 0.08f;
				pos[1] /= 0.08f;
				pos[2] /= 0.08f;
			}
			return;
		}

		if (pCamera->IsTargetFocused())
		{
			pCamera->GetTargetPos (pos);
			if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
			{
				pos[0] /= 0.08f;
				pos[1] /= 0.08f;
				pos[2] /= 0.08f;
			}
			return;
		}
	}

	// TODO: light

	pos[0] = pos[1] = pos[2] = 0.0f;
}

void Project::FindObjectFromPoint(int x, int y, LC_CLICKLINE* pLine)
{
	GLdouble px, py, pz, rx, ry, rz;
	GLdouble modelMatrix[16], projMatrix[16];
	GLint viewport[4];
	Piece* pPiece;
	Camera* pCamera;
	Light* pLight;

	LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX,projMatrix);
	glGetIntegerv(GL_VIEWPORT,viewport);

	// Unproject the selected point against both the front and the back clipping plane
	gluUnProject(x, y, 0, modelMatrix, projMatrix, viewport, &px, &py, &pz);
	gluUnProject(x, y, 1, modelMatrix, projMatrix, viewport, &rx, &ry, &rz);

	pLine->a1 = px;
	pLine->b1 = py;
	pLine->c1 = pz;
	pLine->a2 = rx-px;
	pLine->b2 = ry-py;
	pLine->c2 = rz-pz;
	pLine->mindist = DBL_MAX;
	pLine->pClosest = NULL;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			pPiece->MinIntersectDist(pLine);

	for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		if (pCamera != m_pViewCameras[m_nActiveViewport])
			pCamera->MinIntersectDist(pLine);

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		pLight->MinIntersectDist(pLine);
}

/////////////////////////////////////////////////////////////////////////////
// Mouse handling

// Returns true if point is not inside the current viewport.
bool Project::SetActiveViewport(int px, int py)
{
	float x, y, w, h;
	int vp;

	for (vp = 0; vp < viewports[m_nViewportMode].n; vp++)
	{
		x = viewports[m_nViewportMode].dim[vp][0] * (float)m_nViewX;
		y = viewports[m_nViewportMode].dim[vp][1] * (float)m_nViewY;
		w = viewports[m_nViewportMode].dim[vp][2] * (float)m_nViewX;
		h = viewports[m_nViewportMode].dim[vp][3] * (float)m_nViewY;

		if (px > x && px < x + w && py > y && py < y + h)
		{
			if (m_nActiveViewport != vp)
			{
				SystemUpdateCurrentCamera(m_pViewCameras[m_nActiveViewport], m_pViewCameras[vp], m_pCameras);
				m_nActiveViewport = vp;
				Render(false);
//				glDrawBuffer(GL_FRONT);
//				RenderViewports(true);
//				glDrawBuffer(GL_BACK);
				UpdateAllViews();

				return true;
			}
			else
				return false;
		}
	}

	// We shouldn't get here...
	m_nActiveViewport = 0;
	return true;
}

void Project::LoadViewportProjection()
{
	int x, y, w, h;
	float ratio;

	x = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][0] * (float)m_nViewX);
	y = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][1] * (float)m_nViewY);
	w = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][2] * (float)m_nViewX);
	h = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][3] * (float)m_nViewY);

	ratio = (float)w/h;
	glViewport(x, y, w, h);
	m_pViewCameras[m_nActiveViewport]->LoadProjection(ratio);
}

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

	if (bAccept)
	{
		if (m_nCurAction == LC_ACTION_CAMERA)
		{
			SystemUpdateCameraMenu(m_pCameras);
			SystemUpdateCurrentCamera(NULL, m_pViewCameras[m_nActiveViewport], m_pCameras);
			SetModifiedFlag(true);
			CheckPoint("Inserting");
		}
		else if (m_nCurAction == LC_ACTION_CURVE)
		{
			SetModifiedFlag(true);
			CheckPoint("Inserting");
		}
		else if (m_nCurAction == LC_ACTION_SPOTLIGHT)
		{
			SetModifiedFlag(true);
			CheckPoint("Inserting");
		}
	}
	else if (m_pTrackFile != NULL)
	{
		DeleteContents (true);
		FileLoad (m_pTrackFile, true, false);
    delete m_pTrackFile;
    m_pTrackFile = NULL;
	}

	return true;
/*
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	if (bAccept)
	{
// ADD CHECKPOINT()

		if ((m_nCurAction == ACTION_ZOOM_REGION) && (m_ptTrack != pt))
		{
//	int m_nDownX;
//	int m_nDownY;
			Camera* pCam = m_pViewCameras[m_nActiveViewport];

			int out;
			double modelMatrix[16], projMatrix[16], Pos[3] = { 0,0,0 };
			int	 viewport[4];
			float eye[3], target[3], up[3];
			memcpy(eye, pCam->m_fEye, sizeof(eye));
			memcpy(target, pCam->m_fTarget, sizeof(target));
			memcpy(up, pCam->m_fUp, sizeof(up));
			double obj1x,obj1y,obj1z, obj2x,obj2y,obj2z;
			int x = (int)(viewports[m_nActiveViewport].dim[m_nActiveViewport][0] * ((float)m_szView.cx));
			int y = (int)(viewports[m_nActiveViewport].dim[m_nActiveViewport][1] * ((float)m_szView.cy));
			int w = (int)(viewports[m_nActiveViewport].dim[m_nActiveViewport][2] * ((float)m_szView.cx)); 
			int h = (int)(viewports[m_nActiveViewport].dim[m_nActiveViewport][3] * ((float)m_szView.cy));

			POINT pt1, pt2;
			pt1.x = pt.x;
			pt1.y = m_szView.cy - pt.y - 1;
			pt2.x = m_ptTrack.x;
			pt2.y = m_szView.cy - m_ptTrack.y - 1;

			if (pt1.x < x) pt1.x = x;
			if (pt1.y < y) pt1.y = y;
			if (pt1.x > x+w) pt1.x = x+w;
			if (pt1.y > y+h) pt1.y = y+h;

			float ratio = (float)w/h;
			glViewport(x,y,w,h);
			pCam->LoadProjection(ratio);

			glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);
			glGetDoublev(GL_PROJECTION_MATRIX,projMatrix);
			glGetIntegerv(GL_VIEWPORT,viewport);

			double line[4][3];
			gluUnProject((double)pt1.x,(double)pt1.y,0.9,modelMatrix,projMatrix,viewport,&line[0][0],&line[0][1],&line[0][2]);
			gluUnProject((double)pt1.x,(double)pt1.y,0,modelMatrix,projMatrix,viewport,&line[1][0],&line[1][1],&line[1][2]);
			gluUnProject((double)pt2.x,(double)pt2.y,0.9,modelMatrix,projMatrix,viewport,&line[2][0],&line[2][1],&line[2][2]);
			gluUnProject((double)pt2.x,(double)pt2.y,0,modelMatrix,projMatrix,viewport,&line[3][0],&line[3][1],&line[3][2]);

//		pCam->GetPosition(m_nCurStep, &eye[0],&target[0],&up[0]);

			gluUnProject((double)(viewport[0]+viewport[2]/2),(double)(viewport[1]+viewport[3]/2),0,modelMatrix,projMatrix,viewport,&obj1x,&obj1y,&obj1z);
			gluUnProject((double)(viewport[0]+viewport[2]/2),(double)(viewport[1]+viewport[3]/2),1,modelMatrix,projMatrix,viewport,&obj2x,&obj2y,&obj2z);

			double d = (2 * PointDistance(obj1x, obj1y, obj1z, obj2x, obj2y, obj2z));
			for (out = 0; out < 10000; out++) // Zoom in
			{
				eye[0] += (float)((obj2x-obj1x)/d);
				eye[1] += (float)((obj2y-obj1y)/d);
				eye[2] += (float)((obj2z-obj1z)/d);
				target[0] += (float)((obj2x-obj1x)/d);
				target[1] += (float)((obj2y-obj1y)/d);
				target[2] += (float)((obj2z-obj1z)/d);

				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				gluLookAt(eye[0], eye[1], eye[2], target[0], target[1], target[2], up[0], up[1], up[2]);

				glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);

				double px,py,pz; 
				gluUnProject((double)(viewport[0]+viewport[2]/2),(double)(viewport[1]+viewport[3]/2),0,modelMatrix,projMatrix,viewport,&px,&py,&pz);

				double u1 = ((obj2x-obj1x)*(px-line[0][0])+(obj2y-obj1y)*(py-line[0][1])+(obj2z-obj1z)*(pz-line[0][2]))
					/((obj2x-obj1x)*(px-line[1][0])+(obj2y-obj1y)*(py-line[1][1])+(obj2z-obj1z)*(pz-line[1][2]));
				double u2 = ((obj2x-obj1x)*(px-line[2][0])+(obj2y-obj1y)*(py-line[2][1])+(obj2z-obj1z)*(pz-line[2][2]))
					/((obj2x-obj1x)*(px-line[3][0])+(obj2y-obj1y)*(py-line[3][1])+(obj2z-obj1z)*(pz-line[3][2]));

				double winx, winy, winz;
				gluProject (line[0][0]+u1*(line[1][0]-line[0][0]), line[0][1]+u1*(line[1][1]-line[0][1]), line[0][2]+u1*(line[1][2]-line[0][2]), modelMatrix, projMatrix, viewport, &winx, &winy, &winz);
				if ((winx < viewport[0] + 1) || (winy < viewport[1] + 1) || 
					(winx > viewport[0] + viewport[2] - 1) || (winy > viewport[1] + viewport[3] - 1))
					out = 10000;

				gluProject (line[2][0]+u2*(line[3][0]-line[2][0]), line[2][1]+u2*(line[3][1]-line[2][1]), line[2][2]+u2*(line[3][2]-line[2][2]), modelMatrix, projMatrix, viewport, &winx, &winy, &winz);
				if ((winx < viewport[0] + 1) || (winy < viewport[1] + 1) || 
					(winx > viewport[0] + viewport[2] - 1) || (winy > viewport[1] + viewport[3] - 1))
					out = 10000;
			}
			pCam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKey, eye, CK_EYE);
			pCam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKey, target, CK_TARGET);
			pCam->UpdateInformation(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
//		m_pViewModeless->UpdatePosition(GetActiveCamera());
			InvalidateRect (NULL, FALSE);
		}

		if (m_ptTrack != pt)
		{
//				if (m_Lights.GetCount() == 8)
//					m_nCurAction = ACTION_SELECT;
// CheckPoint();
		}
	}
*/
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

void Project::SnapPoint (float *point, float *reminder) const
{
	int i;

	if (m_nSnap & LC_DRAW_SNAP_X)
	{
		i = (int)(point[0]/0.4f);

    if (reminder != NULL)
      reminder[0] = point[0] - (0.4f * i);

		point[0] = 0.4f * i;
	}

	if (m_nSnap & LC_DRAW_SNAP_Y)
	{
		i = (int)(point[1]/0.4f);

    if (reminder != NULL)
      reminder[1] = point[1] - (0.4f * i);

		point[1] = 0.4f * i;
	}

	if (m_nSnap & LC_DRAW_SNAP_Z)
	{
		i = (int)(point[2]/0.32f);

    if (reminder != NULL)
      reminder[2] = point[2] - (0.32f * i);

		point[2] = 0.32f * i;
	}
}

void Project::MoveSelectedObjects(float x, float y, float z)
{
	Piece* pPiece;
	Camera* pCamera;
	Light* pLight;

	if (m_nSnap & LC_DRAW_LOCK_X)
		x = 0;
	if (m_nSnap & LC_DRAW_LOCK_Y)
		y = 0;
	if (m_nSnap & LC_DRAW_LOCK_Z)
		z = 0;

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
}

void Project::RotateSelectedObjects(float x, float y, float z)
{
  if (m_nSnap & LC_DRAW_LOCK_X)
    x = 0;
  if (m_nSnap & LC_DRAW_LOCK_Y)
    y = 0;
  if (m_nSnap & LC_DRAW_LOCK_Z)
    z = 0;

  if (x == 0 && y == 0 && z == 0)
    return;

  float bs[6] = { 10000, 10000, 10000, -10000, -10000, -10000 };
  float pos[3], rot[4];
  int nSel = 0;
  Piece *pPiece, *pFocus = NULL;

  for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
    if (pPiece->IsSelected())
    {
      if (pPiece->IsFocused ())
        pFocus = pPiece;
      /*
        pPiece->GetPosition (pos);
        if (pos[0] < bs[0]) bs[0] = pos[0];
        if (pos[1] < bs[1]) bs[1] = pos[1];
        if (pos[2] < bs[2]) bs[2] = pos[2];
        if (pos[0] > bs[3]) bs[3] = pos[0];
        if (pos[1] > bs[4]) bs[4] = pos[1];
        if (pos[2] > bs[5]) bs[5] = pos[2];
      */
      pPiece->CompareBoundingBox (bs);
      nSel++;
    }

  if (pFocus != NULL)
  {
    pFocus->GetPosition (pos);
    bs[0] = bs[3] = pos[0];
    bs[1] = bs[4] = pos[1];
    bs[2] = bs[5] = pos[2];
  }

  for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
  {
    if (!pPiece->IsSelected())
      continue;

    pPiece->GetPosition(pos);
    pPiece->GetRotation(rot);
    Matrix m(rot, pos);

    if (nSel == 1)
    {
      if (!(m_nSnap & LC_DRAW_LOCK_X))
        m.Rotate(x,1,0,0);
      if (!(m_nSnap & LC_DRAW_LOCK_Y))
        m.Rotate(y,0,1,0);
      if (!(m_nSnap & LC_DRAW_LOCK_Z))
        m.Rotate(z,0,0,1);
    }
    else
    {
      if (!(m_nSnap & LC_DRAW_LOCK_X))
        m.RotateCenter(x,1,0,0,(bs[0]+bs[3])/2,(bs[1]+bs[4])/2,(bs[2]+bs[5])/2);
      if (!(m_nSnap & LC_DRAW_LOCK_Y))
        m.RotateCenter(y,0,1,0,(bs[0]+bs[3])/2,(bs[1]+bs[4])/2,(bs[2]+bs[5])/2);
      if (!(m_nSnap & LC_DRAW_LOCK_Z))
        m.RotateCenter(z,0,0,1,(bs[0]+bs[3])/2,(bs[1]+bs[4])/2,(bs[2]+bs[5])/2);
      m.GetTranslation(pos);

      // TODO: check if moved
      pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, pos, LC_PK_POSITION);
    }

    m.ToAxisAngle(rot);
    pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, rot, LC_PK_ROTATION);
    pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
/*
		for (POSITION pos2 = m_Pieces.GetHeadPosition(); pos2 != NULL;)
		{
			CPiece* tmp = m_Pieces.GetNext(pos2);
			if (tmp == pPiece)
				continue;

			if (pPiece->Collide(tmp))
				wprintf("Collision");
			else
				wprintf("No Collision");
		}
*/
  }

  for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
    if (pPiece->IsSelected())
      pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);
}

bool Project::OnKeyDown(char nKey, bool bControl, bool bShift)
{
	bool ret = false;

	if (IsDrawing())
		return false;

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
			}
			else
			{
				m_nMoveSnap = nKey - 0x30;
				SystemUpdateMoveSnap(m_nMoveSnap);
			}
			ret = true;
		} break;

		case 'F':
		if (!bControl)
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_FRONT);
			ret = true;
		} break;
		case 'B':
		if (!bControl)
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_BACK); break;
			ret = true;
		} break;
		case 'T':
		if (!bControl)
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_TOP); break;
			ret = true;
		} break;
		case 'U':
		if (!bControl)
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_UNDER); break;
			ret = true;
		} break;
		case 'L':
		if (!bControl)
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_LEFT); break;
			ret = true;
		} break;
		case 'R':
		if (!bControl)
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_RIGHT); break;
			ret = true;
		} break;
		case 'M':
		if (!bControl)
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_MAIN); break;
			ret = true;
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
			SystemUpdateFocus(pFocus, LC_PIECE|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
			ret = true;
		} break;

		case KEY_UP:    case KEY_DOWN: case KEY_LEFT: 
		case KEY_RIGHT: case KEY_NEXT: case KEY_PRIOR:
//		if (AnyObjectSelected(FALSE))
		{
			float axis[3];
			if (bShift)
			{
				if (m_nSnap & LC_DRAW_SNAP_A)
					axis[0] = axis[1] = axis[2] = m_nAngleSnap;
				else
					axis[0] = axis[1] = axis[2] = 1;
			}
			else
			{
				if (m_nMoveSnap > 0)
				{
					axis[0] = axis[1] = 0.8f * m_nMoveSnap;
					axis[2] = 0.32f * m_nMoveSnap;
				}
				else
				{
					axis[0] = axis[1] = 0.4f;
					axis[2] = 0.32f;
				}

				if ((m_nSnap & LC_DRAW_SNAP_X == 0) || bControl)
					axis[0] = 0.01f;
				if ((m_nSnap & LC_DRAW_SNAP_Y == 0) || bControl)
					axis[1] = 0.01f;
				if ((m_nSnap & LC_DRAW_SNAP_Z == 0) || bControl)
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
        Camera *camera = m_pViewCameras[m_nActiveViewport];

        if (camera->IsSide ())
        {
          Matrix mat;

          mat.CreateLookat (camera->GetEyePos (), camera->GetTargetPos (), camera->GetUpVec ());
          mat.SetTranslation (0, 0, 0);
          mat.Invert ();

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
				
          Vector vx((float)(p2[0] - p1[0]), (float)(p2[1] - p1[1]), 0);//p2[2] - p1[2] };
          Vector x(1, 0, 0);
          ax = vx.Angle(x);
				
          Vector vy((float)(p3[0] - p1[0]), (float)(p3[1] - p1[1]), 0);//p2[2] - p1[2] };
          Vector y(0, -1, 0);
          ay = vy.Angle(y);
				
          if (ax > 135)
            axis[0] = -axis[0];
				
          if (ay < 45)
            axis[1] = -axis[1];
				
          if (ax >= 45 && ax <= 135)
          {
            float tmp = axis[0];
            
            ax = vx.Angle(y);
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
				RotateSelectedObjects(axis[0], axis[1], axis[2]);
			else
				MoveSelectedObjects(axis[0], axis[1], axis[2]);
			UpdateAllViews();
			SetModifiedFlag(true);
			CheckPoint((bShift) ? "Rotating" : "Moving");
			SystemUpdateFocus(NULL, 0);
			ret = true;
		} break;
	}

	return ret;
}

void Project::OnLeftButtonDown(int x, int y, bool bControl, bool bShift)
{
  GLdouble modelMatrix[16], projMatrix[16], point[3];
  GLint viewport[4];

  if (IsDrawing())
    return;

  if (m_nTracking != LC_TRACK_NONE)
    if (StopTracking(false))
      return;

  if (SetActiveViewport(x, y))
    return;

  m_bTrackCancel = false;
  m_nDownX = x;
  m_nDownY = y;

  LoadViewportProjection();
  glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
  glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
  glGetIntegerv(GL_VIEWPORT, viewport);

  gluUnProject(x, y, 0.9, modelMatrix, projMatrix, viewport, &point[0], &point[1], &point[2]);
  m_fTrack[0] = (float)point[0]; m_fTrack[1] = (float)point[1]; m_fTrack[2] = (float)point[2];

  switch (m_nCurAction)
  {
    case LC_ACTION_SELECT:
    case LC_ACTION_ERASER:
    case LC_ACTION_PAINT:
    {
      LC_CLICKLINE ClickLine;
      FindObjectFromPoint (x, y, &ClickLine);

      if (m_nCurAction == LC_ACTION_SELECT) 
      {
	if (ClickLine.pClosest != NULL)
        {
	  switch (ClickLine.pClosest->GetType ())
	  {
	    case LC_OBJECT_PIECE:
	    {
	      Piece* pPiece = (Piece*)ClickLine.pClosest;
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
	      ClickLine.pClosest->Select (true, true, bControl);
	    } break;
	  }
        }
        else
          SelectAndFocusNone (bControl);

	UpdateSelection();
	UpdateAllViews();
	if (ClickLine.pClosest)
	  SystemUpdateFocus(ClickLine.pClosest, ClickLine.pClosest->GetType()|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
	else
	  SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
      }

      if ((m_nCurAction == LC_ACTION_ERASER) && (ClickLine.pClosest != NULL))
      {
	switch (ClickLine.pClosest->GetType ())
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

	    for (int i = 0; i < 4; i++)
	      if (pCamera == m_pViewCameras[i])
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
		  SystemUpdateCurrentCamera(NULL, m_pViewCameras[m_nActiveViewport], m_pCameras);
		  break;
		}
	    }
	  } break;

	  case LC_OBJECT_LIGHT:
	  case LC_OBJECT_LIGHT_TARGET:
	  { 
/*						pos = m_Lights.Find(pObject->m_pParent);
						m_Lights.RemoveAt(pos);
						delete pObject->m_pParent;
*/	  } break;
	}

	UpdateSelection();
	UpdateAllViews();
	SetModifiedFlag(true);
	CheckPoint("Deleting");
//				AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);
      }

      if ((m_nCurAction == LC_ACTION_PAINT) && (ClickLine.pClosest != NULL) && 
	  (ClickLine.pClosest->GetType() == LC_OBJECT_PIECE))
      {
	Piece* pPiece = (Piece*)ClickLine.pClosest;

	if (pPiece->GetColor() != m_nCurColor)
	{
	  bool bTrans = pPiece->IsTransparent();
	  pPiece->SetColor(m_nCurColor);
	  if (bTrans != pPiece->IsTransparent())
	    pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, true, true);

	  SetModifiedFlag(true);
	  CheckPoint("Painting");
	  SystemUpdateFocus(NULL, 0);
	  UpdateAllViews();
	}
      }
    } break;

    case LC_ACTION_INSERT:
    case LC_ACTION_LIGHT:
    {
      if (m_nCurAction == LC_ACTION_INSERT)
      {
        Piece* pPiece = new Piece(m_pCurPiece);
        SnapPoint (m_fTrack, NULL);
        pPiece->Initialize(m_fTrack[0], m_fTrack[1], m_fTrack[2], m_nCurStep, m_nCurFrame, m_nCurColor);

        SelectAndFocusNone(false);
        pPiece->CreateName(m_pPieces);
        AddPiece(pPiece);
        pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);
        pPiece->Select (true, true, false);
        UpdateSelection();
        SystemPieceComboAdd(m_pCurPiece->m_strDescription);
        SystemUpdateFocus(pPiece, LC_PIECE|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);

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

        SnapPoint (m_fTrack, NULL);
        pLight = new Light (m_fTrack[0], m_fTrack[1], m_fTrack[2]);

        SelectAndFocusNone (false);

        //	pLight->CreateName (m_pPieces);
        pLight->m_pNext = m_pLights;
        m_pLights = pLight;
        SystemUpdateFocus (pLight, LC_LIGHT|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
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

      double tmp[3];
      gluUnProject(x+1, y-1, 0.9, modelMatrix, projMatrix, viewport, &tmp[0], &tmp[1], &tmp[2]);
      SelectAndFocusNone(false);
      StartTracking(LC_TRACK_START_LEFT);
      pLight = new Light (m_fTrack[0], m_fTrack[1], m_fTrack[2], (float)tmp[0], (float)tmp[1], (float)tmp[2]);
      pLight->GetTarget ()->Select (true, true, false);
      pLight->m_pNext = m_pLights;
      m_pLights = pLight;
      UpdateSelection();
      UpdateAllViews();
      SystemUpdateFocus(pLight, LC_LIGHT|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
    } break;

    case LC_ACTION_CAMERA:
    {
      double tmp[3];
      gluUnProject(x+1, y-1, 0.9, modelMatrix, projMatrix, viewport, &tmp[0], &tmp[1], &tmp[2]);
      SelectAndFocusNone(false);
      StartTracking(LC_TRACK_START_LEFT);
      Camera* pCamera = new Camera(m_fTrack[0], m_fTrack[1], m_fTrack[2], (float)tmp[0], (float)tmp[1], (float)tmp[2], m_pCameras);
      pCamera->GetTarget ()->Select (true, true, false);
      UpdateSelection();
      UpdateAllViews();
      SystemUpdateFocus(pCamera, LC_CAMERA|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
    } break;

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
				StartTracking(LC_TRACK_START_LEFT);
        m_fTrack[0] = m_fTrack[1] = m_fTrack[2] = 0.0f;
      }
		} break;

		case LC_ACTION_ROTATE:
		{
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					StartTracking(LC_TRACK_START_LEFT);
          m_fTrack[0] = m_fTrack[1] = m_fTrack[2] = 0.0f;
					break;
				}
		} break;

		case LC_ACTION_ZOOM:
		case LC_ACTION_ROLL:
		case LC_ACTION_PAN:
		case LC_ACTION_ROTATE_VIEW:
		{
			StartTracking(LC_TRACK_START_LEFT);
		} break;

		case LC_ACTION_ZOOM_REGION:
		{
			SystemCaptureMouse();
			m_nTracking = LC_TRACK_START_LEFT;

      if (m_pTrackFile != NULL)
      {
        delete m_pTrackFile;
  			m_pTrackFile = NULL;
      }
		} break;
	}
}

void Project::OnLeftButtonDoubleClick(int x, int y, bool bControl, bool bShift)
{
  GLdouble modelMatrix[16], projMatrix[16], point[3];
  GLint viewport[4];

  if (IsDrawing())
    return;

  if (SetActiveViewport(x, y))
    return;

  LoadViewportProjection ();
  glGetDoublev (GL_MODELVIEW_MATRIX, modelMatrix);
  glGetDoublev (GL_PROJECTION_MATRIX, projMatrix);
  glGetIntegerv (GL_VIEWPORT, viewport);

  // why this is here ?
  gluUnProject(x, y, 0.9, modelMatrix, projMatrix, viewport, &point[0], &point[1], &point[2]);
  m_fTrack[0] = (float)point[0]; m_fTrack[1] = (float)point[1]; m_fTrack[2] = (float)point[2];

  LC_CLICKLINE ClickLine;
  FindObjectFromPoint (x, y, &ClickLine);

//  if (m_nCurAction == LC_ACTION_SELECT) 
  {
    SelectAndFocusNone(bControl);

    if (ClickLine.pClosest != NULL)
      switch (ClickLine.pClosest->GetType ())
      {
        case LC_OBJECT_PIECE:
        {
          Piece* pPiece = (Piece*)ClickLine.pClosest;
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
          ClickLine.pClosest->Select (true, true, bControl);
        } break;
      }

    UpdateSelection();
    UpdateAllViews();
    if (ClickLine.pClosest)
      SystemUpdateFocus(ClickLine.pClosest, ClickLine.pClosest->GetType()|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
    else
      SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
  }
}

void Project::OnLeftButtonUp(int x, int y, bool bControl, bool bShift)
{
  StopTracking(true);
}

void Project::OnRightButtonDown(int x, int y, bool bControl, bool bShift)
{
	GLdouble modelMatrix[16], projMatrix[16], point[3];
	GLint viewport[4];

	if (StopTracking(false))
		return;
	if (SetActiveViewport(x, y))
		return;

	m_nDownX = x;
	m_nDownY = y;
	m_bTrackCancel = false;

	LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluUnProject(x, y, 0.9, modelMatrix, projMatrix, viewport, &point[0], &point[1], &point[2]);
	m_fTrack[0] = (float)point[0]; m_fTrack[1] = (float)point[1]; m_fTrack[2] = (float)point[2];

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
	}
}

void Project::OnRightButtonUp(int x, int y, bool bControl, bool bShift)
{
	if (!StopTracking(true) && !m_bTrackCancel)
		SystemDoPopupMenu(1, -1, -1);
	m_bTrackCancel = false;
}

void Project::OnMouseMove(int x, int y, bool bControl, bool bShift)
{
	// && m_nAction != ACTION_INSERT
	if (m_nTracking == LC_TRACK_NONE)
		return;

	if (m_nTracking == LC_TRACK_START_RIGHT)
		m_nTracking = LC_TRACK_RIGHT;

	if (m_nTracking == LC_TRACK_START_LEFT)
		m_nTracking = LC_TRACK_LEFT;

	if (IsDrawing())
		return;

	GLdouble modelMatrix[16], projMatrix[16], tmp[3];
	GLint viewport[4];
	float ptx, pty, ptz;

	LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluUnProject(x, y, 0.9, modelMatrix, projMatrix, viewport, &tmp[0], &tmp[1], &tmp[2]);
	ptx = (float)tmp[0]; pty = (float)tmp[1]; ptz = (float)tmp[2];

	switch (m_nCurAction)
	{
		case LC_ACTION_INSERT:
			// TODO: handle action_insert (draw preview)
			break;

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

			SystemUpdateFocus(NULL, 0);
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

			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;

		case LC_ACTION_MOVE:
		{
      Camera *camera = m_pViewCameras[m_nActiveViewport];

      if (camera->IsSide ())
      {
        Matrix mat;
        float delta[3];

        mat.CreateLookat (camera->GetEyePos (), camera->GetTargetPos (), camera->GetUpVec ());
        mat.SetTranslation (0, 0, 0);
        mat.Invert ();

				if (m_nTracking == LC_TRACK_LEFT)
        {
          delta[0] = (float)(x - m_nDownX);
          delta[1] = (float)(y - m_nDownY);
          delta[2] = 0;
        }
        else
        {
          delta[0] = 0;
          delta[1] = 0;
          delta[2] = (float)(y - m_nDownY);
        }

        mat.TransformPoints (delta, 1);

  			float mouse = 0.25f/(21-m_nMouse);
        delta[0] = delta[0] * mouse + m_fTrack[0];
        delta[1] = delta[1] * mouse + m_fTrack[1];
        delta[2] = delta[2] * mouse + m_fTrack[2];

        SnapPoint (delta, m_fTrack);

				MoveSelectedObjects(delta[0], delta[1], delta[2]);

        m_nDownX = x;
  			m_nDownY = y;
      }
      else
      {
        // TODO: rewrite

        float mouse = 5.0f/(21-m_nMouse);
        float delta[3] = {
          (ptx - m_fTrack[0])*mouse,
          (pty - m_fTrack[1])*mouse,
          (ptz - m_fTrack[2])*mouse };
        float d[3] = { delta[0], delta[1], delta[2] };

  			SnapPoint (delta, NULL);

  			m_fTrack[0] = ptx + (delta[0]-d[0])/mouse;
	  		m_fTrack[1] = pty + (delta[1]-d[1])/mouse;
		  	m_fTrack[2] = ptz + (delta[2]-d[2])/mouse;

  			if (m_nSnap & LC_DRAW_3DMOUSE)
	  			MoveSelectedObjects(delta[0], delta[1], delta[2]);
		  	else
			  {
				  if (m_nTracking == LC_TRACK_LEFT)
					  MoveSelectedObjects(delta[0], delta[1], 0);
  				else
	  				MoveSelectedObjects(0, 0, delta[2]);
		  	}
      }

			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;
		
		case LC_ACTION_ROTATE:
		{
      Camera *camera = m_pViewCameras[m_nActiveViewport];

      if (camera->IsSide ())
      {
        Matrix mat;
        float delta[3];

        mat.CreateLookat (camera->GetEyePos (), camera->GetTargetPos (), camera->GetUpVec ());
        mat.SetTranslation (0, 0, 0);
        mat.Invert ();

				if (m_nTracking == LC_TRACK_LEFT)
        {
          delta[0] = (float)(x - m_nDownX);
          delta[1] = (float)(y - m_nDownY);
          delta[2] = 0;
        }
        else
        {
          delta[0] = 0;
          delta[1] = 0;
          delta[2] = (float)(y - m_nDownY);
        }

        mat.TransformPoints (delta, 1);

  			float mouse = 36.0f/(21-m_nMouse);
  			ldiv_t result;

	  		for (int i = 0; i < 3; i++)
  		  	if (m_nSnap & LC_DRAW_SNAP_A)
	  		  {
            delta[i] = delta[i] * mouse + m_fTrack[i];
			  	  result = ldiv ((long)delta[i], m_nAngleSnap);
  			  	delta[i] = (float)(result.quot * m_nAngleSnap);
            m_fTrack[i] = (float)(result.rem);
  	  		}
	  	  	else
		  	  {
            delta[i] = delta[i] * mouse + m_fTrack[i];
			  	  result = ldiv ((long)delta[i], 1);
		  	  	delta[i] = (float)(result.quot);
            m_fTrack[i] = (float)(result.rem);
  		  	}

				RotateSelectedObjects (delta[0], delta[1], delta[2]);

        m_nDownX = x;
  			m_nDownY = y;
      }
      else
      {

			// TODO: rewrite

			float mouse = 360.0f/(21-m_nMouse);
			float delta[3] = {
				(ptx - m_fTrack[0])*mouse,
				(pty - m_fTrack[1])*mouse,
				(ptz - m_fTrack[2])*mouse };
			float d[3] = { delta[0], delta[1], delta[2] };

			ldiv_t result;
			for (int i = 0; i < 3; i++)
			if (m_nSnap & LC_DRAW_SNAP_A)
			{
				result = ldiv ((long)delta[i], m_nAngleSnap);
				delta[i] = (float)(result.quot * m_nAngleSnap);
			}
			else
			{
				result = ldiv ((long)delta[i], 1);
				delta[i] = (float)result.quot;
			}

			m_fTrack[0] = ptx + (delta[0]-d[0])/mouse;
			m_fTrack[1] = pty + (delta[1]-d[1])/mouse;
			m_fTrack[2] = ptz + (delta[2]-d[2])/mouse;

			if (m_nSnap & LC_DRAW_3DMOUSE)
				RotateSelectedObjects (delta[0], delta[1], delta[2]);
			else
			{
				if (m_nTracking == LC_TRACK_LEFT)
					RotateSelectedObjects (delta[0], delta[1], 0);
				else
					RotateSelectedObjects (0, 0, delta[2]);
			}
      }

			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;
		
		case LC_ACTION_ZOOM:
		{
			if (m_nDownY == y)
				break;

			m_pViewCameras[m_nActiveViewport]->DoZoom(y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownY = y;
			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;

		case LC_ACTION_ZOOM_REGION:
		{
			Render(false);

			glColor3f (0,0,0);
			glViewport(0, 0, m_nViewX, m_nViewY);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, m_nViewX, 0, m_nViewY, -1, 1);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			int vx = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][0] * ((float)m_nViewX));
			int vy = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][1] * ((float)m_nViewY));
			int vw = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][2] * ((float)m_nViewX)); 
			int vh = (int)(viewports[m_nViewportMode].dim[m_nActiveViewport][3] * ((float)m_nViewY));

			int rx, ry;
			rx = x;
			ry = y;

			if (rx < vx) rx = vx;
			if (ry < vy) ry = vy;
			if (rx > vx+vw) rx = vx+vw;
			if (ry > vy+vh) ry = vy+vh;

			glBegin (GL_LINE_LOOP);
			glVertex2i(rx, ry);
			glVertex2i(m_nDownX, ry);
			glVertex2i(m_nDownX, m_nDownY);
			glVertex2i(rx, m_nDownY);
			glEnd();

                        //			SystemSwapBuffers();
		} break;
		
		case LC_ACTION_PAN:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			m_pViewCameras[m_nActiveViewport]->DoPan(x - m_nDownX, y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownX = x;
			m_nDownY = y;
			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;
		
		case LC_ACTION_ROTATE_VIEW:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			// We can't rotate the side cameras.
			if (m_pViewCameras[m_nActiveViewport]->IsSide())
			{
				float eye[3], target[3], up[3];
				m_pViewCameras[m_nActiveViewport]->GetEyePos(eye);
				m_pViewCameras[m_nActiveViewport]->GetTargetPos(target);
				m_pViewCameras[m_nActiveViewport]->GetUpVec(up);
				Camera* pCamera = new Camera(eye, target, up, m_pCameras);

				m_pViewCameras[m_nActiveViewport] = pCamera;
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

			m_pViewCameras[m_nActiveViewport]->DoRotate(x - m_nDownX, y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, bs);
			m_nDownX = x;
			m_nDownY = y;
			SystemUpdateFocus(NULL, 0);
			UpdateAllViews();
		} break;
		
		case LC_ACTION_ROLL:
		{
			if (m_nDownX == x)
				break;

			m_pViewCameras[m_nActiveViewport]->DoRoll(x - m_nDownX, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownX = x;
			SystemUpdateFocus(NULL, 0);
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

      SystemUpdateFocus(NULL, 0);
      UpdateAllViews();
    } break;
                */
	}
}
