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
	m_TrackNode			= NULL;

	m_Indent				= 16;				// Indentation for tree branches
	m_TreeWidth				= 100;
	m_StepWidth				= 8;

	m_TotalHeight			= 0;				// A polite yet meaningless default

	m_TextColor = GetSysColor(COLOR_WINDOWTEXT);
	m_TextBgColor = GetSysColor(COLOR_WINDOW);
	m_HeaderColor = GetSysColor(COLOR_3DFACE);
//	m_crConnectingLines		= RGB(128,128,128);	// Some default

	m_TrackMode = LC_TIMELINE_TRACK_NONE;

	// Safeguards
	m_Selected				= NULL;
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

void CTimelineCtrl::Repopulate(lcPiece* Piece)
{
	m_Nodes.RemoveAll();
	m_TotalHeight = 0;

	while (Piece)
	{
		CTimelineNode Node;

		Node.Piece = Piece;
		Node.y = m_TotalHeight;
		m_TotalHeight += m_LineHeight;

		m_Nodes.Add(Node);

		Piece = (lcPiece*)Piece->m_Next;
	}

	ResetScrollBar();
	InvalidateRect(NULL, FALSE);
}

void CTimelineCtrl::DrawNode(CDC* pDC, int NodeIndex, int x, int y, CRect rFrame)
{
	CTimelineNode* pNode = &m_Nodes[NodeIndex];
	CRect RowRect;

	RowRect.left = x + m_Indent;
	RowRect.top = pNode->y - GetScrollPos(SB_VERT) + m_HeaderHeight;
	RowRect.right = rFrame.right;
	RowRect.bottom = RowRect.top + m_LineHeight;

	COLORREF crOldText = pDC->SetTextColor(m_TextColor);

	int iJointX = RowRect.left - m_Indent - 8;
	int iJointY = RowRect.top + (m_LineHeight / 2);

	pDC->FillSolidRect(iJointX + m_Indent - 2, iJointY - 2, 5, 5, m_TextColor);
	pDC->FillSolidRect(iJointX + m_Indent - 1, iJointY - 1, 3, 3, RGB(255,255,255));

	CString	cs = pNode->Piece->m_Name;
	RowRect.right = m_TreeWidth;
	pDC->DrawText(cs, RowRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

	pDC->SetTextColor(crOldText);

	CPen GrayPen;
	GrayPen.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(192, 192, 192));
	CPen* OldPen = pDC->SelectObject(&GrayPen);

	pDC->MoveTo(RowRect.right, RowRect.bottom);
	pDC->LineTo(rFrame.right, RowRect.bottom);

	int px = RowRect.right + 10 * m_StepWidth;
	while (px < rFrame.right)
	{
		pDC->MoveTo(px, RowRect.top);
		pDC->LineTo(px, RowRect.bottom);
		px += 10 * m_StepWidth;
	}

	CPen BlackPen;
	BlackPen.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(0, 0, 0));
	pDC->SelectObject(&BlackPen);

	pDC->MoveTo(RowRect.right, RowRect.top);
	pDC->LineTo(RowRect.right, RowRect.bottom);

	CBrush GreenBrush;
	GreenBrush.CreateSolidBrush(RGB(0, 192, 0));
	CBrush* OldBrush = pDC->SelectObject(&GreenBrush);

	CRect Rect;
	int Time = pNode->Piece->m_TimeShow;

	if (m_TrackMode == LC_TIMELINE_TRACK_SHOW && m_TrackNode == NodeIndex)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);

		Time = (pt.x - m_TreeWidth - 4) / m_StepWidth + 1;
		if (Time < 1)
			Time = 1;
	}

	Rect = CalcTimeRect(pNode, Time);
	pDC->Rectangle(Rect);

	if (m_TrackMode == LC_TIMELINE_TRACK_SHOW && m_TrackNode == NodeIndex)
	{
		CBrush GrayBrush;
		GrayBrush.CreateSolidBrush(RGB(224, 224, 224));
		pDC->SelectObject(&GrayBrush);

		Rect = CalcTimeRect(pNode, pNode->Piece->m_TimeShow);
		pDC->Rectangle(Rect);
	}

	if (pNode->Piece->m_TimeHide != (u32)-1)
	{
		CBrush RedBrush;
		RedBrush.CreateSolidBrush(RGB(192, 0, 0));
		pDC->SelectObject(&RedBrush);

		Rect = CalcTimeRect(pNode, pNode->Piece->m_TimeHide);
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
	CBrush FixedBack(m_HeaderColor), TextBack(m_TextBgColor);

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
	CRect TimeHeader(m_TreeWidth, 0, ClientRect.right, m_HeaderHeight);

	CPen BlackPen;
	BlackPen.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(0, 0, 0));
	CPen* OldPen = pDC->SelectObject(&BlackPen);
	COLORREF OldText = pDC->SetTextColor(m_TextColor);
	UINT OldAlign = pDC->SetTextAlign(TA_CENTER|TA_BASELINE);

	pDC->MoveTo(TimeHeader.left , TimeHeader.top);
	pDC->LineTo(TimeHeader.left, TimeHeader.bottom);

	pDC->Draw3dRect(TimeHeader, GetSysColor(COLOR_3DHILIGHT), GetSysColor(COLOR_3DSHADOW));

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

		DrawNode(pDC, i, 0, ScrollPos, ClientRect);

		if (y > ClipRect.bottom)
			break;
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

	Rect.left = m_TreeWidth + 4 + (Time - 1) * m_StepWidth;
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

	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	return TRUE;
}

void CTimelineCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_TrackMode == LC_TIMELINE_TRACK_SHOW)
	{
		int Time = (point.x - m_TreeWidth - 4) / m_StepWidth + 1;
		if (Time < 1)
			Time = 1;

		CTimelineNode* Node = &m_Nodes[m_TrackNode];
		Node->Piece->m_TimeShow = Time;
		if (!Node->Piece->IsVisible(lcGetActiveProject()->m_ActiveModel->m_CurFrame))
			Node->Piece->Select(false, false, false);

		lcGetActiveProject()->UpdateSelection();
		lcGetActiveProject()->UpdateAllViews();

		m_TrackMode = LC_TIMELINE_TRACK_NONE;
		InvalidateRect(NULL, FALSE);
		ReleaseCapture();
	}
}

void CTimelineCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	int ClickedIndex = FindNodeByPoint(point);

	if (ClickedIndex != -1)
	{
		CTimelineNode* Node = &m_Nodes[ClickedIndex];
		CRect Rect = CalcTimeRect(Node, Node->Piece->m_TimeShow);

		if (Rect.PtInRect(point))
		{
			m_TrackNode = ClickedIndex;
			m_TrackMode = LC_TIMELINE_TRACK_SHOW;
			SetCapture();
		}
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CTimelineCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_TrackMode == LC_TIMELINE_TRACK_SHOW)
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
		if (m_TrackMode == LC_TIMELINE_TRACK_SHOW)
		{
			m_TrackMode = LC_TIMELINE_TRACK_NONE;
			InvalidateRect(NULL, FALSE);
		}
	}

	CWnd::OnCaptureChanged(pWnd);
}
