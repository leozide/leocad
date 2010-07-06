// TerrCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "TerrCtrl.h"
#include "IPEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDC_INPLACE_EDIT  8  // ID of inplace edit control
#define GRIDSIZE 28

#define ControlPoint(row, col) m_pControl[row-1][(col-1)*3+2]

#define GetFixedRowHeight() GRIDSIZE
#define GetFixedColumnWidth() GRIDSIZE
#define m_nFixedRows 1
#define m_nFixedCols 1
#define GetRowHeight(a) GRIDSIZE
#define GetColumnWidth(a) GRIDSIZE
#define GetVirtualWidth() m_nCols*GRIDSIZE
#define GetVirtualHeight() m_nRows*GRIDSIZE
#define m_nMargin 1
#define GetFixedRowCount() 1
#define GetFixedColumnCount() 1
#define GetColumnCount() m_nCols
#define GetRowCount() m_nRows

/////////////////////////////////////////////////////////////////////////////
// CTerrainCtrl

CTerrainCtrl::CTerrainCtrl()
{
	RegisterWindowClass();

	m_nRows = 0;
	m_nCols = 0;
	m_nVScrollMax = 0; // Scroll position
	m_nHScrollMax = 0;
	m_MouseMode = MOUSE_NOTHING;
	m_pControl = NULL;
/*
	m_nMargin			 = 0;		  // cell padding
	m_nRowsPerWheelNotch = GetMouseScrollLines(); // Get the number of lines
												  // per mouse wheel notch to scroll

	m_bHandleTabKey 	 = TRUE;
	m_bTitleTips		 = TRUE;	  // show cell title tips

	m_nTimerID			 = 0;		  // For drag-selection
	m_nTimerInterval	 = 25;		  // (in milliseconds)

	m_crShadow			 = ::GetSysColor(COLOR_3DSHADOW);
	m_crGridColour		 = RGB(0,0,0);
	SetTextColor(m_crWindowText);
	SetBkColor(m_crShadow);

	SetTextBkColor(RGB(0xFF, 0xFF, 0xE0));
*/
	// Set the colours
	m_crFixedBkColour = GetSysColor(COLOR_3DFACE);
	m_crFixedTextColour = GetSysColor(COLOR_WINDOWTEXT);
	m_crTextBkColour = GetSysColor(COLOR_WINDOW);

	// Initially use the system message font for the GridCtrl font
	NONCLIENTMETRICS ncm;
	LOGFONT lf;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0));
	memcpy(&lf, &(ncm.lfMessageFont), sizeof(LOGFONT));
//	lf.lfWeight = SELECTED_CELL_FONT_WEIGHT;
	m_Font.CreateFontIndirect(&lf);

	// Set up the initial grid size
	SetRowCount(5);
	SetColumnCount(5);

/*
	// set initial selection range (ie. none)
	m_SelectedCellMap.RemoveAll();
	m_PrevSelectedCellMap.RemoveAll();
*/
}

CTerrainCtrl::~CTerrainCtrl()
{
/*
	DeleteAllItems();
*/
	DestroyWindow();
	m_Font.DeleteObject();
}

