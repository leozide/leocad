// SizingControlBar.cpp : implementation file
//

#include "stdafx.h"
#include "afxpriv.h"    // for CDockContext
#include "resource.h"
#include "piecebar.h"
#include "project.h"
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////
// CPiecesBar

CPiecesBar::CPiecesBar()
{
	int i = AfxGetApp()->GetProfileInt("Settings", "Piecebar Options", 
		PIECEBAR_PREVIEW|PIECEBAR_GROUP|PIECEBAR_COMBO|PIECEBAR_ZOOMPREVIEW);
	m_bPreview = (i & PIECEBAR_PREVIEW) != 0;
	m_bSubParts = (i & PIECEBAR_SUBPARTS) != 0;
	m_bGroups = (i & PIECEBAR_GROUP) != 0;
	m_bCombo = (i & PIECEBAR_COMBO) != 0;
	m_bNumbers = (i & PIECEBAR_PARTNUMBERS) != 0;
	m_wndPiecePreview.m_bZoomPreview = (i & PIECEBAR_ZOOMPREVIEW) != 0;
	m_nCurGroup = 1;

    m_sizeMin = CSize(222, 200);
    m_sizeHorz = CSize(200, 200);
    m_sizeVert = CSize(200, 200);
    m_sizeFloat = CSize(200, 200);
    m_bTracking = FALSE;
    m_bInRecalcNC = FALSE;
    m_cxEdge = 5;
	m_bDragShowContent = FALSE;
	m_nPreviewHeight = AfxGetApp()->GetProfileInt("Settings", "Preview Height", 93);
	m_bNoContext = FALSE;
}

CPiecesBar::~CPiecesBar()
{
	AfxGetApp()->WriteProfileInt("Settings", "Preview Height", m_nPreviewHeight);
}

BEGIN_MESSAGE_MAP(CPiecesBar, CControlBar)
    //{{AFX_MSG_MAP(CPiecesBar)
    ON_WM_PAINT()
    ON_WM_NCPAINT()
    ON_WM_WINDOWPOSCHANGED()
    ON_WM_NCCALCSIZE()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_NCLBUTTONDOWN()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
    ON_WM_CAPTURECHANGED()
    ON_WM_NCHITTEST()
    ON_WM_SETCURSOR()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
	ON_LBN_SELCHANGE(IDW_COLORSLIST, OnSelChangeColor)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_MESSAGE(WM_LC_SPLITTER_MOVED, OnSplitterMoved)
	ON_COMMAND_RANGE(ID_PIECE_GROUP01, ID_PIECE_GROUP32, OnPieceGroupClicked)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////
// CPiecesBar message handlers

BOOL CPiecesBar::Create(LPCTSTR lpszWindowName, CWnd* pParentWnd,
                               CSize sizeDefault, BOOL bHasGripper,
                               UINT nID, DWORD dwStyle)
{
    ASSERT_VALID(pParentWnd);   // must have a parent
    ASSERT (!((dwStyle & CBRS_SIZE_FIXED)
        && (dwStyle & CBRS_SIZE_DYNAMIC)));

    // save the style
    SetBarStyle(dwStyle & CBRS_ALL);

    CString wndclass = ::AfxRegisterWndClass(CS_DBLCLKS,
        ::LoadCursor(NULL, IDC_ARROW),
        ::GetSysColorBrush(COLOR_BTNFACE), 0);

    dwStyle &= ~CBRS_ALL;
    dwStyle &= WS_VISIBLE | WS_CHILD;
    if (!CWnd::Create(wndclass, lpszWindowName, dwStyle, CRect(0,0,0,0),
        pParentWnd, nID))
        return FALSE;

    m_sizeHorz = sizeDefault;
    m_sizeVert = sizeDefault;
    m_sizeFloat = sizeDefault;

    m_bHasGripper = bHasGripper;
    m_cyGripper = m_bHasGripper ? 12 : 0;

    return TRUE;
}

BOOL CPiecesBar::IsHorzDocked() const
{
    return (m_nDockBarID == AFX_IDW_DOCKBAR_TOP ||
        m_nDockBarID == AFX_IDW_DOCKBAR_BOTTOM);
}

