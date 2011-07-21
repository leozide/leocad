#include "lc_global.h"
#include "timectrl.h"
#include "piece.h"
#include "project.h"
#include "lc_application.h"
#include "lc_model.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LC_TIMELINECTRL_CLASSNAME _T("TimelineCtrl")

/////////////////////////////////////////////////////////////////////////////
// CTimelineCtrl

CTimelineCtrl::CTimelineCtrl()
{
	m_TreeWidth = 100;
	m_StepWidth = 8;

	m_TrackMode = LC_TIMELINE_TRACK_NONE;
	m_TotalHeight = 0;

	m_FilterStart = 1;
	m_FilterEnd = LC_OBJECT_TIME_MAX;
}

CTimelineCtrl::~CTimelineCtrl()
{
	DestroyWindow();
	m_Font.DeleteObject();
}


BEGIN_MESSAGE_MAP(CTimelineCtrl, CWnd)
	//{{AFX_MSG_MAP(CTimelineCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_SETCURSOR()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_CAPTURECHANGED()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimelineCtrl message handlers


BOOL CTimelineCtrl::Create(const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwStyle)
{
	ASSERT(pParentWnd->GetSafeHwnd());

	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetResourceHandle();

	if (!(::GetClassInfo(hInst, LC_TIMELINECTRL_CLASSNAME, &wndcls)))
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
		wndcls.lpszClassName	= LC_TIMELINECTRL_CLASSNAME;

		if (!AfxRegisterClass(&wndcls))
			AfxThrowResourceException();
	}

	if (!CWnd::Create(LC_TIMELINECTRL_CLASSNAME, NULL, dwStyle, rect, pParentWnd, nID)) 
		return FALSE;

	if (m_Font.GetSafeHandle() != NULL)
		m_Font.DeleteObject();
	
	NONCLIENTMETRICS ncm;
	LOGFONT lf;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	VERIFY(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0));
	memcpy(&lf, &(ncm.lfMessageFont), sizeof(LOGFONT));
	m_Font.CreateFontIndirect(&lf);

	CDC		*pDC		= GetDC();
	int		iSaved		= pDC->SaveDC();
	CFont*	pOldFont	= pDC->SelectObject(&m_Font);

	TEXTMETRIC tm;
	GetTextMetrics(pDC->m_hDC, &tm);
	m_LineHeight = tm.tmHeight + 4;
	m_HeaderHeight = m_LineHeight;
	pDC->SelectObject(pOldFont);
	pDC->RestoreDC(iSaved);
	ReleaseDC(pDC);

	ResetScrollBar();

	return TRUE;
}

