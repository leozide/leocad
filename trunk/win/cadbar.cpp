// CADBar.cpp: implementation of the CCADStatusBar class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "leocad.h"
#include "CADBar.h"
#include "StepPop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CCADStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(CCADStatusBar)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCADStatusBar::CCADStatusBar()
{
	m_pPopup = NULL;
}

CCADStatusBar::~CCADStatusBar()
{

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
		CMenu menuPopups;
		menuPopups.LoadMenu(IDR_POPUPS);
		CMenu* pMenu = menuPopups.GetSubMenu(7);
		pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
	}

	CStatusBar::OnLButtonDown(nFlags, point);
}

void CCADStatusBar::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CRect rect;
	GetItemRect(CommandToIndex(ID_INDICATOR_STEP), rect);
	if (rect.PtInRect(point))
		AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_VIEW_STEP_CHOOSE);
	
	CStatusBar::OnRButtonDown(nFlags, point);
}