// Register the window class if it has not already been registered.
BOOL CTerrainCtrl::RegisterWindowClass()
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetResourceHandle();

	if (!(::GetClassInfo(hInst, TERRAINCTRL_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style			= CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc		= ::DefWindowProc;
		wndcls.cbClsExtra		= wndcls.cbWndExtra = 0;
		wndcls.hInstance		= hInst;
		wndcls.hIcon			= NULL;
		wndcls.hCursor			= LoadCursor(NULL,IDC_ARROW);
		wndcls.hbrBackground	= (HBRUSH) (COLOR_3DFACE + 1);
		wndcls.lpszMenuName 	= NULL;
		wndcls.lpszClassName	= TERRAINCTRL_CLASSNAME;

		if (!AfxRegisterClass(&wndcls)) {
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

BEGIN_MESSAGE_MAP(CTerrainCtrl, CWnd)
	//{{AFX_MSG_MAP(CTerrainCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_GETDLGCODE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_GETFONT, OnGetFont)
	ON_MESSAGE(WM_LC_EDIT_CLOSED, OnEditClosed)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTerrainCtrl message handlers

BOOL CTerrainCtrl::Create(const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwStyle)
{
	ASSERT(pParentWnd->GetSafeHwnd());

	if (!CWnd::Create(TERRAINCTRL_CLASSNAME, NULL, dwStyle, rect, pParentWnd, nID)) 
		return FALSE;

	// Create titletips
#ifdef GRIDCONTROL_USE_TITLETIPS
	if (m_bTitleTips)		 
		m_TitleTip.Create(this);
#endif

	ResetScrollBars();
	return TRUE;
}

void CTerrainCtrl::PreSubclassWindow() 
{
	CWnd::PreSubclassWindow();
/*
	HFONT hFont = ::CreateFontIndirect(&m_Logfont);
	OnSetFont((LPARAM)hFont, 0);
	DeleteObject(hFont);
*/
	ResetScrollBars();	 
}

BOOL CTerrainCtrl::SubclassWindow(HWND hWnd)
{
	if (!CWnd::SubclassWindow(hWnd))
		return FALSE;

#ifdef GRIDCONTROL_USE_TITLETIPS
	if (m_bTitleTips && !IsWindow(m_TitleTip.m_hWnd))
		m_TitleTip.Create(this);
#endif

	return TRUE;
}

LRESULT CTerrainCtrl::OnGetFont(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return (LRESULT) (HFONT) m_Font;
}

void CTerrainCtrl::OnPaint() 
{
	CPaintDC dc(this);		// device context for painting

	CDC MemDC;
	CRect rect;
    CBitmap bitmap, *pOldBitmap;
	dc.GetClipBox(&rect);
	MemDC.CreateCompatibleDC(&dc);
	bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
	pOldBitmap = MemDC.SelectObject(&bitmap);
	MemDC.SetWindowOrg(rect.left, rect.top);
	OnDraw(&MemDC);
	dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &MemDC, rect.left, rect.top, SRCCOPY);
	MemDC.SelectObject(pOldBitmap);
}

BOOL CTerrainCtrl::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;	// Don't erase the background.
}

// Custom background erasure. This gets called from within the OnDraw function,
// since we will (most likely) be using a memory DC to stop flicker. If we just
// erase the background normally through OnEraseBkgnd, and didn't fill the memDC's
// selected bitmap with colour, then all sorts of vis problems would occur
void CTerrainCtrl::EraseBkgnd(CDC* pDC)
{

	CRect  VisRect, ClipRect, rect;
	CBrush FixedBack(m_crFixedBkColour), TextBack(m_crTextBkColour);

	if (pDC->GetClipBox(ClipRect) == ERROR)
		return;

	int nFixedColumnWidth = GetFixedColumnWidth();
	int nFixedRowHeight = GetFixedRowHeight();
	GetClientRect(VisRect);
	VisRect.top = nFixedRowHeight;
	VisRect.left = nFixedColumnWidth;

	// Draw Fixed columns background
	if (ClipRect.left < nFixedColumnWidth && ClipRect.top < VisRect.bottom)
		pDC->FillRect(CRect(ClipRect.left, ClipRect.top, nFixedColumnWidth, VisRect.bottom),
					  &FixedBack);
		
	// Draw Fixed rows background
	if (ClipRect.top < nFixedRowHeight && 
		ClipRect.right > nFixedColumnWidth && ClipRect.left < VisRect.right)
		pDC->FillRect(CRect(nFixedColumnWidth-1, ClipRect.top, VisRect.right, nFixedRowHeight),
					  &FixedBack);

	// Draw non-fixed cell background
	if (rect.IntersectRect(VisRect, ClipRect)) 
	{
		CRect CellRect(max(nFixedColumnWidth, rect.left), max(nFixedRowHeight, rect.top),
					   rect.right, rect.bottom);
		pDC->FillRect(CellRect, &TextBack);
	}
}

void CTerrainCtrl::OnSize(UINT nType, int cx, int cy) 
{
  if (::IsWindow(GetSafeHwnd()) && GetFocus()->GetSafeHwnd() != GetSafeHwnd()) 
		SetFocus(); // Auto-destroy any InPlaceEdit's

	CWnd::OnSize(nType, cx, cy);
	ResetScrollBars();	  
}

UINT CTerrainCtrl::OnGetDlgCode() 
{
	UINT nCode = DLGC_WANTARROWS | DLGC_WANTCHARS;
/*
	if (m_bHandleTabKey && !IsCTRLpressed()) 
		nCode |= DLGC_WANTTAB;
*/
	return nCode;
}

// wParam = key pressed, lParam = modified
LRESULT CTerrainCtrl::OnEditClosed(WPARAM wParam, LPARAM lParam)
{
	// In case OnEndInPlaceEdit called as window is being destroyed
	if (!IsWindow(GetSafeHwnd()))
		return TRUE;

	// Only set as modified if it actually was, and ESC was not hit.
	if ((wParam != VK_ESCAPE) && (lParam == TRUE))
	{
	CWnd* pParent = GetOwner();
	if (pParent)
		pParent->SendMessage(WM_LC_EDIT_CLOSED);
//		SetModified(TRUE);
	}

	switch (wParam)
	{
	case VK_DOWN:
	case VK_UP:
	case VK_RIGHT:
	case VK_LEFT:
	case VK_NEXT:
	case VK_PRIOR:
	case VK_HOME:
	case VK_END:
		OnKeyDown(wParam, 0, 0);
		EditCell(m_idCurrentCell.row, m_idCurrentCell.col, wParam);
    }

	return TRUE;
}

// Handle horz scrollbar notifications
void CTerrainCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if (GetFocus()->GetSafeHwnd() != GetSafeHwnd()) 
		SetFocus();  // Auto-destroy any InPlaceEdit's

#ifdef GRIDCONTROL_USE_TITLETIPS
	m_TitleTip.Hide();	// hide any titletips
#endif

	int scrollPos = GetScrollPos32(SB_HORZ);

	CELLID idTopLeft = GetTopleftNonFixedCell();

	CRect rect;
	GetClientRect(rect);

	switch (nSBCode)
	{
		case SB_LINERIGHT:
		if (scrollPos < m_nHScrollMax)
		{
			int xScroll = GetColumnWidth(nTopLeftCol);
			SetScrollPos32(SB_HORZ, scrollPos + xScroll);
			if (GetScrollPos32(SB_HORZ) == scrollPos)
				break; // didn't work

			rect.left = GetFixedColumnWidth() + xScroll;
			ScrollWindow(-xScroll, 0, rect);
			rect.left = rect.right - xScroll;
			InvalidateRect(rect);
		} break;

		case SB_LINELEFT:
		if (scrollPos > 0 && idTopLeft.col > GetFixedColumnCount())
		{
			int xScroll = GetColumnWidth(nTopLeftCol-1);
			SetScrollPos32(SB_HORZ, max(0,scrollPos - xScroll));
			rect.left = GetFixedColumnWidth();
			ScrollWindow(xScroll, 0, rect);
			rect.right = rect.left + xScroll;
			InvalidateRect(rect);
		} break;

		case SB_PAGERIGHT:
		if (scrollPos < m_nHScrollMax)
		{
			rect.left = GetFixedColumnWidth();
			int offset = rect.Width();
			int pos = min(m_nHScrollMax, scrollPos + offset);
			SetScrollPos32(SB_HORZ, pos);
			rect.left = GetFixedColumnWidth();
			InvalidateRect(rect);
		} break;

		case SB_PAGELEFT:
		if (scrollPos > 0)
		{
			rect.left = GetFixedColumnWidth();
			int offset = -rect.Width();
			int pos = max(0, scrollPos + offset);
			SetScrollPos32(SB_HORZ, pos);
			rect.left = GetFixedColumnWidth();
			InvalidateRect(rect);
		} break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
		{
			SetScrollPos32(SB_HORZ, GetScrollPos32(SB_HORZ, TRUE));
			rect.left = GetFixedColumnWidth();
			InvalidateRect(rect);
		} break;

		case SB_LEFT:
		if (scrollPos > 0)
		{
			SetScrollPos32(SB_HORZ, 0);
			Invalidate();
		} break;

		case SB_RIGHT:
		if (scrollPos < m_nHScrollMax)
		{
			SetScrollPos32(SB_HORZ, m_nHScrollMax);
			Invalidate();
		} break;
	}
}

// Handle vert scrollbar notifications
void CTerrainCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if (GetFocus()->GetSafeHwnd() != GetSafeHwnd()) 
		SetFocus(); 	   // Auto-destroy any InPlaceEdit's

#ifdef GRIDCONTROL_USE_TITLETIPS
	m_TitleTip.Hide();	// hide any titletips
#endif

	// Get the scroll position ourselves to ensure we get a 32 bit value
	int scrollPos = GetScrollPos32(SB_VERT);

	CELLID idTopLeft = GetTopleftNonFixedCell();

	CRect rect;
	GetClientRect(rect);

	switch (nSBCode)
	{
		case SB_LINEDOWN:
		if (scrollPos < m_nVScrollMax)
		{
			int yScroll = GetRowHeight(nTopLeftRow);
			SetScrollPos32(SB_VERT, scrollPos + yScroll);
			if (GetScrollPos32(SB_VERT) == scrollPos)
				break; // didn't work

			rect.top = GetFixedRowHeight() + yScroll;
			ScrollWindow( 0, -yScroll, rect);
			rect.top = rect.bottom - yScroll;
			InvalidateRect(rect);
		} break;

		case SB_LINEUP:
		if (scrollPos > 0 && idTopLeft.row > GetFixedRowCount())
		{
			int yScroll = GetRowHeight(nTopLeftRow-1);
			SetScrollPos32(SB_VERT, max(0, scrollPos - yScroll));
			rect.top = GetFixedRowHeight();
			ScrollWindow(0, yScroll, rect);
			rect.bottom = rect.top + yScroll;
			InvalidateRect(rect);
		} break;

		case SB_PAGEDOWN:
		if (scrollPos < m_nVScrollMax)
		{
			rect.top = GetFixedRowHeight();
			scrollPos = min(m_nVScrollMax, scrollPos + rect.Height());
			SetScrollPos32(SB_VERT, scrollPos);
			rect.top = GetFixedRowHeight();
			InvalidateRect(rect);
		} break;

		case SB_PAGEUP:
		if (scrollPos > 0)
		{
			rect.top = GetFixedRowHeight();
			int offset = -rect.Height();
			int pos = max(0, scrollPos + offset);
			SetScrollPos32(SB_VERT, pos);
			rect.top = GetFixedRowHeight();
			InvalidateRect(rect);
		} break;

		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
		{
			SetScrollPos32(SB_VERT, GetScrollPos32(SB_VERT, TRUE));
			rect.top = GetFixedRowHeight();
			InvalidateRect(rect);
		} break;

		case SB_TOP:
		if (scrollPos > 0)
		{
			SetScrollPos32(SB_VERT, 0);
			Invalidate();
		} break;

		case SB_BOTTOM:
		if (scrollPos < m_nVScrollMax)
		{
			SetScrollPos32(SB_VERT, m_nVScrollMax);
			Invalidate();
		}
	}
}

