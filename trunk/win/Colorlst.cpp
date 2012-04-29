#include "lc_global.h"
#include "lc_colors.h"
#include "ColorLst.h"
#include "Piecebar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CColorToolTipCtrl, CMFCToolTipCtrl)

BEGIN_MESSAGE_MAP(CColorToolTipCtrl, CMFCToolTipCtrl)
END_MESSAGE_MAP()

CSize CColorToolTipCtrl::GetIconSize()
{
	return CSize(24, 24);
}

BOOL CColorToolTipCtrl::OnDrawIcon(CDC* pDC, CRect rectImage)
{
	ASSERT_VALID(pDC);

	CString Text;
	GetWindowText(Text);

	const char* Ptr = strrchr((const char*)Text, '(');
	if (!Ptr)
		return FALSE;

	int Code;
	if (sscanf(Ptr, "(%d)", &Code) != 1)
		return FALSE;

	lcColor* Color = &gColorList[lcGetColorIndex(Code)];

	pDC->SetBkColor(RGB(Color->Value[0] * 255, Color->Value[1] * 255, Color->Value[2] * 255));
	pDC->ExtTextOut(0, 0, ETO_OPAQUE, rectImage, NULL, 0, NULL);

	return TRUE;
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
	mCurCell = 0;
	mTracking = FALSE;

	UpdateColors();
}

CColorList::~CColorList()
{
}

void CColorList::UpdateColors()
{
	mCells.RemoveAll();
	mGroups.RemoveAll();
	mGroups.SetSize(LC_NUM_COLORGROUPS);

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];

		for (int ColorIdx = 0; ColorIdx < Group->Colors.GetSize(); ColorIdx++)
		{
			lcColor* Color = &gColorList[Group->Colors[ColorIdx]];
			CColorListCell Cell;

			Cell.Color = RGB(Color->Value[0] * 255, Color->Value[1] * 255, Color->Value[2] * 255);
			Cell.ColorIndex = Group->Colors[ColorIdx];

			mCells.Add(Cell);
		}
	}

	mColumns = 14;
	mRows = 0;

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
		mRows += (gColorGroups[GroupIdx].Colors.GetSize() + mColumns - 1) / mColumns;
}

void CColorList::UpdateLayout()
{
	if (!IsWindow(m_hWnd))
		return;

	CRect ClientRect;
	GetClientRect(&ClientRect);

	CClientDC dc(this);
	CFont* OldFont = dc.SelectObject(&afxGlobalData.fontRegular);
	float TextHeight = 0.0f;

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];

		CSize TextSize = dc.GetTextExtent(Group->Name);
		TextHeight += TextSize.cy;
	}

	float CellWidth = (float)ClientRect.Width() / (float)mColumns;
	float CellHeight = ((float)ClientRect.Height() - TextHeight) / (float)mRows;

	int CurCell = 0;
	float CurY = 0.0f;

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];
		int CurColumn = 0;

		CSize TextSize = dc.GetTextExtent(Group->Name);
		mGroups[GroupIdx].Rect.SetRect(0, (int)CurY, ClientRect.Width(), (int)CurY + TextSize.cy);
		CurY += TextSize.cy;

		for (int ColorIdx = 0; ColorIdx < Group->Colors.GetSize(); ColorIdx++)
		{
			float Left = CurColumn * CellWidth;
			float Right = (CurColumn + 1) * CellWidth;
			float Top = CurY;
			float Bottom = CurY + CellHeight;

			CRect CellRect((int)Left, (int)Top, (int)Right, (int)Bottom);

			mCells[CurCell].Rect = CellRect;

			CString Text;
			lcColor* Color = &gColorList[mCells[CurCell].ColorIndex];
			Text.Format("%s (%d)", Color->Name, Color->Code);
			mToolTip.AddTool(this, Text, CellRect, CurCell + 1);

			CurColumn++;
			if (CurColumn == mColumns)
			{
				CurColumn = 0;
				CurY += CellHeight;
			}

			CurCell++;
		}

		if (CurColumn != 0)
			CurY += CellHeight;
	}

	dc.SelectObject(OldFont);
}

void CColorList::SelectCell(int CellIdx)
{
	if (CellIdx < 0 || CellIdx >= mCells.GetSize())
		return;

	if (CellIdx == mCurCell)
		return;

	InvalidateRect(mCells[mCurCell].Rect, TRUE);
	InvalidateRect(mCells[CellIdx].Rect, TRUE);
	mCurCell = CellIdx;

	CPiecesBar* Bar = (CPiecesBar*)GetParent();
	Bar->OnSelChangeColor();
}

BOOL CColorList::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	ASSERT(pParentWnd && ::IsWindow(pParentWnd->GetSafeHwnd()));

	// Get the class name and create the window
	CString ClassName = AfxRegisterWndClass(CS_CLASSDC|CS_SAVEBITS|CS_HREDRAW|CS_VREDRAW, 0, CreateSolidBrush(GetSysColor(COLOR_BTNFACE)), 0);

	if (!CWnd::Create(ClassName, _T(""), dwStyle, rect, pParentWnd, nID, NULL))
		return FALSE;

	return TRUE;
}

int CColorList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	mToolTip.Create(this);

	return 0;
}

