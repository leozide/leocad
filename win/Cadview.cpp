// CADView.cpp : implementation of the CCADView class
//

#include "stdafx.h"
#include "LeoCAD.h"

#include "CADDoc.h"
#include "CADView.h"
#include "WheelWnd.h"
#include "Tools.h"
#include "PrevView.h"
#include "project.h"
#include "globals.h"
#include "system.h"
#include "Camera.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CCADView

IMPLEMENT_DYNCREATE(CCADView, CView)

BEGIN_MESSAGE_MAP(CCADView, CView)
	//{{AFX_MSG_MAP(CCADView)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_PALETTECHANGED()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_WM_RBUTTONDOWN()
	ON_WM_CAPTURECHANGED()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_MBUTTONDOWN()
	ON_WM_TIMER()
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_NOTIFY(TBN_DROPDOWN, AFX_IDW_TOOLBAR, OnDropDown)
	ON_MESSAGE(WM_LC_SET_STEP, OnSetStep)
	ON_MESSAGE(WM_LC_WHEEL_PAN, OnAutoPan)
	ON_MESSAGE(WM_LC_SET_CURSOR, OnChangeCursor)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCADView construction/destruction

CCADView::CCADView()
{
	m_pPixels = NULL;
	m_pPalette = NULL;
	m_hglRC = NULL;
	m_pDC = NULL;
	m_hCursor = NULL;
}

CCADView::~CCADView()
{
	if (m_pPixels)
		free (m_pPixels);
}

BOOL CCADView::PreCreateWindow(CREATESTRUCT& cs)
{
#define CUSTOM_CLASSNAME _T("LeoCADOpenGLClass")

	cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	// call base class PreCreateWindow to get the cs.lpszClass filled in with the MFC default class name
	if(!CView::PreCreateWindow(cs))
		return 0;

	// Register the window class if it has not already been registered.
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();

	// check if our class is registered
	if(!(::GetClassInfo(hInst, CUSTOM_CLASSNAME, &wndcls)))
	{
		// get default MFC class settings 
		if(::GetClassInfo(hInst, cs.lpszClass, &wndcls))
		{
			// set our class name
			wndcls.lpszClassName = CUSTOM_CLASSNAME;

			// change settings for your custom class
			wndcls.style |= CS_OWNDC;
			wndcls.hbrBackground = NULL;

			// register class
			if (!AfxRegisterClass(&wndcls))
				AfxThrowResourceException();
		}
		else
			AfxThrowResourceException();
	}

	// set our class name in CREATESTRUCT
	cs.lpszClass = CUSTOM_CLASSNAME;

	return 1;                                                   // we're all set
}

/////////////////////////////////////////////////////////////////////////////
// CCADView drawing

void CCADView::OnDraw(CDC* /*pDC*/)
{
	project->Render(false);
/*
	CCADDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	if (m_pPixels)
	{
		glViewport(0, 0, pDoc->m_szView.cx, pDoc->m_szView.cy);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0, pDoc->m_szView.cx, 0, pDoc->m_szView.cy);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRasterPos2i(0, 0);
		glDrawPixels(pDoc->m_szView.cx, pDoc->m_szView.cy, GL_RGBA, GL_UNSIGNED_BYTE, m_pPixels);
	}
	else
		pDoc->Render(FALSE);

	SwapBuffers(wglGetCurrentDC());
*/
}

/////////////////////////////////////////////////////////////////////////////
// CCADView printing

// Derived to use our version of the toolbar
void CCADView::OnFilePrintPreview() 
{
	// In derived classes, implement special window handling here
	// Be sure to Unhook Frame Window close if hooked.

	// must not create this on the frame.  Must outlive this function
	CPrintPreviewState* pState = new CPrintPreviewState;
	pState->lpfnCloseProc = _AfxPreviewCloseProcEx;

	// DoPrintPreview's return value does not necessarily indicate that
	// Print preview succeeded or failed, but rather what actions are necessary
	// at this point.  If DoPrintPreview returns TRUE, it means that
	// OnEndPrintPreview will be (or has already been) called and the
	// pState structure will be/has been deleted.
	// If DoPrintPreview returns FALSE, it means that OnEndPrintPreview
	// WILL NOT be called and that cleanup, including deleting pState
	// must be done here.

	if (!DoPrintPreview(IDR_PREVIEW, this,
			RUNTIME_CLASS(CPreviewViewEx), pState))
	{
		// In derived classes, reverse special window handling here for
		// Preview failure case

		TRACE0("Error: DoPrintPreview failed.\n");
		AfxMessageBox(AFX_IDP_COMMAND_FAILURE);
		delete pState;      // preview failed to initialize, delete State now
	}
}

