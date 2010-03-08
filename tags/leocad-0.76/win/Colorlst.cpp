#include "lc_global.h"
#include "leocad.h"
#include "ColorLst.h"
#include "lc_colors.h"
#include "lc_message.h"
#include "lc_application.h"
#include "project.h"
#include <math.h>
#include <windowsx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CColorToolTipCtrl, CToolTipCtrl)

BEGIN_MESSAGE_MAP(CColorToolTipCtrl, CToolTipCtrl)
	ON_NOTIFY_REFLECT(TTN_SHOW, OnNotifyShow)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

CColorToolTipCtrl::CColorToolTipCtrl()
{
}

CColorToolTipCtrl::~CColorToolTipCtrl()
{
}

void CColorToolTipCtrl::OnNotifyShow(NMHDR* pNMHDR, LRESULT* pResult)
{
	// Grow window by 55 pixels.
	CRect rc;
	GetWindowRect(&rc);
	rc.right += 55;
	int sx = GetSystemMetrics(SM_CXSCREEN);
	if (rc.right > sx)
		rc.OffsetRect(sx - rc.right, 0);

	SetWindowPos(0, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
	*pResult = TRUE;
}

void CColorToolTipCtrl::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTTCUSTOMDRAW pcd = (LPNMTTCUSTOMDRAW)pNMHDR;

	switch (pcd->nmcd.dwDrawStage) 
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYPOSTPAINT;
		return;

	case CDDS_POSTPAINT:
		CRect rc;
		::GetClientRect(pNMHDR->hwndFrom, rc);
		rc.left = rc.right - 50;

		CString Text;
		GetWindowText(Text);
		COLORREF clr = RGB(0,0,0);

		// String compares are terrible but I can't find a better way to get the colors.
		for (int i = 0; i < lcNumUserColors; i++)
			if (g_ColorList[i].Name == (const char*)Text)
			{
				clr = RGB(g_ColorList[i].Value[0]*255, g_ColorList[i].Value[1]*255, g_ColorList[i].Value[2]*255);
				break;
			}

		SetBkColor(pcd->nmcd.hdc, clr);
		ExtTextOut(pcd->nmcd.hdc, 0, 0, ETO_OPAQUE, rc, NULL, 0, NULL);

		*pResult = CDRF_DODEFAULT;
		return;
	}

	*pResult = CDRF_DODEFAULT;
}

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

void CColorTab::Draw(CDC& dc, CFont& Font, BOOL Selected, BOOL Focus)
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

	if (Focus && Selected)
		dc.DrawFocusRect(rc);
}

BEGIN_MESSAGE_MAP(CColorList, CWnd)
	//{{AFX_MSG_MAP(CColorList)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_GETDLGCODE()
	ON_WM_SETCURSOR()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CColorList::CColorList()
{
	m_CurTab = -1;

	UpdateColorConfig();

	m_CurColor = 0;
	m_ColorFocus = true;

	m_Tracking = FALSE;
}

CColorList::~CColorList()
{
	for (int i = 0; i < m_Tabs.GetSize(); i++)
		delete m_Tabs[i];
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

	CDC MemDC;
	CRect rect;
	CBitmap bitmap, *pOldBitmap;
	dc.GetClipBox(&rect);
	MemDC.CreateCompatibleDC(&dc);
	bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
	pOldBitmap = MemDC.SelectObject(&bitmap);
	MemDC.SetWindowOrg(rect.left, rect.top);
	Draw(MemDC);
	dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &MemDC, rect.left, rect.top, SRCCOPY);
	MemDC.SelectObject(pOldBitmap);
}

BOOL CColorList::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;	// Don't erase the background.
}

void CColorList::Draw(CDC& dc)
{
	CRect VisRect, ClipRect, rect;
	GetClientRect(VisRect);
	CBrush Back(GetSysColor(COLOR_BTNFACE));
	dc.FillRect(VisRect, &Back);

	CColorTab* CurTab = NULL;
	BOOL Focus = (GetFocus() == this);

	// Draw all the normal tabs.
	for (int i = 0; i < m_Tabs.GetSize(); i++) 
	{
		CColorTab* Tab = m_Tabs[i];

		if (i == m_CurTab)
			CurTab = Tab;
		else
			Tab->Draw(dc, m_NormalFont, FALSE, Focus && !m_ColorFocus);
	}

	// Draw selected tab last so it will be "on top" of the others.
	if (CurTab)
		CurTab->Draw(dc, m_SelectedFont, TRUE, Focus && !m_ColorFocus);

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

	CBrush* OldBrush = dc.SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));

	if (m_CurColor < m_Colors.GetSize())
	{
		COLORREF cr = m_Colors[m_CurColor].Color;
		CPen BorderPen;
		BorderPen.CreatePen(PS_SOLID, 1, RGB(255-GetRValue(cr), 255-GetGValue(cr), 255-GetBValue(cr)));
		dc.SelectObject(&BorderPen);

		CRect rc = m_Colors[m_CurColor].Rect;
		rc.OffsetRect(1, 1);
		rc.bottom--;
		rc.right--;
		dc.Rectangle(rc);

		if (Focus && m_ColorFocus)
		{
			rc.DeflateRect(2, 2);
			dc.DrawFocusRect(rc);
		}
	}

	dc.SelectObject(OldPen);
	dc.SelectObject(OldBrush);
}