void CTimelineCtrl::Repopulate()
{
	m_Nodes.RemoveAll();
	m_TotalHeight = 0;

	lcModel* Model = lcGetActiveProject()->m_ActiveModel;

	for (lcPiece* Piece = Model->m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
	{
		if (!Piece->IsVisible(Model->m_CurFrame))
			continue;

		if (Piece->m_TimeShow < m_FilterStart || Piece->m_TimeShow > m_FilterEnd)
			continue;

		CTimelineNode Node;

		Node.Piece = Piece;
		Node.y = m_TotalHeight;
		m_TotalHeight += m_LineHeight;

		m_Nodes.Add(Node);
	}

	ResetScrollBar();
	InvalidateRect(NULL, FALSE);
}

void CTimelineCtrl::DrawNode(CDC* pDC, int NodeIndex, int ScrollPos, const CRect& ClientRect)
{
	CTimelineNode* Node = &m_Nodes[NodeIndex];
	lcPiece* Piece = Node->Piece;

	CRect RowRect;
	RowRect.left = ClientRect.left;
	RowRect.top = Node->y - ScrollPos + m_HeaderHeight;
	RowRect.right = ClientRect.right;
	RowRect.bottom = RowRect.top + m_LineHeight;

	// Draw text.
	CRect TextRect(RowRect);
	TextRect.right = m_TreeWidth;

	COLORREF OldText;
	if (Piece->IsFocused())
	{
		pDC->FillSolidRect(TextRect, GetSysColor(COLOR_HIGHLIGHT));
		OldText = pDC->SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else if (Piece->IsSelected())
	{
		pDC->FillSolidRect(TextRect, GetSysColor(COLOR_BTNFACE));
		OldText = pDC->SetTextColor(GetSysColor(COLOR_BTNTEXT));
	}
	else
	{
		OldText = pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	}

	TextRect.DeflateRect(1,1);
	pDC->DrawText(CString(Piece->m_Name), TextRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
	pDC->SetTextColor(OldText);

	// Draw grid.
	CPen GrayPen;
	GrayPen.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(192, 192, 192));
	CPen* OldPen = pDC->SelectObject(&GrayPen);

	pDC->MoveTo(TextRect.right, RowRect.bottom);
	pDC->LineTo(RowRect.right, RowRect.bottom);

	int px = TextRect.right + 10 * m_StepWidth;
	while (px < RowRect.right)
	{
		pDC->MoveTo(px, RowRect.top);
		pDC->LineTo(px, RowRect.bottom);
		px += 10 * m_StepWidth;
	}

	CPen BlackPen;
	BlackPen.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(0, 0, 0));
	pDC->SelectObject(&BlackPen);

	pDC->MoveTo(TextRect.right, RowRect.top);
	pDC->LineTo(TextRect.right, RowRect.bottom);

	CBrush GreenBrush;
	GreenBrush.CreateSolidBrush(RGB(0, 192, 0));
	CBrush* OldBrush = pDC->SelectObject(&GreenBrush);

	CRect Rect;
	int Time = Piece->m_TimeShow;

	if (m_TrackMode == LC_TIMELINE_TRACK_SHOW && Piece->IsSelected())
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);

		Time = GetTimeFromX(pt.x) - m_TrackTime + Piece->m_TimeShow;
		if (Time < 1)
			Time = 1;
	}

	Rect = CalcTimeRect(Node, Time);
	pDC->Rectangle(Rect);

	if (m_TrackMode == LC_TIMELINE_TRACK_SHOW && Piece->IsSelected())
	{
		CBrush GrayBrush;
		GrayBrush.CreateSolidBrush(RGB(224, 224, 224));
		pDC->SelectObject(&GrayBrush);

		Rect = CalcTimeRect(Node, Piece->m_TimeShow);
		pDC->Rectangle(Rect);
	}

	if (Piece->m_TimeHide != (u32)-1)
	{
		CBrush RedBrush;
		RedBrush.CreateSolidBrush(RGB(192, 0, 0));
		pDC->SelectObject(&RedBrush);

		Rect = CalcTimeRect(Node, Piece->m_TimeHide);
		pDC->Rectangle(Rect);
	}

	pDC->SelectObject(OldBrush);
	pDC->SelectObject(OldPen);
}

void CTimelineCtrl::ResetScrollBar()
{
	CRect rFrame;

	GetClientRect(rFrame);

	// Need for scrollbars?
	if (rFrame.Height() > m_TotalHeight + m_HeaderHeight)
	{
		ShowScrollBar(SB_VERT, FALSE);	// Hide it
		SetScrollPos(SB_VERT, 0);
	}
	else
	{
		SCROLLINFO	si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_PAGE | SIF_RANGE;
		si.nPage = rFrame.Height();
		si.nMax = m_TotalHeight + m_HeaderHeight;
		si.nMin = 0 ;

		SetScrollInfo(SB_VERT, &si);
		EnableScrollBarCtrl(SB_VERT, TRUE);
	}
}

int CTimelineCtrl::FindNodeByPoint(const CPoint& point)
{
	if (point.y < m_HeaderHeight)
		return -1;

	int y = point.y + GetScrollPos(SB_VERT) - m_HeaderHeight; 

	for (int i = 0; i < m_Nodes.GetSize(); i++)
		if (y > m_Nodes[i].y && y < m_Nodes[i].y + m_LineHeight)
			return i;

	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CTimelineCtrl message handlers

void CTimelineCtrl::OnPaint()
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

BOOL CTimelineCtrl::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;	// Don't erase the background.
}

void CTimelineCtrl::EraseBkgnd(CDC* pDC)
{
	CRect VisRect, ClipRect, rect;
	CBrush FixedBack(GetSysColor(COLOR_3DFACE)), TextBack(GetSysColor(COLOR_WINDOW));

	if (pDC->GetClipBox(ClipRect) == ERROR)
		return;

	GetClientRect(VisRect);
	VisRect.top = m_HeaderHeight;
	VisRect.left = 0;

	// Draw header background
	if (ClipRect.top < m_HeaderHeight && ClipRect.right > 0 && ClipRect.left < VisRect.right)
		pDC->FillRect(CRect(-1, ClipRect.top, VisRect.right, m_HeaderHeight), &FixedBack);

	if (rect.IntersectRect(VisRect, ClipRect)) 
	{
		CRect CellRect(max(0, rect.left), max(m_HeaderHeight, rect.top), rect.right, rect.bottom);
		pDC->FillRect(CellRect, &TextBack);
	}
}