void CTerrainCtrl::OnDraw(CDC* pDC)
{
	int row,col;
	CRect clientRect;
	GetClientRect(clientRect);

	CRect clipRect;
	if (pDC->GetClipBox(&clipRect) == ERROR)
		return;

	// OnEraseBkgnd does nothing, so erase bkgnd here.
	// This necessary since we may be using a Memory DC.
	EraseBkgnd(pDC);

	int nFixedRowHeight = GetFixedRowHeight();
	int nFixedColWidth	= GetFixedColumnWidth();

	CELLID idTopLeft = GetTopleftNonFixedCell();
/*
	CRect VisRect;
	CCellRange VisCellRange = GetVisibleNonFixedCellRange(VisRect);
	int maxVisibleRow = VisCellRange.GetMaxRow(),
		maxVisibleCol = VisCellRange.GetMaxCol();
*/
	// calc bottom
	int i, bottom = GetFixedRowHeight();
	for (i = idTopLeft.row; i < m_nRows; i++)
	{
		bottom += GetRowHeight(i);
		if (bottom >= clientRect.bottom)
		{
			bottom = clientRect.bottom;
			break;
		}
	}								 
	int maxVisibleRow = min(i, m_nRows - 1);
	
	// calc right
	int right = GetFixedColumnWidth();
	for (i = idTopLeft.col; i < m_nCols; i++)
	{
		right += GetColumnWidth(i);
		if (right >= clientRect.right)
		{
			right = clientRect.right;
			break;
		}
	}
	int maxVisibleCol = min(i, m_nCols - 1);

	// draw top-left cell
	CRect rect;
	rect.top = 0;
	rect.bottom = GetRowHeight(0)-1;
	rect.left = 0;
	rect.right = GetColumnWidth(0)-1;
	DrawFixedCell(pDC, 0, 0, rect);
	 
	// draw fixed column cells
	rect.bottom = nFixedRowHeight-1;
	for (row = idTopLeft.row; row <= maxVisibleRow; row++)
	{
		rect.top = rect.bottom+1;
		rect.bottom = rect.top + GetRowHeight(row)-1;

		// rect.bottom = bottom pixel of previous row
		if (rect.top > clipRect.bottom) break;	  // Gone past cliprect
		if (rect.bottom < clipRect.top) continue; // Reached cliprect yet?

		rect.left = 0;
		rect.right = GetColumnWidth(0)-1;

		if (rect.left > clipRect.right) break;	  // gone past cliprect
		if (rect.right < clipRect.left) continue; // Reached cliprect yet?

		DrawFixedCell(pDC, row, 0, rect);
	}

	// draw end of column
	if (rect.bottom < clientRect.bottom)
	{
		rect.top = rect.bottom+1;
		rect.bottom = clientRect.bottom-1;

		if (rect.top < clipRect.bottom)
		if (rect.bottom > clipRect.top)
		if (rect.left < clipRect.right)
		if (rect.right > clipRect.left)
			DrawFixedCell(pDC, 0, 0, rect);
	}
	
	// draw fixed row cells  0..m_nFixedRows, m_nFixedCols..n
	rect.top = 0;
	rect.bottom = GetRowHeight(0)-1;

	// rect.bottom = bottom pixel of previous row
	if (rect.top < clipRect.bottom) // Gone past cliprect
	if (rect.bottom > clipRect.top) // Reached cliprect yet?
	{
		rect.right = nFixedColWidth-1;
		for (col = idTopLeft.col; col <= maxVisibleCol; col++)
		{
			rect.left = rect.right+1;
			rect.right = rect.left + GetColumnWidth(col)-1;
			
			if (rect.left > clipRect.right) break;	  // gone past cliprect
			if (rect.right < clipRect.left) continue; // Reached cliprect yet?
			
			DrawFixedCell(pDC, 0, col, rect);
		}
	}

	// draw end of row
	if (rect.right < clientRect.right)
	{
		rect.left = rect.right+1;
		rect.right = clientRect.right - 1;

		if (rect.top < clipRect.bottom)
		if (rect.bottom > clipRect.top)
		if (rect.left < clipRect.right)
		if (rect.right > clipRect.left)
			DrawFixedCell(pDC, 0, 0, rect);

	}

	// draw rest of non-fixed cells
	rect.bottom = nFixedRowHeight-1;
	for (row = idTopLeft.row; row <= maxVisibleRow; row++)
	{
		rect.top = rect.bottom+1;
		rect.bottom = rect.top + GetRowHeight(row)-1;

		// rect.bottom = bottom pixel of previous row
		if (rect.top > clipRect.bottom) break;				  // Gone past cliprect
		if (rect.bottom < clipRect.top) continue;			  // Reached cliprect yet?

		rect.right = nFixedColWidth-1;
		for (col = idTopLeft.col; col <= maxVisibleCol; col++)
		{
			rect.left = rect.right+1;
			rect.right = rect.left + GetColumnWidth(col)-1;

			if (rect.left > clipRect.right) break;		  // gone past cliprect
			if (rect.right < clipRect.left) continue;	  // Reached cliprect yet?

			DrawCell(pDC, row, col, rect);
		}
	}
}

// Get/Set scroll position using 32 bit functions
int CTerrainCtrl::GetScrollPos32(int nBar, BOOL bGetTrackPos /* = FALSE */)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);

	if (bGetTrackPos)
	{
		if (GetScrollInfo(nBar, &si, SIF_TRACKPOS))
			return si.nTrackPos;
	}
	else 
	{
		if (GetScrollInfo(nBar, &si, SIF_POS))
			return si.nPos;
	}

	return 0;
}

BOOL CTerrainCtrl::SetScrollPos32(int nBar, int nPos, BOOL bRedraw /* = TRUE */)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask  = SIF_POS;
	si.nPos   = nPos;
	return SetScrollInfo(nBar, &si, bRedraw);
}

void CTerrainCtrl::ResetScrollBars()
{
	if (!::IsWindow(GetSafeHwnd())) 
		return;

	CRect rect;
	GetClientRect(rect);
	rect.left  += GetFixedColumnWidth();
	rect.top   += GetFixedRowHeight();
	if (rect.left >= rect.right || rect.top >= rect.bottom)
		return;

	CRect VisibleRect(GetFixedColumnWidth(), GetFixedRowHeight(), rect.right, rect.bottom);
	CRect VirtualRect(GetFixedColumnWidth(), GetFixedRowHeight(), GetVirtualWidth(), GetVirtualHeight());
/*
	CCellRange visibleCells = GetUnobstructedNonFixedCellRange();
	if (!IsValid(visibleCells)) return;
*/
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask  = SIF_PAGE;
	si.nPage  = VisibleRect.Width();
	SetScrollInfo(SB_HORZ, &si, FALSE); 
	si.nPage  = VisibleRect.Height();
	SetScrollInfo(SB_VERT, &si, FALSE); 

	if (VisibleRect.Height() < VirtualRect.Height())
		m_nVScrollMax = VirtualRect.Height()-1;//-VisibleRect.Height();//+GetRowHeight(0);
	else
		m_nVScrollMax = 0;

	if (VisibleRect.Width() < VirtualRect.Width())
		m_nHScrollMax = VirtualRect.Width()-1;//-VisibleRect.Width();//+GetColumnWidth(0);
	else
		m_nHScrollMax = 0;

	ASSERT(m_nVScrollMax < INT_MAX && m_nHScrollMax < INT_MAX); // This should be fine :)
	SetScrollRange(SB_VERT, 0, m_nVScrollMax, TRUE);
	SetScrollRange(SB_HORZ, 0, m_nHScrollMax, TRUE);
}

