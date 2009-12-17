// PrefSht.cpp : implementation file
//

#include "lc_global.h"
#include "resource.h"
#include "PrefSht.h"
#include "defines.h"
#include "str.h"
#include "mainfrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define IDW_BUTTON 5000

/////////////////////////////////////////////////////////////////////////////
// CPreferencesSheet

IMPLEMENT_DYNAMIC(CPreferencesSheet, CPropertySheet)

CPreferencesSheet::CPreferencesSheet(CWnd* pWndParent)
	 : CPropertySheet(IDS_PROPSHT_CAPTION, pWndParent)
{
	AddPage(&m_PageGeneral);
	AddPage(&m_PageDetail);
	AddPage(&m_PageDrawing);
	AddPage(&m_PageColors);
	AddPage(&m_PagePrint);
	AddPage(&m_PageKeyboard);
	SetActivePage(AfxGetApp()->GetProfileInt("Settings", "Page", 0));
}

CPreferencesSheet::~CPreferencesSheet()
{
}


BEGIN_MESSAGE_MAP(CPreferencesSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPreferencesSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDW_BUTTON, OnDefault)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPreferencesSheet message handlers

BOOL CPreferencesSheet::PreTranslateMessage(MSG* pMsg) 
{
	if (LOWORD(pMsg->wParam) == IDOK)
		AfxGetApp()->WriteProfileInt("Settings", "Page", GetActiveIndex());
	
	return m_tabCtrl.TranslatePropSheetMsg(pMsg) ? TRUE :
		CPropertySheet::PreTranslateMessage(pMsg);
}

BOOL CPreferencesSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
	// get HWND of tab control and subclass it
	HWND hWnd = (HWND)SendMessage(PSM_GETTABCONTROL);
	m_tabCtrl.SubclassDlgItem(::GetDlgCtrlID(hWnd), this);

	CRect rectWnd;
	CWnd *pWnd = GetDlgItem(ID_APPLY_NOW);
	pWnd->GetWindowRect(rectWnd);
	pWnd->ShowWindow(FALSE);
	ScreenToClient(rectWnd);

	hWnd = CreateWindow(_T("BUTTON"), _T("Make Default"),BS_PUSHBUTTON|WS_CHILD|WS_VISIBLE|WS_TABSTOP,
		rectWnd.left, rectWnd.top, rectWnd.Width(), rectWnd.Height(), m_hWnd, (HMENU)IDW_BUTTON, AfxGetInstanceHandle(), NULL);
	::SendMessage(hWnd, WM_SETFONT, (WPARAM)GetFont()->GetSafeHandle(), TRUE);
	::SetWindowPos (hWnd, ::GetDlgItem(m_hWnd, IDCANCEL), 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	
	return bResult;
}

void CPreferencesSheet::OnDefault()
{
	if (m_PageGeneral.m_hWnd)
		m_PageGeneral.UpdateData();
	if (m_PageDetail.m_hWnd)
		m_PageDetail.UpdateData();
	if (m_PageDrawing.m_hWnd)
		m_PageDrawing.UpdateData();
	if (m_PagePrint.m_hWnd)
		m_PagePrint.UpdateData();
	if (m_PageKeyboard.m_hWnd)
		m_PageKeyboard.UpdateData();

	char str[LC_MAXPATH], st1[256], st2[256];
	int i, j;
	float f;
	unsigned long l;
	unsigned short s1;

	m_PageGeneral.GetOptions(&i, &j, str, st1);
	AfxGetApp()->WriteProfileInt("Settings", "Autosave", i);
	AfxGetApp()->WriteProfileInt("Default", "Mouse", j);
	AfxGetApp()->WriteProfileString("Default", "Projects", str);
	AfxGetApp()->WriteProfileString("Default", "User", st1);
	m_PageDetail.GetOptions(&l, &f);
	AfxGetApp()->WriteProfileInt("Default", "Detail", l);
	AfxGetApp()->WriteProfileInt("Default", "Line", (int)(f*100));
	m_PageDrawing.GetOptions(&l, &s1);
	AfxGetApp()->WriteProfileInt("Default", "Snap", l);
	AfxGetApp()->WriteProfileInt("Default", "Angle", s1);
	m_PageColors.GetOptions();
	m_PagePrint.GetOptions(st1, st2);
	AfxGetApp()->WriteProfileString("Default", "Header", st1);
	AfxGetApp()->WriteProfileString("Default", "Footer", st2);

	String views;
	CMainFrame* Frame = (CMainFrame*)AfxGetMainWnd();
	Frame->GetViewLayout(NULL, views);
	AfxGetApp()->WriteProfileString("Settings", "ViewLayout", views);
}
