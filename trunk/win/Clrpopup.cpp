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

#define MAX_COLOURS 256

/////////////////////////////////////////////////////////////////////////////
// CColorPopup

CColorPopup::CColorPopup()
{
	Initialise();
}

CColorPopup::CColorPopup(CPoint p, int nColor, CWnd* pParentWnd)
{
	Initialise();

	m_nColor = m_nInitialColor = nColor;
	m_pParent = pParentWnd;

	CColorPopup::Create(p, nColor, pParentWnd);
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
	if (m_nBoxSize - 2 * m_nMargin - 2 < 5)
		m_nBoxSize = 5 + 2 * m_nMargin + 2;

	// Create the font
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0));
	m_Font.CreateFontIndirect(&(ncm.lfMessageFont));

	// Create the palette
	int NumColors = gColorList.GetSize() < MAX_COLOURS ? gColorList.GetSize() : MAX_COLOURS;
	struct {
		LOGPALETTE    LogPalette;
		PALETTEENTRY  PalEntry[MAX_COLOURS];
	} pal;

	LOGPALETTE* pLogPalette = (LOGPALETTE*) &pal;
	pLogPalette->palVersion    = 0x300;
	pLogPalette->palNumEntries = (WORD)gColorList.GetSize(); 

	for (int i = 0; i < NumColors; i++)
	{
		float* Value = gColorList[i].Value;
		pLogPalette->palPalEntry[i].peRed   = (BYTE)(Value[0] * 255);
		pLogPalette->palPalEntry[i].peGreen = (BYTE)(Value[1] * 255);
		pLogPalette->palPalEntry[i].peBlue  = (BYTE)(Value[2] * 255);
		pLogPalette->palPalEntry[i].peFlags = 0;
	}

	m_Palette.CreatePalette(pLogPalette);
}

CColorPopup::~CColorPopup()
{
	m_Font.DeleteObject();
	m_Palette.DeleteObject();
}