BOOL CTerrainCtrl::DrawFixedCell(CDC* pDC, int nRow, int nCol, CRect rect, BOOL bEraseBk)
{
	if (bEraseBk)
	{
		CBrush brush(m_crFixedBkColour);
		pDC->FillRect(rect, &brush);
	}
	pDC->SetTextColor(m_crFixedTextColour);
	
	int nSavedDC = pDC->SaveDC();
/*	
	// Create the appropriate font and select into DC
	LOGFONT lf, *pLF = GetItemFont(nRow, nCol);
	if (pLF)
		memcpy(&lf, pLF, sizeof(LOGFONT));
	else
		memcpy(&lf, &m_Logfont, sizeof(LOGFONT));
		
	CCellID FocusCell = GetFocusCell();
	if (FocusCell.row == nRow || FocusCell.col == nCol)
		lf.lfWeight = SELECTED_CELL_FONT_WEIGHT;
	
	CFont Font;
	Font.CreateFontIndirect(&lf);
	pDC->SelectObject(&Font);

	if (IsValid(FocusCell) &&  (FocusCell.row == nRow || FocusCell.col == nCol))
	{
		rect.right++; rect.bottom++;
		pDC->DrawEdge(rect, EDGE_RAISED, BF_RECT);
		rect.DeflateRect(1,1);
	}
	else
*/	{
		CPen lightpen(PS_SOLID, 1, ::GetSysColor(COLOR_3DHIGHLIGHT)),
			  darkpen(PS_SOLID, 1, ::GetSysColor(COLOR_3DDKSHADOW)),
			 *pOldPen = pDC->GetCurrentPen();
	
		pDC->SelectObject(&lightpen);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.left, rect.top);
		pDC->LineTo(rect.left, rect.bottom);

		pDC->SelectObject(&darkpen);
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.right, rect.bottom);
		pDC->LineTo(rect.left, rect.bottom);

		pDC->SelectObject(pOldPen);
		rect.DeflateRect(1,1);
	}

	pDC->SetBkMode(TRANSPARENT);
	rect.DeflateRect(m_nMargin, 0);

	if ((nRow != 0 || nCol != 0) && nRow < m_nRows && nCol < m_nCols)
	{
		char szText[10];
		sprintf(szText, "%d", nRow == 0 ? nCol : nRow);
		DrawText(pDC->m_hDC, szText, -1, rect, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
	}

	pDC->RestoreDC(nSavedDC);
	return TRUE;
}

