// Everything that is a part of a LeoCAD project goes here.
//

#ifdef LC_WINDOWS
#include "stdafx.h"
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
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

typedef struct
{
	unsigned char n;
	float dim [4][4];
} VIEWPORT;

static VIEWPORT viewports[14] = {
  { 1,  {{0, 0,    1,    1}, {    0,    0,    0,    0}, {    0,    0,    0,    0}, {    0,    0,    0,    0} }}, // 1
  { 2,  {{0, 0, 0.5f,    1}, { 0.5f,    0, 0.5f,    1}, {    0,    0,    0,    0}, {    0,    0,    0,    0} }}, // 2V
  { 2,  {{0, 0,    1, 0.5f}, {    0, 0.5f,    1, 0.5f}, {    0,    0,    0,    0}, {    0,    0,    0,    0} }}, // 2H
  { 2,  {{0, 0,    1, 0.7f}, {    0, 0.7f,    1, 0.3f}, {    0,    0,    0,    0}, {    0,    0,    0,    0} }}, // 2HT
  { 2,  {{0, 0,    1, 0.3f}, {    0, 0.3f,    1, 0.7f}, {    0,    0,    0,    0}, {    0,    0,    0,    0} }}, // 2HB
  { 3,  {{0, 0, 0.5f, 0.5f}, {    0, 0.5f, 0.5f, 0.5f}, { 0.5f,    0, 0.5f,    1}, {    0,    0,    0,    0} }}, // 3VL
  { 3,  {{0, 0, 0.5f,    1}, { 0.5f,    0, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f, 0.5f}, {    0,    0,    0,    0} }}, // 3VR
  { 3,  {{0, 0,    1, 0.5f}, {    0, 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f, 0.5f}, {    0,    0,    0,    0} }}, // 3HB
  { 3,  {{0, 0, 0.5f, 0.5f}, { 0.5f,    0, 0.5f, 0.5f}, {    0, 0.5f,    1, 0.5f}, {    0,    0,    0,    0} }}, // 3HT
  { 4,  {{0, 0, 0.3f, 0.3f}, {    0, 0.3f, 0.3f, 0.4f}, {    0, 0.7f, 0.3f, 0.3f}, { 0.3f,    0, 0.7f,    1} }}, // 4VL
  { 4,  {{0, 0, 0.7f,    1}, { 0.7f,    0, 0.3f, 0.3f}, { 0.7f, 0.3f, 0.3f, 0.4f}, { 0.7f, 0.7f, 0.3f, 0.3f} }}, // 4VR
  { 4,  {{0, 0,    1, 0.7f}, {    0, 0.7f, 0.3f, 0.3f}, { 0.3f, 0.7f, 0.4f, 0.3f}, { 0.7f, 0.7f, 0.3f, 0.3f} }}, // 4HT
  { 4,  {{0, 0, 0.3f, 0.3f}, { 0.3f,    0, 0.4f, 0.3f}, { 0.7f,    0, 0.3f, 0.3f}, {    0, 0.3f,    1, 0.7f} }}, // 4HB
  { 4,  {{0, 0, 0.5f, 0.5f}, { 0.5f,    0, 0.5f, 0.5f}, {    0, 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f, 0.5f} }}};// 4

typedef struct 
{
	unsigned char width;
	float left, right, top, bottom;
} TXFVERT;

static TXFVERT glyphs[93];

/////////////////////////////////////////////////////////////////////////////
// Project construction/destruction

Project::Project()
{
	m_bModified = false;
	m_bTrackCancel = false;
	m_nTracking = LC_TRACK_NONE;
	m_pPieceIdx = NULL;
	m_nPieceCount = 0;
	m_pTextures = NULL;
	m_nTextureCount = 0;
	m_pPieces = NULL;
	m_pCameras = NULL;
	m_pLights = NULL;
	m_pGroups = NULL;
	m_pMovedReference = NULL;
	m_nMovedCount = 0;
	m_pUndoList = NULL;
	m_pRedoList = NULL;
	m_nGridList = 0;
	m_nCurClipboard = 0;
	m_pTerrain = new Terrain();
	m_pBackground = new Texture();
	m_nAutosave = SystemGetProfileInt("Settings", "Autosave", 10);
	m_nMouse = SystemGetProfileInt("Default", "Mouse", 11);
	strcpy(m_strModelsPath, SystemGetProfileString("Default", "Projects", ""));

	int i;
	for (i = 0; i < LC_CONNECTIONS; i++)
	{
		m_pConnections[i].entries = NULL;
		m_pConnections[i].numentries = 0;
	}

	for (i = 0; i < 10; i++)
		m_pClipboard[i] = NULL;

	char entry[8];
	for (i = 0; i < 4; i++)
	{
		sprintf(entry, "File%d", i+1);
		strcpy(m_strRecentFiles[i], SystemGetProfileString("RecentFiles", entry, ""));
	}

	// Create font table
	float inv = 1.0f/128;
	char *charlines[16] = { 
	"abcdefghijklmn", "opqrstuvwxyz0", "123456789ABC", "DEFGHIJKLMN", 
	"OPQRSTUVWX", "YZ,.!;:<>/?{}@$%", "&*()-+=_[] #" };
	unsigned char lefts[7][17] = { 
	{ 1, 11, 21, 30, 40, 50, 56, 66, 76, 80, 84, 93, 97, 111, 121 },
	{ 1, 11, 21, 31, 38, 47, 53, 63, 72, 83, 94, 103, 111, 120 },
	{ 1, 10, 19, 28, 37, 46, 55, 64, 73, 82, 94, 106, 118, },
	{ 1, 13, 24, 34, 47, 59, 64, 73, 84, 94, 108, 120 },
	{ 1, 14, 25, 38, 50, 61, 71, 83, 94, 109, 120 },
	{ 1, 12, 22, 26, 30, 35, 39, 43, 52, 61, 65, 75, 81, 87, 103, 112, 125 },
	{ 3, 14, 23, 28, 33, 38, 47, 56, 65, 70, 75, 79, 88 } };
	// tops = 1 20 39 58 77 96 112 (+16)
	memset(glyphs, 0, sizeof(glyphs));

	// ASCII 32-125
	for (i = 32; i < 126; i++)
	for (int x = 0; x < 7; x++)
	for (int y = 0; charlines[x][y]; y++)
	if (charlines[x][y] == i)
	{
		glyphs[i-32].width = lefts[x][y+1] - lefts[x][y];
		glyphs[i-32].left = (float)lefts[x][y]*inv;
		glyphs[i-32].right = (float)(lefts[x][y+1])*inv;
		if (x != 6)
			glyphs[i-32].top = (float)(1 + 19*x);
		else
			glyphs[i-32].top = 112;
		glyphs[i-32].bottom = glyphs[i-32].top + 16;
		glyphs[i-32].top *= inv;
		glyphs[i-32].bottom *= inv;
	}
}

Project::~Project()
{
	DeleteContents(false);
	SystemFinish();

	if (m_pPieceIdx != NULL)
	{
		PieceInfo* pInfo;
		for (pInfo = m_pPieceIdx; m_nPieceCount--; pInfo++)
			pInfo->~PieceInfo();
		delete [] m_pPieceIdx;
	}

	if (m_pTextures != NULL)
	{
		Texture* pTexture;
		for (pTexture = m_pTextures; m_nTextureCount--; pTexture++)
			pTexture->~Texture();
		delete [] m_pTextures;
	}

	if (m_pMovedReference != NULL)
		free(m_pMovedReference);

	int i;
	char entry[8];
	for (i = 0; i < 4; i++)
	{
		sprintf(entry, "File%d", i+1);
//		if (strlen(m_strRecentFiles[i]) > 0)
			SystemSetProfileString("RecentFiles", entry, m_strRecentFiles[i]);
	}

	for (i = 0; i < 10; i++)
		if (m_pClipboard[i] != NULL)
			delete m_pClipboard[i];

	delete m_pTerrain;
	delete m_pBackground;
}


/////////////////////////////////////////////////////////////////////////////
// Project attributes, general services

