// ColorPopup.cpp : implementation file
//

#include "lc_global.h"
#include <math.h>
#include "ClrPick.h"
#include "ClrPopup.h"
#include "resource.h"
#include "lc_colors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define INVALID_COLOUR    -1

/////////////////////////////////////////////////////////////////////////////
// CColorPopup

CColorPopup::CColorPopup()
{
	Initialise();
}

CColorPopup::CColorPopup(CPoint p, int ColorIndex, CWnd* pParentWnd)
{
	Initialise();

	m_nColor = m_nInitialColor = ColorIndex;
	m_pParent = pParentWnd;

	CColorPopup::Create(p, ColorIndex, pParentWnd);
}

void CColorPopup::Initialise()
{
	m_nNumColumns       = 0;
	m_nNumRows          = 0;
	m_nBoxSize          = 18;
	m_nMargin           = ::GetSystemMetrics(SM_CXEDGE);
	m_nCurrentSel       = INVALID_COLOUR;
	m_nChosenColorSel   = INVALID_COLOUR;
	m_pParent           = NULL;
	m_nColor            = m_nInitialColor = 0;

	// Idiot check: Make sure the colour square is at least 5 x 5;
	if (m_nBoxSize - 2*m_nMargin - 2 < 5)
		m_nBoxSize = 5 + 2*m_nMargin + 2;

	// Create the font
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0));
	m_Font.CreateFontIndirect(&(ncm.lfMessageFont));
}

CColorPopup::~CColorPopup()
{
	m_Font.DeleteObject();
}

BOOL CColorPopup::Create(CPoint p, int ColorIndex, CWnd* pParentWnd)
{
	ASSERT(pParentWnd && ::IsWindow(pParentWnd->GetSafeHwnd()));
	ASSERT(pParentWnd->IsKindOf(RUNTIME_CLASS(CColorPicker)));

	m_pParent  = pParentWnd;
	m_nColor = m_nInitialColor = ColorIndex;

	// Get the class name and create the window
	CString szClassName = AfxRegisterWndClass(CS_CLASSDC|CS_SAVEBITS|CS_HREDRAW|CS_VREDRAW, 0, (HBRUSH)GetStockObject(LTGRAY_BRUSH),0);

	if (!CWnd::CreateEx(0, szClassName, _T(""), WS_VISIBLE|WS_POPUP, 
	                    p.x, p.y, 100, 100, // size updated soon
	                    pParentWnd->GetSafeHwnd(), 0, NULL))
		return FALSE;

	// Set the window size
	SetWindowSize();

	// Create the tooltips
	CreateToolTips();

	m_nChosenColorSel = ColorIndex;

	// Capture all mouse events for the life of this window
	SetCapture();

	return TRUE;
}

BEGIN_MESSAGE_MAP(CColorPopup, CWnd)
	//{{AFX_MSG_MAP(CColorPopup)
	ON_WM_NCDESTROY()
	ON_WM_LBUTTONUP()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorPopup message handlers

// For tooltips
BOOL CColorPopup::PreTranslateMessage(MSG* pMsg) 
{
	m_ToolTip.RelayEvent(pMsg);
	return CWnd::PreTranslateMessage(pMsg);
}

// If an arrow key is pressed, then move the selection
void CColorPopup::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	int row = GetRow(m_nCurrentSel), col = GetColumn(m_nCurrentSel);
	int nSelection = m_nCurrentSel;

	if (nChar == VK_DOWN) 
	{
		row++;

		if (row >= m_nNumRows || row * m_nNumColumns + col >= lcNumUserColors)
			row = 0;

		ChangeSelection(GetIndex(row, col));
	}

	if (nChar == VK_UP) 
	{
		if (row > 0)
			row--;
		else
		{
			col = GetColumn(GetIndex(row, col));
			row = m_nNumRows - 1;
			if (row * m_nNumColumns + col >= lcNumUserColors)
				row--;
		}

		ChangeSelection(GetIndex(row, col));
	}

	if (nChar == VK_RIGHT) 
	{
		col++;
		if (col >= m_nNumColumns || row * m_nNumColumns + col >= lcNumUserColors)
			col = 0;

		ChangeSelection(GetIndex(row, col));
	}

	if (nChar == VK_LEFT) 
	{
		if (col > 0)
			col--;
		else
		{
			col = m_nNumColumns - 1;

			if (row * m_nNumColumns + col >= lcNumUserColors)
				col = lcNumUserColors - row * m_nNumColumns - 1;
		}

		ChangeSelection(GetIndex(row, col));
	}

	if (nChar == VK_ESCAPE) 
	{
		m_nColor = m_nInitialColor;
		EndSelection(CPN_SELENDCANCEL);
		return;
	}

	if (nChar == VK_RETURN || nChar == VK_SPACE)
	{
		EndSelection(CPN_SELENDOK);
		return;
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

// auto-deletion
void CColorPopup::OnNcDestroy() 
{
	CWnd::OnNcDestroy();
	delete this;
}

void CColorPopup::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetClientRect(rect);

	dc.FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

	// Draw colour cells
	for (int i = 0; i < lcNumUserColors; i++)
		DrawCell(&dc, i);

	// Draw raised window edge (ex-window style WS_EX_WINDOWEDGE is sposed to do this,
	// but for some reason isn't
	dc.DrawEdge(rect, EDGE_RAISED, BF_RECT);
}

