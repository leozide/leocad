#include "lc_global.h"
#include "lc_mesh.h"
#include "Tools.h"
#include "resource.h"
#include "PrefSht.h"
#include "lc_application.h"
#include <math.h>
#include <shlobj.h>

#ifdef LC_HAVE_3DSFTK
#pragma comment(lib, "3dsftk")
#endif

#ifdef LC_HAVE_JPEGLIB
#pragma comment(lib, "jpeglib")
#endif

#ifdef LC_HAVE_PNGLIB
#pragma comment(lib, "libpng")
#pragma comment(lib, "zlib")
#endif

// Create a bitmap of a given size and color
HBITMAP CreateColorBitmap (UINT cx, UINT cy, COLORREF cr)
{
	HWND hwndDesktop = GetDesktopWindow(); 
	HDC hdcDesktop = GetDC(hwndDesktop); 
	HDC hdcMem = CreateCompatibleDC(hdcDesktop); 
	HBRUSH hbr = CreateSolidBrush(cr); 
	HBITMAP hbm = CreateCompatibleBitmap(hdcDesktop, cx, cy);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm); 

	// Paint the bitmap
	FillRect(hdcMem, CRect(0, 0, cx, cy), hbr);

	// Clean up
	SelectObject(hdcMem, hbmOld); 
	DeleteObject(hbr); 
	DeleteDC(hdcMem); 
	ReleaseDC(hwndDesktop, hdcDesktop); 

	return hbm;
}

///////////////////////////////////////
// Hooks for common dialogs

#define IDW_BUTTON 5000

UINT APIENTRY PrintHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
/*	switch (uiMsg)
	{
		case WM_INITDIALOG: 
		{
			PRINTDLG* pd = (PRINTDLG*)lParam;
			SetWindowLong(hdlg, GWL_USERDATA, pd->lCustData);
			RECT rc1, rc2;
			GetWindowRect(GetDlgItem(hdlg, IDOK), &rc1);
			GetWindowRect(GetDlgItem(hdlg, IDCANCEL), &rc2);
			ScreenToClient(hdlg, (LPPOINT)&rc1);
			ScreenToClient(hdlg, ((LPPOINT)&rc1)+1);
			ScreenToClient(hdlg, (LPPOINT)&rc2);
			CreateWindow("BUTTON","&Options...", BS_PUSHBUTTON|WS_CHILD|WS_VISIBLE|WS_TABSTOP, 
				rc1.left+rc1.left-rc2.left, rc1.top, rc1.right-rc1.left, rc1.bottom-rc1.top, hdlg, (HMENU)IDW_BUTTON, AfxGetInstanceHandle(), NULL);
			HFONT f = (HFONT)SendDlgItemMessage(hdlg, IDOK, WM_GETFONT, 0, 0);
			SendDlgItemMessage(hdlg, IDW_BUTTON, WM_SETFONT, (WPARAM)f, MAKELPARAM(TRUE, 0));
		} break;

		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDW_BUTTON)
			{
				CFrameWnd* pFrame = (CFrameWnd*)GetWindowLong(hdlg, GWL_USERDATA);
				CView* pView = pFrame->GetActiveView();
				CCADDoc* pDoc = (CCADDoc*)pView->GetDocument();

				CWnd *w = CWnd::FromHandle(hdlg);
				CPreferencesSheet propSheet(w);
				propSheet.m_tabCtrl.m_bPrintOnly = TRUE;
				propSheet.m_PagePrint.SetOptions(pDoc->m_strHeader, pDoc->m_strFooter);
				if (propSheet.DoModal() == IDOK)
					propSheet.m_PagePrint.GetOptions(&pDoc->m_strHeader, &pDoc->m_strFooter);
			}
		} break;
	}
*/	return 0;
}

#undef IDW_BUTTON

///////////////////////////////////////////////////////////
// Palette Utility Functions

static unsigned char threeto8[8] = { 0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377 };
static unsigned char twoto8[4] = { 0, 0x55, 0xaa, 0xff };
static unsigned char oneto8[2] = { 0, 255 };
static int defaultOverride[13] = { 0, 3, 24, 27, 64, 67, 88, 173, 181, 236, 247, 164, 91 };