// The main window should be created before calling this
bool Project::Initialize(int argc, char *argv[], char* libpath)
{
	m_LibraryPath = libpath;
	m_strPathName[0] = 0;

	LC_IMAGE_OPTS imopts;
	char picture[LC_MAXPATH];
	picture[0] = 0;

	unsigned long image = SystemGetProfileInt ("Default", "Image Options", 1|LC_IMAGE_TRANSPARENT);
	int width = SystemGetProfileInt("Default", "Image Width", 640);
	int height = SystemGetProfileInt("Default", "Image Height", 480);
//	int width = SystemGetProfileInt("Default", "Image Width", GetSystemMetrics(SM_CXSCREEN));
//	int height = SystemGetProfileInt("Default", "Image Height", GetSystemMetrics(SM_CYSCREEN));
	imopts.quality = SystemGetProfileInt("Default", "JPEG Quality", 70);
	imopts.interlaced = (image & LC_IMAGE_PROGRESSIVE) != 0;
	imopts.transparent = (image & LC_IMAGE_TRANSPARENT) != 0;
	imopts.truecolor = (image & LC_IMAGE_HIGHCOLOR) != 0;
	imopts.format = (unsigned char)(image & ~(LC_IMAGE_MASK));

	for (int i = 1; i < argc; i++)
	{
		char* param = argv[i];

		if (argv[i][0] == '-')
		{
			++param;

			if ((strcmp (param, "l") == 0) && ((i+1) < argc))
			{
				i++;
				m_LibraryPath = argv[i];
			}

			if (strcmp (param, "i") == 0)
			{
				if (((i+1) != argc) && (argv[i+1][0] != '-'))
				{
					i++;
					strcpy(picture, argv[i]);
				}
				else
					picture[0] = 1;
			}

			if ((strcmp (param, "w") == 0) && ((i+1) < argc))
			{
				int w;
				i++;
				if (sscanf(argv[i], "%d", &w) == 1)
					width = w;
			}

			if ((strcmp (param, "h") == 0) && ((i+1) < argc))
			{
				int h;
				i++;
				if (sscanf(argv[i], "%d", &h) == 1)
					height = h;
			}
		}
		else
		{
			strcpy(m_strPathName, param);
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

	if (LoadPieceLibrary() == false)
	{
#ifdef LC_WINDOWS
		SystemDoMessageBox("Cannot load piece library.", LC_MB_OK|LC_MB_ICONERROR);
#else
		printf("Cannot load piece library !\n");
#endif
		return false;
	}

	SystemInit();

	if (strlen(m_strPathName) && OnOpenDocument(m_strPathName))
	{
		SetPathName(m_strPathName, true);

		if (picture[0] != 0)
		{
			bool need_ext = false;

			if (picture[0] == 1)
			{
				strcpy(picture, m_strPathName);
				char *p = strrchr(picture, '.');
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

			LC_IMAGE* image;
			CreateImages(&image, width, height, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation ? m_nCurFrame : m_nCurStep, false);
			SaveImage(picture, image, &imopts);
			free(image);

			return false;
		}
	}
	else
		OnNewDocument();

	return true;
}

// Load the piece library
bool Project::LoadPieceLibrary()
{
	FileDisk idx;
	char filename[LC_MAXPATH];
	unsigned char version;
	unsigned short count, movedcount;
	unsigned long binsize;
	PieceInfo* pElements;
	Texture* pTexture;
	int i;

	// Make sure that the path ends with a '/'
	i = strlen(m_LibraryPath)-1;
	if ((m_LibraryPath[i] != '\\') && (m_LibraryPath[i] != '/'))
	  strcat(m_LibraryPath, "/");

	// Read the piece library index.
	strcpy(filename, m_LibraryPath);
	strcat(filename, "pieces.idx");

	if (!idx.Open(filename, "rb"))
		return false;

	idx.Seek(-(long)(2*sizeof(count)+sizeof(binsize)), SEEK_END);
	idx.Read(&movedcount, sizeof(movedcount));
	idx.Read(&binsize, sizeof(binsize));
	idx.Read(&count, sizeof(count));
	idx.Seek(32, SEEK_SET);
	idx.Read(&version, sizeof(version));

	if ((version != 3) || (count == 0))
	{
		idx.Close();
		return false;
	}
	idx.Seek(34, SEEK_SET); // skip update byte

	// TODO: check .bin file size.
	
	if (m_pPieceIdx != NULL)
	{
		// call the destructors
		for (pElements = m_pPieceIdx; m_nPieceCount--; pElements++)
			pElements->~PieceInfo();
		delete [] m_pPieceIdx;
	}

	m_pPieceIdx = new PieceInfo[count];
	m_nPieceCount = count;
	memset(m_pPieceIdx, 0, count * sizeof(PieceInfo));

	for (pElements = m_pPieceIdx; count--; pElements++)
		pElements->LoadIndex(&idx);

	// Load moved files reference.
	if (m_pMovedReference != NULL)
		free(m_pMovedReference);
	m_pMovedReference = (char*)malloc(18*movedcount);
	memset(m_pMovedReference, 0, 18*movedcount);
	m_nMovedCount = movedcount;

	for (i = 0; i < movedcount; i++)
	{
		idx.Read(&m_pMovedReference[i*18], 8);
		idx.Read(&m_pMovedReference[i*18+9], 8);
	}

	idx.Close();

	// TODO: Load group configuration here

	// Read the texture index.
	strcpy(filename, m_LibraryPath);
	strcat(filename, "textures.idx");

	if (m_pTextures != NULL)
	{
		// call the destructors
		for (pTexture = m_pTextures; m_nTextureCount--; pTexture++)
			pTexture->~Texture();
		delete [] m_pTextures;
	
		m_pTextures = NULL;
		m_nTextureCount = 0;
	}

	if (!idx.Open(filename, "rb"))
		return false;

	idx.Seek(-(long)(sizeof(count)+sizeof(binsize)), SEEK_END);
	idx.Read(&binsize, sizeof(binsize));
	idx.Read(&count, sizeof(count));
	idx.Seek(32, SEEK_SET);
	idx.Read(&version, sizeof(version));

	if ((version != 1) || (count == 0))
	{
		idx.Close();
		return false;
	}
	idx.Seek(34, SEEK_SET); // skip update byte

	// TODO: check .bin file size.

	m_pTextures = new Texture[count];
	m_nTextureCount = count;
	memset(m_pTextures, 0, count * sizeof(Texture));

	for (pTexture = m_pTextures; count--; pTexture++)
		pTexture->LoadIndex(&idx);

	idx.Close();

	return true;
}

void Project::SetTitle(char* lpszTitle)
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
		UNDOINFO* pUndo;

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
	m_nDetail = SystemGetProfileInt("Default", "Detail", LC_DET_BRICKEDGES);
	SystemUpdateRenderingMode((m_nDetail & LC_DET_BACKGROUND) != 0, (m_nDetail & LC_DET_FAST) != 0);
	m_nAngleSnap = (unsigned short)SystemGetProfileInt("Default", "Angle", 30);
	m_nSnap = SystemGetProfileInt("Default", "Snap", LC_DRAW_SNAP_A | LC_DRAW_SNAP_X | LC_DRAW_SNAP_Y | LC_DRAW_SNAP_Z | LC_DRAW_MOVE | LC_DRAW_PREVIEW);
	SystemUpdateSnap(m_nSnap);
	m_nMoveSnap = 0;
	SystemUpdateMoveSnap(m_nMoveSnap);
    m_fLineWidth = (float)SystemGetProfileInt("Default", "Line", 100)/100;
	m_fFogDensity = (float)SystemGetProfileInt("Default", "Density", 10)/100;
	rgb = SystemGetProfileInt("Default", "Fog", 0xFFFFFF);
	m_fFogColor[0] = (float)((unsigned char) (rgb))/255;
	m_fFogColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fFogColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_fFogColor[3] = 1.0f;
	m_nGridSize = (unsigned short)SystemGetProfileInt("Default", "Grid", 20);
	rgb = SystemGetProfileInt("Default", "Ambient", 0x4B4B4B);
	m_fAmbient[0] = (float)((unsigned char) (rgb))/255;
	m_fAmbient[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fAmbient[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_fAmbient[3] = 1.0f;
	rgb = SystemGetProfileInt("Default", "Background", 0xFFFFFF);
	m_fBackground[0] = (float)((unsigned char) (rgb))/255;
	m_fBackground[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fBackground[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_fBackground[3] = 1.0f;
	rgb = SystemGetProfileInt("Default", "Gradient1", 0xBF0000);
	m_fGradient1[0] = (float)((unsigned char) (rgb))/255;
	m_fGradient1[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fGradient1[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	rgb = SystemGetProfileInt("Default", "Gradient2", 0xFFFFFF);
	m_fGradient2[0] = (float)((unsigned char) (rgb))/255;
	m_fGradient2[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fGradient2[2] = (float)((unsigned char) ((rgb) >> 16))/255;
	m_nFPS = SystemGetProfileInt("Default", "FPS", 24);
	m_nCurStep = 1;
	m_nCurFrame = 1;
	m_nTotalFrames = 100;
	SystemUpdateTime(false, 1, 255);
	m_nScene = SystemGetProfileInt("Default", "Scene", 0);
	m_nSaveTimer = 0;
	strcpy(m_strHeader, SystemGetProfileString("Default", "Header", ""));
	strcpy(m_strFooter, SystemGetProfileString("Default", "Footer", "Page &P"));
	strcpy(m_strBackground, SystemGetProfileString("Default", "BMP", ""));
	m_pTerrain->LoadDefaults((m_nDetail & LC_DET_LINEAR) != 0);
	RenderInitialize();

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
	if (fv > 0.4f)
		file->Read(&fv, sizeof(fv));

	file->Read(&rgb, sizeof(rgb));
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
		file->Read(&eye, sizeof(eye));
		file->Read(&target, sizeof(target));
		float tmp[3] = { (float)eye[0], (float)eye[1], (float)eye[2] };
		pCam->ChangeKey(1, false, false, tmp, CK_EYE);
		pCam->ChangeKey(1, true, false, tmp, CK_EYE);
		tmp[0] = (float)target[0]; tmp[1] = (float)target[1]; tmp[2] = (float)target[2];
		pCam->ChangeKey(1, false, false, tmp, CK_TARGET);
		pCam->ChangeKey(1, true, false, tmp, CK_TARGET);

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
		pCam->ChangeKey(1, false, false, tmp, CK_UP);
		pCam->ChangeKey(1, true, false, tmp, CK_UP);
	}

	if (bMerge)
		file->Seek(32, SEEK_CUR);
	else
	{
		file->Read(&i, 4); m_nAngleSnap = i;
		file->Read(&m_nSnap, 4);
		file->Read(&m_fLineWidth, 4);
		file->Read(&m_nDetail, 4);
		file->Read(&i, 4); m_nCurGroup = i;
		file->Read(&i, 4); m_nCurColor = i;
		file->Read(&i, 4); action = i;
		file->Read(&i, 4); m_nCurStep = i;
	}

	if (fv > 0.8f)
		file->Read(&m_nScene, sizeof(m_nScene));

	file->Read(&count, sizeof(count));
	while (count--)
	{	
		if (fv > 0.4f)
		{
			char name[9];
			Piece* pPiece = new Piece(NULL);
			pPiece->FileLoad(file, name);
			PieceInfo* pInfo = FindPieceInfo(name);
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
		
			file->Read(pos, sizeof(pos));
			file->Read(rot, sizeof(rot));
			file->Read(&color, sizeof(color));
			file->Read(name, sizeof(name));
			file->Read(&step, sizeof(step));
			file->Read(&group, sizeof(group));

			const unsigned char conv[20] = { 0,2,4,9,7,6,22,8,10,11,14,16,18,9,21,20,22,8,10,11 };
			color = conv[color];

			PieceInfo* pInfo = FindPieceInfo(name);
			if (pInfo != NULL)
			{
				Piece* pPiece = new Piece(pInfo);
				Matrix mat;

				pPiece->Initialize(pos[0], pos[1], pos[2], step, 1, color);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				mat.CreateOld(0,0,0, rot[0],rot[1],rot[2]);
				mat.ToAxisAngle(param);
				pPiece->ChangeKey(1, false, false, param, PK_ROTATION);
				pPiece->ChangeKey(1, true, false, param, PK_ROTATION);
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
			if (ch == 0xFF) file->Read(&sh, 2); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
				file->Read(m_strAuthor, sh);

			file->Read(&ch, 1);
			if (ch == 0xFF) file->Read(&sh, 2); else sh = ch;
			if (sh > 100)
				file->Seek(sh, SEEK_CUR);
			else
				file->Read(m_strDescription, sh);
				
			file->Read(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->Read(&sh, 2); else sh = ch;
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
			file->Read(&ch, 1);
			if (ch == 0xFF) file->Read(&sh, 2); else sh = ch;
			file->Seek(sh, SEEK_CUR);

			file->Read(&ch, 1);
			if (ch == 0xFF) file->Read(&sh, 2); else sh = ch;
			file->Seek(sh, SEEK_CUR);
				
			file->Read(&ch, 1);
			if (ch == 0xFF && fv < 1.3f) file->Read(&sh, 2); else sh = ch;
			file->Seek(sh, SEEK_CUR);
		}
	}

	if (fv >= 0.5f)
	{
		file->Read(&count, sizeof(count));

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
				file->Read(&i, sizeof(i));
				m_nViewportMode = i;
			}
			else
			{
				file->Read(&m_nViewportMode, 1);
				file->Read(&m_nActiveViewport, 1);
			}

			file->Read(&count, 4);
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
					pCam->FileLoad(file);
				delete pCam;
			}
			else
				for (pCam = m_pCameras; pCam; pCam = pCam->m_pNext)
					pCam->FileLoad(file);
		}

		if (fv >= 0.7f)
		{
			for (count = 0; count < 4; count++)
			{
				file->Read(&i, 4);

				Camera* pCam = m_pCameras;
				while (i--)
					pCam = pCam->m_pNext;
				m_pViewCameras[count] = pCam;
			}

			file->Read(&rgb, sizeof(rgb));
			m_fFogColor[0] = (float)((unsigned char) (rgb))/255;
			m_fFogColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fFogColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv < 1.0f)
			{
				file->Read(&rgb, sizeof(rgb));
				m_fFogDensity = (float)rgb/100;
			}
			else
				file->Read(&m_fFogDensity, sizeof(m_fFogDensity));

			if (fv < 1.3f)
			{
				file->Read(&ch, 1);
				if (ch == 0xFF)
					file->Read(&sh, 2);
				sh = ch;
			}
			else
				file->Read(&sh, 2);

			if (sh < LC_MAXPATH)
				file->Read(m_strBackground, sh);
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
			file->Read(&rgb, sizeof(rgb));
			m_fAmbient[0] = (float)((unsigned char) (rgb))/255;
			m_fAmbient[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fAmbient[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv < 1.3f)
			{
				file->Read(&i, 4); m_bAnimation = (i != 0);
				file->Read(&i, 4); m_bAddKeys = (i != 0);
				file->Read(&m_nFPS, 1);
				file->Read(&i, 4); m_nCurFrame = i;
				file->Read(&m_nTotalFrames, 2);
				file->Read(&i, 4); m_nGridSize = i;
				file->Read(&i, 4); m_nMoveSnap = i;
			}
			else
			{
				file->Read(&ch, 1); m_bAnimation = (ch != 0);
				file->Read(&ch, 1); m_bAddKeys = (ch != 0);
				file->Read(&m_nFPS, 1);
				file->Read(&m_nCurFrame, 2);
				file->Read(&m_nTotalFrames, 2);
				file->Read(&m_nGridSize, 2);
				file->Read(&m_nMoveSnap, 2);
			}
		}
			
		if (fv > 1.0f)
		{
			file->Read(&rgb, sizeof(rgb));
			m_fGradient1[0] = (float)((unsigned char) (rgb))/255;
			m_fGradient1[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fGradient1[2] = (float)((unsigned char) ((rgb) >> 16))/255;
			file->Read(&rgb, sizeof(rgb));
			m_fGradient2[0] = (float)((unsigned char) (rgb))/255;
			m_fGradient2[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
			m_fGradient2[2] = (float)((unsigned char) ((rgb) >> 16))/255;

			if (fv > 1.1f)
				m_pTerrain->FileLoad(file);
			else
			{
				file->Seek(4, SEEK_CUR);
				file->Read(&ch, 1);
				file->Seek(ch, SEEK_CUR);
			}
		}
	}

	RenderInitialize();
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
	SystemRedrawView();

	return true;
}

void Project::FileSave(File* file, bool bUndo)
{
	float ver_flt = 1.3f; // LeoCAD 0.70
	unsigned long rgb;
	unsigned char ch;
	unsigned short sh;
	int i, j;

	file->Seek(0, SEEK_SET);
	file->Write(LC_STR_VERSION, 32);
	file->Write(&ver_flt, 4);

	rgb = FLOATRGB(m_fBackground);
	file->Write(&rgb, 4);

	i = m_nAngleSnap; file->Write(&i, 4);
	file->Write(&m_nSnap, 4);
	file->Write(&m_fLineWidth, 4);
	file->Write(&m_nDetail, 4);
	i = m_nCurGroup; file->Write(&i, 4);
	i = m_nCurColor; file->Write(&i, 4);
	i = m_nCurAction; file->Write(&i, 4);
	i = m_nCurStep; file->Write(&i, 4);
	file->Write(&m_nScene, sizeof(m_nScene));

	Piece* pPiece;
	for (i = 0, pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		i++;
	file->Write(&i, 4);

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		pPiece->FileSave(file, m_pGroups);

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
	file->Write(&i, 4);

	for (pGroup = m_pGroups; pGroup; pGroup = pGroup->m_pNext)
		pGroup->FileSave(file, m_pGroups);

	file->Write(&m_nViewportMode, 1);
	file->Write(&m_nActiveViewport, 1);

	Camera* pCamera;
	for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		i++;
	file->Write(&i, 4);

	for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		pCamera->FileSave(file);

	for (j = 0; j < 4; j++)
	{
		for (i = 0, pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
			if (pCamera == m_pViewCameras[j])
				break;
			else
				i++;

		file->Write(&i, 4);
	}

	rgb = FLOATRGB(m_fFogColor);
	file->Write(&rgb, 4);
	file->Write(&m_fFogDensity, 4);
	sh = strlen(m_strBackground);
	file->Write(&sh, 2);
	file->Write(m_strBackground, sh);
	ch = strlen(m_strHeader);
	file->Write(&ch, 1);
	file->Write(m_strHeader, ch);
	ch = strlen(m_strFooter);
	file->Write(&ch, 1);
	file->Write(m_strFooter, ch);
	// 0.60 (1.0)
	rgb = FLOATRGB(m_fAmbient);
	file->Write(&rgb, 4);
	ch = m_bAnimation;
	file->Write(&ch, 1);
	ch = m_bAddKeys;
	file->Write(&ch, 1);
	file->Write(&m_nFPS, 1);
	file->Write(&m_nCurFrame, 2);
	file->Write(&m_nTotalFrames, 2);
	file->Write(&m_nGridSize, 2);
	file->Write(&m_nMoveSnap, 2);
	// 0.62 (1.1)
	rgb = FLOATRGB(m_fGradient1);
	file->Write(&rgb, 4);
	rgb = FLOATRGB(m_fGradient2);
	file->Write(&rgb, 4);
	// 0.64 (1.2)
	m_pTerrain->FileSave(file);

	if (!bUndo)
	{
		unsigned long pos = 0;

		i = SystemGetProfileInt("Default", "Save Preview", 0);
		if (i != 0) 
		{
			pos = file->GetPosition();

			LC_IMAGE* image;
			LC_IMAGE_OPTS opts;
			opts.interlaced = false;
			opts.transparent = false;
			opts.format = LC_IMAGE_GIF;

			i = m_bAnimation ? m_nCurFrame : m_nCurStep;
			CreateImages(&image, 120, 100, i, i, false);
			SaveImage(file, image, &opts);
			free(image);
		}

		file->Write(&pos, 4);
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
		incmat.ConvertFromLDraw(fmat);
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

				PieceInfo* pInfo = FindPieceInfo(name);
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
					pPiece->ChangeKey(1, false, false, rot, PK_ROTATION);
					pPiece->ChangeKey(1, true, false, rot, PK_ROTATION);
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

		if (strcmp(ext, "dat") == 0)
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

			strcpy(newName, ".lcd");
		}

		if (!SystemDoDialog(LC_DLG_FILE_SAVE, &newName))
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

	if (strcmp(ext, "dat") == 0)
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
					mat.ConvertToLDraw(f);
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
	SetModifiedFlag(false); // make clean
	LoadDefaults(true);
	CheckPoint("");

	SystemUpdateRecentMenu(m_strRecentFiles);
	SystemUpdateFocus(NULL, LC_PIECE|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);

//	CWnd* pFrame = AfxGetMainWnd();
//	if (pFrame != NULL)
//		pFrame->PostMessage (WM_LC_UPDATE_LIST, 1, m_nCurColor+1);
// set cur group to 0
 
	return true;
}

bool Project::OnOpenDocument(char* lpszPathName)
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

	if (strcmp(ext, "dat") == 0)
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
			SystemRedrawView();

			char msg[50];
			sprintf(msg, "%d objects imported.", ok);
//			AfxMessageBox(msg, MB_OK|MB_ICONINFORMATION);
			bSuccess = true;
		}
		else
			bSuccess = FileLoad(&file, false, false); // load me

		CheckPoint("");
		m_nSaveTimer = 0;
	}
	file.Close();
	SystemDoWaitCursor(-1);

	if (bSuccess == false)
	{
//		MessageBox("Failed to load.");
		DeleteContents(false);   // remove failed contents
		return false;
	}

	SetModifiedFlag(false);     // start off with unmodified

	return true;
}

void Project::SetPathName(char* lpszPathName, bool bAddToMRU)
{
	strcpy(m_strPathName, lpszPathName);

	// always capture the complete file name including extension (if present)
	char* lpszTemp = lpszPathName;
	for (char* lpsz = lpszPathName; *lpsz != '\0'; lpsz++)
	{
		// remember last directory/drive separator
		if (*lpsz == '\\' || *lpsz == '/' || *lpsz == ':')
			lpszTemp = lpsz + 1;
	}

	// set the document title based on path name
	SetTitle(lpszTemp);

	// add it to the file MRU list
	if (bAddToMRU)
	{
		// update the MRU list, if an existing MRU string matches file name
		int i;
		for (i = 0; i < 3; i++)
		{
			if (strcmp(m_strRecentFiles[i], lpszPathName) == 0)
				break;
		}

		// move MRU strings before this one down
		for (; i > 0; i--)
			strcpy(m_strRecentFiles[i], m_strRecentFiles[i-1]);
		strcpy(m_strRecentFiles[0], lpszPathName);
		SystemUpdateRecentMenu(m_strRecentFiles);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Undo/Redo support

// Save current state.
void Project::CheckPoint(char* text)
{
	UNDOINFO* pTmp;
	UNDOINFO* pUndo = new UNDOINFO;
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
	SystemSwapBuffers();

	if ((m_nDetail & LC_DET_FAST) && (m_nDetail & LC_DET_BACKGROUND))
	{
		RenderScene(true, true);
		if (!m_bStopRender)
			SystemSwapBuffers();
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

/*
//		if ((m_dwDetail & DET_LIGHTING) != 0)
	for (POSITION pos = m_Lights.GetHeadPosition(); pos != NULL;)
	{
		CLight* pLight = m_Lights.GetNext(pos);

//			pLight->LoadPosition();
		glLightfv(GL_LIGHT0, GL_POSITION, pLight->m_fPosition);

float one[] = {1.f, 1.f, 1.f, 1.f};
glEnable(GL_LIGHT0);
//	glLightfv(GL_LIGHT0, GL_AMBIENT, one);
glLightfv(GL_LIGHT0, GL_SPECULAR, one);

	}
*/
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
				m_pTextures[0].MakeCurrent();
				glEnable(GL_TEXTURE_2D);
				glEnable(GL_ALPHA_TEST);

				TXFVERT* glyph;

				glPushMatrix();
				glTranslatef(1.4f*ds, 0, 0);
				glMultMatrixf(m.m);
				glyph = &glyphs['X'-32];
				glBegin(GL_QUADS);
				glTexCoord2f(glyph->left, glyph->top);
				glVertex2f(0, 0.4f*ds);
				glTexCoord2f(glyph->left, glyph->bottom);
				glVertex2f(0, -0.4f*ds);
				glTexCoord2f(glyph->right, glyph->bottom);
				glVertex2f(glyph->width*ds/20, -0.4f*ds);
				glTexCoord2f(glyph->right, glyph->top);
				glVertex2f(glyph->width*ds/20, 0.4f*ds);
				glEnd();
				glPopMatrix();

				glPushMatrix();
				glTranslatef(0, 1.4f*ds, 0);
				glMultMatrixf(m.m);
				glyph = &glyphs['Y'-32];
				glBegin(GL_QUADS);
				glTexCoord2f(glyph->left, glyph->top);
				glVertex2f(0, 0.4f*ds);
				glTexCoord2f(glyph->left, glyph->bottom);
				glVertex2f(0, -0.4f*ds);
				glTexCoord2f(glyph->right, glyph->bottom);
				glVertex2f(glyph->width*ds/20, -0.4f*ds);
				glTexCoord2f(glyph->right, glyph->top);
				glVertex2f(glyph->width*ds/20, 0.4f*ds);
				glEnd();
				glPopMatrix();

				glPushMatrix();
				glTranslatef(0, 0, 1.4f*ds);
				glMultMatrixf(m.m);
				glyph = &glyphs['Z'-32];
				glBegin(GL_QUADS);
				glTexCoord2f(glyph->left, glyph->top);
				glVertex2f(0, 0.4f*ds);
				glTexCoord2f(glyph->left, glyph->bottom);
				glVertex2f(0, -0.4f*ds);
				glTexCoord2f(glyph->right, glyph->bottom);
				glVertex2f(glyph->width*ds/20, -0.4f*ds);
				glTexCoord2f(glyph->right, glyph->top);
				glVertex2f(glyph->width*ds/20, 0.4f*ds);
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
				m_pViewCameras[vp]->GetEye(eye);
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
			Camera* pCamera;

			for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
			{
				if ((pCamera == m_pViewCameras[vp]) || !pCamera->IsVisible())
					continue;
				pCamera->Render(m_fLineWidth);
			}

			if ((m_nDetail & LC_DET_LIGHTING) && (m_pLights != NULL))
			{
//				Light* pLight;

				glDisable (GL_LIGHTING);
				glColor3f(1, 1, 0);

//				for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
//					pLight->Render();

				glEnable (GL_LIGHTING);
			}
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
//	glTranslatef(0.375, 0.375, 0.0);

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
		m_pTextures[0].MakeCurrent();
		glEnable(GL_ALPHA_TEST);
		glBegin(GL_QUADS);

		for (vp = 0; vp < viewports[m_nViewportMode].n; vp++)
		{
			x = viewports[m_nViewportMode].dim[vp][0] * (float)m_nViewX;
			y = viewports[m_nViewportMode].dim[vp][1] * (float)m_nViewY;
			w = viewports[m_nViewportMode].dim[vp][2] * (float)m_nViewX;
			h = viewports[m_nViewportMode].dim[vp][3] * (float)m_nViewY;

			float l = x+3, t = y+h-6;
			for (const char* p = m_pViewCameras[vp]->GetName(); *p; p++)
			{
				if (*p < 32 || *p > 125)
					continue;
				if (glyphs[*p-32].width == 0)
					continue;

				glTexCoord2f(glyphs[*p-32].left, glyphs[*p-32].top);
				glVertex2f(l, t);
				glTexCoord2f(glyphs[*p-32].left, glyphs[*p-32].bottom);
				glVertex2f(l, t-16);
				glTexCoord2f(glyphs[*p-32].right, glyphs[*p-32].bottom);
				glVertex2f(l + glyphs[*p-32].width, t-16);
				glTexCoord2f(glyphs[*p-32].right, glyphs[*p-32].top);
				glVertex2f(l + glyphs[*p-32].width, t);
				l += glyphs[*p-32].width;
			}
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
			w = viewports[m_nViewportMode].dim[vp][2] * (float)m_nViewX;
			h = viewports[m_nViewportMode].dim[vp][3] * (float)m_nViewY;

			glBegin(GL_LINE_LOOP);
			glVertex2f(x, y);
			glVertex2f(x+w, y);
			glVertex2f(x+w, y+h);
			glVertex2f(x, y+h);
			glEnd();
		}

		x = viewports[m_nViewportMode].dim[m_nActiveViewport][0] * (float)m_nViewX;
		y = viewports[m_nViewportMode].dim[m_nActiveViewport][1] * (float)m_nViewY;
		w = viewports[m_nViewportMode].dim[m_nActiveViewport][2] * (float)m_nViewX;
		h = viewports[m_nViewportMode].dim[m_nActiveViewport][3] * (float)m_nViewY;

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
		glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);
float spec[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 10);
// call initlights()
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

	// Load font (always the first texture)
	m_pTextures[0].Load(false);
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
void Project::CreateImages(LC_IMAGE** images, int width, int height, unsigned short from, unsigned short to, bool hilite)
{
	int oldx, oldy;
	unsigned short oldtime;
	void* render = SystemStartRender(width, height);
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
					pPiece->Select();
				else
					pPiece->UnSelect();
			}
		}

		CalculateStep();
		Render(true);

		images[i-from] = SystemGetRenderImage(render);
	}
//	pDoc->m_ViewCameras[pDoc->m_nActiveViewport] = pOld;
	m_nViewX = oldx;
	m_nViewY = oldy;
	if (m_bAnimation)
		m_nCurFrame = oldtime;
	else
		m_nCurStep = (unsigned char)oldtime;
	CalculateStep();
	SystemFinishRender(render);
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
	for (int j = 0; j < m_nPieceCount; j++)
	{
		bool Add = false;
		int count[LC_MAXCOLORS];
		memset (&count, 0, sizeof (count));
		pInfo = &m_pPieceIdx[j];

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

		case LC_PIECE_MODIFIED:
		{
			LC_PIECE_MODIFY* mod = (LC_PIECE_MODIFY*)param;
			Piece* pPiece = (Piece*)mod->piece;

			float pos[3], rot[4];
			pPiece->GetPosition(pos);
			pPiece->GetRotation(rot);
			Matrix mat(rot, pos);
			mat.GetEulerAngles(rot);
			
			if (mod->pos[0] != pos[0] || mod->pos[1] != pos[1] || mod->pos[2] != pos[2])
				pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->pos, PK_POSITION);

			if (mod->rot[0] != rot[0] || mod->rot[1] != rot[1] || mod->rot[2] != rot[2])
			{
				mat.FromEuler(mod->rot[0], mod->rot[1], mod->rot[2]);
				mat.ToAxisAngle(rot);
				pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, rot, PK_ROTATION);
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
			SystemRedrawView();
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

			pCamera->GetUp(tmp);

			if (tmp[0] != mod->eye[0] || tmp[1] != mod->eye[1] || tmp[2] != mod->eye[2])
				pCamera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->eye, CK_EYE);
			if (tmp[0] != mod->target[0] || tmp[1] != mod->target[1] || tmp[2] != mod->target[2])
				pCamera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->target, CK_TARGET);
			if (tmp[0] != mod->up[0] || tmp[1] != mod->up[1] || tmp[2] != mod->up[2])
				pCamera->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, mod->up, CK_UP);

			pCamera->m_fovy = mod->fovy;
			pCamera->m_zNear = mod->znear;
			pCamera->m_zFar = mod->zfar;
			pCamera->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
			SystemRedrawView();
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
			SystemRedrawView();
		} break;
		
		case LC_FILE_OPEN:
		{
			char filename[LC_MAXPATH];
			strcpy(filename, m_strModelsPath);

			if (SystemDoDialog(LC_DLG_FILE_OPEN, filename))
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

			if (SystemDoDialog(LC_DLG_FILE_MERGE, filename))
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

				LC_IMAGE** images;
				images = (LC_IMAGE**)malloc(sizeof(LC_IMAGE*)*(opts.to-opts.from+1));
				CreateImages(images, opts.width, opts.height, opts.from, opts.to, false);

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
					
					for (int i = 0; i <= opts.to-opts.from; i++)
						free(images[i]);
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
							sprintf(filename, "%s%02d.%s", opts.filename, i, ext+1);
						}
						else
							strcpy(filename, opts.filename);

						SaveImage(filename, images[i], &opts.imopts);
						free(images[i]);
					}
				}
				free(images);
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
			strcpy(opts.path, m_strPathName);
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
			unsigned long  image = SystemGetProfileInt ("Default", "HTML Options", 1|LC_IMAGE_TRANSPARENT);
			opts.imdlg.imopts.background[0] = (unsigned char)(m_fBackground[0]*255);
			opts.imdlg.imopts.background[1] = (unsigned char)(m_fBackground[1]*255);
			opts.imdlg.imopts.background[2] = (unsigned char)(m_fBackground[2]*255);
			opts.imdlg.from = 1;
			opts.imdlg.to = 1;
			opts.imdlg.multiple = false;
			opts.imdlg.width = SystemGetProfileInt("Default", "HTML Width", 256);
			opts.imdlg.height = SystemGetProfileInt("Default", "HTML Height", 160);
			opts.imdlg.imopts.quality = SystemGetProfileInt("Default", "JPEG Quality", 70);
			opts.imdlg.imopts.interlaced = (image & LC_IMAGE_PROGRESSIVE) != 0;
			opts.imdlg.imopts.transparent = (image & LC_IMAGE_TRANSPARENT) != 0;
			opts.imdlg.imopts.truecolor = (image & LC_IMAGE_HIGHCOLOR) != 0;
			opts.imdlg.imopts.pause = 1;
			opts.imdlg.imopts.format = (unsigned char)(image & ~(LC_IMAGE_MASK));

			if (SystemDoDialog(LC_DLG_HTML, &opts))
			{
				FILE* f;
				char* ext = ".bmp", fn[LC_MAXPATH];
				int i;
				unsigned short last = GetLastStep();

				switch (opts.imdlg.imopts.format)
				{
				case LC_IMAGE_BMP: ext = ".bmp"; break;
				case LC_IMAGE_GIF: ext = ".gif"; break;
				case LC_IMAGE_JPG: ext = ".jpg"; break;
				case LC_IMAGE_PNG: ext = ".png"; break;
				}
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
				if (opts.singlepage)
				{
					strcpy(fn, opts.path);
					strcat(fn, m_strTitle);
					strcat(fn, ".htm");
					f = fopen(fn, "wt");
					fprintf(f, "<HTML>\n<HEAD>\n<TITLE>Instructions for %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

					for (i = 1; i <= last; i++)
					{
						fprintf(f, "<IMG SRC=\"%s-%02d%s\" ALT=\"Step %02d\" WIDTH=%d HEIGHT=%d><BR><BR>\n", 
							m_strTitle, i, ext, i, opts.imdlg.width, opts.imdlg.height);
	
						if (opts.liststep)
							CreateHTMLPieceList(f, i, opts.images, ext);
					}

					if (opts.listend)
						CreateHTMLPieceList(f, 0, opts.images, ext);

					fputs("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.geocities.com/Colosseum/3479/leocad.htm\">LeoCAD</A></B></I><BR></HTML>\n", f);
					fclose(f);
				}
				else
				{
					if (opts.index)
					{
						strcpy(fn, opts.path);
						strcat(fn, m_strTitle);
						strcat(fn, "-index.htm");
						f = fopen(fn, "wt");

						fprintf(f, "<HTML>\n<HEAD>\n<TITLE>Instructions for %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);

						for (i = 1; i <= last; i++)
							fprintf(f, "<A HREF=\"%s-%02d.htm\">Step %d<BR>\n</A>", m_strTitle, i, i);

						if (opts.listend)
							fprintf(f, "<A HREF=\"%s-pieces.htm\">Pieces Used</A><BR>\n", m_strTitle);

						fputs("</CENTER>\n<BR><HR><BR><B><I>Created by <A HREF=\"http://www.geocities.com/Colosseum/3479/leocad.htm\">LeoCAD</A></B></I><BR></HTML>\n", f);
						fclose(f);
					}

					// Create each step
					for (i = 1; i <= last; i++)
					{
						sprintf(fn, "%s%s-%02d.htm", opts.path, m_strTitle, i);
						f = fopen(fn, "wt");

						fprintf(f, "<HTML>\n<HEAD>\n<TITLE>%s - Step %02d</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle, i);
						fprintf(f, "<IMG SRC=\"%s-%02d%s\" ALT=\"Step %02d\" WIDTH=%d HEIGHT=%d><BR><BR>\n", 
							m_strTitle, i, ext, i, opts.imdlg.width, opts.imdlg.height);
	
						if (opts.liststep)
							CreateHTMLPieceList(f, i, opts.images, ext);

						fputs("</CENTER>\n<BR><HR><BR>", f);
						if (i != 1)
							fprintf(f, "<A HREF=\"%s-%02d.htm\">Previous</A> ", m_strTitle, i-1);

						if (opts.index)
							fprintf(f, "<A HREF=\"%s-index.htm\">Index</A> ", m_strTitle);

						if (i != last)
							fprintf(f, "<A HREF=\"%s-%02d.htm\">Next</A>", m_strTitle, i+1);
						else
							if (opts.listend)
								fprintf(f, "<A HREF=\"%s-pieces.htm\">Pieces Used</A>", m_strTitle);

						fputs("<BR></HTML>\n",f);
						fclose(f);
					}

					if (opts.listend)
					{
						strcpy(fn, opts.path);
						strcat(fn, m_strTitle);
						strcat(fn, "-pieces.htm");
						f = fopen(fn, "wt");
						fprintf(f, "<HTML>\n<HEAD>\n<TITLE>Pieces used by %s</TITLE>\n</HEAD>\n<BR>\n<CENTER>\n", m_strTitle);
				
						CreateHTMLPieceList(f, 0, opts.images, ext);

						fputs("</CENTER>\n<BR><HR><BR>", f);
						fprintf(f, "<A HREF=\"%s-%02d.htm\">Previous</A> ", m_strTitle, i-1);

						if (opts.index)
							fprintf(f, "<A HREF=\"%s-index.htm\">Index</A> ", m_strTitle);

						fputs("<BR></HTML>\n",f);
						fclose(f);
					}
				}

				// Save step pictures
				LC_IMAGE** images;
				images = (LC_IMAGE**)malloc(sizeof(LC_IMAGE*)*last);
				CreateImages(images, opts.imdlg.width, opts.imdlg.height, 1, last, opts.hilite);

				for (i = 0; i < last; i++)
				{
					sprintf(fn, "%s%s-%02d%s", opts.path, m_strTitle, i+1, ext);
					SaveImage(fn, images[i], &opts.imdlg.imopts);
					free(images[i]);
				}
				free(images);

				if (opts.images)
				{
					int cx = 120, cy = 100;
					void* render = SystemStartRender(cx, cy);

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

						LC_IMAGE* image = SystemGetRenderImage(render);
						sprintf(fn, "%s%s%s", opts.path, pInfo->m_strName, ext);
						SaveImage(fn, image, &opts.imdlg.imopts);
						free(image);
					}
					SystemFinishRender(render);
				}
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
			char* conv = (char*)malloc(9*m_nPieceCount);
			memset(conv, 0, 9*m_nPieceCount);

			// read LGEO conversion table
			if (strlen(opts.libpath))
			{
				strcpy(fn, opts.libpath);
				strcat(fn, "l2p_elmt.tab");
				f = fopen(fn, "rb");

				if (f == NULL)
				{
//					AfxMessageBox(IDS_OPENFILE_ERROR, MB_OK|MB_ICONSTOP);
					return;
				}

				unsigned char bt[4];
				while (fread(&bt, 4, 1, f))
				{
					u = (((unsigned char)(bt[3])|((unsigned short)(bt[2]) << 8))|(((unsigned long)(bt[1])) << 16)) + bt[0] * 16581375;
					sprintf(tmp, "%d", (int)u);
					pInfo = FindPieceInfo(tmp);

					fread(&tmp, 9, 1, f);
					if (tmp[8] != 0)
						fread(&tmp[9], 1, 1, f);

					if (pInfo != NULL)
					{
						int idx = (((char*)pInfo - (char*)m_pPieceIdx)/sizeof(PieceInfo));
						memcpy(&conv[idx*9], &tmp[1], 9);
					}
				}
				fclose(f);

				strcpy(fn, opts.libpath);
				strcat(fn, "l2p_ptrn.tab");
				f = fopen(fn, "rb");
		
				if (f == NULL)
				{
//					AfxMessageBox(IDS_OPENFILE_ERROR, MB_OK|MB_ICONSTOP);
					free(conv);
					return;
				}

				u = 0;
				do 
				{
					if ((tmp[u] == 0) && (u != 0))
					{
						u = 0;
						pInfo = FindPieceInfo(tmp);
						fread(&tmp, 8, 1, f);
				
						if (pInfo != NULL)
						{
							int idx = (((char*)pInfo - (char*)m_pPieceIdx)/sizeof(PieceInfo));
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

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				Piece* pNext;

				for (pNext = m_pPieces; pNext; pNext = pNext->m_pNext)
				{
					pInfo = pNext->GetPieceInfo();

					if (pNext == pPiece)
					{
						int idx = (((char*)pInfo - (char*)m_pPieceIdx)/sizeof(PieceInfo));
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
				int idx = (((char*)pInfo - (char*)m_pPieceIdx)/sizeof(PieceInfo));
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
			m_pViewCameras[m_nActiveViewport]->GetEye(eye);
			m_pViewCameras[m_nActiveViewport]->GetTarget(target);
			m_pViewCameras[m_nActiveViewport]->GetUp(up);

			fprintf(f, "\ncamera {\n  sky<%1g,%1g,%1g>\n  location <%1g, %1g, %1g>\n  look_at <%1g, %1g, %1g>\n  angle %.0f\n}\n\n",
				up[0], up[1], up[2], eye[1], eye[0], eye[2], target[1], target[0], target[2], m_pViewCameras[m_nActiveViewport]->m_fovy);
			fprintf(f, "background { color rgb <%1g, %1g, %1g> }\n\nlight_source { <0, 0, 20> White shadowless }\n\n",
				m_fBackground[0], m_fBackground[1], m_fBackground[2]);

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				float fl[12], pos[3], rot[4];
				char name[20];
				int idx = (((char*)pPiece->GetPieceInfo() - (char*)m_pPieceIdx)/sizeof(PieceInfo));

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
				mat.ConvertToLDraw(fl);

				fprintf(f, "object {\n %s\n texture { lg_%s } matrix <%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f>\n}\n",
					name, lg_colors[pPiece->GetColor()], -fl[11], -fl[5], fl[8], -fl[9], -fl[3], fl[6],
					-fl[10], -fl[4], fl[7], pos[1], pos[0], pos[2]);
			}
			fclose(f);
			free(conv);

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

			opts.lines = m_nPieceCount;
			opts.count = (unsigned short*)malloc(m_nPieceCount*LC_MAXCOLORS*sizeof(unsigned short));
			memset (opts.count, 0, m_nPieceCount*LC_MAXCOLORS*sizeof(unsigned short));
			opts.names = (char**)malloc(m_nPieceCount*sizeof(char*));
			for (int i = 0; i < m_nPieceCount; i++)
				opts.names[i] = m_pPieceIdx[i].m_strDescription;

			for (Piece* pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
			{
				int idx = (((char*)pPiece->GetPieceInfo() - (char*)m_pPieceIdx)/sizeof(PieceInfo));
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

			if (!OnOpenDocument(m_strRecentFiles[nParam]))
			{
				// check if we corrupted the original document
				if (!IsModified())
					SetModifiedFlag(bWasModified);
				else
					OnNewDocument();

				for (int i = nParam; i < 3; i++)
					strcpy(m_strRecentFiles[i], m_strRecentFiles[i+1]);
				memset(m_strRecentFiles[3], 0, LC_MAXPATH);
				SystemUpdateRecentMenu(m_strRecentFiles);

				return;  // open failed
			}
			SetPathName(m_strRecentFiles[nParam], false);
		} break;

		case LC_EDIT_UNDO:
		case LC_EDIT_REDO:
		{
			UNDOINFO *pUndo, *pTmp;
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
					pPiece->FileSave(m_pClipboard[m_nCurClipboard], m_pGroups);

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
					pCamera->FileSave(m_pClipboard[m_nCurClipboard]);
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
				SystemRedrawView();
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
				pPiece->FileLoad(file, name);
				PieceInfo* pInfo = FindPieceInfo(name);
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
				pPiece->Select();

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
				pCamera->FileLoad(file);
				pCamera->Select();
			}

			// TODO: lights
			CalculateStep();
			SetModifiedFlag(true);
			CheckPoint("Pasting");
			SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
			UpdateSelection();
			SystemRedrawView();
		} break;

		case LC_EDIT_SELECT_ALL:
		{
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
					pPiece->Select();

//	pFrame->UpdateInfo();
			UpdateSelection();
			SystemRedrawView();
		} break;
		
		case LC_EDIT_SELECT_NONE:
		{
			SelectAndFocusNone(false);
			SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
			UpdateSelection();
			SystemRedrawView();
		} break;
		
		case LC_EDIT_SELECT_INVERT:
		{
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
				{
					if (pPiece->IsSelected())
						pPiece->UnSelect();
					else
						pPiece->Select();
				}

			SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
			UpdateSelection();
			SystemRedrawView();
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
							((Piece*)opts[i].pointer)->Select();
						} break;

						case LC_SELDLG_CAMERA:
						{
							((Camera*)opts[i].pointer)->Select();
						} break;

						case LC_SELDLG_LIGHT:
						{
							((Light*)opts[i].pointer)->Select();
						} break;

						case LC_SELDLG_GROUP:
						{
							pGroup = (Group*)opts[i].pointer;
							pGroup = pGroup->GetTopGroup();
							for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
								if (pPiece->GetTopGroup() == pGroup)
									pPiece->Select();
						} break;
					}
				}

				UpdateSelection();
				SystemRedrawView();
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
				SnapPoint(&pos[0], &pos[1], &pos[2]);
				pPiece->Initialize(pos[0], pos[1], pos[2], m_nCurStep, m_nCurFrame, m_nCurColor);
				pPiece->ChangeKey(1, false, false, rot, PK_ROTATION);
				pPiece->ChangeKey(1, true, false, rot, PK_ROTATION);
			}
			else
				pPiece->Initialize(0, 0, 0, m_nCurStep, m_nCurFrame, m_nCurColor);

			SelectAndFocusNone(false);
			pPiece->CreateName(m_pPieces);
			AddPiece(pPiece);
			pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, true, true);
			pPiece->Focus();
			SystemUpdateFocus(pPiece, LC_PIECE|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
			UpdateSelection();
			SystemPieceComboAdd(m_pCurPiece->m_strDescription);

			if (m_nSnap & LC_DRAW_MOVE)
				SetAction(LC_ACTION_MOVE);

//			AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)pNew, OT_PIECE);
			SystemRedrawView();
			SetModifiedFlag(true);
			CheckPoint("Inserting");
		} break;

		case LC_PIECE_DELETE:
		{
			if (RemoveSelectedObjects())
			{
				SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
				UpdateSelection();
				SystemRedrawView();
				SetModifiedFlag(true);
				CheckPoint("Deleting");
			}
		} break;

		case LC_PIECE_MINIFIG:
		{
			LC_MINIFIGDLG_OPTS opts;
	 		const unsigned char colors[15] = { 0, 6, 4, 22, 0, 0, 6, 6, 22, 22, 9, 9, 9, 22, 22 };
			const float pos[15][3] = { {0,0,3.84f},{0,0,3.84f},{0,0,2.88f},{0,0,2.96f},{0,0,2.56f},{0,0,2.56f},{0.9f,-0.62f,1.76f},
		{-0.9f,-0.62f,1.76f},{0.92f,-0.62f,1.76f},{-0.92f,-0.62f,1.76f},{0,0,1.6f},{0,0,1.12f},{0,0,1.12f},{0.42f,0,0},{-0.42f,0,0} };
			int i;

			for (i = 0; i < 15; i++)
			{
			    opts.info[i] = NULL;
			    opts.colors[i] = colors[i];
			    opts.pos[i][0] = pos[i][0];
			    opts.pos[i][1] = pos[i][1];
			    opts.pos[i][2] = pos[i][2];
			    opts.rot[i][0] = 0;
			    opts.rot[i][1] = 0;
			    opts.rot[i][2] = 0;
			}

			for (i = 0; i < 13; i++)
			{
			    if (i == 3 || i == 7 || i == 8 || i == 9)
			      continue;

			    PieceInfo* pInfo = FindPieceInfo(mfwpieceinfo[i].name);
			    if (pInfo == NULL)
			      continue;

			    if (i == 6)
			    {
				opts.info[6] = pInfo;
				opts.info[7] = pInfo;
				pInfo->AddRef();
				pInfo->AddRef();
				opts.rot[6][0] = 45;
				opts.rot[6][2] = 90;
				opts.rot[7][0] = 45;
				opts.rot[7][2] = 90;
			    }
			    else
			    {
				opts.info[i] = pInfo;
				pInfo->AddRef();
			    }
			}

			if (SystemDoDialog(LC_DLG_MINIFIG, &opts))
			{
				SelectAndFocusNone(false);

				for (i = 0; i < 15; i++)
				{
					if (opts.info[i] == NULL)
						continue;

					Matrix mat;
					float rot[4];
					Piece* pPiece = new Piece(opts.info[i]);

					pPiece->Initialize(opts.pos[i][0], opts.pos[i][1], opts.pos[i][2], m_nCurStep, m_nCurFrame, opts.colors[i]);
					pPiece->CreateName(m_pPieces);
					AddPiece(pPiece);
					pPiece->Select();

					mat.CreateOld(0,0,0,opts.rot[i][0],opts.rot[i][1],opts.rot[i][2]);
					mat.ToAxisAngle(rot);
					pPiece->ChangeKey(1, false, false, rot, PK_ROTATION);
					pPiece->ChangeKey(1, true, false, rot, PK_ROTATION);
					pPiece->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
					pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);

					SystemPieceComboAdd(opts.info[i]->m_strDescription);
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

				SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
				UpdateSelection();
				SystemRedrawView();
				SetModifiedFlag(true);
				CheckPoint("Minifig");
			}

			for (i = 0; i < 15; i++)
			  if (opts.info[i])
			    opts.info[i]->DeRef();
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
					opts.f2D[0] *= 0.8f;
					opts.f2D[1] *= 0.8f;
					opts.f2D[2] *= 0.96f;
					opts.f3D[0] *= 0.8f;
					opts.f3D[1] *= 0.8f;
					opts.f3D[2] *= 0.96f;
					opts.fMove[0] *= 0.8f;
					opts.fMove[1] *= 0.8f;
					opts.fMove[2] *= 0.96f;
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
							pLast->ChangeKey(1, false, false, param, PK_ROTATION);
							pLast->ChangeKey(1, true, false, param, PK_ROTATION);
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
								pLast->ChangeKey(1, false, false, param, PK_ROTATION);
								pLast->ChangeKey(1, true, false, param, PK_ROTATION);
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
								pLast->ChangeKey(1, false, false, param, PK_ROTATION);
								pLast->ChangeKey(1, true, false, param, PK_ROTATION);
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
				SystemRedrawView();
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
					pPiece->CalculatePositionRotation(m_bAnimation ? m_nCurStep : m_nCurFrame, !m_bAnimation, move, rot);
					pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, move, PK_POSITION);
					pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, rot, PK_ROTATION);
				}

			// TODO: cameras and lights

			CalculateStep();
			SystemRedrawView();
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
				SystemRedrawView();
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
			SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
			SystemRedrawView();
		} break;

		case LC_PIECE_HIDE_UNSELECTED:
		{
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (!pPiece->IsSelected())
					pPiece->Hide();
			UpdateSelection();
			SystemRedrawView();
		} break;

		case LC_PIECE_UNHIDE_ALL:
		{
			Piece* pPiece;
			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				pPiece->UnHide();
			UpdateSelection();
			SystemRedrawView();
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
				SystemRedrawView();
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
						}
					}
					else
					{
						unsigned char t = pPiece->GetStepShow();
						if (t < 255)
						{
							redraw = true;
							pPiece->SetStepShow(t+1);
						}
					}
				}

			if (redraw)
			{
				SetModifiedFlag(true);
				CheckPoint("Modifying");
				SystemRedrawView();
			}
		} break;

		case LC_VIEW_PREFERENCES:
		{
			LC_PREFERENCESDLG_OPTS opts;
			opts.nMouse = m_nMouse;
			opts.nSaveInterval = m_nAutosave;
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

				RenderInitialize();
				SystemRedrawView();
			}
		} break;

		case LC_VIEW_ZOOMIN:
		{
			m_pViewCameras[m_nActiveViewport]->DoZoom(-1, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();
		} break;

		case LC_VIEW_ZOOMOUT:
		{
			m_pViewCameras[m_nActiveViewport]->DoZoom(1, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();
		} break;

		case LC_VIEW_ZOOMEXTENTS:
		{
			if (m_pPieces == 0) break;

			bool bControl = IsKeyDown(KEY_CONTROL);
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

				pCam->GetTarget(target);
				pCam->GetEye(eye);

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

				pCam->GetUp(up);
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

				pCam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, eye, CK_EYE);
				pCam->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, target, CK_TARGET);
				pCam->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
			}

			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();
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
			SystemRedrawView();
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
			SystemRedrawView();

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
			SystemRedrawView();

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
			SystemRedrawView();

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
				m_nCurStep = 255;

			CalculateStep();
			UpdateSelection();
			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();

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
			SystemRedrawView();

			if (m_bAnimation)
				SystemUpdateTime(m_bAnimation, m_nCurFrame, m_nTotalFrames);
			else
				SystemUpdateTime(m_bAnimation, m_nCurStep, 255);
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
				RenderScene((m_nDetail & LC_DET_FAST) == 0, true);
				SystemSwapBuffers();
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
			SystemRedrawView();
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
			SystemRedrawView();
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
			SystemRedrawView();
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
			SystemRedrawView();

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
				SystemRedrawView();
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

void Project::SelectAndFocusNone(bool bFocusOnly)
{
	Piece* pPiece;
	Camera* pCamera;
	Light* pLight;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (bFocusOnly)
			pPiece->UnFocus();
		else
			pPiece->UnSelect();

	for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		if (bFocusOnly)
			pCamera->UnFocus();
		else
			pCamera->UnSelect();

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		if (bFocusOnly)
			pLight->UnFocus();
		else
			pLight->UnSelect();

//	AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);
}

PieceInfo* Project::GetPieceInfo(int index)
{
	return &m_pPieceIdx[index];
}

Camera* Project::GetCamera(int i)
{
	Camera* pCamera;

	for (pCamera = m_pCameras; i--, pCamera; pCamera = pCamera->m_pNext)
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
				pos[0] /= 0.8f;
				pos[1] /= 0.8f;
				pos[2] /= 0.96f;
			}
			return;
		}

	for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
	{
		if (pCamera->IsEyeFocused())
		{
			pCamera->GetEye(pos);
			if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
			{
				pos[0] /= 0.8f;
				pos[1] /= 0.8f;
				pos[2] /= 0.96f;
			}
			return;
		}

		if (pCamera->IsTargetFocused())
		{
			pCamera->GetTarget(pos);
			if ((m_nSnap & LC_DRAW_CM_UNITS) == 0)
			{
				pos[0] /= 0.8f;
				pos[1] /= 0.8f;
				pos[2] /= 0.96f;
			}
			return;
		}
	}

	// TODO: light

	pos[0] = pos[1] = pos[2] = 0.0f;
}