BOOL CColorPopup::Create(CPoint p, int nColor, CWnd* pParentWnd)
{
	ASSERT(pParentWnd && ::IsWindow(pParentWnd->GetSafeHwnd()));
	ASSERT(pParentWnd->IsKindOf(RUNTIME_CLASS(CColorPicker)));

	m_pParent = pParentWnd;
	m_nColor = m_nInitialColor = nColor;

	// Get the class name and create the window
	CString szClassName = AfxRegisterWndClass(CS_CLASSDC|CS_SAVEBITS|CS_HREDRAW|CS_VREDRAW, 0, (HBRUSH)GetStockObject(LTGRAY_BRUSH),0);

	if (!CWnd::CreateEx(0, szClassName, _T(""), WS_VISIBLE|WS_POPUP, p.x, p.y, 100, 100, pParentWnd->GetSafeHwnd(), 0, NULL))
		return FALSE;

	// Set the window size
	SetWindowSize();

	// Calculate the layout
	CalculateLayout();

	// Create the tooltips
	CreateToolTips();

	// Find which cell (if any) corresponds to the initial colour
	for (int i = 0; i < mCells.GetSize(); i++)
	{
		if (mCells[i].ColorIndex == nColor)
		{
			m_nChosenColorSel = i;
			break;
		}
	}

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
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
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
	if (nChar == VK_LEFT)
	{
		if (m_nCurrentSel > 0)
			ChangeSelection(m_nCurrentSel - 1);
	}
	else if (nChar == VK_RIGHT)
	{
		if (m_nCurrentSel < mCells.GetSize() - 1)
			ChangeSelection(m_nCurrentSel + 1);
	}
		else if (nChar == VK_UP || nChar == VK_DOWN)
	{
		if (m_nCurrentSel < 0 || m_nCurrentSel >= mCells.GetSize())
			m_nCurrentSel = 0;

		int CurGroup = 0;
		int NumCells = 0;

		for (CurGroup = 0; CurGroup < LC_NUM_COLORGROUPS; CurGroup++)
		{
			int NumColors = gColorGroups[CurGroup].Colors.GetSize();

			if (m_nCurrentSel < NumCells + NumColors)
				break;

			NumCells += NumColors;
		}

		int Row = (m_nCurrentSel - NumCells) / m_nNumColumns;
		int Column = (m_nCurrentSel - NumCells) % m_nNumColumns;

		if (nChar == VK_UP)
		{
			if (Row > 0)
				ChangeSelection(m_nCurrentSel - m_nNumColumns);
			else if (CurGroup > 0)
			{
				int NumColors = gColorGroups[CurGroup - 1].Colors.GetSize();
				int NumColumns = NumColors % m_nNumColumns;

				if (NumColumns <= Column + 1)
					ChangeSelection(m_nCurrentSel - NumColumns - m_nNumColumns);
				else
					ChangeSelection(m_nCurrentSel - NumColumns);
			}
		}
		else if (nChar == VK_DOWN)
		{
			int NumColors = gColorGroups[CurGroup].Colors.GetSize();

			if (m_nCurrentSel + m_nNumColumns < NumCells + NumColors)
				ChangeSelection(m_nCurrentSel + m_nNumColumns);
			else
			{
				int NumColumns = NumColors % m_nNumColumns;

				if (NumColumns > Column)
					ChangeSelection(m_nCurrentSel + NumColumns);
				else
					ChangeSelection(m_nCurrentSel + m_nNumColumns + NumColumns);
			}
		}
	}
	else if (nChar == VK_ESCAPE) 
	{
		m_nColor = m_nInitialColor;
		EndSelection(CPN_SELENDCANCEL);
		return;
	}
	else if (nChar == VK_RETURN || nChar == VK_SPACE)
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

	CFont *pOldFont = (CFont*)dc.SelectObject(&m_Font);
	dc.SetBkMode(TRANSPARENT);

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
		dc.DrawText(gColorGroups[GroupIdx].Name, mGroups[GroupIdx], DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	dc.SelectObject(pOldFont);

	// Draw colour cells
	for (int i = 0; i < mCells.GetSize(); i++)
		DrawCell(&dc, i);

	// Draw raised window edge (ex-window style WS_EX_WINDOWEDGE is sposed to do this,
	// but for some reason isn't
	dc.DrawEdge(rect, EDGE_RAISED, BF_RECT);
}

void CColorPopup::OnMouseMove(UINT nFlags, CPoint point) 
{
	int nNewSelection = INVALID_COLOUR;

	for (int i = 0; i < mCells.GetSize(); i++)
	{
		if (mCells[i].Rect.PtInRect(point))
		{
			nNewSelection = i;
			break;
		}
	}

	if (nNewSelection == INVALID_COLOUR)
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

// Works out an appropriate size and position of this window
void CColorPopup::SetWindowSize()
{
	CSize TextSize(0, 0);

	// Calculate the number of columns and rows.
	m_nNumColumns = 14;
	m_nNumRows = 0;

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
		m_nNumRows += (gColorGroups[GroupIdx].Colors.GetSize() + m_nNumColumns - 1) / m_nNumColumns;

	// Calculate text size.
	CClientDC dc(this);
	CFont* pOldFont = (CFont*) dc.SelectObject(&m_Font);

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];

		CSize NameSize = dc.GetTextExtent(Group->Name);
		if (NameSize.cx > TextSize.cx)
			TextSize.cx = NameSize.cx;
		TextSize.cy += NameSize.cy + 2;
	}

	dc.SelectObject(pOldFont);

	// Get the current window position, and set the new size
	CRect rect;
	GetWindowRect(rect);

	m_WindowRect.SetRect(rect.left, rect.top, 
	                     rect.left + m_nNumColumns*m_nBoxSize + 2*m_nMargin,
	                     rect.top  + m_nNumRows*m_nBoxSize + 2*m_nMargin);

	if (TextSize.cx > m_WindowRect.Width())
		m_WindowRect.right = m_WindowRect.left + TextSize.cx;
	m_WindowRect.bottom += TextSize.cy;

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

void CColorPopup::CalculateLayout()
{
	CRect ClientRect;
	GetClientRect(&ClientRect);

	CClientDC dc(this);
	CFont* pOldFont = (CFont*) dc.SelectObject(&m_Font);

	int CurCell = 0;
	int CurY = m_nMargin;

	mGroups.RemoveAll();
	mGroups.SetSize(LC_NUM_COLORGROUPS);

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];
		int CurColumn = 0;

		CSize TextSize = dc.GetTextExtent(Group->Name);
		mGroups[GroupIdx].SetRect(0, CurY + 1, ClientRect.Width(), CurY + TextSize.cy + 2);
		CurY += TextSize.cy + 2;

		for (int ColorIdx = 0; ColorIdx < Group->Colors.GetSize(); ColorIdx++)
		{
			int Left = m_nMargin + CurColumn * m_nBoxSize;
			int Right = Left + m_nBoxSize;
			int Top = CurY;
			int Bottom = CurY + m_nBoxSize;

			lcColor* Color = &gColorList[Group->Colors[ColorIdx]];
			CColorPopupCell Cell;

			Cell.Color = RGB(Color->Value[0] * 255, Color->Value[1] * 255, Color->Value[2] * 255);
			Cell.ColorIndex = Group->Colors[ColorIdx];
			Cell.Rect.SetRect(Left, Top, Right, Bottom);

			mCells.Add(Cell);

			CurColumn++;
			if (CurColumn == m_nNumColumns)
			{
				CurColumn = 0;
				CurY += m_nBoxSize;
			}

			CurCell++;
		}

		if (CurColumn != 0)
			CurY += m_nBoxSize;
	}

	dc.SelectObject(pOldFont);
}

