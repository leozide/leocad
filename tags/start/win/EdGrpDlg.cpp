// EdGrpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "EdGrpDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditGroupsDlg dialog


CEditGroupsDlg::CEditGroupsDlg(void* param, CWnd* pParent /*=NULL*/)
	: CDialog(CEditGroupsDlg::IDD, pParent)
{
	m_Tree.opts = (LC_GROUPEDITDLG_OPTS*)param;

	//{{AFX_DATA_INIT(CEditGroupsDlg)
	//}}AFX_DATA_INIT
}


void CEditGroupsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditGroupsDlg)
	DDX_Control(pDX, IDC_TREE, m_Tree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditGroupsDlg, CDialog)
	//{{AFX_MSG_MAP(CEditGroupsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditGroupsDlg message handlers


BOOL CEditGroupsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_TreeImages.Create(IDB_PARTICONS, 16, 0, RGB (0,128,128));
	m_Tree.SetImageList(&m_TreeImages, TVSIL_NORMAL);

	m_Tree.DeleteAllItems();
	m_Tree.AddChildren(NULL, NULL);
	
	return TRUE;
}
