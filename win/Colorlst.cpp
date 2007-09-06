#include "lc_global.h"
#include "leocad.h"
#include "ColorLst.h"
#include "lc_colors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CXOFFSET 8  // Defined pitch of trapezoid slant.
#define CXMARGIN 2  // Left/Right text margin.
#define CYMARGIN 1  // Top/Bottom text margin.
#define CYBORDER 1  // Top border thickness.

// Given the boundint rect, compute trapezoid region.
void CColorTab::GetTrapezoid(const CRect& rc, CPoint* pts) const
{
	pts[0] = CPoint(rc.left, rc.bottom);
	pts[1] = CPoint(rc.left + CXOFFSET, rc.top);
	pts[2] = CPoint(rc.right - CXOFFSET-1, rc.top);
	pts[3] = CPoint(rc.right - 1, rc.bottom);
}

void CColorTab::Draw(CDC& dc, CFont& Font, BOOL Selected)
{
	// Tab drawing code based on an article by Paul DiLascia.
	COLORREF bgColor = GetSysColor(Selected ? COLOR_WINDOW : COLOR_3DFACE);
	COLORREF fgColor = GetSysColor(Selected ? COLOR_WINDOWTEXT : COLOR_BTNTEXT);

	CBrush brush(bgColor);
	dc.SetBkColor(bgColor);
	dc.SetTextColor(fgColor);

	CPen blackPen(PS_SOLID, 1, RGB(0, 0, 0));
	CPen shadowPen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));

	// Fill trapezoid.
	CPoint pts[4];
	CRect rc = m_Rect;
	GetTrapezoid(rc, pts);
	CPen* pOldPen = dc.SelectObject(&blackPen);
	dc.FillRgn(&m_Rgn, &brush);

	// Draw edges. This is requires two corrections:
	// 1) Trapezoid dimensions don't include the right and bottom edges,
	// so must use one pixel less on bottom (cybottom)
	// 2) the endpoint of LineTo is not included when drawing the line, so
	// must add one pixel (cytop)
	pts[0].y--;
	dc.MoveTo(pts[0]);            // bottom left
	dc.LineTo(pts[1]);            // upper left
	dc.SelectObject(&shadowPen);  // top line is shadow color
	dc.MoveTo(pts[1]);            // line is inside trapezoid top
	dc.LineTo(pts[2]);
	dc.SelectObject(&blackPen);   // upstroke is black
	dc.LineTo(pts[3]);            // y-1 to include endpoint
	if (!Selected) 
	{
		// If not highlighted, upstroke has a 3D shadow, one pixel inside.
		pts[2].x--;  // offset left one pixel
		pts[3].x--;  // ...ditto
		dc.SelectObject(&shadowPen);
		dc.MoveTo(pts[2]);
		dc.LineTo(pts[3]);
	}
	dc.SelectObject(pOldPen);

	// draw text
	rc.DeflateRect(CXOFFSET + CXMARGIN, CYMARGIN);
	CFont* OldFont = dc.SelectObject(&Font);
	dc.DrawText(m_Text, &rc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
	dc.SelectObject(OldFont);
}

BEGIN_MESSAGE_MAP(CColorList, CWnd)
	//{{AFX_MSG_MAP(CColorList)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()
	ON_WM_GETDLGCODE()
	ON_WM_SETCURSOR()
	ON_WM_CREATE()
	//ON_WM_NCDESTROY()
	//ON_WM_LBUTTONUP()
	//ON_WM_MOUSEMOVE()
	//ON_WM_QUERYNEWPALETTE()
	//ON_WM_PALETTECHANGED()
	//ON_WM_KILLFOCUS()
	//ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CColorList::CColorList()
{
	m_Tabs.Add(new CColorTab("test"));
	m_Tabs.Add(new CColorTab("test 2"));

	m_Colors.SetSize(lcNumUserColors);
	for (int i = 0; i < lcNumUserColors; i++)
	{
		CColorEntry& Entry = m_Colors[i];
		Entry.Name = lcColorList[i].Name;
		Entry.Color = LC_COLOR_RGB(i);
		Entry.Index = i;
	}

	m_CurTab = 0;
	m_CurColor = 0;
}

CColorList::~CColorList()
{
	for (int i = 0; i < m_Tabs.GetSize(); i++)
		delete (CColorTab*)m_Tabs[i];
	m_Tabs.RemoveAll();
}