BOOL CPiecesBar::IsVertDocked() const
{
    return (m_nDockBarID == AFX_IDW_DOCKBAR_LEFT ||
        m_nDockBarID == AFX_IDW_DOCKBAR_RIGHT);
}

CSize CPiecesBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
    CRect rc;

    m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_TOP)->GetWindowRect(rc);
    int nHorzDockBarWidth = bStretch ? 32767 : rc.Width() + 4;
    m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_LEFT)->GetWindowRect(rc);
    int nVertDockBarHeight = bStretch ? 32767 : rc.Height() + 4;

    if (bHorz)
        return CSize(nHorzDockBarWidth, m_sizeHorz.cy);
    else
        return CSize(m_sizeVert.cx, nVertDockBarHeight);
}

CSize CPiecesBar::CalcDynamicLayout(int nLength, DWORD dwMode)
{
    if (dwMode & (LM_HORZDOCK | LM_VERTDOCK))
    {
        if (nLength == -1)
            GetDockingFrame()->DelayRecalcLayout();
        return CControlBar::CalcDynamicLayout(nLength,dwMode);
    }

    if (dwMode & LM_MRUWIDTH)
        return m_sizeFloat;

    if (dwMode & LM_COMMIT)
    {
        m_sizeFloat.cx = nLength;
        return m_sizeFloat;
    }

    if (dwMode & LM_LENGTHY)
        return CSize(m_sizeFloat.cx,
            m_sizeFloat.cy = max(m_sizeMin.cy, nLength));
    else
        return CSize(max(m_sizeMin.cx, nLength), m_sizeFloat.cy);
}

void CPiecesBar::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos) 
{
    CControlBar::OnWindowPosChanged(lpwndpos);

    // Find on which side are we docked
    m_nDockBarID = GetParent()->GetDlgCtrlID();

    if (!m_bInRecalcNC)
    {
        m_bInRecalcNC = TRUE;

        // Force recalc the non-client area
        SetWindowPos(NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE |
            SWP_NOACTIVATE | SWP_NOZORDER |
            SWP_FRAMECHANGED);

        m_bInRecalcNC = FALSE;
    }
}

BOOL CPiecesBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
    if ((nHitTest != HTSIZE) || m_bTracking)
        return CControlBar::OnSetCursor(pWnd, nHitTest, message);

    if (IsHorzDocked())
        ::SetCursor(::LoadCursor(NULL, IDC_SIZENS));
    else
        ::SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////
// Mouse Handling
//
void CPiecesBar::OnLButtonDown(UINT nFlags, CPoint point) 
{
    if (m_pDockBar != NULL)
    {
        // start the drag
        ASSERT(m_pDockContext != NULL);
        ClientToScreen(&point);
        m_pDockContext->StartDrag(point);
    }
    else
        CWnd::OnLButtonDown(nFlags, point);
}

void CPiecesBar::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
    if (m_pDockBar != NULL)
    {
        // toggle docking
        ASSERT(m_pDockContext != NULL);
        m_pDockContext->ToggleDocking();
    }
    else
        CWnd::OnLButtonDblClk(nFlags, point);
}

void CPiecesBar::OnNcLButtonDown(UINT nHitTest, CPoint point) 
{
    if (m_bTracking) return;

    if ((nHitTest == HTSIZE) && !IsFloating())
        StartTracking();
    else    
        CControlBar::OnNcLButtonDown(nHitTest, point);
}

void CPiecesBar::OnLButtonUp(UINT nFlags, CPoint point) 
{
    if (m_bTracking)
		StopTracking(TRUE);

    CControlBar::OnLButtonUp(nFlags, point);
}

void CPiecesBar::OnRButtonDown(UINT nFlags, CPoint point) 
{
	if (m_bTracking)
		StopTracking(FALSE);

	CControlBar::OnRButtonDown(nFlags, point);
}

void CPiecesBar::OnMouseMove(UINT nFlags, CPoint point) 
{
    if (m_bTracking)
    {
		ASSERT (!IsFloating());
		CPoint pt = point;
		ClientToScreen(&pt);

		OnTrackUpdateSize(pt);
    }

    CControlBar::OnMouseMove(nFlags, point);
}