BOOL CCADView::OnPreparePrinting(CPrintInfo* pInfo)
{
	pInfo->m_pPD->m_pd.nMinPage = 1; 
	pInfo->m_pPD->m_pd.hInstance = AfxGetInstanceHandle();
	pInfo->m_pPD->m_pd.Flags |= PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOSELECTION | PD_ENABLEPRINTHOOK;
	pInfo->m_pPD->m_pd.lpfnPrintHook = PrintHookProc;
	pInfo->m_pPD->m_pd.lCustData = (LONG)AfxGetMainWnd();

	int cols = theApp.GetProfileInt("Default", "Print Columns", 1);
	int rows = theApp.GetProfileInt("Default", "Print Rows", 1);
	int Max = (project->GetLastStep()/(rows*cols));
	if (project->GetLastStep()%(rows*cols) != 0) 
		Max++;
	pInfo->SetMaxPage(Max);

	if (pInfo->m_bPreview)
	{
		CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();

		POSITION pos = pFrame->m_listControlBars.GetHeadPosition();
		while (pos != NULL)
		{
			CControlBar* pBar = (CControlBar*)pFrame->m_listControlBars.GetNext(pos);
			CString str;
			pBar->GetWindowText(str);
			if (str == _T("Full Screen"))
			{
				AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
				break;
			}
		}
	}

	return DoPreparePrinting(pInfo);
}

void CCADView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* pInfo)
{
	int cols = theApp.GetProfileInt("Default", "Print Columns", 1);
	int rows = theApp.GetProfileInt("Default", "Print Rows", 1);
	int Max = (project->GetLastStep()/(rows*cols));
	if (project->GetLastStep()%(rows*cols) != 0)
		Max++;
	pInfo->SetMaxPage(Max);
}