BOOL CColorList::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	ASSERT(pParentWnd && ::IsWindow(pParentWnd->GetSafeHwnd()));

	// Get the class name and create the window
	CString ClassName = AfxRegisterWndClass(CS_CLASSDC|CS_SAVEBITS|CS_HREDRAW|CS_VREDRAW, 0, CreateSolidBrush(GetSysColor(COLOR_BTNFACE)), 0);

	if (!CWnd::Create(ClassName, _T(""), dwStyle, rect, pParentWnd, nID, NULL))
		return FALSE;

	// Initialize fonts.
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfHeight = GetSystemMetrics(SM_CYHSCROLL)-CYMARGIN;
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	_tcscpy(lf.lfFaceName, _T("Arial"));
	m_NormalFont.CreateFontIndirect(&lf);

	lf.lfWeight = FW_BOLD;
	m_SelectedFont.CreateFontIndirect(&lf);

	UpdateLayout();

	return TRUE;
}

int CColorList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_ToolTip.Create(this);

	return 0;
}

BOOL CColorList::PreTranslateMessage(MSG* pMsg)
{
	if (m_ToolTip.m_hWnd)
		m_ToolTip.RelayEvent(pMsg);

	return CWnd::PreTranslateMessage(pMsg);
}

void CColorList::OnPaint() 
{
	CPaintDC dc(this);

	CColorTab* CurTab = NULL;

	// Draw all the normal tabs.
	for (int i = 0; i < m_Tabs.GetSize(); i++) 
	{
		CColorTab* Tab = (CColorTab*)m_Tabs[i];

		if (i == m_CurTab)
			CurTab = Tab;
		else
			Tab->Draw(dc, m_NormalFont, FALSE);
	}

	// Draw selected tab last so it will be "on top" of the others.
	if (CurTab)
		CurTab->Draw(dc, m_SelectedFont, TRUE);

	// Draw the colors.
	CPen BlackPen;
	BlackPen.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	CPen* OldPen = (CPen*)dc.SelectObject(&BlackPen);

	for (int i = 0; i < m_Colors.GetSize(); i++)
	{
		CBrush brush;

		if (LC_COLOR_TRANSLUCENT(m_Colors[i].Index))
		{
			WORD CheckerBits[8] = { 0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33 };

			// Use the bit pattern to create a bitmap.
			CBitmap bm;
			bm.CreateBitmap(8, 8, 1, 1, CheckerBits);

			// Create a pattern brush from the bitmap.
			brush.CreatePatternBrush(&bm);
			dc.SetTextColor(m_Colors[i].Color);
			dc.SetBkColor(RGB(255, 255, 255));
		}
		else
		{
			brush.CreateSolidBrush(m_Colors[i].Color);
		}

		CBrush* OldBrush = (CBrush*)dc.SelectObject(&brush);

		CRect rc = m_Colors[i].Rect;
		rc.bottom++;
		rc.right++;
		dc.Rectangle(rc);

		dc.SelectObject(OldBrush);
	}

	CBrush* OldBrush = (CBrush*)dc.SelectObject(GetStockObject(NULL_BRUSH));

	COLORREF cr = m_Colors[m_CurColor].Color;
	CPen BorderPen;
	BorderPen.CreatePen(PS_SOLID, 2, RGB(255-GetRValue(cr), 255-GetGValue(cr), 255-GetBValue(cr)));
	dc.SelectObject(&BorderPen);

	CRect rc = m_Colors[m_CurColor].Rect;
	rc.OffsetRect(1, 1);
	rc.DeflateRect(1, 1);
	dc.Rectangle(rc);

	// fixme: draw focus
/*
		rc.DeflateRect(2, 2);
		dc.DrawFocusRect(rc);
*/

	dc.SelectObject(OldPen);
	dc.SelectObject(OldBrush);
}