void CColorPopup::CreateToolTips()
{
	// Create the tool tip
	if (!m_ToolTip.Create(this)) return;

	// Add a tool for each cell
	for (int i = 0; i < mCells.GetSize(); i++)
	{
		CString Text;
		lcColor* Color = &gColorList[mCells[i].ColorIndex];
		Text.Format("%s (%d)", Color->Name, Color->Code);
		m_ToolTip.AddTool(this, Text, mCells[i].Rect, i + 1);
	}
}

void CColorPopup::ChangeSelection(int nIndex)
{
	CClientDC dc(this);        // device context for drawing

	if (m_nCurrentSel >= 0 && m_nCurrentSel < mCells.GetSize())
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

	m_nColor = m_nCurrentSel;
}

void CColorPopup::EndSelection(int nMessage)
{
	ReleaseCapture();

	if (nMessage == CPN_SELENDCANCEL)
		m_nColor = m_nInitialColor;

	int ColorIndex = m_nCurrentSel >= 0 ? mCells[m_nCurrentSel].ColorIndex : 0;
	m_pParent->SendMessage(nMessage, 0, (LPARAM)ColorIndex);

	DestroyWindow();
}

void CColorPopup::DrawCell(CDC* pDC, int nIndex)
{
	CRect rect = mCells[nIndex].Rect;

	// Select and realize the palette
	CPalette* pOldPalette;
	if (pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
	{
		pOldPalette = pDC->SelectPalette(&m_Palette, FALSE);
		pDC->RealizePalette();
	}

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

	CBrush brush(PALETTERGB(GetRValue(mCells[nIndex].Color), GetGValue(mCells[nIndex].Color), GetBValue(mCells[nIndex].Color) ));
	CPen   pen;
	pen.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));

	CBrush* pOldBrush = (CBrush*) pDC->SelectObject(&brush);
	CPen*   pOldPen   = (CPen*)   pDC->SelectObject(&pen);

	// Draw the cell colour
	rect.DeflateRect(m_nMargin+1, m_nMargin+1);
	pDC->Rectangle(rect);

	rect.DeflateRect(1, 1);
	rect.bottom -= 1;

	// restore DC and cleanup
	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
	brush.DeleteObject();
	pen.DeleteObject();

	if (pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
		pDC->SelectPalette(pOldPalette, FALSE);
}

BOOL CColorPopup::OnQueryNewPalette() 
{
	Invalidate();
	return CWnd::OnQueryNewPalette();
}

void CColorPopup::OnPaletteChanged(CWnd* pFocusWnd) 
{
	CWnd::OnPaletteChanged(pFocusWnd);

	if (pFocusWnd->GetSafeHwnd() != GetSafeHwnd())
		Invalidate();
}

void CColorPopup::OnKillFocus(CWnd* pNewWnd) 
{
	CWnd::OnKillFocus(pNewWnd);

	ReleaseCapture();
	//DestroyWindow(); - causes crash when Custom colour dialog appears.
}

// KillFocus problem fix suggested by Paul Wilkerson.
void CColorPopup::OnActivateApp(BOOL bActive, DWORD hTask) 
{
	CWnd::OnActivateApp(bActive, hTask);

	// If Deactivating App, cancel this selection
	if (!bActive)
		 EndSelection(CPN_SELENDCANCEL);
}