void CPiecesBar::OnCaptureChanged(CWnd *pWnd) 
{
    if (m_bTracking && pWnd != this)
        StopTracking(FALSE); // cancel tracking

    CControlBar::OnCaptureChanged(pWnd);
}

void CPiecesBar::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp) 
{
	UNREFERENCED_PARAMETER(bCalcValidRects);

    // Compute the rectangle of the mobile edge
    GetWindowRect(m_rectBorder);
    m_rectBorder.OffsetRect(-m_rectBorder.left, -m_rectBorder.top);
    m_rectBorder.DeflateRect(1, 1);
    
    m_rectGripper = m_rectBorder;
    m_rectGripper.DeflateRect(5, 5);
    m_rectGripper.right -= m_cxEdge;
    m_rectGripper.bottom -= m_cxEdge;
    CRect rc = lpncsp->rgrc[0];

    DWORD dwBorderStyle = m_dwStyle | CBRS_BORDER_ANY;

    switch(m_nDockBarID)
    {
    case AFX_IDW_DOCKBAR_TOP:
        dwBorderStyle &= ~CBRS_BORDER_BOTTOM;
        rc.DeflateRect(m_cyGripper + 2, 2, 2, m_cxEdge + 2);
        m_rectBorder.top = m_rectBorder.bottom - m_cxEdge;
        break;
    case AFX_IDW_DOCKBAR_BOTTOM:
        dwBorderStyle &= ~CBRS_BORDER_TOP;
        rc.DeflateRect(m_cyGripper + 2, m_cxEdge + 2, 2, 2);
        m_rectBorder.bottom = m_rectBorder.top + m_cxEdge;
        m_rectGripper.OffsetRect(0, m_cxEdge);
        break;
    case AFX_IDW_DOCKBAR_LEFT:
        dwBorderStyle &= ~CBRS_BORDER_RIGHT;
        rc.DeflateRect(2, m_cyGripper + 2, m_cxEdge + 2, 6);
        m_rectBorder.left = m_rectBorder.right - m_cxEdge;
        break;
    case AFX_IDW_DOCKBAR_RIGHT:
        dwBorderStyle &= ~CBRS_BORDER_LEFT;
        rc.DeflateRect(m_cxEdge + 2, m_cyGripper + 2, 2, 6);
        m_rectBorder.right = m_rectBorder.left + m_cxEdge;
        m_rectGripper.OffsetRect(m_cxEdge, 0);
        break;
    default:
        m_rectBorder.SetRectEmpty();
        break;
    }

    lpncsp->rgrc[0] = rc;

    SetBarStyle(dwBorderStyle);
}

void CPiecesBar::OnNcPaint() 
{
    // get window DC that is clipped to the non-client area
    CWindowDC dc(this);
    CRect rectClient;
    GetClientRect(rectClient);
    CRect rectWindow;
    GetWindowRect(rectWindow);
    ScreenToClient(rectWindow);
    rectClient.OffsetRect(-rectWindow.left, -rectWindow.top);
    dc.ExcludeClipRect(rectClient);

    // draw borders in non-client area
    rectWindow.OffsetRect(-rectWindow.left, -rectWindow.top);
    DrawBorders(&dc, rectWindow);

    // erase parts not drawn
    dc.IntersectClipRect(rectWindow);

    // erase NC background the hard way
    HBRUSH hbr = (HBRUSH)GetClassLong(m_hWnd, GCL_HBRBACKGROUND);
    ::FillRect(dc.m_hDC, rectWindow, hbr);

    // paint the mobile edge
    dc.Draw3dRect(m_rectBorder, ::GetSysColor(COLOR_BTNHIGHLIGHT),
        ::GetSysColor(COLOR_BTNSHADOW));

    if (m_bHasGripper)
    {
        // paint the gripper
        CRect gripper = m_rectGripper;
        
        if (IsHorzDocked())
        {
            // gripper at left
            gripper.right = gripper.left + 3;
            dc.Draw3dRect(gripper, ::GetSysColor(COLOR_BTNHIGHLIGHT),
                ::GetSysColor(COLOR_BTNSHADOW));
            gripper.OffsetRect(3, 0);
            dc.Draw3dRect(gripper, ::GetSysColor(COLOR_BTNHIGHLIGHT),
                ::GetSysColor(COLOR_BTNSHADOW));
        }
        else if (IsVertDocked())
        {
            // gripper at top
            gripper.bottom = gripper.top + 3;
            dc.Draw3dRect(gripper, ::GetSysColor(COLOR_BTNHIGHLIGHT),
                ::GetSysColor(COLOR_BTNSHADOW));
            gripper.OffsetRect(0, 3);
            dc.Draw3dRect(gripper, ::GetSysColor(COLOR_BTNHIGHLIGHT),
                ::GetSysColor(COLOR_BTNSHADOW));
        }
    }

    ReleaseDC(&dc);
}