void CColorList::UpdateLayout()
{
	CClientDC dc(this);

	CFont* OldFont = dc.SelectObject(&m_SelectedFont);
	int x = 0;

	for (int i = 0; i < m_Tabs.GetSize(); i++) 
	{
		CColorTab* Tab = (CColorTab*)m_Tabs[i];

		// Calculate desired text rectangle.
		CRect& rc = Tab->m_Rect;
		rc.SetRectEmpty();
		dc.DrawText(Tab->m_Text, &rc, DT_CALCRECT);
		rc.right += 2 * CXOFFSET + 3 * CXMARGIN;
		rc.bottom = rc.top + GetSystemMetrics(SM_CYHSCROLL);
		rc += CPoint(x,0);

		// Create trapezoid region.
		CPoint pts[4];
		Tab->GetTrapezoid(rc, pts);
		Tab->m_Rgn.DeleteObject();
		Tab->m_Rgn.CreatePolygonRgn(pts, 4, WINDING);

		x += rc.Width() - CXOFFSET;
	}

	dc.SelectObject(OldFont);

	CRect rc;
	GetClientRect(&rc);
	rc.top = ((CColorTab*)m_Tabs[0])->m_Rect.bottom;

	m_ColorRows = 6;
	m_ColorCols = 13;

	int CellWidth = rc.Width() / m_ColorCols;
	int CellHeight = rc.Height() / m_ColorRows;

	for (int i = 0; i < lcNumUserColors; i++)
		m_ToolTip.DelTool(this, i+1);

	for (int i = 0; i < m_ColorRows; i++)
	{
		for (int j = 0; j < m_ColorCols; j++)
		{
			CRect cell(0, 0, CellWidth, CellHeight);
			cell.OffsetRect(j * CellWidth + rc.left, i * CellHeight + rc.top);

			int Index = i*m_ColorCols + j;
			m_Colors[Index].Rect = cell;
			m_ToolTip.AddTool(this, lcColorList[Index].Name, cell, Index+1);
		}
	}
}

void CColorList::OnLButtonDown(UINT Flags, CPoint pt)
{
	CWnd::OnLButtonDown(Flags, pt);
	SetFocus();

	CRect rc;
	GetClientRect(&rc);

	if (!rc.PtInRect(pt)) 
		return;

	for (int i = 0; i < m_Tabs.GetSize(); i++) 
	{
		CColorTab* TabPtr = (CColorTab*)m_Tabs[i];

		if (TabPtr->m_Rgn.PtInRegion(pt))
		{
			SelectTab(i);
			return;
		}
	}

	for (int i = 0; i < m_Colors.GetSize(); i++)
	{
		if (m_Colors[i].Rect.PtInRect(pt))
		{
			SelectColor(i);
			return;
		}
	}
}

void CColorList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	int Row = m_CurColor / m_ColorCols;
	int Col = m_CurColor % m_ColorCols;

	switch (nChar)
	{
		case VK_UP:
			if (Row > 0)
				Row--;
			break;

		case VK_DOWN:
			if (Row < m_ColorRows - 1)
				Row++;
			break;

		case VK_LEFT:
			if (Col > 0)
				Col--;
			else if (Row > 0)
			{
				Row--;
				Col = m_ColorCols - 1;
			}
			break;

		case VK_RIGHT:
			if (Col < m_ColorCols - 1)
				Col++;
			else if (Row < m_ColorRows - 1)
			{
				Row++;
				Col = 0;
			}
			break;
	}

/*
	if (nChar == VK_INSERT)
	{
		CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
		pFrame->PostMessage(WM_COMMAND, ID_PIECE_INSERT, 0);

		CView* pView = pFrame->GetActiveView();
		pView->SetFocus();
	}
*/

	int Color = Row * m_ColorCols + Col;
	SelectColor(Color);
}

BOOL CColorList::OnSetCursor(CWnd* inWnd, UINT inHitTest, UINT inMessage) 
{
	HCURSOR Cursor = LoadCursor(NULL, IDC_ARROW);

	if (Cursor)
	{
		SetCursor(Cursor);
		return TRUE;
	}

	return CWnd::OnSetCursor(inWnd, inHitTest, inMessage);
}

UINT CColorList::OnGetDlgCode()
{
	return DLGC_WANTARROWS;
}

void CColorList::OnSetFocus(CWnd* pOldWnd)
{
	// fixme: draw focus

	CWnd::OnSetFocus(pOldWnd);
}

void CColorList::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	UpdateLayout();
}

void CColorList::SelectTab(int Tab)
{
	if (Tab < 0 || Tab >= m_Tabs.GetSize())
		return;

	if (Tab == m_CurTab)
		return;

	InvalidateRect(((CColorTab*)m_Tabs[m_CurTab])->m_Rect, TRUE);
	InvalidateRect(((CColorTab*)m_Tabs[Tab])->m_Rect, TRUE);
	m_CurTab = Tab;
}

void CColorList::SelectColor(int Color)
{
	if (Color < 0 || Color >= m_Colors.GetSize())
		return;

	if (Color == m_CurColor)
		return;

	InvalidateRect(m_Colors[m_CurColor].Rect, TRUE);
	InvalidateRect(m_Colors[Color].Rect, TRUE);
	m_CurColor = Color;
}
