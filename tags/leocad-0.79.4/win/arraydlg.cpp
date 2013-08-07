#include "lc_global.h"
#include "leocad.h"
#include "ArrayDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CArrayDlg dialog


CArrayDlg::CArrayDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CArrayDlg::IDD, pParent)
{
	m_bInitDone = FALSE;

	//{{AFX_DATA_INIT(CArrayDlg)
	m_n1DCount = 10;
	m_n2DCount = 1;
	m_n3DCount = 1;
	m_nArrayDimension = 0;
	m_nTotal = 10;
	m_f2DX = 0.0f;
	m_f2DY = 0.0f;
	m_f2DZ = 0.0f;
	m_f3DX = 0.0f;
	m_f3DY = 0.0f;
	m_f3DZ = 0.0f;
	m_fMoveX = 0.0f;
	m_fMoveY = 0.0f;
	m_fMoveZ = 0.0f;
	m_fRotateX = 0.0f;
	m_fRotateY = 0.0f;
	m_fRotateZ = 0.0f;
	//}}AFX_DATA_INIT
}


void CArrayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CArrayDlg)
	DDX_Text(pDX, IDC_ARRAY_1D_COUNT, m_n1DCount);
	DDX_Text(pDX, IDC_ARRAY_2D_COUNT, m_n2DCount);
	DDX_Text(pDX, IDC_ARRAY_3D_COUNT, m_n3DCount);
	DDX_Radio(pDX, IDC_ARRAY_1D, m_nArrayDimension);
	DDX_Text(pDX, IDC_ARRAY_TOTAL, m_nTotal);
	DDX_Text(pDX, IDC_ARRAY_2D_X, m_f2DX);
	DDX_Text(pDX, IDC_ARRAY_2D_Y, m_f2DY);
	DDX_Text(pDX, IDC_ARRAY_2D_Z, m_f2DZ);
	DDX_Text(pDX, IDC_ARRAY_3D_X, m_f3DX);
	DDX_Text(pDX, IDC_ARRAY_3D_Y, m_f3DY);
	DDX_Text(pDX, IDC_ARRAY_3D_Z, m_f3DZ);
	DDX_Text(pDX, IDC_ARRAY_MOVE_X, m_fMoveX);
	DDX_Text(pDX, IDC_ARRAY_MOVE_Y, m_fMoveY);
	DDX_Text(pDX, IDC_ARRAY_MOVE_Z, m_fMoveZ);
	DDX_Text(pDX, IDC_ARRAY_ROTATE_X, m_fRotateX);
	DDX_Text(pDX, IDC_ARRAY_ROTATE_Y, m_fRotateY);
	DDX_Text(pDX, IDC_ARRAY_ROTATE_Z, m_fRotateZ);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CArrayDlg, CDialog)
	//{{AFX_MSG_MAP(CArrayDlg)
	ON_BN_CLICKED(IDC_ARRAY_1D, OnArrayDimension)
	ON_BN_CLICKED(IDC_ARRAY_2D, OnArrayDimension)
	ON_BN_CLICKED(IDC_ARRAY_3D, OnArrayDimension)
	ON_EN_CHANGE(IDC_ARRAY_1D_COUNT, OnChangeArrayCount)
	ON_EN_CHANGE(IDC_ARRAY_2D_COUNT, OnChangeArrayCount)
	ON_EN_CHANGE(IDC_ARRAY_3D_COUNT, OnChangeArrayCount)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CArrayDlg message handlers

BOOL CArrayDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UINT IDs[15] = { IDC_ARRAY_1D_COUNT_SPIN, IDC_ARRAY_2D_COUNT_SPIN, IDC_ARRAY_3D_COUNT_SPIN,
		IDC_ARRAY_2D_X_SPIN, IDC_ARRAY_2D_Y_SPIN, IDC_ARRAY_2D_Z_SPIN,
		IDC_ARRAY_3D_X_SPIN, IDC_ARRAY_3D_Y_SPIN, IDC_ARRAY_3D_Z_SPIN,
		IDC_ARRAY_MOVE_X_SPIN, IDC_ARRAY_ROTATE_X_SPIN, IDC_ARRAY_MOVE_Y_SPIN,
		IDC_ARRAY_ROTATE_Y_SPIN, IDC_ARRAY_MOVE_Z_SPIN, IDC_ARRAY_ROTATE_Z_SPIN };

	for (int i = 0; i < 3; i++)
		((CSpinButtonCtrl*)GetDlgItem(IDs[i]))->SetRange(1, 1000);
	for (int i = 3; i < 15; i++)
		((CSpinButtonCtrl*)GetDlgItem(IDs[i]))->SetRange(-1000, 1000);

	m_bInitDone = TRUE;
	
	return TRUE;
}

void CArrayDlg::OnArrayDimension() 
{
	if (!IsWindow(m_hWnd))
		return;

	UpdateData();
	for (UINT u = IDC_ARRAY_2D_COUNT; u <= IDC_ARRAY_2D_Z; u++)
		GetDlgItem(u)->EnableWindow(m_nArrayDimension > 0);
	for (UINT u = IDC_ARRAY_2D_X_SPIN; u <= IDC_ARRAY_2D_Z_SPIN; u++)
		GetDlgItem(u)->EnableWindow(m_nArrayDimension > 0);
	for (UINT u = IDC_ARRAY_3D_COUNT; u <= IDC_ARRAY_3D_Z; u++)
		GetDlgItem(u)->EnableWindow(m_nArrayDimension > 1);
	for (UINT u = IDC_ARRAY_3D_X_SPIN; u <= IDC_ARRAY_3D_Z_SPIN; u++)
		GetDlgItem(u)->EnableWindow(m_nArrayDimension > 1);
	OnChangeArrayCount();
}

void CArrayDlg::OnChangeArrayCount()
{
	if (!m_bInitDone)
		return;

	UpdateData();
	m_nTotal = m_n1DCount;
	if (m_nArrayDimension > 0)
		m_nTotal *= m_n2DCount;
	if (m_nArrayDimension > 1)
		m_nTotal *= m_n3DCount;
	UpdateData(FALSE);
}