void CColorPopup::OnMouseMove(UINT nFlags, CPoint point) 
{
	int nNewSelection = INVALID_COLOUR;

	// Translate points to be relative raised window edge
	point.x -= m_nMargin;
	point.y -= m_nMargin;

	// Get the row and column
	nNewSelection = GetIndex(point.y / m_nBoxSize, point.x / m_nBoxSize);

	// In range? If not, default and exit
	if (nNewSelection < 0 || nNewSelection >= lcNumUserColors)
	{
		CWnd::OnMouseMove(nFlags, point);
		return;
	}

	// Has the row/col selection changed? If yes, then redraw old and new cells.
	if (nNewSelection != m_nCurrentSel)
		ChangeSelection(nNewSelection);

	CWnd::OnMouseMove(nFlags, point);
}

// End selection on LButtonUp
void CColorPopup::OnLButtonUp(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonUp(nFlags, point);

	DWORD pos = GetMessagePos();
	point = CPoint(LOWORD(pos), HIWORD(pos));

	if (m_WindowRect.PtInRect(point))
		EndSelection(CPN_SELENDOK);
	else
		EndSelection(CPN_SELENDCANCEL);
}

/////////////////////////////////////////////////////////////////////////////
// CColorPopup implementation

int CColorPopup::GetIndex(int row, int col) const
{ 
	if (row < 0 || col < 0 || row >= m_nNumRows || col >= m_nNumColumns)
		return INVALID_COLOUR;
	else
	{
		if (row*m_nNumColumns + col >= lcNumUserColors)
			return INVALID_COLOUR;
		else
			return row*m_nNumColumns + col;
	}
}

int CColorPopup::GetRow(int nIndex) const
{
	if (nIndex < 0 || nIndex >= lcNumUserColors)
		return INVALID_COLOUR;
	else
		return nIndex / m_nNumColumns; 
}

int CColorPopup::GetColumn(int nIndex) const
{
	if (nIndex < 0 || nIndex >= lcNumUserColors)
		return INVALID_COLOUR;
	else
		return nIndex % m_nNumColumns; 
}

// Gets the dimensions of the colour cell given by (row,col)
BOOL CColorPopup::GetCellRect(int nIndex, const LPRECT& rect)
{
	if (nIndex < 0 || nIndex >= lcNumUserColors)
		return FALSE;

	rect->left = GetColumn(nIndex) * m_nBoxSize + m_nMargin;
	rect->top  = GetRow(nIndex) * m_nBoxSize + m_nMargin;

	rect->right = rect->left + m_nBoxSize;
	rect->bottom = rect->top + m_nBoxSize;

	return TRUE;
}

// Works out an appropriate size and position of this window
void CColorPopup::SetWindowSize()
{
	// Get the number of columns and rows
	m_nNumColumns = 13;
	m_nNumRows = lcNumUserColors / m_nNumColumns;
	if (lcNumUserColors % m_nNumColumns)
		m_nNumRows++;

	// Get the current window position, and set the new size
	CRect rect;
	GetWindowRect(rect);

	m_WindowRect.SetRect(rect.left, rect.top, 
		rect.left + m_nNumColumns*m_nBoxSize + 2*m_nMargin,
		rect.top  + m_nNumRows*m_nBoxSize + 2*m_nMargin);

	// Need to check it'll fit on screen: Too far right?
	CSize ScreenSize(::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));
	if (m_WindowRect.right > ScreenSize.cx)
		m_WindowRect.OffsetRect(-(m_WindowRect.right - ScreenSize.cx), 0);

	// Too far left?
	if (m_WindowRect.left < 0)
		m_WindowRect.OffsetRect( -m_WindowRect.left, 0);

	// Bottom falling out of screen?
	if (m_WindowRect.bottom > ScreenSize.cy)
	{
		CRect ParentRect;
		m_pParent->GetWindowRect(ParentRect);
		m_WindowRect.OffsetRect(0, -(ParentRect.Height() + m_WindowRect.Height()));
	}

	// Set the window size and position
	MoveWindow(m_WindowRect, TRUE);
}

