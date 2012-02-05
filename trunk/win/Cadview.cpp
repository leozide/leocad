// CADView.cpp : implementation of the CCADView class
//

#include "stdafx.h"
#include "LeoCAD.h"
#include <WindowsX.h>

#include "CADDoc.h"
#include "CADView.h"
#include "Tools.h"
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
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
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
		CFrameWndEx* pFrame = (CFrameWndEx*)AfxGetMainWnd();

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

	if (tw > 1024 || th > 1024)
		tw = th = 1024;

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

	View view(project, project->m_ActiveView);
	view.m_Camera = project->m_ActiveView->m_Camera;
	view.CreateFromBitmap(hMemDC);
	view.MakeCurrent();
	view.OnSize(tw, th);
	project->AddView(&view);

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
				project->Render(&view, true);
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
			project->Render(&view, true);
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

	if (project->m_bAnimation)
		project->m_nCurFrame = nOldTime;
	else
		project->m_nCurStep = (unsigned char)nOldTime;

	view.DestroyContext();
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
//	pfnwglMakeCurrent(m_pDC->GetSafeHdc(), m_hglRC);
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

	m_pView = new View(project, project->m_ActiveView);
	if (project->m_ActiveView)
		m_pView->m_Camera = project->m_ActiveView->m_Camera;
	else
		m_pView->m_Camera = project->GetCamera(LC_CAMERA_MAIN);
	m_pView->CreateFromWindow(m_hWnd);
	m_pView->OnInitialUpdate();

	SetTimer(IDT_LC_SAVETIMER, 5000, NULL);

	return 0;
}

void CCADView::OnDestroy() 
{
	delete m_pView;
	m_pView = NULL;

	KillTimer(IDT_LC_SAVETIMER);

	CView::OnDestroy();
}

void CCADView::OnDropDown(NMHDR* pNotifyStruct, LRESULT* pResult)
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
	CView::OnMButtonDown(nFlags, point);
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

	switch (nChar)
	{
		case VK_NUMPAD0 : case VK_NUMPAD1 : case VK_NUMPAD2 : case VK_NUMPAD3 : case VK_NUMPAD4 : 
		case VK_NUMPAD5 : case VK_NUMPAD6 : case VK_NUMPAD7 : case VK_NUMPAD8 : case VK_NUMPAD9 :
		{
			nKey = nChar - VK_NUMPAD0 + 0x30;
		} break;

    // select the next/previous piece on the pieces list
    case VK_HOME:
    case VK_END:
    {
      /*
      CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd ();
      CPiecesList& pList = pMain->m_wndPiecesBar.m_wndPiecesList;
      LV_FINDINFO lvfi;
      int sel;

			lvfi.flags = LVFI_PARAM;
			lvfi.lParam = (LPARAM)pMain->m_wndPiecesBar.m_wndPiecePreview.GetPieceInfo ();
			sel = pList.FindItem (&lvfi);

      if (sel != -1)
      {
        if (nChar == VK_HOME)
          sel--;
        else
          sel++;

  			pList.SetItemState (sel, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
	  		pList.EnsureVisible (sel, FALSE);
      }
*/
    } break;
	}

	lcGetActiveProject()->OnKeyDown(nKey, GetKeyState (VK_CONTROL) < 0, GetKeyState (VK_SHIFT) < 0);
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
//	if (pActivateView && pActivateView->IsKindOf(RUNTIME_CLASS(CCADView)))
//		lcGetActiveProject()->SetActiveView(((CCADView*)pActivateView)->m_pView);

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