BOOL CTerrainCtrl::DrawCell(CDC* pDC, int nRow, int nCol, CRect rect, BOOL bEraseBk)
{
	COLORREF TextClr = m_crFixedTextColour;

	int nSavedDC = pDC->SaveDC();

	pDC->SetBkMode(TRANSPARENT);

	if (m_RowData[nRow][nCol].state & GS_FOCUSED) 
	{
		rect.right++; rect.bottom++;	// FillRect doesn't draw RHS or bottom
		if (bEraseBk) 
		{
			CBrush brush(m_crTextBkColour);
			pDC->FillRect(rect, &brush);
		}
		rect.right--; rect.bottom--;	
		pDC->SelectStockObject(BLACK_PEN);
		pDC->SelectStockObject(NULL_BRUSH);
		pDC->Rectangle(rect);
		pDC->SetTextColor(TextClr);

		rect.DeflateRect(1,1);

	}
	else if (m_RowData[nRow][nCol].state & GS_SELECTED) 
	{
		rect.right++; rect.bottom++;	// FillRect doesn't draw RHS or bottom
		pDC->FillSolidRect(rect, ::GetSysColor(COLOR_HIGHLIGHT));
		rect.right--; rect.bottom--;
		pDC->SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else
	{
		rect.right++; rect.bottom++;	// FillRect doesn't draw RHS or bottom
		if (bEraseBk) 
		{
			CBrush brush(m_crTextBkColour);
			pDC->FillRect(rect, &brush);
		}
		rect.right--; rect.bottom--;
		pDC->SetTextColor(TextClr);
	}
/*
	if (Item.state & GVIS_DROPHILITED)
	{
		pDC->SelectStockObject(BLACK_PEN);
		pDC->SelectStockObject(NULL_BRUSH);
		pDC->Rectangle(rect);
	}
*/

	CPen lightpen(PS_SOLID, 1, RGB(0xE0, 0xE0, 0xE0)),
		 darkpen(PS_SOLID, 1, RGB(0,0,0)),
		 *pOldPen = pDC->GetCurrentPen();

	int left = nCol == 1 ? GRIDSIZE/2-1 : 0;
	int top = nRow == 1 ? GRIDSIZE/2-1 : 0;
	int right = nCol == m_nCols-1 ? GRIDSIZE/2-1 : -1;
	int bottom = nRow == m_nRows-1 ? GRIDSIZE/2-1 : -1;

	if ((nRow-1) % 3)
	{
		pDC->SelectObject(&lightpen);
		pDC->MoveTo(rect.left + left, (rect.top + rect.bottom)/2);
		pDC->LineTo(rect.right - right, (rect.top + rect.bottom)/2);
	}

	if ((nCol-1) % 3)
	{
		pDC->SelectObject(&lightpen);
		pDC->MoveTo((rect.left + rect.right)/2, rect.top + top);
		pDC->LineTo((rect.left + rect.right)/2, rect.bottom - bottom);
	}

	if ((nRow-1) % 3 == 0)
	{
		pDC->SelectObject(&darkpen);
		pDC->MoveTo(rect.left + left, (rect.top + rect.bottom)/2);
		pDC->LineTo(rect.right - right, (rect.top + rect.bottom)/2);
	}

	if ((nCol-1) % 3 == 0)
	{
		pDC->SelectObject(&darkpen);
		pDC->MoveTo((rect.left + rect.right)/2, rect.top + top);
		pDC->LineTo((rect.left + rect.right)/2, rect.bottom - bottom);
	}

	pDC->SelectObject(pOldPen);

/*	// Create the appropriate font and select into DC
	CFont Font;
	LOGFONT *pLF = GetItemFont(nRow, nCol);
	if (pLF)
		Font.CreateFontIndirect(pLF);
	else
		Font.CreateFontIndirect(&m_Logfont);
*/
	CFont *pOldFont = pDC->SelectObject(&m_Font);
	rect.DeflateRect(m_nMargin, 0);
	rect.OffsetRect(GRIDSIZE/2, GRIDSIZE/4+2);

	char szText[10];
	sprintf(szText, "%.0f", ControlPoint(nRow, nCol));

	DrawText(pDC->m_hDC, szText, -1, rect, DT_LEFT|DT_VCENTER|DT_SINGLELINE);

	pDC->SelectObject(pOldFont);

	pDC->RestoreDC(nSavedDC);
	return TRUE;
}

BOOL CTerrainCtrl::SetRowCount(int nRows)
{
	ASSERT(nRows > 0);
	if (nRows == m_nRows)
		return TRUE;

	if (m_idCurrentCell.row >= nRows)
		SetFocusCell(-1,-1);

	int addedRows = nRows - m_nRows;

	// Change the number of rows.
	m_nRows = nRows;
	m_RowData.SetSize(m_nRows);

	// If we have just added rows, we need to construct new elements for each cell
	// and set the default row height
	if (addedRows > 0) 
	{
		// initialize row heights and data
		int startRow = nRows - addedRows;
		for (int row = startRow; row < m_nRows; row++)
		{
			m_RowData[row].SetSize(m_nCols);
			for (int col = 0; col < m_nCols; col++)
				m_RowData[row][col].state = 0;
		}
	}

	if (GetSafeHwnd())
	{
		ResetScrollBars();
		Invalidate();
	}

	return TRUE;
}

BOOL CTerrainCtrl::SetColumnCount(int nCols)
{
	ASSERT(nCols > 0);

	if (nCols == m_nCols)
		return TRUE;

	if (m_idCurrentCell.col >= nCols)
		SetFocusCell(-1,-1);

	int addedCols = nCols - m_nCols;

	// Change the number of columns.
	m_nCols = nCols;

	// Change the number of columns in each row.
	for (int i = 0; i < m_nRows; i++)
		m_RowData[i].SetSize(nCols);

	// If we have just added columns, we need to construct new elements for each cell
	// and set the default column width
	if (addedCols > 0)
	{
		int startCol = nCols - addedCols;

		for (int row = 0; row < m_nRows; row++)
			for (int col = startCol; col < m_nCols; col++)
				m_RowData[row][col].state = 0;
	}

	if (GetSafeHwnd())
	{
		ResetScrollBars();
		Invalidate();
	}
	return TRUE;
}

CELLID CTerrainCtrl::GetTopleftNonFixedCell()
{
	CELLID cell;

	int nVertScroll = GetScrollPos(SB_VERT), nHorzScroll = GetScrollPos(SB_HORZ);

	int nColumn = m_nFixedCols, nRight = 0;
	while (nRight < nHorzScroll && nColumn < (m_nCols-1))
	{
		nColumn++;
		nRight += GetColumnWidth(nColumn);
	}

	int nRow = m_nFixedRows, nTop = 0;
	while (nTop < nVertScroll && nRow < (m_nRows-1))
	{
		nRow++;
		nTop += GetRowHeight(nRow);
	}

	cell.row = nRow;
	cell.col = nColumn;

	return cell;
}

void CTerrainCtrl::SetControlPoints(int uCount, int vCount, float** pControl)
{
	SetRowCount(uCount+1);
	SetColumnCount(vCount+1);
	m_pControl = pControl;
	InvalidateRect(NULL, FALSE);
}

void CTerrainCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
/*
	HWND hOldFocusWnd = ::GetFocus();
*/
	m_LeftClickDownPoint = point;
	m_LeftClickDownCell = GetCellFromPt(point);
	if (!IsValid(m_LeftClickDownCell))
		return;
/*
	m_SelectionStartCell = (nFlags & MK_SHIFT)? m_idCurrentCell : m_LeftClickDownCell;
*/
	SetFocus(); // Auto-destroy any InPlaceEdit's

	// If the user clicks on the current cell, then prepare to edit it.
	// (If the user moves the mouse, then dragging occurs)
	if (m_LeftClickDownCell == m_idCurrentCell)
	{
		m_MouseMode = MOUSE_PREPARE_EDIT;
		return;
	}
	else
	{
		SetFocusCell(-1,-1);
		SetFocusCell(max(m_LeftClickDownCell.row, 1),
					 max(m_LeftClickDownCell.col, 1));
	}
/*
	// If the user clicks on a selected cell, then prepare to drag it.
	// (If the user moves the mouse, then dragging occurs)
	if (m_bAllowDragAndDrop && hOldFocusWnd == GetSafeHwnd() && 
		GetItemState(m_LeftClickDownCell.row, m_LeftClickDownCell.col) & GVNI_SELECTED)
	{
		m_MouseMode = MOUSE_PREPARE_DRAG;
		return;
	}
*/
	SetCapture();

	// If Ctrl pressed, save the current cell selection. This will get added
/*	// to the new cell selection at the end of the cell selection process
	m_PrevSelectedCellMap.RemoveAll();
	if (nFlags & MK_CONTROL)
	{
		for (POSITION pos = m_SelectedCellMap.GetStartPosition(); pos != NULL; )
		{
			DWORD key;
			CCellID cell;
			m_SelectedCellMap.GetNextAssoc(pos, key, (CCellID&)cell);
			m_PrevSelectedCellMap.SetAt(key, cell);
		}
	}
		
	if (m_LeftClickDownCell.row < GetFixedRowCount())
		OnFixedRowClick(m_LeftClickDownCell);
	else if (m_LeftClickDownCell.col < GetFixedColumnCount())
		OnFixedColumnClick(m_LeftClickDownCell);
	else
	{
		m_MouseMode = m_bListMode? MOUSE_SELECT_ROW : MOUSE_SELECT_CELLS;
		OnSelecting(m_LeftClickDownCell);
	}

	m_nTimerID = SetTimer(WM_LBUTTONDOWN, m_nTimerInterval, 0);

	m_LastMousePoint = point;
*/
}

void CTerrainCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CWnd::OnLButtonUp(nFlags, point);
	ClipCursor(NULL);

	if (GetCapture()->GetSafeHwnd() == GetSafeHwnd())
	{
		ReleaseCapture();
/*		KillTimer(m_nTimerID);
		m_nTimerID = 0;
*/	}

	// m_MouseMode == MOUSE_PREPARE_EDIT only if user clicked down on current cell
	// and then didn't move mouse before clicking up (releasing button)
	if (m_MouseMode == MOUSE_PREPARE_EDIT)
	{
		EditCell(m_idCurrentCell.row, m_idCurrentCell.col, VK_LBUTTON);
	}
	// m_MouseMode == MOUSE_PREPARE_DRAG only if user clicked down on a selected cell
/*	// and then didn't move mouse before clicking up (releasing button)
	else if (m_MouseMode == MOUSE_PREPARE_DRAG) 
	{
		ResetSelectedRange();
	}
*/
	m_MouseMode = MOUSE_NOTHING;
	SetCursor(::LoadCursor(NULL, IDC_ARROW));

	if (!IsValid(m_LeftClickDownCell))
		return;
/*
	CWnd *pOwner = GetOwner();
	if (pOwner && IsWindow(pOwner->m_hWnd))
		pOwner->PostMessage(WM_COMMAND, MAKELONG(GetDlgCtrlID(), BN_CLICKED), 
							(LPARAM) GetSafeHwnd());
*/
}

void CTerrainCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	CRect rect;
	GetClientRect(rect);
