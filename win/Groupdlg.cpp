// GroupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "GroupDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupDlg dialog


CGroupDlg::CGroupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGroupDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGroupDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
}


void CGroupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupDlg)
	DDX_Text(pDX, IDC_GRPDLG_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 64);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupDlg, CDialog)
	//{{AFX_MSG_MAP(CGroupDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupDlg message handlers

void CGroupDlg::OnOK() 
{
	UpdateData(TRUE);
	if (m_strName.GetLength() > 0)
		CDialog::OnOK();
}