Texture* Project::FindTexture(char* name)
{
	for (int i = 0; i < m_nTextureCount; i++)
		if (!strcmp (name, m_pTextures[i].m_strName))
			return &m_pTextures[i];

	return NULL;
}

// Remeber to make 'name' uppercase.
PieceInfo* Project::FindPieceInfo(char* name)
{
	PieceInfo* pInfo;
	int i;

	for (i = m_nPieceCount, pInfo = m_pPieceIdx; i--; pInfo++)
		if (!strcmp (name, pInfo->m_strName))
			return pInfo;

	for (i = 0; i < m_nMovedCount; i++)
		if (!strcmp(&m_pMovedReference[i*18], name))
		{
			char* tmp = &m_pMovedReference[i*18+9];

			for (i = m_nPieceCount, pInfo = m_pPieceIdx; i--; pInfo++)
				if (!strcmp (tmp, pInfo->m_strName))
					return pInfo;

			break; // something went wrong.
		}

	return NULL;
}

BoundingBox* Project::FindObjectFromPoint(int x, int y)
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

	CLICKLINE ClickLine = { px, py, pz, rx-px, ry-py, rz-pz, DBL_MAX, NULL };

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation))
			pPiece->MinIntersectDist(&ClickLine);

	for (pCamera = m_pCameras; pCamera; pCamera = pCamera->m_pNext)
		if (pCamera != m_pViewCameras[m_nActiveViewport])
			pCamera->MinIntersectDist(&ClickLine);

	for (pLight = m_pLights; pLight; pLight = pLight->m_pNext)
		pLight->MinIntersectDist(&ClickLine);

	return ClickLine.pClosest;
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
	}
	else if (m_pTrackFile != NULL)
	{
		DeleteContents(true);
		FileLoad(m_pTrackFile, true, false);
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
	m_pTrackFile = new FileMem;
	FileSave(m_pTrackFile, true);
}