void CColorList::UpdateLayout()
{
	if (!IsWindow(m_hWnd))
		return;

	CClientDC dc(this);

	CFont* OldFont = dc.SelectObject(&m_SelectedFont);
	int x = 0;

	for (int i = 0; i < m_Tabs.GetSize(); i++) 
	{
		CColorTab* Tab = m_Tabs[i];

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
	rc.top = m_Tabs[0]->m_Rect.bottom;

	int TotalWidth = rc.Width();
	int TotalHeight = rc.Height();

	if (!TotalWidth || !TotalHeight)
		return;

	TotalWidth -= 2;
	TotalHeight -= 2;

	int Aspect = ABS(TotalWidth / TotalHeight);

//	Cols = Aspect * Rows
//	Cols * Rows = m_Colors.GetSize()

	m_ColorRows = (int)sqrtf((float)(m_Colors.GetSize()) / (float)Aspect);

	if (!m_ColorRows)
		return;

	m_ColorCols = (m_Colors.GetSize() + m_ColorRows - 1) / m_ColorRows;

	float CellWidth = (float)TotalWidth / (float)m_ColorCols;
	float CellHeight = (float)TotalHeight / (float)m_ColorRows;

	for (int i = 0; i < lcNumUserColors; i++)
		m_ToolTip.DelTool(this, i+1);

	for (int ColorIndex = 0; ColorIndex < m_Colors.GetSize(); ColorIndex++)
	{
		int Row = ColorIndex / m_ColorCols;
		int Col = ColorIndex % m_ColorCols;

		float Left = Col * CellWidth + rc.left + 1;
		float Right = (Col + 1) * CellWidth + rc.left + 1;
		float Top = Row * CellHeight + rc.top + 1;
		float Bottom = (Row + 1) * CellHeight + rc.top + 1;

		CRect Cell((int)Left, (int)Top, (int)Right, (int)Bottom);

		m_Colors[ColorIndex].Rect = Cell;
		m_ToolTip.AddTool(this, m_Colors[ColorIndex].Name, Cell, ColorIndex+1);
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
		CColorTab* TabPtr = m_Tabs[i];

		if (TabPtr->m_Rgn.PtInRegion(pt))
		{
			SelectTab(i);
			return;
		}
	}

	for (int i = 0; i < m_Colors.GetSize(); i++)
	{
		if (!m_Colors[i].Rect.PtInRect(pt))
			continue;

		SelectColor(i);

		SetCapture();
		m_MouseDown = pt;
		m_Tracking = TRUE;
	}
}

void CColorList::OnLButtonUp(UINT Flags, CPoint pt)
{
	m_Tracking = FALSE;
	ReleaseCapture();

	CWnd::OnLButtonUp(Flags, pt);
}

void CColorList::OnMouseMove(UINT Flags, CPoint pt)
{
	if (m_Tracking && m_MouseDown != pt)
	{
		m_Tracking = FALSE;
		ReleaseCapture();
		lcGetActiveProject()->BeginColorDrop();

		// Force a cursor update.
		CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
		CView* pView = pFrame->GetActiveView();
		pView->PostMessage(WM_LC_SET_CURSOR, 0, 0);
	}

	CWnd::OnMouseMove(Flags, pt);
}

void CColorList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	int Row = m_CurColor / m_ColorCols;
	int Col = m_CurColor % m_ColorCols;

	switch (nChar)
	{
		case VK_UP:
			if (m_ColorFocus)
			{
				if (Row > 0)
				{
					Row--;

					SelectColor(Row * m_ColorCols + Col);
				}
				else
					SelectTab(m_CurTab);
			}
			break;

		case VK_DOWN:
			if (m_ColorFocus)
			{
				if (Row < m_ColorRows - 1)
					Row++;

				SelectColor(Row * m_ColorCols + Col);
			}
			else
				SelectColor(m_CurColor);
			break;

		case VK_LEFT:
			if (m_ColorFocus)
			{
				if (Col > 0)
					Col--;
				else if (Row > 0)
				{
					Row--;
					Col = m_ColorCols - 1;
				}

				SelectColor(Row * m_ColorCols + Col);
			}
			else
			{
				if (m_CurTab > 0)
					SelectTab(m_CurTab - 1);
			}
			break;

		case VK_RIGHT:
			if (m_ColorFocus)
			{
				if (Col < m_ColorCols - 1)
					Col++;
				else if (Row < m_ColorRows - 1)
				{
					Row++;
					Col = 0;
				}

				SelectColor(Row * m_ColorCols + Col);
			}
			else
			{
				if (m_CurTab < m_Tabs.GetSize() - 1)
					SelectTab(m_CurTab + 1);
			}
			break;
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
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
	if (m_ColorFocus)
	{
		if (m_CurColor < m_Colors.GetSize())
		InvalidateRect(m_Colors[m_CurColor].Rect, TRUE);
	}
	else
		InvalidateRect(m_Tabs[m_CurTab]->m_Rect, TRUE);

	CWnd::OnSetFocus(pOldWnd);
}

void CColorList::OnKillFocus(CWnd* pNewWnd)
{
	if (m_ColorFocus)
	{
		if (m_CurColor < m_Colors.GetSize())
			InvalidateRect(m_Colors[m_CurColor].Rect, TRUE);
	}
	else
		InvalidateRect(m_Tabs[m_CurTab]->m_Rect, TRUE);

	CWnd::OnKillFocus(pNewWnd);
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

	if (m_ColorFocus && m_Colors.GetSize())
	{
		InvalidateRect(m_Tabs[Tab]->m_Rect, TRUE);
		InvalidateRect(m_Colors[m_CurColor].Rect, TRUE);
		m_ColorFocus = false;
	}

	if (Tab == m_CurTab)
		return;

	int OldColorCode = -1;
	if (m_CurColor < m_Colors.GetSize())
		OldColorCode = m_Colors[m_CurColor].Index;

	lcColorGroup& Group = g_App->m_ColorConfig.mColorGroups[Tab];

	m_Colors.RemoveAll();
	m_Colors.SetSize(Group.Colors.GetSize());

	for (int i = 0; i < Group.Colors.GetSize(); i++)
	{
		CColorEntry& Entry = m_Colors[i];
		int Color = Group.Colors[i];

		Entry.Name = g_ColorList[Color].Name;
		Entry.Color = LC_COLOR_RGB(Color);
		Entry.Index = Color;
	}

	UpdateLayout();

	m_CurColor = 0;

	for (int i = 0; i < m_Colors.GetSize(); i++)
	{
		if (OldColorCode == m_Colors[i].Index)
		{
			m_CurColor = i;
			break;
		}
	}

	if (m_Colors.GetSize())
		g_App->m_SelectedColor = m_Colors[m_CurColor].Index;
	else
		g_App->m_SelectedColor = LC_COLOR_DEFAULT;

	lcPostMessage(LC_MSG_COLOR_CHANGED, NULL);

	if (IsWindow(m_hWnd))
		InvalidateRect(NULL, TRUE);
//	InvalidateRect(m_Tabs[m_CurTab]->m_Rect, TRUE);
//	InvalidateRect(m_Tabs[Tab]->m_Rect, TRUE);
	m_CurTab = Tab;
}

void CColorList::SelectColor(int Color)
{
	if (Color < 0 || Color >= m_Colors.GetSize())
		return;

	if (!m_ColorFocus)
	{
		InvalidateRect(m_Colors[Color].Rect, TRUE);
		InvalidateRect(m_Tabs[m_CurTab]->m_Rect, TRUE);
		m_ColorFocus = true;
	}

	if (Color == m_CurColor)
		return;

	InvalidateRect(m_Colors[m_CurColor].Rect, TRUE);
	InvalidateRect(m_Colors[Color].Rect, TRUE);
	m_CurColor = Color;

	g_App->m_SelectedColor = m_Colors[Color].Index;
	lcPostMessage(LC_MSG_COLOR_CHANGED, NULL);
}

void CColorList::UpdateColorConfig()
{
	for (int i = 0; i < m_Tabs.GetSize(); i++)
		delete m_Tabs[i];
	m_Tabs.RemoveAll();

	for (int TabIndex = 0; TabIndex < g_App->m_ColorConfig.mColorGroups.GetSize(); TabIndex++)
	{
		lcColorGroup& Group = g_App->m_ColorConfig.mColorGroups[TabIndex];

		m_Tabs.Add(new CColorTab(Group.Name));
	}

	m_CurTab = -1;
	m_CurColor = 0;
	m_ColorFocus = true;

	SelectTab(0);
}
