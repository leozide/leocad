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
#include "camera.h"
#include "view.h"
#include "MainFrm.h"
#include "PiecePrv.h"
#include "lc_application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL GLWindowPreTranslateMessage (GLWindow *wnd, MSG *pMsg);

/////////////////////////////////////////////////////////////////////////////
// CCADView

IMPLEMENT_DYNCREATE(CCADView, CView)

BEGIN_MESSAGE_MAP(CCADView, CView)
	//{{AFX_MSG_MAP(CCADView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETCURSOR()
	ON_WM_CAPTURECHANGED()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_MBUTTONDOWN()
	ON_WM_TIMER()
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
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
	m_hCursor = NULL;
  m_pView = NULL;
}

CCADView::~CCADView()
{
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
	int Max = (lcGetActiveProject()->GetLastStep()/(rows*cols));
	if (lcGetActiveProject()->GetLastStep()%(rows*cols) != 0) 
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
	int Max = (lcGetActiveProject()->GetLastStep()/(rows*cols));
	if (lcGetActiveProject()->GetLastStep()%(rows*cols) != 0)
		Max++;
	pInfo->SetMaxPage(Max);
}

void CCADView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
	int cols = theApp.GetProfileInt("Default","Print Columns", 1);
	int rows = theApp.GetProfileInt("Default","Print Rows", 1);
	Project* project = lcGetActiveProject();

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
	float viewaspect = (float)m_pView->GetWidth ()/(float)m_pView->GetHeight ();
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
	int pixelformat = OpenGLChoosePixelFormat(hMemDC, &pfd);
	OpenGLDescribePixelFormat (hMemDC, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	OpenGLSetPixelFormat (hMemDC, pixelformat, &pfd);
	
	// Creating OpenGL context
  HGLRC hmemrc = pfnwglCreateContext(hMemDC);
	pfnwglMakeCurrent(hMemDC, hmemrc);
//	if (!pfnwglShareLists(m_hglRC, hmemrc))
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

	u32 OldTime = project->GetCurrentTime();
	UINT nRenderTime = 1+((pInfo->m_nCurPage-1)*rows*cols);

	View view(project, NULL);
	view.OnSize(tw, th);
	view.SetCamera(project->GetCamera(LC_CAMERA_MAIN));

	for (int r = 0; r < rows; r++)
	for (int c = 0; c < cols; c++)
	{
		if (nRenderTime > project->GetLastStep())
			continue;
		project->m_ActiveModel->m_CurTime = nRenderTime;
		project->CalculateStep();
		FillRect(hMemDC, CRect(0,th,tw,0), (HBRUSH)GetStockObject(WHITE_BRUSH));

		// Tile rendering
		if (tw != pw)
		{
			Camera* pCam = view.GetCamera();
			pCam->StartTiledRendering(tw, th, pw, ph, viewaspect);
			do 
			{
				project->Render(&view, false, false);
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
			project->Render(&view, false, false);
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

			CRect rcNumber (rc);
			rcNumber.left += (w*c)+(int)(pDC->GetDeviceCaps (LOGPIXELSX)/2);
			rcNumber.top += (h*r)+(int)(pDC->GetDeviceCaps (LOGPIXELSY)/2);

			pDC->SetTextAlign (TA_TOP|TA_LEFT|TA_NOUPDATECP);
			pDC->DrawText(tmp, strlen (tmp), rcNumber, DT_LEFT|DT_TOP|DT_SINGLELINE);
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

	project->m_ActiveModel->m_CurTime = OldTime;

	pfnwglMakeCurrent(NULL, NULL);
	pfnwglDeleteContext(hmemrc);
	SelectObject(hMemDC, hBmOld);
	DeleteObject(hBm);
	DeleteDC(hMemDC);

	SelectObject(pDC->m_hDC, hpOld);
	SelectObject(pDC->m_hDC, OldFont);
	DeleteObject(font);
	glFinish();

	lf.lfHeight = -MulDiv(12, pDC->GetDeviceCaps(LOGPIXELSY), 72);
	lf.lfWeight = FW_REGULAR;
	font = CreateFontIndirect(&lf);
	OldFont = (HFONT)SelectObject(pDC->m_hDC, font);
	pDC->SetTextColor(RGB (0,0,0));
	pDC->SetTextAlign (TA_TOP|TA_LEFT|TA_NOUPDATECP);
 	rc.top -= pDC->GetDeviceCaps(LOGPIXELSY)*theApp.GetProfileInt("Default","Margin Top", 50)/200;
	rc.bottom += pDC->GetDeviceCaps(LOGPIXELSY)*theApp.GetProfileInt("Default","Margin Bottom", 50)/200;
	PrintHeader(FALSE, pDC->GetSafeHdc(), rc, pInfo->m_nCurPage, pInfo->GetMaxPage(), FALSE);
	PrintHeader(TRUE, pDC->GetSafeHdc(), rc, pInfo->m_nCurPage, pInfo->GetMaxPage(), FALSE);
	SelectObject(pDC->m_hDC, OldFont);
	DeleteObject(font);
}

void CCADView::PrintHeader(BOOL bFooter, HDC hDC, CRect rc, UINT nCurPage, UINT nMaxPage, BOOL bCatalog)
{
	Project* project = lcGetActiveProject();
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
				tmp += "LeoCAD Pieces Catalog";
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
			sprintf (buf, "%d", nCurPage);
			tmp = str.Left (r);
			tmp += buf;
			tmp += str.Right(str.GetLength()-r-2);
			str = tmp;
		}
		while ((r = str.Find("&O")) != -1)
		{
			char buf[5];
			sprintf (buf, "%d", nMaxPage);
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

int CCADView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	Project* project = lcGetActiveProject();

	m_pView = new View(project, project->GetFirstView());
	m_pView->Create(m_hWnd);
	m_pView->OnInitialUpdate();

	CCADView* ActiveView = (CCADView*)GetParentFrame()->GetActiveView();
	if (ActiveView)
	{
		m_pView->SetCamera(ActiveView->m_pView->GetCamera());
	}

	SetTimer (IDT_LC_SAVETIMER, 5000, NULL);

	return 0;
}

void CCADView::OnDestroy() 
{
	delete m_pView;
	m_pView = NULL;

	KillTimer (IDT_LC_SAVETIMER);

	CView::OnDestroy();
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
	UINT Cursor;

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	switch (m_pView->GetCursor(pt.x, pt.y))
	{
		case LC_CURSOR_NONE: Cursor = NULL; break;
		case LC_CURSOR_BRICK: Cursor = IDC_BRICK; break;
		case LC_CURSOR_LIGHT: Cursor = IDC_LIGHT; break;
		case LC_CURSOR_SPOTLIGHT: Cursor = IDC_SPOTLIGHT; break;
		case LC_CURSOR_CAMERA: Cursor = IDC_CAMERA; break;
		case LC_CURSOR_SELECT: Cursor = IDC_SELECT; break;
		case LC_CURSOR_SELECT_GROUP: Cursor = IDC_SELECT_GROUP; break;
		case LC_CURSOR_MOVE: Cursor = IDC_MOVE; break;
		case LC_CURSOR_ROTATE: Cursor = IDC_ROTATE; break;
		case LC_CURSOR_ROTATEX: Cursor = IDC_ROTX; break;
		case LC_CURSOR_ROTATEY: Cursor = IDC_ROTY; break;
		case LC_CURSOR_DELETE: Cursor = IDC_ERASER; break;
		case LC_CURSOR_PAINT: Cursor = IDC_PAINT; break;
		case LC_CURSOR_ZOOM: Cursor = IDC_ZOOM; break;
		case LC_CURSOR_ZOOM_REGION: Cursor = IDC_ZOOM_REGION; break;
		case LC_CURSOR_PAN: Cursor = IDC_PAN; break;
		case LC_CURSOR_ROLL: Cursor = IDC_ROLL; break;
		case LC_CURSOR_ROTATE_VIEW: Cursor = IDC_ANGLE; break;

		default:
			LC_ASSERT_FALSE("Unknown cursor type.");
	}

	if (Cursor)
	{
		m_hCursor = theApp.LoadCursor(Cursor);
		SetCursor(m_hCursor);
	}
	else
		m_hCursor = NULL;

	return TRUE;
}

BOOL CCADView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (CView::OnSetCursor(pWnd, nHitTest, message))
		return TRUE;

	OnChangeCursor(0, 0);

	if (m_hCursor)
	{
		SetCursor(m_hCursor);
		return TRUE;
	}

	return FALSE;
}

BOOL CCADView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if (zDelta > 0)
		lcGetActiveProject()->HandleCommand(LC_VIEW_ZOOMOUT, 0);
	else
		lcGetActiveProject()->HandleCommand(LC_VIEW_ZOOMIN, 0);

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
	lcGetActiveProject()->HandleCommand(LC_VIEW_AUTOPAN, pt);	

	return TRUE;
}

void CCADView::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == IDT_LC_SAVETIMER)
		lcGetActiveProject()->CheckAutoSave();
	
	CView::OnTimer(nIDEvent);
}