/*
	// If outside client area, return (unless we are drag n dropping)
	if (m_MouseMode != MOUSE_DRAGGING && !rect.PtInRect(point))
		return;

	// If the left mouse button is up, then test to see if row/column sizing is imminent
	if (!(nFlags & MK_LBUTTON))
	{
		if (point.y < GetFixedRowHeight() && m_bAllowColumnResize)
		{
			CCellID idCurrentCell = GetCellFromPt(point);
			CPoint start;
			if (!GetCellOrigin(idCurrentCell, &start)) return;

			int endx = start.x + GetColumnWidth(idCurrentCell.col);

			if ((point.x - start.x <= m_nResizeCaptureRange && idCurrentCell.col != 0) || 
				endx - point.x <= m_nResizeCaptureRange)
			{
				if (m_MouseMode != MOUSE_OVER_COL_DIVIDE)
					SetCursor(::LoadCursor(NULL, IDC_SIZEWE));
				m_MouseMode = MOUSE_OVER_COL_DIVIDE;
			}
			else 
			{
				if (m_MouseMode != MOUSE_NOTHING)
					SetCursor(::LoadCursor(NULL, IDC_ARROW));
				m_MouseMode = MOUSE_NOTHING;
			}
		}
		else if (point.x < GetFixedColumnWidth() && m_bAllowRowResize)
		{
			CCellID idCurrentCell = GetCellFromPt(point);
			CPoint start;
			if (!GetCellOrigin(idCurrentCell, &start)) return;

			int endy = start.y + GetRowHeight(idCurrentCell.row);

			if ((point.y - start.y <= m_nResizeCaptureRange && idCurrentCell.row != 0) || 
				endy - point.y <= m_nResizeCaptureRange)
			{
				if (m_MouseMode != MOUSE_OVER_ROW_DIVIDE)
					SetCursor(::LoadCursor(NULL, IDC_SIZENS));
				m_MouseMode = MOUSE_OVER_ROW_DIVIDE;
			}
			else
			{
				if (m_MouseMode != MOUSE_NOTHING)
					SetCursor(::LoadCursor(NULL, IDC_ARROW));
				m_MouseMode = MOUSE_NOTHING;
			}
		}
		else
		{
			if (m_MouseMode != MOUSE_NOTHING)
				SetCursor(::LoadCursor(NULL, IDC_ARROW));
			m_MouseMode = MOUSE_NOTHING;
		}

#ifdef GRIDCONTROL_USE_TITLETIPS
		if (m_MouseMode == MOUSE_NOTHING && m_bTitleTips)
		{
			CCellID idCurrentCell = GetCellFromPt(point);
			CRect rect;
			if (GetCellRect(idCurrentCell.row, idCurrentCell.col, rect))
				m_TitleTip.Show( rect, GetItemText(idCurrentCell.row, idCurrentCell.col), 0);
		}
#endif

		m_LastMousePoint = point;
		return;
	}

	if (!IsValid(m_LeftClickDownCell))
	{
		m_LastMousePoint = point;
		return;
	}

	// If the left mouse button is down, the process appropriately
	if (nFlags & MK_LBUTTON) 
	{
		switch(m_MouseMode)
		{
			case MOUSE_SELECT_ALL:
				break;

			case MOUSE_SELECT_COL:
			case MOUSE_SELECT_ROW:	  
			case MOUSE_SELECT_CELLS:
			{
				CCellID idCurrentCell = GetCellFromPt(point);
				if (!IsValid(idCurrentCell))
					return;
				OnSelecting(idCurrentCell);
//				SetFocusCell(max(idCurrentCell.row, m_nFixedRows),
//				max(idCurrentCell.col, m_nFixedCols));
				if (idCurrentCell.row >= m_nFixedRows &&
					idCurrentCell.col >= m_nFixedCols)
					SetFocusCell(idCurrentCell);
				break;
			}

			case MOUSE_PREPARE_DRAG:
				OnBeginDrag();
				break;
		}	 
	}

	m_LastMousePoint = point;
*/
}

// Sets the currently selected cell
void CTerrainCtrl::SetFocusCell(CELLID cell)
{
	SetFocusCell(cell.row, cell.col);
}

void CTerrainCtrl::SetFocusCell(int nRow, int nCol)
{
	CELLID cell(nRow, nCol);

	if (cell == m_idCurrentCell) 
		return;

	CELLID idPrev = m_idCurrentCell;
	m_idCurrentCell = cell;

	if (IsValid(idPrev)) 
	{
/*		SendMessageToParent(idPrev.row, idPrev.col, GVN_SELCHANGING);
*/
		m_RowData[idPrev.row][idPrev.col].state &= ~GS_FOCUSED;

		RedrawCell(idPrev);
/*
		if (idPrev.col != m_idCurrentCell.col)
			for (int row = 0; row < m_nFixedRows; row++)
				RedrawCell(row, idPrev.col);
		if (idPrev.row != m_idCurrentCell.row)
			for (int col = 0; col < m_nFixedCols; col++) 
				RedrawCell(idPrev.row, col);
*/	}

	if (IsValid(m_idCurrentCell))
	{
		m_RowData[nRow][nCol].state |= GS_FOCUSED;

		RedrawCell(m_idCurrentCell);
/*
		if (idPrev.col != m_idCurrentCell.col)
			for (int row = 0; row < m_nFixedRows; row++) 
				RedrawCell(row, m_idCurrentCell.col);
		if (idPrev.row != m_idCurrentCell.row)
			for (int col = 0; col < m_nFixedCols; col++) 
				RedrawCell(m_idCurrentCell.row, col);

		SendMessageToParent(m_idCurrentCell.row, m_idCurrentCell.col, GVN_SELCHANGED); 
*/	}
}

BOOL CTerrainCtrl::IsValid(int nRow, int nCol)
{
	return (nRow >= 0 && nRow < m_nRows && nCol >= 0 && nCol < m_nCols);
}

BOOL CTerrainCtrl::IsValid(CELLID cell)
{
	return IsValid(cell.row, cell.col);
}

// Get cell from point
CELLID CTerrainCtrl::GetCellFromPt(CPoint point, BOOL bAllowFixedCellCheck)
{
	CELLID idTopLeft = GetTopleftNonFixedCell();
	CELLID cellID; // return value

	// calculate column index
	int fixedColWidth = GetFixedColumnWidth();

	if (point.x < 0 || (!bAllowFixedCellCheck && point.x < fixedColWidth)) // not in window
		cellID.col = -1;
	else if (point.x < fixedColWidth) // in fixed col
	{
		int col, xpos = 0;
		for (col = 0; col < m_nFixedCols; col++)
		{
			xpos += GetColumnWidth(col);
			if (xpos > point.x) break;
		}
		cellID.col = col;
	}
	else	// in non-fixed col
	{
		int col, xpos = fixedColWidth;
		for (col = idTopLeft.col; col < m_nCols; col++)
		{
			xpos += GetColumnWidth(col);
			if (xpos > point.x) break;
		}

		if (col >= GetColumnCount())
			cellID.col = -1;
		else
			cellID.col = col;
	}
	
	// calculate row index
	int fixedRowHeight = GetFixedRowHeight();
	if (point.y < 0 || (!bAllowFixedCellCheck && point.y < fixedRowHeight)) // not in window
		cellID.row = -1;
	else if (point.y < fixedRowHeight) // in fixed col
	{
		int row, ypos = 0;
		for (row = 0; row < m_nFixedRows; row++)
		{
			ypos += GetRowHeight(row);
			if (ypos > point.y) break;
		}
		cellID.row = row;
	}
	else
	{
		int row, ypos = fixedRowHeight;
		for (row = idTopLeft.row; row < GetRowCount(); row++)
		{
			ypos += GetRowHeight(row);
			if (ypos > point.y) break;
		}

		if (row >= GetRowCount())
			cellID.row = -1;
		else
			cellID.row = row;
	}

	return cellID;
}