void CCADView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
	int cols = theApp.GetProfileInt("Default","Print Columns", 1);
	int rows = theApp.GetProfileInt("Default","Print Rows", 1);
	if (rows < 1) rows = 1;
	if (cols < 1) cols = 1;
	CRect rc(0, 0, pDC->GetDeviceCaps(HORZRES), pDC->GetDeviceCaps(VERTRES));
	pDC->DPtoLP(&rc);
	int lpx = pDC->GetDeviceCaps(LOGPIXELSX);
	int lpy = pDC->GetDeviceCaps(LOGPIXELSY);
	rc.DeflateRect(lpx*theApp.GetProfileInt("Default","Margin Left", 50)/100, 
		lpy*theApp.GetProfileInt("Default","Margin Top", 50)/100,
		lpx*theApp.GetProfileInt("Default","Margin Right", 50)/100, 
		lpy*theApp.GetProfileInt("Default","Margin Bottom", 50)/100);

	int w = rc.Width()/cols, h = rc.Height()/rows; // cell size
	float viewaspect = (float)m_szView.cx/(float)m_szView.cy;
	int pw = w, ph = h; // picture
	int mx = 0, my = 0; // offset

	if (w < h)
	{
		ph = (int) (w / viewaspect);
		my = (h - ph)/2;
	}
	else
	{
		pw = (int) (h * viewaspect);
		mx = (w - pw)/2;
	}

	int tw = pw, th = ph; // tile size

	MEMORYSTATUS MemStat;
	MemStat.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&MemStat);

	if (DWORD(pw*ph*3) > MemStat.dwTotalPhys)
	{
		tw = 512;
		th = 512;
	}

	HDC hMemDC = CreateCompatibleDC(GetDC()->m_hDC);
	LPBITMAPINFOHEADER lpbi;

	// Preparing bitmap header for DIB section
	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = tw;
	bi.bmiHeader.biHeight = th;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = tw * th * 3;
	bi.bmiHeader.biXPelsPerMeter = 2925;
	bi.bmiHeader.biYPelsPerMeter = 2925;
	
    HBITMAP hBm = CreateDIBSection(hMemDC, &bi, DIB_RGB_COLORS, (void **)&lpbi, NULL, (DWORD)0);
	HBITMAP hBmOld = (HBITMAP)SelectObject(hMemDC, hBm);

    // Setting up a Pixel format for the DIB surface
	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),
			1,PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI,
			PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
			0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
	int pixelformat = ChoosePixelFormat(hMemDC, &pfd);
	DescribePixelFormat(hMemDC, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	SetPixelFormat(hMemDC, pixelformat, &pfd);
	
	// Creating OpenGL context
    HGLRC hmemrc = wglCreateContext(hMemDC);
	wglMakeCurrent(hMemDC, hmemrc);
//	if (!wglShareLists(m_hglRC, hmemrc))
//		pDoc->RebuildDisplayLists(TRUE);
	project->RenderInitialize();

	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = -MulDiv(50, pDC->GetDeviceCaps(LOGPIXELSY), 72);
	lf.lfWeight = FW_BOLD;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfQuality = PROOF_QUALITY;
	strcpy (lf.lfFaceName , "Arial");
	HFONT font = CreateFontIndirect(&lf);
	HFONT OldFont = (HFONT)SelectObject(pDC->m_hDC, font);
	pDC->SetTextAlign(TA_BASELINE|TA_CENTER|TA_NOUPDATECP);
	float* bg = project->GetBackgroundColor();
	pDC->SetTextColor(RGB (1.0f - bg[0], 1.0f - bg[1], 1.0f - bg[2]));
	pDC->SetBkMode(TRANSPARENT);
	HPEN hpOld = (HPEN)SelectObject(pDC->m_hDC,(HPEN)GetStockObject(BLACK_PEN));

	unsigned short nOldTime = project->m_bAnimation ? project->m_nCurFrame : project->m_nCurStep;
	UINT nRenderTime = 1+((pInfo->m_nCurPage-1)*rows*cols);

	int oldSizex = project->m_nViewX;
	int oldSizey = project->m_nViewY;
	project->m_nViewX = tw;
	project->m_nViewY = th;

	for (int r = 0; r < rows; r++)
	for (int c = 0; c < cols; c++)
	{
		if (nRenderTime > project->GetLastStep())
			continue;
		if (project->m_bAnimation)
			project->m_nCurFrame = nRenderTime;
		else
			project->m_nCurStep = nRenderTime;
		project->CalculateStep();
		FillRect(hMemDC, CRect(0,th,tw,0), (HBRUSH)GetStockObject(WHITE_BRUSH));

		// Tile rendering
		if (tw != pw)
		{
			Camera* pCam = project->m_pCameras;
			for (int i = LC_CAMERA_MAIN; pCam; pCam = pCam->m_pNext)
				if (i-- == 0)
					break;
			pCam->StartTiledRendering(tw, th, pw, ph, viewaspect);
			do 
			{
				project->Render(true);
				glFinish();
				int tr, tc, ctw, cth;
				pCam->GetTileInfo(&tr, &tc, &ctw, &cth);

				lpbi = (LPBITMAPINFOHEADER)GlobalLock(MakeDib(hBm, 24));
	
				BITMAPINFO bi;
				ZeroMemory(&bi, sizeof(BITMAPINFO));
				memcpy (&bi.bmiHeader, lpbi, sizeof(BITMAPINFOHEADER));

				pDC->SetStretchBltMode(COLORONCOLOR);
				StretchDIBits(pDC->m_hDC, rc.left+1+(w*c)+mx + tc*tw, rc.top+1+(h*r)+my + tr*th, ctw, cth, 0, 0, ctw, cth, 
					(LPBYTE) lpbi + lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD), &bi, DIB_RGB_COLORS, SRCCOPY);
				if (lpbi) GlobalFreePtr(lpbi);
			} while (pCam->EndTile());
		}
		else
		{
			project->Render(true);
			glFinish();
			lpbi = (LPBITMAPINFOHEADER)GlobalLock(MakeDib(hBm, 24));
			
			BITMAPINFO bi;
			ZeroMemory(&bi, sizeof(BITMAPINFO));
			memcpy (&bi.bmiHeader, lpbi, sizeof(BITMAPINFOHEADER));
			pDC->SetStretchBltMode(COLORONCOLOR);
			StretchDIBits(pDC->m_hDC, rc.left+1+(w*c)+mx, rc.top+1+(h*r)+my, w-(2*mx), h-(2*my), 0, 0, pw, ph, 
				(LPBYTE) lpbi + lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD), &bi, DIB_RGB_COLORS, SRCCOPY);
			if (lpbi) GlobalFreePtr(lpbi);
		}

		// OpenGL Rendering
