#include "lc_global.h"
#include "timebar.h"

#include "lc_application.h"
#include "project.h"
#include "lc_model.h"
#include "piece.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDW_TIMEBAR_TIMECTRL 1000

/////////////////////////////////////////////////////////////////////////
// CTimeBar

CTimeBar::CTimeBar()
{
}

CTimeBar::~CTimeBar()
{
}

BEGIN_MESSAGE_MAP(CTimeBar, CSizingControlBarG)
	//{{AFX_MSG_MAP(CTimeBar)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////
// CTimeBar message handlers

void CTimeBar::OnSize(UINT nType, int cx, int cy) 
{
	CSizingControlBarG::OnSize(nType, cx, cy);

	if (!IsWindow(m_TimeCtrl.m_hWnd))
		return;

	m_TimeCtrl.SetWindowPos(NULL, 5, 5, cx - 10, cy - 10, SWP_NOZORDER);
}

int CTimeBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSizingControlBarG::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_TimeCtrl.Create(CRect(0,0,0,0), this, IDW_TIMEBAR_TIMECTRL);

	return 0;
}

void CTimeBar::ProcessMessage(lcMessageType Message, void* Data)
{
	if (Message == LC_MSG_FOCUS_OBJECT_CHANGED)
		m_TimeCtrl.Repopulate(lcGetActiveProject()->m_ActiveModel->m_Pieces);
}