// Forces a redraw of a cell immediately using a direct
// DC construction,  or the supplied DC
BOOL CTerrainCtrl::RedrawCell(CELLID cell, CDC* pDC)
{
	return RedrawCell(cell.row, cell.col, pDC);
}

BOOL CTerrainCtrl::RedrawCell(int nRow, int nCol, CDC* pDC)
{
	BOOL bResult = TRUE;
	BOOL bMustReleaseDC = FALSE;

	if (!IsCellVisible(nRow, nCol))
		return FALSE;

	CRect rect;
	if (!GetCellRect(nRow, nCol, rect))
		return FALSE;

	if (!pDC)
	{
		pDC = GetDC();
		if (pDC)
			bMustReleaseDC = TRUE;
	}

	if (pDC)
	{
		// Redraw cells directly
		if (nRow < m_nFixedRows || nCol < m_nFixedCols)
			bResult = DrawFixedCell(pDC, nRow, nCol, rect, TRUE);
		else
			bResult = DrawCell(pDC, nRow, nCol, rect, TRUE);
/*
		// Since we have erased the background, we will need to redraw the gridlines
		CPen pen;
		try {
			pen.CreatePen(PS_SOLID, 0, m_crGridColour);
		} catch (...) {}

		CPen* pOldPen = (CPen*) pDC->SelectObject(&pen);
		if (m_nGridLines == GVL_BOTH || m_nGridLines == GVL_HORZ) 
		{
			pDC->MoveTo(rect.left,	  rect.bottom);
			pDC->LineTo(rect.right+1, rect.bottom);
		}
		if (m_nGridLines == GVL_BOTH || m_nGridLines == GVL_VERT) 
		{
			pDC->MoveTo(rect.right, rect.top);
			pDC->LineTo(rect.right, rect.bottom+1);    
		}
		pDC->SelectObject(pOldPen);
*/
	} else
		InvalidateRect(rect, TRUE); 	// Could not get a DC - invalidate it anyway
										// and hope that OnPaint manages to get one

	if (bMustReleaseDC) 
		ReleaseDC(pDC);

	return bResult;
}

// returns the top left point of the cell. Returns FALSE if cell not visible.
BOOL CTerrainCtrl::GetCellOrigin(CELLID cell, LPPOINT p)
{
	return GetCellOrigin(cell.row, cell.col, p);
}

BOOL CTerrainCtrl::GetCellOrigin(int nRow, int nCol, LPPOINT p)
{
	int i;

	if (!IsValid(nRow, nCol))
		return FALSE;

	CELLID idTopLeft;
	if (nCol >= m_nFixedCols || nRow >= m_nFixedRows)
		idTopLeft = GetTopleftNonFixedCell();

	if ((nRow >= m_nFixedRows && nRow < idTopLeft.row) ||
		(nCol>= m_nFixedCols && nCol < idTopLeft.col))
		return FALSE;

	p->x = 0;
	if (nCol < m_nFixedCols) // is a fixed column
		for (i = 0; i < nCol; i++)
			p->x += GetColumnWidth(i);
	else // is a scrollable data column
	{
		for (i = 0; i < m_nFixedCols; i++)
			p->x += GetColumnWidth(i);
		for (i = idTopLeft.col; i < nCol; i++)
			p->x += GetColumnWidth(i);
	}

	p->y = 0;
	if (nRow < m_nFixedRows) // is a fixed row
		for (i = 0; i < nRow; i++)
			p->y += GetRowHeight(i);
	else // is a scrollable data row
	{
		for (i = 0; i < m_nFixedRows; i++)
			p->y += GetRowHeight(i);
		for (i = idTopLeft.row; i < nRow; i++)
			p->y += GetRowHeight(i);
	}

	return TRUE;
}

// Returns the bounding box of the cell
BOOL CTerrainCtrl::GetCellRect(CELLID cell, LPRECT pRect)
{
	return GetCellRect(cell.row, cell.col, pRect);
}

BOOL CTerrainCtrl::GetCellRect(int nRow, int nCol, LPRECT pRect)
{
	CPoint CellOrigin;
	if (!GetCellOrigin(nRow, nCol, &CellOrigin))
		return FALSE;

	pRect->left   = CellOrigin.x;
	pRect->top	  = CellOrigin.y;
	pRect->right  = CellOrigin.x + GetColumnWidth(nCol)-1;
	pRect->bottom = CellOrigin.y + GetRowHeight(nRow)-1;

	return TRUE;
}

BOOL CTerrainCtrl::IsCellVisible(CELLID cell)
{
	return IsCellVisible(cell.row, cell.col);
}

BOOL CTerrainCtrl::IsCellVisible(int nRow, int nCol)
{
	if (!IsWindow(m_hWnd))
		return FALSE;

	int x,y;

	CELLID TopLeft;
	if (nCol >= GetFixedColumnCount() || nRow >= GetFixedRowCount())
	{
		TopLeft = GetTopleftNonFixedCell();
		if (nCol >= GetFixedColumnCount() && nCol < TopLeft.col) return FALSE;
		if (nRow >= GetFixedRowCount() && nRow < TopLeft.row) return FALSE;
	}

	CRect rect;
	GetClientRect(rect);
	if (nCol < GetFixedColumnCount())
	{
		x = 0;
		for (int i = 0; i <= nCol; i++) 
		{
			if (x >= rect.right) return FALSE;
			x += GetColumnWidth(i);    
		}
	} 
	else 
	{
		x = GetFixedColumnWidth();
		for (int i = TopLeft.col; i <= nCol; i++) 
		{
			if (x >= rect.right) return FALSE;
			x += GetColumnWidth(i);    
		}
	}

	if (nRow < GetFixedRowCount())
	{
		y = 0;
		for (int i = 0; i <= nRow; i++) 
		{
			if (y >= rect.bottom) return FALSE;
			y += GetRowHeight(i);	 
		}
	} 
	else 
	{
		if (nRow < TopLeft.row) return FALSE;
		y = GetFixedRowHeight();
		for (int i = TopLeft.row; i <= nRow; i++) 
		{
			if (y >= rect.bottom) return FALSE;
			y += GetRowHeight(i);	 
		}
	}

	return TRUE;
}