void Project::SnapPoint(float *x, float *y, float *z)
{
	int i;

	if (m_nSnap & LC_DRAW_SNAP_X)
	{
		i = (int)(*x/0.4f);
		*x = 0.4f * i;
	}

	if (m_nSnap & LC_DRAW_SNAP_Y)
	{
		i = (int)(*y/0.4f);
		*y = 0.4f * i;
	}

	if (m_nSnap & LC_DRAW_SNAP_Z)
	{
		i = (int)(*z/0.32f);
		*z = 0.32f * i;
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
//		pLight->UpdatePosition(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation);
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
	Piece* pPiece;

	for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
		if (pPiece->IsSelected())
		{
			pPiece->CompareBoundingBox (bs);
			nSel++;
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
			pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, pos, PK_POSITION);
		}

		m.ToAxisAngle(rot);
		pPiece->ChangeKey(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys, rot, PK_ROTATION);
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
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_FRONT);
			ret = true;
		} break;
		case 'B':
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_BACK); break;
			ret = true;
		} break;
		case 'T':
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_TOP); break;
			ret = true;
		} break;
		case 'U':
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_UNDER); break;
			ret = true;
		} break;
		case 'L':
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_LEFT); break;
			ret = true;
		} break;
		case 'R':
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_RIGHT); break;
			ret = true;
		} break;
		case 'M':
		{
			HandleCommand(LC_VIEW_CAMERA_MENU, LC_CAMERA_MAIN); break;
			ret = true;
		} break;

		case KEY_PLUS: // case '+': case '=':
		{
			HandleCommand(LC_VIEW_ZOOMIN, 0);
			ret = true;
		} break;
			
		case KEY_MINUS: // case '-': case '_':
		{
			HandleCommand(LC_VIEW_ZOOMOUT, 0);
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
				pFocus->Focus();
				Group* pGroup = pFocus->GetTopGroup();
				if (pGroup != NULL)
				{
					for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
						if (pPiece->IsVisible(m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation) &&
							pPiece->GetTopGroup() == pGroup)
								pPiece->Select();
				}
			}

			UpdateSelection();
			SystemRedrawView();
			SystemUpdateFocus(pFocus, LC_PIECE|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
			ret = true;
		} break;

		case KEY_UP:	case KEY_DOWN: case KEY_LEFT: 
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

			if (bShift)
				RotateSelectedObjects(axis[0], axis[1], axis[2]);
			else
				MoveSelectedObjects(axis[0], axis[1], axis[2]);
			SystemRedrawView();
			SetModifiedFlag(true);
			CheckPoint((bShift) ? (char*) "Rotating" : (char*) "Moving");
			SystemUpdateFocus(NULL, 0);
			ret = true;
		} break;
	}

	return ret;
}