//			CCamera* pOld = pDoc->GetActiveCamera();
//			pDoc->m_ViewCameras[pDoc->m_nActiveViewport] = pDoc->GetCamera(CAMERA_MAIN);
//			pDoc->m_ViewCameras[pDoc->m_nActiveViewport] = pOld;

		DWORD dwPrint = theApp.GetProfileInt("Settings","Print", PRINT_NUMBERS|PRINT_BORDER);
		if (dwPrint & PRINT_NUMBERS)
		{
			char tmp[4];
			sprintf (tmp, "%d", nRenderTime);
			pDC->TextOut(rc.left+(w*c)+(int)(pDC->GetDeviceCaps (LOGPIXELSX)/2), rc.top+(h*r)+(int)(pDC->GetDeviceCaps (LOGPIXELSY)), tmp, strlen (tmp));
		}

		if (dwPrint & PRINT_BORDER)
		{
			if (r == 0)
			{
				pDC->MoveTo(rc.left+(w*c), rc.top+(h*r));
				pDC->LineTo(rc.left+(w*(c+1)), rc.top+(h*r));
			}
			if (c == 0)
			{
				pDC->MoveTo(rc.left+(w*c), rc.top+(h*r));
				pDC->LineTo(rc.left+(w*c), rc.top+(h*(r+1)));
			}
			
			pDC->MoveTo(rc.left+(w*(c+1)), rc.top+(h*r));
			pDC->LineTo(rc.left+(w*(c+1)), rc.top+(h*(r+1)));
			pDC->MoveTo(rc.left+(w*c), rc.top+(h*(r+1)));
			pDC->LineTo(rc.left+(w*(c+1)), rc.top+(h*(r+1)));
		}
		nRenderTime++;
	}

	if (project->m_bAnimation)
		project->m_nCurFrame = nOldTime;
	else
		project->m_nCurStep = (unsigned char)nOldTime;
	project->m_nViewX = oldSizex;
	project->m_nViewY = oldSizey;

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hmemrc);
	SelectObject(hMemDC, hBmOld);
	DeleteObject(hBm);
	DeleteDC(hMemDC);

	SelectObject(pDC->m_hDC, hpOld);
	SelectObject(pDC->m_hDC, OldFont);
	DeleteObject(font);
	glFinish();
//		wglMakeCurrent(m_pDC->m_hDC, m_hglRC);

	lf.lfHeight = -MulDiv(12, pDC->GetDeviceCaps(LOGPIXELSY), 72);
	lf.lfWeight = FW_REGULAR;
	font = CreateFontIndirect(&lf);
	OldFont = (HFONT)SelectObject(pDC->m_hDC, font);
	pDC->SetTextColor(RGB (0,0,0));
	pDC->SetTextAlign (TA_TOP|TA_LEFT|TA_NOUPDATECP);
 	rc.top -= pDC->GetDeviceCaps(LOGPIXELSY)*theApp.GetProfileInt("Default","Margin Top", 50)/200;
	rc.bottom += pDC->GetDeviceCaps(LOGPIXELSY)*theApp.GetProfileInt("Default","Margin Bottom", 50)/200;
	PrintHeader(FALSE, pDC->GetSafeHdc(), rc, pInfo->m_nCurPage, FALSE);
	PrintHeader(TRUE, pDC->GetSafeHdc(), rc, pInfo->m_nCurPage, FALSE);
	SelectObject(pDC->m_hDC, OldFont);
	DeleteObject(font);
}