void CPiecesBar::OnPaint() 
{
    // overridden to skip border painting based on clientrect
    CPaintDC dc(this);
}

UINT CPiecesBar::OnNcHitTest(CPoint point) 
{
    if (IsFloating())
        return CControlBar::OnNcHitTest(point);

    CRect rc;
    GetWindowRect(rc);
    point.Offset(-rc.left, -rc.top);
    if (m_rectBorder.PtInRect(point))
        return HTSIZE;
    else
        return HTCLIENT;
}

/////////////////////////////////////////////////////////////////////////
// CPiecesBar implementation helpers

void CPiecesBar::StartTracking()
{
    SetCapture();

    // make sure no updates are pending
    RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW);
	
	m_bDragShowContent = QueryDragFullWindows();
	
	if (!m_bDragShowContent)
	    m_pDockSite->LockWindowUpdate();

	m_sizeOld = IsHorzDocked() ? m_sizeHorz : m_sizeVert;

	CRect rect;
	GetWindowRect(&rect);
    m_ptOld = m_rectBorder.CenterPoint() + rect.TopLeft();

	m_sizeMax = CalcMaxSize();
    m_bTracking = TRUE;
    
	if (!m_bDragShowContent)
	    OnTrackInvertTracker();
}

void CPiecesBar::StopTracking(BOOL bAccept)
{
	if (!m_bDragShowContent)
	{
		OnTrackInvertTracker();
	    m_pDockSite->UnlockWindowUpdate();
	}

    m_bTracking = FALSE;
    ReleaseCapture();
    
    if (!bAccept) // resize canceled?
	{
		// restore old size
		if (IsHorzDocked())
	        m_sizeHorz = m_sizeOld;
		else
			m_sizeVert = m_sizeOld;
	}

    m_pDockSite->DelayRecalcLayout();
}

void CPiecesBar::OnTrackUpdateSize(CPoint& point)
{
	BOOL bHorz = IsHorzDocked();

    CSize sizeNew = m_sizeOld;
	
    if ((m_nDockBarID == AFX_IDW_DOCKBAR_TOP) ||
		(m_nDockBarID == AFX_IDW_DOCKBAR_LEFT))
        sizeNew += point - m_ptOld;
	else
        sizeNew -= point - m_ptOld;

	// check limits
  	sizeNew.cx = max(m_sizeMin.cx, sizeNew.cx);
  	sizeNew.cy = max(m_sizeMin.cy, sizeNew.cy);
  	sizeNew.cx = min(m_sizeMax.cx, sizeNew.cx);
  	sizeNew.cy = min(m_sizeMax.cy, sizeNew.cy);

    if ((sizeNew.cy == m_sizeHorz.cy) && bHorz ||
		(sizeNew.cx == m_sizeVert.cx) && !bHorz)
		return; // no size change

	if (!m_bDragShowContent)
		OnTrackInvertTracker();

    if (bHorz)
		m_sizeHorz = sizeNew;
	else
		m_sizeVert = sizeNew;

	if (!m_bDragShowContent)
		OnTrackInvertTracker();
	else
	    m_pDockSite->DelayRecalcLayout();
}