void Project::OnLeftButtonDown(int x, int y)
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
			BoundingBox* pBox;
			pBox = FindObjectFromPoint(x, y);

			if (m_nCurAction == LC_ACTION_SELECT) 
			{
				SelectAndFocusNone(IsKeyDown(KEY_CONTROL));

				if (pBox != NULL)
				switch (pBox->GetOwnerType())
				{
					case LC_PIECE:
					{
						Piece* pPiece = (Piece*)pBox->GetOwner();
						pPiece->Focus();
						Group* pGroup = pPiece->GetTopGroup();

						if (pGroup != NULL)
							for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
								if (pPiece->GetTopGroup() == pGroup)
									pPiece->Select();
					} break;

					case LC_CAMERA:
					{
						((Camera*)pBox->GetOwner())->FocusEye();
					} break;
				
					case LC_CAMERA_TARGET:
					{
						((Camera*)pBox->GetOwner())->FocusTarget();
					} break;

					case LC_LIGHT:
					{
						((Light*)pBox->GetOwner())->FocusEye();
					} break;

					case LC_LIGHT_TARGET:
					{
						((Light*)pBox->GetOwner())->FocusTarget();
					} break;
				}

				UpdateSelection();
				SystemRedrawView();
				if (pBox)
					SystemUpdateFocus(pBox->GetOwner(), pBox->GetOwnerType()|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
				else
					SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
			}

			if ((m_nCurAction == LC_ACTION_ERASER) && (pBox != NULL))
			{
				switch (pBox->GetOwnerType())
				{
					case LC_PIECE:
					{
						Piece* pPiece = (Piece*)pBox->GetOwner();
						RemovePiece(pPiece);
						delete pPiece;
//						CalculateStep();
						RemoveEmptyGroups();
					} break;

					case LC_CAMERA:
					case LC_CAMERA_TARGET:
					{
						Camera* pCamera = (Camera*)pBox->GetOwner();
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

					case LC_LIGHT:
					case LC_LIGHT_TARGET:
					{ 
/*						pos = m_Lights.Find(pObject->m_pParent);
						m_Lights.RemoveAt(pos);
						delete pObject->m_pParent;
*/					} break;
				}

				UpdateSelection();
				SystemRedrawView();
				SetModifiedFlag(true);
				CheckPoint("Deleting");
//				AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, NULL, OT_PIECE);
			}

			if ((m_nCurAction == LC_ACTION_PAINT) && (pBox != NULL) && 
				(pBox->GetOwnerType() == LC_PIECE))
			{
				Piece* pPiece = (Piece*)pBox->GetOwner();

				if (pPiece->GetColor() != m_nCurColor)
				{
					bool bTrans = pPiece->IsTransparent();
					pPiece->SetColor(m_nCurColor);
					if (bTrans != pPiece->IsTransparent())
						pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, true, true);

					SetModifiedFlag(true);
					CheckPoint("Painting");
					SystemUpdateFocus(NULL, 0);
					SystemRedrawView();
				}
			}
		} break;

		case LC_ACTION_INSERT:
