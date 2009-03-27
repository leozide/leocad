// WheelWnd.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "WheelWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWheelWnd

//	One and only origin window, used by the one and only hook.
static CWheelWnd *g_pOriginWnd = NULL;
static HHOOK g_hdlHook = NULL;

CWheelWnd::CWheelWnd(CPoint ptOrigin)
{
	VERIFY(m_bitmap.LoadBitmap(IDB_AUTOPAN));

	BITMAP bmp;
	VERIFY(m_bitmap.GetBitmap(&bmp));
	m_sizeBitmap.cx = bmp.bmWidth;
	m_sizeBitmap.cy = bmp.bmHeight;

	m_ptOrigin = ptOrigin;
	m_ptCursorPrevious = ptOrigin;
}

CWheelWnd::~CWheelWnd()
{
}


BEGIN_MESSAGE_MAP(CWheelWnd, CWnd)
	//{{AFX_MSG_MAP(CWheelWnd)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWheelWnd message handlers

void CWheelWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	//	Simple drawing code, all it does is blt the bitmap we have chosen onto our window.
	CDC dcMem;
	VERIFY(dcMem.CreateCompatibleDC(&dc));
	CGdiObject *pOldBmp = dcMem.SelectObject(&m_bitmap);
	VERIFY(dc.BitBlt(0, 0, m_sizeBitmap.cx, m_sizeBitmap.cy, &dcMem, 0, 0, SRCCOPY));
	dcMem.SelectObject(pOldBmp);
}

void CWheelWnd::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == IDT_LC_WHEELTIMER)
	{
		CPoint ptCursor;
		VERIFY(GetCursorPos(&ptCursor));
		ASSERT_VALID(m_pParentWnd);
		m_pParentWnd->ScreenToClient(&ptCursor);

		CPoint pt = ptCursor;

		//	If the point is within one of our margins or we are doing one way scrolling then we correct the point
		//	by setting it's axis value to the same as our origin. This makes makes the cursor behaviour
		//	the same as IE.

		CRgn rgn;
		rgn.CreateEllipticRgn(0, 0, m_sizeBitmap.cx, m_sizeBitmap.cy);
		rgn.OffsetRgn(m_ptOrigin.x-m_sizeBitmap.cx/2, m_ptOrigin.y-m_sizeBitmap.cy/2);
		if (rgn.PtInRegion(pt))
			pt = m_ptOrigin;

//			if (pt.x > m_rcCursorMargin.left && pt.x < m_rcCursorMargin.right)
//				pt.x = m_ptOrigin.x;
//	
//			if (pt.y > m_rcCursorMargin.top && pt.y < m_rcCursorMargin.bottom)
//				pt.y = m_ptOrigin.y;

		//	Now that we have our distance from our origin and we have normalised it to be within our margins
		//	where necessary we now figure out which cursor to use, this is a simple lookup into a table again.
		if (m_ptCursorPrevious != ptCursor)
		{
			m_ptCursorPrevious = ptCursor;
			const int nHoriz = max(-1, min(1, pt.x - m_ptOrigin.x)) + 1;
			const int nVert = max(-1, min(1, pt.y - m_ptOrigin.y)) + 1;

			const UINT Cursors[3][3] = { IDC_PAN_NW, IDC_PAN_LEFT, IDC_PAN_SW,
				IDC_PAN_UP, IDC_PAN_ALL, IDC_PAN_DOWN, IDC_PAN_NE, IDC_PAN_RIGHT, IDC_PAN_SE };
			HCURSOR hc = theApp.LoadCursor(Cursors[nHoriz][nVert]);
			SetCursor(hc);
		}

		if (m_ptOrigin != pt)
			m_pParentWnd->PostMessage(WM_LC_WHEEL_PAN, MAKELPARAM(m_ptOrigin.x, m_ptOrigin.y), MAKELPARAM(pt.x, pt.y));
	}
	else
		CWnd::OnTimer(nIDEvent);
}