void CTimelineCtrl::OnDraw(CDC* pDC) 
{
	CRect ClientRect;
	GetClientRect(ClientRect);

	CRect ClipRect;
	if (pDC->GetClipBox(&ClipRect) == ERROR)
		return;

	EraseBkgnd(pDC);

	UINT Mode = pDC->SetBkMode(TRANSPARENT);
	CFont* OldFont = pDC->SelectObject(&m_Font);

	// Draw header.
	CPen BlackPen;
	BlackPen.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(0, 0, 0));
	CPen* OldPen = pDC->SelectObject(&BlackPen);
	COLORREF OldText = pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));

	CRect ListHeader(0, 0, m_TreeWidth, m_HeaderHeight);
	pDC->Draw3dRect(ListHeader, GetSysColor(COLOR_3DHILIGHT), GetSysColor(COLOR_3DSHADOW));
	ListHeader.DeflateRect(2,1);
	pDC->DrawText("Pieces", ListHeader, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

	CRect TimeHeader(m_TreeWidth, 0, ClientRect.right, m_HeaderHeight);
	pDC->MoveTo(TimeHeader.left, TimeHeader.top);
	pDC->LineTo(TimeHeader.left, TimeHeader.bottom);

	pDC->Draw3dRect(TimeHeader, GetSysColor(COLOR_3DHILIGHT), GetSysColor(COLOR_3DSHADOW));

	UINT OldAlign = pDC->SetTextAlign(TA_CENTER|TA_BASELINE);
	for (int Step = 1, x = TimeHeader.left + m_StepWidth; x < TimeHeader.right; Step++, x += m_StepWidth)
	{
		int TickHeight = 3;

		if (Step % 5 == 0)
			TickHeight += 2;

		pDC->MoveTo(x, TimeHeader.bottom - TickHeight);
		pDC->LineTo(x, TimeHeader.bottom - 1);

		if (Step % 10 == 0)
		{
			CString str;
			str.Format("%d", Step);
			pDC->TextOut(x, TimeHeader.bottom - TickHeight - 1, str);
		}
	}

	CBrush BlackBrush;
	BlackBrush.CreateSolidBrush(GetSysColor(COLOR_3DFACE));
	CBrush* OldBrush = pDC->SelectObject(&BlackBrush);

	int tx = GetXFromTime(lcGetActiveProject()->m_ActiveModel->m_CurFrame);
	CPoint Arrow[3];
	Arrow[0].x = tx - 2;
	Arrow[0].y = m_HeaderHeight - 6;
	Arrow[1].x = tx + 2;
	Arrow[1].y = m_HeaderHeight - 6;
	Arrow[2].x = tx;
	Arrow[2].y = m_HeaderHeight - 1;
	pDC->Polygon(Arrow, 3);

	pDC->SelectObject(OldBrush);
	pDC->SetTextAlign(OldAlign);
	pDC->SetTextColor(OldText);
	pDC->SelectObject(OldPen);

	// Draw items.
	int ScrollPos = GetScrollPos(SB_VERT);

	CRgn rgn;
	rgn.CreateRectRgn(ClientRect.left, m_HeaderHeight, ClientRect.right, ClientRect.bottom);
	pDC->SelectClipRgn(&rgn, RGN_AND);

	for (int i = 0; i < m_Nodes.GetSize(); i++)
	{
		CTimelineNode* Node = &m_Nodes[i];
		int y = Node->y - ScrollPos;

		if (y + m_LineHeight < ClipRect.top)
			continue;

		DrawNode(pDC, i, ScrollPos, ClientRect);

		if (y > ClipRect.bottom)
			break;
	}

	int FilterStart = m_FilterStart, FilterEnd = m_FilterEnd;

	if (m_TrackMode == LC_TIMELINE_TRACK_TIME)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);

		int EndTime = GetTimeFromX(pt.x);
		FilterStart = lcMin(m_TrackTime, EndTime);
		FilterEnd = lcMax(m_TrackTime, EndTime);
	}

	if (FilterStart != 1 || FilterEnd != LC_OBJECT_TIME_MAX)
	{
		CRect SelRect(GetXFromTime(FilterStart) - m_StepWidth / 2, ClientRect.top, GetXFromTime(FilterEnd) + m_StepWidth / 2, ClientRect.bottom);

		CDC MemDC;
		CRect rect(0, 0, 4, 4);
		CBitmap bitmap, *OldBitmap;

		MemDC.CreateCompatibleDC(pDC);
		bitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
		OldBitmap = MemDC.SelectObject(&bitmap);
		MemDC.SetWindowOrg(rect.left, rect.top);

		MemDC.FillSolidRect(rect, 0);
		BLENDFUNCTION Blend;
		Blend.BlendOp = AC_SRC_OVER;
		Blend.BlendFlags = 0;
		Blend.SourceConstantAlpha = 128;
		Blend.AlphaFormat = 0;

		pDC->AlphaBlend(SelRect.left, SelRect.top, SelRect.Width(), SelRect.Height(), &MemDC, rect.left, rect.top, rect.Width(), rect.Height(), Blend);
		MemDC.SelectObject(OldBitmap);
	}

	if (m_TrackMode == LC_TIMELINE_TRACK_TREE_SPLIT)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);

		pDC->MoveTo(pt.x, 0);
		pDC->LineTo(pt.x, ClientRect.bottom);
	}

	pDC->SelectObject(OldFont);
	pDC->SetBkMode(Mode);
}