static PALETTEENTRY defaultPalEntry[20] = {
	{ 0,   0,   0,    0 }, { 0x80,0,   0,    0 },
	{ 0,   0x80,0,    0 }, { 0x80,0x80,0,    0 },
	{ 0,   0,   0x80, 0 }, { 0x80,0,   0x80, 0 },
	{ 0,   0x80,0x80, 0 }, { 0xC0,0xC0,0xC0, 0 },

	{ 192, 220, 192,  0 }, { 166, 202, 240,  0 },
	{ 255, 251, 240,  0 }, { 160, 160, 164,  0 },

	{ 0x80,0x80,0x80, 0 }, { 0xFF,0,   0,    0 },
	{ 0,   0xFF,0,    0 }, { 0xFF,0xFF,0,    0 },
	{ 0,   0,   0xFF, 0 }, { 0xFF,0,   0xFF, 0 },
	{ 0,   0xFF,0xFF, 0 }, { 0xFF,0xFF,0xFF, 0 }
};

static unsigned char ComponentFromIndex(int i, UINT nbits, UINT shift)
{
	unsigned char val = (unsigned char) (i >> shift);
	switch (nbits) {
	case 1: val &= 0x1; return oneto8[val];
	case 2: val &= 0x3; return twoto8[val];
	case 3: val &= 0x7; return threeto8[val];
	default: return 0;
	}
}

BOOL CreateRGBPalette(HDC hDC, CPalette **ppCPalette)
{
	PIXELFORMATDESCRIPTOR pfd;
	LOGPALETTE	*pPal;
	WORD		n, i;

	n = GetPixelFormat(hDC);
	DescribePixelFormat(hDC, n, sizeof(pfd), &pfd);

	if (!(pfd.dwFlags & PFD_NEED_PALETTE)) return FALSE;

	n = 1 << pfd.cColorBits;
	pPal = (PLOGPALETTE) new BYTE[sizeof(LOGPALETTE) + n * sizeof(PALETTEENTRY)];
	ASSERT(pPal != NULL);
	pPal->palVersion = 0x300;
	pPal->palNumEntries = n;

	for (i=0; i<n; i++)
	{
		pPal->palPalEntry[i].peRed = ComponentFromIndex(i, pfd.cRedBits, pfd.cRedShift);
		pPal->palPalEntry[i].peGreen = ComponentFromIndex(i, pfd.cGreenBits, pfd.cGreenShift);
		pPal->palPalEntry[i].peBlue = ComponentFromIndex(i, pfd.cBlueBits, pfd.cBlueShift);
		pPal->palPalEntry[i].peFlags = 0;
	}

	// fix up the palette to include the default GDI palette
	if ((pfd.cColorBits == 8)							&&
		(pfd.cRedBits	== 3) && (pfd.cRedShift   == 0) &&
		(pfd.cGreenBits == 3) && (pfd.cGreenShift == 3) &&
		(pfd.cBlueBits	== 2) && (pfd.cBlueShift  == 6))
	{
		for (i = 1 ; i <= 12 ; i++)
			pPal->palPalEntry[defaultOverride[i]] = defaultPalEntry[i];
	}

	BOOL result = (*ppCPalette)->CreatePalette(pPal);
	delete pPal;

	return result;
}