CSize CPiecesBar::CalcMaxSize()
{
	// the control bar cannot grow with more than the size of 
	// remaining client area of the frame
	CRect rect;
	m_pDockSite->GetClientRect(&rect);
	CSize size = rect.Size();
	CWnd* pBar;
	if (IsHorzDocked())
	{
		if (pBar = m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_TOP))
		{
			pBar->GetWindowRect(&rect);
			size -= rect.Size();
		}
		if (pBar = m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_BOTTOM))
		{
			pBar->GetWindowRect(&rect);
			size -= rect.Size();
		}
		if (pBar = m_pDockSite->GetControlBar(AFX_IDW_STATUS_BAR))
		{
			pBar->GetWindowRect(&rect);
			size -= rect.Size();
		}
	}
	else
	{
		if (pBar = m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_LEFT))
		{
			pBar->GetWindowRect(&rect);
			size -= rect.Size();
		}
		if (pBar = m_pDockSite->GetControlBar(AFX_IDW_DOCKBAR_RIGHT))
		{
			pBar->GetWindowRect(&rect);
			size -= rect.Size();
		}
	}
	
	size -= CSize(4, 4);
    size += IsHorzDocked() ? m_sizeHorz : m_sizeVert;

	return size;
}

void CPiecesBar::OnTrackInvertTracker()
{
    ASSERT_VALID(this);
    ASSERT(m_bTracking);

	CRect rect = m_rectBorder;
    CRect rectBar, rectFrame;
    GetWindowRect(rectBar);
    rect.OffsetRect(rectBar.TopLeft());
    m_pDockSite->GetWindowRect(rectFrame);
    rect.OffsetRect(-rectFrame.left, -rectFrame.top);

	switch (m_nDockBarID)
	{
	case AFX_IDW_DOCKBAR_TOP:
		rect.OffsetRect(0, m_sizeHorz.cy - m_sizeOld.cy); break;
	case AFX_IDW_DOCKBAR_BOTTOM:
		rect.OffsetRect(0, m_sizeOld.cy - m_sizeHorz.cy); break;
	case AFX_IDW_DOCKBAR_LEFT:
		rect.OffsetRect(m_sizeVert.cx - m_sizeOld.cx, 0); break;
	case AFX_IDW_DOCKBAR_RIGHT:
		rect.OffsetRect(m_sizeOld.cx - m_sizeVert.cx, 0); break;
	}
	if (IsVertDocked())
	    rect.bottom -= 4;
    rect.DeflateRect(1, 1);

    CDC *pDC = m_pDockSite->GetDCEx(NULL,
        DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);

    CBrush* pBrush = CDC::GetHalftoneBrush();
    HBRUSH hOldBrush = NULL;
    if (pBrush != NULL)
        hOldBrush = (HBRUSH)::SelectObject(pDC->m_hDC, pBrush->m_hObject);

    pDC->PatBlt(rect.left, rect.top,
		rect.Width(), rect.Height(), PATINVERT);

    if (hOldBrush != NULL)
        ::SelectObject(pDC->m_hDC, hOldBrush);

    m_pDockSite->ReleaseDC(pDC);
}

BOOL CPiecesBar::QueryDragFullWindows() const
{
	TCHAR sDragfullWindows[2];
	DWORD cbDragfullWindows = sizeof(DWORD);
	DWORD dwType;
	HKEY hKey;
	BOOL bRet = FALSE;

	RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\desktop"),
		0, KEY_QUERY_VALUE, &hKey);

	if (!FAILED(RegQueryValueEx(hKey, _T("DragfullWindows"),
		NULL, &dwType, (LPBYTE)&sDragfullWindows, &cbDragfullWindows)))
		if (!_tcscmp(sDragfullWindows, _T("1")))
			bRet = TRUE;

	RegCloseKey(hKey);

	return bRet;
}