void CCADView::PrintHeader(BOOL bFooter, HDC hDC, CRect rc, UINT page, BOOL bCatalog)
{
	CString str,tmp;
	UINT nFormat = DT_CENTER;
	int r;

	if (bFooter)
		str = project->m_strFooter;
	else
		str = project->m_strHeader;

	if (str.GetLength())
	{
		while ((r = str.Find("&L")) != -1)
		{
			nFormat = DT_LEFT;
			tmp = str.Left (r);
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
		while ((r = str.Find("&C")) != -1)
		{
			nFormat = DT_CENTER;
			tmp = str.Left (r);
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
		while ((r = str.Find("&R")) != -1)
		{
			nFormat = DT_RIGHT;
			tmp = str.Left (r);
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
		while ((r = str.Find("&F")) != -1)
		{
			tmp = str.Left (r);
			if (bCatalog)
				tmp += "LeoCAD Parts Catalog";
			else
				tmp += project->m_strTitle;
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
		while ((r = str.Find("&A")) != -1)
		{
			tmp = str.Left (r);
			tmp += project->m_strAuthor;
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
		while ((r = str.Find("&N")) != -1)
		{
			tmp = str.Left (r);
			tmp += project->m_strDescription;
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
		while ((r = str.Find("&D")) != -1)
		{
			char dbuffer [9];
			_strdate( dbuffer );
			tmp = str.Left (r);
			tmp += dbuffer;
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
		while ((r = str.Find("&T")) != -1)
		{
			char tbuffer [9];
			_strtime( tbuffer );
			tmp = str.Left (r);
			tmp += tbuffer;
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
		while ((r = str.Find("&P")) != -1)
		{
			char buf[5];
			sprintf (buf, "%d", page);
			tmp = str.Left (r);
			tmp += buf;
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
	}

	if (bFooter)
		nFormat |= DT_BOTTOM|DT_SINGLELINE;
	else
		nFormat |= DT_TOP|DT_SINGLELINE;

	DrawText(hDC, (LPCTSTR)str, str.GetLength(), rc, nFormat);
}

void CCADView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	wglMakeCurrent(m_pDC->GetSafeHdc(), m_hglRC);
}

void CCADView::OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewViewEx* pView) 
{
//	CView::OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT, CPreviewView* pView)
	ASSERT_VALID(pDC);
	ASSERT_VALID(pView);

	if (pView->m_pPrintView != NULL)
		pView->m_pPrintView->OnEndPrinting(pDC, pInfo);

	CFrameWnd* pParent;
	CWnd* pNaturalParent = pView->GetParentFrame();
	pParent = DYNAMIC_DOWNCAST(CFrameWnd, pNaturalParent);
	if (pParent == NULL || pParent->IsIconic())
		pParent = (CFrameWnd*)AfxGetThread()->m_pMainWnd;

	ASSERT_VALID(pParent);
	ASSERT_KINDOF(CFrameWnd, pParent);

	// restore the old main window
	pParent->OnSetPreviewMode(FALSE, pView->m_pPreviewState);

	// Force active view back to old one
	pParent->SetActiveView(pView->m_pPreviewState->pViewActiveOld);
	if (pParent != GetParentFrame())
		OnActivateView(TRUE, this, this);   // re-activate view in real frame
	pView->DestroyWindow();     // destroy preview view
			// C++ object will be deleted in PostNcDestroy

	// restore main frame layout and idle message
	pParent->RecalcLayout();
	pParent->SendMessage(WM_SETMESSAGESTRING, (WPARAM)AFX_IDS_IDLEMESSAGE, 0L);
	pParent->UpdateWindow();
///


	wglMakeCurrent(NULL, NULL);

	if (GetPixelFormat(m_pDC->GetSafeHdc()) == 0)
	{
		delete m_pDC;
		m_pDC = new CClientDC(this);

		PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32,
 			0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
	
		int pixelformat = ChoosePixelFormat(m_pDC->GetSafeHdc(), &pfd);
		if (pixelformat == 0)
		{
			AfxMessageBox("ChoosePixelFormat failed");
		}

		if (SetPixelFormat(m_pDC->m_hDC, pixelformat, &pfd) == FALSE)
		{
			AfxMessageBox("SetPixelFormat failed");
		}
	}

	if (wglMakeCurrent(m_pDC->m_hDC, m_hglRC) == FALSE)
	{
		LPTSTR lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		    NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf, 0, NULL);

		::MessageBox(NULL, lpMsgBuf, "Error", MB_OK|MB_ICONINFORMATION);
		LocalFree(lpMsgBuf);	
	}

	InvalidateRect(NULL, FALSE);
}

BOOL CCADView::DoPrintPreview(UINT nIDResource, CView* pPrintView, CRuntimeClass* pPreviewViewClass, CPrintPreviewState* pState)
{
	ASSERT_VALID_IDR(nIDResource);
	ASSERT_VALID(pPrintView);
	ASSERT(pPreviewViewClass != NULL);
	ASSERT(pPreviewViewClass->IsDerivedFrom(RUNTIME_CLASS(CPreviewViewEx)));
	ASSERT(pState != NULL);

	CFrameWnd* pParent;
	CWnd* pNaturalParent = pPrintView->GetParentFrame();
	pParent = DYNAMIC_DOWNCAST(CFrameWnd, pNaturalParent);
	if (pParent == NULL || pParent->IsIconic())
		pParent = (CFrameWnd*)AfxGetThread()->m_pMainWnd;

	ASSERT_VALID(pParent);
	ASSERT_KINDOF(CFrameWnd, pParent);

	CCreateContext context;
	context.m_pCurrentFrame = pParent;
	context.m_pCurrentDoc = GetDocument();
	context.m_pLastView = this;

	// Create the preview view object
	CPreviewViewEx* pView = (CPreviewViewEx*)pPreviewViewClass->CreateObject();
	if (pView == NULL)
	{
		TRACE0("Error: Failed to create preview view.\n");
		return FALSE;
	}
	ASSERT_KINDOF(CPreviewViewEx, pView);
	pView->m_pPreviewState = pState;        // save pointer

	pParent->OnSetPreviewMode(TRUE, pState);    // Take over Frame Window

	// Create the toolbar from the dialog resource
	pView->m_pToolBar = new CFlatToolBar;

	if (!pView->m_pToolBar->Create(pParent, WS_CHILD | WS_VISIBLE | CBRS_TOP, AFX_IDW_PREVIEW_BAR) ||
		!pView->m_pToolBar->LoadToolBar(nIDResource))
	{
		TRACE0("Error: Preview could not create toolbar.\n");
		pParent->OnSetPreviewMode(FALSE, pState);   // restore Frame Window
		delete pView->m_pToolBar;       // not autodestruct yet
		pView->m_pToolBar = NULL;
		pView->m_pPreviewState = NULL;  // do not delete state structure
		delete pView;
		return FALSE;
	}
	pView->m_pToolBar->SetBarStyle(pView->m_pToolBar->GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

/*
	pView->m_pToolBar = new CDialogBar;
	if (!pView->m_pToolBar->Create(pParent, MAKEINTRESOURCE(nIDResource),
		CBRS_TOP, AFX_IDW_PREVIEW_BAR))
	{
		TRACE0("Error: Preview could not create toolbar dialog.\n");
		pParent->OnSetPreviewMode(FALSE, pState);   // restore Frame Window
		delete pView->m_pToolBar;       // not autodestruct yet
		pView->m_pToolBar = NULL;
		pView->m_pPreviewState = NULL;  // do not delete state structure
		delete pView;
		return FALSE;
	}
*/	pView->m_pToolBar->m_bAutoDelete = TRUE;    // automatic cleanup

	// Create the preview view as a child of the App Main Window.  This
	// is a sibling of this view if this is an SDI app.  This is NOT a sibling
	// if this is an MDI app.

	if (!pView->Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0,0,0,0), pParent, AFX_IDW_PANE_FIRST, &context))
	{
		TRACE0("Error: couldn't create preview view for frame.\n");
		pParent->OnSetPreviewMode(FALSE, pState);   // restore Frame Window
		pView->m_pPreviewState = NULL;  // do not delete state structure
		delete pView;
		return FALSE;
	}

	// Preview window shown now

	pState->pViewActiveOld = pParent->GetActiveView();
	CCADView* pActiveView = (CCADView*)pParent->GetActiveFrame()->GetActiveView();
	if (pActiveView != NULL)
		pActiveView->OnActivateView(FALSE, pActiveView, pActiveView);

	if (!pView->SetPrintView((CCADView*)pPrintView))
	{
		pView->OnPreviewClose();
		return TRUE;            // signal that OnEndPrintPreview was called
	}

	pParent->SetActiveView(pView);  // set active view - even for MDI

	// update toolbar and redraw everything
	pView->m_pToolBar->SendMessage(WM_IDLEUPDATECMDUI, (WPARAM)TRUE);
	pParent->RecalcLayout();            // position and size everything
	pParent->UpdateWindow();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCADView diagnostics

#ifdef _DEBUG
void CCADView::AssertValid() const
{
	CView::AssertValid();
}

void CCADView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CCADDoc* CCADView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCADDoc)));
	return (CCADDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCADView message handlers

BOOL CCADView::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

int CCADView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
    m_pDC = new CClientDC(this);
    ASSERT(m_pDC != NULL);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 24,				// RGBA, 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		32, 							// 16(32)-bit z-buffer
		0, 0,							// no stencil & auxiliary buffer
		PFD_MAIN_PLANE, 0, 0, 0, 0 };	// layer masks ignored
	
	int pixelformat = ChoosePixelFormat(m_pDC->GetSafeHdc(), &pfd);
	if (pixelformat == 0)
	{
		AfxMessageBox("ChoosePixelFormat failed");
		return -1;
	}

	if (SetPixelFormat(m_pDC->m_hDC, pixelformat, &pfd) == FALSE)
	{
		AfxMessageBox("SetPixelFormat failed");
		return -1;
	}

	m_pPalette = new CPalette;
	if (CreateRGBPalette(m_pDC->GetSafeHdc(), &m_pPalette))
	{
		m_pDC->SelectPalette(m_pPalette, FALSE);
		m_pDC->RealizePalette();
	}
	else
	{
		delete m_pPalette;
		m_pPalette = NULL;
	}

	m_hglRC = wglCreateContext(m_pDC->m_hDC);
	wglMakeCurrent(m_pDC->m_hDC, m_hglRC);

	SetTimer (IDT_LC_SAVETIMER, 5000, NULL);

	return 0;
}

void CCADView::OnPaletteChanged(CWnd* pFocusWnd) 
{
	// See if the change was caused by us and ignore it if not.
	if (pFocusWnd != this)
		OnQueryNewPalette();
}

BOOL CCADView::OnQueryNewPalette() 
{
	if (m_pPalette)
	{
		m_pDC->SelectPalette(m_pPalette, FALSE);
		if (m_pDC->RealizePalette() != 0)
		{
			// Some colors changed, so we need to do a repaint.
			InvalidateRect(NULL, TRUE);
		}
	}

	return TRUE;
}

void CCADView::OnDestroy() 
{
	wglMakeCurrent(NULL,  NULL);

	if (m_hglRC)
		wglDeleteContext(m_hglRC);

	if (m_pPalette)
	{
		CPalette palDefault;
		palDefault.CreateStockObject(DEFAULT_PALETTE);
		m_pDC->SelectPalette(&palDefault, FALSE);
		delete m_pPalette;
	}

	if (m_pDC)
		delete m_pDC;

	KillTimer (IDT_LC_SAVETIMER);
	
	CView::OnDestroy();
}

void CCADView::OnSize(UINT nType, int cx, int cy) 
{
	m_szView.cx = cx;
	m_szView.cy = cy;
	project->SetViewSize(cx, cy);
	CView::OnSize(nType, cx, cy);
}

void CCADView::OnMouseMove(UINT /*nFlags*/, CPoint point) 
{
	project->OnMouseMove(point.x, m_szView.cy - point.y - 1);
}

void CCADView::OnLButtonUp(UINT /*nFlags*/, CPoint point) 
{
	project->OnLeftButtonUp(point.x, m_szView.cy - point.y - 1);
}

void CCADView::OnLButtonDown(UINT /*nFlags*/, CPoint point) 
{
	project->OnLeftButtonDown(point.x, m_szView.cy - point.y - 1);
}

void CCADView::OnLButtonDblClk(UINT /*nFlags*/, CPoint point) 
{
	project->OnLeftButtonDoubleClick(point.x, m_szView.cy - point.y - 1);
}

void CCADView::OnRButtonDown(UINT /*nFlags*/, CPoint point) 
{
	project->OnRightButtonDown(point.x, m_szView.cy - point.y - 1);
}

void CCADView::OnRButtonUp(UINT /*nFlags*/, CPoint point) 
{
	project->OnRightButtonUp(point.x, m_szView.cy - point.y - 1);
}

void CCADView::OnDropDown (NMHDR* pNotifyStruct, LRESULT* pResult)
{
	NMTOOLBAR* pNMToolBar = (NMTOOLBAR*)pNotifyStruct;
	RECT rc;

	::SendMessage(pNMToolBar->hdr.hwndFrom, TB_GETRECT, (WPARAM)pNMToolBar->iItem, (LPARAM)&rc);

	CRect rect(rc);
	rect.top = rect.bottom;
	::ClientToScreen(pNMToolBar->hdr.hwndFrom, &rect.TopLeft());

	if (pNMToolBar->iItem == ID_SNAP_ON)
	{
		POINT pt = { rect.left, rect.top + 1 };
		SystemDoPopupMenu(2, pt.x, pt.y);
	}

	if (pNMToolBar->iItem == ID_LOCK_ON)
	{
		POINT pt = { rect.left, rect.top + 1 };
		SystemDoPopupMenu(8, pt.x, pt.y);
	}

	*pResult = TBDDRET_DEFAULT;
}

LONG CCADView::OnChangeCursor(UINT lParam, LONG /*wParam*/)
{
	UINT c;

	switch (lParam)
	{
		case LC_ACTION_SELECT:
			if (GetKeyState (VK_CONTROL) < 0)
				c = IDC_SELECT_GROUP;
			else
				c = IDC_SELECT;
			break;
		case LC_ACTION_INSERT: c = IDC_BRICK; break;
		case LC_ACTION_LIGHT: c = IDC_LIGHT; break;
		case LC_ACTION_SPOTLIGHT: c = IDC_SPOTLIGHT; break;
		case LC_ACTION_CAMERA: c = IDC_CAMERA; break;
		case LC_ACTION_MOVE: c = IDC_MOVE; break;
		case LC_ACTION_ROTATE: c = IDC_ROTATE; break;
		case LC_ACTION_ERASER: c = IDC_ERASER; break;
		case LC_ACTION_PAINT: c = IDC_PAINT; break;
		case LC_ACTION_ZOOM: c = IDC_ZOOM; break;
		case LC_ACTION_ZOOM_REGION: c = IDC_ZOOM_REGION; break;
		case LC_ACTION_PAN: c = IDC_PAN; break;
		case LC_ACTION_ROTATE_VIEW: c = IDC_ANGLE; break;
		case LC_ACTION_ROLL: c = IDC_ROLL; break;
		default:
			c = NULL;
	}

	if (c)
	{
		m_hCursor = theApp.LoadCursor(c);
		SetCursor(m_hCursor);
	}
	else
		m_hCursor = NULL;

	return TRUE;
}

BOOL CCADView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_hCursor)
	{
		SetCursor(m_hCursor);
		return TRUE;
	}

	return CView::OnSetCursor(pWnd, nHitTest, message);
}