// move about with keyboard
void CTerrainCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (!IsValid(m_idCurrentCell)) 
	{
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	CELLID next = m_idCurrentCell;
	BOOL bChangeLine = FALSE;
/*
	if (IsCTRLpressed())
	{
		switch (nChar)
		{
		   case 'A': OnEditSelectAll();  break;
		}
	}
*/
	switch (nChar)
	{
/*
		case VK_DELETE: 
			if (IsCellEditable(m_idCurrentCell.row, m_idCurrentCell.col))
			{
				SetItemText(m_idCurrentCell.row, m_idCurrentCell.col, _T(""));
				RedrawCell(m_idCurrentCell);
				SetModified(TRUE);
			}
			break;

		case VK_TAB:	
			if (IsSHIFTpressed())
			{
				if (next.col > m_nFixedCols) 
					next.col--;
				else if (next.col == m_nFixedCols && next.row > m_nFixedRows) 
				{
					next.row--; 
					next.col = GetColumnCount() - 1; 
					bChangeLine = TRUE;
				}
				else
					CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
			}
			else
			{
				if (next.col < (GetColumnCount() - 1)) 
					next.col++;
				else if (next.col == (GetColumnCount() - 1) && 
						 next.row < (GetRowCount() - 1) )
				{
					next.row++; 
					next.col = m_nFixedCols; 
					bChangeLine = TRUE;
				}
				else
					CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
			} 
			break;
*/
		case VK_DOWN:
			if (next.row < (GetRowCount() - 1))
				next.row++;
			break;

		case VK_UP:
			if (next.row > 1)
				next.row--; 
			break;

		case VK_RIGHT:
			if (next.col < (GetColumnCount() - 1))
				next.col++;
			break;

		case VK_LEFT:
			if (next.col > 1)
				next.col--;
			break;

		case VK_NEXT:	
		{
			CELLID idOldTopLeft = GetTopleftNonFixedCell();
			SendMessage(WM_VSCROLL, SB_PAGEDOWN, 0);
			CELLID idNewTopLeft = GetTopleftNonFixedCell();

			int increment = idNewTopLeft.row - idOldTopLeft.row;
			if (increment)
			{
				next.row += increment;
				if (next.row > (GetRowCount() - 1)) 
					next.row = GetRowCount() - 1;
			}
			else
				next.row = GetRowCount() - 1;
			break;
		}

		case VK_PRIOR:	
		{
			CELLID idOldTopLeft = GetTopleftNonFixedCell();
			SendMessage(WM_VSCROLL, SB_PAGEUP, 0);
			CELLID idNewTopLeft = GetTopleftNonFixedCell();

			int increment = idNewTopLeft.row - idOldTopLeft.row;
			if (increment) 
			{
				next.row += increment;
				if (next.row < m_nFixedRows) 
					next.row = m_nFixedRows;
			}
			else
				next.row = m_nFixedRows;
			break;
		}

		case VK_HOME:	
			SendMessage(WM_VSCROLL, SB_TOP, 0);
			next.row = m_nFixedRows;
			break;

		case VK_END:	
			SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
			next.row = GetRowCount() - 1;
			break;

		default:
			CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
	}
  
	if (next != m_idCurrentCell) 
	{
		// While moving with the Cursorkeys the current ROW/CELL will get selected
		// OR Selection will get expanded when SHIFT is pressed
		// Cut n paste from OnLButtonDown - Franco Bez 
/*		// Added check for NULL mouse mode - Chris Maunder.
		if (m_MouseMode == MOUSE_NOTHING)
		{
			m_PrevSelectedCellMap.RemoveAll();
			m_MouseMode = m_bListMode? MOUSE_SELECT_ROW : MOUSE_SELECT_CELLS;
			if (!IsSHIFTpressed() || nChar == VK_TAB)
				m_SelectionStartCell = next;
			OnSelecting(next);
			m_MouseMode = MOUSE_NOTHING;
		}
*/
		SetFocusCell(next);

		if (!IsCellVisible(next))
		{	
			EnsureVisible(next); // Make sure cell is visible

			switch (nChar)
			{
				case VK_RIGHT:	
					SendMessage(WM_HSCROLL, SB_LINERIGHT, 0); 
					break;

				case VK_LEFT:	
					SendMessage(WM_HSCROLL, SB_LINELEFT, 0);  
					break;

				case VK_DOWN:	
					SendMessage(WM_VSCROLL, SB_LINEDOWN, 0);  
					break;
				
				case VK_UP: 	
					SendMessage(WM_VSCROLL, SB_LINEUP, 0);	  
					break;				  
/*				
				case VK_TAB:	
					if (IsSHIFTpressed())
					{
						if (bChangeLine) 
						{
							SendMessage(WM_VSCROLL, SB_LINEUP, 0);
							SetScrollPos32(SB_HORZ, m_nHScrollMax);
							break;
						}
						else 
							SendMessage(WM_HSCROLL, SB_LINELEFT, 0);
					}
					else
					{
						if (bChangeLine) 
						{
							SendMessage(WM_VSCROLL, SB_LINEDOWN, 0);
							SetScrollPos32(SB_HORZ, 0);
							break;
						}
						else 
							SendMessage(WM_HSCROLL, SB_LINERIGHT, 0);
					}
					break;
*/
			}
			Invalidate();
		}
	}
}

void CTerrainCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (
	/*	!IsCTRLpressed() && 
	*/
	m_MouseMode == MOUSE_NOTHING)
	{
/*		if (!m_bHandleTabKey || (m_bHandleTabKey && nChar != VK_TAB))
*/			EditCell(m_idCurrentCell.row, m_idCurrentCell.col, nChar);
	}

	CWnd::OnChar(nChar, nRepCnt, nFlags);
}

// Instant editing of cells when keys are pressed
void CTerrainCtrl::EditCell(int nRow, int nCol, UINT nChar)
{
	EnsureVisible(nRow, nCol);

	if (!IsValid(nRow, nCol) || !IsCellVisible(nRow, nCol)) 
		return;

	CRect rect;
	if (!GetCellRect(nRow, nCol, rect))
		return;
/*
	SendMessageToParent(nRow, nCol, GVN_BEGINLABELEDIT);
*/
	char szText[20];
	sprintf(szText, "%.2f", ControlPoint(nRow, nCol));
	new CInPlaceEdit(this, rect, ES_LEFT, IDC_INPLACE_EDIT, &ControlPoint(nRow, nCol), szText, nChar);
}

void CTerrainCtrl::EnsureVisible(CELLID cell)
{
	EnsureVisible(cell.row, cell.col);
}

void CTerrainCtrl::EnsureVisible(int nRow, int nCol)
{
//	CCellRange VisibleCells = GetVisibleNonFixedCellRange();
	CRect rect;
	GetClientRect(rect);
	CELLID idTopLeft = GetTopleftNonFixedCell();

	// calc bottom
	int i, bottom = GetFixedRowHeight();
	for (i = idTopLeft.row; i < m_nRows; i++)
	{
		bottom += GetRowHeight(i);
		if (bottom >= rect.bottom)
		{
			bottom = rect.bottom;
			break;
		}
	}								 
	int maxVisibleRow = min(i, m_nRows - 1);
	
	// calc right
	int right = GetFixedColumnWidth();
	for (i = idTopLeft.col; i < m_nCols; i++)
	{
		right += GetColumnWidth(i);
		if (right >= rect.right)
		{
			right = rect.right;
			break;
		}
	}
	int maxVisibleCol = min(i, m_nCols - 1);

	right = nCol - maxVisibleCol;
	int left  = idTopLeft.col - nCol;
	int down  = nRow - maxVisibleRow;
	int up	  = idTopLeft.row - nRow;
	
	while (right > 0)
	{
		SendMessage(WM_HSCROLL, SB_LINERIGHT, 0);
		right--;
	}
	while (left > 0)
	{
		SendMessage(WM_HSCROLL, SB_LINELEFT, 0);
		left--;
	}
	while (down > 0)
	{
		SendMessage(WM_VSCROLL, SB_LINEDOWN, 0);
		down--;
	}
	while (up > 0)
	{
		SendMessage(WM_VSCROLL, SB_LINEUP, 0);
		up--;
	}
	
	// Move one more if we only see a small bit of the cell
	CRect rectCell, rectWindow;
	GetCellRect(nRow, nCol, rectCell);
	GetClientRect(rectWindow);
	if (rectCell.right > rectWindow.right)
		SendMessage(WM_HSCROLL, SB_LINERIGHT, 0);
	if (rectCell.bottom > rectWindow.bottom)
		SendMessage(WM_VSCROLL, SB_LINEDOWN, 0);
}