HANDLE MakeDib(HBITMAP hbitmap, UINT bits)
{
	HANDLE hdib;
	HDC hdc;
	BITMAP bitmap;
	UINT wLineLen;
	DWORD dwSize, wColSize;
	LPBITMAPINFOHEADER lpbi;
	LPBYTE lpBits;
	
	GetObject(hbitmap, sizeof(BITMAP), &bitmap) ;

	// DWORD align the width of the DIB
	// Figure out the size of the colour table
	// Calculate the size of the DIB
	wLineLen = (bitmap.bmWidth*bits+31)/32 * 4;
	wColSize = sizeof(RGBQUAD)*((bits <= 8) ? 1<<bits : 0);
	dwSize = sizeof(BITMAPINFOHEADER) + wColSize +
		(DWORD)(UINT)wLineLen*(DWORD)(UINT)bitmap.bmHeight;

	// Allocate room for a DIB and set the LPBI fields
	hdib = GlobalAlloc(GHND,dwSize);
	if (!hdib)
		return NULL;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib) ;
	lpbi->biSize = sizeof(BITMAPINFOHEADER) ;
	lpbi->biWidth = bitmap.bmWidth ;
	lpbi->biHeight = bitmap.bmHeight ;
	lpbi->biPlanes = 1 ;
	lpbi->biBitCount = (WORD) bits ;
	lpbi->biCompression = BI_RGB ;
	lpbi->biSizeImage = dwSize - sizeof(BITMAPINFOHEADER) - wColSize ;
	lpbi->biXPelsPerMeter = 0 ;
	lpbi->biYPelsPerMeter = 0 ;
	lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;
	lpbi->biClrImportant = 0 ;

	// Get the bits from the bitmap and stuff them after the LPBI
	lpBits = (LPBYTE)(lpbi+1)+wColSize;
	hdc = CreateCompatibleDC(NULL);
	GetDIBits(hdc,hbitmap,0,bitmap.bmHeight,lpBits,(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

	// Fix this if GetDIBits messed it up....
	lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;

	DeleteDC(hdc);
	GlobalUnlock(hdib);

	return hdib;
}

BOOL FolderBrowse(CString *strFolder, LPCSTR lpszTitle, HWND hWndOwner)
{
	LPMALLOC pMalloc;

	if (SHGetMalloc(&pMalloc) == NOERROR)
	{
		BROWSEINFO bi;
		char pszBuffer[MAX_PATH];
		LPITEMIDLIST pidl;
		
		bi.hwndOwner = hWndOwner;
		bi.pidlRoot = NULL;
		bi.pszDisplayName = pszBuffer;
		bi.lpszTitle = lpszTitle;
		bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
		bi.lpfn = NULL;
		bi.lParam = 0;
		
		if ((pidl = SHBrowseForFolder(&bi)) != NULL)
		{
			if (SHGetPathFromIDList(pidl, pszBuffer))
			{ 
				if (pszBuffer[strlen(pszBuffer)-1] != '\\') 
					strcat(pszBuffer, "\\");
				*strFolder = pszBuffer;
				return TRUE;
			}
			pMalloc->Free(pidl);
		}
		pMalloc->Release();
	}
	return FALSE;
} 

#ifdef LC_HAVE_3DSFTK

#include "3dsftk\inc\3dsftk.h"
#include "project.h"
#include "piece.h"
#include "pieceinf.h"

void Export3DStudio() 
{
	CFileDialog dlg(FALSE, "*.dat",NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"3D Studio Files (*.3ds)|*.3ds|All Files (*.*)|*.*||", AfxGetMainWnd());
	if (dlg.DoModal() != IDOK)
		return;

	Project* project = lcGetActiveProject();

	ClearErrList3ds();
	file3ds *file = OpenFile3ds(dlg.GetPathName(), "w");
	database3ds *db = NULL;
	InitDatabase3ds(&db);
	CreateNewDatabase3ds(db, MeshFile);

	// MESH SETTINGS
	meshset3ds *mesh = NULL;
	InitMeshSet3ds(&mesh);
	mesh->ambientlight.r = project->m_fAmbient[0];
	mesh->ambientlight.g = project->m_fAmbient[1];
	mesh->ambientlight.b = project->m_fAmbient[2];
	PutMeshSet3ds(db, mesh);
	ReleaseMeshSet3ds(&mesh);

	// BACKGROUND
	background3ds *bgnd = NULL;
	InitBackground3ds(&bgnd);

	float* bg = project->GetBackgroundColor();
	bgnd->solid.color.r = bg[0];
	bgnd->solid.color.g = bg[1];
	bgnd->solid.color.b = bg[2];
	bgnd->bgndused = UseSolidBgnd;
/*
	if (m_dwScene & SCENE_BG)
	{
//			bgnd->bitmap = 
//			bgnd->bgndused = UseBitmapBgnd;
	}
*/
	PutBackground3ds(db, bgnd);
	ReleaseBackground3ds(&bgnd);

	// ATMOSPHERE
	atmosphere3ds *atmo = NULL;
	InitAtmosphere3ds(&atmo);
	if (project->m_nScene & LC_SCENE_FOG)
	{
		atmo->fog.fogcolor.r = project->m_fFogColor[0];
		atmo->fog.fogcolor.g = project->m_fFogColor[1];
		atmo->fog.fogcolor.b = project->m_fFogColor[2];
		atmo->fog.fogbgnd = True3ds;
		atmo->activeatmo = UseFog;
	}
	PutAtmosphere3ds(db, atmo);
	ReleaseAtmosphere3ds(&atmo);



/*

//	CreatePolygon(db);
{
	int i;
	light3ds *slite = NULL;
	kfspot3ds *kflite = NULL;
	camera3ds *cam = NULL;
	kfcamera3ds *kfcam = NULL;
	kfmesh3ds *kobj = NULL;
	kfambient3ds *amlite = NULL; 
	kfsets3ds *kfsets = NULL;
	viewport3ds *view = NULL;
	

	
	
	
	// CAMERA
	InitCamera3ds(&cam);
	ON_ERROR_RETURN;
	
	strcpy(cam->name, "Camera1");
	cam->position.x = 65.0F;
	cam->position.y = -3440.0F;
	cam->position.z = 185.0F;
	cam->target.x = 25.0F;
	cam->target.y = 300.0F;
	cam->target.z = 3000.0F;
	cam->ranges.cam_near = (float3ds)4000.0;
	cam->ranges.cam_far  = (float3ds)9000.0;
	cam->roll = 0.0F;
	cam->fov = 60.0F;
	cam->showcone = True3ds;
	
	PutCamera3ds(db, cam);
	ON_ERROR_RETURN;
	ReleaseCamera3ds(&cam);
	ON_ERROR_RETURN;
	
	// SPOTLIGHT
	InitSpotlight3ds(&slite);
	ON_ERROR_RETURN;
	
	strcpy(slite->name, "Spot1");
	slite->pos.x = 1000.0F;
	slite->pos.y = -4500.0F;
	slite->pos.z = 3000.0F;
	slite->color.r = 0.1F;
	slite->color.g = 0.2F;
	slite->color.b = 0.3F;
	
	slite->dloff = False3ds;
	slite->attenuation.on = False3ds;
	slite->attenuation.inner = 10.0F;
	slite->attenuation.outer = 100.0F;
	
	slite->spot->target.x = 0.0F;
	slite->spot->target.y = 0.0F;
	slite->spot->target.z = 0.0F;
	slite->spot->hotspot = 44.0F;
	slite->spot->falloff = 45.0F;
	slite->spot->roll = 0.0F;
	slite->spot->aspect = 1.0F;
	slite->spot->shadows.cast = False3ds;
	slite->spot->shadows.type = UseShadowMap;
	slite->spot->shadows.local = False3ds;
	slite->spot->shadows.bias = 1.0F;
	slite->spot->shadows.filter = 3.0F;
	slite->spot->shadows.mapsize = 512;
	slite->spot->shadows.raybias = 1.0F;
	slite->spot->cone.type = Circular;
	slite->spot->cone.show = True3ds;
	slite->spot->cone.overshoot = False3ds;
	slite->spot->projector.use = False3ds;
	slite->spot->projector.bitmap = NULL;
	
	PutSpotlight3ds(db, slite);
	ON_ERROR_RETURN;
	ReleaseLight3ds(&slite);
	ON_ERROR_RETURN;
*/	

	// MATERIALS
	material3ds *matr = NULL;
	for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
	{
		lcColor* Color = &gColorList[ColorIdx];

		InitMaterial3ds(&matr);
		sprintf(matr->name, "Material%03d", ColorIdx);

		matr->ambient.r = matr->diffuse.r = Color->Value[0];
		matr->ambient.g = matr->diffuse.g = Color->Value[1];
		matr->ambient.b = matr->diffuse.b = Color->Value[2];
		matr->specular.r = 0.9f;
		matr->specular.g = 0.9f;
		matr->specular.b = 0.9f;

		matr->shininess = 0.25f;
		matr->shinstrength = 0.05f;
		matr->blur = 0.2f;

		if (lcIsColorTranslucent(ColorIdx))
		{
			matr->transparency = 0.5f;
			matr->transfalloff = 0.0f;
		}
		else
		{
			matr->transparency = 0.0f;
			matr->transfalloff = 0.0f;
		}
		matr->selfillumpct = 0.0f;
		matr->wiresize = 1.0f;
		matr->shading = Phong;
		matr->useblur = False3ds;
		matr->twosided = True3ds;
		PutMaterial3ds(db, matr);
		ReleaseMaterial3ds(&matr);
	}

	int objcount = 0;
	UINT* facemats = new UINT[gColorList.GetSize()];

	for (Piece* pPiece = project->m_pPieces; pPiece; pPiece = pPiece->m_pNext)
	{
		PieceInfo* pInfo = pPiece->mPieceInfo;
		lcMesh* Mesh = pInfo->mMesh;

		if (Mesh->mIndexType == GL_UNSIGNED_INT)
			continue; // 3DS can't handle this

		// MESH OBJECT
		mesh3ds *mobj = NULL;
		UINT facecount = 0;
		memset(facemats, 0, sizeof(facemats[0]) * gColorList.GetSize());
		int NumColors = 0;

		// Count the number of triangles.
		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			facecount += Section->NumIndices / 3;
			if (!facemats[Section->ColorIndex])
				NumColors++;
			facemats[Section->ColorIndex] += Section->NumIndices / 3;
		}

		InitMeshObj3ds(&mobj, (unsigned short)pInfo->mMesh->mNumVertices, facecount, InitNoExtras3ds);
		sprintf(mobj->name, "Piece%03d", objcount);
		objcount++;

		const lcMatrix44& ModelWorld = pPiece->mModelWorld;
		float* Verts = (float*)pInfo->mMesh->mVertexBuffer.mData;

		for (int c = 0; c < pInfo->mMesh->mNumVertices; c++)
		{
			lcVector3 Pos(Verts[c*3], Verts[c*3+1], Verts[c*3+2]);
			Pos = lcMul31(Pos, ModelWorld);
			mobj->vertexarray[c].x = Pos[0];
			mobj->vertexarray[c].y = Pos[1];
			mobj->vertexarray[c].z = Pos[2];
		}

		int NumFaces = 0;
		for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
		{
			lcMeshSection* Section = &Mesh->mSections[SectionIdx];

			if (Section->PrimitiveType != GL_TRIANGLES)
				continue;

			lcuint16* Indices = (lcuint16*)Mesh->mIndexBuffer.mData + Section->IndexOffset / sizeof(lcuint16);

			for (int Idx = 0; Idx < Section->NumIndices; Idx += 3, NumFaces++)
			{
				mobj->facearray[NumFaces].v1 = Indices[Idx + 0];
				mobj->facearray[NumFaces].v2 = Indices[Idx + 1];
				mobj->facearray[NumFaces].v3 = Indices[Idx + 2];
				mobj->facearray[NumFaces].flag = FaceABVisable3ds|FaceBCVisable3ds|FaceCAVisable3ds;
			}
		}

		mobj->nmats = NumColors;
		InitMeshObjField3ds(mobj, InitMatArray3ds);

		int MaterialIdx = 0;
		for (int ColorIdx = 0; ColorIdx < gColorList.GetSize(); ColorIdx++)
		{
			if (!facemats[ColorIdx])
				continue;

			InitMatArrayIndex3ds(mobj, MaterialIdx, facemats[ColorIdx]);
			sprintf(mobj->matarray[MaterialIdx].name, "Material%03d", ColorIdx == gDefaultColor ? pPiece->mColorIndex : ColorIdx);
			mobj->matarray[MaterialIdx].nfaces = facemats[ColorIdx];

			UINT curface = 0;
			facecount = 0;

			for (int SectionIdx = 0; SectionIdx < Mesh->mNumSections; SectionIdx++)
			{
				lcMeshSection* Section = &Mesh->mSections[SectionIdx];

				if (Section->PrimitiveType != GL_TRIANGLES)
					continue;

				if (Section->ColorIndex == ColorIdx)
				{
					for (int k = 0; k < Section->NumIndices; k += 3)
					{
						mobj->matarray[MaterialIdx].faceindex[facecount] = curface;
						facecount++;
						curface++;
					}
				}
				else
				{
					curface += Section->NumIndices / 3;
				}
			}

			MaterialIdx++;
		}
	
		FillMatrix3ds(mobj);
		mobj->locmatrix[9] = 0;
		mobj->locmatrix[10] = 0;
		mobj->locmatrix[11] = 0;
		PutMesh3ds(db, mobj);
		RelMeshObj3ds(&mobj);
	}

	delete[] facemats;

/*	
	InitMeshObj3ds(&mobj, 12, 8, InitNoExtras3ds);
	SetPoint(mobj->vertexarray[0],  -46.62F,   -2.31F, -103.57F);
	SetFace(mobj->facearray[7], 7, 4,  8, FaceABVisable3ds|FaceBCVisable3ds|FaceCAVisable3ds);
	mobj->nmats = 1;
	InitMeshObjField3ds (mobj, InitMatArray3ds);
	InitMatArrayIndex3ds (mobj, 0, 7);
	strcpy(mobj->matarray[0].name, "RedSides");
	for (i=0; i<7; i++) mobj->matarray[0].faceindex[i] = i;
	mobj->matarray[0].nfaces = 7;
	
	FillMatrix3ds(mobj);
	PutMesh3ds(db, mobj);

	// release later
	
	// KEYFRAMER SECTION
	
	// Any object used in keyframer section must be defined already
	
	// SPOTLIGHT
	InitSpotlightMotion3ds(&kflite, 3, 2, 1, 1, 1, 2);
	ON_ERROR_RETURN;
	
	// POSITION
	strcpy (kflite->name, "Spot1");
	kflite->pkeys[0].time = 0; 
	SetPoint(kflite->pos[0], 1000.0F, -4500.0F,  3000.0F);
	kflite->pkeys[1].time = 10;
	SetPoint(kflite->pos[1], 1000.0F,     0.0F,  3000.0F);
	kflite->pkeys[2].time = 15;
	SetPoint(kflite->pos[2], 1000.0F, -4500.0F, -1000.0F);
	
	// COLOR
	kflite->ckeys[0].time = 0;
	kflite->color[0].r = 0.1F;
	kflite->color[0].g = 0.2F;
	kflite->color[0].b = 0.3F;
	kflite->ckeys[1].time = 30;
	kflite->color[1].r = 0.5F;
	kflite->color[1].g = 0.4F;
	kflite->color[1].b = 0.3F;
	// HOTSPOT
	kflite->hkeys[0].time = 0;
	kflite->hot[0] = 44.0F;
	// FALLOFF
	kflite->fkeys[0].time = 0;
	kflite->fall[0] = 45.0F;
	// ROLL
	kflite->rkeys[0].time = 0;
	kflite->roll[0] = 0.0F;
	
	kflite->flags1 = 20; 
	kflite->flags2 = 0; 
	
	kflite->tkeys[0].time = 0;
	SetPoint(kflite->tpos[0], 0.0F,  0.0F, 0.0F)
		kflite->tkeys[1].time = 30;
	SetPoint(kflite->tpos[1], 0.0F, 0.0F, 3000.0F)
		
		PutSpotlightMotion3ds(db, kflite); 
	ON_ERROR_RETURN;
	ReleaseSpotlightMotion3ds(&kflite);
	ON_ERROR_RETURN;
	
	// CAMERA MOTION
	InitCameraMotion3ds(&kfcam, 1, 5, 1, 5);
	ON_ERROR_RETURN;
	
	strcpy (kfcam->name, "Camera1");
	kfcam->ntflag = TrackLoops3ds;
	kfcam->flags2 = 1;
	
	kfcam->pkeys[0].time = 0;
	SetPoint(kfcam->pos[0], 65.0F, -3440.0F, 185.0F)
		
		kfcam->tkeys[0].time = 0;
	SetPoint(kfcam->tpos[0],  0.0F,   0.0F,   0.0F)
		kfcam->tkeys[1].time = 5;
	SetPoint(kfcam->tpos[1], 25.0F,   0.0F, 300.0F)
		kfcam->tkeys[2].time = 10;
	SetPoint(kfcam->tpos[2], 25.0F, -30.0F, 300.0F)
		kfcam->tkeys[3].time = 15;
	SetPoint(kfcam->tpos[3], 25.0F,   0.0F, 300.0F)
		kfcam->tkeys[4].time = 20;
	SetPoint(kfcam->tpos[4], 25.0F,  30.0F, 300.0F)
		
		kfcam->rkeys[0].time = 0;
	kfcam->roll[0] = 0.0F;
	
	kfcam->nfflag = TrackRepeats3ds;
	kfcam->fkeys[0].time = 0;
	kfcam->fov[0] = 20.0F;
	kfcam->fkeys[1].time = 5;
	kfcam->fov[1] = 30.0F;
	kfcam->fkeys[2].time = 10;
	kfcam->fov[2] = 20.0F;
	kfcam->fkeys[3].time = 15;
	kfcam->fov[3] = 33.5F;
	kfcam->fkeys[4].time = 20;
	kfcam->fov[4] = 20.0F;
	
	PutCameraMotion3ds(db, kfcam);   // this is the last villan
	ON_ERROR_RETURN;
	ReleaseCameraMotion3ds(&kfcam);
	ON_ERROR_RETURN;
	
	// OBJECT MOTION POLY1
	InitObjectMotion3ds(&kobj, 6, 2, 1, 0, 0);
	ON_ERROR_RETURN;
	
	strcpy (kobj->name, "Poly1");
	
	// Set the pivot point to the mesh's center
	SetPoint(kobj->pivot, 0.0F, 0.0F, 0.0F);
	
	SetBoundBox3ds (mobj, kobj);
	ON_ERROR_RETURN;
	
	kobj->pkeys[0].time = 0;
	kobj->pos[0].x = (kobj->boundmax.x - kobj->boundmax.x) / 2.0F;
	kobj->pos[0].y = (kobj->boundmax.y - kobj->boundmax.y) / 2.0F;
	kobj->pos[0].z = (kobj->boundmax.z - kobj->boundmax.z) / 2.0F;
	
	kobj->pkeys[1].time = 8;
	SetPoint(kobj->pos[1], -421.5F, -1392.0F,  30.5F)
		kobj->pkeys[2].time = 15;
	SetPoint(kobj->pos[2],  403.0F, -1639.0F,  25.0F)
		kobj->pkeys[3].time = 24;
	SetPoint(kobj->pos[3], 1145.5F,  -484.0F,  71.5F)
		kobj->pkeys[4].time = 31;
	SetPoint(kobj->pos[4],  870.5F,   224.0F, 610.5F)
		kobj->pkeys[5].time = 40;
	SetPoint(kobj->pos[5],   10.5F,    30.0F,  57.0F)
		
		kobj->nrflag = TrackLoops3ds;
	kobj->nsflag = TrackSingle3ds;
	
	kobj->rkeys[0].time = 0;
	kobj->rot[0].angle = 0.0F;
	kobj->rot[0].x =  0.0F;
	kobj->rot[0].y =  0.0F;
	kobj->rot[0].z = -1.0F;
	
	kobj->rkeys[1].time = 20;
	kobj->rot[1].angle = 3.1415F;
	kobj->rot[1].x =  0.0F;
	kobj->rot[1].y =  0.0F;
	kobj->rot[1].z = -1.0F;
	
	
	PutObjectMotion3ds(db, kobj);
	ON_ERROR_RETURN;
	ReleaseObjectMotion3ds (&kobj);
	ON_ERROR_RETURN;
	
	RelMeshObj3ds(&mobj);
	
	// SET KEYFRAMER AMBIENT LIGHT
	InitAmbientLightMotion3ds(&amlite, 1);
	ON_ERROR_RETURN;
	amlite->flags1 = (ushort3ds)10;
	amlite->ckeys[0].time = 15;
	amlite->ckeys[0].rflags = (ushort3ds)40;
	amlite->color[0].r = amlite->color[0].g = amlite->color[0].b = 0.5F;
	PutAmbientLightMotion3ds(db, amlite);
	ON_ERROR_RETURN;
	ReleaseAmbientLightMotion3ds(&amlite);
	ON_ERROR_RETURN;
	
	// KEYFRAMER ANIMATION DATA
	InitKfSets3ds(&kfsets);
	ON_ERROR_RETURN;
	kfsets->anim.length = 60;
	kfsets->anim.curframe = 0;
	kfsets->seg.use = False3ds;
	
	PutKfSets3ds(db, kfsets);
	ON_ERROR_RETURN;
	ReleaseKfSets3ds(&kfsets);
	ON_ERROR_RETURN;
	
	// Viewport setup
	InitViewport3ds(&view);
	ON_ERROR_RETURN;
	
	view->type = CameraView3ds;
	strcpy(view->camera.name, "Camera1");
	
	PutViewport3ds(db, view); 
	ReleaseViewport3ds(&view);
}
*/
	WriteDatabase3ds(file, db);
	CloseFile3ds(file);
	ReleaseDatabase3ds(&db);
}

#else

void Export3DStudio() 
{
}

#endif