BOOL CCADView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if (zDelta > 0)
		project->HandleCommand(LC_VIEW_ZOOMOUT, 0);
	else
		project->HandleCommand(LC_VIEW_ZOOMIN, 0);

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CCADView::OnMButtonDown(UINT nFlags, CPoint point) 
{
	BOOL bCtl = GetKeyState(VK_CONTROL) & 0x8000;
	if(!bCtl && nFlags == MK_MBUTTON)
	{
		CWheelWnd *pwndPanWindow = new CWheelWnd(point);
		if (!pwndPanWindow->Create(this))
			delete pwndPanWindow;
	}
	else
		CView::OnMButtonDown(nFlags, point);
}

// Notification from the auto-pan window
LONG CCADView::OnAutoPan(UINT lParam, LONG wParam)
{
	CPoint pt1(lParam), pt2(wParam);
	pt2 -= pt1;
	pt2.y = -pt2.y;

	unsigned long pt = ((short)pt2.x) | ((unsigned long)(((short)pt2.y) << 16));
	project->HandleCommand(LC_VIEW_AUTOPAN, pt);	

	return TRUE;
}

void CCADView::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == IDT_LC_SAVETIMER)
		project->CheckAutoSave();
	
	CView::OnTimer(nIDEvent);
}

// lParam -> new step/frame
LONG CCADView::OnSetStep(UINT lParam, LONG /*wParam*/)
{
	if (lParam > 0)
		project->HandleCommand(LC_VIEW_STEP_SET, lParam);

	return TRUE;
}

