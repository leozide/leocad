////////////////////////////////////////////////////////////////////////////
// TitleTip.cpp : implementation file
//

#include "stdafx.h"
#include "TitleTip.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTitleTip

CTitleTip::CTitleTip()
{
	// Register the window class if it has not already been registered.
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();
	if(!(::GetClassInfo(hInst, TITLETIP_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style = CS_SAVEBITS ;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = LoadCursor( hInst, IDC_ARROW );
		wndcls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); 
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = TITLETIP_CLASSNAME;
		if (!AfxRegisterClass(&wndcls))
			AfxThrowResourceException();
	}
}

CTitleTip::~CTitleTip()
{
}


BEGIN_MESSAGE_MAP(CTitleTip, CWnd)
//{{AFX_MSG_MAP(CTitleTip)
ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTitleTip message handlers

BOOL CTitleTip::Create(CWnd * pParentWnd)
{
	ASSERT_VALID(pParentWnd);
	
	DWORD dwStyle = WS_POPUP; 
	DWORD dwExStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
	m_pParentWnd = pParentWnd;
	return CreateEx( dwExStyle, TITLETIP_CLASSNAME, NULL, dwStyle, 0, 0, 0, 0, 
		NULL, NULL, NULL );
}


// rectTitle            - The rectangle within which the original 
//                        title is constrained - in client coordinates
// lpszTitleText        - The text to be displayed
// xoffset              - Number of pixel that the text is offset from
//                        left border of the cell
void CTitleTip::Show(CRect rectTitle, LPCTSTR lpszTitleText, int xoffset, BOOL bFocus)
{
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(!rectTitle.IsRectEmpty());
	
	// If titletip is already displayed, don't do anything.
	if (IsWindowVisible()) 
		return;
	
	// Do not display the titletip is app does not have focus
	if (GetFocus() == NULL)
		return;
	
	// Define the rectangle outside which the titletip will be hidden.
	// We add a buffer of one pixel around the rectangle
	m_rectTitle.top = -1;
	m_rectTitle.left = -xoffset-1;
	m_rectTitle.right = rectTitle.Width()-xoffset;
	m_rectTitle.bottom = rectTitle.Height()+1;
	
	// Determine the width of the text
	m_pParentWnd->ClientToScreen(rectTitle);
	
	CClientDC dc(this);
	CString strTitle(lpszTitleText);
	CFont *pFont = m_pParentWnd->GetFont();  // use same font as ctrl
	CFont *pFontDC = dc.SelectObject(pFont);
	
	CRect rectDisplay = rectTitle;
	CSize size = dc.GetTextExtent(strTitle);
	rectDisplay.left += xoffset-2;
	rectDisplay.right = rectDisplay.left + size.cx + 5;
	rectDisplay.bottom -= 1;
	
	// Do not display if the text fits within available space
	if (rectDisplay.right <= rectTitle.right-xoffset+5)
		return;

	int screen = GetSystemMetrics(SM_CXSCREEN) - (rectDisplay.left + rectDisplay.Width());

	if (screen < 0)
	{
		rectDisplay.left += screen;
		rectDisplay.right += screen;
		m_rectTitle.left -= screen;
		m_rectTitle.right -= screen;
	}

	// Show the titletip
	SetWindowPos(&wndTop, rectDisplay.left, rectDisplay.top+1, 
		rectDisplay.Width(), rectDisplay.Height(), 
		SWP_SHOWWINDOW|SWP_NOACTIVATE);

	if (bFocus)
	{
		dc.SetBkColor(GetSysColor(COLOR_HIGHLIGHT));
		dc.ExtTextOut(0, 0, ETO_OPAQUE, CRect(0,0,rectDisplay.Width(),rectDisplay.Height()), NULL, 0, NULL);
		dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
		dc.DrawFocusRect(CRect(0,0,rectDisplay.Width(),rectDisplay.Height()));
	}
	else
	{
		FrameRect(dc.m_hDC, CRect(0,0,rectDisplay.Width(),rectDisplay.Height()),
			(HBRUSH)GetStockObject(BLACK_BRUSH));
	}
	
	dc.SetBkMode(TRANSPARENT);
	dc.TextOut(2, 0, strTitle);
	dc.SelectObject(pFontDC);
	
	SetCapture();
}

void CTitleTip::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (!m_rectTitle.PtInRect(point))
	{
		ReleaseCapture();
		ShowWindow(SW_HIDE);
		
		// Forward the message
		ClientToScreen(&point);
		CWnd *pWnd = WindowFromPoint(point);
		if (pWnd == this) 
			pWnd = m_pParentWnd;
		int hittest = (int)pWnd->SendMessage(WM_NCHITTEST, 0, MAKELONG(point.x,point.y));
		if (hittest == HTCLIENT) 
		{
			pWnd->ScreenToClient( &point );
			pWnd->PostMessage(WM_MOUSEMOVE, nFlags, MAKELONG(point.x,point.y) );
		} 
		else 
		{
			pWnd->PostMessage(WM_NCMOUSEMOVE, hittest, MAKELONG(point.x,point.y));
		}
	}
}

BOOL CTitleTip::PreTranslateMessage(MSG* pMsg) 
{
	CWnd *pWnd;
	int hittest ;
	switch( pMsg->message )
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		POINTS pts = MAKEPOINTS( pMsg->lParam );
		POINT  point;
		point.x = pts.x;
		point.y = pts.y;
		ClientToScreen(&point);
		pWnd = WindowFromPoint(point);
		if (pWnd == this) 
			pWnd = m_pParentWnd;
		
		hittest = (int)pWnd->SendMessage(WM_NCHITTEST, 0, MAKELONG(point.x,point.y));
		if (hittest == HTCLIENT) 
		{
			pWnd->ScreenToClient( &point );
			pMsg->lParam = MAKELONG(point.x,point.y);
		} 
		else 
		{
			switch (pMsg->message) 
			{
			case WM_LBUTTONDOWN: 
				pMsg->message = WM_NCLBUTTONDOWN;
				break;
			case WM_RBUTTONDOWN: 
				pMsg->message = WM_NCRBUTTONDOWN;
				break;
			case WM_MBUTTONDOWN: 
				pMsg->message = WM_NCMBUTTONDOWN;
				break;
			}
			pMsg->wParam = hittest;
			pMsg->lParam = MAKELONG(point.x,point.y);
		}
		ReleaseCapture();
		ShowWindow(SW_HIDE);
		pWnd->PostMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
		return TRUE;            
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		ReleaseCapture();
		ShowWindow(SW_HIDE);
		m_pParentWnd->PostMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
		return TRUE;
	}
	
	if (GetFocus() == NULL)
	{
		ReleaseCapture();
		ShowWindow(SW_HIDE);
		return TRUE;
	}
	
	return CWnd::PreTranslateMessage(pMsg);
}

void CTitleTip::OnCaptureChanged(CWnd *pWnd) 
{
	if (pWnd != this)
	{
		ShowWindow(SW_HIDE);
	}

	CWnd::OnCaptureChanged(pWnd);
}

void CTitleTip::OnActivateApp(BOOL bActive, HTASK hTask) 
{
	CWnd::OnActivateApp(bActive, hTask);

	if (!bActive)
	{
		ReleaseCapture();
		ShowWindow(SW_HIDE);
	}
}
