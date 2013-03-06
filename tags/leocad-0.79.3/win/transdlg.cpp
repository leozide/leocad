#include "lc_global.h"
#include "leocad.h"
#include "transdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTransformDlg dialog


CTransformDlg::CTransformDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTransformDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTransformDlg)
	m_GlobalX = 0.0f;
	m_GlobalY = 0.0f;
	m_GlobalZ = 0.0f;
	m_OffsetX = 0.0f;
	m_OffsetY = 0.0f;
	m_OffsetZ = 0.0f;
	//}}AFX_DATA_INIT
}


void CTransformDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTransformDlg)
	DDX_Text(pDX, IDC_TRANSDLG_GX, m_GlobalX);
	DDX_Text(pDX, IDC_TRANSDLG_GY, m_GlobalY);
	DDX_Text(pDX, IDC_TRANSDLG_GZ, m_GlobalZ);
	DDX_Text(pDX, IDC_TRANSDLG_OX, m_OffsetX);
	DDX_Text(pDX, IDC_TRANSDLG_OY, m_OffsetY);
	DDX_Text(pDX, IDC_TRANSDLG_OZ, m_OffsetZ);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTransformDlg, CDialog)
	//{{AFX_MSG_MAP(CTransformDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTransformDlg message handlers
