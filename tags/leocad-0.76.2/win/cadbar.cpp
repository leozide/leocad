// CADBar.cpp: implementation of the CCADStatusBar class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "leocad.h"
#include "CADBar.h"
#include "StepPop.h"
#include "project.h"
#include "lc_application.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define ID_STATUS_PROGRESS  17234

BEGIN_MESSAGE_MAP(CCADStatusBar, CMFCStatusBar)
	//{{AFX_MSG_MAP(CCADStatusBar)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCADStatusBar::CCADStatusBar()
{
	m_pPopup = NULL;
	m_nProgressWidth = 150;
	m_bProgressVisible = FALSE;
}

CCADStatusBar::~CCADStatusBar()
{
}

BOOL CCADStatusBar::Create(CWnd *pParentWnd, DWORD dwStyle, UINT nID)
{
	// Default creation
	BOOL bCreatedOK = CMFCStatusBar::Create(pParentWnd,dwStyle,nID);

	if (bCreatedOK)
	{
		// Also create the progress bar
		m_Progress.Create(WS_CHILD | WS_EX_STATICEDGE | PBS_SMOOTH, CRect(0,0,m_nProgressWidth,10), this, ID_STATUS_PROGRESS);
	}

	return bCreatedOK;
}

void CCADStatusBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;

	GetItemRect(CommandToIndex(ID_INDICATOR_STEP), rect);
	if (rect.PtInRect(point))
	{
		ClientToScreen(rect);
		m_pPopup = new CStepPopup(CPoint(rect.left, rect.top), this);
	}

	GetItemRect(CommandToIndex(ID_INDICATOR_SNAP), rect);
	if (rect.PtInRect(point))
	{
		ClientToScreen(&point);

		CMenu PopupMenus;
		PopupMenus.LoadMenu(IDR_POPUPS);

		CMFCPopupMenu* Popup = new CMFCPopupMenu();
		Popup->Create(AfxGetMainWnd(), point.x, point.y, PopupMenus.GetSubMenu(7)->Detach());
	}

	CMFCStatusBar::OnLButtonDown(nFlags, point);
}

void CCADStatusBar::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CRect rect;
	GetItemRect(CommandToIndex(ID_INDICATOR_STEP), rect);
	if (rect.PtInRect(point))
		AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_VIEW_STEP_CHOOSE);

	CMFCStatusBar::OnRButtonDown(nFlags, point);
}

void CCADStatusBar::OnSize(UINT nType, int cx, int cy) 
{
	CMFCStatusBar::OnSize(nType, cx, cy);

	if (m_bProgressVisible)
		AdjustProgressBarPosition();
}

BOOL CCADStatusBar::ShowProgressBar(BOOL bShow)
{
	// Save old visible status
	BOOL bOldVisible = m_bProgressVisible;

	if ((bOldVisible != bShow) && ::IsWindow(m_Progress.m_hWnd))
	{
		// Show/hide
		m_Progress.ShowWindow(bShow ? SW_SHOWNA : SW_HIDE);
		m_bProgressVisible = bShow;

		// If just shown, make sure it's in the right position
		if (bShow)
		{
			AdjustProgressBarPosition();
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}

	return bOldVisible;
}

void CCADStatusBar::AdjustProgressBarPosition()
{
	// Make sure the progress bar is created
	if(!::IsWindow(m_Progress.m_hWnd))
		return;

	CRect Rect;
	GetItemRect(0, Rect);
	m_Progress.SetWindowPos(NULL, Rect.right - m_nProgressWidth, Rect.top,
		m_nProgressWidth, Rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
}