//		case LC_ACTION_LIGHT:
		{
			if (m_nCurAction == LC_ACTION_INSERT)
			{
				Piece* pPiece = new Piece(m_pCurPiece);
				SnapPoint(&m_fTrack[0], &m_fTrack[1], &m_fTrack[2]);
				pPiece->Initialize(m_fTrack[0], m_fTrack[1], m_fTrack[2], m_nCurStep, m_nCurFrame, m_nCurColor);

				SelectAndFocusNone(false);
				pPiece->CreateName(m_pPieces);
				AddPiece(pPiece);
				pPiece->CalculateConnections(m_pConnections, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, false, true);
				SystemUpdateFocus(pPiece, LC_PIECE|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
				pPiece->Focus();
				UpdateSelection();
				SystemPieceComboAdd(m_pCurPiece->m_strDescription);

				if (m_nSnap & LC_DRAW_MOVE)
					SetAction(LC_ACTION_MOVE);
			}
/*
			if (m_nCurAction == ACTION_LIGHT)
			{
				SelectAndFocusNone(FALSE);

				CLight* pLight = new CLight(m_fTrackPos[0], m_fTrackPos[1], m_fTrackPos[2]);
				m_Lights.AddTail(pLight);

//					if (m_Lights.GetCount() == 8)
//						m_nCurAction = ACTION_SELECT;
//	strcpy (m_Name
//			pFrame->UpdateInfo();
			}
*/
//			AfxGetMainWnd()->PostMessage(WM_LC_UPDATE_INFO, (WPARAM)pNew, OT_PIECE);
			UpdateSelection();
			SystemRedrawView();
			SetModifiedFlag(true);
			CheckPoint("Inserting");
		} break;

		case LC_ACTION_CAMERA:
		{
			double tmp[3];
			gluUnProject(x+1, y-1, 0.9, modelMatrix, projMatrix, viewport, &tmp[0], &tmp[1], &tmp[2]);
			SelectAndFocusNone(false);
			StartTracking(LC_TRACK_START_LEFT);
			Camera* pCamera = new Camera(m_fTrack[0], m_fTrack[1], m_fTrack[2], (float)tmp[0], (float)tmp[1], (float)tmp[2], m_pCameras);
			pCamera->FocusTarget();
			UpdateSelection();
			SystemRedrawView();
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
				StartTracking(LC_TRACK_START_LEFT);
		} break;

		case LC_ACTION_ROTATE:
		{
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					StartTracking(LC_TRACK_START_LEFT);
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
			m_pTrackFile = NULL;
		} break;

		case LC_ACTION_SPOTLIGHT:
			break;
	}
}