BOOL CColorList::PreTranslateMessage(MSG* pMsg)
{
	if (mToolTip.m_hWnd)
		mToolTip.RelayEvent(pMsg);

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
	return TRUE;
}

void CColorList::Draw(CDC& dc)
{
	CRect VisRect, ClipRect, rect;
	GetClientRect(VisRect);
	CBrush Back(GetSysColor(COLOR_WINDOW));
	dc.FillRect(VisRect, &Back);

	CFont* OldFont = dc.SelectObject(&afxGlobalData.fontRegular);
	dc.SetBkMode(TRANSPARENT);

	for (int GroupIdx = 0; GroupIdx < mGroups.GetSize(); GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];
		CColorListGroup* ListGroup = &mGroups[GroupIdx];

		dc.DrawText(Group->Name, ListGroup->Rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	dc.SelectObject(OldFont);

	for (int CellIdx = 0; CellIdx < mCells.GetSize(); CellIdx++)
	{
		CBrush brush;
		brush.CreateSolidBrush(mCells[CellIdx].Color);

		CBrush* OldBrush = (CBrush*)dc.SelectObject(&brush);

		CRect rc = mCells[CellIdx].Rect;
		rc.bottom++;
		rc.right++;
		dc.Rectangle(rc);

		dc.SelectObject(OldBrush);
	}

	if (mCurCell < mCells.GetSize())
	{
		CBrush* OldBrush = dc.SelectObject(CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)));

		COLORREF cr = mCells[mCurCell].Color;
		CPen BorderPen;
		BorderPen.CreatePen(PS_SOLID, 1, RGB(255-GetRValue(cr), 255-GetGValue(cr), 255-GetBValue(cr)));
		CPen* OldPen = (CPen*)dc.SelectObject(&BorderPen);

		CRect rc = mCells[mCurCell].Rect;
		rc.OffsetRect(1, 1);
		rc.bottom--;
		rc.right--;
		dc.Rectangle(rc);

		if (GetFocus() == this)
		{
			rc.DeflateRect(2, 2);
			dc.DrawFocusRect(rc);
		}

		dc.SelectObject(OldPen);
		dc.SelectObject(OldBrush);
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

	for (int CellIdx = 0; CellIdx < mCells.GetSize(); CellIdx++)
	{
		if (!mCells[CellIdx].Rect.PtInRect(pt))
			continue;

		SelectCell(CellIdx);

		SetCapture();
		mMouseDown = pt;
		mTracking = TRUE;
		break;
	}
}

void CColorList::OnLButtonUp(UINT Flags, CPoint pt)
{
	mTracking = FALSE;
	ReleaseCapture();

	CWnd::OnLButtonUp(Flags, pt);
}

void CColorList::OnMouseMove(UINT Flags, CPoint pt)
{
	/*
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
*/
	CWnd::OnMouseMove(Flags, pt);
}

void CColorList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_LEFT)
	{
		if (mCurCell > 0)
			SelectCell(mCurCell - 1);
	}
	else if (nChar == VK_RIGHT)
	{
		if (mCurCell < mCells.GetSize() - 1)
			SelectCell(mCurCell + 1);
	}
	else if (nChar == VK_UP || nChar == VK_DOWN)
	{
		if (mCurCell < 0 || mCurCell >= mCells.GetSize())
			mCurCell = 0;

		int CurGroup = 0;
		int NumCells = 0;

		for (CurGroup = 0; CurGroup < LC_NUM_COLORGROUPS; CurGroup++)
		{
			int NumColors = gColorGroups[CurGroup].Colors.GetSize();

			if (mCurCell < NumCells + NumColors)
				break;

			NumCells += NumColors;
		}

		int Row = (mCurCell - NumCells) / mColumns;
		int Column = (mCurCell - NumCells) % mColumns;

		if (nChar == VK_UP)
		{
			if (Row > 0)
				SelectCell(mCurCell - mColumns);
			else if (CurGroup > 0)
			{
				int NumColors = gColorGroups[CurGroup - 1].Colors.GetSize();
				int NumColumns = NumColors % mColumns;

				if (NumColumns <= Column + 1)
					SelectCell(mCurCell - NumColumns - mColumns);
				else
					SelectCell(mCurCell - NumColumns);
			}
		}
		else if (nChar == VK_DOWN)
		{
			int NumColors = gColorGroups[CurGroup].Colors.GetSize();

			if (mCurCell + mColumns < NumCells + NumColors)
				SelectCell(mCurCell + mColumns);
			else
			{
				int NumColumns = NumColors % mColumns;

				if (NumColumns > Column)
					SelectCell(mCurCell + NumColumns);
				else
					SelectCell(mCurCell + mColumns + NumColumns);
			}
		}
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
	if (mCurCell < mCells.GetSize())
		InvalidateRect(mCells[mCurCell].Rect, TRUE);

	CWnd::OnSetFocus(pOldWnd);
}

void CColorList::OnKillFocus(CWnd* pNewWnd)
{
	if (mCurCell < mCells.GetSize())
		InvalidateRect(mCells[mCurCell].Rect, TRUE);

	CWnd::OnKillFocus(pNewWnd);
}

void CColorList::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	UpdateLayout();
}