void CPiecesBar::OnSize(UINT nType, int cx, int cy) 
{
	CControlBar::OnSize(nType, cx, cy);

	if (!IsWindow(m_wndColorsList.m_hWnd))
		return;

	int off = 31;
	m_wndColorsList.SetWindowPos (NULL, (cx-210)/2, cy-off, 212, 26, SWP_NOZORDER);

	if (m_bCombo)
	{
		off += 30;
		m_wndPiecesCombo.SetWindowPos (NULL, 5, cy-off, cx-10, 140, SWP_NOZORDER);
	}

	if (m_bGroups)
	{
		off += 28;
		m_wndGroupsBar.SetWindowPos (NULL, 5, cy-off, cx-10, 24, SWP_NOZORDER);
		m_wndGroupsBar.ShowWindow (SW_SHOW);
	}
	else
		m_wndGroupsBar.ShowWindow (SW_HIDE);

	if (m_bPreview)
	{
		m_wndSplitter.SetWindowPos (NULL, 5, m_nPreviewHeight+6, cx-10, 4, SWP_NOZORDER);
		m_wndPiecesList.SetWindowPos (NULL, 5, m_nPreviewHeight+10, cx-10, cy-off-15-m_nPreviewHeight, SWP_NOZORDER);
		m_wndPiecePreview.SetWindowPos (NULL, 5, 5, cx-10, m_nPreviewHeight, 0);
		m_wndPiecePreview.EnableWindow (TRUE);
		m_wndPiecePreview.ShowWindow (SW_SHOW);
		m_wndSplitter.ShowWindow (SW_SHOW);
	}
	else
	{
		m_wndSplitter.ShowWindow (SW_HIDE);
		m_wndPiecesList.SetWindowPos (NULL, 5, 5, cx-10, cy-off-10, SWP_NOZORDER);
		m_wndPiecePreview.ShowWindow (SW_HIDE);
		m_wndPiecePreview.EnableWindow (FALSE);
	}

	RECT rect;
	m_wndPiecesList.GetWindowRect (&rect);

	if (m_bNumbers)
	{
		LV_COLUMN lvc;
		lvc.mask = LVCF_WIDTH;
		if (!m_wndPiecesList.GetColumn(1,&lvc))
		{
			m_wndPiecesList.InsertColumn(1, "Number", LVCFMT_LEFT, 60, 1);
			rect.right -= 60;
		}
		else
			rect.right -= lvc.cx;
	}
	else
		m_wndPiecesList.DeleteColumn (1);

	m_wndPiecesList.SetColumnWidth (0, rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL) -2);
	m_wndPiecesCombo.ShowWindow ((m_bCombo) ? SW_SHOW : SW_HIDE);
}

void CPiecesBar::OnUpdateCmdUI(CFrameWnd * pTarget, BOOL bDisableIfNoHndler)
{
//	CWnd::OnUpdateCmdUI(pTarget, FALSE);
//	int nID = ID_PIECE_GROUP02;

//	int sta = m_wndGroupsBar.GetToolBarCtrl().GetState(nID) & ~TBSTATE_ENABLED;
//	if (bEnable)
//		sta |= TBSTATE_ENABLED|TBSTATE_CHECKED;
//	m_wndGroupsBar.GetToolBarCtrl().SetState(nID, sta);

	m_wndGroupsBar.OnUpdateCmdUI(pTarget, FALSE);
	UpdateDialogControls(pTarget, FALSE);
}

int CPiecesBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CControlBar::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_wndPiecesList.Create (LVS_SINGLESEL|LVS_SHOWSELALWAYS|LVS_AUTOARRANGE|
		LVS_SORTASCENDING|WS_VISIBLE|WS_TABSTOP|LVS_REPORT|WS_BORDER,
		CRect(0,0,0,0), this, IDW_PIECESLIST);

	m_wndPiecesList.InsertColumn(0, "Description", LVCFMT_LEFT, 129, 0);
	m_wndPiecesList.SubclassHeader();
	if (m_bNumbers)
		m_wndPiecesList.InsertColumn(1, "Number", LVCFMT_LEFT, 60, 1);

	m_wndColorsList.Create (LBS_MULTICOLUMN|LBS_NOINTEGRALHEIGHT|LBS_NOTIFY|
		LBS_OWNERDRAWFIXED|WS_VISIBLE|WS_TABSTOP|WS_CHILD|WS_BORDER,
		CRect (0,0,0,0), this, IDW_COLORSLIST);

	for (int i = 0; i < LC_MAXCOLORS; i++)
		m_wndColorsList.AddString("");

	m_wndPiecesCombo.Create (CBS_DROPDOWN|CBS_SORT|CBS_HASSTRINGS|WS_VISIBLE|WS_CHILD|
		WS_VSCROLL|WS_TABSTOP, CRect (0,0,0,0), this, IDW_PIECESCOMBO);

	//  Create a font for the combobox
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(logFont));

	if (!::GetSystemMetrics(SM_DBCSENABLED))
	{
		logFont.lfHeight = -10;
		logFont.lfWeight = 0;
		logFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
		lstrcpy(logFont.lfFaceName, "MS Sans Serif");
		if (m_Font.CreateFontIndirect(&logFont))
			m_wndPiecesCombo.SetFont(&m_Font);
	}
	else
	{
		m_Font.Attach(::GetStockObject(SYSTEM_FONT));
		m_wndPiecesCombo.SetFont(&m_Font);
	}

	m_wndPiecePreview.Create (NULL, NULL, WS_BORDER|WS_CHILD|WS_VISIBLE,
		CRect(0, 0, 0, 0), this, IDW_PIECEPREVIEW);

	CreateGroupsBar();

	CreateWindow("STATIC", "", WS_VISIBLE|WS_CHILD|SS_ETCHEDFRAME, 
		0,0,0,0, m_hWnd, (HMENU)IDW_PIECEBAR_SPLITTER, AfxGetInstanceHandle(), NULL); 

	// y-splitter
	m_wndSplitter.BindWithControl(this, IDW_PIECEBAR_SPLITTER);
	m_wndSplitter.SetMinHeight(40, 40);
	m_wndSplitter.AttachAsAbovePane(IDW_PIECEPREVIEW);
	m_wndSplitter.AttachAsBelowPane(IDW_PIECESLIST);
	m_wndSplitter.RecalcLayout();

	return 0;
}