void CColorPopup::CreateToolTips()
{
	// Create the tool tip
	if (!m_ToolTip.Create(this))
		return;

	// Add a tool for each cell
	for (int i = 0; i < lcNumUserColors; i++)
	{
		CRect rect;
		if (!GetCellRect(i, rect))
			continue;
		m_ToolTip.AddTool(this, g_ColorList[i].Name, rect, 1);
	}
}

void CColorPopup::ChangeSelection(int nIndex)
{
	CClientDC dc(this);        // device context for drawing

	if (nIndex > lcNumUserColors)
		nIndex = 0; 

	if ((m_nCurrentSel >= 0 && m_nCurrentSel < lcNumUserColors))
	{
		// Set Current selection as invalid and redraw old selection (this way
		// the old selection will be drawn unselected)
		int OldSel = m_nCurrentSel;
		m_nCurrentSel = INVALID_COLOUR;
		DrawCell(&dc, OldSel);
	}

	// Set the current selection as row/col and draw (it will be drawn selected)
	m_nCurrentSel = nIndex;
	DrawCell(&dc, m_nCurrentSel);
}

void CColorPopup::EndSelection(int nMessage)
{
	ReleaseCapture();

	if (nMessage == CPN_SELENDCANCEL)
		m_nColor = m_nInitialColor;

	m_pParent->SendMessage(nMessage, (WPARAM)m_nColor, (LPARAM)m_nCurrentSel);

	DestroyWindow();
}

void CColorPopup::DrawCell(CDC* pDC, int nIndex)
{
	CRect rect;
	if (!GetCellRect(nIndex, rect))
		return;

	// fill background
	if (m_nChosenColorSel == nIndex && m_nCurrentSel != nIndex)
		pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DHILIGHT));
	else
		pDC->FillSolidRect(rect, ::GetSysColor(COLOR_3DFACE));

	// Draw button
	if (m_nCurrentSel == nIndex) 
		pDC->DrawEdge(rect, EDGE_RAISED, BF_RECT);
	else if (m_nChosenColorSel == nIndex)
		pDC->DrawEdge(rect, EDGE_SUNKEN, BF_RECT);

	CBrush brush;

	if (LC_COLOR_TRANSLUCENT(nIndex))
	{
		WORD CheckerBits[8] = { 0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33 };

		// Use the bit pattern to create a bitmap.
		CBitmap bm;
		bm.CreateBitmap(8,8,1,1, CheckerBits);

		// Create a pattern brush from the bitmap.
		brush.CreatePatternBrush(&bm);
		pDC->SetTextColor(LC_COLOR_RGB(nIndex));
		pDC->SetBkColor(RGB(255,255,255));
	}
	else
	{
		brush.CreateSolidBrush(LC_COLOR_RGB(nIndex));
	}

	CPen pen;
	pen.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));

	CBrush* pOldBrush = (CBrush*) pDC->SelectObject(&brush);
	CPen*   pOldPen   = (CPen*)   pDC->SelectObject(&pen);

	// Draw the cell colour
	rect.DeflateRect(m_nMargin+1, m_nMargin+1);
	pDC->Rectangle(rect);

	// restore DC and cleanup
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
	brush.DeleteObject();
	pen.DeleteObject();
}

void CColorPopup::OnKillFocus(CWnd* pNewWnd) 
{
	CWnd::OnKillFocus(pNewWnd);

	ReleaseCapture();
	//DestroyWindow(); - causes crash when Custom colour dialog appears.
}

// KillFocus problem fix suggested by Paul Wilkerson.
void CColorPopup::OnActivateApp(BOOL bActive, ACTIVATEAPPPARAM hTask) 
{
	CWnd::OnActivateApp(bActive, hTask);

	// If Deactivating App, cancel this selection
	if (!bActive)
		 EndSelection(CPN_SELENDCANCEL);
}
