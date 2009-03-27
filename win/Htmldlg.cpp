// HTMLDlg.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "HTMLDlg.h"
#include "ImageDlg.h"
#include "Tools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHTMLDlg dialog


CHTMLDlg::CHTMLDlg(void* param, CWnd* pParent /*=NULL*/)
	: CDialog(CHTMLDlg::IDD, pParent)
{
	imopts = param;

	//{{AFX_DATA_INIT(CHTMLDlg)
	m_nLayout = 0;
	m_bIndex = FALSE;
	m_strFolder = _T("");
	m_bImages = FALSE;
	m_bListEnd = FALSE;
	m_bListStep = FALSE;
	m_bHighlight = FALSE;
	m_bHtmlExt = FALSE;
	//}}AFX_DATA_INIT
}


void CHTMLDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHTMLDlg)
	DDX_Radio(pDX, IDC_HTMDLG_SINGLEPAGE, m_nLayout);
	DDX_Check(pDX, IDC_HTMDLG_INDEX, m_bIndex);
	DDX_Text(pDX, IDC_HTMDLG_OUTPUT, m_strFolder);
	DDX_Check(pDX, IDC_HTMDLG_LISTIMAGES, m_bImages);
	DDX_Check(pDX, IDC_HTMDLG_LIST_END, m_bListEnd);
	DDX_Check(pDX, IDC_HTMDLG_LIST_STEP, m_bListStep);
	DDX_Check(pDX, IDC_HTMDLG_HIGHLIGHT, m_bHighlight);
	DDX_Check(pDX, IDC_HTMLDLG_HTMLEXT, m_bHtmlExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHTMLDlg, CDialog)
	//{{AFX_MSG_MAP(CHTMLDlg)
	ON_BN_CLICKED(IDC_HTMDLG_IMAGEOPTIONS, OnImageOptions)
	ON_BN_CLICKED(IDC_HTMDLG_LIST_STEP, OnListClick)
	ON_BN_CLICKED(IDC_HTMDLG_SINGLEPAGE, OnLayoutClick)
	ON_BN_CLICKED(IDC_HTMDLG_BROWSEFOLDER, OnHtmdlgBrowsefolder)
	ON_BN_CLICKED(IDC_HTMDLG_LIST_END, OnListClick)
	ON_BN_CLICKED(IDC_HTMDLG_ONESTEP, OnLayoutClick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHTMLDlg message handlers

void CHTMLDlg::OnImageOptions() 
{
	CImageDlg dlg(TRUE, imopts);
	dlg.DoModal();
}

void CHTMLDlg::OnListClick()
{
	UpdateData();
	GetDlgItem(IDC_HTMDLG_LISTIMAGES)->EnableWindow(m_bListStep || m_bListEnd);
}

void CHTMLDlg::OnLayoutClick() 
{
	UpdateData();
	GetDlgItem(IDC_HTMDLG_INDEX)->EnableWindow(m_nLayout != 0);
}

void CHTMLDlg::OnHtmdlgBrowsefolder() 
{
	CString str;
	if (FolderBrowse(&str, _T("Select Output Folder."), GetSafeHwnd()))
	{
		UpdateData (TRUE);
		m_strFolder = str;
		UpdateData (FALSE);
	}
}

BOOL CHTMLDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GetDlgItem(IDC_HTMDLG_LISTIMAGES)->EnableWindow(m_bListStep || m_bListEnd);
	GetDlgItem(IDC_HTMDLG_INDEX)->EnableWindow(m_nLayout != 0);

	return TRUE;
}
