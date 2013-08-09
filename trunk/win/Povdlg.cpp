#include "lc_global.h"
#include "leocad.h"
#include "POVDlg.h"
#include "tools.h"
#include "resource.hm"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPOVDlg dialog

const DWORD CPOVDlg::m_nHelpIDs[] =
{
	IDC_POVDLG_LGEO,	HIDC_POVDLG_LGEO,
	IDC_POVDLG_OUTPOV,	HIDC_POVDLG_OUTPOV,
	IDC_POVDLG_POVRAY,	HIDC_POVDLG_POVRAY,
	IDC_POVDLG_RENDER,	HIDC_POVDLG_RENDER,
    0,					0
};

CPOVDlg::CPOVDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPOVDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPOVDlg)
	m_bRender = FALSE;
	m_strPOV = _T("");
	m_strOut = _T("");
	m_strLGEO = _T("");
	//}}AFX_DATA_INIT
}

void CPOVDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPOVDlg)
	DDX_Check(pDX, IDC_POVDLG_RENDER, m_bRender);
	DDX_Text(pDX, IDC_POVDLG_POVRAY, m_strPOV);
	DDX_Text(pDX, IDC_POVDLG_OUTPOV, m_strOut);
	DDX_Text(pDX, IDC_POVDLG_LGEO, m_strLGEO);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPOVDlg, CDialog)
	//{{AFX_MSG_MAP(CPOVDlg)
	ON_BN_CLICKED(IDC_POVDLG_POVBROWSE, OnPovbrowse)
	ON_BN_CLICKED(IDC_POVDLG_POVOUTBROWSE, OnPovoutbrowse)
	ON_BN_CLICKED(IDC_POVDLG_LGEOBROWSE, OnPovdlgLgeobrowse)
	ON_WM_CONTEXTMENU()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPOVDlg message handlers

void CPOVDlg::OnOK() 
{
	UpdateData (TRUE);

	if (m_strOut.GetLength() == 0)
		return;

	CDialog::OnOK();
}

void CPOVDlg::OnPovbrowse() 
{
	CFileDialog dlg(TRUE, "*.exe", m_strPOV, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"Executable Files (*.exe)|*.exe|All Files (*.*)|*.*||", this);
	if (dlg.DoModal() == IDOK)
	{
		UpdateData (TRUE);
		m_strPOV = dlg.GetPathName();
		UpdateData (FALSE);
	}
}

void CPOVDlg::OnPovoutbrowse() 
{
	CFileDialog dlg(FALSE, "*.pov", NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"POV-Ray Files (*.pov)|*.pov|All Files (*.*)|*.*||", AfxGetMainWnd());
	if (dlg.DoModal() == IDOK)
	{
		UpdateData (TRUE);
		m_strOut = dlg.GetPathName();
		UpdateData (FALSE);
	}
}

void CPOVDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	::WinHelp(pWnd->m_hWnd, theApp.m_pszHelpFilePath, 
		HELP_CONTEXTMENU, (DWORD)(LPVOID)m_nHelpIDs);
}

BOOL CPOVDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return ::WinHelp((HWND)pHelpInfo->hItemHandle, theApp.m_pszHelpFilePath, 
		HELP_WM_HELP, (DWORD)(LPVOID)m_nHelpIDs);
}

void CPOVDlg::OnPovdlgLgeobrowse() 
{
	CString str;
	if (FolderBrowse(&str, _T("Select LGEO folder"), GetSafeHwnd()))
	{
		UpdateData(TRUE);
		m_strLGEO = str;
		UpdateData(FALSE);
	}
}
