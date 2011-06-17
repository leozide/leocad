// categdlg.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "categdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCategoryDlg dialog


CCategoryDlg::CCategoryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCategoryDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCategoryDlg)
	m_Keywords = _T("");
	m_Name = _T("");
	//}}AFX_DATA_INIT
}


void CCategoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCategoryDlg)
	DDX_Text(pDX, IDC_CATDLG_KEYWORDS, m_Keywords);
	DDX_Text(pDX, IDC_CATDLG_NAME, m_Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCategoryDlg, CDialog)
	//{{AFX_MSG_MAP(CCategoryDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCategoryDlg message handlers