void Project::OnLeftButtonDoubleClick(int x, int y)
{
	GLdouble modelMatrix[16], projMatrix[16], point[3];
	GLint viewport[4];

	if (IsDrawing())
		return;

	if (SetActiveViewport(x, y))
		return;

	LoadViewportProjection();
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluUnProject(x, y, 0.9, modelMatrix, projMatrix, viewport, &point[0], &point[1], &point[2]);
	m_fTrack[0] = (float)point[0]; m_fTrack[1] = (float)point[1]; m_fTrack[2] = (float)point[2];

	BoundingBox* pBox;
	pBox = FindObjectFromPoint(x, y);

//	if (m_nCurAction == LC_ACTION_SELECT) 
	{
		SelectAndFocusNone(IsKeyDown(KEY_CONTROL));

		if (pBox != NULL)
		switch (pBox->GetOwnerType())
		{
			case LC_PIECE:
			{
				Piece* pPiece = (Piece*)pBox->GetOwner();
				pPiece->Focus();
				Group* pGroup = pPiece->GetTopGroup();

				if (pGroup != NULL)
					for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
						if (pPiece->GetTopGroup() == pGroup)
							pPiece->Select();
			} break;

			case LC_CAMERA:
			{
				((Camera*)pBox->GetOwner())->FocusEye();
			} break;

			case LC_CAMERA_TARGET:
			{
				((Camera*)pBox->GetOwner())->FocusTarget();
			} break;

			case LC_LIGHT:
			{
				((Light*)pBox->GetOwner())->FocusEye();
			} break;

			case LC_LIGHT_TARGET:
			{
				((Light*)pBox->GetOwner())->FocusTarget();
			} break;
		}

		UpdateSelection();
		SystemRedrawView();
		if (pBox)
			SystemUpdateFocus(pBox->GetOwner(), pBox->GetOwnerType()|LC_UPDATE_OBJECT|LC_UPDATE_TYPE);
		else
			SystemUpdateFocus(NULL, LC_UPDATE_OBJECT);
	}
}