void CWheelWnd::OnDestroy() 
{
	CWnd::OnDestroy();

	VERIFY(UnhookWindowsHookEx(g_hdlHook));
	g_hdlHook = NULL;
	g_pOriginWnd = NULL;
	KillTimer(IDT_LC_WHEELTIMER);
}

BOOL CWheelWnd::Create(CWnd* pParentWnd) 
{
	// Must be a valid parent window
	ASSERT_VALID(pParentWnd);

	CPoint pt = m_ptOrigin;
	pParentWnd->ClientToScreen(&pt);

	//	We create the window, the centre of the window should be the origin point so when all of the startup is done the cursor
	//	will appear over the centre of the origin window.
	const int nHalfHeight = m_sizeBitmap.cy / 2;
	const int nHalfWidth = m_sizeBitmap.cx / 2;
	if(CWnd::CreateEx(WS_EX_TOOLWINDOW, AfxRegisterWndClass(CS_SAVEBITS), NULL, WS_CLIPSIBLINGS | WS_POPUP, pt.x - nHalfWidth, pt.y - nHalfHeight, m_sizeBitmap.cx, m_sizeBitmap.cy, pParentWnd->GetSafeHwnd(), 0))
	{
		VERIFY(SetWindowPos(&wndTopMost, 0,0,0,0, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE));
		m_pParentWnd = pParentWnd;

		//	Determine the thresholds for the cursor changing, while the cursor is between these points the cursor does not
		//	change to one of the diagonal cursors. I think this is how IE does it - sure looks like it.
		m_rcCursorMargin.SetRect(m_ptOrigin.x - nHalfWidth, m_ptOrigin.y - nHalfHeight, m_ptOrigin.x + nHalfWidth, m_ptOrigin.y + nHalfHeight);

		//	We want a circular(or as much as the bitmap allows) window, so do the usual
		//	thing of creating a region and assigning it to the window.
		CRgn rgn;
		VERIFY(rgn.CreateEllipticRgn(0, 0, m_sizeBitmap.cx, m_sizeBitmap.cy));
		VERIFY(SetWindowRgn((HRGN)rgn.Detach(), TRUE));

		//	Set our mouse capture so that all mouse messages go to us and also set a timer
		//	so that we can periodically send our message to our parent.
		SetCapture();
		VERIFY(SetTimer(IDT_LC_WHEELTIMER, 25, NULL) == IDT_LC_WHEELTIMER);
		g_pOriginWnd = this;
		g_hdlHook = SetWindowsHookEx(WH_GETMESSAGE, HookProc, NULL, GetCurrentThreadId());
		return TRUE;
	}
	TRACE0("Failed to create Origin window\n");
	return FALSE;
}

LRESULT CWheelWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch(message)
	{
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_CANCELMODE:
	case WM_CAPTURECHANGED:
		VERIFY(DestroyWindow());
		break;		
	default:
		return CWnd::WindowProc(message, wParam, lParam);
	}

	return 0;
}

void CWheelWnd::PostNcDestroy() 
{
	CWnd::PostNcDestroy();
	delete this;
}

//	Hook procedure to catch messages that should terminate our window.
LRESULT CALLBACK CWheelWnd::HookProc(int code, WPARAM wParam ,LPARAM lParam)
{
	MSG* pMsg = (MSG*)lParam;

	ASSERT_VALID(g_pOriginWnd);
	ASSERT_VALID(g_pOriginWnd->m_pParentWnd);
	if(pMsg->hwnd == g_pOriginWnd->m_pParentWnd->GetSafeHwnd())
	{
		switch(pMsg->message)
		{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_KILLFOCUS:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_MOUSEWHEEL:
			VERIFY(g_pOriginWnd->DestroyWindow());
			break;
		default:
			//	Intentionally do nothing
			;
		}
	}
	return CallNextHookEx( g_hdlHook, code, wParam, lParam );
}