void CCADView::OnCaptureChanged(CWnd *pWnd) 
{
	if (pWnd != this)
		project->HandleNotify(LC_CAPTURE_LOST, 0);
	
	CView::OnCaptureChanged(pWnd);
}









void CCADView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	char nKey = nChar;

	switch (nChar)
	{
		case VK_NUMPAD0 : case VK_NUMPAD1 : case VK_NUMPAD2 : case VK_NUMPAD3 : case VK_NUMPAD4 : 
		case VK_NUMPAD5 : case VK_NUMPAD6 : case VK_NUMPAD7 : case VK_NUMPAD8 : case VK_NUMPAD9 :
		{
			nKey = nChar - VK_NUMPAD0 + 0x30;
		} break;
	}

	project->OnKeyDown(nKey, GetKeyState (VK_CONTROL) < 0, GetKeyState (VK_SHIFT) < 0);
/*
	switch (nChar)
	{
// HANDLE CTRL if action == pan/zoom
		case VK_CONTROL: 
		if (m_nCurAction == ACTION_SELECT)
		{
			POINT pt;
			GetCursorPos(&pt);
			CRect rc;
			GetWindowRect(rc);
			if (rc.PtInRect(pt))
				OnSetCursor(this, HTCLIENT, 0);
		} break;
*/
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CCADView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
/*	if (nChar == VK_CONTROL)
	{
// HANDLE CTRL if action == pan/zoom
		CCADDoc* pDoc = GetDocument();
		if (pDoc->m_nCurAction == ACTION_SELECT)
		{
			POINT pt;
			GetCursorPos(&pt);
			CRect rc;
			GetWindowRect(rc);
			if (rc.PtInRect(pt))
				OnSetCursor(this, HTCLIENT, 0);
		}
	}
*/	
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CCADView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
/*
	if (IsWindowEnabled())
	{
		if (bActivate)
		{
			if (m_pPixels)
				free (m_pPixels);
			m_pPixels = NULL;
		}
		else
		{
			if (m_pPixels)
				free (m_pPixels);

			CCADDoc* pDoc = GetDocument();
			m_pPixels = malloc(pDoc->m_szView.cx * pDoc->m_szView.cy * sizeof(GLubyte) * 4);
			if (!m_pPixels)
				return;
			glReadPixels(0, 0, pDoc->m_szView.cx, pDoc->m_szView.cy, GL_RGBA, GL_UNSIGNED_BYTE, m_pPixels);
		}
	}
*/
}