void Project::OnLeftButtonUp(int x, int y)
{
	StopTracking(true);
}

void Project::OnRightButtonDown(int x, int y)
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
				StartTracking(LC_TRACK_START_RIGHT);
		} break;

		case LC_ACTION_ROTATE:
		{
			Piece* pPiece;

			for (pPiece = m_pPieces; pPiece; pPiece = pPiece->m_pNext)
				if (pPiece->IsSelected())
				{
					StartTracking(LC_TRACK_START_RIGHT);
					break;
				}
		} break;
	}
}

void Project::OnRightButtonUp(int x, int y)
{
	if (!StopTracking(true) && !m_bTrackCancel)
		SystemDoPopupMenu(1, -1, -1);
	m_bTrackCancel = false;
}

void Project::OnMouseMove(int x, int y)
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
			break;

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

			float target[3];
			pCamera->GetTarget(target);
			target[0] += delta[0];
			target[1] += delta[1];
			target[2] += delta[2];
			pCamera->ChangeKey(1, m_bAnimation, false, target, CK_TARGET);
			pCamera->UpdatePosition(1, m_bAnimation);

			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();
		} break;

		case LC_ACTION_MOVE:
		{
			// TODO: rewrite

			float mouse = 10.0f/(21-m_nMouse);
			float delta[3] = {
				(ptx - m_fTrack[0])*mouse,
				(pty - m_fTrack[1])*mouse,
				(ptz - m_fTrack[2])*mouse };
			float d[3] = { delta[0], delta[1], delta[2] };

			SnapPoint(&delta[0], &delta[1], &delta[2]);

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

			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();
		} break;
		
		case LC_ACTION_ROTATE:
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

			RotateSelectedObjects(delta[0], delta[1], delta[2]);
			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();
		} break;
		
		case LC_ACTION_ZOOM:
		{
			if (m_nDownY == y)
				break;

			m_pViewCameras[m_nActiveViewport]->DoZoom(y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownY = y;
			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();
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

			SystemSwapBuffers();
		} break;
		
		case LC_ACTION_PAN:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			m_pViewCameras[m_nActiveViewport]->DoPan(x - m_nDownX, y - m_nDownY, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownX = x;
			m_nDownY = y;
			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();
		} break;
		
		case LC_ACTION_ROTATE_VIEW:
		{
			if ((m_nDownY == y) && (m_nDownX == x))
				break;

			// We can't rotate the side cameras.
			if (m_pViewCameras[m_nActiveViewport]->IsSide())
			{
				float eye[3], target[3], up[3];
				m_pViewCameras[m_nActiveViewport]->GetEye(eye);
				m_pViewCameras[m_nActiveViewport]->GetTarget(target);
				m_pViewCameras[m_nActiveViewport]->GetUp(up);
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
			SystemRedrawView();
		} break;
		
		case LC_ACTION_ROLL:
		{
			if (m_nDownX == x)
				break;

			m_pViewCameras[m_nActiveViewport]->DoRoll(x - m_nDownX, m_nMouse, m_bAnimation ? m_nCurFrame : m_nCurStep, m_bAnimation, m_bAddKeys);
			m_nDownX = x;
			SystemUpdateFocus(NULL, 0);
			SystemRedrawView();
		} break;
	}
}