void CPiecesBar::OnSelChangeColor()
{
	int i = m_wndColorsList.GetCurSel();
	if (i == LB_ERR)
		return;

	project->HandleNotify(LC_COLOR_CHANGED, (i % 2 == 0) ? (i/2) : (((i-1)/2)+14));
	m_wndPiecePreview.PostMessage (WM_PAINT);
}

BOOL CPiecesBar::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

	if (GetRoutingFrame() != NULL)
		return FALSE;
	
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	UINT nID = pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		nID = ((UINT)(WORD)::GetDlgCtrlID((HWND)nID));
	}
	if (nID >= ID_PIECE_GROUP01 && nID <= ID_PIECE_GROUP32) // will be zero on a separator
	{
		if (pNMHDR->code == TTN_NEEDTEXTA)
			lstrcpyn(pTTTA->szText, m_GroupNames[nID-ID_PIECE_GROUP01],(sizeof(pTTTA->szText)/sizeof(pTTTA->szText[0])));
		else
			_mbstowcsz(pTTTW->szText, m_GroupNames[nID-ID_PIECE_GROUP01],(sizeof(pTTTW->szText)/sizeof(pTTTW->szText[0])));
	}
/*	else if (nID == IDW_COLORSLIST)
	{
		POINT pt;
		GetCursorPos(&pt);
		m_wndColorsList.ScreenToClient(&pt);
		CString str;

		for (int i = 0; i < 14; i++)
		{
			CRect rect (i*15,0,(i+1)*15,12);
			if (rect.PtInRect(pt))
			{
				str.LoadString(IDS_COLOR01+i);
				break;
			}
			rect.OffsetRect (0,12);
			if (rect.PtInRect(pt))
			{
				str.LoadString(IDS_COLOR15+i);
				break;
			}
		}
		if (pNMHDR->code == TTN_NEEDTEXTA)
			lstrcpyn(pTTTA->szText, str,(sizeof(pTTTA->szText)/sizeof(pTTTA->szText[0])));
		else
			_mbstowcsz(pTTTW->szText, str,(sizeof(pTTTW->szText)/sizeof(pTTTW->szText[0])));
	}
*/
	*pResult = 0;
	
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);

	return TRUE; // message was handled
}

LONG CPiecesBar::OnSplitterMoved(UINT lParam, LONG wParam)
{
	UNREFERENCED_PARAMETER(wParam);

	if (lParam == 0)
		m_bNoContext = TRUE;
	else
		m_nPreviewHeight += lParam;

	return TRUE;
}

void CPiecesBar::OnPieceGroupClicked(UINT nID)
{
	m_nCurGroup = nID - ID_PIECE_GROUP01;
	project->HandleNotify(LC_GROUP_CHANGED, m_nCurGroup);
	AfxGetMainWnd()->PostMessage (WM_LC_UPDATE_LIST, 1, 0);
}

