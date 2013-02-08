#include "lc_global.h"
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
	m_bID = FALSE;
	m_bListEnd = FALSE;
	m_bListStep = FALSE;
	m_bHighlight = FALSE;
	//}}AFX_DATA_INIT
}


void CHTMLDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHTMLDlg)
	DDX_Radio(pDX, IDC_HTMDLG_SINGLEPAGE, m_nLayout);
	DDX_Check(pDX, IDC_HTMDLG_INDEX, m_bIndex);
	DDX_Text(pDX, IDC_HTMDLG_OUTPUT, m_strFolder);
	DDX_Check(pDX, IDC_HTMDLG_LIST_IMAGES, m_bImages);
	DDX_Check(pDX, IDC_HTMDLG_LIST_ID, m_bID);
	DDX_Check(pDX, IDC_HTMDLG_LIST_END, m_bListEnd);
	DDX_Check(pDX, IDC_HTMDLG_LIST_STEP, m_bListStep);
	DDX_Control(pDX, IDC_HTMLDLG_LIST_COLOR, m_clrList);
	DDX_Check(pDX, IDC_HTMDLG_HIGHLIGHT, m_bHighlight);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHTMLDlg, CDialog)
	//{{AFX_MSG_MAP(CHTMLDlg)
	ON_BN_CLICKED(IDC_HTMDLG_IMAGEOPTIONS, OnImageOptions)
	ON_BN_CLICKED(IDC_HTMDLG_LIST_STEP, OnListClick)
	ON_BN_CLICKED(IDC_HTMDLG_SINGLEPAGE, OnLayoutClick)
	ON_BN_CLICKED(IDC_HTMDLG_BROWSEFOLDER, OnHtmdlgBrowsefolder)
	ON_BN_CLICKED(IDC_HTMDLG_LIST_END, OnListClick)
	ON_BN_CLICKED(IDC_HTMDLG_LIST_IMAGES, OnListClick)
	ON_BN_CLICKED(IDC_HTMDLG_ONESTEP, OnLayoutClick)
	//}}AFX_MSG_MAP
	ON_MESSAGE(CPN_SELENDOK, OnColorSelEndOK)
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
	bool Update = false;

	UpdateData();

	if (!m_bListStep && !m_bListEnd && m_bImages)
	{
		m_bImages = FALSE;
		Update = true;
	};

	GetDlgItem(IDC_HTMDLG_LIST_IMAGES)->EnableWindow(m_bListStep || m_bListEnd);
	GetDlgItem(IDC_HTMDLG_LIST_COLOR)->EnableWindow((m_bListStep || m_bListEnd) && m_bImages);
	GetDlgItem(IDC_HTMDLG_LIST_ID)->EnableWindow(m_bListStep || m_bListEnd);

	if (Update)
		UpdateData(FALSE);
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
	
	if (!m_bListStep && !m_bListEnd)
		m_bImages = FALSE;

	GetDlgItem(IDC_HTMDLG_LIST_IMAGES)->EnableWindow(m_bListStep || m_bListEnd);
	GetDlgItem(IDC_HTMDLG_LIST_COLOR)->EnableWindow((m_bListStep || m_bListEnd) && m_bImages);
	GetDlgItem(IDC_HTMDLG_LIST_ID)->EnableWindow(m_bListStep || m_bListEnd);

	GetDlgItem(IDC_HTMDLG_INDEX)->EnableWindow(m_nLayout != 0);

	m_clrList.SetColorIndex(mColorIndex);

	return TRUE;
}

LONG CHTMLDlg::OnColorSelEndOK(UINT lParam, LONG wParam)
{
	mColorIndex = lParam;

	return TRUE;
}