// lParam -> new step/frame
LONG CCADView::OnSetStep(UINT lParam, LONG /*wParam*/)
{
	if (lParam > 0)
		lcGetActiveProject()->HandleCommand(LC_VIEW_STEP_SET, lParam);

	return TRUE;
}

void CCADView::OnCaptureChanged(CWnd *pWnd) 
{
	if (pWnd != this)
		lcGetActiveProject()->HandleNotify(LC_CAPTURE_LOST, 0);
	
	CView::OnCaptureChanged(pWnd);
}

void CCADView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	char nKey = nChar;

	if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9)
	{
		nKey = nChar - VK_NUMPAD0 + '0';
	}

	// Update cursor for multiple selection.
	if (nChar == VK_CONTROL)
	{
		if (lcGetActiveProject()->GetAction() == LC_ACTION_SELECT)
		{
			POINT pt;

			GetCursorPos(&pt);
			CRect rc;
			GetWindowRect(rc);

			if (rc.PtInRect(pt))
				OnSetCursor(this, HTCLIENT, 0);
		}
	}

	lcGetActiveProject()->OnKeyDown(nKey, GetKeyState(VK_CONTROL) < 0, GetKeyState(VK_SHIFT) < 0);

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CCADView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// Update cursor for multiple selection.
	if (nChar == VK_CONTROL)
	{
		if (lcGetActiveProject()->GetAction() == LC_ACTION_SELECT)
		{
			POINT pt;

			GetCursorPos(&pt);
			CRect rc;
			GetWindowRect(rc);

			if (rc.PtInRect(pt))
				OnSetCursor(this, HTCLIENT, 0);
		}
	}

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CCADView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if (pActivateView && pActivateView->IsKindOf(RUNTIME_CLASS(CCADView)))
		lcGetActiveProject()->SetActiveView(((CCADView*)pActivateView)->m_pView);
	else
		lcGetActiveProject()->SetActiveView(NULL);

	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

LRESULT CCADView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (m_pView)
	{
		MSG msg;

		msg.message = message;
		msg.wParam = wParam;
		msg.lParam = lParam;

		if (GLWindowPreTranslateMessage(m_pView, &msg))
			return TRUE;
	}

	return CView::WindowProc(message, wParam, lParam);
}