void CPiecesBar::CreateGroupsBar()
{
	BYTE nGroups = 0;
	memset (m_GroupNames, 0, sizeof(m_GroupNames));
	CString str, cfg = AfxGetApp()->GetProfileString("Settings", "Groups", "");

	for (int i = 0; i < 32; i++)
		m_wndGroupsBar.m_ToolbarData.ButtonData[i].iBitmap = min (i,9);
	m_wndGroupsBar.m_ToolbarData.himl = NULL;

	if (!cfg.IsEmpty())
	{
		CFile f;
		if (f.Open(cfg, CFile::modeRead|CFile::shareDenyWrite) != 0)
		if (f.GetLength() != 0)
		{
			CArchive ar(&f, CArchive::load | CArchive::bNoFlushOnDelete);
			char tmp[33];
			float ver;
			CString str;
			ar.Read (tmp, 32);
			ar >> ver;
			if (ver == 0.1f)
				ar >> i;
			else
			{
				ar >> nGroups;
				for (i = 0; i < nGroups; i++)
				{
					ar.Read (m_GroupNames[i], 33);

					if (ver > 0.2f)
						ar >> m_wndGroupsBar.m_ToolbarData.ButtonData[i].iBitmap;
				}
			}

			if (ver > 0.2f)
			{
				CImageList iml;
				iml.Read(&ar);
				m_wndGroupsBar.m_ToolbarData.himl = iml.Detach();
			}

			ar.Close();
			f.Close();
		}
	}

	if (nGroups == 0)
		nGroups = 9;

	for (i = 0; i < nGroups; i++)
	{
		m_wndGroupsBar.m_ToolbarData.ButtonData[i].idCommand = ID_PIECE_GROUP01 + i;
		m_wndGroupsBar.m_ToolbarData.ButtonData[i].fsState = TBSTATE_ENABLED;
		m_wndGroupsBar.m_ToolbarData.ButtonData[i].fsStyle = TBSTYLE_BUTTON;
		m_wndGroupsBar.m_ToolbarData.ButtonData[i].lpszButtonText = "";
		m_wndGroupsBar.m_ToolbarData.ButtonData[i].lpszTooltip = "";

		if (m_GroupNames[i][0] == 0)
		{
			str.LoadString (ID_PIECE_GROUP01 + i);
			strcpy (m_GroupNames[i], str);
		}
	}
	
	m_wndGroupsBar.m_ToolbarData.dwStyle = TBSTYLE_FLAT;
	m_wndGroupsBar.m_ToolbarData.idControl = IDW_GROUPSBAR;
	m_wndGroupsBar.m_ToolbarData.idbDefault = IDB_PIECEBAR;
	m_wndGroupsBar.m_ToolbarData.idbHot = NULL;
	m_wndGroupsBar.m_ToolbarData.iButtons = nGroups;
	m_wndGroupsBar.m_ToolbarData.iButtonCX = 16;
	m_wndGroupsBar.m_ToolbarData.iButtonCY = 15;

	if (m_wndGroupsBar.m_ToolbarData.himl == NULL)
	m_wndGroupsBar.m_ToolbarData.himl = ImageList_LoadImage
		(AfxGetInstanceHandle(), MAKEINTRESOURCE(m_wndGroupsBar.m_ToolbarData.idbDefault), m_wndGroupsBar.m_ToolbarData.iButtonCX, 0, 0x00ff00ff, IMAGE_BITMAP, 0);

	if (m_wndGroupsBar.m_hWnd)
		m_wndGroupsBar.DestroyWindow();
	m_wndGroupsBar.Create(m_hWnd);
	m_wndGroupsBar.SetBarStyle(m_wndGroupsBar.GetBarStyle() | CBRS_FLYBY | CBRS_ALIGN_ANY);
	m_wndGroupsBar.SetWindowPos (NULL,5,210,216,24,SWP_NOZORDER);
	m_wndGroupsBar.EnableToolTips();
}

void CPiecesBar::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	UNREFERENCED_PARAMETER(pWnd);

	if (m_bNoContext)
		m_bNoContext = FALSE;
	else
	{
		CMenu menuPopups;
		menuPopups.LoadMenu(IDR_POPUPS);
		CMenu* pMenu = menuPopups.GetSubMenu(0);
		if (pMenu)
			pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
	}
}