void CTimelineCtrl::OnSize(UINT nType, int cx, int cy) 
{
	ResetScrollBar();

	CWnd::OnSize(nType, cx, cy);
}

void CTimelineCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	int ScrollPos = GetScrollPos(SB_VERT);

	CRect rFrame;

	GetClientRect(rFrame);

	switch (nSBCode)
	{
		case SB_LINEUP:
			ScrollPos = max(ScrollPos - m_LineHeight, 0);
		break;

		case SB_LINEDOWN:
			ScrollPos = min(ScrollPos + m_LineHeight, GetScrollLimit(SB_VERT));
		break;

		case SB_PAGEUP:
			ScrollPos = max(ScrollPos - rFrame.Height(), 0);
		break;

		case SB_PAGEDOWN:
			ScrollPos = min(ScrollPos + rFrame.Height(), GetScrollLimit(SB_VERT));
		break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
		{
			SCROLLINFO si;

			ZeroMemory(&si, sizeof(SCROLLINFO));

			si.cbSize	= sizeof(SCROLLINFO);
			si.fMask	= SIF_TRACKPOS;

			if (GetScrollInfo(SB_VERT, &si, SIF_TRACKPOS))
				ScrollPos = si.nTrackPos;
			else
				ScrollPos = (UINT)nPos;
			break;
		}
	}		

	SetScrollPos(SB_VERT, ScrollPos);
	
	Invalidate();
	
	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

LRESULT CTimelineCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if (message == WM_NCHITTEST || message == WM_NCLBUTTONDOWN || message == WM_NCLBUTTONDBLCLK)
		return ::DefWindowProc(m_hWnd, message, wParam, lParam);
	
	return CWnd::WindowProc(message, wParam, lParam);
}

CRect CTimelineCtrl::CalcTimeRect(CTimelineNode* Node, int Time)
{
	CRect Rect;

	Rect.left = GetXFromTime(Time) - 4;
	Rect.right = Rect.left + 8;
	Rect.bottom = Node->y + m_LineHeight - (m_LineHeight - 8) / 2 + m_HeaderHeight - GetScrollPos(SB_VERT);
	Rect.top = Rect.bottom - 8;

	return Rect;
}

BOOL CTimelineCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint point;
	GetCursorPos(&point);
	ScreenToClient(&point);

	if (point.y < m_HeaderHeight)
	{
		CRect Rect(m_TreeWidth - 2, 0, m_TreeWidth + 2, m_HeaderHeight);

		if (Rect.PtInRect(point))
		{
			::SetCursor(AfxGetApp()->LoadCursor(IDC_HSPLITBAR));
			return TRUE;
		}
	}
	else
	{
		int ClickedIndex = FindNodeByPoint(point);

		if (ClickedIndex != -1)
		{
			CTimelineNode* Node = &m_Nodes[ClickedIndex];
			CRect Rect = CalcTimeRect(Node, Node->Piece->m_TimeShow);

			if (Rect.PtInRect(point))
			{
				::SetCursor(AfxGetApp()->LoadCursor(IDC_HSPLITBAR));
				return TRUE;
			}
		}
	}

	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	return TRUE;
}

void CTimelineCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_TrackMode == LC_TIMELINE_TRACK_SHOW)
	{
		int Time = GetTimeFromX(point.x) - m_TrackTime;

		for (int i = 0; i < m_Nodes.GetSize(); i++)
		{
			CTimelineNode* Node = &m_Nodes[i];
			lcPiece* Piece = Node->Piece;

			if (!Piece->IsSelected())
				continue;

			int NewTime = Piece->m_TimeShow + Time;
			if (NewTime < 1)
				NewTime = 1;
//			NewTime = lcClamp(NewTime, 1, LC_OBJECT_TIME_MAX);
			Piece->m_TimeShow = NewTime;

			if (!Piece->IsVisible(lcGetActiveProject()->m_ActiveModel->m_CurFrame))
				Piece->Select(false, false, false);
		}

		lcGetActiveProject()->UpdateSelection();
		lcGetActiveProject()->UpdateAllViews();

		m_TrackMode = LC_TIMELINE_TRACK_NONE;
		Repopulate();
		InvalidateRect(NULL, FALSE);
		ReleaseCapture();
	}
	else if (m_TrackMode == LC_TIMELINE_TRACK_TIME)
	{
		int EndTime = GetTimeFromX(point.x);
		m_FilterStart = lcMin(m_TrackTime, EndTime);
		m_FilterEnd = lcMax(m_TrackTime, EndTime);
		Repopulate();

		m_TrackMode = LC_TIMELINE_TRACK_NONE;
		InvalidateRect(NULL, FALSE);
		ReleaseCapture();
	}
	else if (m_TrackMode == LC_TIMELINE_TRACK_TREE_SPLIT)
	{
		m_TreeWidth = point.x;
		InvalidateRect(NULL, FALSE);
		ReleaseCapture();
	}
}

void CTimelineCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (point.y < m_HeaderHeight)
	{
		CRect Rect(m_TreeWidth - 2, 0, m_TreeWidth + 2, m_HeaderHeight);

		if (Rect.PtInRect(point))
		{
			m_TrackMode = LC_TIMELINE_TRACK_TREE_SPLIT;
			InvalidateRect(NULL, FALSE);
			SetCapture();
		}

		/*
		if (m_FilterStart != 1 || m_FilterEnd != LC_OBJECT_TIME_MAX)
		{
			m_FilterStart = 1;
			m_FilterEnd = LC_OBJECT_TIME_MAX;
			Repopulate();
			InvalidateRect(NULL, FALSE);
		}
		else
		{
			m_TrackTime = GetTimeFromX(point.x);
			m_TrackMode = LC_TIMELINE_TRACK_TIME;
			SetCapture();
		}
		*/
	}
	else
	{
		int ClickedIndex = FindNodeByPoint(point);

		if (ClickedIndex != -1)
		{
			Project* project = lcGetActiveProject();
			lcPiece* Piece = m_Nodes[ClickedIndex].Piece;

			if (point.x < m_TreeWidth)
				project->ToggleObjectSelectedState(Piece, nFlags & MK_CONTROL ? true : false);

			CTimelineNode* Node = &m_Nodes[ClickedIndex];
			CRect Rect = CalcTimeRect(Node, Node->Piece->m_TimeShow);

			if (Rect.PtInRect(point))
			{
				if (!Piece->IsSelected())
					project->ToggleObjectSelectedState(Piece, nFlags & MK_CONTROL ? true : false);

				m_TrackNode = ClickedIndex;
				m_TrackMode = LC_TIMELINE_TRACK_SHOW;
				m_TrackTime = GetTimeFromX(point.x);
				SetCapture();
			}
		}
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CTimelineCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (point.y < m_HeaderHeight)
	{
		int Time = GetTimeFromX(point.x);
		lcGetActiveProject()->HandleCommand(LC_VIEW_STEP_SET, Time);
	}

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CTimelineCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_TrackMode == LC_TIMELINE_TRACK_SHOW || m_TrackMode == LC_TIMELINE_TRACK_TIME || m_TrackMode == LC_TIMELINE_TRACK_TREE_SPLIT)
	{
		InvalidateRect(NULL, FALSE);
	}

	CWnd::OnMouseMove(nFlags, point);
}

BOOL CTimelineCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	// zDelta greater than 0, means rotating away from the user, that is, scrolling up
	OnVScroll(zDelta > 0 ? SB_LINEUP : SB_LINEDOWN, 0, NULL);

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CTimelineCtrl::OnCaptureChanged(CWnd *pWnd)
{
	if (pWnd != this)
	{
		if (m_TrackMode != LC_TIMELINE_TRACK_NONE)
		{
			m_TrackMode = LC_TIMELINE_TRACK_NONE;
			InvalidateRect(NULL, FALSE);
		}
	}

	CWnd::OnCaptureChanged(pWnd);
}
